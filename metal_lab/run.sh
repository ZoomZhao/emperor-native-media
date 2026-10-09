#!/bin/sh
set -eu
task_metal_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_metal_result=0
mkdir -p "$task_metal_root/build" "$task_metal_root/evidence"
/usr/bin/clang++ --version > "$task_metal_root/evidence/compiler.txt"
for task_metal_config in release undefined; do
    mkdir -p "$task_metal_root/evidence/$task_metal_config"
    if [ "$task_metal_config" = release ]; then set -- -O2; else set -- -O1 -g -fsanitize=undefined -fno-omit-frame-pointer; fi
    if /usr/bin/clang++ -std=c++17 -fobjc-arc -Wall -Wextra -Wpedantic -Werror "$@" \
        "$task_metal_root/backend.mm" "$task_metal_root/tests.mm" "$task_metal_root/../surface_lab/surface.cpp" "$task_metal_root/../surface_lab/oracle.cpp" \
        -framework Metal -framework Foundation -o "$task_metal_root/build/metal-tests-$task_metal_config" \
        > "$task_metal_root/evidence/build-$task_metal_config.stdout.log" 2> "$task_metal_root/evidence/build-$task_metal_config.stderr.log"; then task_metal_rc=0; else task_metal_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_metal_rc" > "$task_metal_root/evidence/build-$task_metal_config.status.json"
    if [ "$task_metal_rc" -ne 0 ]; then cat "$task_metal_root/evidence/build-$task_metal_config.stderr.log"; exit "$task_metal_rc"; fi
    if UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$task_metal_root/build/metal-tests-$task_metal_config" "$task_metal_root/evidence/$task_metal_config" \
        > "$task_metal_root/evidence/$task_metal_config.stdout.log" 2> "$task_metal_root/evidence/$task_metal_config.stderr.log"; then task_metal_rc=0; else task_metal_rc=$?; fi
    printf '{"returncode":%s}\n' "$task_metal_rc" > "$task_metal_root/evidence/$task_metal_config.status.json"
    cat "$task_metal_root/evidence/$task_metal_config.stdout.log" "$task_metal_root/evidence/$task_metal_config.stderr.log"
    if [ "$task_metal_rc" -eq 3 ]; then task_metal_result=3; elif [ "$task_metal_rc" -ne 0 ]; then
        task_metal_fail=$(mktemp -d "$task_metal_root/evidence/failure-$task_metal_config.XXXXXX")
        cp "$task_metal_root/backend.hpp" "$task_metal_root/backend.mm" "$task_metal_root/tests.mm" "$task_metal_root/run.sh" "$task_metal_fail/"
        cp -R "$task_metal_root/evidence/$task_metal_config" "$task_metal_fail/test-evidence"
        cp "$task_metal_root/evidence/$task_metal_config.stdout.log" "$task_metal_root/evidence/$task_metal_config.stderr.log" "$task_metal_fail/"
        exit "$task_metal_rc"
    fi
done
/usr/bin/otool -L "$task_metal_root/build/metal-tests-release" > "$task_metal_root/evidence/native-dependencies.txt"
/usr/bin/shasum -a 256 "$task_metal_root/backend.hpp" "$task_metal_root/backend.mm" "$task_metal_root/tests.mm" "$task_metal_root/run.sh" \
    "$task_metal_root/../surface_lab/surface.hpp" "$task_metal_root/../surface_lab/surface.cpp" "$task_metal_root/../surface_lab/oracle.hpp" "$task_metal_root/../surface_lab/oracle.cpp" \
    "$task_metal_root/build/metal-tests-release" "$task_metal_root/build/metal-tests-undefined" \
    "$task_metal_root/evidence/release/summary.json" "$task_metal_root/evidence/undefined/summary.json" \
    > "$task_metal_root/evidence/source-and-result.sha256"
exit "$task_metal_result"
