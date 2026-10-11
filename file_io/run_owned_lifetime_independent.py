#!/usr/bin/env python3
"""Review only fixed self-authored 08152cc wrapper/parser and synthetic tests."""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

COMMIT = "08152cc48fa0229ea2e0d592a1c6f201145f45b8"
EXPECTED = {
    "include/owned_reader.hpp": "c7d3f5ada30ddddd748c429f61f7bb4525504bccf0cc6d016325b8e60a13e937",
    "include/framed_reader.hpp": "d3d21e07110f57a19cd37d7e1fa61a922203a4c2ccb7c4988f60cb2e543abbd9",
    "include/strict_zlib.h": "8884a8a9bc6d8222320d93023d088cd18430017e479a282ca698f102c3eaf7a4",
    "src/framed_reader.cpp": "648bb362ff03f96afa0596cc5c84e628b1b666d78325d41099a70181eab5d68b",
    "src/strict_zlib.cpp": "1c13bf06ba9721a85000105f4b18763f88bfadf71626f2cae1127288c6fb2f0e",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--component-root", type=Path, default=Path(__file__).resolve().parent)
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists():
        parser.error("--output must be a fresh absolute directory")
    for rel, expected in EXPECTED.items():
        source = args.component_root / rel
        if not source.is_file() or sha(source) != expected:
            parser.error("fixed self-authored baseline missing or changed: " + rel)
    output = args.output
    output.mkdir(parents=True, exist_ok=False)
    frozen = output / "sources/file_io"
    sources = []
    for rel in EXPECTED:
        target = frozen / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(args.component_root / rel, target)
        sources.append({"path": "file_io/" + rel, "sha256": sha(target)})
    test_name = "tests/owned_lifetime_independent_tests.cpp"
    test = frozen / test_name
    test.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(Path(__file__).resolve().parent / test_name, test)
    sources.append({"path": "file_io/" + test_name, "sha256": sha(test)})
    runner = frozen / Path(__file__).name
    shutil.copyfile(Path(__file__), runner)
    sources.append({"path": "file_io/" + runner.name, "sha256": sha(runner)})
    report = {
        "schema": "OWNED_READER_INDEPENDENT_LIFETIME_V1", "baseline_commit": COMMIT,
        "scope": "Only fixed new self-authored C++ wrapper/parser/adapter, reviewer synthetic inputs and installed host zlib; no original/derived code or file, original profile, EXE/GameData, real save, UI/audio, main changes or push.",
        "sources": sources, "commands": [], "modes": [], "negative_object_transfer_probes": [],
        "ASan_run": False, "ASan_pass_claimed": False,
        "actual_expired_storage_read": False, "original_compatibility_claimed": False,
        "findings": [], "passed": False,
    }

    def run(name, command):
        started = time.monotonic()
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        stdout, stderr = output / (name + ".stdout"), output / (name + ".stderr")
        stdout.write_text(result.stdout)
        stderr.write_text(result.stderr)
        entry = {"name": name, "command": command, "exit_code": result.returncode,
                 "seconds": round(time.monotonic() - started, 3),
                 "stdout": stdout.name, "stdout_sha256": sha(stdout),
                 "stderr": stderr.name, "stderr_sha256": sha(stderr)}
        report["commands"].append(entry)
        return result

    try:
        sdk_result = run("installed-sdk", ["/usr/bin/xcrun", "--show-sdk-path"])
        if sdk_result.returncode:
            raise RuntimeError("installed SDK lookup failed")
        sdk = sdk_result.stdout.strip()
        flags = ["/usr/bin/clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                 "-isysroot", sdk, "-I", str(frozen / "include")]
        for mode, optimize in (("release", ["-O2"]), ("undefined", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"])):
            binary = output / ("owned-lifetime-" + mode)
            command = flags + optimize + [str(frozen / "src/strict_zlib.cpp"), str(frozen / "src/framed_reader.cpp"),
                                         str(test), "-lz", "-o", str(binary)]
            compiled = run("compile-" + mode, command)
            if compiled.returncode:
                raise RuntimeError("self-authored test compilation failed: " + mode)
            executed = run("execute-" + mode, [str(binary)])
            records = [json.loads(line) for line in executed.stdout.splitlines() if line]
            summaries = [item for item in records if item.get("summary")]
            checks = [item for item in records if "id" in item]
            if len(summaries) != 1:
                raise RuntimeError("missing unique runtime summary")
            summary = summaries[0]
            probes = [item for item in records if item.get("borrowed_alias_probe")]
            mode_passed = (executed.returncode == 0 and summary["failed"] == 0
                           and summary["passed"] == len(checks) and len({x["id"] for x in checks}) == len(checks)
                           and all(x["passed"] for x in checks) and len(probes) == 1
                           and probes[0]["actual_expired_read"] is False and probes[0]["guard_prevented"] == 1)
            report["modes"].append({"mode": mode, "summary": summary, "checks": checks,
                                    "borrowed_forbidden_alias_probe": probes, "binary_sha256": sha(binary), "passed": mode_passed})
            if not mode_passed:
                raise RuntimeError("independent runtime checks failed: " + mode)
        # Compile-only: prove deleted operations cannot be invoked, while the
        # positive header has already compiled and run above. No probe executed.
        probes = {
            "copy-construct": "OwnedFramedReader b(a);",
            "move-construct": "OwnedFramedReader b(std::move(a));",
            "copy-assign": "OwnedFramedReader b({}, p); b = a;",
            "move-assign": "OwnedFramedReader b({}, p); b = std::move(a);",
        }
        for name, expression in probes.items():
            source = output / ("forbidden-" + name + ".cpp")
            source.write_text('#include "owned_reader.hpp"\n#include <utility>\nusing namespace emperor::file_io;\n'
                              "void probe() { FrameProfile p{}; OwnedFramedReader a({}, p); " + expression + " }\n")
            result = run("forbidden-" + name, flags + ["-fsyntax-only", str(source)])
            prohibited = result.returncode != 0 and "deleted" in result.stderr
            report["negative_object_transfer_probes"].append({"operation": name, "compile_rejected": prohibited,
                                                             "source": source.name, "source_sha256": sha(source), "executed": False})
            if not prohibited:
                raise RuntimeError("deleted operation unexpectedly accepted or failed for unrelated cause: " + name)
        report["passed"] = True
    except (RuntimeError, subprocess.SubprocessError, ValueError) as exc:
        report["failure"] = str(exc)
    finally:
        (output / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({"baseline_commit": COMMIT, "passed": report["passed"],
                      "modes": [dict(mode=mode["mode"], **mode["summary"]) for mode in report["modes"]],
                      "forbidden_object_operations": len(report["negative_object_transfer_probes"]),
                      "actual_expired_storage_read": False, "ASan_run": False}, ensure_ascii=False))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
