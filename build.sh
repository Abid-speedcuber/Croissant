#!/usr/bin/env bash
# Usage: ./build.sh linux | windows | macos | android
# Desktop artifacts must be built on their target OS, or with an appropriately
# configured Rust/C++ cross toolchain. Android requires the Tauri Android SDK.
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
platform="${1:-}"
if [[ -z "$platform" ]]; then
  echo "Usage: $0 {windows|linux|macos|android}" >&2
  exit 2
fi

cd "$root_dir/main"
npm ci

case "$platform" in
  linux)
    "$root_dir/scripts/build-solver.sh" tables
    npx tauri build
    ;;
  windows)
    "$root_dir/scripts/build-solver.sh" tables
    npx tauri build
    ;;
  macos)
    "$root_dir/scripts/build-solver.sh" tables
    npx tauri build
    ;;
  android)
    "$root_dir/scripts/build-solver.sh" tables
    if [[ ! -d src-tauri/gen/android ]]; then
      npx tauri android init
    fi
    npx tauri android build --apk
    ;;
  *) echo "Unknown platform: $platform" >&2; exit 2 ;;
esac
