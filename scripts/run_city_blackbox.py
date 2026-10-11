#!/usr/bin/env python3
"""Capture selected city headers, compile and run without touching source checkout.

Each subprocess is capped at 60 seconds. Total subprocess budget: 150 seconds.
All inputs are synthetic. No App, media, network, screen or audio device access.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time

HEADERS = ["game_lab/city_simulation.hpp", "game_lab/city_state_io.hpp",
           "game_lab/emperor_assets.hpp", "game_lab/asset_limits.hpp", "game_lab/map_geometry.hpp"]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True, help="new task-owned directory")
    parser.add_argument("--profile", choices=["baseline", "strict-labor", "invariants"], required=True)
    parser.add_argument("--mode", choices=["release", "undefined"], default="release")
    parser.add_argument("--with-existing", action="store_true")
    args = parser.parse_args()
    source = args.source_root.resolve()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    patch_root = Path(__file__).resolve().parents[1]
    snapshot = out / "source_snapshot"
    manifest = {"source_root": str(source), "profile": args.profile, "mode": args.mode,
                "process_timeout_seconds": 60, "total_process_budget_seconds": 150,
                "source_files": {}, "processes": [], "cases": [], "artifacts": {}}
    entries = [(source, item) for item in HEADERS]
    entries += [(patch_root, "game_lab/city_blackbox_tests.cpp"), (patch_root, "scripts/run_city_blackbox.py")]
    if args.with_existing:
        entries += [(source, "game_lab/city_simulation_tests.cpp"),
                    (source, "scripts/self_fixture_evidence.hpp")]
    for root, item in entries:
        path = root / item
        payload = path.read_bytes()
        destination = snapshot / item
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(payload)
        manifest["source_files"][item] = hashlib.sha256(payload).hexdigest()
    # Refuse a source capture that changed while its dependencies were copied.
    for root, item in entries:
        if sha(root / item) != manifest["source_files"][item]:
            manifest["capture_error"] = "source changed during capture: " + item
            (out / "report.json").write_text(json.dumps(manifest, indent=2) + "\n")
            raise SystemExit("source changed during capture; use an immutable snapshot")
    deadline = time.monotonic() + 150

    def command(label, argv, cwd=out):
        remaining = deadline - time.monotonic()
        entry = {"label": label, "argv": [str(a) for a in argv], "timeout_seconds": min(60, max(0, remaining))}
        manifest["processes"].append(entry)
        start = time.monotonic()
        with (out / (label + ".stdout")).open("wb") as stdout, (out / (label + ".stderr")).open("wb") as stderr:
            if remaining <= 0:
                entry.update(exit_code=None, timed_out=True, elapsed_seconds=0)
                return False
            process = subprocess.Popen(entry["argv"], cwd=cwd, stdout=stdout, stderr=stderr, start_new_session=True)
            try:
                entry["exit_code"] = process.wait(timeout=min(60, remaining))
                entry["timed_out"] = False
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
                entry.update(exit_code=process.returncode, timed_out=True)
        entry["elapsed_seconds"] = round(time.monotonic() - start, 4)
        return entry["exit_code"] == 0

    flags = ["-O2"] if args.mode == "release" else ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"]
    compiler = ["/usr/bin/clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror", *flags,
                "-I", str(snapshot / "game_lab")]
    compiled = command("compile-blackbox", [*compiler, snapshot / "game_lab/city_blackbox_tests.cpp", "-o", out / "city-blackbox-tests"])
    ok = compiled
    if compiled:
        ok = command("blackbox", [out / "city-blackbox-tests", out / "fixtures", args.profile])
        for line in (out / "blackbox.stdout").read_text().splitlines():
            fields = line.split("\t", 3)
            if len(fields) == 4 and fields[0] in ["PASS", "FAIL", "SKIP", "UNIMPLEMENTED"]:
                manifest["cases"].append(dict(zip(["status", "id", "expectation_source", "detail"], fields)))
    if args.with_existing:
        compiled_existing = command("compile-existing", [*compiler, snapshot / "game_lab/city_simulation_tests.cpp", "-o", out / "city-existing-tests"])
        ok = compiled_existing and ok
        if compiled_existing:
            ok = command("existing", [out / "city-existing-tests", out / "existing-fixtures"]) and ok
    for path in sorted(out.rglob("*")):
        if path.is_file():
            manifest["artifacts"][str(path.relative_to(out))] = {"sha256": sha(path), "bytes": path.stat().st_size}
    manifest["passed"] = ok
    (out / "report.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    for case in manifest["cases"]:
        if case["status"] == "FAIL":
            print(case["id"] + ": " + case["detail"])
    print(json.dumps({"passed": ok, "profile": args.profile, "mode": args.mode,
                      "cases": {state: sum(c["status"] == state for c in manifest["cases"])
                                for state in ["PASS", "FAIL", "SKIP", "UNIMPLEMENTED"]},
                      "report": str(out / "report.json")}, ensure_ascii=False))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
