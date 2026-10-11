#!/usr/bin/env python3
"""Build only the self-authored host reader; consume independent synthetic data.

This script neither opens original binaries/GameData nor runs repository games.
Frozen pending expectations are retained, with separate modern policy checks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def sha(data):
    return hashlib.sha256(data).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--mode", choices=("release", "undefined"), default="release")
    parser.add_argument("--corpus", type=Path)
    args = parser.parse_args()
    output = args.output
    if not output.is_absolute() or output.exists():
        parser.error("output must be a fresh absolute directory")
    component = Path(__file__).resolve().parent
    root = component.parent
    output.mkdir(parents=True)
    start = time.monotonic()
    commands, results = [], []

    def command(argv, environment=None, timeout=60):
        p = subprocess.run([str(a) for a in argv], cwd=root, env=environment,
                           capture_output=True, text=True, timeout=timeout)
        commands.append({"argv": [str(a) for a in argv], "exit_code": p.returncode,
                         "stderr": p.stderr})
        if p.returncode != 0:
            raise RuntimeError("command failed: " + repr(argv) + "\n" + p.stderr)
        return p.stdout

    def check(identifier, category, action):
        try:
            detail = action()
            results.append({"id": identifier, "category": category, "passed": True, "detail": detail})
        except Exception as error:
            results.append({"id": identifier, "category": category, "passed": False,
                            "error": str(error)})

    command(["/bin/sh", component / "build_driver.sh", output / "build", args.mode])
    driver = output / "build/fixture_driver"
    runtime = json.loads(command([driver, "--version"]))
    corpus = args.corpus
    if corpus is None:
        corpus = output / "corpus"
        command([sys.executable, "-B", root / "tests/original_file_fixtures/generate_fixtures.py",
                 "--output", corpus])
    corpus = corpus.resolve()
    manifest_bytes = (corpus / "manifest.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    assert manifest["schema"] == "EMPEROR_SYNTHETIC_BLOCK_FIXTURES_V1" and manifest["synthetic_only"] is True
    assert manifest["generation_source_sha256"] == sha((root / "tests/original_file_fixtures/generate_fixtures.py").read_bytes())
    assert manifest["contract_metadata_sha256"] == sha((root / "tests/original_file_fixtures/source_contract.json").read_bytes())
    cases = {c["id"]: c for c in manifest["cases"]}
    assert len(cases) == manifest["case_count"] == 166

    def fixture(spec):
        p = (corpus / spec["path"]).resolve()
        assert corpus in p.parents and p.is_file() and not p.is_symlink()
        data = p.read_bytes()
        assert len(data) == spec["bytes"] and sha(data) == spec["sha256"]
        return p, data

    for case in cases.values():
        fixture(case["input"])
        if case["expected_output"] is not None:
            fixture(case["expected_output"])
    head = int.from_bytes(b"IO01", "little")  # Invented outer test token only.
    zipped = int.from_bytes(bytes.fromhex(manifest["compressed_marker_hex"]), "little")
    raw = int.from_bytes(bytes.fromhex(manifest["raw_marker_hex"]), "little")
    chunks = (None, 1, 2, 3, 4, 7, 4096)
    probes = output / "codec-slices"
    probes.mkdir()

    def invoke(mode, path, capacity=36144, chunk=None, max_input=1048576,
               max_output=1048576, max_blocks=1024, failure=None, exceptions=False):
        environment = dict(os.environ)
        environment.pop("EMFR_TEST_CHUNK_SIZE", None)
        environment.pop("EMFR_TEST_FAIL_AFTER", None)
        environment.pop("EMFR_TEST_STREAM_EXCEPTIONS", None)
        if chunk is not None:
            environment["EMFR_TEST_CHUNK_SIZE"] = str(chunk)
        if failure is not None:
            environment["EMFR_TEST_FAIL_AFTER"] = str(failure)
        if exceptions:
            environment["EMFR_TEST_STREAM_EXCEPTIONS"] = "1"
        if mode == "codec":
            argv = [driver, mode, path, capacity, 36144, capacity]
        else:
            argv = [driver, mode, path, head, zipped, raw, 36144, capacity,
                    max_input, max_output, max_blocks]
        return json.loads(command(argv, environment, timeout=5))

    def summarize(value):
        # Raw output remains in controlled fixtures; receipts retain hashes.
        if isinstance(value, dict):
            result = {k: summarize(v) for k, v in value.items() if k != "output_hex"}
            if "output_hex" in value:
                data = bytes.fromhex(value["output_hex"])
                result["output_bytes"] = len(data)
                result["output_sha256"] = sha(data)
            return result
        if isinstance(value, list):
            return [summarize(v) for v in value]
        return value

    def block_test(case, chunk):
        spec = case["expected"]
        observations = case["trusted_codec_observations_not_native_results"]
        capacity = observations[0]["observation"]["capacity"] if observations else 36144
        value = invoke("blocks", fixture(case["input"])[0], capacity, chunk)
        first = value["events"][0]
        assert value["sticky_terminal"], "terminal result/caller buffer changed on repeat"
        assert first["codec_calls"] == spec["codec_calls"], (first, spec)
        if spec["block_outcome"] == "accept":
            assert first["status"] == "frame", first
            expected = fixture(case["expected_output"])[1]
            assert bytes.fromhex(first["output_hex"]) == expected
            assert first["output_available"] == spec["published_available"] == len(expected)
            layout = case["construction_layout_not_reader_result"]
            assert first["input_offset"] == layout["payload_offset"] + layout["declared_n"]
            if layout["kind"] == "zlib":
                assert first["opaque_word"] == layout["opaque_word"]
                assert first["codec"]["status"] == "ok" and first["codec"]["codec_status"] == 1
                reference = observations[0]["observation"]
                assert first["codec"]["input_consumed"] == reference["codec_consumed_bytes"]
                assert first["codec"]["trailing_input"] == reference["unused_data_bytes"]
        else:
            reason = spec["reason"]
            expected_status = "truncated" if reason.startswith("short_") else (
                "input_limit" if reason.startswith("input_limit") else (
                    "output_limit" if reason == "output_limit" else "codec_error"))
            assert first["status"] == expected_status, (first, expected_status)
            assert first["output_available"] == 0 and first["destination_unchanged"]
            assert not first["output_hex"] and first["codec"]["output_available"] == 0
            if reason == "missing_stream_end":
                assert first["codec"]["status"] == "truncated"
            if reason == "dictionary_required":
                assert first["codec"]["status"] == "dictionary_required"
                layout = case["construction_layout_not_reader_result"]
                assert first["codec"]["input_consumed"] == 6
                assert first["codec"]["trailing_input"] == layout["declared_n"] - 6
            if case["id"] == "bad_adler_after_output_prefix":
                assert first["codec"]["output_produced"] > 0, "did not exercise private failed prefix"
        return summarize(first)

    for case in cases.values():
        if case["expected"]["status"] != "ready_block_assertion":
            continue
        for chunk in chunks:
            check(case["id"] + "/chunk=" + str(chunk), "independent_single_block", lambda c=case, n=chunk: block_test(c, n))
        for index, probe in enumerate(case["trusted_codec_observations_not_native_results"]):
            _, data = fixture(case["input"])
            observation = probe["observation"]
            payload = data[probe["payload_offset"]:probe["payload_offset"] + observation["payload_bytes"]]
            path = probes / (case["id"] + ".bin")
            path.write_bytes(payload)

            def codec_test(c=case, p=path, obs=observation):
                value = invoke("codec", p, obs["capacity"])
                result = value["result"]
                assert value["guards_ok"]
                if c["expected"]["block_outcome"] == "accept":
                    assert result["status"] == "ok"
                    assert bytes.fromhex(value["output_hex"]) == fixture(c["expected_output"])[1]
                    assert result["input_consumed"] == obs["codec_consumed_bytes"]
                    assert result["trailing_input"] == obs["unused_data_bytes"]
                else:
                    assert result["status"] != "ok" and result["output_available"] == 0
                    assert value["destination_unchanged"] and not value["output_hex"]
                    if c["id"] == "fdict_no_external_dictionary":
                        assert result["input_consumed"] == 6 and result["trailing_input"] == obs["payload_bytes"] - 6
                    if c["id"] == "bad_adler_after_output_prefix":
                        assert result["status"] == "invalid_data" and result["output_produced"] > 0
                return summarize(value)
            check(case["id"] + "/codec=" + str(index), "host_adapter_guards", codec_test)

    expected_small = fixture(cases["raw_small"]["expected_output"])[1]
    expected_dynamic = fixture(cases["zlib_dynamic"]["expected_output"])[1]
    # These are explicit modern frame-event policies, not replacement original
    # whole-file expectations. The immutable 15 pending entries stay pending.
    policies = {
        "physical_eof_before_marker": ([], "truncated", {}),
        "unknown_complete_marker": ([], "unknown_marker", {}),
        "whole_frame_then_file_junk": ([expected_small], "unknown_marker", {}),
        "multiblock_raw_raw": ([expected_small, expected_small], "end", {}),
        "multiblock_zlib_zlib": ([expected_small, expected_dynamic], "end", {}),
        "multiblock_raw_zlib": ([expected_small, expected_small], "end", {}),
        "multiblock_zlib_raw": ([expected_small, expected_small], "end", {}),
        "multiblock_good_then_corrupt": ([expected_small], "truncated", {}),
        "multiblock_empty_then_nonempty": ([b"", expected_small], "end", {}),
        "multiblock_empty_128": ([b""] * 64, "block_limit", {"max_blocks": 64}),
        "aggregate_budget_two_blocks": ([expected_small], "output_limit", {"max_output": len(expected_small)}),
    }
    for identifier, (outputs, terminal, options) in policies.items():
        for chunk in chunks:
            def policy_test(name=identifier, expected=outputs, status=terminal, opts=options, n=chunk):
                value = invoke("blocks", fixture(cases[name]["input"])[0], chunk=n, **opts)
                events = value["events"]
                assert [bytes.fromhex(e["output_hex"]) for e in events[:-1]] == expected
                assert all(e["status"] == "frame" for e in events[:-1])
                assert events[-1]["status"] == status and events[-1]["output_available"] == 0
                assert events[-1]["destination_unchanged"] and value["sticky_terminal"]
                assert events[-1]["total_output"] == sum(map(len, expected))
                assert events[-1]["blocks_completed"] == len(expected)
                return summarize(value)
            check(identifier + "/chunk=" + str(chunk), "modern_stream_policy_only", policy_test)

    # The only extra byte construction is a synthetic outer header around an
    # independently supplied block, not a second fixture generator.
    source = fixture(cases["raw_small"]["input"])[1]
    container = output / "synthetic-container.bin"
    container.write_bytes(head.to_bytes(4, "little") + source)
    empty_container = output / "synthetic-empty-container.bin"
    empty_container.write_bytes(head.to_bytes(4, "little"))

    def exact_input_budget():
        value = invoke("frames", container, max_input=container.stat().st_size)
        assert [e["status"] for e in value["events"]] == ["frame", "end"]
        assert value["sticky_terminal"]
        return summarize(value)
    check("container/eof-at-exact-input-budget", "modern_container_policy_only", exact_input_budget)

    def empty_container_test():
        value = invoke("frames", empty_container, max_input=4, max_blocks=0, max_output=0)
        assert [e["status"] for e in value["events"]] == ["end"] and value["sticky_terminal"]
        return summarize(value)
    check("container/empty-header-is-end", "modern_container_policy_only", empty_container_test)
    def api_test():
        value = json.loads(command([driver, "api"]))
        assert value["token_unchanged"]
        assert [r["status"] for r in value["results"]] == [
            "invalid_argument", "invalid_argument", "input_limit", "output_limit", "truncated"]
        assert all(r["output_available"] == 0 for r in value["results"])
        return value
    check("adapter/null-and-host-integer-limits", "host_api_rejection", api_test)
    for identifier, terminal in (("raw_small", "end"), ("raw_small_cut_001", "truncated")):
        def exception_eof_test(name=identifier, expected=terminal):
            value = invoke("blocks", fixture(cases[name]["input"])[0], chunk=1, exceptions=True)
            assert value["events"][-1]["status"] == expected and value["sticky_terminal"]
            return summarize(value)
        check("transport/exception-mask/" + identifier, "injected_io_error", exception_eof_test)
    for stop in (0, 1, 3, 4, 5, 8, 12, len(source) - 1):
        for exceptions in (False, True):
            def fault_test(offset=stop, mask=exceptions):
                value = invoke("blocks", fixture(cases["raw_small"]["input"])[0], chunk=1,
                               failure=offset, exceptions=mask)
                assert value["events"][0]["status"] == "io_error"
                assert value["events"][0]["destination_unchanged"] and value["sticky_terminal"]
                return summarize(value)
            check("transport/throw-at=" + str(stop) + "/exception-mask=" + str(exceptions),
                  "injected_io_error", fault_test)

    remaining_api_pending = [c["id"] for c in cases.values()
                             if c["expected"]["status"] == "pending_evidence" and c["id"] not in policies]
    failures = [r for r in results if not r["passed"]]
    source_hashes = [{"path": str(p.relative_to(root)), "sha256": sha(p.read_bytes())}
                     for p in sorted(component.rglob("*")) if p.is_file()]
    receipt = {
        "schema": "SELF_AUTHORED_FILE_READER_ACCEPTANCE_V1", "mode": args.mode,
        "passed_checks": len(results) - len(failures), "failed_checks": len(failures),
        "results": results, "commands": commands, "elapsed_seconds": time.monotonic() - start,
        "host_runtime": runtime, "fixture_manifest_sha256": sha(manifest_bytes),
        "fixture_baseline": manifest["baseline_commit"], "source_hashes": source_hashes,
        "independent_ready_single_block_cases": 151,
        "immutable_original_pending_cases": 15,
        "modern_policy_supplements": list(policies), "remaining_byte_request_api_pending": remaining_api_pending,
        "native_implementation_independently_reviewed": False,
        "actual_original_file_or_GameData_read": False, "original_or_derived_game_executed": False,
        "real_original_container_compatibility_accepted": False,
        "no_original_format_marker_constants_used": True,
    }
    save(output / "results.json", receipt)
    print(json.dumps({k: receipt[k] for k in ("mode", "passed_checks", "failed_checks", "elapsed_seconds", "host_runtime", "remaining_byte_request_api_pending")}, ensure_ascii=False))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
