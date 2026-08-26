import { t } from "./i18n";

type WebChannel<T> = { onmessage: (message: T) => void };

type WorkerEvent =
  | { id: number; type: "line"; line: string }
  | { id: number; type: "result"; result: unknown }
  | { id: number; type: "error"; message: string };

type PendingRequest = {
  resolve: (value: unknown) => void;
  reject: (reason?: unknown) => void;
  onLine?: WebChannel<string>;
  solve?: boolean;
};

let nextId = 1;
let solveWorker: Worker | undefined;
let utilityWorker: Worker | undefined;
const pending = new Map<number, PendingRequest>();

// remember the last config sent so it can be relayed into the solve worker too
let lastRatingConfig: Record<string, unknown> | undefined;

class Channel<T> {
  onmessage: (message: T) => void = () => undefined;
}

function attachWorkerEvents(worker: Worker) {
  worker.onmessage = (event: MessageEvent<WorkerEvent>) => {
    const request = pending.get(event.data.id);
    if (!request) return;
    if (event.data.type === "line") {
      request.onLine?.onmessage(event.data.line);
      return;
    }
    pending.delete(event.data.id);
    if (event.data.type === "error") request.reject(new Error(event.data.message));
    else request.resolve(event.data.result);
  };
  worker.onerror = (event) => {
    const error = new Error(event.message || t('status.workerFailed'));
    for (const [id, request] of pending) {
      pending.delete(id);
      request.reject(error);
    }
  };
}

function ensureSolveWorker() {
  if (solveWorker) return solveWorker;
  solveWorker = new Worker(new URL("./solverWorker.ts", import.meta.url), { type: "module" });
  attachWorkerEvents(solveWorker);
  if (lastRatingConfig) postInvoke(solveWorker, "set_rating_config", lastRatingConfig);
  return solveWorker;
}

function ensureUtilityWorker() {
  if (utilityWorker) return utilityWorker;
  utilityWorker = new Worker(new URL("./solverWorker.ts", import.meta.url), { type: "module" });
  attachWorkerEvents(utilityWorker);
  return utilityWorker;
}

function rejectMatchingRequests(predicate: (request: PendingRequest) => boolean, error: Error) {
  for (const [id, request] of pending) {
    if (!predicate(request)) continue;
    pending.delete(id);
    if (request.solve) request.resolve({ code: null, stdout: "", stderr: "" });
    else request.reject(error);
  }
}

function postInvoke<T>(worker: Worker, command: string, args: Record<string, unknown>): Promise<T> {
  const id = nextId++;
  const request: PendingRequest = {
    resolve: (value) => undefined,
    reject: () => undefined,
    onLine: args.onLine as WebChannel<string> | undefined,
  };
  const promise = new Promise<T>((resolve, reject) => {
    request.resolve = (value) => resolve(value as T);
    request.reject = reject;
  });
  pending.set(id, request);
  worker.postMessage({ id, type: "invoke", command, args });
  return promise;
}

function invoke<T>(command: string, args: Record<string, unknown> = {}): Promise<T> {
  if (command === "stop_solver") {
    solveWorker?.terminate();
    solveWorker = undefined;
    rejectMatchingRequests((request) => !!request.solve, new Error(t('status.workerStopped')));
    return Promise.resolve(undefined as T);
  }

  if (command === "set_rating_config") {
    lastRatingConfig = args;
    const targets = [ensureUtilityWorker()];
    if (solveWorker) targets.push(solveWorker);
    return Promise.all(targets.map((worker) => postInvoke<unknown>(worker, command, args))).then(() => undefined as T);
  }

  if (command === "solve") {
    const id = nextId++;
    const request: PendingRequest = {
      resolve: (value) => undefined,
      reject: () => undefined,
      onLine: args.onLine as WebChannel<string> | undefined,
      solve: true,
    };
    const promise = new Promise<T>((resolve, reject) => {
      request.resolve = (value) => resolve(value as T);
      request.reject = reject;
    });
    pending.set(id, request);
    ensureSolveWorker().postMessage({ id, type: "solve", position: args.position, flags: args.flags || [] });
    return promise;
  }

  if (command === "batch_init") {
    const id = nextId++;
    const request: PendingRequest = {
      resolve: (value) => undefined,
      reject: () => undefined,
      solve: true,
    };
    const promise = new Promise<T>((resolve, reject) => {
      request.resolve = (value) => resolve(value as T);
      request.reject = reject;
    });
    pending.set(id, request);
    ensureSolveWorker().postMessage({ id, type: "batchInit", flags: args.flags || [] });
    return promise;
  }

  if (command === "batch_solve_position") {
    const id = nextId++;
    const request: PendingRequest = {
      resolve: (value) => undefined,
      reject: () => undefined,
      onLine: args.onLine as WebChannel<string> | undefined,
      solve: true,
    };
    const promise = new Promise<T>((resolve, reject) => {
      request.resolve = (value) => resolve(value as T);
      request.reject = reject;
    });
    pending.set(id, request);
    ensureSolveWorker().postMessage({ id, type: "batchSolvePosition", position: args.position });
    return promise;
  }

  if (command === "batch_destroy") {
    const id = nextId++;
    const request: PendingRequest = {
      resolve: (value) => undefined,
      reject: () => undefined,
      solve: true,
    };
    const promise = new Promise<T>((resolve, reject) => {
      request.resolve = (value) => resolve(value as T);
      request.reject = reject;
    });
    pending.set(id, request);
    ensureSolveWorker().postMessage({ id, type: "batchDestroy" });
    return promise;
  }

  return postInvoke<T>(ensureUtilityWorker(), command, args);
}

export function createWebNative() {
  return {
    core: { invoke },
    event: { listen: async () => () => undefined },
    Channel,
  };
}

// Removes a table from the in-memory WASM filesystem of the live solve worker
// (if any).  Used together with the IndexedDB deletion so the table does not
// get re-persisted on the next solve.
export function deleteSolverTable(name: string): void {
  if (!solveWorker) return;
  solveWorker.postMessage({ id: 0, type: "deleteTable", name });
}
