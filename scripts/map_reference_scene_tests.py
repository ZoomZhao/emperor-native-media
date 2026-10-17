"""Authored CLI contract fixtures; no original data or original progress."""
from pathlib import Path
import argparse
import copy
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

BINARY = None


def write_fixtures(root):
    def put(data, offset, value, width=4):
        data[offset:offset + width] = value.to_bytes(width, 'little')

    base = 3003
    data = bytearray(base + 2226670)
    data[0] = 0x13
    put(data, 127, 112)
    data[143:159] = b'Brief descriptio'
    for i in range(51984):
        put(data, base + 4 * i, 0x80000)
    for i in range(3):
        put(data, base + 4 * i, 0x80005)
    for y in range(58, 170):
        for x in range(58, 170):
            put(data, base + 4 * (y * 228 + x), 128)
    put(data, base + 4 * (59 * 228 + 59), 0x80000)  # authored internal hole
    framed = bytearray(struct.pack('<I', 0xfedcbaaa))
    for offset in range(0, len(data), 32768):
        part = data[offset:offset + 32768]
        packed = zlib.compress(part)
        framed += struct.pack('<III', 0xabcdefff, len(packed), len(part)) + packed
    (root / 'own.sav').write_bytes(framed)
    table = 40680
    index = bytearray(table + 2 * 64)
    for offset, value in ((0, len(index)), (4, 213), (12, 20), (16, 1), (20, 1)):
        put(index, offset, value)
    index[680] = ord('o')
    record = table + 64
    for offset, value in ((4, 3203), (8, 3200)):
        put(index, record + offset, value)
    for offset, value in ((20, 78), (22, 46), (50, 30)):
        put(index, record + offset, value, 2)
    index[record + 55] = 1
    (root / 'own.sg3').write_bytes(index)
    (root / 'own.555').write_bytes(struct.pack('<H', 0x7c00) * 1600 + bytes((1, 0xe0, 3)))


