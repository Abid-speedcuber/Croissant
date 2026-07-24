#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: ./icon.sh /path/to/icon.png" >&2
  echo "Example: ./icon.sh ~/Downloads/example.png" >&2
}

if [[ $# -ne 1 ]]; then
  usage
  exit 2
fi

source_icon=$1
if [[ $source_icon == "~" ]]; then
  source_icon=$HOME
elif [[ $source_icon == "~/"* ]]; then
  source_icon="$HOME/${source_icon#~/}"
fi

if [[ ! -f $source_icon ]]; then
  echo "Icon source not found: $source_icon" >&2
  exit 1
fi

root_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
main_dir="$root_dir/main"
public_dir="$main_dir/public"
legacy_icon="$root_dir/legacy/res/icon.ico"

if command -v magick >/dev/null 2>&1; then
  image_cmd=(magick)
elif command -v convert >/dev/null 2>&1; then
  image_cmd=(convert)
else
  echo "ImageMagick is required. Install 'magick' or 'convert' and try again." >&2
  exit 1
fi

mkdir -p "$public_dir" "$(dirname "$legacy_icon")"

echo "Generating Tauri desktop/mobile icons..."
(
  cd "$main_dir"
  npx tauri icon "$source_icon"
)

echo "Generating web favicon and PWA icons..."
"${image_cmd[@]}" "$source_icon" -resize 16x16 "$public_dir/favicon-16x16.png"
"${image_cmd[@]}" "$source_icon" -resize 32x32 "$public_dir/favicon-32x32.png"
"${image_cmd[@]}" "$source_icon" -resize 48x48 "$public_dir/favicon-48x48.png"
"${image_cmd[@]}" "$source_icon" -resize 180x180 "$public_dir/apple-touch-icon.png"
"${image_cmd[@]}" "$source_icon" -resize 192x192 "$public_dir/pwa-192x192.png"
"${image_cmd[@]}" "$source_icon" -resize 512x512 "$public_dir/pwa-512x512.png"
"${image_cmd[@]}" "$source_icon" -resize 512x512 "$public_dir/maskable-icon-512x512.png"
"${image_cmd[@]}" "$source_icon" -define icon:auto-resize=256,128,64,48,32,16 "$public_dir/favicon.ico"

echo "Updating legacy Windows/Qt icon..."
cp "$main_dir/src-tauri/icons/icon.ico" "$legacy_icon"

echo "Done. Updated icons from: $source_icon"
