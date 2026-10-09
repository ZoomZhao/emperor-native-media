#!/usr/bin/env python3
"""Bounded, silent replay in one native process using one Controller instance."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    length = parser.add_mutually_exclusive_group()
    length.add_argument("--cycles", type=int, choices=(2, 3, 4), default=None)
    length.add_argument("--duration", type=int, choices=(16, 24, 32), default=None,
                        help="Total playback seconds; entire invocation remains bounded to 55s")
    parser.add_argument("--config", choices=("release", "undefined"), default="release")
    parser.add_argument("--fixture", default=".generated/self_authored_8s.mov",
                        help="Only this checkout's generated 8s fixture is accepted")
    args = parser.parse_args()
    fixture = (ROOT / args.fixture).resolve()
    if fixture != (ROOT / ".generated/self_authored_8s.mov").resolve():
        parser.error("only the self-authored generated 8s fixture is permitted")
    if not fixture.is_file():
        parser.error("fixture missing; run make fixtures first")
    cycles = args.cycles or (args.duration // 8 if args.duration else 3)
    out = ROOT / ".local-results" / ("endurance-" + args.config + "-" + str(time.time_ns()))
    out.mkdir(parents=True)
    deadline = time.monotonic() + 55

    def run(name, command):
        env = os.environ.copy()
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        start = time.monotonic()
        with (out / (name + ".stdout.log")).open("wb") as stdout, (out / (name + ".stderr.log")).open("wb") as stderr:
            child = subprocess.Popen(command, stdout=stdout, stderr=stderr, env=env, start_new_session=True)
            try:
                code = child.wait(timeout=max(.1, deadline - time.monotonic()))
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.wait()
                code = 124
        (out / (name + ".status.json")).write_text(json.dumps({"returncode": code, "seconds": time.monotonic() - start}) + "\n")
        if code:
            raise RuntimeError(name + " failed (" + str(code) + "): " + (out / (name + ".stderr.log")).read_text())

    sources = [ROOT / name for name in (
        "media_demo/endurance_tests.mm", "media_demo/controller.cpp", "media_demo/controller.hpp",
        "media_demo/host_clock.hpp", "media_demo/device_monitor.mm", "media_demo/device_monitor.hpp",
        "stream_lab/movie_io.mm", "stream_lab/movie_io.hpp", "stream_lab/presenter.cpp", "stream_lab/presenter.hpp",
        "video_lab/stream.cpp", "video_lab/stream.hpp", "audio_lab/audio.cpp", "audio_lab/audio.hpp",
        "surface_lab/surface.cpp", "surface_lab/surface.hpp", "metal_lab/backend.mm", "metal_lab/backend.hpp",
        "scripts/endurance_test.py")]
    binary = out / "endurance-runner"
    try:
        run("fixture-integrity", ["/usr/bin/shasum", "-a", "256", "-c", str(ROOT / ".generated/source-and-fixtures.sha256")])
        command = ["/usr/bin/clang++", "-std=c++17", "-fobjc-arc", "-mmacosx-version-min=15.0",
                   "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        command += ["-O2"] if args.config == "release" else ["-O1", "-g", "-fsanitize=undefined", "-fno-omit-frame-pointer"]
        command += [str(path) for path in sources if path.suffix in (".mm", ".cpp")]
        for framework in ("Foundation", "AVFoundation", "CoreAudio", "AudioToolbox", "CoreMedia", "CoreVideo", "Metal"):
            command += ["-framework", framework]
        run("build", command + ["-o", str(binary)])
        run("replay", [str(binary), str(ROOT), str(out), str(cycles)])
        summary = json.loads((out / "summary.json").read_text())
        if (not summary.get("passed") or not summary.get("same_process") or
                summary.get("controller_instances") != 1 or summary.get("cycles_completed") != cycles or
                len(summary.get("records", [])) != cycles or
                not all(row.get("resources_zero") for row in summary["records"])):
            raise RuntimeError("incomplete or failed native replay evidence")
        paths = sources + [fixture, binary, out / "summary.json", out / "timeline.jsonl", out / "replay.status.json"]
        (out / "source-and-result.sha256").write_text("".join(
            hashlib.sha256(path.read_bytes()).hexdigest() + "  " + str(path) + "\n" for path in paths))
        print("PASS same-process replay: " + json.dumps(summary, ensure_ascii=False))
    except Exception as error:
        (out / "failure.json").write_text(json.dumps({"passed": False, "error": str(error)}) + "\n")
        raise SystemExit(str(error))


if __name__ == "__main__":
    main()
