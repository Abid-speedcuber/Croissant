import { spawnSync } from "node:child_process";
import { existsSync, mkdirSync } from "node:fs";
import { homedir } from "node:os";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = dirname(fileURLToPath(import.meta.url));
const main = resolve(root, "main");
const outDir = process.env.BUILD_OUT_DIR
  ? resolve(process.env.BUILD_OUT_DIR)
  : resolve(root, "public");
const wasmOut = resolve(main, "public/wasm");
const localEmsdk = resolve(homedir(), ".local/share/emsdk");
const localEmscripten = resolve(localEmsdk, "upstream/emscripten");
const localEmsdkNode = resolve(localEmsdk, "node/22.16.0_64bit/bin/node");

if (existsSync(resolve(localEmscripten, "emcc"))) {
  process.env.EMSDK ??= localEmsdk;
  if (existsSync(localEmsdkNode)) process.env.EMSDK_NODE ??= localEmsdkNode;
  process.env.PATH = [localEmsdk, localEmscripten, process.env.PATH].filter(Boolean).join(":");
}
function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: options.cwd || root,
    env: { ...process.env, ...(options.env || {}) },
    stdio: "inherit",
    shell: process.platform === "win32",
  });
  if (result.status !== 0) {
    process.exit(result.status || 1);
  }
}

function requireTool(command, installHint) {
  const result = spawnSync(command, ["--version"], {
    cwd: root,
    stdio: "ignore",
    shell: process.platform === "win32",
  });
  if (result.status !== 0) {
    console.error(`${command} is required to build the web version.`);
    console.error(installHint);
    process.exit(1);
  }
}

requireTool("em++", "Install and activate Emscripten, then rerun: node build.mjs");

const iconScript = resolve(root, "icon.sh");
if (existsSync(iconScript)) {
  run("bash", [iconScript, "--web"]);
}

mkdirSync(wasmOut, { recursive: true });

run("em++", [
  "src-tauri/native/sq1opt.cpp",
  "src-tauri/native/sq1-logic.cpp",
  "src-tauri/native/web_bridge.cpp",
  "-Isrc-tauri/native",
  "-std=c++17",
  "-O3",
  "-DSQ1OPT_NO_QT",
  "-sMODULARIZE=1",
  "-sEXPORT_ES6=1",
  "-sENVIRONMENT=worker",
  "-sINVOKE_RUN=0",
  "-sEXIT_RUNTIME=0",
  "-sALLOW_MEMORY_GROWTH=1",
  "-sINITIAL_MEMORY=268435456",
  "-sEXPORTED_FUNCTIONS=['_main','_sq1opt_web_set_table_directory','_sq1opt_web_request_stop','_sq1_web_unkarnify_alloc','_sq1_web_karnify_alloc','_sq1_web_rate_algorithm_json_alloc','_sq1_web_two_gen_status_json_alloc','_sq1_web_free_string','_malloc','_free']",
  "-sEXPORTED_RUNTIME_METHODS=['callMain','cwrap','UTF8ToString','FS','HEAP32']",
  "-o",
  resolve(wasmOut, "sq1opt.js"),
], { cwd: main });

run("npx", [
  "vite",
  "build",
  "--mode",
  "web",
  "--outDir",
  outDir,
  "--emptyOutDir",
], {
  cwd: main,
  env: { VITE_SQ1_TARGET: "web" },
});
