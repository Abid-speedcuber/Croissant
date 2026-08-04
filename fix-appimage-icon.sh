#!/usr/bin/env bash
set -euo pipefail

root_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
bundle_dir="$root_dir/main/src-tauri/target/release/bundle/appimage"
source_icon="$root_dir/main/src-tauri/icons/icon.png"
plugin="${LINUXDEPLOY_PLUGIN_APPIMAGE:-$HOME/.cache/tauri/linuxdeploy-plugin-appimage.AppImage}"
arch="${ARCH:-x86_64}"

if [[ ! -f "$source_icon" ]]; then
  echo "Missing source icon: $source_icon" >&2
  exit 1
fi

if [[ ! -d "$bundle_dir" ]]; then
  echo "No AppImage bundle directory found: $bundle_dir" >&2
  exit 0
fi

shopt -s nullglob
appdirs=("$bundle_dir"/*.AppDir)
if [[ ${#appdirs[@]} -eq 0 ]]; then
  echo "No AppDir found in $bundle_dir" >&2
  exit 0
fi

for appdir in "${appdirs[@]}"; do
  desktop_file=$(find "$appdir" -maxdepth 1 -name '*.desktop' -print -quit)
  if [[ -z "$desktop_file" ]]; then
    echo "Skipping $appdir: no desktop file found" >&2
    continue
  fi

  icon_name=$(sed -n 's/^Icon=//p' "$desktop_file" | head -n 1)
  app_name=$(sed -n 's/^Name=//p' "$desktop_file" | head -n 1)
  if [[ -z "$icon_name" ]]; then
    echo "Skipping $appdir: desktop file has no Icon entry" >&2
    continue
  fi

  echo "Patching AppDir icon: $appdir"
  if [[ -L "$appdir/$icon_name.png" ]]; then
    unlink "$appdir/$icon_name.png"
  fi
  if [[ -L "$appdir/.DirIcon" ]]; then
    unlink "$appdir/.DirIcon"
  fi
  cp "$source_icon" "$appdir/$icon_name.png"
  cp "$source_icon" "$appdir/.DirIcon"

  if [[ -x "$plugin" ]]; then
    echo "Repacking AppImage for $app_name..."
    (
      cd "$bundle_dir"
      ARCH="$arch" "$plugin" --appdir="$(basename "$appdir")"
    )

    generated="$bundle_dir/${app_name}-${arch}.AppImage"
    versioned=("$bundle_dir"/"${app_name}"_*".AppImage")
    if [[ -f "$generated" && ${#versioned[@]} -gt 0 ]]; then
      for image in "${versioned[@]}"; do
        [[ "$image" == "$generated" ]] && continue
        cp "$generated" "$image"
      done
    fi
  else
    echo "AppImage plugin not found at $plugin; patched AppDir only." >&2
  fi
done

if command -v gio >/dev/null 2>&1; then
  for image in "$bundle_dir"/*.AppImage; do
    gio set -t string "$image" metadata::custom-icon "file://$source_icon" || true
  done
fi

echo "AppImage icon patch complete."
