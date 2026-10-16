"""Authored exact-match/alpha/phase fixtures; no original pixels or assets."""
import unittest,sys,os,subprocess,json,tempfile,struct
from unittest.mock import patch
from pathlib import Path
import numpy as np
from base_content_audit import diamond_mask,reference_base,match_masked_bases,load_pool,run,crop_reference_base,source_over_fixed_base,run_single_layer,match_causal_bases,verify_single_cell_cache,opaque_colour_envelope


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

    def cache_fixture(self,p,offset=0):
        images=np.zeros((3,40,78,4),np.uint8)
        for i in range(3):images[i][diamond_mask()]=[32*(i+1),0,0,255]
        (p/'pixels').write_bytes(images.tobytes());(p/'raw').write_bytes(images[:1].tobytes())
        (p/'single').write_text(f'DATA/own.sg3\t1\t78\t40\t{offset}\t0\t0\t0\t1\n')
        (p/'regular').write_text(f'DATA/own.sg3\t1\t1\t256\t0\t{offset}\t1\t0\n')
        return {'single':{'schema':'single','metadata':str(p/'single'),'raw':str(p/'raw'),'full':str(p/'pixels')},
                'regular':{'schema':'regular','metadata':str(p/'regular'),'pixels':str(p/'pixels')}}

    def test_negative_offsets_never_relabel_a_different_image(self):
        with tempfile.TemporaryDirectory()as folder:
            for schema,pool in self.cache_fixture(Path(folder),-24960).items():
                with self.subTest(schema=schema),self.assertRaisesRegex(ValueError,'record extent'):load_pool(pool)

    def test_valid_offsets_keep_record_bytes_and_labels(self):
        with tempfile.TemporaryDirectory()as folder:
            for schema,pool in self.cache_fixture(Path(folder),12480).items():
                bases,labels=load_pool(pool)
                self.assertTrue(np.all(bases[0,:,0]==64));self.assertEqual(labels[0]['id'],1)
                self.assertEqual(len(bases),2 if schema=='single'else 1)
                if schema=='single':self.assertTrue(np.all(bases[1,:,0]==32))

    def test_record_end_truncation_out_of_bounds_and_split_pixels_refuse(self):
        with tempfile.TemporaryDirectory()as folder:
            p=Path(folder)
            for offset in (24964,37440,37444,1):
                for schema,pool in self.cache_fixture(p,offset).items():
                    with self.subTest(schema=schema,offset=offset),self.assertRaisesRegex(ValueError,'record extent'):load_pool(pool)
            for schema,pool in self.cache_fixture(p).items():
                (p/'pixels').write_bytes((p/'pixels').read_bytes()[:12476])
                with self.subTest(schema=schema),self.assertRaisesRegex(ValueError,'record extent'):load_pool(pool)

    def test_full_regular_and_raw_storage_refuse_incomplete_pixels(self):
        with tempfile.TemporaryDirectory()as folder:
            p=Path(folder)
            for schema in ('single','regular'):
                pool=self.cache_fixture(p)[schema];(p/'pixels').write_bytes((p/'pixels').read_bytes()+b'x')
                with self.subTest(schema=schema),self.assertRaisesRegex(ValueError,'byte extent'):load_pool(pool)
            for size in (12476,12481,12484):
                pool=self.cache_fixture(p)['single'];(p/'raw').write_bytes(bytes(size))
                with self.subTest(raw_size=size),self.assertRaisesRegex(ValueError,'byte extent'):load_pool(pool)

    def test_multibase_fixed_storage_and_offsets_refuse_invalid_extents(self):
        with tempfile.TemporaryDirectory()as folder:
            p=Path(folder);pool={'schema':'multibase','metadata':str(p/'meta'),'pixels':str(p/'pixels')}
            header='archive\tid\tn\tx\ty\twidth\theight\toffset\topaque\tvariant\n'
            for offset in (-6400,4,6400):
                (p/'meta').write_text(header+f'DATA/own.sg3\t1\t2\t0\t0\t158\t80\t{offset}\t1\traw\n');(p/'pixels').write_bytes(bytes(6400))
                with self.subTest(offset=offset),self.assertRaisesRegex(ValueError,'geometry/extent'):load_pool(pool)
            (p/'meta').write_text(header+'DATA/own.sg3\t1\t2\t0\t0\t158\t80\t0\t1\traw\n')
            for size in (6396,6401,6404):
                (p/'pixels').write_bytes(bytes(size))
                with self.subTest(size=size),self.assertRaisesRegex(ValueError,'byte extent'):load_pool(pool)

    def layer_fixture(self,p,offset=0,selected=1):
        from PIL import Image
        images=np.zeros((3,40,78,4),np.uint8);images[:,diamond_mask()]=[8,0,0,16]
        (p/'pixels').write_bytes(images.tobytes());(p/'map').write_bytes(b'own');(p/'own.sg3').write_bytes(b'own')
        (p/'meta').write_text(f'own.sg3\t1\t1\t256\t0\t{offset}\t0\t{selected}\n')
        Image.new('RGB',(156,80)).save(p/'reference.png')
        return {'source_root':str(p),'map':'map','map_raw_width':4,'map_raw_offset':88,'base_pool':{},
                'layers_pixels':str(p/'pixels'),'layers_metadata':str(p/'meta'),'expected_layers':selected,
                'reference':str(p/'reference.png'),'targets':[]}

    def replay_own_layer(self,p,manifest):
        payload=bytearray(88+51984*4);struct.pack_into('<I',payload,84,140)
        with patch('map_sg3_audit.decode_container',return_value=payload),patch('map_sg3_audit.city_fields'),patch('base_content_audit.load_pool',return_value=([],[])):
            return run_single_layer(p.resolve(),manifest)

    def test_layer_reader_valid_offsets_and_all_row_bounds(self):
        with tempfile.TemporaryDirectory()as folder:
            p=Path(folder)
            self.assertEqual(self.replay_own_layer(p,self.layer_fixture(p,12480))['single_layer_count'],1)
            for selected in (0,1):
                for offset in (-24960,24964,37440,37444,1):
                    manifest=self.layer_fixture(p,offset,selected)
                    with self.subTest(selected=selected,offset=offset),self.assertRaisesRegex(ValueError,'record extent'):self.replay_own_layer(p,manifest)

    def test_layer_reader_refuses_truncated_and_incomplete_pixel_storage(self):
        with tempfile.TemporaryDirectory()as folder:
            p=Path(folder)
            for size,error in ((12476,'record extent'),(12479,'byte extent'),(37441,'byte extent')):
                manifest=self.layer_fixture(p);(p/'pixels').write_bytes(bytes(size))
                with self.subTest(size=size),self.assertRaisesRegex(ValueError,error):self.replay_own_layer(p,manifest)

    def test_causal_subset_never_hides_unchanged_mismatches(self):
        before,base=own();after=before.copy();y,x=np.argwhere(diamond_mask())[900]
        after[2*y:2*y+2,2*x:2*x+2]=[8,32,56]
        partial=base.copy();partial[900]=[56,32,8,255];partial[901,:3]=0
        r=match_causal_bases(before,after,np.stack([partial,partial]))
        self.assertEqual(r['raw_changed_logical_pixels'],1)
        self.assertEqual(len(r['causal_subset_matches']),2)
        self.assertEqual(r['causal_subset_matches'][0]['unchanged_base_mismatches'],1)
        self.assertEqual(r['complete_base_matching_indices'],[])
        self.assertFalse(r['visibility_mask_inferred']);self.assertFalse(r['engine_selected_ID_proven'])

    def test_causal_subset_full_match_is_separate_and_all_changes_required(self):
        before,base=own();after=before.copy();coords=np.argwhere(diamond_mask())
        for i in (100,900):
            y,x=coords[i];after[2*y:2*y+2,2*x:2*x+2]=[8,32,56]
        good=base.copy();good[[100,900]]=[56,32,8,255]
        bad=good.copy();bad[900]=base[900]
        r=match_causal_bases(before,after,np.stack([bad,good]))
        self.assertEqual([h['index']for h in r['causal_subset_matches']],[1])
        self.assertEqual(r['complete_base_matching_indices'],[1])
        np.testing.assert_array_equal(base[0],[9,123,255,255])

    def test_causal_subset_requires_information_and_unmodified_physical_samples(self):
        before,base=own()
        with self.assertRaisesRegex(ValueError,'discriminating'):match_causal_bases(before,before,base[None])
        after=before.copy();y,x=np.argwhere(diamond_mask())[900];after[2*y,2*x,0]^=8
        with self.assertRaisesRegex(ValueError,'raw nearest'):match_causal_bases(before,after,base[None])

    def test_causal_subset_nonopaque_never_becomes_an_inferred_mask(self):
        before,base=own();after=before.copy();y,x=np.argwhere(diamond_mask())[900]
        after[2*y:2*y+2,2*x:2*x+2]=[8,32,56];base[900]=[56,32,8,255];base[901,3]=0
        r=match_causal_bases(before,after,base[None])
        self.assertEqual(r['unknown_nonopaque_candidates'],1);self.assertEqual(r['causal_subset_matches'],[])
        for bad in (base,base[None].astype(float),np.zeros((1,1599,4),np.uint8)):
            with self.assertRaises(ValueError):match_causal_bases(before,after,bad)

    def complete_export_pair(self,p):
        pools=[]
        for name in ('cached','native'):
            d=p/name;d.mkdir();rows=[];blob=bytearray();raw=bytearray()
            for logical,height in ((201,41),(1443,40)):
                off=len(blob);canvas=np.zeros((height,78,4),np.uint8)
                canvas[-40:][diamond_mask()]=[8,16,24,255]
                rows.append(f'own.sg3\t{logical}\t78\t{height}\t{off}\t1600\t0\t0\t{logical}\t0\t0')
                blob.extend(canvas.tobytes());raw.extend(canvas[-40:].tobytes())
            (d/'meta').write_text('\n'.join(rows)+'\n');(d/'full').write_bytes(blob);(d/'raw').write_bytes(raw)
            pools.append({'schema':'single','metadata':str(d/'meta'),'full':str(d/'full'),'raw':str(d/'raw')})
        return pools

    def test_complete_native_replay_counts_only_actual_full_byte_comparisons(self):
        with tempfile.TemporaryDirectory()as folder:
            a,b=self.complete_export_pair(Path(folder));r=verify_single_cell_cache(a,b)
            self.assertEqual(r['full_canvases_verified'],[2,2]);self.assertEqual(r['raw_bases_verified'],[2,2])
            self.assertEqual(r['full_canvas_bytes_compared'],78*(41+40)*4)

    def test_complete_native_replay_rejects_each_canvas_and_each_BGRA_channel(self):
        with tempfile.TemporaryDirectory()as folder:
            a,b=self.complete_export_pair(Path(folder));file=Path(a['full']);original=file.read_bytes()
            # Includes transparent top, an opaque unselected pixel, and the final
            # pixel. Low-bit mutations must fail without RGB5 quantization.
            for off,size in ((0,78*41*4),(78*41*4,78*40*4)):
                for pos in (off,off+(41-40)*78*4+38*4,off+size-4):
                    for channel in range(4):
                        changed=bytearray(original);changed[pos+channel]^=1;file.write_bytes(changed)
                        with self.subTest(offset=pos,channel=channel),self.assertRaisesRegex(ValueError,'full canvas differs'):
                            verify_single_cell_cache(a,b)
            file.write_bytes(original);self.assertEqual(verify_single_cell_cache(a,b)['full_canvases_verified'],[2,2])

    def test_complete_native_replay_rejects_raw_corruption_in_every_record(self):
        with tempfile.TemporaryDirectory()as folder:
            a,b=self.complete_export_pair(Path(folder));file=Path(a['raw']);original=file.read_bytes()
            for off in (0,12480):
                changed=bytearray(original);changed[off+12479]^=1;file.write_bytes(changed)
                with self.subTest(offset=off),self.assertRaisesRegex(ValueError,'raw base differs'):verify_single_cell_cache(a,b)

    def test_complete_native_replay_rejects_relabelled_incomplete_or_extra_storage(self):
        with tempfile.TemporaryDirectory()as folder:
            a,b=self.complete_export_pair(Path(folder));meta=Path(a['metadata']);original=meta.read_text()
            meta.write_text(original.replace('\t201\t','\t202\t'))
            with self.assertRaisesRegex(ValueError,'metadata differs'):verify_single_cell_cache(a,b)
            meta.write_text(original)
            for key in ('full','raw'):
                file=Path(a[key]);blob=file.read_bytes()
                for data in (blob[:-4],blob+b'\0'*4):
                    file.write_bytes(data)
                    with self.subTest(key=key,length=len(data)),self.assertRaisesRegex(ValueError,'byte extent'):verify_single_cell_cache(a,b)
                file.write_bytes(blob)
            meta.write_text(original.replace('\t12792\t','\t0\t'))
            with self.assertRaisesRegex(ValueError,'geometry/order'):verify_single_cell_cache(a,b)

    def colour_fixture(self):
        frame=np.full((1,2,3),[8,16,24],np.uint8);mask=np.ones((1,2),bool)
        image=np.full((1,2,4),[24,16,8,255],np.uint8)
        return frame,mask,np.zeros_like(mask),image

    def test_colour_union_can_pass_without_any_consistent_source(self):
        frame,need,guard,image=self.colour_fixture();a=image.copy();b=image.copy()
        a[0,1]=[0,0,0,255];b[0,0]=[0,0,0,255]
        c=[{'label':{'id':i,'pose':0},'image':im,'top_left':[0,0]}for i,im in enumerate((a,b))]
        r=opaque_colour_envelope(frame,need,guard,c)
        self.assertEqual(r['matching_candidate_indices'],[[0],[1]])
        self.assertEqual(r['summary']['necessary_condition'],'not_excluded')
        self.assertEqual(r['summary']['whole_visible_component_candidates'],[])
        self.assertFalse(r['summary']['actual_assignment_or_order_proven'])
        self.assertEqual([z['required_conflict_indices']for z in r['candidates']],[[1],[0]])

    def test_colour_support_without_correct_colour_is_excluded(self):
        frame,need,guard,image=self.colour_fixture();image[:,:,:3]=0
        r=opaque_colour_envelope(frame,need,guard,[{'label':'wrong','image':image,'top_left':[0,0]}])
        self.assertEqual(r['summary']['geometric_support_reachable'],2)
        self.assertEqual(r['summary']['opaque_colour_explainable'],0)
        self.assertEqual(r['unexplained_positions'],[[0,0],[1,0]])
        self.assertEqual(r['summary']['necessary_condition'],'excluded')

    def test_colour_matches_retain_protected_region_conflicts(self):
        frame,need,guard,image=self.colour_fixture();need[0,1]=False;guard[0,1]=True;image[0,1]=[0,0,0,255]
        r=opaque_colour_envelope(frame,need,guard,[{'label':'retained','image':image,'top_left':[0,0]}])
        self.assertEqual(r['matching_candidate_indices'],[[0]])
        self.assertEqual(r['candidates'][0]['protected_conflict_positions'],[[1,0]])
        self.assertEqual(r['candidates'][0]['visible_opaque_mismatch_positions'],[[1,0]])

    def test_colour_partial_alpha_stays_unknown_without_a_blend_formula(self):
        frame,need,guard,image=self.colour_fixture();image[:]=[4,8,12,128]
        r=opaque_colour_envelope(frame,need,guard,[{'label':'unknown','image':image,'top_left':[0,0]}])
        self.assertEqual(r['summary']['unknown_only'],2);self.assertEqual(r['unexplained_positions'],[])
        self.assertEqual(r['summary']['necessary_condition'],'unknown');self.assertFalse(r['summary']['blend_formula_used'])
        self.assertEqual(r['unknown_candidate_indices'],[[0],[0]])

    def test_colour_duplicates_whole_candidates_and_empty_sources_stay_distinct(self):
        frame,need,guard,image=self.colour_fixture()
        c=[{'label':i,'image':im,'top_left':[0,0]}for i,im in enumerate((image,image,np.zeros_like(image)))]
        r=opaque_colour_envelope(frame,need,guard,c)
        self.assertEqual(r['matching_candidate_indices'],[[0,1],[0,1]])
        self.assertEqual(r['summary']['whole_visible_component_candidates'],[0,1]);self.assertEqual(len(r['candidates']),3)
        self.assertEqual(r['candidates'][2]['visible_opaque_pixels'],0)

    def test_colour_clips_only_at_fixed_frame_bounds(self):
        frame,need,guard,image=self.colour_fixture();image[0,0]=[0,0,0,255]
        r=opaque_colour_envelope(frame,need,guard,[{'label':1,'image':image,'top_left':[-1,0]}])
        self.assertEqual(r['matching_candidate_indices'],[[0],[]]);self.assertEqual(r['unexplained_positions'],[[1,0]])
        self.assertEqual(r['summary']['whole_visible_component_candidates'],[0])

    def test_colour_RGB5_contract_is_fixed_for_all_candidates(self):
        frame,need,guard,image=self.colour_fixture();image[:]=[31,23,15,255]
        r=opaque_colour_envelope(frame,need,guard,[{'label':1,'image':image,'top_left':[0,0]}])
        self.assertEqual(r['summary']['opaque_colour_explainable'],2);self.assertEqual(r['summary']['RGB_bits'],5)

    def test_colour_bounds_masks_and_source_contract_refuse(self):
        frame,need,guard,image=self.colour_fixture();candidate={'label':1,'image':image,'top_left':[0,0]}
        for target,protected in ((need.astype('u1'),guard),(need,need),(guard,guard)):
            with self.assertRaises(ValueError):opaque_colour_envelope(frame,target,protected,[candidate])
        for position in ([True,0],[0.0,0]):
            with self.assertRaises(ValueError):opaque_colour_envelope(frame,need,guard,[dict(candidate,top_left=position)])
        bad=image.copy();bad[0,0]=[1,0,0,0]
        with self.assertRaises(ValueError):opaque_colour_envelope(frame,need,guard,[dict(candidate,image=bad)])
        with self.assertRaises(ValueError):opaque_colour_envelope(frame,need,guard,[candidate]*4097)

    def test_unset_empty_ROOT_SKIP_without_site_packages(self):
        script=Path(__file__).with_name('base_content_audit.py')
        for value in (None,''):
            env=os.environ.copy();env.pop('EMPEROR_GAMEDATA_ROOT',None)
            if value is not None:env['EMPEROR_GAMEDATA_ROOT']=value
            r=subprocess.run([sys.executable,'-S',str(script)],env=env,capture_output=True,text=True)
            self.assertEqual(r.returncode,0);self.assertTrue(json.loads(r.stdout)['skipped'])

if __name__=='__main__':unittest.main()
