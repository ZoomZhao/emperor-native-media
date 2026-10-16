"""Entirely authored fixtures for geometry and quantized-vs-raw comparison."""
import unittest
import numpy as np
from reference_compare import (compare_rgb, compare_nearest_scaled_rgb, bottom_base_canvas_bounds, isometric_subbase_centres, diagonal_grid_matrix,
                               project_points, landmark_residuals, nearest_mask_distances,
                               mask_translation_candidates,check_fixed_candidates,audit_static_labels,compare_cell_tints,base_interior_mask,classify_spatial_composite)


class ReferenceComparisonTests(unittest.TestCase):
    def test_base_boundary_keeps_outer_edges_and_one_pixel_width(self):
        diamond=np.zeros((40,78),bool)
        for y in range(40):
            left=38-2*y if y<20 else 2*y-40;diamond[y,left:78-left]=True
        original=diamond.copy();inside=base_interior_mask(diamond)
        self.assertEqual((int(diamond.sum()),int(inside.sum()),int((diamond&~inside).sum())),(1600,1444,156))
        np.testing.assert_array_equal(diamond,original)
        self.assertFalse(base_interior_mask(np.ones((1,1),bool)).any())

    def test_spatial_partition_is_disjoint_with_history_and_background(self):
        ref=np.zeros((1,6,3),np.uint8);src=ref.copy();src[0,1]=8;src[0,4]=8
        kind=np.array([[1,2,1,2,3,0]],np.uint8)
        bases=np.array([[1,1,1,1,1,0]],np.uint16);rles=np.array([[0,0,1,1,1,0]],np.uint16)
        result=classify_spatial_composite(ref,src,np.ones((1,6),bool),kind,bases,rles)
        self.assertEqual(result['total'],{'equal':4,'different':2,'denominator':6})
        self.assertEqual(sum(x['denominator']for x in result['partitions'].values()),6)
        self.assertTrue(all(x['denominator']==1 for x in result['partitions'].values()))
        self.assertEqual(result['one_provided_source']['different'],1)
        self.assertEqual(result['multiple_provided_sources']['different'],1)

    def test_spatial_eligibility_and_quantization_do_not_create_masks(self):
        ref=np.array([[[248,0,0],[0,0,0]]],np.uint8);src=np.array([[[255,0,0],[255,0,0]]],np.uint8)
        eligible=np.array([[True,False]]);kind=np.array([[1,1]],np.uint8);base=np.ones((1,2),np.uint16);rle=np.zeros((1,2),np.uint16)
        self.assertEqual(classify_spatial_composite(ref,src,eligible,kind,base,rle)['total'],{'equal':1,'different':0,'denominator':1})
        self.assertEqual(classify_spatial_composite(ref,src,eligible,kind,base,rle,8)['total']['different'],1)
        self.assertEqual(classify_spatial_composite(ref,src,np.zeros_like(eligible),kind,base,rle)['total']['denominator'],0)

    def test_spatial_invalid_support_kind_and_dtype_refuse(self):
        rgb=np.zeros((1,1,3),np.uint8);eligible=np.ones((1,1),bool);kind=np.ones((1,1),np.uint8);base=np.ones((1,1),np.uint16);rle=np.zeros((1,1),np.uint16)
        for invalid in (np.zeros_like(kind),np.full_like(kind,4),np.full_like(kind,3)):
            with self.assertRaises(ValueError):classify_spatial_composite(rgb,rgb,eligible,invalid,base,rle)
        for bits in (True,0,9,5.0):
            with self.assertRaises(ValueError):classify_spatial_composite(rgb,rgb,eligible,kind,base,rle,bits)
        with self.assertRaises(ValueError):classify_spatial_composite(rgb,rgb,eligible,kind,base.astype(np.uint8),rle)
        with self.assertRaises(ValueError):base_interior_mask(np.zeros((0,1),bool))

    def test_spatial_support_addition_does_not_wrap_uint16(self):
        rgb=np.zeros((1,1,3),np.uint8);mask=np.ones((1,1),bool);kind=np.full((1,1),3,np.uint8);counts=np.full((1,1),32768,np.uint16)
        result=classify_spatial_composite(rgb,rgb,mask,kind,counts,counts)
        self.assertEqual(result['multiple_provided_sources']['denominator'],1)
        self.assertEqual(result['partitions']['final_RLE']['denominator'],1)

    def test_tint_preserves_unchanged_and_neutral_pixels(self):
        before=np.array([[[0,32,0],[24,0,24],[0,0,0],[0,0,0]]],np.uint8)
        preview=np.array([[[0,32,0],[24,0,24],[40,0,0],[8,0,0]]],np.uint8)
        masks=np.array([[[True,True,False,False]],[[False,False,True,True]]])
        r=compare_cell_tints(before,preview,before,masks)
        self.assertEqual((r[0].mask_pixels,r[0].preview_changed_pixels,r[0].green_pixels,r[0].neutral_pixels),(2,0,1,1))
        self.assertEqual((r[1].red_pixels,r[1].neutral_pixels),(1,1))
        self.assertEqual(sum(c.cancel_changed_pixels for c in r),0)

    def test_tint_cancel_and_background_are_separate(self):
        a=np.zeros((1,2,3),np.uint8);b=a.copy();b[0,0]=[40,0,0]
        after=a.copy();after[0,1]=255;masks=np.array([[[True,False]]])
        self.assertEqual(compare_cell_tints(a,b,after,masks)[0].cancel_changed_pixels,0)
        after[0,0,0]=1
        self.assertEqual(compare_cell_tints(a,b,after,masks)[0].cancel_changed_pixels,1)

    def test_tint_margin_does_not_overflow_uint8(self):
        a=np.array([[[255,254,0],[254,0,0]]],np.uint8);masks=np.ones((1,1,2),bool)
        r=compare_cell_tints(a,a,a,masks)[0]
        self.assertEqual((r.red_pixels,r.green_pixels,r.neutral_pixels),(1,0,1))
        self.assertEqual(r.preview_changed_pixels,0)

    def test_tint_duplicate_empty_or_overlap_masks_refuse(self):
        a=np.zeros((2,2,3),np.uint8);m=np.zeros((2,2,2),bool);m[:,0,0]=True
        with self.assertRaisesRegex(ValueError,'nonoverlapping'):compare_cell_tints(a,a,a,m)
        m[1]=False
        with self.assertRaisesRegex(ValueError,'nonempty'):compare_cell_tints(a,a,a,m)

    def test_tint_bad_frames_margin_and_budget_refuse(self):
        a=np.zeros((2,2,3),np.uint8);m=np.ones((1,2,2),bool)
        for margin in (True,-1,256,8.0):
            with self.assertRaises(ValueError):compare_cell_tints(a,a,a,m,margin)
        with self.assertRaises(ValueError):compare_cell_tints(a.astype(float),a,a,m)
        with self.assertRaises(ValueError):compare_cell_tints(a,a[:1],a,m)
        with self.assertRaises(ValueError):compare_cell_tints(a,a,a,m.astype(np.uint8))
        large=np.zeros((1024,1024,3),np.uint8)
        with self.assertRaises(ValueError):compare_cell_tints(large,large,large,np.zeros((8,1024,1024),bool))

    def test_quantization_is_not_raw_equality(self):
        ref=np.array([[[248,120,8]]],np.uint8);decoded=np.array([[[255,123,9]]],np.uint8)
        r=compare_rgb(ref,decoded,np.ones((1,1),bool));self.assertTrue(r.normalized_exact)
        self.assertEqual(r.raw_max_error,7);self.assertGreater(r.raw_rmse,0)
        self.assertFalse(compare_rgb(ref,decoded,np.ones((1,1),bool),8).normalized_exact)

    def test_corrupted_channel_cannot_be_float_clamped_to_match(self):
        ref=np.array([[[64,64,64]]],np.uint8);decoded=ref.copy();decoded[0,0,1]=72
        r=compare_rgb(ref,decoded,np.ones((1,1),bool));self.assertFalse(r.normalized_exact)
        self.assertEqual((r.normalized_equal_samples,r.normalized_max_error),(2,1))

    def test_transparent_background_excluded(self):
        ref=np.array([[[0,0,0],[64,64,64]]],np.uint8);d=ref.copy();d[0,0]=255
        r=compare_rgb(ref,d,np.array([[False,True]]));self.assertTrue(r.normalized_exact)
        self.assertEqual((r.opaque_pixels,r.color_samples),(1,3))

    def test_invalid_or_empty_arrays_reject(self):
        a=np.zeros((2,2,3),np.uint8)
        for mask in [np.zeros((2,2),bool),np.ones((1,2),bool),np.ones((2,2),np.uint8)]:
            with self.assertRaises(ValueError):compare_rgb(a,a,mask)
        with self.assertRaises(ValueError):compare_rgb(a.astype(float),a,np.ones((2,2),bool))
        with self.assertRaises(ValueError):compare_rgb(a,a,np.ones((2,2),bool),0)

    def test_affine_basis_and_inverse(self):
        m=diagonal_grid_matrix(2,3,100,-10);points=np.array([[0,0],[1,0],[0,1],[5,8]],float)
        result=project_points(points,m);np.testing.assert_allclose(result,[[100,-10],[102,-7],[98,-7],[94,29]])
        np.testing.assert_allclose((result-m[:,2])@np.linalg.inv(m[:,:2]).T,points,atol=1e-12)

    def test_heldout_matched_residuals(self):
        m=diagonal_grid_matrix(2,2,20,30);points=np.array([[0,0],[1,3],[5,7]])
        target=project_points(points,m)+[[3,4],[0,0],[0,0]];r=landmark_residuals(points,target,m)
        self.assertEqual(r['landmarks'],3);self.assertAlmostEqual(r['RMS_pixels'],5/np.sqrt(3));self.assertEqual(r['max_pixels'],5)

    def test_invalid_geometry_not_camera_parameters(self):
        for params in [(0,1,0,0),(1,-1,0,0),(1,1,float('nan'),0)]:
            with self.assertRaises(ValueError):diagonal_grid_matrix(*params)
        with self.assertRaises(ValueError):project_points([[float('inf'),0]],np.eye(2,3))
        with self.assertRaises(ValueError):landmark_residuals([],[],np.eye(2,3))

    def test_nearest_pixel_is_not_unique_landmark(self):
        mask=np.zeros((10,10),bool);mask[4,4]=True;mask[4,5]=True
        self.assertEqual(nearest_mask_distances([[4,4],[4,7],[-.1,4],[10,4]],mask),[0.,3.,None,None])
        with self.assertRaises(ValueError):nearest_mask_distances([[0,0]],np.zeros((10,10),bool))

    def test_public_two_by_two_base_layout_uses_actual_height(self):
        self.assertEqual(isometric_subbase_centres(158,100),
                         ((0,0,79,40),(0,1,39,60),(1,0,119,60),(1,1,79,80)))
        self.assertEqual(isometric_subbase_centres(78,49),((0,0,39,29),))

    def test_public_three_by_three_layout_retains_each_square_once(self):
        a=isometric_subbase_centres(238,120)
        self.assertEqual(len(a),9)
        self.assertEqual({(x,y)for x,y,_,_ in a},{(x,y)for x in range(3)for y in range(3)})
        self.assertEqual(a[0],(0,0,119,20));self.assertEqual(a[-1],(2,2,119,100))

    def test_subbase_target_alignment_is_semantic_not_fitted(self):
        layout=isometric_subbase_centres(158,100);target=np.array([400,600])
        for _,_,cx,cy in layout:
            origin=target-2*np.array([cx,cy])
            np.testing.assert_array_equal(origin+2*np.array([cx,cy]),target)
        self.assertEqual(len({(cx,cy)for _,_,cx,cy in layout}),4)

    def test_subbase_wrong_size_short_canvas_and_budgets_refuse(self):
        for w,h in [(157,80),(158,79),(58,30),(158.,80),(True,80),(1358,680),(158,4097)]:
            with self.assertRaises(ValueError):isometric_subbase_centres(w,h)

    def test_bottom_base_handles_tall_canvas_without_recentering_ground(self):
        self.assertEqual(bottom_base_canvas_bounds((100,100),(78,40),(78,40)),(22,60,178,140))
        self.assertEqual(bottom_base_canvas_bounds((100,100),(78,49),(78,40)),(22,42,178,140))
        self.assertEqual(bottom_base_canvas_bounds((100,100),(78,46),(78,40)),(22,48,178,140))
        self.assertEqual(bottom_base_canvas_bounds((50,50),(78,46),(78,40),1),(11,24,89,70))

    def test_bottom_base_sampling_preserves_authored_positive_and_exposes_wrong_centre(self):
        tile=np.arange(9*6*3,dtype=np.uint8).reshape(9,6,3)
        image=np.zeros((50,50,3),np.uint8)
        image[16:34,14:26]=np.repeat(np.repeat(tile,2,0),2,1)
        l,t,r,b=bottom_base_canvas_bounds((20,30),(6,9),(6,4))
        exact=compare_nearest_scaled_rgb(image[t:b,l:r],tile,np.ones((9,6),bool))
        wrong=compare_nearest_scaled_rgb(image[21:39,14:26],tile,np.ones((9,6),bool))
        self.assertTrue(exact.physical.normalized_exact)
        self.assertTrue(exact.raw_nearest_replication)
        self.assertFalse(wrong.physical.normalized_exact)

    def test_bottom_base_negative_coordinates_are_not_clipped_or_shifted(self):
        self.assertEqual(bottom_base_canvas_bounds((0,0),(6,9),(6,4)),(-6,-14,6,4))
        with self.assertRaises(ValueError):bottom_base_canvas_bounds((.5,0),(6,9),(6,4))

    def test_bottom_base_bad_sizes_scale_and_nonfinite_centres_refuse(self):
        for center,canvas,base,scale in [((float('nan'),0),(6,9),(6,4),2),
                ((0,float('inf')),(6,9),(6,4),2),((0,0),(6,9),(6,10),2),
                ((0,0),(6,9),(7,4),2),((0,0),(6.,9),(6,4),2),
                ((0,0),(6,9),(6,4),True),((0,0),(6,9),(6,4),2.),
                ((0,0),(6,0),(6,4),2),((0,0),(4096,9),(6,4),2)]:
            with self.assertRaises(ValueError):bottom_base_canvas_bounds(center,canvas,base,scale)

    def test_scaled_counts_distinguish_repeated_physical_pixels(self):
        decoded=np.array([[[64,80,96],[96,80,64]]],np.uint8)
        ref=np.repeat(np.repeat(decoded,2,axis=0),2,axis=1)
        r=compare_nearest_scaled_rgb(ref,decoded,np.ones((1,2),bool))
        self.assertEqual((r.logical.opaque_pixels,r.logical.color_samples),(2,6))
        self.assertEqual((r.physical.opaque_pixels,r.physical.color_samples),(8,24))
        self.assertTrue(r.physical.normalized_exact);self.assertTrue(r.raw_nearest_replication)

    def test_scaled_subpixel_corruption_is_not_hidden_by_subsampling(self):
        decoded=np.full((1,1,3),64,np.uint8);ref=np.repeat(np.repeat(decoded,2,axis=0),2,axis=1)
        ref[1,1,0]=72;r=compare_nearest_scaled_rgb(ref,decoded,np.ones((1,1),bool))
        self.assertTrue(r.logical.normalized_exact);self.assertFalse(r.physical.normalized_exact)
        self.assertFalse(r.raw_nearest_replication);self.assertEqual(r.physical.normalized_equal_samples,11)

    def test_raw_replication_is_separate_from_quantized_equality(self):
        decoded=np.full((1,1,3),64,np.uint8);ref=np.repeat(np.repeat(decoded,2,axis=0),2,axis=1)
        ref[0,1,1]=65;r=compare_nearest_scaled_rgb(ref,decoded,np.ones((1,1),bool))
        self.assertTrue(r.physical.normalized_exact);self.assertFalse(r.raw_nearest_replication)
        self.assertEqual(r.raw_pixels_equal_to_block_representative,3)

    def test_scaled_shape_and_integer_scale_guards(self):
        a=np.zeros((1,1,3),np.uint8);ref=np.zeros((2,2,3),np.uint8)
        for scale in [0,9,2.0,True]:
            with self.assertRaises(ValueError):compare_nearest_scaled_rgb(ref,a,np.ones((1,1),bool),scale)
        with self.assertRaises(ValueError):compare_nearest_scaled_rgb(a,a,np.ones((1,1),bool))

    def test_translation_fit_and_independent_holdout_counts(self):
        mask=np.zeros((4,3),bool);mask[[0,0,1,2,2],[0,1,0,0,1]]=True
        r=mask_translation_candidates(mask,[[0,0],[1,0],[0,1]],[0,1],[2],[((1,0),(0,1))])
        self.assertEqual(len(r),2);self.assertEqual(sum(c.heldout_passed for c in r),1)
        self.assertEqual({c.offset_xy for c in r},{(0,0),(0,2)})

    def test_translation_ambiguity_is_preserved_across_all_orientations(self):
        r=mask_translation_candidates(np.ones((3,3),bool),[[0,0],[1,0],[0,1]],[0,1],[2])
        self.assertEqual(len(r),48);self.assertEqual(sum(c.heldout_passed for c in r),32)
        self.assertEqual({c.orientation_index for c in r},set(range(8)))

    def test_heldout_origin_does_not_restrict_training_search(self):
        mask=np.zeros((3,3),bool);mask[0,1]=True;mask[1,1]=True
        r=mask_translation_candidates(mask,[[0,0],[1,0],[1,1]],[1,2],[0],[((1,0),(0,1))])
        self.assertEqual(len(r),1);self.assertEqual(r[0].offset_xy,(0,0));self.assertFalse(r[0].heldout_passed)

    def test_heldout_outside_mask_cannot_wrap_or_clip_into_a_match(self):
        r=mask_translation_candidates(np.ones((2,2),bool),[[0,0],[1,0],[0,-1]],
                                      [0,1],[2],[((1,0),(0,1))])
        self.assertEqual(len(r),2)
        by_offset={c.offset_xy:c.heldout_passed for c in r}
        self.assertEqual(by_offset,{(0,0):False,(0,1):True})

    def test_translation_invalid_partitions_or_orientation_refuse(self):
        mask=np.ones((3,3),bool);points=[[0,0],[1,0],[0,1]]
        for train,hold in [([0,1],[1,2]),([0],[2]),([0,0],[1,2]),([0,1],[]),([0,1],[3])]:
            with self.assertRaises(ValueError):mask_translation_candidates(mask,points,train,hold)
        with self.assertRaises(ValueError):mask_translation_candidates(mask,points,[0,1],[2],[((1,1),(0,1))])
        with self.assertRaises(ValueError):mask_translation_candidates(mask,[[0,0],[0,0]],[0],[1])
        self.assertEqual(mask_translation_candidates(np.zeros((3,3),bool),points,[0,1],[2]),())

    def test_translation_budget_refuses_before_large_candidate_arrays(self):
        points=np.array([(x,y)for y in range(20)for x in range(20)],int)
        with self.assertRaisesRegex(ValueError,'budget'):
            mask_translation_candidates(np.ones((512,512),bool),points,list(range(399)),[399])

    def test_fixed_candidates_preserve_failed_training_and_heldout_separately(self):
        mask=np.zeros((4,4),bool);mask[0,0]=True;mask[1,1]=True;mask[2,2]=True
        coords=[[[0,0],[1,1],[2,2]],[[0,0],[1,1],[3,3]],[[3,3],[0,3],[2,2]]]
        r=check_fixed_candidates(mask,coords,[0,1],[2])
        self.assertEqual([c.candidate_index for c in r],[0,1,2])
        self.assertEqual([(c.training_passed,c.heldout_passed)for c in r],
                         [(True,True),(True,False),(False,True)])

    def test_fixed_candidates_do_not_select_a_winner_among_equal_support(self):
        r=check_fixed_candidates(np.ones((4,4),bool),
                                 [[[0,0],[1,1]],[[2,2],[3,3]]],[0],[1])
        self.assertEqual(len(r),2)
        self.assertTrue(all(c.training_passed and c.heldout_passed for c in r))

    def test_fixed_candidate_outside_cells_do_not_wrap_or_clip(self):
        coords=np.array([[[-1,0],[0,0]],[[2,1],[1,1]]],int);original=coords.copy()
        coords.setflags(write=False)
        r=check_fixed_candidates(np.ones((2,2),bool),coords,[0],[1])
        self.assertEqual([c.training_matches for c in r],[0,0])
        self.assertTrue(all(c.heldout_passed for c in r))
        np.testing.assert_array_equal(coords,original)

    def test_fixed_candidate_invalid_geometry_and_partition_refuse(self):
        mask=np.ones((2,2),bool);coords=[[[0,0],[1,1]]]
        for train,hold in [([0],[0]),([0],[]),([0],[2])]:
            with self.assertRaises(ValueError):check_fixed_candidates(mask,coords,train,hold)
        with self.assertRaises(ValueError):check_fixed_candidates(mask,np.array(coords,float),[0],[1])
        with self.assertRaises(ValueError):check_fixed_candidates(mask.astype(np.uint8),coords,[0],[1])
        with self.assertRaises(ValueError):check_fixed_candidates(mask,[[[8193,0],[1,1]]],[0],[1])
        with self.assertRaises(ValueError):check_fixed_candidates(mask,[[[0,0],[0,0]]],[0],[1])

    def test_fixed_candidate_budget_refuses_before_large_hit_arrays(self):
        with self.assertRaisesRegex(ValueError,'budget'):
            check_fixed_candidates(np.ones((2,2),bool),np.zeros((4096,2048,2),np.int16),
                                   list(range(2047)),[2047])

    def test_static_same_key_different_labels_refutes_single_value(self):
        r=audit_static_labels(['a','a','a'],[10,20,30],[0,1],[2])
        self.assertEqual(r['training']['ambiguous_keys'],1)
        self.assertEqual(r['training']['minimum_single_label_disagreements'],1)
        self.assertEqual(r['first_training_counterexample_indices'],[0,1])
        self.assertEqual(r['heldout_with_ambiguous_training_key'],1)
        self.assertEqual(r['heldout_new_labels_for_seen_key'],1)

    def test_static_consistency_counts_without_returning_an_engine_selector(self):
        r=audit_static_labels([(1,2),(3,4),(1,2),(3,4)],[7,8,7,8],[0,1],[2,3])
        self.assertEqual(r['joint']['minimum_single_label_disagreements'],0)
        self.assertEqual(r['heldout_unique_key_disagreements'],0)
        self.assertEqual(r['heldout_with_unique_training_key'],2)
        self.assertNotIn('mapping',r);self.assertNotIn('verified_rule',r)

    def test_static_unseen_keys_are_unknown_not_passes(self):
        r=audit_static_labels(['a','b'],[10,20],[0],[1])
        self.assertEqual(r['heldout_unseen_keys'],1)
        self.assertEqual(r['heldout_with_unique_training_key'],0)

    def test_static_heldout_conflicts_do_not_leak_into_training(self):
        r=audit_static_labels(['a','a','a'],[10,20,30],[0],[1,2])
        self.assertEqual(r['training']['ambiguous_keys'],0)
        self.assertEqual(r['heldout']['ambiguous_keys'],1)
        self.assertEqual(r['heldout_unique_key_disagreements'],2)
        self.assertEqual(r['joint']['minimum_single_label_disagreements'],2)
        self.assertIsNone(r['first_training_counterexample_indices'])

    def test_static_invalid_partitions_missing_or_unbounded_categories_refuse(self):
        for train,hold in [([0],[0]),([0],[]),([0],[2])]:
            with self.assertRaises(ValueError):audit_static_labels(['a','b'],[1,2],train,hold)
        for key in [None,float('nan'),['a'],'a'*257,()]:
            with self.assertRaises(ValueError):audit_static_labels([key,'b'],[1,2],[0],[1])
        with self.assertRaises(ValueError):audit_static_labels(['a']*4097,[1]*4097,[0],list(range(1,4097)))
        with self.assertRaises(ValueError):audit_static_labels(['a','b'],[1],[0],[1])


