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
  | { id: number; type: "batchSolve"; positions: string[]; flags: string[] }
  | { id: number; type: "batchInit"; flags: string[] }
  | { id: number; type: "batchSolvePosition"; position: string }
  | { id: number; type: "batchSolveMulti"; candidates: string[] }
  | { id: number; type: "batchDestroy" }
  | { id: number; type: "invoke"; command: "unkarnify"; args: { input: string } }
  | { id: number; type: "invoke"; command: "karnify"; args: { input: string; position?: string | null; generator: boolean } }
  | { id: number; type: "invoke"; command: "rate_algorithm"; args: { algorithm: string; initialTopA: boolean } }
  | { id: number; type: "invoke"; command: "two_gen_status"; args: { position: number[]; specificAngleBot: boolean } }
  | { id: number; type: "invoke"; command: "set_rating_config"; args: { weights: [number, number, number, number]; moveValues: Record<string, number> } }
  | { id: number; type: "deleteTable"; name: string };

type WorkerEvent =
  | { id: number; type: "line"; line: string }
  | { id: number; type: "result"; result: unknown }
  | { id: number; type: "error"; message: string };

type EmscriptenModule = {
  FS: {
    mkdir: (path: string) => void;
    writeFile: (path: string, data: Uint8Array) => void;
    readdir: (path: string) => string[];
    readFile: (path: string) => Uint8Array;
    analyzePath: (path: string) => { exists: boolean };
    stat: (path: string) => { size: number };
    unlink: (path: string) => void;
  };
  HEAP32: Int32Array;
  HEAPU8: Uint8Array;
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
  twoGenStatus: (position: number, specificAngleBot: number) => number;
  setRatingWeights: (w1: number, w2: number, w3: number, w4: number) => void;
  setMoveValue: (key: string, value: number) => number;
  resetRatingConfig: () => void;
  freeString: (ptr: number) => void;
  batchInit: (argc: number, argv: number) => number;
  batchSolve: (position: string) => number;
  batchSolveMulti: (argc: number, argv: number) => number;
  batchDestroy: () => void;
};

let modulePromise: Promise<EmscriptenModule> | undefined;
let moduleInstance: EmscriptenModule | undefined;
let api: WasmApi | undefined;
let activeSolveId: number | undefined;

const emit = (event: WorkerEvent) => self.postMessage(event);

const TABLE_DIR = "/tables";

async function restorePersistedTables(mod: EmscriptenModule) {
  const { listTableEntries, loadTableBlob } = await import("./tableStore");
  const entries = await listTableEntries();
  for (const entry of entries) {
    const blob = await loadTableBlob(entry.name);
    if (!blob) continue;
    try {
      if (!mod.FS.analyzePath(TABLE_DIR).exists) mod.FS.mkdir(TABLE_DIR);
      mod.FS.writeFile(`${TABLE_DIR}/${entry.name}`, new Uint8Array(blob));
    } catch { /* skip unreadable blob */ }
  }
}

