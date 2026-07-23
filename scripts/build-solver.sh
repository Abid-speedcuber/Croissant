#!/usr/bin/env bash
# Prepare the precomputed tables consumed by the embedded sq1opt.cpp solver.
# sq1opt.cpp itself is compiled for the active Rust target by src-tauri/build.rs,
# including Android's NDK target; no child-process sidecar is used.
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
app_dir="$root_dir/main"
resource_dir="$app_dir/src-tauri/resources/pruning-tables"
if [[ -d "$root_dir/legacy/build/Desktop-Debug/pruning-tables" ]]; then
  mkdir -p "$resource_dir"
  cp "$root_dir"/legacy/build/Desktop-Debug/pruning-tables/*.dat "$resource_dir"/
fi
echo "Prepared embedded solver pruning tables in $resource_dir"
