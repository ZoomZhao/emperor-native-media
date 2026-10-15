"""Authored PAK framing/budget/root fixtures; no original assets or execution."""
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

import pak_payload_audit as audit
from map_sg3_audit import CITY_SIGNATURE, OBJECTS, CELLS


def framed(payload):
    out = bytearray()
    for i in range(0, len(payload), 32768):
        part = payload[i:i + 32768]
        compressed = zlib.compress(part)
        out.extend(struct.pack("<III", 0xABCDEFFF, len(compressed), len(part)))
        out.extend(compressed)
    return bytes(out)


def package(payloads, extra_slots=()):
    out = bytearray(struct.pack("<I", 0xFEDCBAAA))
    starts = []
    for part in payloads:
        starts.append(len(out))
        out.extend(framed(part))
    slots = starts[1:] + list(extra_slots)
    if len(slots) > 11:
        raise ValueError("authored fixture slots")
    out.extend(struct.pack("<17I", 0xAAABCDEF, 0x3C, *slots,
                           *([0] * (15 - len(slots)))))
    return bytes(out)


def city():
    data = bytearray(OBJECTS + 4 * CELLS)
    data[:8] = CITY_SIGNATURE
    struct.pack_into("<I", data, 84, 140)
    return bytes(data)


class PakTests(unittest.TestCase):
    def test_complete_parts_and_city_role(self):
        payloads = (b"authored settings", b"authored empire", city())
        data = package(payloads)
        self.assertEqual(audit.decode_parts(data), payloads)
        r = audit.package_metrics(data)
        self.assertEqual((r["parts"], r["city_parts"], r["non_city_parts_uninterpreted"]), (3, 1, 2))

    def test_inactive_offsets_do_not_create_or_hide_parts(self):
        data = package((b"one", b"two"))
        end = len(data) - 68
        second = struct.unpack_from("<I", data, end + 8)[0]
        data = package((b"one", b"two"), (second, end, 0xFFFFFFFE))
        self.assertEqual(audit.decode_parts(data), (b"one", b"two"))

    def test_nonmonotone_offsets_rejected(self):
        data = bytearray(package((b"one", b"two", b"three")))
        struct.pack_into("<I", data, len(data) - 68 + 12, 5)
        with self.assertRaisesRegex(audit.AuditError, "nonmonotone"):
            audit.decode_parts(data)

    def test_part_boundary_must_be_frame_boundary(self):
        data = bytearray(package((b"first", b"second")))
        struct.pack_into("<I", data, len(data) - 68 + 8, 5)
        with self.assertRaises(audit.AuditError):
            audit.decode_parts(data)

    def test_late_checksum_failure_rejects_package(self):
        data = bytearray(package((b"first", b"second")))
        data[-69] ^= 1
        with self.assertRaisesRegex(audit.AuditError, "checksum"):
            audit.decode_parts(data)

    def test_truncation_trailing_bytes_and_markers(self):
        good = package((b"authored",))
        for data in (good[:3], good[:-1], good + b"x", b"bad!" + good[4:]):
            with self.subTest(length=len(data)), self.assertRaises(audit.AuditError):
                audit.decode_parts(data)

    def test_per_part_and_total_budgets(self):
        data = package((b"first", b"second"))
        with patch.object(audit, "MAX_PACKAGE", 10), self.assertRaises(audit.AuditError):
            audit.decode_parts(data)
        with patch.object(audit, "MAX_PACKAGE_DECODED", 7), self.assertRaises(audit.AuditError):
            audit.decode_parts(data)
        with patch("map_sg3_audit.MAX_DECODED", 2), self.assertRaises(audit.AuditError):
            audit.decode_parts(data)

    def test_false_city_signature_with_short_grids_fails(self):
        short = bytearray(CITY_SIGNATURE + b"x" * 80)
        struct.pack_into("<I", short, 84, 140)
        with self.assertRaisesRegex(audit.AuditError, "grid extent"):
            audit.package_metrics(package((short,)))

    def test_scope_reports_failure_without_partial_package_credit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); directory = root / "Campaigns"; directory.mkdir()
            (directory / "good.PAK").write_bytes(package((b"one", b"two")))
            broken = bytearray(package((b"valid prefix", b"bad later"))); broken[-69] ^= 1
            (directory / "bad.pak").write_bytes(broken)
            r = audit.audit_root(root)
            self.assertEqual((r["packages"], r["complete_packages"], r["parts"]), (2, 1, 2))
            self.assertEqual(len(r["failures"]), 1)
            self.assertEqual(r["verified_SG3_tile_joins"], 0)

    def test_root_alias_and_file_count_guards(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); directory = root / "Campaigns"; directory.mkdir()
            path = directory / "one.pak"; path.write_bytes(package((b"one",)))
            with patch.object(audit, "MAX_PACKAGES", 0), self.assertRaises(audit.AuditError):
                audit.audit_root(root)
            (directory / "alias.pak").symlink_to(path)
            with self.assertRaises(audit.AuditError):
                audit.audit_root(root)

    def test_symlink_escape_rejected(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            root = Path(a); (root / "Campaigns").mkdir()
            outside = Path(b) / "outside.pak"; outside.write_bytes(package((b"outside",)))
            (root / "Campaigns" / "escape.pak").symlink_to(outside)
            with self.assertRaises(audit.AuditError):
                audit.audit_root(root)

    def test_unset_empty_and_invalid_root_cli(self):
        script = Path(audit.__file__)
        for value in (None, "", "relative"):
            env = dict(os.environ); env.pop("EMPEROR_GAMEDATA_ROOT", None)
            if value is not None:
                env["EMPEROR_GAMEDATA_ROOT"] = value
            r = subprocess.run([sys.executable, str(script)], env=env, capture_output=True, text=True)
            self.assertEqual(r.returncode, int(value == "relative"))
            self.assertEqual(json.loads(r.stdout)["skipped"], value != "relative")


if __name__ == "__main__":
    unittest.main()
