/// <reference lib="webworker" />

type RatingResult = {
  finalScore: number;
  phase1: number;
  phase2: number;
  phase3: number;
  phase4: number;
  ergoUp: number;
  ergoDown: number;
  sliceCount: number;
  movement: number;
  bonus: number;
  valid: boolean;
  sliceStart: number;
};

type TwoGenStatus = {
  compatibility: number;
  cornersTwo: boolean;
  cornersPseudo: boolean;
};

type InvokeRequest =
  | { id: number; type: "solve"; position: string; flags: string[] }
  | { id: number; type: "invoke"; command: "unkarnify"; args: { input: string } }
  | { id: number; type: "invoke"; command: "karnify"; args: { input: string; position?: string | null; generator: boolean } }
  | { id: number; type: "invoke"; command: "rate_algorithm"; args: { algorithm: string; initialTopA: boolean } }
  | { id: number; type: "invoke"; command: "two_gen_status"; args: { position: number[] } };

type WorkerEvent =
  | { id: number; type: "line"; line: string }
  | { id: number; type: "result"; result: unknown }
  | { id: number; type: "error"; message: string };

type EmscriptenModule = {
  FS: { mkdir: (path: string) => void };
  HEAP32: Int32Array;
  UTF8ToString: (ptr: number) => string;
  callMain: (args: string[]) => number;
  cwrap: (name: string, returnType: "number" | null, argTypes: string[]) => (...args: unknown[]) => number | void;
  _malloc: (size: number) => number;
  _free: (ptr: number) => void;
};

type WasmApi = {
  unkarnify: (input: string) => number;
  karnify: (input: string, position: string, generator: number) => number;
  rateAlgorithm: (algorithm: string, initialTopA: number) => number;
  twoGenStatus: (position: number) => number;
  freeString: (ptr: number) => void;
};

let modulePromise: Promise<EmscriptenModule> | undefined;
let moduleInstance: EmscriptenModule | undefined;
let api: WasmApi | undefined;
let activeSolveId: number | undefined;

const emit = (event: WorkerEvent) => self.postMessage(event);

async function loadModule(): Promise<EmscriptenModule> {
  if (modulePromise) return modulePromise;
  modulePromise = (async () => {
    const moduleUrl = new URL("../wasm/sq1opt.js", self.location.href).href;
    const factory = (await import(/* @vite-ignore */ moduleUrl)).default as (options: Record<string, unknown>) => Promise<EmscriptenModule>;
    const instance = await factory({
      noInitialRun: true,
      print: (line: string) => {
        if (activeSolveId !== undefined) {
          emit({ id: activeSolveId, type: "line", line });
        }
      },
      printErr: (line: string) => {
        if (activeSolveId !== undefined) {
          emit({ id: activeSolveId, type: "line", line });
        }
      },
      locateFile: (file: string) => new URL(`../wasm/${file}`, self.location.href).href,
    });
    try {
      instance.FS.mkdir("/tables");
    } catch {
      // The module can be reused for the whole tab session.
    }
    instance.cwrap("sq1opt_web_set_table_directory", null, ["string"])("/tables");
    moduleInstance = instance;
    api = {
      unkarnify: instance.cwrap("sq1_web_unkarnify_alloc", "number", ["string"]) as (input: string) => number,
      karnify: instance.cwrap("sq1_web_karnify_alloc", "number", ["string", "string", "number"]) as (input: string, position: string, generator: number) => number,
      rateAlgorithm: instance.cwrap("sq1_web_rate_algorithm_json_alloc", "number", ["string", "number"]) as (algorithm: string, initialTopA: number) => number,
      twoGenStatus: instance.cwrap("sq1_web_two_gen_status_json_alloc", "number", ["number"]) as (position: number) => number,
      freeString: instance.cwrap("sq1_web_free_string", null, ["number"]) as (ptr: number) => void,
    };
    return instance;
  })();
  return modulePromise;
}

function takeString(ptr: number): string {
  if (!moduleInstance || !api || !ptr) return "";
  const value = moduleInstance.UTF8ToString(ptr);
  api.freeString(ptr);
  return value;
}

function readJson<T>(ptr: number): T {
  return JSON.parse(takeString(ptr)) as T;
}

async function invoke(command: InvokeRequest & { type: "invoke" }): Promise<unknown> {
  const mod = await loadModule();
  if (!api) throw new Error("The Square-1 WASM module is not ready.");
  if (command.command === "unkarnify") return takeString(api.unkarnify(command.args.input));
  if (command.command === "karnify") {
    return takeString(api.karnify(command.args.input, command.args.position || "", command.args.generator ? 1 : 0));
  }
  if (command.command === "rate_algorithm") {
    return readJson<RatingResult>(api.rateAlgorithm(command.args.algorithm, command.args.initialTopA ? 1 : 0));
  }
  if (command.command === "two_gen_status") {
    const ptr = mod._malloc(24 * 4);
    try {
      mod.HEAP32.set(command.args.position.slice(0, 24), ptr >> 2);
      return readJson<TwoGenStatus>(api.twoGenStatus(ptr));
    } finally {
      mod._free(ptr);
    }
  }
  throw new Error("Unsupported command.");
}

async function solve(request: InvokeRequest & { type: "solve" }) {
  const mod = await loadModule();
  activeSolveId = request.id;
  try {
    const code = mod.callMain(["-v5", ...request.flags, request.position]);
    emit({ id: request.id, type: "result", result: { code, stdout: "", stderr: "" } });
  } finally {
    activeSolveId = undefined;
  }
}

self.onmessage = (event: MessageEvent<InvokeRequest>) => {
  const request = event.data;
  void (async () => {
    try {
      if (request.type === "solve") await solve(request);
      else emit({ id: request.id, type: "result", result: await invoke(request) });
    } catch (error) {
      emit({ id: request.id, type: "error", message: error instanceof Error ? error.message : String(error) });
    }
  })();
};
