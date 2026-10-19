"""Bounded exact MAP-surface/SAV byte hypotheses; no image-field inference.

Original input only through EMPEROR_GAMEDATA_ROOT. Output contains aggregates,
never grid values, pixels or per-record hashes. See docs/SAV.md for exclusions.
"""
import argparse
import json
import os
from pathlib import Path


ENCODINGS = ((4, 0, '<'), (4, 0, '>'), (2, 0, '<'),
             (2, 0, '>'), (2, 16, '<'), (2, 16, '>'))
HIT_CAP = 64


def locations(data, pattern):
    """Bounded overlapping hits; cap is saturation, never a negative result."""
    if not pattern:
        return [], False
    # Exact acceleration for long mostly-zero grids: anchor a window containing
    # the least frequent present byte, then verify the ENTIRE original pattern.
    # Every complete hit contains this window, so no candidate is omitted.
    offset, needle = 0, pattern
    if len(pattern) > 4096:
        import numpy as np
        counts = np.bincount(np.frombuffer(pattern, dtype='u1'), minlength=256)
        present = np.flatnonzero(counts)
        rare = int(present[np.argmin(counts[present])])
        offset = min(max(0, pattern.index(bytes([rare])) - 32), len(pattern) - 64)
        needle = pattern[offset:offset + 64]
    hits, start = [], 0
    while True:
        anchor = data.find(needle, start)
        if anchor < 0:
            return hits, False
        at = anchor - offset
        if at >= 0 and data.startswith(pattern, at):
            if len(hits) == HIT_CAP:
                return hits, True
            hits.append(at)
        start = anchor + 1


def projected(array, width, shift, endian):
    import numpy as np
    return np.asarray((array.astype('uint64') >> shift) & ((1 << (8 * width)) - 1),
                      dtype=endian + 'u' + str(width))


def tiles(array, size):
    import numpy as np
    n = len(array)
    return np.concatenate([array[y:y + size, x:x + size].ravel()
                           for y in range(0, n, size) for x in range(0, n, size)])


def sparse_records(values, original, kind, endian):
    import numpy as np
    indices = np.flatnonzero(original.ravel() != 0)
    value = values.ravel()[indices]
    if kind == 'values':
        return value.tobytes()
    if kind.startswith('index'):
        dtype = np.dtype([('i', endian + 'u' + kind[-1]), ('v', values.dtype)])
        records = np.empty(len(indices), dtype=dtype)
        records['i'], records['v'] = indices, value
    else:
        width = kind[-1]
        dtype = np.dtype([('x', endian + 'u' + width), ('y', endian + 'u' + width),
                          ('v', values.dtype)])
        records = np.empty(len(indices), dtype=dtype)
        records['x'], records['y'] = indices % len(original), indices // len(original)
        records['v'] = value
    return records.tobytes()


