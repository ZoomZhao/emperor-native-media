"""Bounded published building-table prefix audit; no object artwork/logic guesses.

Original inputs only through EMPEROR_GAMEDATA_ROOT. Exact record details stay
in memory; CLI emits aggregates, not a record/coordinate or asset dump.
"""
import argparse
import json
import os
from pathlib import Path
import re
import struct

TABLE_OFFSET = 1093607
COUNT = 4000
PREAMBLE = struct.pack('<HIHHH', 1, COUNT, 0xffff, 0, 8) + b'Building'
COMMON_BYTES = {3: 179, 4: 181, 5: 182}
# Fixed lengths from the pinned public modern mapper, not an executable.
# Only the closed profile needed for these bounded samples is supported.
TAIL_BYTES = {kind: length for length, kinds in (
    (0, (0x0,)),
    (65, (0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11)),
    (2, (0x5e,)),
    (130, (0x48,0x6f,0x71,0x7c,0x7e,0x7f,0x8a,0x97,0xb1,0xb2,0xb3,0xb4,0xb5,0xb7,0xb9,0xba,0xbb,0xbc,0xbd,0xa1,0xa2,0xa3,0xa4,0xa5,0xa7,0xad,0xae,0xaf,0xcf,0xd0)),
    (132, (0x59,0x5a,0x5b,0xe7,0x68,0x69,0x6a,0xe8,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7d,0x83,0xcb,0xd1,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc)),
    (134, (0x1f,0x21,0x26,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x31,0x34,0xd3,0xd4,0xd5,0xe2,0xe9,0xeb,0xec)),
    (136, (0x23,0x24,0x25,0x27,0x28,0x29,0x30,0xdc,0xdd,0xdf,0xe0,0xe1)),
    (140, (0x1a,0x1b,0x1c,0x3e,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x82,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7)),
    (142, (0xc0,0xed,0xee,0xef)),
    (165, (0xd6,0xd7,0xd9)),
    (167, (0xd8,0xda)),
    (169, (0xdb,)),
    (179, (0x6e,)),
    (222, (0xc1,)),
    (236, (0x4b,)),
    (248, (0x36,)),
    (266, (0x3b,0x3c)),
    (288, (0x35,)),
    (348, (0x3a,)),
    (352, (0x47,)),
    (356, (0x38,)),
    (2140, (0xd2,)),
) for kind in kinds}
MONUMENT_TYPES = frozenset([76, 77, 78, 79, 80, 81, 82, 83, 84, 92, 93, 253, 254, 255, 256, 257, 258, 259, 260])


def parse_table(data, start, limit=None):
    """Sequential membership; unknown tail stops, malformed input clears prefix.

    Prefix records are exposed ONLY after complete known spans, and only for
    the explicit unsupported-tail status. No marker resync or type fitting.
    """
    limit = len(data) if limit is None else limit
    records = []
    def stop(status, index=0):
        return {'status': status, 'records': records if status == 'unsupported_tail' else [],
                'complete': False, 'stop_index': index, 'end': None}
    if not 0 <= start <= limit <= len(data) or limit - start < len(PREAMBLE):
        return stop('truncated')
    if data[start:start + len(PREAMBLE)] != PREAMBLE:
        return stop('preamble')
    at = start + 6
    for index in range(COUNT):
        begin = at
        if at + 2 > limit:return stop('truncated', index)
        tag = struct.unpack_from('<H', data, at)[0];at += 2
        name = ''
        if tag == 0xffff:
            if at + 4 > limit:return stop('truncated', index)
            zero, length = struct.unpack_from('<HH', data, at);at += 4
            if zero or not 1 <= length <= 255:return stop('class_header', index)
            if at + length > limit:return stop('truncated', index)
            try:name = data[at:at + length].decode('ascii')
            except UnicodeDecodeError:return stop('class_header', index)
            if '\0' in name:return stop('class_header', index)
            at += length
        elif tag < 0x8000:
            return stop('unsupported_tag', index)
        if at + 2 > limit:return stop('truncated', index)
        version = struct.unpack_from('<H', data, at)[0]
        if version not in COMMON_BYTES:return stop('unsupported_version', index)
        common_end = at + COMMON_BYTES[version]
        if common_end > limit:return stop('truncated', index)
        kind = struct.unpack_from('<H', data, at + 16)[0]
        if kind in MONUMENT_TYPES:
            if common_end + 4 > limit:return stop('truncated', index)
            subversion = struct.unpack_from('<H', data, common_end + 2)[0]
            if subversion not in (9, 10):return stop('unsupported_tail', index)
            tail = 140 if subversion == 9 else 141
        elif kind in TAIL_BYTES:tail = TAIL_BYTES[kind]
        else:return stop('unsupported_tail', index)
        end = common_end + tail
        if end > limit:return stop('truncated', index)
        x, y = struct.unpack_from('<HH', data, at + 8)
        records.append({'slot': index, 'start': begin, 'body': at, 'end': end,
                        'tag': tag, 'name': name, 'version': version,
                        'alive': data[at + 2], 'size': data[at + 5],
                        'x': x, 'y': y, 'type': kind,
                        'common_rotation': data[at + 112 + (version == 5)]})
        at = end
    return {'status': 'complete', 'records': records, 'complete': True,
            'stop_index': COUNT, 'end': at}