class SceneCLI(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='emperor-scene-own-')
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name)
        self.root = self.work / 'data'
        self.root.mkdir()
        write_fixtures(self.root)
        self.doc = {'schema': 'sg3-diagnostic-sav-scene-v1', 'diagnostic': True,
                    'archive': 'own.sg3', 'save': 'own.sav', 'width': 300,
                    'height': 200, 'scale': 2,
                    'registration': {'matrix': [[40, -40, 190], [20, 20, 0]],
                                     'canvas_origin': [0, 0]},
                    'layers': [{'id': 1, 'component': 'full', 'cell': [2, 3]}]}
        self.serial = 0

    def run_scene(self, doc, succeeds=True, output=None, producer=None):
        self.serial += 1
        manifest = self.work / ('manifest%d.json' % self.serial)
        manifest.write_text(json.dumps(doc))
        output = output or self.work / ('out%d.png' % self.serial)
        producer = producer or self.work / ('out%d.u16le' % self.serial)
        existing = {path: path.read_bytes() for path in (output, producer) if path.is_file()}
        result = subprocess.run([str(BINARY), '--data-root', str(self.root),
                                 '--scene-manifest', str(manifest), '--no-window',
                                 '--output', str(output), '--producer-output', str(producer)],
                                capture_output=True, text=True)
        if succeeds:
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(output.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'))
            self.assertEqual(len(producer.read_bytes()), 300 * 200 * 2)
            return json.loads(result.stdout), output.read_bytes(), producer.read_bytes()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        for path in (output, producer):
            if path in existing:
                self.assertEqual(path.read_bytes(), existing[path], result.stderr)
            else:
                self.assertFalse(path.exists(), result.stderr)
        return result.stderr

    def test_registered_and_pixel_scenes_are_identical(self):
        stats, png, producer = self.run_scene(self.doc)
        self.assertTrue(stats['sav_coordinates'])
        self.assertEqual(stats['validated_sav_cells'], 1)
        self.assertEqual(stats['selector_status'], 'unsupported')
        self.assertFalse(stats['source_rule_confirmed'])
        self.assertEqual(stats['screenshot_inputs'], 0)
        old = copy.deepcopy(self.doc)
        old['schema'] = 'sg3-diagnostic-scene-v1'
        del old['save'], old['registration']
        old['layers'][0] = {'id': 1, 'component': 'full', 'centre': [150, 100]}
        oldstats, oldpng, oldproducer = self.run_scene(old)
        self.assertEqual((png, producer), (oldpng, oldproducer))
        self.assertFalse(oldstats['sav_coordinates'])

    def test_cell_coordinates_and_internal_hole_refuse(self):
        for cell in ([-1, 0], [112, 0], [0, 112], [1, 1], [True, 3], [2.5, 3], [2]):
            with self.subTest(cell=cell):
                doc = copy.deepcopy(self.doc)
                doc['layers'][0]['cell'] = cell
                self.run_scene(doc, False)

    def test_registration_shape_type_singularity_and_budgets_refuse(self):
        for matrix in ([[1, 2]], [[1, 2], [3, 4]], [[True, -40, 190], [20, 20, 0]],
                       [[1, 1, 0], [1, 1, 0]], [[4097, 0, 0], [0, 1, 0]],
                       [[40, -40, 1000001], [20, 20, 0]]):
            with self.subTest(matrix=matrix):
                doc = copy.deepcopy(self.doc)
                doc['registration']['matrix'] = matrix
                self.run_scene(doc, False)
        for origin in ([False, 0], [0], [1000001, 0]):
            doc = copy.deepcopy(self.doc)
            doc['registration']['canvas_origin'] = origin
            self.run_scene(doc, False)

    def test_exact_keys_no_pixel_centre_or_implicit_source(self):
        for kind in ('centre', 'missing_id', 'background', 'registration_extra'):
            with self.subTest(kind=kind):
                doc = copy.deepcopy(self.doc)
                if kind == 'centre':
                    doc['layers'][0]['centre'] = [150, 100]
                elif kind == 'missing_id':
                    del doc['layers'][0]['id']
                elif kind == 'background':
                    doc['background'] = 'capture.png'
                else:
                    doc['registration']['infer'] = True
                self.run_scene(doc, False)

    def test_missing_escape_corrupt_or_symlink_save_refuses(self):
        for name in ('absent.sav', '../escape.sav', str(self.root / 'own.sav'), 'broken.sav', 'link.sav', 'own.sav\0truncated'):
            (self.root / 'broken.sav').write_bytes(b'broken')
            if not (self.root / 'link.sav').is_symlink():
                (self.work / 'escape.sav').write_bytes((self.root / 'own.sav').read_bytes())
                (self.root / 'link.sav').symlink_to(self.work / 'escape.sav')
            with self.subTest(save=name):
                doc = copy.deepcopy(self.doc)
                doc['save'] = name
                self.run_scene(doc, False)

    def test_canonical_output_alias_refused_before_creation(self):
        output = self.work / 'alias.png'
        self.run_scene(self.doc, False, output, self.work / '.' / 'alias.png')
        (self.work / 'data-alias').symlink_to(self.root, target_is_directory=True)
        self.run_scene(self.doc, False, self.work / 'data-alias' / 'no.png')

    def schema_document(self, saved):
        doc = copy.deepcopy(self.doc)
        if not saved:
            doc['schema'] = 'sg3-diagnostic-scene-v1'
            del doc['save'], doc['registration']
            doc['layers'][0] = {'id': 1, 'component': 'full', 'centre': [150, 100]}
        return doc

    def test_fresh_case_and_unicode_outputs_follow_filesystem_identity(self):
        pairs = [('Scene.PNG', 'scene.png'), ('Caf\u00e9.png', 'Cafe\u0301.png')]
        original = {name: (self.root / name).read_bytes() for name in ('own.sav', 'own.sg3', 'own.555')}
        for saved in (False, True):
            for first, second in pairs:
                prefix = str(saved) + '-'
                output, producer = self.work / (prefix + first), self.work / (prefix + second)
                output.write_bytes(b'filesystem-probe')
                equivalent = producer.exists() and output.samefile(producer)
                output.unlink()
                with self.subTest(saved=saved, first=first, equivalent=equivalent):
                    self.run_scene(self.schema_document(saved), not equivalent, output, producer)
                    self.assertEqual({name: (self.root / name).read_bytes() for name in original}, original)

    def test_distinct_case_unicode_targets_keep_png_and_producer(self):
        for saved in (False, True):
            for first, second in [('Scene.PNG', 'scene.u16le'), ('Caf\u00e9.png', 'Cafe\u0301.u16le')]:
                prefix = str(saved) + '-'
                self.run_scene(self.schema_document(saved), True,
                               self.work / (prefix + first), self.work / (prefix + second))

    def test_existing_case_unicode_alias_payloads_are_preserved(self):
        for saved in (False, True):
            for index, (first, second) in enumerate([('Scene.PNG', 'scene.png'), ('Caf\u00e9.png', 'Cafe\u0301.png')]):
                prefix = str(saved) + '-existing-'
                old = self.work / (prefix + first)
                alias = self.work / (prefix + second)
                sentinel = b'preserved-existing-input'
                old.write_bytes(sentinel)
                equivalent = alias.exists() and old.samefile(alias)
                output = self.work / ('fresh-%s-%d.png' % (saved, index))
                self.run_scene(self.schema_document(saved), not equivalent, output, alias)
                self.assertEqual(old.read_bytes(), sentinel)

    def test_case_unicode_data_root_aliases_refuse_writes(self):
        for saved in (False, True):
            for first, second in [('CaseData', 'casedata'), ('Caf\u00e9Data', 'Cafe\u0301Data')]:
                prefix = str(saved) + '-'
                moved = self.work / (prefix + first)
                self.root.rename(moved)
                self.root = moved
                alias = self.work / (prefix + second)
                if not alias.exists():
                    alias.mkdir()  # distinct filesystem spelling is outside root
                equivalent = alias.samefile(self.root)
                output = alias / 'no-write.png'
                producer = self.work / ('root-alias-%d.u16le' % self.serial)
                if equivalent:
                    self.run_scene(self.schema_document(saved), False, output, producer)
                    self.assertFalse(output.exists())
                else:
                    self.run_scene(self.schema_document(saved), True, output, producer)

    def test_crop_origin_uses_declared_active_coordinates(self):
        doc = copy.deepcopy(self.doc)
        doc['registration']['canvas_origin'] = [100, 40]
        stats, png, producer = self.run_scene(doc)
        old = copy.deepcopy(self.doc)
        old['schema'] = 'sg3-diagnostic-scene-v1'
        del old['save'], old['registration']
        old['layers'][0] = {'id': 1, 'component': 'full', 'centre': [50, 60]}
        oldstats, oldpng, oldproducer = self.run_scene(old)
        self.assertEqual((png, producer), (oldpng, oldproducer))
        self.assertEqual(stats['covered'], oldstats['covered'])

    def test_original_metrics_skip_unset_and_empty_before_build_or_manifest(self):
        runner = Path(__file__).with_name('map_reference.py')
        for value in (None, ''):
            env = os.environ.copy()
            if value is None:
                env.pop('EMPEROR_GAMEDATA_ROOT', None)
            else:
                env['EMPEROR_GAMEDATA_ROOT'] = value
            result = subprocess.run([sys.executable, '-B', str(runner), 'scene-metrics',
                                     '--scene-manifest', '/absent/no.json',
                                     '--build-dir', str(self.work / 'never-built')],
                                    env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(json.loads(result.stdout)['skipped'])
            self.assertFalse((self.work / 'never-built').exists())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    options = parser.parse_args()
    BINARY = options.binary.resolve(strict=True)
    unittest.main(argv=[sys.argv[0]])
