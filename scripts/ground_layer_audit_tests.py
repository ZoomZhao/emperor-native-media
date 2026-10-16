"""Authored checks for failed-evidence paths; synthetic pixels are not data progress."""
from pathlib import Path
import json
import os
import struct
import subprocess
import sys
import unittest
from unittest.mock import patch

import numpy as np

from ground_layer_audit import decode_components, identify, place_mask, score, exposed_geometry, centre_removal_necessary, rear_source_identification, paint_complete, restore_removed_top, score_known
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

    def test_rear_no_exact_source_does_not_promote_minimum_or_use_post(self):
        full = np.zeros((40, 78, 4), np.uint8)
        full[..., 3][diamond_mask()] = 255
        full[..., 0][diamond_mask()] = 8
        pre = np.zeros((1592, 2048, 3), np.uint8)
        sources = {241: full, 325: full.copy()}
        result = rear_source_identification(pre, pre.copy(), pre.copy(), [241, 325], sources)
        self.assertEqual(result['centre_branches_retained'], [241, 325])
        self.assertEqual([r['state'] for r in result['rear_sources']], ['no_exact_source'] * 3)
        for row in result['rear_sources']:
            self.assertGreater(row['identification_pixels'], 0)
            self.assertEqual(row['exact_IDs'], [])
            self.assertEqual(row['minimum_IDs_not_promoted'], [241, 325])
            self.assertEqual(row['centre_branch_opacity_overlap'], 0)
        self.assertFalse(result['conditional_prediction_gate_passed'])
        self.assertFalse(result['conditional_predictions_constructed'])
        changed = rear_source_identification(pre, np.full_like(pre, 255), pre.copy(), [241, 325], sources)
        self.assertEqual([r['all_candidate_errors'] for r in result['rear_sources']],
                         [r['all_candidate_errors'] for r in changed['rear_sources']])

    def test_rear_zero_information_does_not_pass_empty_comparison(self):
        # Force complete occlusion. Real78-wide cells can retain narrow grid gaps.
        full = np.full((40, 78, 4), 255, np.uint8)
        pre = np.zeros((1592, 2048, 3), np.uint8)
        def covered(shape, mask, centre):
            return np.ones(shape, bool), (0, 0, shape[1], shape[0])
        with patch('ground_layer_audit.place_mask', side_effect=covered):
            result = rear_source_identification(pre, pre.copy(), pre.copy(), [241, 325], {241: full, 325: full.copy()})
        for row in result['rear_sources']:
            self.assertEqual(row['state'], 'zero_information')
            self.assertEqual(row['identification_pixels'], 0)
            self.assertEqual(row['all_candidate_errors'], [])
            self.assertEqual(row['exact_IDs'], [])
            self.assertIsNone(row['minimum_errors_diagnostic_only'])
        self.assertFalse(result['conditional_predictions_constructed'])

    def test_unknown_matches_never_reduce_the_complete_denominator(self):
        predicted = np.zeros((2, 2, 3), np.uint8)
        actual = predicted.copy()
        actual[0, 1] = 8
        known = np.array([[True, True], [False, False]])
        result = score_known(predicted, actual, known, np.ones((2, 2), bool))
        self.assertEqual((result['denominator'], result['predictable'], result['unknown']), (4, 2, 2))
        self.assertEqual((result['RGB5_equal'], result['RGB5_errors_known'], result['RGB5_not_explained']), (1, 1, 3))
        self.assertEqual(result['RGB8_not_explained'], 3)

    def test_old_top_holes_use_only_the_underlay_and_preserve_other_pre_pixels(self):
        pre = np.full((2, 2, 3), 24, np.uint8)
        under = np.full_like(pre, 80)
        old = np.array([[True, True], [False, False]])
        under_known = np.array([[True, False], [False, False]])
        original = pre.copy()
        rgb, known = restore_removed_top(pre, old, under, under_known)
        self.assertTrue(np.array_equal(rgb[~old], pre[~old]))
        self.assertTrue(np.array_equal(rgb[old], under[old]))
        self.assertEqual(known.tolist(), [[True, False], [True, True]])
        self.assertTrue(np.array_equal(pre, original))
        # A perfect raw value at an unknown hole still cannot become a pass.
        result = score_known(rgb, rgb.copy(), known, old)
        self.assertEqual((result['RGB5_equal'], result['unknown'], result['denominator']), (1, 1, 2))

    def test_complete_source_alpha_is_painted_without_a_visibility_crop(self):
        source = np.zeros((46, 78, 4), np.uint8)
        source[0, 0] = [8, 16, 24, 255]
        source[-1, -1] = [24, 16, 8, 255]
        original = source.copy()
        rgb = np.zeros((200, 300, 3), np.uint8)
        known = np.zeros(rgb.shape[:2], bool)
        support, _ = paint_complete(rgb, known, source, (150, 100))
        self.assertEqual(int(known.sum()), 8)
        self.assertTrue(np.array_equal(support, known))
        self.assertTrue(np.all(np.any(rgb[support] != 0, axis=1)))
        self.assertTrue(np.array_equal(source, original))
        source[0, 0, 3] = 128
        with self.assertRaisesRegex(ValueError, 'natural binary'):
            paint_complete(rgb, known, source, (150, 100))

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
