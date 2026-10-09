#!/usr/bin/python3
"""One real native reliability run per <=55-second process; no installs/network."""
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
if len(sys.argv) != 3 or sys.argv[1] not in ("release", "undefined") or sys.argv[2] not in ("long", "faults", "metadata"):
    raise SystemExit("usage: reliability_run.py release|undefined long|faults|metadata")
CONFIG, MODE = sys.argv[1:]
OUT = ROOT / "evidence" / ("reliability-" + CONFIG + "-" + MODE)
if OUT.exists():
    OUT.rename(ROOT / "evidence" / ("previous-" + OUT.name + "-" + str(time.time_ns())))
OUT.mkdir(parents=True)
DEADLINE = time.monotonic() + 55
SOURCES = [ROOT / p for p in ("controller.cpp", "controller.hpp", "host_clock.hpp", "device_monitor.mm", "device_monitor.hpp", "reliability_tests.mm", "reliability_run.py")]
SOURCES += [ROOT.parent / "stream_lab/movie_io.hpp", ROOT.parent / "stream_lab/movie_io.mm"]

def run(name, argv):
    started = time.monotonic()
    with (OUT / (name + ".stdout.log")).open("wb") as out, (OUT / (name + ".stderr.log")).open("wb") as error:
        env = os.environ.copy()
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        process = subprocess.Popen(argv, stdout=out, stderr=error, env=env, start_new_session=True)
        try:
            code = process.wait(timeout=max(.1, DEADLINE - time.monotonic()))
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            code = 124
    (OUT / (name + ".status.json")).write_text(json.dumps({"returncode": code, "seconds": time.monotonic() - started}) + "\n")
    if code:
        failure = OUT / "failure-sources"
        failure.mkdir()
        for source in SOURCES:
            shutil.copy2(source, failure / source.name)
        print((OUT / (name + ".stderr.log")).read_text(), end="")
        raise SystemExit(code)

run("fixture-integrity", ["/usr/bin/shasum", "-a", "256", "-c", str(ROOT.parent / ".generated/source-and-fixtures.sha256")])
# This immutable run directory owns its measured binary. Debug link maps can
# differ between builds, and a shared output slot would invalidate old hashes.
binary = OUT / ("reliability-tests-" + CONFIG)
command = ["/usr/bin/clang++", "-std=c++17", "-fobjc-arc", "-mmacosx-version-min=15.0", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
command += ["-O2"] if CONFIG == "release" else ["-O1", "-g", "-fsanitize=undefined", "-fno-omit-frame-pointer"]
command += [str(p) for p in (ROOT / "reliability_tests.mm", ROOT / "controller.cpp", ROOT / "device_monitor.mm", ROOT.parent / "stream_lab/movie_io.mm", ROOT.parent / "stream_lab/presenter.cpp", ROOT.parent / "video_lab/stream.cpp", ROOT.parent / "audio_lab/audio.cpp", ROOT.parent / "surface_lab/surface.cpp", ROOT.parent / "metal_lab/backend.mm")]
for framework in ("Foundation", "AVFoundation", "CoreAudio", "AudioToolbox", "CoreMedia", "CoreVideo", "Metal"):
    command += ["-framework", framework]
command += ["-o", str(binary)]
run("build", command)
run("run", [str(binary), MODE, str(OUT), str(ROOT.parent / ".generated/reliability-fixtures")])
summary = json.loads((OUT / "summary.json").read_text())
if not summary["passed"]:
    raise SystemExit(1)
manifest = SOURCES + [binary, OUT / "summary.json", OUT / "timeline.jsonl", OUT / "run.status.json"]
(OUT / "source-and-result.sha256").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p) + "\n" for p in manifest))
print("PASS " + CONFIG + " " + MODE + ": " + json.dumps(summary, ensure_ascii=False))
