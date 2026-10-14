"""Authored format/candidate boundary fixtures; no user data or game execution."""
from collections import Counter
from pathlib import Path
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

import map_sg3_audit as audit


def container(payload):
    result = bytearray(struct.pack("<I", 0xFEDCBAAA))
    for start in range(0, len(payload), audit.MAX_CHUNK):
        part = payload[start:start + audit.MAX_CHUNK]
        encoded = zlib.compress(part)
        result.extend(struct.pack("<III", 0xABCDEFFF, len(encoded), len(part)))
        result.extend(encoded)
    return bytes(result)


def city(object_value=2, candidate=5):
    payload = bytearray(audit.OBJECTS + audit.CELLS * 4)
    payload[:8] = audit.CITY_SIGNATURE
    struct.pack_into("<I", payload, 84, 84)
    payload[audit.TERRAIN:audit.OBJECTS] = struct.pack("<I", audit.OFF_MAP) * audit.CELLS
    active = 114 * audit.SIDE + 114
    struct.pack_into("<I", payload, audit.TERRAIN + 4 * active, 0)
    struct.pack_into("<I", payload, audit.OBJECTS + 4 * active, object_value)
    struct.pack_into("<I", payload, audit.CANDIDATE + 4 * active, candidate)
    # Deliberately invalid ID at an off-map position must not count as active.
    struct.pack_into("<I", payload, audit.CANDIDATE, 0xFFFFFFFF)
    return bytes(payload)


def sg(count=4, version=214, group=1):
    step = 64 if version == 213 else 72
    data = bytearray(audit.INDEX_TABLE + (count + 1) * step)
    struct.pack_into("<II", data, 0, len(data), version)
    struct.pack_into("<III", data, 12, 20, count, 1)
    struct.pack_into("<H", data, 80 + 2 * 299, group)
    data[680:688] = b"own.bmp\0"
    for image_id in range(1, count + 1):
        position = audit.INDEX_TABLE + image_id * step
        struct.pack_into("<I", data, position + 4, 4)
        struct.pack_into("<HH", data, position + 20, 2, 1)
        struct.pack_into("<H", data, position + 50, 1)
    return bytes(data)


def corpus(root, candidate=5, archives=1, group=1):
    (root / "Cities").mkdir()
    (root / "DATA").mkdir()
    (root / "Campaigns" / "own").mkdir(parents=True)
    (root / "Cities" / "own.map").write_bytes(container(city(candidate=candidate)))
    (root / "Campaigns" / "own" / "ownP.map").write_bytes(container(b"own empire"))
    for n in range(archives):
        (root / "DATA" / ("own%d.sg3" % n)).write_bytes(sg(group=group))


