"""Authored correspondence fixtures; no original data or renderer rules."""
from pathlib import Path
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
import map_save_compare as audit
from save_state_audit import parse_save
from save_state_audit_tests import fixture, framed


def paired():
    data = bytearray(fixture())
    for _, (offset, width) in audit.SAVED_ONLY.items():
        if width == 1:
            data[256 + offset:256 + offset + 51984] = bytes([255]) * 51984
    for n, (name, (_, relative, width)) in enumerate(audit.COMMON.items()):
        if name == 'terrain':
            continue
        a = np.arange(51984, dtype='<u4') + 0x80000000 if width == 4 else np.arange(51984, dtype='u1')
        data[256 + relative:256 + relative + width * 51984] = a.tobytes()
    saved_bytes = framed(data)
    saved = parse_save(saved_bytes)
    mp = bytearray(989239)
    mp[:8] = bytes.fromhex('0500feca00000200')
    struct.pack_into('<I', mp, 84, saved.side)
    for name, (offset, relative, width) in audit.COMMON.items():
        mp[offset:offset + width * 51984] = data[256 + relative:256 + relative + width * 51984]
    return bytes(mp), saved_bytes, saved


class ComparisonTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mp, cls.sav_bytes, cls.saved = paired()

    def test_exact_unsigned_common_fields_and_unknown_diagnostics(self):
        row, arrays = audit.compare_pair(self.mp, self.saved, {'sample': [[0, 0], [3, 3]]})
        self.assertTrue(row['whole_terrain_identity_verified'])
        self.assertTrue(all(v['whole'] == {'equal_values': 51984, 'compared_values': 51984} for v in row['fields'].values()))
        self.assertEqual(int(arrays['objects'][227, 227]), 0x80000000 + 51983)
        self.assertEqual(row['saved_only']['following_u32_unknown']['zero_values'], 51983)
        self.assertEqual(row['saved_only']['stone']['distinct_values'], 1)

    def test_objects_are_not_the_adjacent_unknown_grid(self):
        mp = bytearray(self.mp)
        mp[469391:677327] = self.saved.grid('following_u32').tobytes()
        row, _ = audit.compare_pair(mp, self.saved, {'sample': [[0, 0]]})
        self.assertLess(row['fields']['objects']['whole']['equal_values'], 51984)
        self.assertEqual(row['groups']['sample']['centres']['any_common_field_changed'], 1)

    def test_single_selected_change_and_unselected_endpoint_are_counted(self):
        mp = bytearray(self.mp)
        selected = 729311 + (112 * 228 + 112)
        mp[selected] ^= 1
        mp[729311 + 51983] ^= 1
        row, _ = audit.compare_pair(mp, self.saved, {'cell': [[0, 0]], 'other': [[3, 3]]})
        self.assertEqual(row['fields']['random']['whole']['equal_values'], 51982)
        self.assertEqual(row['fields']['random']['on_map']['equal_values'], 15)
        self.assertEqual(row['groups']['cell']['centres']['any_common_field_changed'], 1)
        self.assertEqual(row['groups']['other']['centres']['any_common_field_changed'], 0)

    def test_source_gate_checks_full_terrain_not_only_samples_or_crop(self):
        mp = bytearray(self.mp)
        struct.pack_into('<I', mp, 261455 + 4 * 51983, 0xdeadbeef)
        with self.assertRaisesRegex(ValueError, 'whole terrain'):
            audit.compare_pair(mp, self.saved, {'cell': [[0, 0]]})
        mp = bytearray(self.mp)
        struct.pack_into('<I', mp, 84, 6)
        with self.assertRaisesRegex(ValueError, 'side'):
            audit.compare_pair(mp, self.saved, {})
        with self.assertRaisesRegex(ValueError, 'role'):
            audit.compare_pair(b'bad', self.saved, {})

    def test_every_truncated_common_endpoint_refuses(self):
        for offset, _, width in audit.COMMON.values():
            with self.assertRaisesRegex(ValueError, 'extent'):
                audit.compare_pair(self.mp[:offset + width * 51984 - 1], self.saved, {})

    def test_canonical_bounds_duplicate_budget_and_cardinal_union(self):
        for points in [[[-1, 0]], [[4, 0]], [[True, 0]], [[0.0, 0]], [[0, 0], [0, 0]], [[0, 0]] * 129]:
            with self.assertRaises(ValueError):
                audit.sample_indices({'sample': points}, 4)
        centres, neighbours = audit.sample_indices({'sample': [[0, 0], [1, 0]]}, 4)['sample']
        self.assertEqual(len(centres), 2)
        self.assertEqual(len(neighbours), 5)
        self.assertIn((112, 112), centres)
        self.assertNotIn((111, 112), neighbours)

    def write_root(self, root):
        (root / 'Cities').mkdir()
        (root / 'Save').mkdir()
        (root / 'Cities/Authored.map').write_bytes(framed(self.mp))
        (root / 'Save/Authored.sav').write_bytes(self.sav_bytes)
        (root / 'Save/Repeat.sav').write_bytes(self.sav_bytes)
        pair = {'map': 'Cities/Authored.map', 'save': 'Save/Authored.sav', 'groups': {'sample': [[0, 0]]}}
        return {'pairs': [pair, dict(pair, save='Save/Repeat.sav')], 'repeats': [[0, 1]]}

    def test_full_audit_explicit_repeat_and_aggregate_only_output(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            result = audit.audit(root, self.write_root(root))
        self.assertEqual((result['pairs_verified'], result['common_field_arrays_compared']), (2, 8))
        self.assertEqual(result['input_files_unchanged'], [3, 3])
        self.assertFalse(result['graphics_field_counterpart_identified'])
        self.assertEqual(len(result['snapshot_comparisons'][0]['fields']), 6)
        self.assertTrue(all(x['equal_values'] == 51984 for x in result['snapshot_comparisons'][0]['fields'].values()))
        output = json.dumps(result)
        self.assertNotIn('2147483648', output)
        self.assertNotIn('hash', output)
        self.assertNotIn('payload', output)
        self.assertNotIn('pixels', output)

    def test_unknown_snapshot_change_is_reported_without_inventing_MAP_pair(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            manifest = self.write_root(root)
            payload = bytearray(self.saved.payload)
            struct.pack_into('<I', payload, 256 + 207936 + 4 * (112 * 228 + 112), 7)
            (root / 'Save/Repeat.sav').write_bytes(framed(payload))
            result = audit.audit(root, manifest)
        fields = result['snapshot_comparisons'][0]['fields']
        self.assertEqual(fields['following_u32_unknown']['equal_values'], 51983)
        self.assertEqual(result['snapshot_comparisons'][0]['groups']['sample']['centres']['following_u32_unknown']['equal_values'], 0)
        self.assertTrue(all(fields[f]['equal_values'] == 51984 for f in audit.COMMON))
        self.assertFalse(result['graphics_field_counterpart_identified'])

    def test_final_recheck_rejects_change_across_audit(self):
        manifest = {'pairs': [{'map': 'Cities/A.map', 'save': 'Save/A.sav'}]}
        changed = framed(self.mp) + b'x'
        with patch.object(audit, 'stable_bytes', side_effect=[framed(self.mp), self.sav_bytes, changed]):
            with self.assertRaisesRegex(ValueError, 'across complete audit'):
                audit.audit(Path('/unused'), manifest)

    def test_repeated_source_change_is_not_hidden_by_last_read(self):
        pair = {'map': 'Cities/A.map', 'save': 'Save/A.sav'}
        manifest = {'pairs': [pair, pair]}
        with patch.object(audit, 'stable_bytes', side_effect=[framed(self.mp), self.sav_bytes, framed(self.mp) + b'x', self.sav_bytes]):
            with self.assertRaisesRegex(ValueError, 'between pairs'):
                audit.audit(Path('/unused'), manifest)

    def test_bad_manifest_repeats_and_arbitrary_descriptors_refuse(self):
        for m in [[], {}, {'pairs': []}, {'pairs': [{'map': 'a', 'save': 'b', 'offset': 1535}]}]:
            with self.assertRaises(ValueError):
                audit.audit(Path('/unused'), m)
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            m = self.write_root(root)
            for indices in [[0, 0], [True, 1], [0, 2], [0]]:
                m['repeats'] = [indices]
                with self.assertRaises(ValueError):
                    audit.audit(root, m)
            m['repeats'] = [[0, 1]]
            m['pairs'][1]['groups'] = {'sample': [[1, 0]]}
            with self.assertRaisesRegex(ValueError, 'sample groups differ'):
                audit.audit(root, m)

    def test_root_escape_and_changed_two_read_input_refuse(self):
        with tempfile.TemporaryDirectory() as folder, tempfile.TemporaryDirectory() as other:
            root = Path(folder)
            f = root / 'owned'
            f.write_bytes(b'a')
            outside = Path(other) / 'outside'
            outside.write_bytes(b'b')
            (root / 'link').symlink_to(outside)
            for name in ['../outside', str(outside), 'link']:
                with self.assertRaises(ValueError):
                    audit.stable_bytes(root, name)
            with patch.object(audit, 'read_bounded', side_effect=[b'a', b'b']):
                with self.assertRaisesRegex(ValueError, 'two reads'):
                    audit.stable_bytes(root, 'owned')

    def test_unset_empty_environment_skip_before_manifest_or_numpy(self):
        for value in [None, '']:
            env = os.environ.copy()
            env.pop('EMPEROR_GAMEDATA_ROOT', None)
            if value is not None:
                env['EMPEROR_GAMEDATA_ROOT'] = value
            run = subprocess.run([sys.executable, '-S', str(Path(audit.__file__)), '--manifest', '/missing'], env=env, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0)
            self.assertTrue(json.loads(run.stdout)['skipped'])


if __name__ == '__main__':
    unittest.main()
