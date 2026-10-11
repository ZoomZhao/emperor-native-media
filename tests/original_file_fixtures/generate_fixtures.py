#!/usr/bin/env python3
"""Synthetic fixture construction only; no file reader or mainline codec adapter.

All framing marker defaults are invented test tokens. Python's installed zlib
is the only decompressor used, as a bounded reference on self-created payloads.
"""
import argparse
import hashlib
import json
import platform
import struct
import sys
import zlib
from dataclasses import dataclass, field
from pathlib import Path

BASELINE = "79726e882de14e6195816a806cef93b5aedb5ac1"
CAPACITY = 36144
DEFAULT_COMPRESSED_MARKER = b"SZL1"
DEFAULT_RAW_MARKER = b"RAW1"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def u32(value):
    if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= 0xFFFFFFFF:
        raise ValueError("fixture word must be an unsigned LE32 integer")
    return struct.pack("<I", value)


def compressed(data, level=9, strategy=zlib.Z_DEFAULT_STRATEGY, wbits=15, dictionary=None):
    args = (level, zlib.DEFLATED, wbits, 8, strategy)
    codec = zlib.compressobj(*args) if dictionary is None else zlib.compressobj(*args, zdict=dictionary)
    return codec.compress(data) + codec.flush(zlib.Z_FINISH)


def observe_codec(payload, capacity=CAPACITY):
    """Observe a synthetic zlib stream, returning at most capacity+1 bytes.

    max_length=0 means UNBOUNDED in Python, so even capacity=0 passes 1.
    Byte feeds expose a returned prefix before a later Adler failure. This is
    not a model of the original codec or of the unimplemented native reader.
    """
    if not isinstance(capacity, int) or isinstance(capacity, bool) or not 0 <= capacity <= CAPACITY:
        raise ValueError("reference capacity must be 0..36144")
    decoder = zlib.decompressobj(15)
    output = bytearray()
    fed = 0
    status = "incomplete"
    error = None
    for index in range(len(payload)):
        fed = index + 1
        try:
            output.extend(decoder.decompress(payload[index:fed], capacity + 1 - len(output)))
        except zlib.error as exc:
            status, error = "codec_error", str(exc)
            break
        if len(output) > capacity:
            status = "output_limit"
            break
        if decoder.eof:
            # After eof, zlib records remaining bytes as unused_data, not a
            # second member. Remaining output budget is strictly positive.
            output.extend(decoder.decompress(payload[fed:], capacity + 1 - len(output)))
            fed = len(payload)
            status = "stream_end"
            break
    return {
        "reference": "installed Python zlib, wbits=+15; synthetic bytes only",
        "status": status,
        "stream_end": decoder.eof,
        "returned_prefix_bytes": len(output),
        "returned_prefix_sha256": digest(output),
        "payload_bytes": len(payload),
        "bytes_fed": fed,
        "codec_consumed_bytes": None if error else fed - len(decoder.unused_data) - len(decoder.unconsumed_tail),
        "unused_data_bytes": len(decoder.unused_data),
        "unconsumed_tail_bytes": len(decoder.unconsumed_tail),
        "capacity": capacity,
        "max_returned_bytes": capacity + 1,
        "error_diagnostic_only": error,
        "native_reader_tested": False,
    }


def expectation(outcome, reason=None, count=0, basis="proposed_native_boundary_in_published_contract", codec_calls=None):
    return {
        "status": "pending_evidence" if outcome == "pending" else "ready_block_assertion",
        "block_outcome": outcome,
        "basis": basis,
        "reason": reason,
        "published_available": None if outcome == "pending" else count,
        "codec_calls": codec_calls,
        "whole_file_termination": "pending_evidence",
        "original_save_compatibility": "not_claimed",
    }


