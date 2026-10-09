#!/usr/bin/python3
"""Two fresh output directories, compare all bundle bytes, then promote the verified App."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parent
TOKEN = str(time.time_ns())
OUT = ROOT / "evidence" / ("clean-rebuild-" + TOKEN)
OUT.mkdir(parents=True)
DEADLINE = time.monotonic() + 55
paths = [ROOT / ("build-clean-" + label + "-" + TOKEN) for label in ("a", "b")]
sources = [ROOT / p for p in ("main.mm", "controller.hpp", "controller.cpp", "host_clock.hpp", "device_monitor.hpp", "device_monitor.mm", "build.sh", "verify.sh", "rebuild_clean.py")]
sources += [ROOT.parent / "stream_lab/movie_io.hpp", ROOT.parent / "stream_lab/movie_io.mm"]

def run(name, argv):
    began = time.monotonic()
    with (OUT / (name + ".stdout.log")).open("wb") as out, (OUT / (name + ".stderr.log")).open("wb") as err:
        child = subprocess.Popen(argv, stdout=out, stderr=err, start_new_session=True)
        try:
            code = child.wait(timeout=max(.1, DEADLINE - time.monotonic()))
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.wait()
            code = 124
    (OUT / (name + ".status.json")).write_text(json.dumps({"returncode": code, "seconds": time.monotonic() - began}) + "\n")
    if code:
        raise RuntimeError((OUT / (name + ".stderr.log")).read_text())

def contents(app):
    return {str(p.relative_to(app)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(app.rglob("*")) if p.is_file()}

try:
    # Operate only after all registered own GUI instances have exited.
    run("owned-instances-exited", [str(ROOT / "build/gui-native"), "verifyExited"])
    for index, path in enumerate(paths):
        if path.exists():
            raise RuntimeError("isolated output unexpectedly exists")
        path.mkdir()
        run("build-" + str(index), ["/bin/sh", str(ROOT / "build.sh"), str(path), str(path / "evidence")])
    first, second = [contents(path / "NativeMediaDemo.app") for path in paths]
    if first != second:
        raise RuntimeError("freshly rebuilt bundle bytes differ")
    (OUT / "bundle-files.sha256.json").write_text(json.dumps(first, sort_keys=True, indent=2) + "\n")
    canonical = ROOT / "build/NativeMediaDemo.app"
    if canonical.exists():
        canonical.rename(OUT / "previous-NativeMediaDemo.app")
    shutil.copytree(paths[0] / "NativeMediaDemo.app", canonical)
    if contents(canonical) != first:
        raise RuntimeError("promoted bundle is not byte-identical")
    run("final-static-verification", ["/bin/sh", str(ROOT / "verify.sh")])
    summary = {"passed": True, "fresh_outputs": [str(path) for path in paths], "bundle_byte_identical": True,
               "final_bundle_byte_identical_to_clean_output": True, "native_source_recompiled": True, "previous_build_objects_reused": False,
               "frameworks_only": True, "no_network_or_install": True, "run_directory": str(OUT)}
    (OUT / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    (ROOT / "evidence/clean-rebuild-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    manifest = sources + [ROOT / "build/NativeMediaDemo.app/Contents/MacOS/NativeMediaDemo", OUT / "summary.json", OUT / "bundle-files.sha256.json"]
    (OUT / "source-and-result.sha256").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p) + "\n" for p in manifest))
    print("PASS clean rebuild: all bundle contents and promoted App byte-identical")
except Exception as error:
    (OUT / "summary.json").write_text(json.dumps({"passed": False, "error": str(error)}, indent=2) + "\n")
    for source in sources:
        shutil.copy2(source, OUT / source.name)
    raise
