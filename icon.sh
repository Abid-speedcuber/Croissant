#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Usage: ./icon.sh [flags]

Flags:
  --pc       Generate desktop icons (Tauri + legacy Qt)
  --web      Generate web/PWA favicons
  --mobile   Generate Android mipmaps
  (no flags) Generate all

Examples:
  ./icon.sh           # everything
  ./icon.sh --web     # web icons only
EOF
}

root_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
main_dir="$root_dir/main"
public_dir="$main_dir/public"
icons_dir="$root_dir/icons"
legacy_icon="$root_dir/legacy/res/icon.ico"

pc_icon="$icons_dir/icon-pc.png"
mobile_icon="$icons_dir/icon-mobile.png"
web_icon="$icons_dir/icon-web.png"

do_pc=false
do_web=false
do_mobile=false

for arg in "$@"; do
  case "$arg" in
    --pc)     do_pc=true ;;
    --web)    do_web=true ;;
    --mobile) do_mobile=true ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown flag: $arg" >&2; usage; exit 2 ;;
  esac
done

if ! $do_pc && ! $do_web && ! $do_mobile; then
  do_pc=true
  do_web=true
  do_mobile=true
fi

if $do_pc && [[ ! -f "$pc_icon" ]]; then
  echo "Missing source icon: $pc_icon" >&2; exit 1
fi
if $do_web && [[ ! -f "$web_icon" ]]; then
  echo "Missing source icon: $web_icon" >&2; exit 1
fi
if $do_mobile && [[ ! -f "$mobile_icon" ]]; then
  echo "Missing source icon: $mobile_icon" >&2; exit 1
fi

if $do_web || $do_mobile; then
  if command -v magick >/dev/null 2>&1; then
    image_cmd=(magick)
  elif command -v convert >/dev/null 2>&1; then
    image_cmd=(convert)
  else
    echo "ImageMagick is required. Install 'magick' or 'convert' and try again." >&2
    exit 1
  fi
fi

if $do_pc; then
  mkdir -p "$(dirname "$legacy_icon")"
  echo "Generating Tauri desktop icons from icon-pc.png..."
  (
    cd "$main_dir"
    npx tauri icon "$pc_icon"
  )
  echo "Updating legacy Windows/Qt icon..."
  cp "$main_dir/src-tauri/icons/icon.ico" "$legacy_icon"
fi

if $do_web; then
  mkdir -p "$public_dir"
  echo "Generating web favicon and PWA icons from icon-web.png..."
  "${image_cmd[@]}" "$web_icon" -resize 16x16 "$public_dir/favicon-16x16.png"
  "${image_cmd[@]}" "$web_icon" -resize 32x32 "$public_dir/favicon-32x32.png"
  "${image_cmd[@]}" "$web_icon" -resize 48x48 "$public_dir/favicon-48x48.png"
  "${image_cmd[@]}" "$web_icon" -resize 180x180 "$public_dir/apple-touch-icon.png"
  "${image_cmd[@]}" "$web_icon" -resize 192x192 "$public_dir/pwa-192x192.png"
  "${image_cmd[@]}" "$web_icon" -resize 512x512 "$public_dir/pwa-512x512.png"
  "${image_cmd[@]}" "$web_icon" -resize 512x512 "$public_dir/maskable-icon-512x512.png"
  "${image_cmd[@]}" "$web_icon" -define icon:auto-resize=256,128,64,48,32,16 "$public_dir/favicon.ico"
fi

if $do_mobile; then
  android_res="$main_dir/src-tauri/gen/android/app/src/main/res"
  if [[ -d "$android_res" ]]; then
    echo "Generating Android mipmaps from icon-mobile.png..."
    densities=(mdpi:48 hdpi:72 xhdpi:96 xxhdpi:144 xxxhdpi:192)
    fg_size=432
    for entry in "${densities[@]}"; do
      density="${entry%%:*}"
      size="${entry##*:}"
      mipmap_dir="$android_res/mipmap-$density"
      mkdir -p "$mipmap_dir"

      "${image_cmd[@]}" "$mobile_icon" -resize "${size}x${size}" "$mipmap_dir/ic_launcher.png"
      "${image_cmd[@]}" "$mobile_icon" -resize "${size}x${size}" \
        \( +clone -threshold -1 -negate -fill white -draw "circle $((size/2)),$((size/2)) $((size/2)),0" \) \
        -alpha off -compose CopyOpacity -composite \
        "$mipmap_dir/ic_launcher_round.png"
    done

    fg_dir="$android_res/mipmap-xxxhdpi"
    mkdir -p "$fg_dir"
    "${image_cmd[@]}" "$mobile_icon" -resize "288x288" \
      -gravity center -background none -extent "${fg_size}x${fg_size}" \
      "$fg_dir/ic_launcher_foreground.png"

    for density in mdpi hdpi xhdpi xxhdpi; do
      cp "$fg_dir/ic_launcher_foreground.png" "$android_res/mipmap-$density/ic_launcher_foreground.png"
    done

    echo "Android mipmaps updated."
  else
    echo "Android project not initialized, skipping mipmap generation."
  fi
fi

echo "Done."
