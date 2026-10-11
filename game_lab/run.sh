#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"

mkdir -p "$DIR/build" "$DIR/evidence"

echo "Building game_lab test runner..."
clang++ -std=c++17 -O3 -Wall -Wextra -Werror -fobjc-arc \
    -I"$ROOT" \
    -framework Foundation -framework ImageIO -framework CoreGraphics -lz \
    "$DIR/emperor_assets.mm" \
    "$DIR/wav_pcm.cpp" "$DIR/asset_pixels.cpp" \
    "$ROOT/surface_lab/surface.cpp" \
    "$DIR/tests.mm" \
    -o "$DIR/build/game-assets-tests"

echo "Executing game_lab asset test suite..."
"$DIR/build/game-assets-tests"

echo "PASS game_lab"
