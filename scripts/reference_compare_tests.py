"""Entirely authored fixtures for geometry and quantized-vs-raw comparison."""
import unittest
import numpy as np
from reference_compare import compare_rgb, diagonal_grid_matrix, project_points, landmark_residuals, nearest_mask_distances


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


if __name__=='__main__':unittest.main()
