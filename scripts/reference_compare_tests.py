"""Entirely authored fixtures for geometry and quantized-vs-raw comparison."""
import unittest
import numpy as np
from reference_compare import (compare_rgb, compare_nearest_scaled_rgb, diagonal_grid_matrix,
                               project_points, landmark_residuals, nearest_mask_distances,
                               mask_translation_candidates)


class ReferenceComparisonTests(unittest.TestCase):
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


if __name__=='__main__':unittest.main()
