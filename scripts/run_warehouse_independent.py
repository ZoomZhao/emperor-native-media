#!/usr/bin/env python3
"""Prepare declared warehouse tests, or run only against an explicitly pinned own frozen copy.

Prepare phase runs the independent reference oracle and syntax-checks declarations only.
It NEVER links or executes an unfinished warehouse implementation.
Every subprocess <=60 seconds; aggregate subprocess budget150 seconds.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time

BASELINE_CORE = "4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d"
MAIN = Path("/Users/zoomzhao/Workspace/emperor-native-media")
HEADERS = ["game_lab/city_simulation.hpp", "game_lab/city_state_io.hpp", "game_lab/city_game_state.hpp",
           "game_lab/emperor_assets.hpp", "game_lab/asset_limits.hpp", "game_lab/map_geometry.hpp",
           "game_lab/app/city_goal_feedback.hpp", "game_lab/app/city_goal_feedback_adapter.hpp",
           "game_lab/app/city_scenario.hpp", "game_lab/app/city_scenario_checkpoint.hpp"]
OWN = ["game_lab/warehouse_roads_reference.hpp", "game_lab/warehouse_reference_selftest.cpp",
       "game_lab/warehouse_roads_independent_tests.cpp", "scripts/run_warehouse_independent.py"]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=["prepare", "frozen"], required=True)
    parser.add_argument("--source-root", type=Path, required=True,
                        help="prepare: pinned c239a7b baseline; frozen: own immutable supplied final source copy")
    parser.add_argument("--output", type=Path, required=True, help="new task-owned evidence directory")
    parser.add_argument("--mode", choices=["release", "undefined"], default="release")
    parser.add_argument("--legacy-saves", type=Path, required=True)
    parser.add_argument("--expected-core-sha256")
    parser.add_argument("--expected-adapter-sha256")
    args = parser.parse_args()
    source = args.source_root.resolve()
    if source == MAIN or MAIN in source.parents:
        parser.error("Use your own immutable source copy; this runner refuses the main checkout")
    if args.phase == "frozen" and (not args.expected_core_sha256 or not args.expected_adapter_sha256):
        parser.error("Frozen execution requires the final core AND adapter SHA-256 supplied by the integrator")
    if args.phase == "prepare" and sha(source / HEADERS[0]) != BASELINE_CORE:
        parser.error("Preparation syntax scaffold accepts only the captured c239a7b baseline")
    if args.phase == "frozen":
        for path, expected in [(HEADERS[0], args.expected_core_sha256),
                               ("game_lab/app/city_goal_feedback_adapter.hpp", args.expected_adapter_sha256)]:
            if sha(source / path) != expected:
                parser.error("Frozen source pin mismatch: " + path)
        if sha(source / HEADERS[0]) == BASELINE_CORE:
            parser.error("Supplied baseline has no warehouse implementation; wait for final frozen core")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    review = Path(__file__).resolve().parents[1]
    snapshot = out / "source_snapshot"
    report = {"phase": args.phase, "mode": args.mode, "source_root": str(source),
              "implementation_tests_executed": False, "status": "IN_PROGRESS",
              "timeout_seconds": 60, "total_process_budget_seconds": 150,
              "source_files": {}, "legacy_inputs": {}, "processes": [], "cases": [], "artifacts": {}}
    entries = [(source, name) for name in HEADERS] + [(review, name) for name in OWN]
    for base, name in entries:
        target = snapshot / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((base / name).read_bytes())
        report["source_files"][name] = sha(target)
    for base, name in entries:
        if sha(base / name) != report["source_files"][name]:
            raise SystemExit("Source changed during capture; use an immutable source copy: " + name)
    legacy = out / "legacy_inputs"
    legacy.mkdir()
    for file in sorted(args.legacy_saves.resolve().glob("*.emp")):
        data = file.read_bytes()
        if len(data) == 65580 and data[:8] == b"EMP1\x01\x00\x00\x00":
            (legacy / file.name).write_bytes(data)
            report["legacy_inputs"][file.name] = sha(legacy / file.name)
    if len(report["legacy_inputs"]) != 30:
        raise SystemExit("Expected all30 previously authored canonical v1 inputs")
    deadline = time.monotonic() + 150

    def run(label, argv):
        remaining = deadline - time.monotonic()
        process_report = {"label": label, "argv": [str(a) for a in argv], "timeout_seconds": min(60, max(0, remaining))}
        report["processes"].append(process_report)
        start = time.monotonic()
        with (out / (label + ".stdout")).open("wb") as stdout, (out / (label + ".stderr")).open("wb") as stderr:
            if remaining <= 0:
                process_report.update(exit_code=None, timed_out=True)
                return False
            child = subprocess.Popen(process_report["argv"], cwd=out, stdout=stdout, stderr=stderr, start_new_session=True)
            try:
                process_report.update(exit_code=child.wait(timeout=min(60, remaining)), timed_out=False)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.wait(timeout=5)
                process_report.update(exit_code=child.returncode, timed_out=True)
        process_report["elapsed_seconds"] = round(time.monotonic() - start, 4)
        return process_report["exit_code"] == 0

    flags = ["-O2"] if args.mode == "release" else ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"]
    compiler = ["/usr/bin/clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror", *flags]
    binary = out / "independent-reference-only"
    ok = run("compile-reference", [*compiler, "-I", snapshot / "game_lab",
                                   snapshot / "game_lab/warehouse_reference_selftest.cpp", "-o", binary])
    if ok:
        ok = run("reference-only", [binary])
    if ok and args.phase == "prepare":
        # DECLARATIONS ONLY: retain original published bodies, add declared types/methods.
        # This directory is never linked, run, or supplied as a production core.
        interface = out / "DECLARATIONS-ONLY-NOT-IMPLEMENTATION"
        for name in HEADERS:
            dest = interface / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes((snapshot / name).read_bytes())
        for name in OWN:
            if name.startswith("game_lab/"):
                dest = interface / name
                dest.write_bytes((snapshot / name).read_bytes())
        core = interface / HEADERS[0]
        text = core.read_text()
        text = text.replace("StaffedServices = 3 };", "StaffedServices = 3, RoadWarehouses = 4 };")
        text = text.replace("Watchtower = 7\n", "Watchtower = 7, Warehouse = 8\n")
        text = text.replace("int totalWatchtowers = 0;", "int totalWatchtowers = 0;\n    int totalWarehouses=0, grainStorageCapacity=0;")
        declarations = """// DECLARATIONS ONLY FROM CONTRACT, NEVER EXECUTED.
