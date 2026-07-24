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
    const error = new Error(event.message || "The Square-1 WASM worker failed.");
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

function invoke<T>(command: string, args: Record<string, unknown> = {}): Promise<T> {
  if (command === "stop_solver") {
    solveWorker?.terminate();
    solveWorker = undefined;
    rejectMatchingRequests((request) => !!request.solve, new Error("The solver worker was stopped."));
    return Promise.resolve(undefined as T);
  }

  const id = nextId++;
  const request: PendingRequest = {
    resolve: (value) => undefined,
    reject: () => undefined,
    onLine: args.onLine as WebChannel<string> | undefined,
    solve: command === "solve",
  };
  const promise = new Promise<T>((resolve, reject) => {
    request.resolve = (value) => resolve(value as T);
    request.reject = reject;
  });
  pending.set(id, request);

  if (command === "solve") {
    ensureSolveWorker().postMessage({ id, type: "solve", position: args.position, flags: args.flags || [] });
  } else {
    ensureUtilityWorker().postMessage({ id, type: "invoke", command, args });
  }
  return promise;
}

export function createWebNative() {
  return {
    core: { invoke },
    event: { listen: async () => () => undefined },
    Channel,
  };
}
