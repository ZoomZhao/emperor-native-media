#!/bin/sh
set -eu
APP_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
LAB_ROOT=$(CDPATH= cd -- "$APP_ROOT/.." && pwd)
REPO_ROOT=$(CDPATH= cd -- "$LAB_ROOT/.." && pwd)

BUILD_DIR="$LAB_ROOT/build"
APP_BUNDLE="$BUILD_DIR/EmperorApp.app"

mkdir -p "$BUILD_DIR"
rm -rf "$APP_BUNDLE"

echo "Compiling EmperorApp binary..."
/usr/bin/clang++ -std=c++17 -fobjc-arc -O2 -mmacosx-version-min=15.0 \
    -Wall -Wextra -Werror \
    -I"$REPO_ROOT" \
    "$APP_ROOT/main.mm" \
    "$LAB_ROOT/emperor_assets.mm" \
    "$REPO_ROOT/surface_lab/surface.cpp" \
    -framework AppKit -framework Foundation -framework AVFoundation \
    -framework Metal -framework MetalKit -framework ImageIO -framework CoreGraphics \
    -lz \
    -o "$BUILD_DIR/EmperorApp.bin"

echo "Packaging EmperorApp.app bundle..."
mkdir -p "$APP_BUNDLE/Contents/MacOS" "$APP_BUNDLE/Contents/Resources"
mv "$BUILD_DIR/EmperorApp.bin" "$APP_BUNDLE/Contents/MacOS/EmperorApp"
cp "$APP_ROOT/Info.plist" "$APP_BUNDLE/Contents/Info.plist"
printf 'APPL????' > "$APP_BUNDLE/Contents/PkgInfo"

echo "Signing EmperorApp bundle..."
/usr/bin/codesign --force --sign - --timestamp=none "$APP_BUNDLE"

echo "Build complete: $APP_BUNDLE"
