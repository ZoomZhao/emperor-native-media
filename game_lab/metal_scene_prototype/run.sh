#!/bin/sh
# Build and run the isolated offscreen Metal sprite prototype.
# Output goes to ./build (git-ignored by **/build*/) unless MSP_BUILD_DIR is set.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
BUILD="${MSP_BUILD_DIR:-$HERE/build}"
mkdir -p "$BUILD"
/usr/bin/xcrun clang++ -std=c++17 -fobjc-arc -fno-modules -O2 -mmacosx-version-min=15.0 \
    -Wall -Wextra -Werror \
    "$HERE/prototype_main.mm" "$HERE/metal_sprite_renderer.mm" \
    -framework Foundation -framework Metal -framework CoreGraphics \
    -o "$BUILD/metal_scene_prototype"
exec "$BUILD/metal_scene_prototype" "$@"
