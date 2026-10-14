"""Authored finite-span bounds and discovery/heldout regressions; no GameData."""
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
import map_raw_spans as audit
from map_sg3_audit import SGMetadata


def city():
    p = bytearray(audit.SPAN_END)
    p[:8] = audit.CITY_SIGNATURE
    struct.pack_into('<I', p, 84, 140)
    return p


def framed(body):
    p = bytearray(struct.pack('<I', 0xfedcbaaa))
    for i in range(0, len(body), 32768):
        raw = body[i:i + 32768]; compressed = zlib.compress(raw)
        p += struct.pack('<III', 0xabcdefff, len(compressed), len(raw)) + compressed
    return p


def archive():
    groups = [0] * 300
    groups[0], groups[1], groups[2], groups[299] = 3, 4, 99, 4
    return SGMetadata(8, 20, tuple(groups), frozenset((3, 4)))


class RawSpanTests(unittest.TestCase):
    def test_frozen_layouts_and_no_tail_guess(self):
        self.assertEqual((len(audit.SPANS), len(audit.BYTE_OFFSETS)), (11, 8))
        self.assertEqual(audit.SPAN_END, 937255)
        self.assertEqual(audit.SPANS[10].end, 209471)
        self.assertFalse(any(s.offset >= 989239 for s in audit.SPANS))

    def test_every_span_row_boundaries_unsigned_and_last_byte(self):
        for span in audit.SPANS:
            with self.subTest(span=span.name):
                body = bytearray(span.end)
                limit = (1 << (span.width * 8)) - 1
                values = [(0, 1), (227, limit), (228, limit - 1), (audit.CELLS - 1, limit - 2)]
                for index, value in values:
                    body[span.offset + index * span.width:span.offset + (index + 1) * span.width] = value.to_bytes(span.width, 'little')
                parsed = audit.raw_values(body, span)
                self.assertEqual(len(parsed), 51984)
                self.assertEqual([parsed[i] for i, _ in values], [v for _, v in values])
                with self.assertRaises(audit.AuditError):
                    audit.raw_values(body[:-1], span)

    def test_preflight_no_partial_set_wrong_role_side_and_budget(self):
        body = city(); self.assertEqual(audit.city_payload(body)[0], 140)
        for bad in (body[:-1], bytes(audit.MAX_DECODED + 1), b'wrong header'):
            with self.assertRaises(audit.AuditError): audit.city_payload(bad)
        for side in (0, 229):
            bad = body.copy(); struct.pack_into('<I', bad, 84, side)
            with self.assertRaises(audit.AuditError): audit.city_payload(bad)
        bad = body.copy(); bad[0] = 2
        with self.assertRaises(audit.AuditError): audit.city_payload(bad)

    def test_only_registered_descriptors(self):
        for descriptor in (audit.RawSpan('invented', 1536, 1, 'none'), audit.RawSpan('invented', -1, 4, 'none')):
            with self.assertRaises(audit.AuditError): audit.raw_values(city(), descriptor)

    def test_exact_direct_and_public_slot_bounds_without_map_sentinel_guess(self):
        a = archive()
        self.assertIsNone(audit.lookup_candidate(0, 'direct_id', a))
        self.assertEqual(audit.lookup_candidate(0, 'public_index_slot', a), 3)
        self.assertEqual(audit.lookup_candidate(299, 'public_index_slot', a), 4)
        for value in (2, 300, 0x80000001):
            self.assertIsNone(audit.lookup_candidate(value, 'public_index_slot', a))
        self.assertEqual(audit.lookup_candidate(8, 'direct_id', a), 8)
        self.assertIsNone(audit.lookup_candidate(9, 'direct_id', a))
        with self.assertRaises(audit.AuditError): audit.lookup_candidate(3, 'plus_one', a)

    def test_discovery_survivor_validates_heldout_with_unchanged_encoding(self):
        body = city(); samples = [{'x': 1, 'y': 1, 'content_id': 3}, {'x': 2, 'y': 1, 'content_id': 4}]
        held = [{'x': 3, 'y': 1, 'content_id': 3}]
        for index, label in audit.sample_indices(samples + held, 140): body[1535 + index] = label
        r = audit.compare_samples(body, archive(), samples, held)['results'][0]['relations'][0]
        self.assertEqual((r['discovery_matches'], r['heldout_evaluated'], r['heldout_matches']), (2, 1, 1))

    def test_failed_discovery_does_not_read_heldout_values(self):
        body = city(); train = [{'x': 1, 'y': 1, 'content_id': 3}, {'x': 2, 'y': 1, 'content_id': 4}]
        hold = [{'x': 3, 'y': 1, 'content_id': 3}]
        allowed = {i for i, _ in audit.sample_indices(train, 140)}
        class Values:
            def __getitem__(self, index):
                if index not in allowed: raise AssertionError('heldout leaked')
                return 99
        with patch.object(audit, 'raw_values', return_value=Values()):
            r = audit.compare_samples(body, archive(), train, hold)
        self.assertEqual(r['survivors'], 0)
        self.assertTrue(all(t['heldout_evaluated'] == 0 and t['heldout_matches'] is None for s in r['results'] for t in s['relations']))

    def test_repeated_content_counterexample_and_invalid_partitions(self):
        body = city(); train = [{'x': 1, 'y': 1, 'content_id': 3}, {'x': 2, 'y': 1, 'content_id': 3}]
        hold = [{'x': 3, 'y': 1, 'content_id': 4}]
        for n, (index, _) in enumerate(audit.sample_indices(train, 140)): body[1535 + index] = n + 1
        r = audit.compare_samples(body, archive(), train, hold)['results'][0]
        self.assertEqual((r['same_content_pairs'], r['same_content_raw_different']), (1, 1))
        for bad in ([dict(x=True, y=1, content_id=3)], [dict(x=140, y=1, content_id=3)], train + [train[0]]):
            with self.assertRaises(audit.AuditError): audit.sample_indices(bad, 140)
        with self.assertRaises(audit.AuditError): audit.compare_samples(body, archive(), train, [train[0]])

    def test_two_read_stability_and_late_corruption(self):
        with tempfile.TemporaryDirectory(prefix='raw-map-own-') as temp:
            root = Path(temp); (root / 'Cities').mkdir(); file = root / 'Cities/own.map'
            file.write_bytes(framed(city())); r = audit.audit_root(root)
            self.assertEqual((r['city_span_sets'], r['complete_candidate_spans'], r['unchanged_two_read_inputs']), (1, 11, 1))
            broken = framed(city()); broken[-1] ^= 1; file.write_bytes(broken)
            r = audit.audit_root(root); self.assertFalse(r['accepted_raw_spans']); self.assertEqual(r['complete_candidate_spans'], 0)

    def test_aggregate_distinct_cap_is_explicit_lower_bound(self):
        with tempfile.TemporaryDirectory(prefix='raw-map-own-') as temp:
            root = Path(temp); (root / 'Cities').mkdir()
            for block in range(2):
                body = city()
                body[1535:209471] = struct.pack('<' + str(audit.CELLS) + 'I', *range(block * audit.CELLS, (block + 1) * audit.CELLS))
                (root / 'Cities' / (str(block) + '.map')).write_bytes(framed(body))
            r = audit.audit_root(root)
            span = next(x for x in r['layouts'] if x['name'] == 'prefix_quad_u32')
            self.assertEqual(span['values'], 2 * audit.CELLS)
            self.assertEqual(span['distinct_values_or_lower_bound'], 65536)
            self.assertFalse(span['distinct_exact'])
            self.assertEqual((span['minimum'], span['maximum']), (0, 2 * audit.CELLS - 1))

    def test_relative_root_escape_refused(self):
        with tempfile.TemporaryDirectory(prefix='raw-map-own-') as temp:
            root = Path(temp); own = root / 'own'; own.mkdir(); (root / 'outside').write_bytes(b'authored')
            with self.assertRaises(audit.AuditError): audit.inside_root(own, '../outside')
            with self.assertRaises(audit.AuditError): audit.inside_root(own, str(root / 'outside'))

    def test_explicit_invalid_manifest_is_not_silently_ignored(self):
        with tempfile.TemporaryDirectory(prefix='raw-map-own-') as temp:
            root = Path(temp); (root / 'Cities').mkdir()
            (root / 'Cities/own.map').write_bytes(framed(city()))
            for manifest in ({}, [], {'training': [], 'heldout': []}):
                with self.assertRaises(audit.AuditError): audit.audit_root(root, manifest)

    def test_unset_and_empty_roots_skip_before_manifest(self):
        for value in (None, ''):
            env = os.environ.copy(); env.pop('EMPEROR_GAMEDATA_ROOT', None)
            if value is not None: env['EMPEROR_GAMEDATA_ROOT'] = value
            env['PYTHONDONTWRITEBYTECODE'] = '1'
            r = subprocess.run([sys.executable, str(Path(audit.__file__)), '--sample-manifest', '/missing'], env=env, capture_output=True, text=True)
            self.assertEqual(r.returncode, 0); self.assertTrue(json.loads(r.stdout)['skipped'])


if __name__ == '__main__': unittest.main()
