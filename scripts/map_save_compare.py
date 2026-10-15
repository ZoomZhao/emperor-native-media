"""Aggregate-only MAP/SAV field correspondence; no image-field equivalence inferred.

Published modern mapper names establish four cross-format pairs. Other extents
stay unpaired. Uses strict existing framing and SAV candidate selection, never
MAP content to select the SAV anchor. Original data only through process ENV.
"""
from pathlib import Path
import argparse
import json
import os
import struct

from map_sg3_audit import decode_container, CITY_SIGNATURE, CELLS
from save_state_audit import parse_save, read_bounded, rooted

COMMON = {'terrain': (261455, 0, 4), 'objects': (469391, 2018734, 4),
          'random': (729311, 415872, 1), 'fertile': (937255, 1822996, 1)}
SAVED_ONLY = {'following_u32_unknown': (207936, 4), 'stone': (1510985, 1)}
STEPS = ((0, 0), (-1, 0), (1, 0), (0, -1), (0, 1))


def stable_bytes(root, relative):
    path = rooted(root, relative)
    before = path.stat()
    first = read_bounded(path)
    second = read_bounded(path)
    after = path.stat()
    fields = ('st_dev', 'st_ino', 'st_size', 'st_mtime_ns')
    if first != second or any(getattr(before, k) != getattr(after, k) for k in fields):
        raise ValueError('input changed during two reads')
    return first


def grid(payload, offset, width):
    import numpy as np
    if width not in (1, 4) or offset < 0 or offset + width * CELLS > len(payload):
        raise ValueError('complete known field extent required')
    return np.frombuffer(payload, '<u4' if width == 4 else 'u1', CELLS, offset).reshape(228, 228)


def sample_indices(groups, side):
    if type(side) is not int or not 1 <= side <= 228:
        raise ValueError('supported active side required')
    if not isinstance(groups, dict) or len(groups) > 32:
        raise ValueError('bounded named sample groups required')
    origin = (228 - side) // 2
    result = {}
    total = 0
    for name, points in groups.items():
        if not isinstance(name, str) or not 1 <= len(name) <= 80 or not isinstance(points, list):
            raise ValueError('group name/cells required')
        total += len(points)
        if not points or len(points) > 128 or total > 256:
            raise ValueError('cell count budget')
        centres, neighbours = set(), set()
        for point in points:
            if (not isinstance(point, (tuple, list)) or len(point) != 2
                    or any(type(v) is not int for v in point)
                    or not all(0 <= v < side for v in point)):
                raise ValueError('canonical cell outside declared active square')
            x, y = point
            centres.add((y + origin, x + origin))
            for dx, dy in STEPS:
                if 0 <= x + dx < side and 0 <= y + dy < side:
                    neighbours.add((y + dy + origin, x + dx + origin))
        if len(centres) != len(points):
            raise ValueError('duplicate group cell')
        result[name] = (sorted(centres), sorted(neighbours))
    return result


def take(array, indices):
    import numpy as np
    return np.array([array[y, x] for y, x in indices])


def counts(a, b):
    import numpy as np
    return {'equal_values': int(np.count_nonzero(a == b)), 'compared_values': int(a.size)}


def categories(a):
    import numpy as np
    return {'values': int(a.size), 'zero_values': int(np.count_nonzero(a == 0)),
            'distinct_values': int(np.unique(a).size)}


def compare_pair(map_payload, saved, groups):
    import numpy as np
    if map_payload[:8] != CITY_SIGNATURE or len(map_payload) < 88:
        raise ValueError('verified city MAP role required')
    side = struct.unpack_from('<I', map_payload, 84)[0]
    if not 1 <= side <= 228 or side != saved.side:
        raise ValueError('MAP/SAV declared side differs')
    indices = sample_indices(groups, side)
    maps, saves = {}, {}
    for name, (map_offset, save_offset, width) in COMMON.items():
        maps[name] = grid(map_payload, map_offset, width)
        saves[name] = grid(saved.payload, saved.terrain_offset + save_offset, width)
    for name, (offset, width) in SAVED_ONLY.items():
        saves[name] = grid(saved.payload, saved.terrain_offset + offset, width)
    # A complete array equality gate follows independent structural selection.
    # Filename, samples and MAP contents never select or rank an anchor.
    if not np.array_equal(maps['terrain'], saves['terrain']):
        raise ValueError('whole terrain source-identity gate failed')
    on_map = (maps['terrain'] & 0x80000) == 0
    fields = {name: {'whole': counts(maps[name], saves[name]),
                     'on_map': counts(maps[name][on_map], saves[name][on_map])}
              for name in COMMON}
    samples = {}
    for name, (centres, neighbours) in indices.items():
        row = {}
        for scope, points in [('centres', centres), ('cardinal_union', neighbours)]:
            changed = np.zeros(len(points), dtype=bool)
            common = {}
            for field in COMMON:
                a, b = take(maps[field], points), take(saves[field], points)
                changed |= a != b
                common[field] = counts(a, b)
            row[scope] = {'distinct_cells': len(points), 'any_common_field_changed': int(changed.sum()),
                          'named_fields': common,
                          'following_u32_unknown': categories(take(saves['following_u32_unknown'], points)),
                          'stone': categories(take(saves['stone'], points))}
        samples[name] = row
    return {'side': side, 'whole_terrain_identity_verified': True, 'fields': fields,
            'groups': samples, 'saved_only': {name: categories(saves[name]) for name in SAVED_ONLY}}, saves