@dataclass
class Case:
    name: str
    category: str
    data: bytes
    expected: dict
    notes: str
    layout: dict = field(default_factory=dict)
    expected_output: bytes = None
    codec_samples: list = field(default_factory=list)
    context: dict = field(default_factory=dict)

    def description(self):
        probes = []
        for offset, payload, capacity in self.codec_samples:
            probes.append({"payload_offset": offset, "observation": observe_codec(payload, capacity)})
        return {
            "id": self.name, "category": self.category,
            "input": {"path": self.name + "/input.bin", "bytes": len(self.data), "sha256": digest(self.data)},
            "expected": self.expected, "notes_zh": self.notes,
            "construction_layout_not_reader_result": self.layout,
            "context": self.context,
            "delivery_schedules_to_try": [{"maximum_positive_chunk": n} for n in (1, 2, 3, 4, 7, 4096)],
            "expected_output": None if self.expected_output is None else {
                "path": self.name + "/expected-output.bin", "bytes": len(self.expected_output),
                "sha256": digest(self.expected_output),
            },
            "trusted_codec_observations_not_native_results": probes,
        }


def build_cases(compressed_marker=DEFAULT_COMPRESSED_MARKER, raw_marker=DEFAULT_RAW_MARKER):
    if not isinstance(compressed_marker, bytes) or not isinstance(raw_marker, bytes):
        raise ValueError("markers must be bytes")
    if len(compressed_marker) != 4 or len(raw_marker) != 4 or compressed_marker == raw_marker:
        raise ValueError("use two distinct complete four-byte synthetic marker tokens")
    unknown_marker = next(value for value in (b"END?", b"UNK?", b"???!")
                          if value not in (compressed_marker, raw_marker))
    cases = []

    def raw(name, payload, declared=None, expected=None, notes="完整自造原样块；不等于raw-DEFLATE。", category="raw", output=None):
        count = len(payload) if declared is None else declared
        data = raw_marker + u32(count) + payload
        exp = expected or expectation("accept", count=count, basis="modern_raw_block_strict_policy_proposal", codec_calls=0)
        layout = {"kind": "raw", "marker_hex": raw_marker.hex(), "declared_n": count,
                  "payload_offset": 8, "supplied_payload_bytes": len(payload)}
        value = payload if output is None and exp["block_outcome"] == "accept" else output
        case = Case(name, category, data, exp, notes, layout, value)
        cases.append(case)
        return case

    def zipped(name, payload, output=None, word=0xA5A55A5A, declared=None, expected=None,
               notes="完整自造zlib块，未知word只消费，不作为产量。", category="zlib", capacity=CAPACITY):
        count = len(payload) if declared is None else declared
        data = compressed_marker + u32(count) + u32(word) + payload
        exp = expected or expectation("accept", count=len(output), codec_calls=1)
        layout = {"kind": "zlib", "marker_hex": compressed_marker.hex(), "declared_n": count,
                  "opaque_word": word, "payload_offset": 12, "supplied_payload_bytes": len(payload)}
        samples = [] if exp["codec_calls"] == 0 else [(12, payload[:count], capacity)]
        case = Case(name, category, data, exp, notes, layout, output, samples)
        cases.append(case)
        return case

    small = b"synthetic file bytes\x00\x01\xff"
    repetitive = b"abacabadabacaba" * 2300  # 34500, dynamic trees with the installed reference.
    stored = zipped("zlib_stored", compressed(small, level=0), small)
    fixed = zipped("zlib_fixed", compressed(small, strategy=zlib.Z_FIXED), small)
    dynamic = zipped("zlib_dynamic", compressed(repetitive), repetitive)
    # These check the actual generated first DEFLATE block, not an assumed
    # compressor strategy or a claim about all later blocks.
    for case, kind in ((stored, 0), (fixed, 1), (dynamic, 2)):
        actual = (case.data[14] >> 1) & 3
        if actual != kind:
            raise RuntimeError("reference compressor did not generate requested first block type: " + case.name)
        case.layout["first_deflate_block_type"] = actual
    raw_small = raw("raw_small", small)
    raw("raw_zero", b"", notes="块available=0；不把零块自动命名为真实EOF。")
    raw("raw_exact_capacity", bytes(range(256)) * 141 + bytes(range(48)))
    raw("raw_over_capacity", b"x" * (CAPACITY + 1), expected=expectation("reject", "input_limit", codec_calls=0), category="limits")
    zipped("zlib_empty_stream", compressed(b""), b"", notes="合法terminal零产量；whole-file EOF仍待证据。")
    zipped("zlib_exact_output_capacity", compressed(b"x" * CAPACITY), b"x" * CAPACITY, category="limits")
    zipped("zlib_one_over_output_capacity", compressed(b"x" * (CAPACITY + 1)), expected=expectation("reject", "output_limit", codec_calls=1), category="limits")
    zipped("zlib_expansion_1m", compressed(b"x" * (1024 * 1024)), expected=expectation("reject", "output_limit", codec_calls=1), category="limits")
    blob = compressed(small)
    padded = blob + b"\xa5" * (CAPACITY - len(blob))
    zipped("zlib_exact_input_capacity_with_tail", padded, small, category="limits", notes="N=36144；第一个stream后余字节保持tail，不能仅据tail失败。")
    zipped("zlib_over_input_capacity", padded + b"!", expected=expectation("reject", "input_limit", codec_calls=0), category="limits")
    zipped("zlib_declared_zero", b"", expected=expectation("reject", "missing_stream_end", codec_calls=1), category="lengths")
    for word in (0, 1, len(small), CAPACITY, 0xFFFFFFFF):
        zipped("opaque_word_%08x" % word, blob, small, word=word, category="opaque_word",
               notes="同一payload的未知word变体；不得假定此word须等于实际产量。")
    raw("raw_declared_long_short_payload", small, declared=len(small) + 1,
        expected=expectation("reject", "short_payload", codec_calls=0), category="lengths")
    zipped("zlib_declared_long_short_payload", blob, declared=len(blob) + 1,
           expected=expectation("reject", "short_payload", codec_calls=0), category="lengths")
    zipped("zlib_declared_short_stream", blob, declared=len(blob) - 1,
           expected=expectation("reject", "missing_stream_end", codec_calls=1), category="lengths",
           notes="声明N只覆盖去掉最后Adler字节的前缀；剩余文件字节不能偷喂codec。")
    raw("raw_declared_short_extra_file_bytes", small, declared=3, output=small[:3],
        expected=expectation("accept", count=3, basis="modern_raw_block_strict_policy_proposal", codec_calls=0), category="tails",
        notes="单块只消费N=3；物理文件余字节如何解析/终止待证据，不能把整文件当该payload。")
    for value in (0xFFFFFFFF, 0x80000000, CAPACITY + 1):
        raw("raw_header_huge_%08x" % value, b"", declared=value,
            expected=expectation("reject", "input_limit_before_read_or_allocation", codec_calls=0), category="limits")
        zipped("zlib_header_huge_%08x" % value, b"", declared=value,
               expected=expectation("reject", "input_limit_before_read_or_allocation", codec_calls=0), category="limits")
    # Every physical cut in both canonical frames, including all tag/word bytes.
    for full in (raw_small, stored):
        for stop in range(len(full.data)):
            boundary = full.layout["payload_offset"]
            if stop < 4:
                reason = "short_marker"
            elif stop < 8:
                reason = "short_length_word"
            elif boundary == 12 and stop < 12:
                reason = "short_opaque_word"
            else:
                reason = "short_payload"
            cases.append(Case(full.name + "_cut_%03d" % stop, "truncation", full.data[:stop],
                              expectation("reject", reason, codec_calls=0),
                              "严格边界政策：各位置截断，不零填未到达字节，codec前失败。",
                              {"prefix_of": full.name, "prefix_bytes": stop, "complete_frame_bytes": len(full.data)}))
    # Complete frames containing incomplete codec streams (different from a
    # framing short payload). All possible proper prefixes of this small stream.
    for stop in range(len(blob)):
        zipped("codec_prefix_%03d" % stop, blob[:stop],
               expected=expectation("reject", "missing_stream_end", codec_calls=1), category="codec_truncation")
    for label, tail in (("zero", b"\x00"), ("junk", b"tail!"), ("second_stream", compressed(b"second"))):
        zipped("payload_tail_" + label, blob + tail, small, category="tails",
               notes="N包含完整stream和尾随数据；第一stream产量不合并第二stream，codec实际consumed单列。")
    bad_adler = blob[:-1] + bytes([blob[-1] ^ 1])
    zipped("bad_adler_after_output_prefix", bad_adler, expected=expectation("reject", "codec_error_discard_partial", codec_calls=1),
           category="codec_damage", notes="参考库可先返回输出前缀后checksum报错；Native应available=0。")
    zipped("bad_fcheck", blob[:1] + bytes([blob[1] ^ 1]) + blob[2:],
           expected=expectation("reject", "codec_error", codec_calls=1), category="codec_damage")
    for label, cmf in (("method", 0x79), ("window", 0x88)):
        # Repair FCHECK, isolating the method/window rejection.
        flg = (-cmf * 256) % 31
        malformed = bytes([cmf, flg]) + blob[2:]
        zipped("bad_" + label + "_valid_fcheck", malformed,
               expected=expectation("reject", "codec_error", codec_calls=1), category="codec_damage")
    dictionary = b"synthetic dictionary only"
    dict_blob = compressed(dictionary * 4, dictionary=dictionary)
    zipped("fdict_no_external_dictionary", dict_blob, expected=expectation("reject", "dictionary_required", codec_calls=1),
           category="codec_damage", notes="只生成FDICT合法流；被测+15适配器不提供外部字典，不锁死host错误文本/码。")
    # RFC1951 BTYPE=3 is reserved; no original instruction bytes are involved.
    zipped("reserved_deflate_block_type", b"\x78\x01\x07\x00\x00\x00\x01",
           expected=expectation("reject", "codec_error", codec_calls=1), category="codec_damage")
    for label, bits in (("raw_deflate", -15), ("gzip", 31)):
        zipped("wrong_wrapper_" + label, compressed(small, wbits=bits),
               expected=expectation("reject", "wrong_wrapper", codec_calls=1), category="codec_damage")
    # Explicit configured budgets are a modern proposal, not an original rule.
    zipped("configured_output_limit_zero_nonempty", blob, expected=expectation("reject", "output_limit", basis="modern_configured_limit_proposal", codec_calls=1),
           category="limits", capacity=0)
    zipped("configured_output_limit_zero_empty", compressed(b""), b"",
           expected=expectation("accept", count=0, basis="modern_configured_limit_proposal", codec_calls=1), category="limits", capacity=0)
    zipped("configured_output_limit_64", compressed(repetitive),
           expected=expectation("reject", "output_limit", basis="modern_configured_limit_proposal", codec_calls=1), category="limits", capacity=64)
    for name, data, note, context in (
        ("physical_eof_before_marker", b"", "无数据不是已证明的块终止marker。", {}),
        ("unknown_complete_marker", unknown_marker, "未知完整tag不命名为真实EOF；终止marker规则待主线证据。", {}),
        ("zero_request_before_valid_block", raw_small.data, "原缓冲零请求也可能fill；新API是否no-op需明确政策，不猜。", {"requested_bytes": 0}),
        ("zero_request_before_corrupt_block", stored.data[:3], "是否触发读取/错误/前移待API政策；零请求不能套raw helper提前返回。", {"requested_bytes": 0}),
        ("whole_frame_then_file_junk", stored.data + b"junk!", "单块成功；frame外尾部按下一marker还是错误/终止待证据。", {}),
        ("multiblock_raw_raw", raw_small.data * 2, "两个完整原样块；按请求跨块及预读终止规则待证据。", {}),
        ("multiblock_zlib_zlib", fixed.data + dynamic.data, "每块payload参考解码独立有效；整流拼接/请求跨块待证据。", {}),
        ("multiblock_raw_zlib", raw_small.data + fixed.data, "混合原样/zlib块；不假造全文件结束规则。", {}),
        ("multiblock_zlib_raw", fixed.data + raw_small.data, "同上，换块顺序。", {}),
        ("multiblock_good_then_corrupt", raw_small.data + stored.data[:3], "后块失败时已返回前缀/整请求事务政策待明确；坏块本身不可发布。", {}),
        ("multiblock_empty_then_nonempty", raw_marker + u32(0) + fixed.data, "空块不自动等于EOF；跳空块/进展规则待证据。", {}),
        ("multiblock_empty_128", raw_marker + u32(0), "下方构造128空块；块数预算应明确，防零产量循环。", {"proposed_max_blocks": 64}),
        ("aggregate_budget_two_blocks", raw_small.data * 2, "若配置总量=单块产量，第二块会超预算；是否发布首块待流API事务政策。", {"proposed_total_output_limit": len(small)}),
        ("checked_counter_near_u64_max", raw_small.data, "内存小夹具配合逻辑累计计数，须先查加法溢出；API尚未给定。", {"logical_total_before": (1 << 64) - 2}),
        ("exact_request_block_boundary", raw_small.data + unknown_marker, "原等量分支仍可能fill；不得未经证据锁死新reader是否预读。", {"requested_bytes": len(small)}),
    ):
        if name == "multiblock_empty_128":
            data *= 128
        cases.append(Case(name, "pending_stream_policy", data, expectation("pending", "termination_or_api_policy_unclosed", basis="awaiting_source_or_native_api_policy"), note, context=context))
    return cases


