#!/bin/sh
set -eu

task_surface_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_surface_compiler=/usr/bin/clang++
task_surface_evidence="$task_surface_root/evidence"
mkdir -p "$task_surface_root/build" "$task_surface_evidence/release" "$task_surface_evidence/undefined"
"$task_surface_compiler" --version > "$task_surface_evidence/compiler.txt"

task_surface_build() {
    task_surface_configuration=$1
    shift
    if "$task_surface_compiler" -std=c++17 -Wall -Wextra -Wpedantic -Werror "$@" \
        "$task_surface_root/surface.cpp" "$task_surface_root/oracle.cpp" "$task_surface_root/tests.cpp" \
        -o "$task_surface_root/build/surface-tests-$task_surface_configuration" \
        > "$task_surface_evidence/build-$task_surface_configuration.stdout.log" \
        2> "$task_surface_evidence/build-$task_surface_configuration.stderr.log"; then
        task_surface_rc=0
    else
        task_surface_rc=$?
    fi
    printf '{"returncode":%s}\n' "$task_surface_rc" > "$task_surface_evidence/build-$task_surface_configuration.status.json"
    if [ "$task_surface_rc" -ne 0 ]; then
        cat "$task_surface_evidence/build-$task_surface_configuration.stderr.log"
        exit "$task_surface_rc"
    fi
}

task_surface_run() {
    task_surface_configuration=$1
    if ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
        "$task_surface_root/build/surface-tests-$task_surface_configuration" \
        "$task_surface_evidence/$task_surface_configuration" \
        > "$task_surface_evidence/$task_surface_configuration.stdout.log" \
        2> "$task_surface_evidence/$task_surface_configuration.stderr.log"; then
        task_surface_rc=0
    else
        task_surface_rc=$?
    fi
    printf '{"returncode":%s}\n' "$task_surface_rc" > "$task_surface_evidence/$task_surface_configuration.status.json"
    cat "$task_surface_evidence/$task_surface_configuration.stdout.log"
    cat "$task_surface_evidence/$task_surface_configuration.stderr.log"
    if [ "$task_surface_rc" -ne 0 ]; then exit "$task_surface_rc"; fi
}

task_surface_build release -O2
task_surface_run release
task_surface_build undefined -O1 -g -fsanitize=undefined -fno-omit-frame-pointer
task_surface_run undefined
/usr/bin/file "$task_surface_root/build/surface-tests-release" "$task_surface_root/build/surface-tests-undefined" \
    > "$task_surface_evidence/native-targets.txt"
/usr/bin/shasum -a 256 "$task_surface_root/surface.hpp" "$task_surface_root/surface.cpp" \
    "$task_surface_root/oracle.hpp" "$task_surface_root/oracle.cpp" \
    "$task_surface_root/tests.cpp" "$task_surface_root/run.sh" \
    "$task_surface_root/build/surface-tests-release" "$task_surface_root/build/surface-tests-undefined" \
    "$task_surface_evidence/release/summary.json" "$task_surface_evidence/undefined/summary.json" \
    > "$task_surface_evidence/source-and-result.sha256"

# Optional diagnostic, kept separate because the installed ASAN runtime aborted
# during initialization in the recorded run. Its failure must remain nonzero.
if [ "${1:-}" = "--asan" ]; then
    mkdir -p "$task_surface_evidence/sanitized"
    task_surface_build sanitized -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
    task_surface_run sanitized
elif [ "$#" -ne 0 ]; then
    printf '%s\n' 'usage: /bin/sh run.sh [--asan]' >&2
    exit 2
fi