def audit(root, manifest):
    import numpy as np
    if not isinstance(manifest, dict):
        raise ValueError('pair manifest object required')
    pairs = manifest.get('pairs')
    repeats = manifest.get('repeats', [])
    if not isinstance(pairs, list) or not 1 <= len(pairs) <= 16 or not isinstance(repeats, list) or len(repeats) > 16:
        raise ValueError('bounded pair/repeat manifest required')
    rows, retained, originals = [], [], {}
    for pair in pairs:
        if not isinstance(pair, dict) or set(pair) - {'map', 'save', 'groups'} or not {'map', 'save'} <= set(pair):
            raise ValueError('explicit pair manifest fields required')
        map_bytes = stable_bytes(root, pair['map'])
        saved_bytes = stable_bytes(root, pair['save'])
        for relative, data in [(pair['map'], map_bytes), (pair['save'], saved_bytes)]:
            if relative in originals and originals[relative] != data:
                raise ValueError('source changed between pairs')
            originals[relative] = data
        saved = parse_save(saved_bytes)
        row, arrays = compare_pair(decode_container(map_bytes), saved, pair.get('groups', {}))
        row.update({'map': pair['map'], 'save': pair['save'], 'two_read_stable': True})
        rows.append(row)
        retained.append(arrays)
    temporal = []
    for pair in repeats:
        if (not isinstance(pair, list) or len(pair) != 2 or any(type(i) is not int or not 0 <= i < len(rows) for i in pair)
                or pair[0] == pair[1] or rows[pair[0]]['map'] != rows[pair[1]]['map']):
            raise ValueError('explicit same-source snapshot indices required')
        a, b = pair
        groups_a, groups_b = pairs[a].get('groups', {}), pairs[b].get('groups', {})
        if groups_a != groups_b:
            raise ValueError('snapshot sample groups differ')
        groups = {}
        for name, (centres, neighbours) in sample_indices(groups_a, rows[a]['side']).items():
            groups[name] = {scope: {field: counts(take(retained[a][field], points),
                                                 take(retained[b][field], points))
                                   for field in retained[a]}
                            for scope, points in [('centres', centres), ('cardinal_union', neighbours)]}
        temporal.append({'pair_indices': pair, 'fields': {name: counts(retained[a][name], retained[b][name])
                                                          for name in retained[a]}, 'groups': groups})
    for relative, original in originals.items():
        if stable_bytes(root, relative) != original:
            raise ValueError('input changed across complete audit')
    return {'pairs_verified': len(rows), 'common_field_arrays_compared': len(rows) * len(COMMON),
            'input_files_unchanged': [len(originals), len(originals)],
            'common_field_values_compared': len(rows) * len(COMMON) * CELLS,
            'graphics_field_counterpart_identified': False,
            'unpaired_fields': ['MAP prefix candidate: no identified SAV counterpart',
                               'MAP edges: no identified SAV counterpart',
                               'SAV following_u32_unknown: not MAP objects by adjacency',
                               'SAV stone: no identified MAP counterpart'],
            'rows': rows, 'snapshot_comparisons': temporal,
            'scope': 'literal persisted raw fields; no snapshot-instant screenshot, memory, graphics consumer or causal rule',
            'original_game_inputs': 0}


def main():
    env = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not env:
        print(json.dumps({'skipped': True, 'reason': 'EMPEROR_GAMEDATA_ROOT unset/empty'}))
        return 0
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    root = Path(env)
    try:
        if not root.is_absolute() or not root.is_dir():
            raise ValueError('existing absolute environment root required')
        if args.manifest.stat().st_size > 65536:
            raise ValueError('manifest budget')
        result = audit(root.resolve(), json.loads(args.manifest.read_text()))
        print(json.dumps(result, ensure_ascii=False))
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(str(error), file=__import__('sys').stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