class ScreenCellRegistrationTests(unittest.TestCase):
    def fixture(self,scale=1):
        from screen_cell import CaptureSpace
        width,height=400*scale,256*scale;space=CaptureSpace((10,20,400,256),(width,height),scale)
        a=np.zeros((height,width,3),np.uint8);b=a.copy();diamond=np.zeros((40,78),bool)
        for y in range(40):
            edge=38-2*y if y<20 else 2*y-40;diamond[y,edge:78-edge]=True
        mask=np.repeat(np.repeat(diamond,scale,0),scale,1);b[100*scale:140*scale,81*scale:159*scale][mask]=[0,248,0]
        return a,b,a.copy(),[space]*3,[130,140],[120*scale,120*scale],[[40*scale,-40*scale],[20*scale,20*scale]],[[0,0],[1,0],[0,1]],[30*scale,70*scale,250*scale,210*scale],[0,0,width,height]

    def test_recorded_point_is_not_a_centre_or_a_scale_correction(self):
        from screen_cell import CaptureSpace,require_cell_center,cell_center
        space=CaptureSpace((211,60,1024,796),(2048,1592),2);point=space.screen_to_pixel([850,328])
        self.assertEqual(point,(1278,536))
        with self.assertRaisesRegex(ValueError,'not an integer lattice centre'):require_cell_center(point,[-482,16],[[80,-80],[40,40]])
        center=cell_center([18,-4],[-482,16],[[80,-80],[40,40]])
        self.assertEqual(center,(1278,576));self.assertEqual(space.pixel_to_screen(center),(850,348))
        self.assertEqual(require_cell_center(center,[-482,16],[[80,-80],[40,40]]),(18,-4))

    def test_exact_roundtrip_preserves_half_points_and_rejects_subpixels(self):
        from screen_cell import CaptureSpace
        space=CaptureSpace((10,20,100,100),(200,200),2)
        self.assertEqual(space.screen_to_pixel([10.5,20.5]),(1,1));self.assertEqual(space.pixel_to_screen([1,1]),(10.5,20.5))
        for p in ([10.25,20],[9,20],[110,20],[True,20],[float('nan'),20]):
            with self.assertRaises(ValueError):space.screen_to_pixel(p)

    def test_unknown_or_unequal_scale_and_singular_lattice_refuse(self):
        from screen_cell import CaptureSpace,require_cell_center
        for pixels,scale in (((2048,1592),None),((2048,796),2),((2048,1592),1)):
            with self.assertRaises(ValueError):CaptureSpace((211,60,1024,796),pixels,scale)
        with self.assertRaises(ValueError):require_cell_center([0,0],[0,0],[[1,2],[2,4]])
        with self.assertRaises(ValueError):CaptureSpace(None,(200,200),2)
        with self.assertRaises(ValueError):require_cell_center([0,0],[0,0],None)

    def test_complete_hover_is_registered_without_any_sprite_identity(self):
        from screen_cell import register_hover_cell
        for scale in (1,2):
            r=register_hover_cell(*self.fixture(scale));self.assertEqual(r['center_physical'],[120*scale,120*scale]);self.assertEqual(r['footprint_pixels'],1600*scale*scale)
            self.assertTrue(r['mouse_is_center']);self.assertFalse(r['source_identity_required']);self.assertFalse(r['absence_of_occlusion_proved'])

    def test_corner_hover_does_not_turn_mouse_point_into_centre(self):
        from screen_cell import register_hover_cell
        args=list(self.fixture());args[4]=[130,120];r=register_hover_cell(*args)
        self.assertFalse(r['mouse_is_center']);self.assertEqual(r['mouse_physical'],[120,100]);self.assertEqual(r['center_physical'],[120,120])

    def test_changed_window_origin_is_refused_not_fitted(self):
        from screen_cell import CaptureSpace,register_hover_cell
        args=list(self.fixture());args[3]=list(args[3]);args[3][1]=CaptureSpace((11,20,400,256),(400,256),1)
        with self.assertRaisesRegex(ValueError,'geometry changed'):register_hover_cell(*args)

    def test_incomplete_or_shifted_hover_never_refits_phase(self):
        from screen_cell import register_hover_cell
        for kind in ('missing','shifted','no_pointer'):
            args=list(self.fixture());args[1]=args[1].copy()
            if kind=='missing':args[1][100,119]=0
            elif kind=='shifted':args[1]=np.roll(args[1],1,axis=1)
            else:args[4]=[20,30]
            with self.subTest(kind=kind),self.assertRaises(ValueError):register_hover_cell(*args)

    def test_control_change_or_failed_cancel_is_unknown(self):
        from screen_cell import register_hover_cell
        for stage in (1,2):
            args=list(self.fixture());args[stage]=args[stage].copy();args[stage][0,0]=1
            with self.subTest(stage=stage),self.assertRaisesRegex(ValueError,'unknown'):register_hover_cell(*args)

    def test_whole_prediction_failures_survive_unidentified_background(self):
        # No requirement that this authored before-background matches any asset.
        from screen_cell import register_hover_cell
        args=self.fixture();self.assertEqual(register_hover_cell(*args)['relative_screen_cell'],[0,0])
        source=np.full((40,78,3),[248,120,8],np.uint8);opaque=np.ones((40,78),bool)
        after=np.repeat(np.repeat(source,2,0),2,1)
        good=compare_nearest_scaled_rgb(after,source,opaque)
        self.assertTrue(good.physical.normalized_exact)
        after[1,1,0]=0;failed=compare_nearest_scaled_rgb(after,source,opaque)
        self.assertFalse(failed.physical.normalized_exact);self.assertEqual(failed.physical.color_samples,40*78*4*3)


    def prediction_fixture(self):
        from screen_cell import CaptureSpace
        space=CaptureSpace((0,0,200,100),(400,200),2);before=np.full((200,400,3),[13,37,59],np.uint8)
        source=np.zeros((40,78,4),np.uint8);source[20,38:40]=[8,120,248,255]
        after=before.copy();after[100:102,198:202]=[248,120,8]
        return before,after,source,[200,100],space,[200,100],[[80,-80],[40,40]],[0,0,400,200]

    def test_whole_source_and_simple_overlay_are_distinct_without_background_match(self):
        from screen_cell import compare_fixed_sprite
        args=list(self.prediction_fixture());r=compare_fixed_sprite(*args)
        self.assertEqual(r['opaque_prediction']['denominator'],8);self.assertTrue(r['opaque_prediction']['passed'])
        self.assertEqual(r['source_over_before_canvas_prediction']['denominator'],12480);self.assertTrue(r['source_over_before_canvas_prediction']['passed'])
        args[1]=args[1].copy();args[1][60,122]=0;r=compare_fixed_sprite(*args)
        self.assertTrue(r['opaque_prediction']['passed']);self.assertFalse(r['source_over_before_canvas_prediction']['passed'])
        args[1][101,201]=0;r=compare_fixed_sprite(*args)
        self.assertEqual(r['opaque_prediction']['different_pixels'],1);self.assertFalse(r['opaque_prediction']['passed'])
        self.assertTrue(r['comparison_failure_retained']);self.assertFalse(r['background_source_match_required'])
        self.assertEqual(r['opaque_mismatch_positions_local'],[[41,79]])

    def test_unknown_sprite_alpha_and_illegal_centre_refuse_without_colour_fitting(self):
        from screen_cell import compare_fixed_sprite
        args=list(self.prediction_fixture());args[2]=args[2].copy();args[2][20,38]=[4,60,124,128]
        with self.assertRaisesRegex(ValueError,'formula is unknown'):compare_fixed_sprite(*args)
        args=list(self.prediction_fixture());args[3]=[200,80]
        with self.assertRaisesRegex(ValueError,'not an integer lattice centre'):compare_fixed_sprite(*args)


