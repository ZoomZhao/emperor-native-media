"""Read-only MAP/SG3 metadata audit; reports aggregates, never artwork or mappings.

Independently authored bounded readers from published field facts. No original
game code or GPL parser/renderer is included. Real input is exclusively selected
by EMPEROR_GAMEDATA_ROOT; unset/empty is successful SKIP.
"""
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import json
import os
import struct
import sys
import zlib

SIDE = 228
CELLS = SIDE * SIDE
TERRAIN = 261455
OBJECTS = TERRAIN + 4 * CELLS
# Experimental interpretation only; the public reader skips this region.
CANDIDATE = TERRAIN - 5 * CELLS
OFF_MAP = 0x80000
CITY_SIGNATURE = bytes([5, 0, 254, 202, 0, 0, 2, 0])
MAX_INPUT = 4 * 1024 * 1024
MAX_DECODED = 8 * 1024 * 1024
MAX_CHUNK = 32768
MAX_FRAMES = 256
MAX_INDEX = 128 * 1024 * 1024
MAX_FILES = 512
MAX_DISTINCT_OBJECTS = 65536
INDEX_TABLE = 40680


class AuditError(ValueError):
    """Malformed, unsupported or unbounded input; never a partial-data pass."""


def decode_container(data):
    if len(data) > MAX_INPUT or len(data) < 4:
        raise AuditError("MAP input budget/header")
    if struct.unpack_from("<I", data)[0] != 0xFEDCBAAA:
        raise AuditError("MAP outer marker")
    offset, frames, decoded = 4, 0, bytearray()
    while offset < len(data):
        if frames >= MAX_FRAMES or len(data) - offset < 12:
            raise AuditError("MAP frame count/truncated header")
        marker, encoded, declared = struct.unpack_from("<III", data, offset)
        if marker != 0xABCDEFFF:
            raise AuditError("unsupported MAP record marker")
        if not 0 < encoded <= 2 * MAX_CHUNK or not 0 < declared <= MAX_CHUNK:
            raise AuditError("MAP chunk budget")
        if declared > MAX_DECODED - len(decoded):
            raise AuditError("MAP decoded budget")
        end = offset + 12 + encoded
        if end > len(data):
            raise AuditError("MAP truncated record")
        try:
            decoder = zlib.decompressobj()
            chunk = decoder.decompress(data[offset + 12:end], declared + 1)
        except zlib.error as error:
            raise AuditError("MAP zlib/checksum failure") from error
        if (len(chunk) != declared or not decoder.eof
                or decoder.unused_data or decoder.unconsumed_tail):
            raise AuditError("MAP zlib stream/declared length mismatch")
        decoded.extend(chunk)
        offset, frames = end, frames + 1
    if not frames:
        raise AuditError("MAP empty container")
    return bytes(decoded)


def u32_grid(payload, offset, side=SIDE):
    """Unsigned LE flat grid in storage order y*side+x; no semantic conversion."""
    if side < 1 or side > SIDE or offset < 0:
        raise AuditError("grid descriptor bounds")
    count = side * side
    if offset + count * 4 > len(payload):
        raise AuditError("grid extent")
    return struct.unpack_from("<" + str(count) + "I", payload, offset)


def city_fields(payload):
    if payload[:8] != CITY_SIGNATURE or len(payload) < 88:
        raise AuditError("unsupported city signature")
    side = struct.unpack_from("<I", payload, 84)[0]
    if not 0 < side <= SIDE:
        raise AuditError("invalid active-side field")
    return (u32_grid(payload, TERRAIN), u32_grid(payload, OBJECTS),
            u32_grid(payload, CANDIDATE))


@dataclass
class SGMetadata:
    count: int
    capacity: int
    groups: tuple
    # Only selected record metadata is retained; no image payload is read.
    type30_ids: frozenset


def read_sg3(path, desired_ids):
    size = path.stat().st_size
    if not INDEX_TABLE <= size <= MAX_INDEX:
        raise AuditError("SG3 metadata input budget")
    with path.open("rb") as stream:
        head = stream.read(INDEX_TABLE)
        if len(head) != INDEX_TABLE:
            raise AuditError("SG3 truncated header metadata")
        declared, version = struct.unpack_from("<II", head)
        capacity, count, bitmaps = struct.unpack_from("<III", head, 12)
        step = {213: 64, 214: 72}.get(version)
        if (step is None or declared not in (size, 74480)
                or not 0 < count < capacity or bitmaps > 200
                or INDEX_TABLE + (count + 1) * step > size):
            raise AuditError("SG3 header/record extent")
        for bitmap in range(bitmaps):
            if b"\0" not in head[680 + bitmap * 200:745 + bitmap * 200]:
                raise AuditError("SG3 unterminated bitmap name")
        groups = struct.unpack_from("<300H", head, 80)
        type30_ids = set()
        for image_id in desired_ids:
            if not 0 < image_id <= count:
                continue
            stream.seek(INDEX_TABLE + image_id * step)
            record = stream.read(step)
            if len(record) != step:
                raise AuditError("SG3 truncated selected metadata")
            # Type is byte50; byte51 is the separate compression flag.
            if record[50] == 30:
                type30_ids.add(image_id)
    return SGMetadata(count, capacity, groups, frozenset(type30_ids))


