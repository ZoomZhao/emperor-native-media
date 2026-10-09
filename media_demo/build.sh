#!/bin/sh
set -eu
task_demo_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_demo_repo=$(CDPATH= cd -- "$task_demo_root/.." && pwd)
test "$#" -le 2
task_demo_build=${1:-"$task_demo_root/build"}
task_demo_evidence=${2:-"$task_demo_root/evidence"}
case "$task_demo_build" in "$task_demo_root"/build|"$task_demo_root"/build-*) ;; *) exit 2;; esac
case "$task_demo_evidence" in "$task_demo_root"/evidence|"$task_demo_build"/evidence) ;; *) exit 2;; esac
mkdir -p "$task_demo_build" "$task_demo_evidence"
printf '{"passed":false,"stage":"build"}\n' > "$task_demo_evidence/build.status.json"
/usr/bin/shasum -a 256 -c "$task_demo_repo/.generated/source-and-fixtures.sha256" > "$task_demo_evidence/generated-fixture-integrity.log"
task_demo_app="$task_demo_build/NativeMediaDemo.app"
if [ -d "$task_demo_app" ]; then
    task_demo_previous=$(mktemp -d "$task_demo_evidence/previous-bundle.XXXXXX")
    mv "$task_demo_app" "$task_demo_previous/"
fi
/usr/bin/clang++ -std=c++17 -fobjc-arc -O2 -mmacosx-version-min=15.0 -Wall -Wextra -Wpedantic -Werror \
    "$task_demo_root/main.mm" "$task_demo_root/controller.cpp" "$task_demo_root/device_monitor.mm" \
    "$task_demo_repo/stream_lab/movie_io.mm" "$task_demo_repo/stream_lab/presenter.cpp" \
    "$task_demo_repo/video_lab/stream.cpp" "$task_demo_repo/audio_lab/audio.cpp" \
    "$task_demo_repo/surface_lab/surface.cpp" "$task_demo_repo/metal_lab/backend.mm" \
    -framework AppKit -framework QuartzCore -framework Foundation -framework AVFoundation -framework AudioToolbox \
    -framework CoreMedia -framework CoreVideo -framework Metal -framework CoreAudio -framework UniformTypeIdentifiers \
    -o "$task_demo_build/NativeMediaDemo.new" > "$task_demo_evidence/build.stdout.log" 2> "$task_demo_evidence/build.stderr.log"
mkdir -p "$task_demo_app/Contents/MacOS" "$task_demo_app/Contents/Resources/TestMedia"
mv "$task_demo_build/NativeMediaDemo.new" "$task_demo_app/Contents/MacOS/NativeMediaDemo"
cp "$task_demo_root/Info.plist" "$task_demo_app/Contents/Info.plist"
cp "$task_demo_repo/.generated/self_authored_8s.mov" "$task_demo_app/Contents/Resources/"
cp "$task_demo_root/README.md" "$task_demo_app/Contents/Resources/README.md"
for task_demo_fixture in self_authored_32s.mov corrupt.mov truncated.mov; do
    cp "$task_demo_repo/.generated/reliability-fixtures/$task_demo_fixture" "$task_demo_app/Contents/Resources/TestMedia/"
done
printf 'APPL????' > "$task_demo_app/Contents/PkgInfo"
/usr/bin/codesign --force --sign - --timestamp=none "$task_demo_app" > "$task_demo_evidence/sign.stdout.log" 2> "$task_demo_evidence/sign.stderr.log"
printf '{"passed":true,"stage":"compile-and-local-package","ad_hoc":true,"GUI_launched":false,"audio_device_opened":false}\n' > "$task_demo_evidence/build.status.json"
/bin/sh "$task_demo_root/verify.sh" "$task_demo_build" "$task_demo_evidence"