class FinalClickBindingTests(unittest.TestCase):
    def fixture(self,scale=1):
        import copy
        args=ScreenCellRegistrationTests().fixture(scale)
        before,hover,cancelled,spaces,point,phase,basis,cells,probe,world=args;space=spaces[0]
        owner={'pid':123,'uid':501,'birth_seconds':100,'birth_micros':20,'executable':'/authored/fixture'}
        frozen={'id':'authored-frozen-trial','owner':owner,'window':7,'frame':[10,20,400,256],
                'capture_pixels':list(space.capture_pixels),'scale':scale,'camera_epoch':3,'pointer_epoch':4,
                'clock_id':'authored-process-monotonic-clock','cell':[0,0],'center':[120*scale,120*scale],
                'click_global':point,'phase':phase,'basis':basis,'probe_bounds':probe,'world_bounds':world}
        contexts=[]
        for stage,t,serial in zip(('before','hover','cancelled','final_hover','dispatch'),
                                  (1000000000,1100000000,1200000000,1300000000,1400000000),(10,11,12,13,13)):
            context={k:copy.deepcopy(frozen[k])for k in ('owner','window','frame','capture_pixels','scale','camera_epoch','pointer_epoch','clock_id')}
            context.update(stage=stage,freeze_id=frozen['id'],cursor_global=point.copy(),
                           identity_frame_front_verified=True,monotonic_ns=t,event_serial=serial);contexts.append(context)
        events=[{'kind':kind,'serial':11+i,'monotonic_ns':1050000000+100000000*i,
                 'pointer_epoch':4,'camera_epoch':3,'cursor_global':point.copy()}
                for i,kind in enumerate(('select_road_tool','cancel_tool','select_road_tool'))]
        journal={'complete':True,'clock_id':frozen['clock_id'],'freeze_id':frozen['id'],'events':events}
        return [before,hover,cancelled,hover.copy(),frozen,contexts,journal]

    def test_exact_point_same_cell_after_cancel_and_reselect_passes_without_background(self):
        from screen_cell import validate_final_click
        for scale in (1,2):
            result=validate_final_click(*self.fixture(scale))
            self.assertTrue(result['mouse_is_center']);self.assertTrue(result['frozen_cell_confirmed'])
            self.assertTrue(result['cancel_reselect_point_unchanged']);self.assertFalse(result['input_sent'])
            self.assertFalse(result['reusable_click_permit']);self.assertFalse(result['background_source_match_required'])

    def test_wrong_observed_point_is_not_replaced_by_a_calculated_centre(self):
        from screen_cell import validate_final_click
        for stage in (1,2,3,4):
            args=self.fixture();args[5][stage]['cursor_global']=[130,120]
            with self.subTest(stage=stage),self.assertRaisesRegex(ValueError,'pointer differs'):
                validate_final_click(*args)
        args=self.fixture();args[4]['click_global']=[130,120]
        with self.assertRaisesRegex(ValueError,'click is not'):validate_final_click(*args)

    def test_wrong_frozen_cell_and_wrong_final_footprint_are_refused(self):
        from screen_cell import validate_final_click
        args=self.fixture();args[4]['cell']=[1,0]
        with self.assertRaisesRegex(ValueError,'cell/centre'):validate_final_click(*args)
        args=self.fixture();args[3]=np.roll(args[3],40,axis=1)
        with self.assertRaises(ValueError):validate_final_click(*args)

    def test_expired_registration_and_clock_or_event_reuse_are_refused(self):
        from screen_cell import validate_final_click
        for kind in ('old_final','old_registration','unordered','clock','serial'):
            args=self.fixture();contexts=args[5]
            if kind=='old_final':contexts[4]['monotonic_ns']=contexts[3]['monotonic_ns']+250000001
            elif kind=='old_registration':contexts[3]['monotonic_ns']=31000000000;contexts[4]['monotonic_ns']=31100000001
            elif kind=='unordered':contexts[4]['monotonic_ns']=contexts[3]['monotonic_ns']
            elif kind=='clock':contexts[4]['clock_id']='other-process-clock'
            else:contexts[4]['event_serial']+=1
            with self.subTest(kind=kind),self.assertRaises(ValueError):validate_final_click(*args)
        args=self.fixture();args[5][4]['monotonic_ns']=args[5][3]['monotonic_ns']+250000000
        self.assertTrue(validate_final_click(*args)['passed'])

    def test_missing_evidence_never_defaults_to_a_click_confirmation(self):
        from screen_cell import validate_final_click
        for target,key in ((4,'pointer_epoch'),(4,'cell'),(4,'clock_id'),(5,'identity_frame_front_verified'),(6,'complete'),(6,'events')):
            args=self.fixture();record=args[target][4]if target==5 else args[target];del record[key]
            with self.subTest(target=target,key=key),self.assertRaises(ValueError):validate_final_click(*args)
        args=self.fixture();args[5]=[]
        with self.assertRaises(ValueError):validate_final_click(*args)

    def test_window_scale_camera_and_pointer_movement_invalidate_confirmation(self):
        from screen_cell import validate_final_click
        for key,value in (('window',8),('scale',2),('frame',[11,20,400,256]),('camera_epoch',4),('pointer_epoch',5),('owner',{}),('identity_frame_front_verified',False)):
            args=self.fixture();args[5][4][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):validate_final_click(*args)
        # Moving away and returning is still a different pointer epoch.
        args=self.fixture();args[6]['events'][2]['pointer_epoch']=6
        with self.assertRaises(ValueError):validate_final_click(*args)

    def test_cancel_or_journal_failure_and_reselection_movement_are_refused(self):
        from screen_cell import validate_final_click
        for kind in ('cancel','final_controls','incomplete','extra_action','reselect_point','event_time'):
            args=self.fixture()
            if kind=='cancel':args[2]=args[2].copy();args[2][0,0]=1
            elif kind=='final_controls':args[3][0,0]=1
            elif kind=='incomplete':args[6]['complete']=False
            elif kind=='extra_action':args[6]['events'].append(dict(args[6]['events'][-1]))
            elif kind=='reselect_point':args[6]['events'][2]['cursor_global']=[131,140]
            else:args[6]['events'][2]['monotonic_ns']=args[5][3]['monotonic_ns']+1
            with self.subTest(kind=kind),self.assertRaises(ValueError):validate_final_click(*args)


if __name__=='__main__':unittest.main()
