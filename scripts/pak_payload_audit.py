"""Bounded, read-only Emperor PAK framing and city-role audit.

Independently authored from public format facts in citybuilding-mappers/zlibfile.cpp.
No original game code or GPL implementation is included. Unknown footer fields
and campaign/mission semantics are not interpreted. Real input only comes from
EMPEROR_GAMEDATA_ROOT; unset/empty is successful SKIP.
"""
from pathlib import Path
import json
import os
import struct
import sys

from map_sg3_audit import AuditError, CITY_SIGNATURE, city_fields, decode_container

FOOTER_BYTES = 68
OFFSET_SLOTS = 11
MAX_PACKAGE = 128 * 1024 * 1024
MAX_PACKAGE_DECODED = 32 * 1024 * 1024
MAX_PACKAGES = 512


def decode_parts(data):
    """Decode every framed part, or reject the entire package; no partial pass."""
    if not 4 + FOOTER_BYTES <= len(data) <= MAX_PACKAGE:
        raise AuditError("PAK input budget/header")
    if data[:4] != struct.pack("<I", 0xFEDCBAAA):
        raise AuditError("PAK outer marker")
    footer = len(data) - FOOTER_BYTES
    if struct.unpack_from("<II", data, footer) != (0xAAABCDEF, 0x3C):
        raise AuditError("unsupported PAK multipart footer")
    slots = struct.unpack_from("<" + str(OFFSET_SLOTS) + "I", data, footer + 8)
    # Public multipart framing permits zero, consecutive duplicate and stale
    # >=footer slots. They are not part extents; all bytes before footer still
    # have to belong to completely verified compression frames.
    starts = [4]
    for i, value in enumerate(slots):
        previous = slots[i - 1] if i else 4
        if value == 0 or value == previous or value >= footer:
            continue
        if value <= starts[-1]:
            raise AuditError("PAK nonmonotone part offsets")
        starts.append(value)
    bounds = starts + [footer]
    parts, total = [], 0
    for start, end in zip(bounds, bounds[1:]):
        if not 4 <= start < end <= footer:
            raise AuditError("PAK part extent")
        part = decode_container(data[:4] + data[start:end])
        total += len(part)
        if total > MAX_PACKAGE_DECODED:
            raise AuditError("PAK aggregate decoded budget")
        parts.append(part)
    return tuple(parts)


def package_metrics(data):
    parts = decode_parts(data)
    cities = []
    for part in parts:
        if part[:8] == CITY_SIGNATURE:
            city_fields(part)  # Full existing terrain/object/candidate extent gate.
            cities.append(struct.unpack_from("<I", part, 84)[0])
    return {"parts": len(parts), "city_parts": len(cities),
            "non_city_parts_uninterpreted": len(parts) - len(cities),
            "decoded_bytes": sum(map(len, parts))}


def audit_root(root):
    root = Path(root)
    if not root.is_absolute() or not root.is_dir():
        raise AuditError("data-root must be an existing absolute directory")
    root = root.resolve(strict=True)
    directory = root / "Campaigns"
    if not directory.is_dir() or root not in directory.resolve(strict=True).parents:
        raise AuditError("Campaigns directory missing or outside selected root")
    selected, seen = [], set()
    for path in directory.rglob("*"):
        if path.suffix.lower() != ".pak" or not path.is_file():
            continue
        actual = path.resolve(strict=True)
        if root not in actual.parents or actual in seen:
            raise AuditError("PAK escapes selected root or duplicates a file alias")
        seen.add(actual)
        selected.append((path.relative_to(root).as_posix(), actual))
        if len(selected) > MAX_PACKAGES:
            raise AuditError("PAK file count budget")
    if not selected:
        raise AuditError("no PAK files in selected Campaigns")
    report = {"skipped": False, "packages": len(selected), "complete_packages": 0,
              "parts": 0, "city_parts": 0, "non_city_parts_uninterpreted": 0,
              "decoded_bytes": 0, "failures": [], "verified_SG3_tile_joins": 0}
    for name, path in sorted(selected):
        try:
            if path.stat().st_size > MAX_PACKAGE:
                raise AuditError("PAK input budget")
            with path.open("rb") as stream:
                data = stream.read(MAX_PACKAGE + 1)
            metrics = package_metrics(data)
        except (AuditError, OSError) as error:
            report["failures"].append({"relative": name, "reason": str(error)})
            continue
        report["complete_packages"] += 1
        for key, value in metrics.items():
            report[key] += value
    return report


def main():
    selected = os.environ.get("EMPEROR_GAMEDATA_ROOT")
    if not selected:
        print(json.dumps({"skipped": True, "reason": "EMPEROR_GAMEDATA_ROOT unset or empty"}))
        return 0
    try:
        report = audit_root(Path(selected))
    except (AuditError, OSError) as error:
        print(json.dumps({"skipped": False, "error": str(error)}))
        return 1
    print(json.dumps(report, indent=2))
    return int(bool(report["failures"]))


if __name__ == "__main__":
    sys.exit(main())
