"""Self-check construction and trusted-zlib observations, never an adapter PASS."""
import json
import tempfile
import unittest
from pathlib import Path

import generate_fixtures as fixtures


class FixtureGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = fixtures.build_cases()
        cls.by_name = {case.name: case for case in cls.cases}
        cls.descriptions = {case.name: case.description() for case in cls.cases}

    def probe(self, name):
        return self.descriptions[name]["trusted_codec_observations_not_native_results"][0]["observation"]

    def test_unique_ids_and_required_categories(self):
        source = json.loads(Path(fixtures.__file__).with_name("source_contract.json").read_text())
        self.assertEqual(source["baseline_commit"], fixtures.BASELINE)
        self.assertEqual(source["known_rule_fragments"]["nominal_single_block_capacity"], fixtures.CAPACITY)
        self.assertEqual(len(self.cases), len(self.by_name))
        self.assertEqual({case.category for case in self.cases}, {
            "raw", "zlib", "limits", "lengths", "tails", "opaque_word", "truncation",
            "codec_truncation", "codec_damage", "pending_stream_policy"})
        self.assertTrue(all(not probe["observation"]["native_reader_tested"] for d in self.descriptions.values()
                            for probe in d["trusted_codec_observations_not_native_results"]))

    def test_distinct_configurable_markers_and_invalid_generator_words(self):
        for first, second in ((b"a", b"RAW1"), (b"SAME", b"SAME"), ("SZL1", b"RAW1")):
            with self.assertRaises(ValueError):
                fixtures.build_cases(first, second)
        for value in (-1, 1 << 32, True):
            with self.assertRaises(ValueError):
                fixtures.u32(value)
        alternate = fixtures.build_cases(b"ZIP!", b"RAW!")
        self.assertEqual(alternate[0].data[:4], b"ZIP!")
        self.assertEqual(next(x for x in alternate if x.name == "raw_small").data[:4], b"RAW!")
        alternative_reserved = fixtures.build_cases(b"END?", b"UNK?")
        self.assertNotIn(next(x for x in alternative_reserved if x.name == "unknown_complete_marker").data,
                         (b"END?", b"UNK?"))

    def test_generated_stored_fixed_dynamic_streams(self):
        for name, kind in (("zlib_stored", 0), ("zlib_fixed", 1), ("zlib_dynamic", 2)):
            case, probe = self.by_name[name], self.probe(name)
            self.assertEqual(case.layout["first_deflate_block_type"], kind)
            self.assertEqual(probe["status"], "stream_end")
            self.assertTrue(probe["stream_end"])
            self.assertEqual(probe["returned_prefix_bytes"], len(case.expected_output))
            self.assertEqual(probe["returned_prefix_sha256"], fixtures.digest(case.expected_output))

    def test_framing_each_truncation_offset_and_no_codec_call(self):
        for name in ("raw_small", "zlib_stored"):
            base = self.by_name[name]
            cuts = [x for x in self.cases if x.layout.get("prefix_of") == name]
            self.assertEqual(len(cuts), len(base.data))
            self.assertEqual({x.layout["prefix_bytes"] for x in cuts}, set(range(len(base.data))))
            for cut in cuts:
                self.assertEqual(cut.data, base.data[:cut.layout["prefix_bytes"]])
                self.assertEqual(cut.expected["block_outcome"], "reject")
                self.assertEqual(cut.expected["published_available"], 0)
                self.assertEqual(cut.expected["codec_calls"], 0)
                self.assertEqual(cut.codec_samples, [])

    def test_complete_frames_with_incomplete_codec_streams(self):
        truncated = [x for x in self.cases if x.category == "codec_truncation"]
        self.assertGreater(len(truncated), 8)
        for case in truncated:
            probe = self.probe(case.name)
            self.assertEqual(probe["status"], "incomplete")
            self.assertFalse(probe["stream_end"])
            self.assertEqual(case.expected["published_available"], 0)
            self.assertEqual(case.expected["codec_calls"], 1)

    def test_unknown_word_does_not_equal_output_length_contract(self):
        variants = [x for x in self.cases if x.category == "opaque_word"]
        self.assertEqual(len(variants), 5)
        expected_output = self.by_name["raw_small"].expected_output
        self.assertTrue(any(x.layout["opaque_word"] != len(expected_output) for x in variants))
        for case in variants:
            self.assertEqual(self.probe(case.name)["returned_prefix_sha256"], fixtures.digest(expected_output))
            self.assertEqual(case.expected["published_available"], len(expected_output))
            self.assertEqual(case.expected["block_outcome"], "accept")

    def test_payload_tails_end_at_first_stream(self):
        for name in ("payload_tail_zero", "payload_tail_junk", "payload_tail_second_stream", "zlib_exact_input_capacity_with_tail"):
            case, probe = self.by_name[name], self.probe(name)
            self.assertEqual(probe["status"], "stream_end")
            self.assertGreater(probe["unused_data_bytes"], 0)
            self.assertEqual(probe["codec_consumed_bytes"] + probe["unused_data_bytes"], case.layout["declared_n"])
            self.assertEqual(probe["returned_prefix_sha256"], fixtures.digest(case.expected_output))
        self.assertEqual(self.probe("payload_tail_second_stream")["returned_prefix_bytes"], len(self.by_name["raw_small"].expected_output))

    def test_corrupt_streams_and_partial_prefix_is_not_publishable(self):
        damaged = [x for x in self.cases if x.category == "codec_damage"]
        for case in damaged:
            probe = self.probe(case.name)
            self.assertEqual(probe["status"], "codec_error")
            self.assertEqual(case.expected["published_available"], 0)
            self.assertIsNone(case.expected_output)
        prefix = self.probe("bad_adler_after_output_prefix")
        self.assertGreater(prefix["returned_prefix_bytes"], 0)
        self.assertFalse(prefix["stream_end"])
        self.assertIsNone(prefix["codec_consumed_bytes"])

    def test_output_capacity_exact_overflow_and_reference_bound(self):
        exact = self.probe("zlib_exact_output_capacity")
        self.assertEqual(exact["status"], "stream_end")
        self.assertEqual(exact["returned_prefix_bytes"], fixtures.CAPACITY)
        for name in ("zlib_one_over_output_capacity", "zlib_expansion_1m", "configured_output_limit_64", "configured_output_limit_zero_nonempty"):
            probe = self.probe(name)
            self.assertEqual(probe["status"], "output_limit")
            self.assertEqual(probe["returned_prefix_bytes"], probe["capacity"] + 1)
        for desc in self.descriptions.values():
            for probe in desc["trusted_codec_observations_not_native_results"]:
                self.assertLessEqual(probe["observation"]["returned_prefix_bytes"], probe["observation"]["max_returned_bytes"])

    def test_empty_stream_is_terminal_not_file_eof(self):
        for name in ("zlib_empty_stream", "configured_output_limit_zero_empty"):
            probe = self.probe(name)
            self.assertTrue(probe["stream_end"])
            self.assertEqual(probe["returned_prefix_bytes"], 0)
            self.assertEqual(self.by_name[name].expected["whole_file_termination"], "pending_evidence")
        self.assertFalse(self.probe("zlib_declared_zero")["stream_end"])

    def test_input_limits_and_short_framing_fail_before_codec(self):
        for case in self.cases:
            if case.expected["reason"] in ("input_limit", "input_limit_before_read_or_allocation", "short_payload"):
                self.assertEqual(case.expected["codec_calls"], 0)
                self.assertEqual(case.codec_samples, [])
                self.assertEqual(case.expected["published_available"], 0)
        self.assertEqual(len(self.by_name["raw_exact_capacity"].expected_output), fixtures.CAPACITY)
        self.assertEqual(self.by_name["zlib_exact_input_capacity_with_tail"].layout["declared_n"], fixtures.CAPACITY)

    def test_pending_stream_zero_request_and_budget_cases_stay_pending(self):
        pending = [case for case in self.cases if case.category == "pending_stream_policy"]
        self.assertEqual(len(pending), 15)
        for case in pending:
            self.assertEqual(case.expected["status"], "pending_evidence")
            self.assertIsNone(case.expected["published_available"])
            self.assertIsNone(case.expected["codec_calls"])
        for name in ("zero_request_before_valid_block", "zero_request_before_corrupt_block"):
            self.assertEqual(self.by_name[name].context["requested_bytes"], 0)
        self.assertEqual(self.by_name["multiblock_empty_128"].data, (fixtures.DEFAULT_RAW_MARKER + fixtures.u32(0)) * 128)

    def test_reproducible_construction_in_same_reference_runtime(self):
        second = fixtures.build_cases()
        self.assertEqual([(x.name, x.data, x.expected_output, x.expected) for x in self.cases],
                         [(x.name, x.data, x.expected_output, x.expected) for x in second])

    def test_written_corpus_hashes_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "fresh"
            manifest = fixtures.write_corpus(output)
            self.assertFalse(manifest["native_reader_executed"])
            self.assertEqual(manifest["case_count"], len(self.cases))
            self.assertEqual(json.loads((output / "manifest.json").read_text()), manifest)
            for desc in manifest["cases"]:
                data = (output / desc["input"]["path"]).read_bytes()
                self.assertEqual(len(data), desc["input"]["bytes"])
                self.assertEqual(fixtures.digest(data), desc["input"]["sha256"])
                if desc["expected_output"] is not None:
                    value = desc["expected_output"]
                    data = (output / value["path"]).read_bytes()
                    self.assertEqual(len(data), value["bytes"])
                    self.assertEqual(fixtures.digest(data), value["sha256"])
            with self.assertRaises(ValueError):
                fixtures.write_corpus(output)
            with self.assertRaises(ValueError):
                fixtures.write_corpus(Path("relative"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
