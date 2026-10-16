"""Authored checks for failed-evidence paths; synthetic pixels are not data progress."""
from pathlib import Path
import json
import os
import struct
import subprocess
import sys
import unittest

import numpy as np

from ground_layer_audit import decode_components, identify, place_mask, score, exposed_geometry, centre_removal_necessary
from base_content_audit import diamond_mask


class GroundLayerTests(unittest.TestCase):
    def record(self, stream=b''):
        record = bytearray(64)
        struct.pack_into('<III', record, 0, 0, 3200 + len(stream), 3200)
        struct.pack_into('<hh', record, 20, 78, 46)
        record[50] = 30
        return record, bytes(3200) + stream

    def test_top_is_complete_and_separate_from_base(self):
        record, pixels = self.record(bytes((1, 31, 0)))
        base, top, full = decode_components(record, pixels)
        self.assertEqual(int((base[..., 3] == 255).sum()), 1600)
        self.assertEqual(int((top[..., 3] == 255).sum()), 1)
        self.assertEqual(full[0, 0].tolist(), [255, 0, 0, 255])
        self.assertTrue(np.array_equal(full[-40:][diamond_mask()], base[diamond_mask()]))

    def test_bad_RLE_never_becomes_partial_success(self):
        for stream in (bytes((255,)), bytes((1, 31)), bytes((255, 255)) * 15):
            with self.subTest(stream=stream), self.assertRaises(ValueError):
                decode_components(*self.record(stream))

    def test_top_cannot_repaint_public_base(self):
        # First public pixel: six top rows, then source diamond edge38.
        skip = 6 * 78 + 38
        stream = bytes((255, 255, 255, skip - 255, 1, 31, 0))
        with self.assertRaisesRegex(ValueError, 'base/top overlap'):
            decode_components(*self.record(stream))

    def test_complete_exposed_support_rejects_base_only_hit(self):
        full = np.zeros((46, 78, 4), np.uint8)
        full[-40:, :, 3][diamond_mask()] = 255
        full[0, 0] = [255, 0, 0, 255]
        frame = np.zeros((200, 300, 3), np.uint8)
        paint, _ = place_mask(frame.shape[:2], full[..., 3] == 255, (150, 100))
        I, _ = place_mask(frame.shape[:2], diamond_mask(), (150, 100))
        row = identify(frame, {1: full}, (150, 100), I, np.ones_like(I))[0]
        self.assertFalse(row['exact'])
        self.assertEqual(row['RGB5_errors'], 4)
        self.assertEqual(row['pixels'], int(paint.sum()))
        # A one-channel threshold crossing remains an error, never a near-hit.
        full[0, 0, 0] = 8
        row = identify(frame, {1: full}, (150, 100), I, np.ones_like(I))[0]
        self.assertFalse(row['exact'])

    def test_identification_is_colour_independent_and_outside_validation(self):
        union = np.zeros((59, 78), bool)
        union[-40:] = diamond_mask()
        I, exposed = exposed_geometry((1592, 2048), union, diamond_mask(), (718, 1256))
        self.assertFalse(I[1120:1280, 700:900].any())
        self.assertTrue(I.any())
        self.assertFalse((I & ~exposed).any())

    def test_full_conflict_including_small_error_is_retained(self):
        predicted = np.zeros((2, 2, 3), np.uint8)
        actual = predicted.copy()
        actual[0, 0] = 8
        predicted[0, 0] = 7
        actual[1, 1] = 200
        result = score(predicted, actual, np.ones((2, 2), bool))
        self.assertEqual((result['pixels'], result['RGB5_errors'], result['small_raw_errors']), (4, 2, 1))
        self.assertEqual(result['max_raw_error'], 200)

    def test_no_wrap_or_clipping(self):
        for centre in ((0, 0), (90, 90)):
            with self.subTest(centre=centre), self.assertRaisesRegex(ValueError, 'no clipping'):
                place_mask((100, 100), diamond_mask(), centre)

    def test_geometry_cannot_choose_one_of_two_base_aliases_or_erase_unknown(self):
        full = np.zeros((54, 78, 4), np.uint8)
        full[-40:, :, 3][diamond_mask()] = 255
        full[0, :2] = [255, 0, 0, 255]
        bare = full.copy()
        bare[0] = 0
        top = np.zeros_like(full)
        top[0, :2] = full[0, :2]
        pre = np.zeros((1592, 2048, 3), np.uint8)
        post = pre.copy()
        nonpaint = np.zeros(pre.shape[:2], bool)
        outside = np.zeros_like(nonpaint)
        nonpaint[1148:1150, 720:722] = True
        outside[1148:1150, 722:724] = True
        post[nonpaint | outside] = 200
        result = centre_removal_necessary(pre, post, pre.copy(), [1, 2],
                                          {1: full, 2: bare}, {1: top, 2: np.zeros_like(top)},
                                          nonpaint, outside)
        self.assertEqual(result['exact_base_IDs'], [1, 2])
        self.assertEqual([b['geometry_necessary_pass'] for b in result['base_compatible_branches']], [True, False])
        self.assertFalse(result['unique_identity_proved'])
        self.assertFalse(result['removal_prediction_constructed'])
        self.assertFalse(result['post_state_used_as_erase_backdrop'])
        self.assertFalse(result['base_compatible_branches'][1]['has_top_information'])
        # Changing post cannot change a pre-only source identification.
        post[:] = 255
        changed = centre_removal_necessary(pre, post, pre.copy(), [1, 2],
                                           {1: full, 2: bare}, {1: top, 2: np.zeros_like(top)},
                                           nonpaint, outside)
        self.assertEqual(result['all_candidate_errors'], changed['all_candidate_errors'])

    def test_unset_empty_root_skip_without_dependencies_or_manifest(self):
        script = Path(__file__).with_name('ground_layer_audit.py')
        for value in (None, ''):
            env = os.environ.copy()
            env.pop('EMPEROR_GAMEDATA_ROOT', None)
            if value is not None:
                env['EMPEROR_GAMEDATA_ROOT'] = value
            result = subprocess.run([sys.executable, '-S', '-B', str(script), '--manifest', '/absent'],
                                    env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(json.loads(result.stdout)['skipped'])


if __name__ == '__main__':
    unittest.main()