async function syncTablesToIdb(mod: EmscriptenModule) {
  const { storeTableBlob, listTableEntries } = await import("./tableStore");
  let names: string[] = [];
  const sizes = new Map<string, number>();
  try {
    names = mod.FS.readdir(TABLE_DIR).filter((name) => name.endsWith(".dat"));
    for (const name of names) sizes.set(name, mod.FS.stat(`${TABLE_DIR}/${name}`).size);
  } catch { return; }
  const stored = await listTableEntries();
  const storedSizes = new Map(stored.map((entry) => [entry.name, entry.size]));
  for (const name of names) {
    if (storedSizes.get(name) === sizes.get(name)) continue;
    try {
      const data = mod.FS.readFile(`${TABLE_DIR}/${name}`);
      const bytes = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength) as ArrayBuffer;
      await storeTableBlob(name, bytes);
    } catch { /* skip unreadable table */ }
  }
}

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
      // stderr (printErr) is intentionally NOT forwarded as solution lines:
      // real solutions are always written to stdout, whereas C++ debug/status
      // traces go to stderr — forwarding those would corrupt solution parsing
      // (e.g. a line like "candidate[0] SOLVED at depth 4" contains brackets,
      // so a consumer looking for "[counts]" would misparse it).
      printErr: () => undefined,
      locateFile: (file: string) => new URL(`../wasm/${file}`, self.location.href).href,
    });
    try {
      instance.FS.mkdir(TABLE_DIR);
    } catch {
      // The module can be reused for the whole tab session.
    }
    instance.cwrap("sq1opt_web_set_table_directory", null, ["string"])(TABLE_DIR);
    await restorePersistedTables(instance);
    moduleInstance = instance;
    api = {
      unkarnify: instance.cwrap("sq1_web_unkarnify_alloc", "number", ["string"]) as (input: string) => number,
      karnify: instance.cwrap("sq1_web_karnify_alloc", "number", ["string", "string", "number"]) as (input: string, position: string, generator: number) => number,
      rateAlgorithm: instance.cwrap("sq1_web_rate_algorithm_json_alloc", "number", ["string", "number"]) as (algorithm: string, initialTopA: number) => number,
      twoGenStatus: instance.cwrap("sq1_web_two_gen_status_json_alloc", "number", ["number", "number"]) as (position: number, specificAngleBot: number) => number,
      setRatingWeights: instance.cwrap("sq1_web_set_rating_weights", null, ["number", "number", "number", "number"]) as (w1: number, w2: number, w3: number, w4: number) => void,
      setMoveValue: instance.cwrap("sq1_web_set_move_value", "number", ["string", "number"]) as (key: string, value: number) => number,
      resetRatingConfig: instance.cwrap("sq1_web_reset_rating_config", null, []) as () => void,
      freeString: instance.cwrap("sq1_web_free_string", null, ["number"]) as (ptr: number) => void,
      batchInit: instance.cwrap("sq1_web_batch_init", "number", ["number", "number"]) as (argc: number, argv: number) => number,
      batchSolve: instance.cwrap("sq1_web_batch_solve", "number", ["string"]) as (position: string) => number,
      batchSolveMulti: instance.cwrap("sq1_web_batch_solve_multi", "number", ["number", "number"]) as (argc: number, argv: number) => number,
      batchDestroy: instance.cwrap("sq1_web_batch_destroy", null, []) as () => void,
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
    const pos = command.args.position;
    if (!pos || !Array.isArray(pos)) throw new Error("Invalid position argument for two_gen_status");
    const ptr = mod._malloc(24 * 4);
    try {
      mod.HEAP32.set(pos.slice(0, 24), ptr >> 2);
      return readJson<TwoGenStatus>(api.twoGenStatus(ptr, command.args.specificAngleBot ? 1 : 0));
    } finally {
      mod._free(ptr);
    }
  }
  if (command.command === "set_rating_config") {
    api.resetRatingConfig();
    const [w1, w2, w3, w4] = command.args.weights;
    api.setRatingWeights(w1, w2, w3, w4);
    for (const [key, value] of Object.entries(command.args.moveValues)) api.setMoveValue(key, value);
    return null;
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
  await syncTablesToIdb(mod);
}

async function batchSolve(request: InvokeRequest & { type: "batchSolve" }) {
  const mod = await loadModule();
  if (!api) throw new Error("The Square-1 WASM module is not ready.");
  activeSolveId = request.id;

  try {
    // Build argv for batchInit: ["sq1opt", "-v5", ...flags]
    const argv = ["sq1opt", "-v1", ...request.flags];
    const encoded = argv.map((s) => {
      const bytes = new TextEncoder().encode(s + "\0");
      const ptr = mod._malloc(bytes.length);
      mod.HEAPU8.set(bytes, ptr);
      return ptr;
    });
    const argvPtr = mod._malloc(encoded.length * 4);
    for (let i = 0; i < encoded.length; i++) {
      mod.HEAP32[(argvPtr >> 2) + i] = encoded[i];
    }

    try {
      const initCode = api.batchInit(encoded.length, argvPtr);
      if (initCode !== 0) {
        emit({ id: request.id, type: "result", result: { code: initCode, stdout: "", stderr: "Batch init failed" } });
        return;
      }

      for (let i = 0; i < request.positions.length; i++) {
        if (activeSolveId !== request.id) break; // stopped
        api.batchSolve(request.positions[i]);
      }

      api.batchDestroy();
    } finally {
      for (const ptr of encoded) mod._free(ptr);
      mod._free(argvPtr);
    }

    emit({ id: request.id, type: "result", result: { code: 0, stdout: "", stderr: "" } });
  } finally {
    activeSolveId = undefined;
  }
  await syncTablesToIdb(mod);
}

