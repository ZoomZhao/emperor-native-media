#!/bin/sh
set -eu
APP_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
LAB_ROOT=$(CDPATH= cd -- "$APP_ROOT/.." && pwd)
REPO_ROOT=$(CDPATH= cd -- "$LAB_ROOT/.." && pwd)

BUILD_DIR="${EMPEROR_BUILD_DIR:-$LAB_ROOT/build}"
case "$BUILD_DIR" in /*) ;; *) echo "EMPEROR_BUILD_DIR must be absolute" >&2; exit 2 ;; esac
APP_BUNDLE="$BUILD_DIR/EmperorApp.app"
if [ -e "$APP_BUNDLE" ]; then echo "Use a fresh build directory; bundle already exists" >&2; exit 2; fi

mkdir -p "$BUILD_DIR"

echo "Compiling EmperorApp binary..."
/usr/bin/clang++ -std=c++17 -fobjc-arc -fno-modules -O2 -mmacosx-version-min=15.0 \
    -Wall -Wextra -Werror \
    -I"$REPO_ROOT" \
    "$APP_ROOT/main.mm" "$APP_ROOT/av_audio_backend.mm" \
    "$LAB_ROOT/app_media.cpp" \
    "$LAB_ROOT/emperor_assets.mm" \
    "$LAB_ROOT/wav_pcm.cpp" "$LAB_ROOT/asset_pixels.cpp" "$LAB_ROOT/resource_root.cpp" \
    "$REPO_ROOT/surface_lab/surface.cpp" \
    -framework AppKit -framework Foundation -framework AVFoundation \
    -framework ImageIO -framework CoreGraphics \
    -lz \
    -o "$BUILD_DIR/EmperorApp.bin"

echo "Packaging EmperorApp.app bundle..."
mkdir -p "$APP_BUNDLE/Contents/MacOS" "$APP_BUNDLE/Contents/Resources"
mv "$BUILD_DIR/EmperorApp.bin" "$APP_BUNDLE/Contents/MacOS/EmperorApp"
cp "$APP_ROOT/Info.plist" "$APP_BUNDLE/Contents/Info.plist"
printf 'APPL????' > "$APP_BUNDLE/Contents/PkgInfo"

/usr/bin/python3 -I -B "$APP_ROOT/build_provenance.py" "$APP_BUNDLE"

echo "Signing EmperorApp bundle..."
/usr/bin/codesign --force --sign - --timestamp=none "$APP_BUNDLE"

echo "Build complete: $APP_BUNDLE"
