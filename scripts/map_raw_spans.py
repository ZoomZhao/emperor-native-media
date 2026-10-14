"""Finite raw MAP spans from published skip boundaries; no graphical semantics.

Read-only original input uses EMPEROR_GAMEDATA_ROOT exclusively. No game code,
image payload, sliding offsets, fitted encoding or source-content lookup rule.
"""
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import argparse
import json
import os
import struct

from map_sg3_audit import (AuditError, CITY_SIGNATURE, CELLS, SIDE, TERRAIN,
                           MAX_INPUT, MAX_DECODED, MAX_FILES, decode_container,
                           read_sg3, u32_grid)


@dataclass(frozen=True)
class RawSpan:
    name: str
    offset: int
    width: int
    basis: str

    @property
    def end(self):
        return self.offset + CELLS * self.width


# Pinned public city reader, bb97d7c: skip1447+5*N (last grid edges),
# post-objects skipN, post-random skipN+8+2*N. Interior splits are candidates.
BYTE_OFFSETS = (1535, 53519, 105503, 157487, 677327, 781295, 833287, 885271)
SPANS = tuple(RawSpan('skipped_byte_grid_' + str(i), offset, 1, 'public_skip_units')
              for i, offset in enumerate(BYTE_OFFSETS)) + (
    RawSpan('prefix_pair_0_u16', 1535, 2, 'length_only_alternative'),
    RawSpan('prefix_pair_1_u16', 105503, 2, 'length_only_alternative'),
    RawSpan('prefix_quad_u32', 1535, 4, 'length_only_alternative'),
)
SPAN_END = max(s.end for s in SPANS)


def city_payload(payload):
    if (len(payload) > MAX_DECODED or len(payload) < SPAN_END
            or payload[:8] != CITY_SIGNATURE):
        raise AuditError('complete candidate spans/city signature/budget required')
    side = struct.unpack_from('<I', payload, 84)[0]
    if not 0 < side <= SIDE:
        raise AuditError('city active-side bounds')
    return side, u32_grid(payload, TERRAIN)


def raw_values(payload, span):
    if span not in SPANS or span.end > len(payload) or len(payload) > MAX_DECODED:
        raise AuditError('fixed raw span extent/descriptor/budget')
    code = {1: 'B', 2: 'H', 4: 'I'}[span.width]
    return struct.unpack_from('<' + str(CELLS) + code, payload, span.offset)


def lookup_candidate(value, encoding, archive):
    if encoding == 'direct_id':
        return value if 0 < value <= archive.count else None
    if encoding != 'public_index_slot':
        raise AuditError('only two declared encodings allowed')
    if not 0 <= value < len(archive.groups):
        return None
    result = archive.groups[value]
    return result if 0 < result <= archive.count else None


def sample_indices(samples, side):
    if not isinstance(samples, list) or not 1 <= len(samples) <= 128:
        raise AuditError('bounded nonempty sample partition')
    origin = (SIDE - side) // 2
    result = []
    for sample in samples:
        if not isinstance(sample, dict):
            raise AuditError('sample object required')
        x, y, image_id = (sample.get(k) for k in ('x', 'y', 'content_id'))
        if (any(type(v) is not int for v in (x, y, image_id))
                or not 0 <= x < side or not 0 <= y < side or image_id <= 0):
            raise AuditError('active coordinate/positive content-ID bounds')
        result.append(((y + origin) * SIDE + x + origin, image_id))
    if len({i for i, _ in result}) != len(result):
        raise AuditError('unique sample coordinates required')
    return result


def compare_samples(payload, archive, training, heldout):
    side, _ = city_payload(payload)
    train, hold = sample_indices(training, side), sample_indices(heldout, side)
    if {i for i, _ in train} & {i for i, _ in hold}:
        raise AuditError('disjoint discovery/heldout required')
    if any(label not in archive.type30_ids for _, label in train + hold):
        raise AuditError('labels require declared type30 source metadata')
    repeated = [(a, b) for a in range(len(train)) for b in range(a + 1, len(train))
                if train[a][1] == train[b][1]]
    results = []
    for span in SPANS:
        values = raw_values(payload, span)
        equal = sum(values[train[a][0]] == values[train[b][0]] for a, b in repeated)
        relations = []
        for encoding in ('direct_id', 'public_index_slot'):
            predictions = [lookup_candidate(values[i], encoding, archive) for i, _ in train]
            matches = sum(p == label for p, (_, label) in zip(predictions, train))
            survived = matches == len(train)
            # Heldout values are inspected only after every training point passes.
            validation = (sum(lookup_candidate(values[i], encoding, archive) == label
                              for i, label in hold) if survived else None)
            relations.append({'encoding': encoding, 'discovery_points': len(train),
                              'valid_discovery_references': sum(p is not None for p in predictions),
                              'discovery_matches': matches, 'survived_discovery': survived,
                              'heldout_evaluated': len(hold) if survived else 0,
                              'heldout_matches': validation})
        results.append({'layout': span.name, 'basis': span.basis,
                        'same_content_pairs': len(repeated), 'discovery_raw_distinct': len({values[i] for i, _ in train}),
                        'same_content_raw_equal': equal,
                        'same_content_raw_different': len(repeated) - equal, 'relations': relations})
    return {'discovery_points': len(train), 'reserved_heldout_points': len(hold),
            'hypotheses': len(SPANS) * 2,
            'survivors': sum(r['survived_discovery'] for s in results for r in s['relations']),
            'results': results, 'verified_map_sg3_join': False,
            'label_scope': 'supplied source-content IDs, not proven engine-used archive or IDs'}