def search_surface(surface, side, payload):
    import numpy as np
    if surface.shape != (228, 228) or not 16 <= side <= 228 or len(payload) > 8 * 1024 * 1024:
        raise ValueError('bounded grid/payload profile')
    origin = (228 - side) // 2
    crop = surface[origin:origin + side, origin:origin + side]
    forms = [('full_rows', surface), ('crop_rows', crop),
             ('full_columns', surface.T), ('crop_columns', crop.T)]
    result = {'whole_patterns': 0, 'whole_pattern_hits': 0, 'whole_saturated': 0,
              'whole_zero_skips': 0, 'row_fragments': 0, 'row_fragment_hits': 0,
              'row_low_information_skips': 0, 'row_saturated': 0,
              'row_geometry_candidates': 0, 'row_complete_candidates': 0,
              'padded_layout_variants': 0, 'padded_layout_variants_excluded': 0,
              'row_layouts_without_eligible_fragments': 0,
              'tile_fragments': 0, 'tile_fragment_hits': 0,
              'tile_low_information_skips': 0, 'tile_saturated': 0,
              'families': {}, 'candidate_semantics_verified': False}
    def whole(pattern, family, nonzero):
        if not nonzero:
            result['whole_zero_skips'] += 1
            return
        result['whole_patterns'] += 1
        family_counts = result['families'].setdefault(family, {'tested': 0, 'hits': 0})
        family_counts['tested'] += 1
        hits, capped = locations(payload, pattern)
        result['whole_pattern_hits'] += bool(hits)
        family_counts['hits'] += bool(hits)
        result['whole_saturated'] += capped
    for name, original in forms:
        n = len(original)
        for width, shift, endian in ENCODINGS:
            values = projected(original, width, shift, endian)
            nonzero = bool(np.any(values))
            rows = [row.tobytes() for row in values]
            whole(values.tobytes(), 'dense', nonzero)
            for size in (8, 16):
                whole(tiles(values, size).tobytes(), 'block_major', nonzero)
            if name.endswith('rows'):
                for kind in ('values', 'index2', 'index4', 'xy1', 'xy2'):
                    whole(sparse_records(values, original, kind, endian), 'sparse_packed',
                          bool(np.any(values.ravel()[original.ravel() != 0])))
            fragments = []
            for y in (n // 4, n // 2, 3 * n // 4):
                for x in (0, (n - 16) // 3, 2 * (n - 16) // 3, n - 16):
                    part = values[y, x:x + 16]
                    if len(np.unique(part[part != 0])) < 4:
                        result['row_low_information_skips'] += 1
                        continue
                    result['row_fragments'] += 1
                    hits, capped = locations(payload, part.tobytes())
                    result['row_fragment_hits'] += bool(hits)
                    result['row_saturated'] += capped
                    fragments.append((y, x, hits))
            result['padded_layout_variants'] += len(range(0, 65, width))
            if not fragments:
                result['row_layouts_without_eligible_fragments'] += 1
            elif any(not hits for y, x, hits in fragments):
                # A complete grid must contain every selected row segment,
                # irrespective of intervening padding content or stride.
                result['padded_layout_variants_excluded'] += len(range(0, 65, width))
            # Any row may have 0..64 opaque padding bytes. This does not compare
            # the padding and does not search arbitrary row strides or origins.
            candidates = set()
            for padding in range(0, 65, width):
                stride = n * width + padding
                votes = {}
                for y, x, hits in fragments:
                    for at in hits:
                        base = at - y * stride - x * width
                        if base >= 0 and base + (n - 1) * stride + n * width <= len(payload):
                            votes.setdefault(base, set()).add((y, x))
                for base, points in votes.items():
                    if len(points) < 3 or len({y for y, x in points}) < 2:
                        continue
                    candidates.add((base, stride))
            result['row_geometry_candidates'] += len(candidates)
            for base, stride in candidates:
                exact = all(payload[base + y * stride:base + y * stride + n * width] == rows[y]
                            for y in range(n))
                result['row_complete_candidates'] += exact
            for size in (8, 16):
                positions = [(y, x) for y in range(0, n - size + 1, size)
                             for x in range(0, n - size + 1, size)]
                for y, x in (positions[0], positions[len(positions) // 2], positions[-1]):
                    part = values[y:y + size, x:x + size]
                    if len(np.unique(part[part != 0])) < 4:
                        result['tile_low_information_skips'] += 1
                        continue
                    result['tile_fragments'] += 1
                    hits, capped = locations(payload, part.tobytes())
                    result['tile_fragment_hits'] += bool(hits)
                    result['tile_saturated'] += capped
    result['no_hits_in_tested_patterns'] = not any(result[k] for k in
        ('whole_pattern_hits', 'row_fragment_hits', 'tile_fragment_hits',
         'whole_saturated', 'row_saturated', 'tile_saturated'))
    return result


def run(root, manifest):
    from map_sg3_audit import decode_container, CITY_SIGNATURE
    from save_state_audit import parse_save
    from map_save_compare import stable_bytes, COMMON
    import numpy as np
    pairs = manifest.get('pairs')
    if not isinstance(pairs, list) or not 1 <= len(pairs) <= 7:
        raise ValueError('1..7 explicit pairs required')
    inputs, cache, records = {}, {}, []
    def read(name):
        if name not in inputs:
            inputs[name] = stable_bytes(root, name)
        return inputs[name]
    for entry in pairs:
        if set(entry) != {'map', 'save', 'role'}:
            raise ValueError('explicit pair and role required')
        name = entry['map']
        if name not in cache:
            body = decode_container(read(name))
            if body[:8] != CITY_SIGNATURE or len(body) < 261455 + 4 * 51984:
                raise ValueError('complete city MAP required')
            side = int.from_bytes(body[84:88], 'little')
            if not 16 <= side <= 228:
                raise ValueError('city side outside protocol')
            cache[name] = (np.frombuffer(body, '<u4', 51984, 1535).reshape(228, 228),
                           np.frombuffer(body, '<u4', 51984, 261455).copy(), side, body)
        surface, terrain, side, map_body = cache[name]
        saved = parse_save(read(entry['save']))
        if side != saved.side:
            raise ValueError('pair sides disagree')
        same = int(np.count_nonzero(saved.grid('terrain').ravel() == terrain))
        positive_controls = 0
        for field, (map_offset, relative, width) in COMMON.items():
            pattern = map_body[map_offset:map_offset + width * 51984]
            expected = saved.terrain_offset + relative
            unchanged = pattern == saved.payload[expected:expected + len(pattern)]
            if unchanged:
                hits, capped = locations(saved.payload, pattern)
                if expected not in hits or capped:
                    raise ValueError('known named-field positive control failed/saturated')
                positive_controls += 1
        report = search_surface(surface, side, saved.payload)
        records.append({'role': entry['role'], 'terrain_equal_values': same,
                        'terrain_values': 51984, 'known_unchanged_field_controls': positive_controls,
                        'known_fields_compared': 4, 'decoded_SAV_bytes': len(saved.payload),
                        'surface_search': report})
    for name, original in inputs.items():
        if stable_bytes(root, name) != original:
            raise ValueError('input changed across full audit')
    return {'protocol_version': 1, 'pairs': len(records), 'MAP_paths': len(cache),
            'SAV_paths': len({e['save'] for e in pairs}), 'records': records,
            'input_files_unchanged': [len(inputs), len(inputs)],
            'identified_SAV_image_fields': 0, 'new_runtime_or_EXE_access': False,
            'scope': 'exact finite layout hypotheses only; hits are candidates, not image semantics'}


def main():
    # Successful no-root skip precedes argument/manifest/NumPy/data access.
    root = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not root:
        print(json.dumps({'skipped': True, 'reason': 'EMPEROR_GAMEDATA_ROOT unset or empty'}))
        return
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True, type=Path)
    args = parser.parse_args()
    if not Path(root).is_absolute() or not args.manifest.is_absolute():
        parser.error('absolute root and local manifest required')
    if args.manifest.stat().st_size > 65536:
        parser.error('manifest budget')
    print(json.dumps(run(root, json.loads(args.manifest.read_text())), sort_keys=True))


if __name__ == '__main__':
    main()
