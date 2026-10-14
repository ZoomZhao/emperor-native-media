"""Entirely authored fixtures for geometry and quantized-vs-raw comparison."""
import unittest
import numpy as np
from reference_compare import (compare_rgb, compare_nearest_scaled_rgb, bottom_base_canvas_bounds, isometric_subbase_centres, diagonal_grid_matrix,
                               project_points, landmark_residuals, nearest_mask_distances,
                               mask_translation_candidates,check_fixed_candidates,audit_static_labels,compare_cell_tints)


class ReferenceComparisonTests(unittest.TestCase):
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


if __name__=='__main__':unittest.main()
