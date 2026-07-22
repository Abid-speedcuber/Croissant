export type SolverRequest = { type: 'solve'; args: string[] } | { type: 'stop' };
export type SolverEvent = { type: 'ready' } | { type: 'line'; line: string } | { type: 'done'; code: number } | { type: 'error'; message: string };

let active: Worker | undefined;

export function runSolver(args: string[], onEvent: (event: SolverEvent) => void): { stop: () => void } {
  active?.terminate();
  active = new Worker(new URL('./worker.ts', import.meta.url), { type: 'module' });
  const worker = active;
  worker.onmessage = (event: MessageEvent<SolverEvent>) => onEvent(event.data);
  worker.onerror = (event) => onEvent({ type: 'error', message: event.message || 'WASM worker failed.' });
  worker.postMessage({ type: 'solve', args } satisfies SolverRequest);
  return { stop: () => { worker.postMessage({ type: 'stop' } satisfies SolverRequest); worker.terminate(); active = undefined; } };
}
