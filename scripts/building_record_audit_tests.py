"""Authored building-profile fixtures; no original bytes or original-data credit."""
import json
import os
from pathlib import Path
import struct
import subprocess
import unittest
import building_record_audit as audit


def record(kind=0, name='', version=4, x=0, y=0, size=0, alive=0):
    header = struct.pack('<HHH', 0xffff, 0, len(name)) + name.encode() if name else struct.pack('<H', 0x8001)
    body = bytearray({3: 179, 4: 181, 5: 182}[version])
    struct.pack_into('<H', body, 0, version);body[2] = alive;body[5] = size
    struct.pack_into('<HH', body, 8, x, y);struct.pack_into('<H', body, 16, kind)
    body[112 + (version == 5)] = 0x5a
    return header + body + bytes({0: 0, 72: 130, 162: 130}.get(kind, 0))


def table(extra=(), version=4):
    records = [record(name='Building', version=version), *extra]
    records += [record(version=version)] * (4000 - len(records))
    return struct.pack('<HI', 1, 4000) + b''.join(records)


class BuildingAuditTests(unittest.TestCase):
    def test_all_published_fixed_spans(self):
        spans = [(0, 0), (2, 65), (3, 65), (4, 65), (5, 65), (6, 65), (7, 65), (8, 65), (9, 65), (10, 65), (11, 65), (12, 65), (13, 65), (14, 65), (15, 65), (16, 65), (17, 65), (94, 2), (72, 130), (111, 130), (113, 130), (124, 130), (126, 130), (127, 130), (138, 130), (151, 130), (177, 130), (178, 130), (179, 130), (180, 130), (181, 130), (183, 130), (185, 130), (186, 130), (187, 130), (188, 130), (189, 130), (161, 130), (162, 130), (163, 130), (164, 130), (165, 130), (167, 130), (173, 130), (174, 130), (175, 130), (207, 130), (208, 130), (89, 132), (90, 132), (91, 132), (231, 132), (104, 132), (105, 132), (106, 132), (232, 132), (116, 132), (117, 132), (118, 132), (119, 132), (120, 132), (121, 132), (122, 132), (125, 132), (131, 132), (203, 132), (209, 132), (243, 132), (244, 132), (245, 132), (246, 132), (247, 132), (248, 132), (249, 132), (250, 132), (251, 132), (252, 132), (31, 134), (33, 134), (38, 134), (42, 134), (43, 134), (44, 134), (45, 134), (46, 134), (47, 134), (49, 134), (52, 134), (211, 134), (212, 134), (213, 134), (226, 134), (233, 134), (235, 134), (236, 134), (35, 136), (36, 136), (37, 136), (39, 136), (40, 136), (41, 136), (48, 136), (220, 136), (221, 136), (223, 136), (224, 136), (225, 136), (26, 140), (27, 140), (28, 140), (62, 140), (64, 140), (65, 140), (66, 140), (67, 140), (68, 140), (69, 140), (70, 140), (130, 140), (194, 140), (195, 140), (196, 140), (197, 140), (198, 140), (199, 140), (192, 142), (237, 142), (238, 142), (239, 142), (214, 165), (215, 165), (217, 165), (216, 167), (218, 167), (219, 169), (110, 179), (193, 222), (75, 236), (54, 248), (59, 266), (60, 266), (53, 288), (58, 348), (71, 352), (56, 356), (210, 2140)]
        for kind, tail in spans:
            for version, common in ((3,179),(4,181),(5,182)):
                root = struct.pack('<HI',1,4000)+record(name='Building',version=version)
                name=b'AuthoredSpan';header=struct.pack('<HHH',65535,0,len(name))+name
                body=bytearray(common);struct.pack_into('<H',body,0,version);struct.pack_into('<H',body,16,kind)
                closed=root+header+body+bytes(tail)
                suffix=record(0x1234,'UnknownTail',version)
                result=audit.parse_table(closed+suffix,0)
                self.assertEqual((result['status'],len(result['records']),result['records'][-1]['end']),('unsupported_tail',2,len(closed)))
                self.assertEqual(audit.parse_table(closed[:-1],0)['records'],[])

    def test_monument_subversion_boundaries(self):
        for kind in [76, 77, 78, 79, 80, 81, 82, 83, 84, 92, 93, 253, 254, 255, 256, 257, 258, 259, 260]:
            for version,common in ((3,179),(4,181),(5,182)):
                for sub,tail in ((9,140),(10,141),(11,141)):
                    root=struct.pack('<HI',1,4000)+record(name='Building',version=version)
                    name=b'AuthoredMonument';header=struct.pack('<HHH',65535,0,len(name))+name
                    body=bytearray(common);struct.pack_into('<H',body,0,version);struct.pack_into('<H',body,16,kind)
                    extra=bytearray(tail);struct.pack_into('<HH',extra,0,1,sub)
                    result=audit.parse_table(root+header+body+extra+record(0x1234,'UnknownTail',version),0)
                    self.assertEqual((result['status'],len(result['records'])),('unsupported_tail',1 if sub==11 else 2))
                    self.assertEqual(audit.parse_table(root+header+body+extra[:3],0)['records'],[])

    def test_cleared_wall_gate_class_spans(self):
        for v in (3,4,5):
            data=bytearray(audit.TABLE_OFFSET);data[:8]=bytes((5,0,254,202,0,0,2,0));data+=struct.pack('<HI',1,4000)+record(name='Building',version=v)
            first=len(data)
            def cleared(name,ref=0x8001):
                b=bytearray(record(name=name,version=v));
                if not name:struct.pack_into('<H',b,0,ref)
                return b+bytes(132)
            data+=cleared('cResWall');wall_end=len(data);data+=cleared('',0x8003)+cleared('cResGate')+cleared('',0x8006)
            data+=record(version=v)*3995
            p=audit.parse_table(data,audit.TABLE_OFFSET)
            self.assertTrue(p['complete']);self.assertEqual((len(p['records']),p['cleared_class_records'],p['class_references_verified']),(4000,4,3997))
            self.assertEqual([p['records'][i]['resolved_class']for i in range(1,5)],['cResWall','cResWall','cResGate','cResGate'])
            self.assertEqual(audit.parse_table(data[:wall_end-1],audit.TABLE_OFFSET)['records'],[])
            bad=bytearray(data);struct.pack_into('<H',bad,wall_end,0x8002);self.assertEqual(audit.parse_table(bad,audit.TABLE_OFFSET)['status'],'class_reference')
            body=first+6+len('cResWall');bad=bytearray(data);bad[body+2]=1;self.assertEqual(audit.parse_table(bad,audit.TABLE_OFFSET)['status'],'unsupported_tail')

    def test_complete_versions_and_offsets(self):
        for v in (3, 4, 5):
            data = b'odd' + table([record(72, 'cWellBldg', v, 29, 38, 2, 1)], v) + b'next-section'
            result = audit.parse_table(data, 3)
            self.assertTrue(result['complete']);self.assertEqual(len(result['records']), 4000)
            target = result['records'][1]
            self.assertEqual((target['type'], target['name'], target['version'], target['x'], target['y'], target['size'], target['alive'], target['common_rotation']), (72, 'cWellBldg', v, 29, 38, 2, 1, 0x5a))
            self.assertEqual(result['end'], len(data) - len(b'next-section'))

    def test_closed_prefix_before_unsupported_tail(self):
        data = struct.pack('<HI', 1, 4000) + record(name='Building') + record(72, 'cWellBldg', 5, 3, 4, 2, 1) + record(0x1234, 'UnsupportedAuthoredClass')
        result = audit.parse_table(data, 0)
        self.assertEqual(result['status'], 'unsupported_tail');self.assertEqual(result['stop_index'], 2)
        self.assertEqual(len(result['records']), 2);self.assertFalse(result['complete'])
        self.assertEqual(data[result['records'][1]['end']:][:2], b'\xff\xff')

    def test_late_malformed_input_does_not_publish_prefix(self):
        prefix = struct.pack('<HI', 1, 4000) + record(name='Building') + record(72, 'cWellBldg')
        for suffix in (b'', b'\xff', b'\xff\xff\0', b'\x01\0', b'\x01\x80\x09\0'):
            result = audit.parse_table(prefix + suffix, 0)
            self.assertEqual(result['records'], []);self.assertFalse(result['complete'])
        complete = table([record(72, 'cWellBldg')])
        self.assertEqual(audit.parse_table(complete[:-1], 0)['records'], [])

    def test_unknown_tag_not_resynchronized(self):
        data = struct.pack('<HI', 1, 4000) + record(name='Building') + b'\0' * 140 + record(72, 'cWellBldg')
        result = audit.parse_table(data, 0)
        self.assertEqual(result['status'], 'unsupported_tag');self.assertEqual(result['records'], [])

    def test_preamble_and_anchor_uniqueness(self):
        one = b'prefix' + table() + b'terrain'
        self.assertEqual(audit.save_table_start(one, len(one) - 7), 6)
        for data in (b'absent', one + one, audit.PREAMBLE * 17):
            with self.assertRaises(ValueError):audit.save_table_start(data, len(data))
        with self.assertRaises(ValueError):audit.save_table_start(one, len(one) + 1)
        wrong = bytearray(table());struct.pack_into('<I', wrong, 2, 3999)
        self.assertEqual(audit.parse_table(wrong, 0)['records'], [])

    def test_name_bounds_and_no_tail_truncation(self):
        for name in (b'', b'x' * 256):
            data = struct.pack('<HIHHH', 1, 4000, 0xffff, 0, len(name)) + name
            self.assertEqual(audit.parse_table(data, 0)['records'], [])
        data = table([record(72, 'cWellBldg')])
        end = 6 + len(record(name='Building')) + len(record(72, 'cWellBldg'))
        self.assertEqual(audit.parse_table(data, 0, end - 1)['records'], [])

    def test_model_identity_not_number_only(self):
        self.assertTrue(audit.well_model('ALL BUILDINGS\n72,Well,{,1,2,3,}\n'))
        for text in ('72,Other,{,1,}', '71,Well,{,1,}', '72,Well,{,1,}\n72,Well,{,2,}'):
            with self.assertRaises(ValueError):audit.well_model(text)

    def test_footprint_and_wrong_coordinate_control(self):
        terrain = [0x80000] * 51984;side = 112;origin = 58
        r = audit.parse_table(table([record(72, 'cWellBldg', 5, 29, 38, 2, 1)]), 0)['records'][1]
        for y in range(2):
            for x in range(2):terrain[(38 + origin + y) * 228 + 29 + origin + x] = 8
        control = audit.footprint_control(r, side, terrain)
        self.assertEqual(control, {'cells': 4, 'on_map': 4, 'building_flag': 4, 'wrong_unshifted_off_map': 4})
        terrain[(38 + origin) * 228 + 29 + origin] = 0
        self.assertEqual(audit.footprint_control(r, side, terrain)['building_flag'], 3)
        for changes in ({'alive': 0}, {'size': 0}, {'size': 9}, {'x': 111}, {'x': -1}, {'name': 'cOther'}, {'type': 71}):
            self.assertIsNone(audit.footprint_control({**r, **changes}, side, terrain))
        self.assertIsNone(audit.footprint_control(r, 229, terrain))
        self.assertIsNone(audit.footprint_control(r, side, terrain[:-1]))

    def test_size_zero_is_not_well_or_renderable_instance(self):
        result = audit.parse_table(table([record(0xa2, 'cIndustrialBldg', 4, 59, 75, 0, 3)]), 0)
        self.assertTrue(result['complete']);self.assertIsNone(audit.footprint_control(result['records'][1], 112, [8] * 51984))

    def test_bitmap_labels_are_bounded_metadata_only(self):
        data = bytearray(40680 + 2 * 64)
        struct.pack_into('<II', data, 0, len(data), 213)
        struct.pack_into('<III', data, 12, 2, 1, 1)
        data[680:689] = b'Well.bmp\0'
        data[745:760] = b'reference label'
        self.assertEqual(audit.bitmap_names(data), [('Well.bmp', 'reference label')])
        bad = bytearray(data);bad[680:745] = b'x' * 65
        with self.assertRaises(ValueError):audit.bitmap_names(bad)
        bad = bytearray(data);struct.pack_into('<I', bad, 20, 201)
        with self.assertRaises(ValueError):audit.bitmap_names(bad)
        with self.assertRaises(ValueError):audit.bitmap_names(data[:-1])

    def test_manifest_budget_refuses_before_read(self):
        with self.assertRaises(ValueError):audit.run('/missing', {'sources': [{}] * 14})

    def test_dependency_free_skip(self):
        for value in (None, ''):
            env = dict(os.environ);env.pop('EMPEROR_GAMEDATA_ROOT', None)
            if value is not None:env['EMPEROR_GAMEDATA_ROOT'] = value
            p = subprocess.run(['/usr/bin/python3', '-I', '-S', '-B', str(Path(audit.__file__).resolve())], env=env, capture_output=True, text=True)
            self.assertEqual(p.returncode, 0, p.stderr);self.assertTrue(json.loads(p.stdout)['skipped'])


if __name__ == '__main__':unittest.main()
