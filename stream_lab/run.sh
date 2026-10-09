#!/bin/sh
set -eu
task_stream_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$task_stream_root/build" "$task_stream_root/evidence/history"
task_stream_mode=${1:-all}
case "$task_stream_mode" in all|--build-only) ;; *) printf 'usage: run.sh [--build-only]\n' >&2; exit 2;; esac
/usr/bin/clang++ --version > "$task_stream_root/evidence/compiler.txt"
task_stream_archive() {
    task_stream_failure=$(mktemp -d "$task_stream_root/evidence/failure-$task_stream_config.XXXXXX")
    cp "$task_stream_root"/*.hpp "$task_stream_root"/*.cpp "$task_stream_root"/*.mm "$task_stream_root/run.sh" "$task_stream_failure/"
    cp -R "$task_stream_root/evidence/$task_stream_config" "$task_stream_failure/test-evidence"
    cp "$task_stream_root/evidence/build-$task_stream_config."* "$task_stream_failure/"
    if [ -f "$task_stream_root/evidence/$task_stream_config.stderr.log" ]; then
        cp "$task_stream_root/evidence/$task_stream_config."* "$task_stream_failure/"
    fi
}
for task_stream_config in release undefined; do
    mkdir -p "$task_stream_root/evidence/$task_stream_config"
    if [ "$task_stream_config" = release ]; then set -- -O2; else set -- -O1 -g -fsanitize=undefined -fno-omit-frame-pointer; fi
    if /usr/bin/clang++ -std=c++17 -fobjc-arc -Wall -Wextra -Wpedantic -Werror "$@" \
        "$task_stream_root/movie_io.mm" "$task_stream_root/presenter.cpp" "$task_stream_root/tests.mm" \
        "$task_stream_root/../video_lab/stream.cpp" "$task_stream_root/../audio_lab/audio.cpp" \
        "$task_stream_root/../metal_lab/backend.mm" "$task_stream_root/../surface_lab/surface.cpp" "$task_stream_root/../surface_lab/oracle.cpp" \
        -framework AVFoundation -framework AudioToolbox -framework CoreMedia -framework CoreVideo -framework Foundation -framework Metal \
        -o "$task_stream_root/build/stream-tests-$task_stream_config" \
        > "$task_stream_root/evidence/build-$task_stream_config.stdout.log" 2> "$task_stream_root/evidence/build-$task_stream_config.stderr.log"; then task_stream_rc=0; else task_stream_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_stream_rc" > "$task_stream_root/evidence/build-$task_stream_config.status.json"
    if [ "$task_stream_rc" -ne 0 ]; then cat "$task_stream_root/evidence/build-$task_stream_config.stderr.log"; task_stream_archive; exit "$task_stream_rc"; fi
    if [ "$task_stream_mode" = --build-only ]; then continue; fi
    if [ -f "$task_stream_root/evidence/$task_stream_config/summary.json" ]; then
        task_stream_previous=$(mktemp -d "$task_stream_root/evidence/history/previous-$task_stream_config.XXXXXX")
        cp -R "$task_stream_root/evidence/$task_stream_config" "$task_stream_previous/"
    fi
    rm -f "$task_stream_root/evidence/$task_stream_config/self_authored_8s.mov"
    if UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$task_stream_root/build/stream-tests-$task_stream_config" "$task_stream_root/evidence/$task_stream_config" \
        > "$task_stream_root/evidence/$task_stream_config.stdout.log" 2> "$task_stream_root/evidence/$task_stream_config.stderr.log"; then task_stream_rc=0; else task_stream_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_stream_rc" > "$task_stream_root/evidence/$task_stream_config.status.json"
    cat "$task_stream_root/evidence/$task_stream_config.stdout.log" "$task_stream_root/evidence/$task_stream_config.stderr.log"
    if [ "$task_stream_rc" -ne 0 ]; then task_stream_archive; exit "$task_stream_rc"; fi
done
if [ "$task_stream_mode" = --build-only ]; then exit 0; fi
# Compatibility checks for the two components whose public APIs were extended here.
for task_stream_component in audio_lab metal_lab; do
    if [ -d "$task_stream_root/../$task_stream_component/evidence" ] && [ ! -d "$task_stream_root/evidence/history/$task_stream_component-before-regression" ]; then
        cp -R "$task_stream_root/../$task_stream_component/evidence" "$task_stream_root/evidence/history/$task_stream_component-before-regression"
    fi
done
for task_stream_component in audio_lab metal_lab; do
    if /bin/sh "$task_stream_root/../$task_stream_component/run.sh" > "$task_stream_root/evidence/$task_stream_component-regression.log" 2>&1; then task_stream_rc=0; else task_stream_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_stream_rc" > "$task_stream_root/evidence/$task_stream_component-regression.status.json"
    if [ "$task_stream_rc" -ne 0 ]; then cat "$task_stream_root/evidence/$task_stream_component-regression.log"; exit "$task_stream_rc"; fi
done
# The native demux packet count is allowed to vary. Playback PTS, consumed samples,
# epochs, and scheduled frame records must match exactly across configurations.
/usr/bin/cmp "$task_stream_root/evidence/release/timeline.jsonl" "$task_stream_root/evidence/undefined/timeline.jsonl"
for task_stream_config in release undefined; do
    test ! -s "$task_stream_root/evidence/$task_stream_config.stderr.log"
done
printf '{"timeline_byte_identical":true,"stream_stderr_empty":true,"audio_compatibility_passed":true,"metal_compatibility_passed":true}\n' > "$task_stream_root/evidence/verification.json"
/usr/bin/shasum -a 256 "$task_stream_root/../audio_lab/audio.hpp" "$task_stream_root/../audio_lab/audio.cpp" \
    "$task_stream_root/../audio_lab/reference.hpp" "$task_stream_root/../audio_lab/reference.cpp" "$task_stream_root/../audio_lab/tests.cpp" "$task_stream_root/../audio_lab/run.sh" \
    "$task_stream_root/../audio_lab/build/audio-tests-release" "$task_stream_root/../audio_lab/build/audio-tests-undefined" \
    "$task_stream_root/../audio_lab/evidence/release/summary.json" "$task_stream_root/../audio_lab/evidence/undefined/summary.json" \
    > "$task_stream_root/../audio_lab/evidence/source-and-result.sha256"
/usr/bin/otool -L "$task_stream_root/build/stream-tests-release" > "$task_stream_root/evidence/native-dependencies.txt"
/usr/bin/shasum -a 256 "$task_stream_root"/*.hpp "$task_stream_root"/*.cpp "$task_stream_root"/*.mm "$task_stream_root/run.sh" \
    "$task_stream_root/../video_lab/stream.hpp" "$task_stream_root/../video_lab/stream.cpp" \
    "$task_stream_root/../audio_lab/audio.hpp" "$task_stream_root/../audio_lab/audio.cpp" \
    "$task_stream_root/../metal_lab/backend.hpp" "$task_stream_root/../metal_lab/backend.mm" \
    "$task_stream_root/../surface_lab/surface.hpp" "$task_stream_root/../surface_lab/surface.cpp" "$task_stream_root/../surface_lab/oracle.hpp" "$task_stream_root/../surface_lab/oracle.cpp" \
    "$task_stream_root/build/stream-tests-release" "$task_stream_root/build/stream-tests-undefined" \
    "$task_stream_root/evidence/release/summary.json" "$task_stream_root/evidence/undefined/summary.json" \
    "$task_stream_root/evidence/release/timeline.jsonl" "$task_stream_root/evidence/undefined/timeline.jsonl" \
    "$task_stream_root/evidence/release/self_authored_8s.mov" "$task_stream_root/evidence/undefined/self_authored_8s.mov" \
    "$task_stream_root/evidence/release.status.json" "$task_stream_root/evidence/undefined.status.json" "$task_stream_root/evidence/verification.json" \
    "$task_stream_root/evidence/audio_lab-regression.status.json" "$task_stream_root/evidence/metal_lab-regression.status.json" \
    "$task_stream_root/../audio_lab/evidence/release/summary.json" "$task_stream_root/../audio_lab/evidence/undefined/summary.json" \
    "$task_stream_root/../metal_lab/evidence/release/summary.json" "$task_stream_root/../metal_lab/evidence/undefined/summary.json" \
    > "$task_stream_root/evidence/source-and-result.sha256"
printf 'PASS stream target and audio/Metal API compatibility checks\n'
