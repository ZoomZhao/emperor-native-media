"""Authored exact-layout controls; no original bytes or original-data credit."""
import json
import os
from pathlib import Path
import subprocess
import unittest
import numpy as np
import sav_surface_search as probe


class SurfaceSearchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Distinct, nonzero words isolate exact layout rather than ID-range hits.
        cls.grid = (np.arange(228 * 228, dtype='uint32').reshape(228, 228) + 1) | 0x12340000
        cls.crop = cls.grid[106:122, 106:122]

    def test_dense_endian_and_crops(self):
        for w, shift, endian in probe.ENCODINGS:
            with self.subTest(width=w, shift=shift, endian=endian):
                a = probe.projected(self.crop, w, shift, endian)
                result = probe.search_surface(self.grid, 16, b'header!' + a.tobytes() + b'tail')
                self.assertGreater(result['whole_pattern_hits'], 0)
                self.assertFalse(result['candidate_semantics_verified'])

    def test_transpose(self):
        result = probe.search_surface(self.grid, 16, self.crop.T.tobytes())
        self.assertGreater(result['families']['dense']['hits'], 0)

    def test_arbitrary_row_padding(self):
        # Nonzero padding must not be assumed to be zero; prefix is unaligned.
        data = b'odd' + b''.join(row.tobytes() + b'\xa5' * 8 for row in self.grid[58:170, 58:170])
        result = probe.search_surface(self.grid, 112, data)
        self.assertGreater(result['row_geometry_candidates'], 0)
        self.assertGreater(result['row_complete_candidates'], 0)

    def test_sparse(self):
        original = self.grid.copy();original[::2] = 0
        crop = original[106:122, 106:122]
        for kind in ('values', 'index2', 'index4', 'xy1', 'xy2'):
            with self.subTest(kind=kind):
                a = probe.projected(crop, 4, 0, '>')
                data = b'head' + probe.sparse_records(a, crop, kind, '>')
                result = probe.search_surface(original, 16, data)
                self.assertGreater(result['families']['sparse_packed']['hits'], 0)

    def test_tile_layout_and_fragments(self):
        for size in (8, 16):
            data = b'head' + probe.tiles(self.crop, size).tobytes()
            self.assertGreater(probe.search_surface(self.grid, 16, data)['families']['block_major']['hits'], 0)
        full = self.grid[58:170, 58:170]
        result = probe.search_surface(self.grid, 112, full[:8, :8].tobytes())
        self.assertGreater(result['tile_fragment_hits'], 0)
        self.assertEqual(result['row_complete_candidates'], 0)

    def test_independent_block_byte_order(self):
        raw = np.arange(228 * 228, dtype='uint32').reshape(228, 228)
        raw = ((raw + 1) << 16) | ((raw * 17 + 3) & 65535)
        crop = raw[106:122, 106:122]
        def oracle(array, tile, width, shift, endian):
            # Independent scalar byte construction: neither tiles() nor NumPy
            # serialization constructs the expected pattern.
            return b''.join(((int(array[y, x]) >> shift) & ((1 << (8 * width)) - 1)).to_bytes(
                width, 'little' if endian == '<' else 'big')
                for by in range(0, len(array), tile) for bx in range(0, len(array), tile)
                for y in range(by, min(by + tile, len(array)))
                for x in range(bx, min(bx + tile, len(array))))
        for array in (raw, crop, raw.T, crop.T):
            for width, shift, endian in probe.ENCODINGS:
                for tile in (8, 16):
                    with self.subTest(side=len(array), width=width, shift=shift, endian=endian, tile=tile):
                        got = probe.tiles(probe.projected(array, width, shift, endian), tile)
                        self.assertEqual(got.tobytes(), oracle(array, tile, width, shift, endian))
        for width, shift in ((4, 0), (2, 0), (2, 16)):
            for tile in (8, 16):
                payload = b'header!' + oracle(crop, tile, width, shift, '>') + b'tail'
                result = probe.search_surface(raw, 16, payload)
                self.assertGreater(result['families']['block_major']['hits'], 0)

    def test_noncontinuous_rows_not_whole_field(self):
        crop = self.grid[58:170, 58:170]
        # Only two complete nonadjacent rows: do not publish a complete grid.
        data = crop[28].tobytes() + crop[84].tobytes()
        result = probe.search_surface(self.grid, 112, data)
        self.assertGreater(result['row_fragment_hits'], 0)
        self.assertEqual(result['row_complete_candidates'], 0)

    def test_negative_and_corrupted_tail(self):
        result = probe.search_surface(self.grid, 16, b'no original sequence here')
        self.assertTrue(result['no_hits_in_tested_patterns'])
        data = bytearray(self.crop.tobytes());data[-1] ^= 1
        result = probe.search_surface(self.grid, 16, bytes(data))
        self.assertEqual(result['families']['dense']['hits'], 0)

    def test_hits_and_saturation(self):
        self.assertEqual(probe.locations(b'ababa', b'aba'), ([0, 2], False))
        self.assertEqual(probe.locations(b'abc', b''), ([], False))
        hits, capped = probe.locations(b'x' * 65, b'x')
        self.assertEqual(len(hits), 64);self.assertTrue(capped)
        self.assertFalse(probe.locations(b'x' * 64, b'x')[1])

    def test_long_anchor_is_exact(self):
        patterns = [b'\0' * 5000 + b'rare-tail', b'A' * 9000,
                    b'head' + b'\0' * 5000, bytes(range(256)) * 25]
        for pattern in patterns:
            data = b'prefix' + pattern + pattern[:-1] + b'X' + pattern + b'tail'
            reference = [];start = 0
            while True:
                at = data.find(pattern, start)
                if at < 0 or len(reference) == 65:break
                reference.append(at);start = at + 1
            got, capped = probe.locations(data, pattern)
            self.assertEqual(got, reference[:64])
            self.assertEqual(capped, len(reference) > 64)

    def test_low_information_skips_are_not_tested(self):
        result = probe.search_surface(np.zeros((228, 228), dtype='uint32'), 16, b'\0' * 2000)
        self.assertEqual(result['whole_patterns'], 0)
        self.assertGreater(result['whole_zero_skips'], 0)
        self.assertEqual(result['row_fragments'], 0)
        self.assertGreater(result['row_low_information_skips'], 0)
        self.assertEqual(result['padded_layout_variants_excluded'], 0)
        self.assertGreater(result['row_layouts_without_eligible_fragments'], 0)

    def test_bounds_and_pair_budget(self):
        for side in (0, 15, 229):
            with self.assertRaises(ValueError):probe.search_surface(self.grid, side, b'')
        with self.assertRaises(ValueError):probe.search_surface(self.grid, 16, bytes(8 * 1024 * 1024 + 1))
        with self.assertRaises(ValueError):probe.run('/missing', {'pairs': [{}] * 8})

    def test_skip_before_manifest_and_numpy(self):
        for value in (None, ''):
            env = dict(os.environ);env.pop('EMPEROR_GAMEDATA_ROOT', None)
            if value is not None:env['EMPEROR_GAMEDATA_ROOT'] = value
            p = subprocess.run(['/usr/bin/python3', '-I', '-S', '-B', str(Path(probe.__file__).resolve())],
                               env=env, capture_output=True, text=True)
            self.assertEqual(p.returncode, 0, p.stderr)
            self.assertTrue(json.loads(p.stdout)['skipped'])


if __name__ == '__main__':
    unittest.main()