struct CityWarehouseRules { static constexpr int capacity=600; static constexpr uint32_t initialX=5,initialY=2; };
struct CityLogisticsMonth { bool valid=false; int beginStored=0,produced=0,delivered=0,noRouteDiscard=0,fullDiscard=0,consumed=0,unmetDemand=0,endStored=0; };
struct CityLogisticsOverview { int warehouses=0,capacity=0,stored=0,connectedFarms=0,disconnectedFarms=0; };
"""
        text = text.replace("class CitySimulation {", declarations + "class CitySimulation {")
        text = text.replace("class CitySimulation {\npublic:", """class CitySimulation {
public:
    int warehouseStockAt(uint32_t,uint32_t) const;
    bool farmHasWarehouseRoute(uint32_t,uint32_t) const;
    CityLogisticsOverview logisticsOverview() const;
    const CityLogisticsMonth& lastLogisticsMonth() const;
    CityLogisticsMonth nextMonthLogistics() const;
    bool migrateToRoadWarehouses(uint32_t,uint32_t,std::string&);
""")
        core.write_text(text)
        goal = interface / "game_lab/app/city_goal_feedback.hpp"
        goal.write_text(goal.read_text().replace("StaffedServicesV1      //", "StaffedServicesV1, RoadWarehousesV1 //"))
        report["declarations_only_headers"] = {str(path.relative_to(interface)): sha(path)
                                                 for path in sorted(interface.rglob("*.hpp"))}
        ok = run("syntax-declarations-only", [*compiler, "-I", interface / "game_lab", "-I", snapshot / "game_lab",
                                               "-fsyntax-only", interface / "game_lab/warehouse_roads_independent_tests.cpp"])
        report["status"] = "READY_TO_RETEST_FROZEN_IMPLEMENTATION" if ok else "PREPARATION_ERROR"
        report["preparation_complete"] = ok
    elif ok:
        target = out / "warehouse-independent-tests"
        ok = run("compile-frozen", [*compiler, "-I", snapshot / "game_lab",
                                    snapshot / "game_lab/warehouse_roads_independent_tests.cpp", "-o", target])
        if ok:
            report["implementation_tests_executed"] = True
            ok = run("frozen-test", [target, out / "fixtures", legacy])
            for line in (out / "frozen-test.stdout").read_text().splitlines():
                fields = line.split("\t", 3)
                if len(fields) == 4 and fields[0] in ["PASS", "FAIL", "SCOPE", "OBSERVE"]:
                    report["cases"].append(dict(zip(["status", "id", "expectation_source", "detail"], fields)))
        report["passed"] = ok
        report["status"] = "FROZEN_TESTS_PASSED" if ok else "FROZEN_TESTS_FAILED"
    else:
        report["status"] = "REFERENCE_OR_PREPARATION_ERROR"
    for file in sorted(out.rglob("*")):
        if file.is_file():
            report["artifacts"][str(file.relative_to(out))] = {"sha256": sha(file), "bytes": file.stat().st_size}
    (out / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "implementation_tests_executed": report["implementation_tests_executed"],
                      "report": str(out / "report.json")}, ensure_ascii=False))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
