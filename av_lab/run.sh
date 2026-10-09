#!/bin/sh
set -eu
task_av_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_av_result=0
mkdir -p "$task_av_root/build" "$task_av_root/evidence"
/usr/bin/clang++ --version > "$task_av_root/evidence/compiler.txt"
if /usr/bin/clang++ -std=c++17 -O2 -fobjc-arc -Wall -Wextra -Wpedantic -Werror "$task_av_root/encoder_inventory.mm" \
    -framework Foundation -framework VideoToolbox -o "$task_av_root/build/encoder-inventory" \
    > "$task_av_root/evidence/inventory-build.stdout.log" 2> "$task_av_root/evidence/inventory-build.stderr.log"; then task_av_rc=0; else task_av_rc=$?; fi
printf '{"returncode":%s}\n' "$task_av_rc" > "$task_av_root/evidence/inventory-build.status.json"
if [ "$task_av_rc" -ne 0 ]; then cat "$task_av_root/evidence/inventory-build.stderr.log"; exit "$task_av_rc"; fi
if "$task_av_root/build/encoder-inventory" "$task_av_root/evidence/encoder_inventory.json" \
    > "$task_av_root/evidence/inventory.stdout.log" 2> "$task_av_root/evidence/inventory.stderr.log"; then task_av_rc=0; else task_av_rc=$?; fi
printf '{"returncode":%s}\n' "$task_av_rc" > "$task_av_root/evidence/inventory.status.json"
if [ "$task_av_rc" -ne 0 ]; then cat "$task_av_root/evidence/inventory.stderr.log"; exit "$task_av_rc"; fi
for task_av_config in release undefined; do
    mkdir -p "$task_av_root/evidence/$task_av_config"
    if [ "$task_av_config" = release ]; then set -- -O2; else set -- -O1 -g -fsanitize=undefined -fno-omit-frame-pointer; fi
    if /usr/bin/clang++ -std=c++17 -fobjc-arc -Wall -Wextra -Wpedantic -Werror -Wno-deprecated-declarations "$@" \
        "$task_av_root/tests.mm" "$task_av_root/../metal_lab/backend.mm" \
        "$task_av_root/../video_lab/video.cpp" "$task_av_root/../video_lab/reference.cpp" "$task_av_root/../audio_lab/audio.cpp" \
        "$task_av_root/../surface_lab/surface.cpp" "$task_av_root/../surface_lab/oracle.cpp" \
        -framework AVFoundation -framework Foundation -framework CoreMedia -framework CoreVideo -framework AudioToolbox -framework Metal -framework ImageIO -framework CoreGraphics \
        -o "$task_av_root/build/av-tests-$task_av_config" > "$task_av_root/evidence/build-$task_av_config.stdout.log" \
        2> "$task_av_root/evidence/build-$task_av_config.stderr.log"; then task_av_rc=0; else task_av_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_av_rc" > "$task_av_root/evidence/build-$task_av_config.status.json"
    if [ "$task_av_rc" -ne 0 ]; then cat "$task_av_root/evidence/build-$task_av_config.stderr.log"; exit "$task_av_rc"; fi
    if UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$task_av_root/build/av-tests-$task_av_config" "$task_av_root/evidence/$task_av_config" \
        > "$task_av_root/evidence/$task_av_config.stdout.log" 2> "$task_av_root/evidence/$task_av_config.stderr.log"; then task_av_rc=0; else task_av_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_av_rc" > "$task_av_root/evidence/$task_av_config.status.json"
    cat "$task_av_root/evidence/$task_av_config.stdout.log" "$task_av_root/evidence/$task_av_config.stderr.log"
    if [ "$task_av_rc" -eq 3 ]; then task_av_result=3; elif [ "$task_av_rc" -ne 0 ]; then
        task_av_fail=$(mktemp -d "$task_av_root/evidence/failure-$task_av_config.XXXXXX")
        cp "$task_av_root/tests.mm" "$task_av_root/run.sh" "$task_av_fail/"
        cp -R "$task_av_root/evidence/$task_av_config" "$task_av_fail/test-evidence"
        cp "$task_av_root/evidence/$task_av_config.stdout.log" "$task_av_root/evidence/$task_av_config.stderr.log" "$task_av_fail/"
        exit "$task_av_rc"
    fi
done
/usr/bin/otool -L "$task_av_root/build/av-tests-release" > "$task_av_root/evidence/native-dependencies.txt"
exit "$task_av_result"
