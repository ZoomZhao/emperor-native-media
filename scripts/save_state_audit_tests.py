"""Authored SAV anchor/framing fixtures; contains no original data."""
from pathlib import Path
import os,struct,tempfile,unittest,zlib,subprocess,sys
from unittest.mock import patch
import numpy as np
import save_state_audit as audit


def fixture(side=4):
    start=256;terrain=np.full((228,228),0x80005,dtype='<u4');origin=(228-side)//2
    terrain[origin:origin+side,origin:origin+side]=128
    follow=np.zeros((228,228),dtype='<u4');follow[origin,origin]=0xffffffff
    data=bytearray(start+8*audit.CELLS);data[:2]=b'\x13\0';struct.pack_into('<I',data,48,side);data[64:80]=audit.BRIEF
    # A repeated descriptive string with an invalid preceding size is allowed.
    data[112:128]=audit.BRIEF;struct.pack_into('<I',data,96,0xffffffff)
    data[start:start+4*audit.CELLS]=terrain.tobytes();data[start+4*audit.CELLS:]=follow.tobytes()
    return bytes(data)


def framed(payload):
    output=bytearray(struct.pack('<I',0xfedcbaaa))
    for offset in range(0,len(payload),32768):
        chunk=payload[offset:offset+32768];encoded=zlib.compress(chunk)
        output+=struct.pack('<III',0xabcdefff,len(encoded),len(chunk))+encoded
    return bytes(output)


class SavedStateTests(unittest.TestCase):
    def test_complete_dynamic_anchors_unsigned_rows_and_canonical_coordinates(self):
        r=audit.parse_save(framed(fixture()));self.assertEqual((r.side,r.terrain_offset,r.following_offset),(4,256,256+4*audit.CELLS))
        self.assertEqual(r.selected('following_u32',[(0,0),(1,0),(0,1)]),[0xffffffff,0,0])
        self.assertFalse(r.grid('terrain').flags.writeable)

    def test_independent_save_offsets_move_with_prefix(self):
        payload=fixture();shifted=payload[:128]+bytes(13)+payload[128:]
        r=audit.parse_save(framed(shifted));self.assertEqual(r.terrain_offset,269)
        self.assertEqual(r.selected('following_u32',[(0,0)]),[0xffffffff])

    def test_late_checksum_never_returns_partial_grid(self):
        data=bytearray(framed(fixture()));data[-1]^=1
        with self.assertRaisesRegex(ValueError,'checksum'):audit.parse_save(data)

    def test_truncated_header_record_and_extra_stream_refuse(self):
        data=framed(fixture())
        for truncated in (data[:2],data[:-1],data+bytes(1)):
            with self.assertRaises(ValueError):audit.parse_save(truncated)
        small=b'\x13\0'+bytes(30);encoded=zlib.compress(small)+b'x'
        malformed=struct.pack('<I',0xfedcbaaa)+struct.pack('<III',0xabcdefff,len(encoded),len(small))+encoded
        with self.assertRaisesRegex(ValueError,'complete zlib'):audit.parse_save(malformed)

    def test_role_and_missing_or_ambiguous_size_refuse(self):
        payload=bytearray(fixture());payload[0]=5
        with self.assertRaisesRegex(ValueError,'role'):audit.parse_save(framed(payload))
        payload=bytearray(fixture());struct.pack_into('<I',payload,48,0)
        with self.assertRaisesRegex(ValueError,'size anchor'):audit.parse_save(framed(payload))
        struct.pack_into('<I',payload,48,4);struct.pack_into('<I',payload,96,4)
        with self.assertRaisesRegex(ValueError,'ambiguous'):audit.parse_save(framed(payload))

    def test_duplicate_terrain_marker_refuses_instead_of_fitting_MAP(self):
        payload=fixture()+audit.TERRAIN_MARKER
        with self.assertRaisesRegex(ValueError,'terrain anchor ambiguous'):audit.parse_save(framed(payload))

    def test_following_span_truncation_refuses(self):
        with self.assertRaisesRegex(ValueError,'complete grid spans'):audit.parse_save(framed(fixture()[:-1]))

    def test_crop_side_cannot_be_invented_from_reference_MAP(self):
        payload=bytearray(fixture());struct.pack_into('<I',payload,48,6)
        with self.assertRaisesRegex(ValueError,'crop'):audit.parse_save(framed(payload))

    def test_cell_field_bounds_and_budgets_refuse(self):
        r=audit.parse_save(framed(fixture()))
        for cells in ([(-1,0)],[(4,0)],[(True,0)],[(0,0)]*129):
            with self.assertRaises(ValueError):r.selected('following_u32',cells)
        with self.assertRaises(ValueError):r.grid('graphics')
        with patch.object(audit,'MAX_DECODED',255):
            with self.assertRaisesRegex(ValueError,'budget'):audit.parse_save(framed(fixture()))

    def test_root_escape_and_symlink_refuse_without_writes(self):
        with tempfile.TemporaryDirectory()as folder,tempfile.TemporaryDirectory()as outside:
            root=Path(folder);owned=root/'inside.sav';owned.write_bytes(b'own');self.assertEqual(audit.rooted(root,'inside.sav'),owned.resolve());other=Path(outside)/'own.sav';other.write_bytes(b'own');(root/'link.sav').symlink_to(other)
            for path in ('../own.sav','link.sav',str(other)):
                with self.assertRaises(ValueError):audit.rooted(root,path)

    def test_hidden_save_identity_comes_from_grid_not_filename(self):
        saved=audit.parse_save(framed(fixture()))
        payload=bytearray(677327);payload[:8]=bytes.fromhex('0500feca00000200');struct.pack_into('<I',payload,84,4)
        payload[261455:469391]=saved.grid('terrain').tobytes()
        with tempfile.TemporaryDirectory()as folder:
            root=Path(folder);(root/'Cities').mkdir();(root/'Save/person').mkdir(parents=True)
            (root/'Cities/Actual.map').write_bytes(framed(payload));(root/'Save/person/.sav').write_bytes(framed(fixture()))
            (root/'Save/person/Wrong_Name.sav').write_bytes(framed(fixture()))
            result=audit.audit(root,[(0,0),(1,0)])
            self.assertEqual((result['save_files'],result['containers_and_grid_spans_verified'],result['city_MAPs']),(2,2,1))
            self.assertTrue(all(r['exact_terrain_identity_matches']==['Actual.map']for r in result['files']))
            self.assertTrue(all(r['selected_following_u32']['zero_count']==1 for r in result['files']))
            self.assertFalse(result['graphics_grid_semantics_verified'])

    def test_unset_empty_roots_succeed_SKIP_before_lookup(self):
        for value in (None,''):
            env=os.environ.copy();env.pop('EMPEROR_GAMEDATA_ROOT',None)
            if value is not None:env['EMPEROR_GAMEDATA_ROOT']=value
            r=subprocess.run([sys.executable,'-S',str(Path(audit.__file__))],env=env,capture_output=True,text=True)
            self.assertEqual(r.returncode,0);self.assertIn('"skipped": true',r.stdout)


if __name__=='__main__':unittest.main()
