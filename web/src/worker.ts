/* Vite emits this as a separate worker chunk. The Emscripten module is loaded
 * inside the worker, keeping both table construction and IDBFS sync off the UI. */
import type { SolverRequest, SolverEvent } from './solverWorker';

type EmscriptenModule = { FS: any; callMain: (args: string[]) => number; cwrap: (name: string, returnType: string | null, argTypes: string[]) => (...args: any[]) => any; print: (line: string) => void; printErr: (line: string) => void };
let moduleInstance: EmscriptenModule | undefined;
let stopped = false;

const emit = (event: SolverEvent) => self.postMessage(event);

async function loadModule(): Promise<EmscriptenModule> {
  if (moduleInstance) return moduleInstance;
  const moduleUrl = new URL('wasm/sq1opt.js', self.location.href).href;
  const factory = (await import(/* @vite-ignore */ moduleUrl)).default;
  const instance = await factory({
    print: (line: string) => emit({ type: 'line', line }),
    printErr: (line: string) => emit({ type: 'line', line }),
    locateFile: (file: string) => new URL(`wasm/${file}`, self.location.href).href
  }) as EmscriptenModule;
  moduleInstance = instance;
  const FS = instance.FS;
  try { FS.mkdir('/tables'); } catch (_) { /* already present */ }
  FS.mount(FS.filesystems.IDBFS, {}, '/tables');
  await new Promise<void>((resolve, reject) => FS.syncfs(true, (error: Error | null) => error ? reject(error) : resolve()));
  instance.cwrap('sq1opt_web_set_table_directory', null, ['string'])('/tables');
  emit({ type: 'ready' });
  return instance;
}

self.onmessage = async (event: MessageEvent<SolverRequest>) => {
  if (event.data.type === 'stop') { stopped = true; return; }
  stopped = false;
  try {
    const mod = await loadModule();
    if (stopped) return;
    const code = mod.callMain(event.data.args);
    await new Promise<void>((resolve, reject) => mod.FS.syncfs(false, (error: Error | null) => error ? reject(error) : resolve()));
    emit({ type: 'done', code });
  } catch (error) {
    emit({ type: 'error', message: error instanceof Error ? error.message : String(error) });
  }
};