let batchAllocatedPtrs: number[] = [];

async function handleBatchInit(request: InvokeRequest & { type: "batchInit" }) {
  const mod = await loadModule();
  if (!api) throw new Error("The Square-1 WASM module is not ready.");

  const argv = ["sq1opt", "-v1", ...request.flags];
  const encoded = argv.map((s) => {
    const bytes = new TextEncoder().encode(s + "\0");
    const ptr = mod._malloc(bytes.length);
    mod.HEAPU8.set(bytes, ptr);
    return ptr;
  });
  const argvPtr = mod._malloc(encoded.length * 4);
  for (let i = 0; i < encoded.length; i++) {
    mod.HEAP32[(argvPtr >> 2) + i] = encoded[i];
  }

  batchAllocatedPtrs = [...encoded, argvPtr];

  const code = api.batchInit(encoded.length, argvPtr);
  if (code !== 0) {
    emit({ id: request.id, type: "result", result: { code } });
    return;
  }
  emit({ id: request.id, type: "result", result: { code: 0 } });
}

async function handleBatchSolvePosition(request: InvokeRequest & { type: "batchSolvePosition" }) {
  const mod = await loadModule();
  if (!api) throw new Error("The Square-1 WASM module is not ready.");
  activeSolveId = request.id;
  try {
    api.batchSolve(request.position);
    emit({ id: request.id, type: "result", result: { code: 0 } });
  } finally {
    activeSolveId = undefined;
  }
}

async function handleBatchSolveMulti(request: InvokeRequest & { type: "batchSolveMulti" }) {
  const mod = await loadModule();
  if (!api) throw new Error("The Square-1 WASM module is not ready.");
  activeSolveId = request.id;
  try {
    const encoded = request.candidates.map((s) => {
      const bytes = new TextEncoder().encode(s + "\0");
      const ptr = mod._malloc(bytes.length);
      mod.HEAPU8.set(bytes, ptr);
      return ptr;
    });
    const argvPtr = mod._malloc(encoded.length * 4);
    for (let i = 0; i < encoded.length; i++) {
      mod.HEAP32[(argvPtr >> 2) + i] = encoded[i];
    }
    try {
      api.batchSolveMulti(encoded.length, argvPtr);
    } finally {
      for (const ptr of encoded) mod._free(ptr);
      mod._free(argvPtr);
    }
    emit({ id: request.id, type: "result", result: { code: 0 } });
  } finally {
    activeSolveId = undefined;
  }
}

async function handleBatchDestroy(request: InvokeRequest & { type: "batchDestroy" }) {
  const mod = await loadModule();
  if (!api) throw new Error("The Square-1 WASM module is not ready.");
  try {
    api.batchDestroy();
  } finally {
    for (const ptr of batchAllocatedPtrs) mod._free(ptr);
    batchAllocatedPtrs = [];
  }
  emit({ id: request.id, type: "result", result: { code: 0 } });
  await syncTablesToIdb(mod);
}

self.onmessage = (event: MessageEvent<InvokeRequest>) => {
  const request = event.data;
  void (async () => {
    try {
      if (request.type === "solve") await solve(request);
      else if (request.type === "batchSolve") await batchSolve(request);
      else if (request.type === "batchInit") await handleBatchInit(request);
      else if (request.type === "batchSolvePosition") await handleBatchSolvePosition(request);
      else if (request.type === "batchSolveMulti") await handleBatchSolveMulti(request);
      else if (request.type === "batchDestroy") await handleBatchDestroy(request);
      else if (request.type === "deleteTable") {
        if (activeSolveId === undefined && moduleInstance) {
          try { moduleInstance.FS.unlink(`${TABLE_DIR}/${request.name}`); } catch { /* not present */ }
        }
      }
      else emit({ id: request.id, type: "result", result: await invoke(request) });
    } catch (error) {
      emit({ id: request.id, type: "error", message: error instanceof Error ? error.message : String(error) });
    }
  })();
};
