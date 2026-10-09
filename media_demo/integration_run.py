#!/usr/bin/python3
"""One bounded, self-authored integration configuration per invocation (<=55s)."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
CONFIG = sys.argv[1] if len(sys.argv) == 2 else ""
if CONFIG not in ("release", "undefined"):
    raise SystemExit("usage: python3 integration_run.py release|undefined")
DEADLINE = time.monotonic() + 55
OUT = ROOT / "evidence" / ("integration-" + CONFIG)
if OUT.exists():
    history = ROOT / "evidence" / ("previous-integration-" + CONFIG + "-" + str(time.time_ns()))
    OUT.rename(history)
OUT.mkdir(parents=True)
SOURCES = [ROOT / p for p in ("controller.hpp", "controller.cpp", "host_clock.hpp", "device_monitor.hpp", "device_monitor.mm", "integration_tests.mm", "integration_run.py")]
SOURCES += [ROOT.parent / "stream_lab/movie_io.hpp", ROOT.parent / "stream_lab/movie_io.mm"]

def run(name, command, env=None):
    started = time.monotonic()
    with (OUT / (name + ".stdout.log")).open("wb") as stdout, (OUT / (name + ".stderr.log")).open("wb") as stderr:
        process = subprocess.Popen(command, stdout=stdout, stderr=stderr, env=env, start_new_session=True)
        try:
            code = process.wait(timeout=max(0.1, DEADLINE - time.monotonic()))
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)  # Only this script's own subprocess family.
            process.wait()
            code = 124
    (OUT / (name + ".status.json")).write_text(json.dumps({"returncode": code, "seconds": time.monotonic() - started}) + "\n")
    if code:
        failure = OUT / "failure-sources"
        failure.mkdir(exist_ok=True)
        for source in SOURCES:
            shutil.copy2(source, failure / source.name)
        print((OUT / (name + ".stderr.log")).read_text(), end="")
        raise SystemExit(code)

run("fixture-and-existing-evidence", ["/usr/bin/shasum", "-a", "256", "-c", str(ROOT.parent / ".generated/source-and-fixtures.sha256")])
binary = OUT / ("integration-tests-" + CONFIG)
flags = ["-O2"] if CONFIG == "release" else ["-O1", "-g", "-fsanitize=undefined", "-fno-omit-frame-pointer"]
command = ["/usr/bin/clang++", "-std=c++17", "-fobjc-arc", "-mmacosx-version-min=15.0", "-Wall", "-Wextra", "-Wpedantic", "-Werror"] + flags
command += [str(p) for p in (ROOT / "controller.cpp", ROOT / "device_monitor.mm", ROOT / "integration_tests.mm", ROOT.parent / "stream_lab/movie_io.mm", ROOT.parent / "stream_lab/presenter.cpp", ROOT.parent / "video_lab/stream.cpp", ROOT.parent / "audio_lab/audio.cpp", ROOT.parent / "surface_lab/surface.cpp", ROOT.parent / "metal_lab/backend.mm")]
for framework in ("Foundation", "CoreAudio", "AVFoundation", "AudioToolbox", "CoreMedia", "CoreVideo", "Metal"):
    command += ["-framework", framework]
command += ["-o", str(binary)]
run("build", command)
env = os.environ.copy()
env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
fixture = ROOT.parent / ".generated/self_authored_8s.mov"
run("run", [str(binary), str(fixture), str(OUT)], env)
summary = json.loads((OUT / "summary.json").read_text())
if not summary["passed"]:
    raise SystemExit(1)
manifest = SOURCES + [binary, fixture, OUT / "summary.json", OUT / "timeline.jsonl", OUT / "run.status.json"]
(OUT / "source-and-result.sha256").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p) + "\n" for p in manifest))
print("PASS " + CONFIG + ": " + json.dumps(summary, ensure_ascii=False))