def save_table_start(data, terrain_offset):
    if not 0 <= terrain_offset <= len(data):raise ValueError('terrain anchor bound')
    hits, at = [], 0
    while True:
        found = data.find(PREAMBLE, at, terrain_offset)
        if found < 0:break
        if len(hits) == 16:raise ValueError('building anchor budget')
        hits.append(found);at = found + 1
    if len(hits) != 1:raise ValueError('unique complete building preamble required')
    return hits[0]


def well_model(text):
    rows = re.findall(r'^72,([^,\r\n]+),\{,', text, re.MULTILINE)
    if rows != ['Well']:raise ValueError('unique explicit72/Well Model row required')
    return True


def footprint_control(record, side, terrain):
    if not 1 <= side <= 228 or len(terrain) != 51984:return None
    if record['type'] != 72 or record['name'] != 'cWellBldg' or not record['alive']:
        return None
    n, x, y = record['size'], record['x'], record['y']
    if not 1 <= n <= 8 or x < 0 or y < 0 or x + n > side or y + n > side:return None
    origin = (228 - side) // 2
    indices = [(y + origin + dy) * 228 + x + origin + dx for dy in range(n) for dx in range(n)]
    wrong = [(y + dy) * 228 + x + dx for dy in range(n) for dx in range(n)]
    return {'cells': n * n,
            'on_map': sum(not (int(terrain[i]) & 0x80000) for i in indices),
            'building_flag': sum(bool(int(terrain[i]) & 8) for i in indices),
            'wrong_unshifted_off_map': sum(bool(int(terrain[i]) & 0x80000) for i in wrong)}


def bitmap_names(data):
    if len(data) < 40680:raise ValueError('SG index extent')
    declared, version = struct.unpack_from('<II', data)
    capacity, count, bitmaps = struct.unpack_from('<III', data, 12)
    if (version not in (213, 214) or declared not in (len(data), 74480)
            or not 0 < count < capacity or bitmaps > 200
            or 40680 + (count + 1) * (64 if version == 213 else 72) > len(data)):
        raise ValueError('SG index header/record budgets')
    result = []
    for i in range(bitmaps):
        row = data[680 + 200 * i:880 + 200 * i]
        if b'\0' not in row[:65]:raise ValueError('SG bitmap filename bound')
        name = row[:65].split(b'\0')[0].decode('ascii', 'replace')
        description = row[65:116].split(b'\0')[0].decode('ascii', 'replace')
        result.append((name, description))
    return result


