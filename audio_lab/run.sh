#!/bin/sh
set -eu
task_audio_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_audio_compiler=/usr/bin/clang++
task_audio_evidence="$task_audio_root/evidence"
mkdir -p "$task_audio_root/build" "$task_audio_evidence/release" "$task_audio_evidence/undefined"
"$task_audio_compiler" --version > "$task_audio_evidence/compiler.txt"

task_audio_archive_failure() {
    task_audio_kind=$1
    task_audio_archive=$(mktemp -d "$task_audio_evidence/failure-$task_audio_kind-$task_audio_config.XXXXXX")
    if [ "$task_audio_kind" = build ]; then
        cp "$task_audio_evidence/build-$task_audio_config.stdout.log" "$task_audio_evidence/build-$task_audio_config.stderr.log" \
            "$task_audio_evidence/build-$task_audio_config.status.json" "$task_audio_archive/"
    else
        cp "$task_audio_evidence/$task_audio_config.stdout.log" "$task_audio_evidence/$task_audio_config.stderr.log" \
            "$task_audio_evidence/$task_audio_config.status.json" "$task_audio_archive/"
        cp -R "$task_audio_evidence/$task_audio_config" "$task_audio_archive/test-evidence"
    fi
    cp "$task_audio_root/audio.hpp" "$task_audio_root/audio.cpp" "$task_audio_root/reference.hpp" \
        "$task_audio_root/reference.cpp" "$task_audio_root/tests.cpp" "$task_audio_root/run.sh" \
        "$task_audio_evidence/compiler.txt" "$task_audio_archive/"
}

task_audio_build() {
    task_audio_config=$1
    shift
    if "$task_audio_compiler" -std=c++17 -Wall -Wextra -Wpedantic -Werror "$@" \
        "$task_audio_root/audio.cpp" "$task_audio_root/reference.cpp" "$task_audio_root/tests.cpp" \
        -o "$task_audio_root/build/audio-tests-$task_audio_config" \
        > "$task_audio_evidence/build-$task_audio_config.stdout.log" \
        2> "$task_audio_evidence/build-$task_audio_config.stderr.log"; then
        task_audio_rc=0
    else
        task_audio_rc=$?
    fi
    printf '{"returncode":%s}\n' "$task_audio_rc" > "$task_audio_evidence/build-$task_audio_config.status.json"
    if [ "$task_audio_rc" -ne 0 ]; then
        cat "$task_audio_evidence/build-$task_audio_config.stderr.log"
        task_audio_archive_failure build
        exit "$task_audio_rc"
    fi
}
task_audio_run() {
    task_audio_config=$1
    if UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
        "$task_audio_root/build/audio-tests-$task_audio_config" "$task_audio_evidence/$task_audio_config" \
        > "$task_audio_evidence/$task_audio_config.stdout.log" \
        2> "$task_audio_evidence/$task_audio_config.stderr.log"; then
        task_audio_rc=0
    else
        task_audio_rc=$?
    fi
    printf '{"returncode":%s}\n' "$task_audio_rc" > "$task_audio_evidence/$task_audio_config.status.json"
    cat "$task_audio_evidence/$task_audio_config.stdout.log"
    cat "$task_audio_evidence/$task_audio_config.stderr.log"
    if [ "$task_audio_rc" -ne 0 ]; then
        task_audio_archive_failure test
        exit "$task_audio_rc"
    fi
}
task_audio_build release -O2
task_audio_run release
task_audio_build undefined -O1 -g -fsanitize=undefined -fno-omit-frame-pointer
task_audio_run undefined
/usr/bin/file "$task_audio_root/build/audio-tests-release" "$task_audio_root/build/audio-tests-undefined" > "$task_audio_evidence/native-targets.txt"
/usr/bin/otool -L "$task_audio_root/build/audio-tests-release" "$task_audio_root/build/audio-tests-undefined" > "$task_audio_evidence/native-dependencies.txt"
