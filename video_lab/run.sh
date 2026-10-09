#!/bin/sh
set -eu
task_video_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_video_compiler=/usr/bin/clang++
task_video_evidence="$task_video_root/evidence"
mkdir -p "$task_video_root/build" "$task_video_evidence/release" "$task_video_evidence/undefined"
"$task_video_compiler" --version > "$task_video_evidence/compiler.txt"
task_video_archive() {
    task_video_kind=$1
    task_video_failure=$(mktemp -d "$task_video_evidence/failure-$task_video_kind-$task_video_config.XXXXXX")
    cp "$task_video_root/video.hpp" "$task_video_root/video.cpp" "$task_video_root/reference.hpp" \
        "$task_video_root/reference.cpp" "$task_video_root/tests.cpp" "$task_video_root/run.sh" \
        "$task_video_evidence/compiler.txt" "$task_video_failure/"
    if [ "$task_video_kind" = test ]; then
        cp -R "$task_video_evidence/$task_video_config" "$task_video_failure/test-evidence"
        cp "$task_video_evidence/$task_video_config.stdout.log" "$task_video_evidence/$task_video_config.stderr.log" \
            "$task_video_evidence/$task_video_config.status.json" "$task_video_failure/"
    else
        cp "$task_video_evidence/build-$task_video_config.stdout.log" "$task_video_evidence/build-$task_video_config.stderr.log" \
            "$task_video_evidence/build-$task_video_config.status.json" "$task_video_failure/"
    fi
}
task_video_build() {
    task_video_config=$1
    shift
    if "$task_video_compiler" -std=c++17 -Wall -Wextra -Wpedantic -Werror "$@" \
        "$task_video_root/video.cpp" "$task_video_root/reference.cpp" "$task_video_root/tests.cpp" \
        -o "$task_video_root/build/video-tests-$task_video_config" \
        > "$task_video_evidence/build-$task_video_config.stdout.log" \
        2> "$task_video_evidence/build-$task_video_config.stderr.log"; then
        task_video_rc=0
    else
        task_video_rc=$?
    fi
    printf '{"returncode":%s}\n' "$task_video_rc" > "$task_video_evidence/build-$task_video_config.status.json"
    if [ "$task_video_rc" -ne 0 ]; then
        cat "$task_video_evidence/build-$task_video_config.stderr.log"
        task_video_archive build
        exit "$task_video_rc"
    fi
}
task_video_run() {
    task_video_config=$1
    if UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
        "$task_video_root/build/video-tests-$task_video_config" "$task_video_evidence/$task_video_config" \
        > "$task_video_evidence/$task_video_config.stdout.log" \
        2> "$task_video_evidence/$task_video_config.stderr.log"; then
        task_video_rc=0
    else
        task_video_rc=$?
    fi
    printf '{"returncode":%s}\n' "$task_video_rc" > "$task_video_evidence/$task_video_config.status.json"
    cat "$task_video_evidence/$task_video_config.stdout.log"
    cat "$task_video_evidence/$task_video_config.stderr.log"
    if [ "$task_video_rc" -ne 0 ]; then task_video_archive test; exit "$task_video_rc"; fi
}
task_video_build release -O2
task_video_run release
task_video_build undefined -O1 -g -fsanitize=undefined -fno-omit-frame-pointer
task_video_run undefined
/usr/bin/file "$task_video_root/build/video-tests-release" "$task_video_root/build/video-tests-undefined" > "$task_video_evidence/native-targets.txt"
/usr/bin/otool -L "$task_video_root/build/video-tests-release" "$task_video_root/build/video-tests-undefined" > "$task_video_evidence/native-dependencies.txt"