def candidate_report(values, archives):
    """Only necessary index-domain checks. Range fit cannot prove a mapping."""
    maximum = max(a.count for a in archives)
    dense = sum(a.count for a in archives)
    beyond = sum(n for value, n in values.items() if value > maximum)
    beyond_dense = sum(n for value, n in values.items() if value > dense)
    return {
        "nonzero_active_values": sum(values.values()),
        "beyond_any_archive_logical_id": beyond,
        "beyond_dense_concatenated_logical_id": beyond_dense,
        "verdict": "rejected_unmodified_direct_ids" if beyond else "inconclusive",
        "verified_join": False,
    }


def audit_root(root):
    if not root.is_absolute() or not root.is_dir():
        raise AuditError("data-root must be an existing absolute directory")
    root = root.resolve(strict=True)
    selected, seen = [], set()
    for path in root.rglob("*"):
        if path.suffix.lower() not in (".map", ".sg3") or not path.is_file():
            continue
        resolved = path.resolve(strict=True)
        try:
            resolved.relative_to(root)
        except ValueError as error:
            raise AuditError("data file escapes selected root") from error
        if resolved in seen:
            raise AuditError("duplicate data-file alias")
        seen.add(resolved)
        selected.append((path.relative_to(root).as_posix().lower(), resolved))
        if len(selected) > MAX_FILES:
            raise AuditError("data corpus file budget")
    maps = [(name, path) for name, path in selected if name.endswith(".map")]
    indexes = [path for name, path in selected if name.endswith(".sg3")]
    if not maps or not indexes:
        raise AuditError("MAP and SG3 corpora both required")
    report = {
        "skipped": False, "complete_map_containers": 0, "city_maps": 0,
        "excluded_empire_maps": 0, "documented_object_grids": 0,
        "non_offmap_cells": 0, "nonzero_object_cells": 0,
        "candidate_maps_beyond_any_archive_id": 0,
        "candidate_maps_beyond_dense_id": 0,
        "verified_SG3_tile_joins": 0,
        "objects_layout": {"offset": OBJECTS, "type": "uint32_le",
                           "side": SIDE, "storage_order": "y*228+x"},
        "raw32_candidate": {"offset": CANDIDATE, "width": 32,
                            "semantics": "unclassified; experimental interpretation"},
    }
    object_values, candidate_values, per_map_bounds = set(), Counter(), []
    for name, path in sorted(maps):
        if path.stat().st_size > MAX_INPUT:
            raise AuditError("MAP input budget")
        payload = decode_container(path.read_bytes())
        report["complete_map_containers"] += 1
        if name.startswith("campaigns/") and path.stem.lower().endswith("p"):
            if payload[:8] == CITY_SIGNATURE:
                raise AuditError("empire path has city role")
            report["excluded_empire_maps"] += 1
            continue
        if not name.startswith("cities/"):
            raise AuditError("unclassified MAP role")
        flags, objects, candidate = city_fields(payload)
        report["city_maps"] += 1
        report["documented_object_grids"] += 1
        local_maximum = 0
        for flag, value, raw in zip(flags, objects, candidate):
            if flag & OFF_MAP:
                continue
            report["non_offmap_cells"] += 1
            if value:
                object_values.add(value)
                report["nonzero_object_cells"] += 1
            if raw:
                candidate_values[raw] += 1
                local_maximum = max(local_maximum, raw)
                if len(candidate_values) > MAX_DISTINCT_OBJECTS:
                    raise AuditError("candidate diversity budget")
            if len(object_values) > MAX_DISTINCT_OBJECTS:
                raise AuditError("object-code diversity budget")
        per_map_bounds.append(local_maximum)
    if not report["city_maps"] or not report["non_offmap_cells"]:
        raise AuditError("nonempty city corpus required")
    archives = [read_sg3(path, object_values) for path in indexes]
    maximum = max(a.count for a in archives)
    dense = sum(a.count for a in archives)
    report.update(
        sg3_archives=len(archives), sg3_logical_ids=dense,
        max_archive_logical_id=maximum,
        group_lookup_slots=300 * len(archives),
        populated_group_references=sum(bool(v) for a in archives for v in a.groups),
        invalid_group_references=sum(v > a.count for a in archives for v in a.groups),
        archives_with_valid_group_references=sum(
            all(v <= a.count for v in a.groups) for a in archives),
        nonzero_object_values_distinct=len(object_values),
        max_object_value=max(object_values, default=0),
        max_raw32_candidate=max(candidate_values, default=0),
        candidate_maps_beyond_any_archive_id=sum(v > maximum for v in per_map_bounds),
        candidate_maps_beyond_dense_id=sum(v > dense for v in per_map_bounds),
        raw32_candidate_domain=candidate_report(candidate_values, archives),
    )
    matches = [sum(v <= a.count for a in archives) for v in object_values]
    report["objects_index_domain"] = {
        "archive_match_min": min(matches, default=0),
        "archive_match_max": max(matches, default=0),
        "uniquely_resolved_values": sum(n == 1 for n in matches),
        "values_with_declared_type30_candidate": sum(
            any(v in a.type30_ids for a in archives) for v in object_values),
        "mapping_verdict": "unverified",
    }
    report["acceptance_passed"] = (
        report["documented_object_grids"] == report["city_maps"]
        and report["invalid_group_references"] == 0)
    return report


def main():
    value = os.environ.get("EMPEROR_GAMEDATA_ROOT")
    if not value:
        print(json.dumps({"skipped": True, "reason": "EMPEROR_GAMEDATA_ROOT unset"}))
        return 0
    try:
        report = audit_root(Path(value))
    except (AuditError, OSError) as error:
        print(json.dumps({"skipped": False, "acceptance_passed": False,
                          "error": str(error)}))
        return 1
    print(json.dumps(report, sort_keys=True))
    return 0 if report["acceptance_passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
