"""Authored exact-match/alpha/phase fixtures; no original pixels or assets."""
import unittest,sys,os,subprocess,json,tempfile
from pathlib import Path
import numpy as np
from base_content_audit import diamond_mask,reference_base,match_masked_bases,load_pool,run,crop_reference_base,source_over_fixed_base


def own():
    mask=diamond_mask();rgb=np.zeros((40,78,3),np.uint8);rgb[mask]=[248,120,8]
    reference=np.repeat(np.repeat(rgb,2,0),2,1)
    base=np.full((1600,4),255,np.uint8);base[:,:3]=[9,123,255]
    return reference,base


class BaseContentTests(unittest.TestCase):
    def test_full_mask_duplicates_retained_not_global_uniqueness(self):
        reference,base=own();r=match_masked_bases(reference,np.stack([base,base]))
        self.assertEqual(r['matching_indices'],[0,1]);self.assertEqual((r['logical_pixels'],r['physical_pixels']),(1600,6400));self.assertTrue(r['finite_domain_only'])

    def test_one_changed_logical_pixel_cannot_be_hidden(self):
        reference,base=own();base[987,2]=232;r=match_masked_bases(reference,base[None])
        self.assertEqual(r['matching_indices'],[]);self.assertEqual(r['minimum_logical_mismatches'],1)

    def test_hash_collision_requires_exact_triplets(self):
        reference,base=own();bad=base.copy();bad[0,0]=32
        r=match_masked_bases(reference,np.stack([base,bad]),hasher=lambda _:b'collision')
        self.assertEqual(r['matching_indices'],[0])

    def test_nonopaque_pixel_keeps_whole_candidate_unknown(self):
        reference,base=own();base[6,3]=254;r=match_masked_bases(reference,base[None])
        self.assertEqual(r['unknown_nonopaque_variants'],1);self.assertIsNone(r['minimum_logical_mismatches']);self.assertEqual(r['matching_indices'],[])

    def test_all_four_physical_samples_and_raw_repetition_required(self):
        reference,base=own();reference[1,76,0]=249
        with self.assertRaisesRegex(ValueError,'raw nearest'):match_masked_bases(reference,base[None])

    def test_fixed_shape_dtype_and_empty_domain(self):
        reference,base=own()
        for candidates in (base,np.zeros((1,1599,4),np.uint8),base[None].astype(float)):
            with self.assertRaises(ValueError):match_masked_bases(reference,candidates)
        self.assertEqual(match_masked_bases(reference,np.zeros((0,1600,4),np.uint8))['matching_indices'],[])

    def test_transparent_outside_footprint_does_not_change_key(self):
        reference,base=own();mask=np.repeat(np.repeat(diamond_mask(),2,0),2,1);reference[~mask]=[8,24,40]
        self.assertEqual(match_masked_bases(reference,base[None])['matching_indices'],[0])

    def test_regular_geometry_is_checked_before_matching(self):
        reference,base=own()
        with tempfile.TemporaryDirectory()as folder:
            p=Path(folder);image=np.zeros((40,78,4),np.uint8);image[diamond_mask()]=base
            (p/'pixels').write_bytes(image.tobytes());(p/'meta').write_text('DATA/own.sg3\t1\t1\t256\t0\t0\t1\t0\n')
            pool={'schema':'regular','pixels':str(p/'pixels'),'metadata':str(p/'meta')};bases,labels=load_pool(pool)
            self.assertEqual(len(labels),1);self.assertEqual(match_masked_bases(reference,bases)['matching_indices'],[0])
            image[0,0,3]=255;(p/'pixels').write_bytes(image.tobytes())
            with self.assertRaisesRegex(ValueError,'alpha geometry'):load_pool(pool)

    def test_source_over_zero_and_full_alpha_are_exact(self):
        _,base=own();layers=np.zeros((2,1600,4),np.uint8);layers[1]=[0,0,255,255]
        out=source_over_fixed_base(base,layers)
        np.testing.assert_array_equal(out[0],base);np.testing.assert_array_equal(out[1],layers[1])

    def test_source_over_half_alpha_matches_integer_rounding(self):
        base=np.tile(np.array([255,0,0,255],np.uint8),(1600,1));layer=np.tile(np.array([0,0,128,128],np.uint8),(1,1600,1))
        out=source_over_fixed_base(base,layer)
        np.testing.assert_array_equal(out[0,0],np.array([127,0,128,255],np.uint8))

    def test_source_over_all_alpha_levels_preserve_inputs_and_bounds(self):
        _,base=own();layers=np.zeros((1,1600,4),np.uint8);a=np.arange(1600,dtype=np.uint32)%256;layers[0,:,3]=a;layers[0,:,1]=a
        before=layers.copy();out=source_over_fixed_base(base,layers)
        self.assertTrue(np.all(out[:,:,3]==255));np.testing.assert_array_equal(layers,before)

    def test_source_over_invalid_premultiplication_and_base_refuse(self):
        _,base=own();layer=np.zeros((1,1600,4),np.uint8);layer[0,3,0]=1
        with self.assertRaisesRegex(ValueError,'premultiplied'):source_over_fixed_base(base,layer)
        base[0,3]=254
        with self.assertRaises(ValueError):source_over_fixed_base(base,np.zeros((1,1600,4),np.uint8))

    def test_source_over_does_not_create_a_base_cartesian_product(self):
        _,base=own()
        with self.assertRaises(ValueError):source_over_fixed_base(np.stack([base,base]),np.zeros((1,1600,4),np.uint8))
        self.assertEqual(source_over_fixed_base(base,np.zeros((0,1600,4),np.uint8)).shape,(0,1600,4))

    def test_reference_centers_never_wrap_clip_or_truncate(self):
        frame=np.zeros((100,200,3),np.uint8)
        self.assertEqual(crop_reference_base(frame,[78,40]).shape,(80,156,3))
        for center in ([-100,-100],[77,40],[78,39],[123,40],[78.0,40],[True,40]):
            with self.assertRaises(ValueError):crop_reference_base(frame,center)

    def test_cache_root_provenance_refuses_before_file_lookup(self):
        with tempfile.TemporaryDirectory()as folder:
            with self.assertRaisesRegex(ValueError,'provenance'):run(Path(folder),{'source_root':'/different','reference':'/missing'})

    def test_unset_empty_ROOT_SKIP_without_site_packages(self):
        script=Path(__file__).with_name('base_content_audit.py')
        for value in (None,''):
            env=os.environ.copy();env.pop('EMPEROR_GAMEDATA_ROOT',None)
            if value is not None:env['EMPEROR_GAMEDATA_ROOT']=value
            r=subprocess.run([sys.executable,'-S',str(script)],env=env,capture_output=True,text=True)
            self.assertEqual(r.returncode,0);self.assertTrue(json.loads(r.stdout)['skipped'])

if __name__=='__main__':unittest.main()
