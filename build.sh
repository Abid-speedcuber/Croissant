#!/usr/bin/env bash
# Usage: ./build.sh {linux|windows|macos|android} [flags]
# Desktop artifacts must be built on their target OS, or with an appropriately
# configured Rust/C++ cross toolchain. Android requires the Tauri Android SDK.
#
# Linux bundle flags:
#   --appimage   Build AppImage only
#   --deb        Build .deb package only
#   --rpm        Build .rpm package only
#   (default: all formats)
#
# Android architecture flags:
#   --aarch64    Build for 64-bit ARM (most modern phones)
#   --armv7      Build for 32-bit ARM (older devices)
#   --i686       Build for 32-bit x86 (emulators)
#   --x86_64     Build for 64-bit x86 (emulators)
#   (default: all four)
#
# Signing (Android):
#   The release keystore is auto-generated at main/keystore/release.jks if absent.
#   Override with env vars:
#     ANDROID_KEYSTORE        Path to keystore (default: main/keystore/release.jks)
#     ANDROID_KEYSTORE_PASS   Keystore password  (default: android)
#     ANDROID_KEY_ALIAS       Key alias           (default: release)
#     ANDROID_KEY_PASS        Key password        (default: android)
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
platform="${1:-}"
if [[ -z "$platform" ]]; then
  echo "Usage: $0 {windows|linux|macos|android} [flags]" >&2
  exit 2
fi

