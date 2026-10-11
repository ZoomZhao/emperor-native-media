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
    -I"$REPO_ROOT" -I"$REPO_ROOT/media_demo" \
    "$APP_ROOT/main.mm" "$APP_ROOT/av_audio_backend.mm" \
    "$LAB_ROOT/app_media.cpp" "$LAB_ROOT/movie_session.mm" \
    "$LAB_ROOT/emperor_assets.mm" \
    "$LAB_ROOT/wav_pcm.cpp" "$LAB_ROOT/asset_pixels.cpp" "$LAB_ROOT/resource_root.cpp" \
    "$REPO_ROOT/surface_lab/surface.cpp" \
    "$REPO_ROOT/stream_lab/movie_io.mm" "$REPO_ROOT/stream_lab/presenter.cpp" \
    "$REPO_ROOT/video_lab/stream.cpp" \
    "$REPO_ROOT/metal_lab/backend.mm" \
    "$REPO_ROOT/audio_lab/audio.cpp" \
    "$REPO_ROOT/media_demo/controller.cpp" \
    -framework AppKit -framework Foundation -framework AVFoundation \
    -framework AudioToolbox -framework CoreMedia -framework CoreVideo \
    -framework ImageIO -framework CoreGraphics \
    -framework Metal \
    -lz \
    -o "$BUILD_DIR/EmperorApp.bin"

echo "Packaging EmperorApp.app bundle..."
mkdir -p "$APP_BUNDLE/Contents/MacOS" "$APP_BUNDLE/Contents/Resources/SelfAuthoredData"
mv "$BUILD_DIR/EmperorApp.bin" "$APP_BUNDLE/Contents/MacOS/EmperorApp"
cp "$APP_ROOT/Info.plist" "$APP_BUNDLE/Contents/Info.plist"
cp "$REPO_ROOT/LICENSE" "$APP_BUNDLE/Contents/Resources/LICENSE"
cp "$REPO_ROOT/docs/FIRST_PLAY.md" "$APP_BUNDLE/Contents/Resources/FIRST_PLAY.md"
printf 'APPL????' > "$APP_BUNDLE/Contents/PkgInfo"

/usr/bin/python3 -I -B "$APP_ROOT/build_provenance.py" "$APP_BUNDLE"

echo "Signing EmperorApp bundle..."
/usr/bin/codesign --force --sign - --timestamp=none "$APP_BUNDLE"

echo "Build complete: $APP_BUNDLE"