def inside_root(root, relative):
    if not isinstance(relative, str) or Path(relative).is_absolute():
        raise AuditError('relative data path required')
    target = (root / relative).resolve(strict=True)
    if root not in target.parents or not target.is_file():
        raise AuditError('data path escapes root/not file')
    return target


def audit_root(root, manifest=None):
    if not root.is_absolute() or not root.is_dir():
        raise AuditError('existing absolute data root required')
    root = root.resolve(strict=True)
    paths = sorted(p for p in root.rglob('*') if p.is_file() and p.suffix.lower() == '.map')
    if not 1 <= len(paths) <= MAX_FILES:
        raise AuditError('MAP corpus file budget')
    stats = {s.name: {'values': 0, 'zero_values': 0, 'minimum': None, 'maximum': None,
                     'distinct': set(), 'distinct_exact': True} for s in SPANS}
    cities = excluded = stable = failed = 0
    target_payload = None
    if manifest is not None and (not isinstance(manifest, dict)
                     or any(not isinstance(manifest.get(k), list) or not 1 <= len(manifest[k]) <= 128
                            for k in ('training', 'heldout'))):
        raise AuditError('bounded sample manifest partitions')
    target_path = inside_root(root, manifest['map']) if manifest is not None else None
    for path in paths:
        relative = path.relative_to(root)
        if relative.parts[0].lower() != 'cities':
            excluded += 1
            continue
        if path.resolve(strict=True).is_relative_to(root) is False:
            raise AuditError('MAP symlink escapes root')
        if path.stat().st_size > MAX_INPUT:
            failed += 1
            continue
        data = path.read_bytes()
        try:
            payload = decode_container(data)
            city_payload(payload)
            for span in SPANS:
                local = Counter(raw_values(payload, span)); item = stats[span.name]
                item['values'] += CELLS; item['zero_values'] += local[0]
                item['minimum'] = min(local) if item['minimum'] is None else min(item['minimum'], min(local))
                item['maximum'] = max(local) if item['maximum'] is None else max(item['maximum'], max(local))
                for value in local:
                    if value not in item['distinct']:
                        if len(item['distinct']) == 65536:
                            item['distinct_exact'] = False
                        else:
                            item['distinct'].add(value)
        except AuditError:
            failed += 1
            continue
        cities += 1
        stable += data == path.read_bytes()
        if target_path and path.resolve(strict=True) == target_path:
            target_payload = payload
    result = {'skipped': False, 'city_files': len(paths) - excluded, 'city_span_sets': cities,
              'failed_city_sets': failed, 'excluded_noncity_paths': excluded,
              'fixed_layouts': len(SPANS), 'primary_byte_layouts': len(BYTE_OFFSETS),
              'length_only_alternatives': 3, 'complete_candidate_spans': cities * len(SPANS),
              'unchanged_two_read_inputs': stable,
              'stability_scope': 'same original file bytes in two reads; not runtime stability',
              'layouts': [{'name': s.name, 'offset': s.offset, 'width': s.width, 'end': s.end,
                           'basis': s.basis, 'values': stats[s.name]['values'],
                           'distinct_values_or_lower_bound': len(stats[s.name]['distinct']),
                           'distinct_exact': stats[s.name]['distinct_exact'],
                           'zero_values': stats[s.name]['zero_values'],
                           'minimum': stats[s.name]['minimum'], 'maximum': stats[s.name]['maximum']} for s in SPANS],
              'verified_map_sg3_joins': 0, 'graphical_layer_semantics': 'unknown',
              'original_inputs': 0, 'image_payload_reads': 0}
    if manifest is not None:
        if target_payload is None:
            raise AuditError('sample MAP absent from verified city sets')
        ids = {s['content_id'] for s in manifest['training'] + manifest['heldout']}
        archive = read_sg3(inside_root(root, manifest['archive']), ids)
        result['source_archives_read'] = 1
        result['label_audit'] = compare_samples(target_payload, archive, manifest['training'], manifest['heldout'])
    result['accepted_raw_spans'] = cities > 0 and failed == 0 and stable == cities
    return result


def main():
    root = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not root:
        print(json.dumps({'skipped': True, 'reason': 'EMPEROR_GAMEDATA_ROOT unset'}))
        return 0
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sample-manifest', type=Path)
    args = parser.parse_args()
    try:
        if args.sample_manifest and args.sample_manifest.stat().st_size > 256 * 1024:
            raise AuditError('sample manifest budget')
        manifest = json.loads(args.sample_manifest.read_text()) if args.sample_manifest else None
        report = audit_root(Path(root), manifest)
    except (AuditError, OSError, KeyError, TypeError, ValueError) as error:
        print(json.dumps({'skipped': False, 'accepted_raw_spans': False, 'error': str(error)}))
        return 1
    print(json.dumps(report, sort_keys=True))
    return 0 if report['accepted_raw_spans'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