find_java_home() {
  if [[ -n "${JAVA_HOME:-}" && -x "$JAVA_HOME/bin/java" ]]; then
    printf '%s\n' "$JAVA_HOME"
    return 0
  fi
  if command -v java >/dev/null 2>&1; then
    local java_bin
    java_bin="$(readlink -f "$(command -v java)")"
    printf '%s\n' "$(cd "$(dirname "$java_bin")/.." && pwd)"
    return 0
  fi
  local candidate
  for candidate in \
    "$root_dir"/.local-jdk/* \
    /usr/lib/jvm/* \
    /opt/android-studio/jbr \
    "$HOME"/.local/share/JetBrains/Toolbox/apps/android-studio/*/jbr; do
    if [[ -x "$candidate/bin/java" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

prepare_android_env() {
  if [[ -z "${ANDROID_HOME:-}" && -d "$HOME/Android/Sdk" ]]; then
    export ANDROID_HOME="$HOME/Android/Sdk"
  fi
  if [[ -z "${ANDROID_SDK_ROOT:-}" && -n "${ANDROID_HOME:-}" ]]; then
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
  fi
  if [[ -n "${ANDROID_HOME:-}" ]]; then
    export PATH="$ANDROID_HOME/platform-tools:$ANDROID_HOME/cmdline-tools/bin:$PATH"
  fi

  local java_home
  if ! java_home="$(find_java_home)"; then
    cat >&2 <<'EOF'
Java/JDK was not found. Android builds need JDK 17 or newer.

Install one, then rerun:
  sudo apt install openjdk-17-jdk

Or place/extract a JDK under:
  ./.local-jdk/<jdk-folder>
EOF
    exit 1
  fi
  export JAVA_HOME="$java_home"
  export PATH="$JAVA_HOME/bin:$PATH"

  if [[ -n "${ANDROID_HOME:-}" ]]; then
    local newest_build_tools=""
    for bt_dir in "$ANDROID_HOME"/build-tools/*/; do
      bt_dir="${bt_dir%/}"
      newest_build_tools="$bt_dir"
    done
    if [[ -n "$newest_build_tools" ]]; then
      export PATH="$newest_build_tools:$PATH"
    fi
  fi
}

find_apksigner() {
  if command -v apksigner >/dev/null 2>&1; then
    return 0
  fi
  if [[ -n "${ANDROID_HOME:-}" ]]; then
    local newest_build_tools=""
    for bt_dir in "$ANDROID_HOME"/build-tools/*/; do
      newest_build_tools="${bt_dir%/}"
    done
    if [[ -n "$newest_build_tools" && -x "$newest_build_tools/apksigner" ]]; then
      export PATH="$newest_build_tools:$PATH"
      return 0
    fi
  fi
  return 1
}

ensure_release_keystore() {
  : "${ANDROID_KEYSTORE:=$root_dir/main/keystore/release.jks}"
  : "${ANDROID_KEYSTORE_PASS:=android}"
  : "${ANDROID_KEY_ALIAS:=release}"
  : "${ANDROID_KEY_PASS:=android}"
  export ANDROID_KEYSTORE ANDROID_KEYSTORE_PASS ANDROID_KEY_ALIAS ANDROID_KEY_PASS

  if [[ -f "$ANDROID_KEYSTORE" ]]; then
    return 0
  fi

  echo "Generating release keystore at $ANDROID_KEYSTORE"
  mkdir -p "$(dirname "$ANDROID_KEYSTORE")"
  keytool -genkeypair -v \
    -keystore "$ANDROID_KEYSTORE" \
    -alias "$ANDROID_KEY_ALIAS" \
    -keyalg RSA -keysize 2048 \
    -validity 10000 \
    -storepass "$ANDROID_KEYSTORE_PASS" \
    -keypass "$ANDROID_KEY_PASS" \
    -dname "CN=Croissant, OU=Development, O=Croissant, L=Unknown, ST=Unknown, C=US"
}

sign_apk() {
  local apk="$1"
  if [[ ! -f "$apk" ]]; then
    echo "APK not found: $apk" >&2
    return 1
  fi
  if ! find_apksigner; then
    echo "Warning: apksigner not found, skipping APK signing" >&2
    return 0
  fi

  ensure_release_keystore

  local aligned_apk="${apk%.apk}-aligned.apk"
  local signed_apk="${apk%.apk}-signed.apk"

  echo "Zip-aligning APK..."
  if command -v zipalign >/dev/null 2>&1; then
    zipalign -f 4 "$apk" "$aligned_apk"
  elif [[ -n "${ANDROID_HOME:-}" ]]; then
    local newest_build_tools=""
    for bt_dir in "$ANDROID_HOME"/build-tools/*/; do
      newest_build_tools="${bt_dir%/}"
    done
    "$newest_build_tools/zipalign" -f 4 "$apk" "$aligned_apk"
  else
    echo "Warning: zipalign not found, signing without alignment" >&2
    aligned_apk="$apk"
  fi

  echo "Signing APK..."
  apksigner sign \
    --ks "$ANDROID_KEYSTORE" \
    --ks-pass "pass:$ANDROID_KEYSTORE_PASS" \
    --ks-key-alias "$ANDROID_KEY_ALIAS" \
    --key-pass "pass:$ANDROID_KEY_PASS" \
    --out "$signed_apk" \
    "$aligned_apk"

  if [[ "$aligned_apk" != "$apk" ]]; then
    rm -f "$aligned_apk"
  fi
  mv "$signed_apk" "$apk"

  echo "Verifying signature..."
  apksigner verify --verbose "$apk" 2>&1 | head -5

  echo "APK signed: $apk"
}

resource_dir="$root_dir/main/src-tauri/resources/pruning-tables"
if [[ -d "$root_dir/legacy/build/Desktop-Debug/pruning-tables" ]]; then
  mkdir -p "$resource_dir"
  cp "$root_dir"/legacy/build/Desktop-Debug/pruning-tables/*.dat "$resource_dir"/
fi
echo "Prepared embedded solver pruning tables in $resource_dir"

if [[ "$platform" == "android" ]]; then
  prepare_android_env
fi

cd "$root_dir/main"
npm install

case "$platform" in
  linux)
    shift || true
    build_appimage=false
    build_deb=false
    build_rpm=false
    for arg in "$@"; do
      case "$arg" in
        --appimage) build_appimage=true ;;
        --deb)      build_deb=true ;;
        --rpm)      build_rpm=true ;;
        *) echo "Unknown flag: $arg" >&2; exit 2 ;;
      esac
    done

    if ! $build_appimage && ! $build_deb && ! $build_rpm; then
      build_appimage=true
      build_deb=true
      build_rpm=true
    fi

    bundles=()
    $build_appimage && bundles+=("appimage")
    $build_deb      && bundles+=("deb")
    $build_rpm      && bundles+=("rpm")

    echo "Building Linux bundles: ${bundles[*]}"

    tauri_args=()
    for b in "${bundles[@]}"; do
      tauri_args+=(--bundles "$b")
    done
    npx tauri build "${tauri_args[@]}"

    if $build_appimage; then
      "$root_dir/fix-appimage-icon.sh"
    fi
    ;;
  windows|macos)
    npx tauri build
    ;;
  android)
    shift || true
    build_aarch64=false
    build_armv7=false
    build_i686=false
    build_x86_64=false
    for arg in "$@"; do
      case "$arg" in
        --aarch64) build_aarch64=true ;;
        --armv7)   build_armv7=true ;;
        --i686)    build_i686=true ;;
        --x86_64)  build_x86_64=true ;;
        *) echo "Unknown flag: $arg" >&2; exit 2 ;;
      esac
    done

    if ! $build_aarch64 && ! $build_armv7 && ! $build_i686 && ! $build_x86_64; then
      build_aarch64=true
      build_armv7=true
      build_i686=true
      build_x86_64=true
    fi

    targets=()
    $build_aarch64 && targets+=("aarch64")
    $build_armv7   && targets+=("armv7")
    $build_i686    && targets+=("i686")
    $build_x86_64  && targets+=("x86_64")

    echo "Building for: ${targets[*]}"

    export CARGO_INCREMENTAL=1
    export CARGO_PROFILE_STRIP=true

    identifier="$(node -e "console.log(JSON.parse(require('fs').readFileSync('src-tauri/tauri.conf.json','utf8')).identifier)")"
    expected_java_dir="src-tauri/gen/android/app/src/main/java/${identifier//.//}"
    if [[ -d src-tauri/gen/android && ! -d "$expected_java_dir" ]]; then
      backup="src-tauri/gen/android.pre-identifier.$(date +%Y%m%d%H%M%S)"
      echo "Android package changed; moving stale generated project to $backup"
      mv src-tauri/gen/android "$backup"
    fi
    if [[ ! -d src-tauri/gen/android ]]; then
      npx tauri android init
      if [[ -x "$root_dir/icon.sh" && -f "$root_dir/main/src-tauri/icons/icon.png" ]]; then
        "$root_dir/icon.sh" "$root_dir/main/src-tauri/icons/icon.png"
      fi
    fi

    tauri_args=(--apk)
    for t in "${targets[@]}"; do
      tauri_args+=(--target "$t")
    done
    npx tauri android build "${tauri_args[@]}"

    apk_dir="src-tauri/gen/android/app/build/outputs/apk"
    for apk_file in "$apk_dir"/*/release/*-release-unsigned.apk; do
      [[ -f "$apk_file" ]] || continue
      sign_apk "$apk_file"
    done
    ;;
  *) echo "Unknown platform: $platform" >&2; exit 2 ;;
esac
