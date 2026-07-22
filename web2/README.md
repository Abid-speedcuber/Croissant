# sq1opt browser proof of concept

This is a one-to-one browser shell around `../sq1opt/sq1opt.cpp`. The C++ solver is compiled with Emscripten and loaded in `src/worker.ts`. The worker mounts `/tables` with Emscripten IDBFS, calls `FS.syncfs(true)` before solving and `FS.syncfs(false)` afterwards, so the pruning files survive reloads in IndexedDB.

```sh
npm install
npm run build:wasm   # requires em++ / Emscripten
npm run dev
```

`npm run build` validates/builds the React shell. `build:wasm` writes `public/wasm/sq1opt.js` and `sq1opt.wasm`; those generated binaries are intentionally ignored by git. The UI passes the same solver switches and position string to the C++ command-line entry point and streams its existing output from the dedicated worker.