def write_corpus(output, compressed_marker=DEFAULT_COMPRESSED_MARKER, raw_marker=DEFAULT_RAW_MARKER):
    if not output.is_absolute() or output.exists():
        raise ValueError("output must be a fresh absolute directory")
    cases = build_cases(compressed_marker, raw_marker)
    descriptions = [case.description() for case in cases]
    output.mkdir(parents=True, exist_ok=False)
    for case, description in zip(cases, descriptions):
        directory = output / case.name
        directory.mkdir()
        (directory / "input.bin").write_bytes(case.data)
        if case.expected_output is not None:
            (directory / "expected-output.bin").write_bytes(case.expected_output)
        (directory / "case.json").write_text(json.dumps(description, ensure_ascii=False, indent=2) + "\n")
    manifest = {
        "schema": "EMPEROR_SYNTHETIC_BLOCK_FIXTURES_V1", "baseline_commit": BASELINE,
        "synthetic_only": True, "original_save_compatibility": "not_claimed",
        "native_reader_executed": False, "mainline_implementation_read_or_copied": False,
        "marker_role": "synthetic configurable tokens; actual F14/F18 values unbound",
        "compressed_marker_hex": compressed_marker.hex(), "raw_marker_hex": raw_marker.hex(),
        "runtime": {"python": platform.python_version(), "executable": sys.executable,
                    "zlib_build": zlib.ZLIB_VERSION, "zlib_runtime": zlib.ZLIB_RUNTIME_VERSION},
        "generation_source_sha256": digest(Path(__file__).read_bytes()),
        "contract_metadata_sha256": digest(Path(__file__).with_name("source_contract.json").read_bytes()),
        "case_count": len(cases),
        "ready_block_assertions": sum(x.expected["status"] == "ready_block_assertion" for x in cases),
        "pending_evidence_cases": sum(x.expected["status"] == "pending_evidence" for x in cases),
        "cases": descriptions,
    }
    (output / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compressed-marker-hex", default=DEFAULT_COMPRESSED_MARKER.hex())
    parser.add_argument("--raw-marker-hex", default=DEFAULT_RAW_MARKER.hex())
    args = parser.parse_args()
    try:
        manifest = write_corpus(args.output, bytes.fromhex(args.compressed_marker_hex), bytes.fromhex(args.raw_marker_hex))
    except (ValueError, RuntimeError) as exc:
        parser.error(str(exc))
    print(json.dumps({key: manifest[key] for key in ("schema", "case_count", "ready_block_assertions", "pending_evidence_cases", "native_reader_executed")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
