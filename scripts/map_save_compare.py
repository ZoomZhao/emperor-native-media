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


# Published SAV read/skip lengths, pinned modern mapper lines137-151.
# Skipped spans remain opaque; they are not new grid/image interpretations.
SAV_BYTE_SPANS = (
    ('terrain', 4 * CELLS), ('following_u32_unknown', 4 * CELLS),
    ('random', CELLS), ('opaque_second_int_skip', 4 * CELLS),
    ('opaque_table_skip', 708025), ('opaque_fertility_skip', CELLS),
    ('opaque_grid_extra_skip', 75184), ('stone', CELLS),
    ('opaque_int_extra_skip', 260027), ('fertile', CELLS),
    ('opaque_before_objects_skip', 143754), ('objects', 4 * CELLS),
)


def saved_byte_partition(saved):
    if not 0 <= saved.size_offset < saved.terrain_offset <= len(saved.payload):
        raise ValueError('ordered SAV anchors required')
    regions = [('opaque_prefix_before_size', 0, saved.size_offset),
               ('opaque_size_to_terrain', saved.size_offset, saved.terrain_offset)]
    position = saved.terrain_offset
    for name, length in SAV_BYTE_SPANS:
        end = position + length
        if end > len(saved.payload):
            raise ValueError('complete public read/skip spans required')
        regions.append((name, position, end))
        position = end
    regions.append(('opaque_remaining_tail', position, len(saved.payload)))
    return regions


def compare_saved_bytes(before, after, target):
    """Exact aligned-byte comparison; no padding, offset fitting or opaque decode."""
    import numpy as np
    if ((before.side, before.size_offset, before.terrain_offset, len(before.payload))
            != (after.side, after.size_offset, after.terrain_offset, len(after.payload))):
        raise ValueError('byte alignment requires identical side, anchors and decoded length')
    centres, _ = sample_indices({'target': [target]}, before.side)['target']
    y, x = centres[0]
    regions = saved_byte_partition(before)
    if regions != saved_byte_partition(after):
        raise ValueError('structural byte partition differs')
    changed = np.frombuffer(before.payload, 'u1') != np.frombuffer(after.payload, 'u1')
    known = np.zeros(len(changed), dtype=bool)
    descriptors = {name: (offset, width) for name, (_, offset, width) in COMMON.items()}
    descriptors.update(SAVED_ONLY)
    fields = {}
    for name, (offset, width) in descriptors.items():
        start = before.terrain_offset + offset
        known[start:start + width * CELLS] = True
        a, b = (grid(saved.payload, start, width) for saved in (before, after))
        if name == 'terrain' and (int(a[y, x]) | int(b[y, x])) & 0x80000:
            raise ValueError('target outside on-map terrain')
        fields[name] = counts(a, b)
        fields[name]['target_changed'] = bool(a[y, x] != b[y, x])
    rows = [{'region': name, 'bytes': end - start,
             'changed_bytes': int(changed[start:end].sum())} for name, start, end in regions]
    total = int(changed.sum())
    if (sum(row['bytes'] for row in rows) != len(changed)
            or sum(row['changed_bytes'] for row in rows) != total):
        raise ValueError('complete byte partition accounting failed')
    return {'decoded_bytes_compared': len(changed), 'changed_bytes': total,
            'accounted_changed_bytes': total, 'partition_regions': rows,
            'six_field_bytes': int(known.sum()), 'six_field_changed_bytes': int(changed[known].sum()),
            'outside_six_field_bytes': int((~known).sum()),
            'outside_six_field_changed_bytes': int(changed[~known].sum()),
            'six_fields': fields,
            'only_target_terrain_value_changed': (fields['terrain']['equal_values'] == CELLS - 1
                                                  and fields['terrain']['target_changed']),
            'selector_status': 'unsupported', 'graphics_field_counterpart_identified': False,
            'absolute_screen_binding_verified': False, 'snapshot_instant_pairing_verified': False,
            'scope': 'aligned raw snapshot bytes only; opaque changes have no graphics or causal meaning',
            'original_game_inputs': 0}


def audit_snapshot_pair(root, manifest):
    if not isinstance(manifest, dict) or set(manifest) != {'before', 'after', 'target'}:
        raise ValueError('explicit before/after/target snapshot manifest required')
    for name in ('before', 'after'):
        if not isinstance(manifest[name], str) or not 0 < len(manifest[name]) <= 1024:
            raise ValueError('bounded relative save name required')
    if manifest['before'] == manifest['after']:
        raise ValueError('two explicit save paths required')
    originals = {name: stable_bytes(root, manifest[name]) for name in ('before', 'after')}
    before, after = (parse_save(originals[name]) for name in ('before', 'after'))
    result = compare_saved_bytes(before, after, manifest['target'])
    for name, data in originals.items():
        if stable_bytes(root, manifest[name]) != data:
            raise ValueError('input changed across complete audit')
    result['input_files_unchanged'] = [2, 2]
    return result


def main():
    env = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not env:
        print(json.dumps({'skipped': True, 'reason': 'EMPEROR_GAMEDATA_ROOT unset/empty'}))
        return 0
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--snapshot-pair', action='store_true',
                        help='compare two SAVs and all aligned decoded bytes; no MAP identity or selector inferred')
    args = parser.parse_args()
    root = Path(env)
    try:
        if not root.is_absolute() or not root.is_dir():
            raise ValueError('existing absolute environment root required')
        if args.manifest.stat().st_size > 65536:
            raise ValueError('manifest budget')
        runner = audit_snapshot_pair if args.snapshot_pair else audit
        result = runner(root.resolve(), json.loads(args.manifest.read_text()))
        print(json.dumps(result, ensure_ascii=False))
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(str(error), file=__import__('sys').stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