def run(root, manifest):
    from map_save_compare import stable_bytes
    from map_sg3_audit import decode_container, CITY_SIGNATURE
    from save_state_audit import parse_save
    from pak_payload_audit import decode_parts
    sources = manifest.get('sources')
    if not isinstance(sources, list) or not 1 <= len(sources) <= 13:
        raise ValueError('1..13 explicit source entries required')
    root = Path(root)
    if not root.is_absolute() or not root.is_dir():raise ValueError('absolute supplied root required')
    root = root.resolve()
    inputs, decoded_paks, reports = {}, {}, []
    def read(path):
        if path not in inputs:inputs[path] = stable_bytes(root, path)
        return inputs[path]
    well_model(read('Model/EmperorBuildingModels.txt').decode('ascii'))
    for entry in sources:
        kind, path = entry['kind'], entry['path']
        if kind == 'save':
            saved = parse_save(read(path));body = saved.payload;side = saved.side
            terrain = saved.grid('terrain').ravel()
            start = save_table_start(body, saved.terrain_offset);limit = saved.terrain_offset
        else:
            if kind == 'map':body = decode_container(read(path))
            elif kind == 'pak':
                part = entry.get('part')
                if type(part) is not int or not 0 <= part < 12:raise ValueError('physical PAK part required')
                if path not in decoded_paks:decoded_paks[path] = decode_parts(read(path))
                if part >= len(decoded_paks[path]):raise ValueError('PAK part unavailable')
                body = decoded_paks[path][part]
            else:raise ValueError('unsupported source kind')
            if body[:8] != CITY_SIGNATURE or len(body) < 469391:raise ValueError('complete city role required')
            side = struct.unpack_from('<I', body, 84)[0]
            if not 1 <= side <= 228:raise ValueError('city side')
            terrain = struct.unpack_from('<51984I', body, 261455)
            start, limit = TABLE_OFFSET, len(body)
        result = parse_table(body, start, limit)
        target = [r for r in result['records'] if r['type'] == 72]
        named = [r for r in target if r['name'] == 'cWellBldg']
        controls = [footprint_control(r, side, terrain) for r in named]
        accepted = [c for c in controls if c and c['on_map'] == c['cells'] and c['building_flag'] == c['cells']]
        reports.append({'kind': kind, 'status': result['status'], 'complete_table': result['complete'],
                        'declared_records': COUNT, 'validated_prefix_records': len(result['records']),
                        'numeric_well_records': len(target), 'named_well_records': len(named),
                        'validated_named_footprints': len(accepted),
                        'footprint_cells': sum(c['cells'] for c in accepted),
                        'wrong_unshifted_off_map_cells': sum(c['wrong_unshifted_off_map'] for c in accepted),
                        'size_zero_records_not_placed': sum(r['size'] == 0 and r['type'] != 0 for r in result['records']),
                        'image_bindings': 0})
    indexes = sorted(f for f in (root / 'DATA').iterdir() if f.suffix.lower() == '.sg3' and f.is_file())
    if not 1 <= len(indexes) <= 64:raise ValueError('SG index count budget')
    bitmaps, well_labels, safety_atlases = 0, 0, 0
    for index in indexes:
        for name, description in bitmap_names(read(index.relative_to(root).as_posix())):
            bitmaps += 1
            well_labels += bool(re.search(r'(?<![a-z])c?well(?:bldg)?(?![a-z])', name + ' ' + description, re.IGNORECASE))
            safety_atlases += 'safety' in name.lower()
    for path, before in inputs.items():
        if stable_bytes(root, path) != before:raise ValueError('source changed across audit')
    return {'scope': 'bounded public-schema prefix audit;not a general building parser or scene renderer',
            'source_entries': len(reports), 'records': reports,
            'complete_tables': sum(r['complete_table'] for r in reports),
            'validated_prefix_records': sum(r['validated_prefix_records'] for r in reports),
            'named_well_records': sum(r['named_well_records'] for r in reports),
            'validated_named_footprints': sum(r['validated_named_footprints'] for r in reports),
            'footprint_cells': sum(r['footprint_cells'] for r in reports),
            'wrong_unshifted_off_map_cells': sum(r['wrong_unshifted_off_map_cells'] for r in reports),
            'SG_indexes': len(indexes), 'bitmap_names_and_descriptions': bitmaps,
            'well_label_matches': well_labels, 'safety_atlas_labels': safety_atlases,
            'image_bindings_confirmed': 0, 'input_files_unchanged': [len(inputs), len(inputs)],
            'original_game_operations': 0, 'EXE_reads': 0}


def main():
    root = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not root:
        print(json.dumps({'skipped': True, 'reason': 'EMPEROR_GAMEDATA_ROOT unset or empty'}));return
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True, type=Path)
    args = parser.parse_args()
    if not args.manifest.is_absolute() or args.manifest.stat().st_size > 65536:parser.error('bounded absolute local manifest required')
    print(json.dumps(run(root, json.loads(args.manifest.read_text())), sort_keys=True))

if __name__ == '__main__':main()