class AuditTests(unittest.TestCase):
    def test_grid_unsigned_endian_row_order(self):
        data = struct.pack("<4I", 1, 258, 0x80000001, 4)
        self.assertEqual(audit.u32_grid(data, 0, 2), (1, 258, 0x80000001, 4))
        with self.assertRaises(audit.AuditError):
            audit.u32_grid(data[:-1], 0, 2)
        with self.assertRaises(audit.AuditError):
            audit.u32_grid(data, -1, 2)
        with self.assertRaises(audit.AuditError):
            audit.u32_grid(data, 0, 229)

    def test_complete_multichunk_container(self):
        payload = city()
        self.assertEqual(audit.decode_container(container(payload)), payload)
        self.assertEqual(audit.OBJECTS, 469391)
        flags, objects, candidate = audit.city_fields(payload)
        position = 114 * 228 + 114
        self.assertEqual((flags[position], objects[position], candidate[position]), (0, 2, 5))

    def test_late_checksum_failure_not_partial_pass(self):
        data = bytearray(container(city()))
        data[-1] ^= 1
        with self.assertRaises(audit.AuditError):
            audit.decode_container(data)

    def test_trailing_and_truncated_containers(self):
        good = container(b"entirely authored")
        for broken in [good + b"x", good[:-1], struct.pack("<I", 0xFEDCBAAA)]:
            with self.assertRaises(audit.AuditError):
                audit.decode_container(broken)

    def test_zlib_extra_stream_bytes_rejected(self):
        encoded = zlib.compress(b"own") + b"extra"
        data = struct.pack("<4I", 0xFEDCBAAA, 0xABCDEFFF, len(encoded), 3) + encoded
        with self.assertRaises(audit.AuditError):
            audit.decode_container(data)

    def test_record_budget_and_declared_length(self):
        data = struct.pack("<4I", 0xFEDCBAAA, 0xABCDEFFF, 1, audit.MAX_CHUNK + 1) + b"x"
        with self.assertRaises(audit.AuditError):
            audit.decode_container(data)
        with patch.object(audit, "MAX_DECODED", 2):
            with self.assertRaises(audit.AuditError):
                audit.decode_container(container(b"more than two"))
        with patch.object(audit, "MAX_FRAMES", 1):
            with self.assertRaises(audit.AuditError):
                audit.decode_container(container(b"x" * (audit.MAX_CHUNK + 1)))

    def test_short_fields_and_wrong_role(self):
        with self.assertRaises(audit.AuditError):
            audit.city_fields(city()[:audit.OBJECTS + 3])
        with self.assertRaises(audit.AuditError):
            audit.city_fields(b"own empire")

    def test_capacity_is_not_used_image_domain(self):
        archives = [audit.SGMetadata(4, 20, (), frozenset())]
        result = audit.candidate_report(Counter({5: 3}), archives)
        self.assertEqual(result["beyond_any_archive_logical_id"], 3)
        self.assertEqual(result["verdict"], "rejected_unmodified_direct_ids")
        self.assertFalse(result["verified_join"])

    def test_range_fit_never_proves_mapping(self):
        result = audit.candidate_report(
            Counter({1: 8, 4: 2}), [audit.SGMetadata(4, 20, (), frozenset())])
        self.assertEqual(result["verdict"], "inconclusive")
        self.assertFalse(result["verified_join"])

    def test_metadata_versions_and_dummy_exclusion(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "own.sg3"
            for version in [213, 214]:
                path.write_bytes(sg(version=version))
                archive = audit.read_sg3(path, {0, 1, 20})
                self.assertEqual((archive.count, archive.capacity), (4, 20))
                self.assertEqual(archive.groups[299], 1)
                self.assertEqual(len(archive.groups), 300)
            for data in [sg()[:-1], sg(version=215)]:
                path.write_bytes(data)
                with self.assertRaises(audit.AuditError):
                    audit.read_sg3(path, {1})

    def test_compatibility_header_and_type30_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "own.sg3"
            data = bytearray(sg())
            struct.pack_into("<I", data, 0, 74480)
            position = audit.INDEX_TABLE + 72  # authored logical ID1, not dummy
            struct.pack_into("<HH", data, position + 20, 78, 40)
            struct.pack_into("<H", data, position + 50, 30 | 256)
            path.write_bytes(data)
            self.assertEqual(audit.read_sg3(path, {0, 1}).type30_ids, frozenset({1}))

    def test_corpus_roles_and_candidate_rejection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            corpus(root)
            report = audit.audit_root(root)
            self.assertTrue(report["acceptance_passed"])
            self.assertEqual((report["city_maps"], report["excluded_empire_maps"]), (1, 1))
            self.assertEqual(report["non_offmap_cells"], 1)
            self.assertEqual(report["raw32_candidate_domain"]["beyond_any_archive_logical_id"], 1)
            self.assertEqual(report["candidate_maps_beyond_any_archive_id"], 1)
            self.assertEqual(report["verified_SG3_tile_joins"], 0)

    def test_ambiguous_archives_not_silently_first_match(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            corpus(root, candidate=2, archives=2)
            report = audit.audit_root(root)
            self.assertEqual(report["objects_index_domain"]["archive_match_min"], 2)
            self.assertEqual(report["objects_index_domain"]["uniquely_resolved_values"], 0)
            self.assertEqual(report["raw32_candidate_domain"]["verdict"], "inconclusive")
            self.assertEqual(report["verified_SG3_tile_joins"], 0)

    def test_invalid_group_reference_prevents_acceptance(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            corpus(root, group=5)
            report = audit.audit_root(root)
            self.assertFalse(report["acceptance_passed"])
            self.assertEqual(report["invalid_group_references"], 1)

    def test_root_empty_unknown_role_and_escape(self):
        with tempfile.TemporaryDirectory() as directory, tempfile.TemporaryDirectory() as outside:
            root = Path(directory)
            with self.assertRaises(audit.AuditError):
                audit.audit_root(root)
            corpus(root)
            external = Path(outside) / "own.map"
            external.write_bytes(container(city()))
            (root / "Cities" / "escape.map").symlink_to(external)
            with self.assertRaises(audit.AuditError):
                audit.audit_root(root)
            (root / "Cities" / "escape.map").unlink()  # our temporary fixture only
            (root / "unclassified.map").write_bytes(container(city()))
            with self.assertRaises(audit.AuditError):
                audit.audit_root(root)

    def test_unset_and_empty_root_clean_skip_without_fallback(self):
        environment = dict(os.environ)
        environment.pop("EMPEROR_GAMEDATA_ROOT", None)
        script = Path(audit.__file__).resolve()
        with tempfile.TemporaryDirectory() as directory:
            # A tempting local GameData must never be used implicitly.
            corpus(Path(directory))
            for value in [None, ""]:
                if value is not None:
                    environment["EMPEROR_GAMEDATA_ROOT"] = value
                result = subprocess.run([sys.executable, str(script)], cwd=directory,
                                        env=environment, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0)
                self.assertTrue(json.loads(result.stdout)["skipped"])


if __name__ == "__main__":
    unittest.main()
