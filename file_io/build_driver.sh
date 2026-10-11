#!/bin/sh
set -eu
component_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
result_dir=${1:?usage: build_driver.sh ABSOLUTE_OUTPUT_DIR [release|undefined]}
mode=${2:-release}
case "$result_dir" in /*) ;; *) echo 'absolute output directory required' >&2; exit 2;; esac
case "$mode" in
  release) set -- -O2 ;;
  undefined) set -- -O1 -g -fsanitize=undefined -fno-sanitize-recover=all ;;
  *) echo 'mode must be release or undefined' >&2; exit 2 ;;
esac
mkdir -p "$result_dir"
sdk_dir=$(/usr/bin/xcrun --sdk macosx --show-sdk-path)
/usr/bin/clang++ -std=c++17 -Wall -Wextra -Werror "$@" -isysroot "$sdk_dir" \
  -I"$component_dir/include" -Demfr_decode_zlib=emfr_decode_zlib_actual \
  -c "$component_dir/src/strict_zlib.cpp" -o "$result_dir/strict_zlib.o"
/usr/bin/clang++ -std=c++17 -Wall -Wextra -Werror "$@" -isysroot "$sdk_dir" \
  -I"$component_dir/include" "$result_dir/strict_zlib.o" \
  "$component_dir/src/framed_reader.cpp" "$component_dir/tests/fixture_driver.cpp" \
  "$component_dir/tests/codec_probe.cpp" \
  -lz -o "$result_dir/fixture_driver"
