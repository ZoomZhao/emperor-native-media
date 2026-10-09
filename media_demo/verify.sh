#!/bin/sh
set -eu
task_demo_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_demo_repo=$(CDPATH= cd -- "$task_demo_root/.." && pwd)
task_demo_build=${1:-"$task_demo_root/build"}
task_demo_evidence=${2:-"$task_demo_root/evidence"}
task_demo_app="$task_demo_build/NativeMediaDemo.app"
task_demo_executable="$task_demo_app/Contents/MacOS/NativeMediaDemo"
mkdir -p "$task_demo_evidence"
test -x "$task_demo_executable"
/usr/bin/plutil -lint "$task_demo_app/Contents/Info.plist" > "$task_demo_evidence/plist-lint.log"
/usr/bin/codesign --verify --deep --strict "$task_demo_app" > "$task_demo_evidence/sign-verify.stdout.log" 2> "$task_demo_evidence/sign-verify.stderr.log"
/usr/bin/codesign --display --verbose=4 "$task_demo_app" > "$task_demo_evidence/sign-info.stdout.log" 2> "$task_demo_evidence/sign-info.stderr.log"
/usr/bin/awk '/^Signature=adhoc$/ {found=1} END {exit found?0:1}' "$task_demo_evidence/sign-info.stderr.log"
/usr/bin/lipo -archs "$task_demo_executable" > "$task_demo_evidence/architectures.txt"
/usr/bin/otool -L "$task_demo_executable" > "$task_demo_evidence/native-dependencies.txt"
/usr/bin/awk 'NR>1 && $1 !~ /^\/System\/Library\/Frameworks\// && $1 !~ /^\/usr\/lib\// {bad=1} END {exit bad?1:0}' "$task_demo_evidence/native-dependencies.txt"
/usr/bin/nm -u "$task_demo_executable" > "$task_demo_evidence/undefined-symbols.txt"
/usr/bin/awk '/AudioQueue(NewOutput|Start)|AudioComponentInstanceNew|AVAudioEngine|AVPlayer|AudioOutputUnitStart|AudioObjectSetPropertyData|AudioDeviceCreateIOProcID/ {found=1} END {exit found?1:0}' "$task_demo_evidence/undefined-symbols.txt"
/usr/bin/shasum -a 256 -c "$task_demo_repo/.generated/source-and-fixtures.sha256" > "$task_demo_evidence/generated-fixture-integrity.log"
/usr/bin/cmp "$task_demo_app/Contents/Resources/self_authored_8s.mov" "$task_demo_repo/.generated/self_authored_8s.mov"
/usr/bin/cmp "$task_demo_app/Contents/Resources/README.md" "$task_demo_root/README.md"
for task_demo_fixture in self_authored_32s.mov corrupt.mov truncated.mov; do
    /usr/bin/cmp "$task_demo_app/Contents/Resources/TestMedia/$task_demo_fixture" "$task_demo_repo/.generated/reliability-fixtures/$task_demo_fixture"
done
printf '{"passed":true,"static_bundle_verified":true,"ad_hoc_signature_verified":true,"self_authored_fixture_verified":true,"native_Apple_dependencies_only":true,"audio_device_opened":false,"GUI_runtime_verified":false}\n' > "$task_demo_evidence/verify.status.json"
/usr/bin/shasum -a 256 "$task_demo_root/main.mm" "$task_demo_root/controller.hpp" "$task_demo_root/controller.cpp" \
    "$task_demo_root/host_clock.hpp" "$task_demo_root/device_monitor.hpp" "$task_demo_root/device_monitor.mm" \
    "$task_demo_repo/stream_lab/movie_io.hpp" "$task_demo_repo/stream_lab/movie_io.mm" \
    "$task_demo_root/Info.plist" "$task_demo_root/build.sh" "$task_demo_root/verify.sh" "$task_demo_root/README.md" \
    "$task_demo_executable" "$task_demo_app/Contents/_CodeSignature/CodeResources" "$task_demo_evidence/verify.status.json" \
    > "$task_demo_evidence/source-and-bundle.sha256"
printf 'PASS generated fixtures, native bundle, local signature and static silent policy\n'
