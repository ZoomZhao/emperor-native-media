"""Independent comparison primitives; no assets, camera or MAP selection rules.

Scientific callers supply arrays and measured correspondences. No file lookup,
UI, game execution or artwork export is performed here.
"""
from dataclasses import dataclass
import math
import numpy as np


@dataclass(frozen=True)
class PixelComparison:
    opaque_pixels: int
    color_samples: int
    normalized_equal_samples: int
    normalized_rmse: float
    normalized_max_error: int
    raw_rmse: float
    raw_max_error: int

    @property
    def normalized_exact(self):
        return self.color_samples > 0 and self.normalized_equal_samples == self.color_samples


def compare_rgb(reference, decoded, opaque_mask, bits=5):
    """Exact integer verification after template search, plus full8-bit errors.

    Quantized equality is a separate result; it never implies raw pixel equality,
    an archive selector, a MAP rule or a whole-scene rendering pass.
    """
    reference, decoded, opaque_mask = map(np.asarray, (reference, decoded, opaque_mask))
    if (reference.dtype != np.uint8 or decoded.dtype != np.uint8
            or reference.ndim != 3 or reference.shape[-1] != 3
            or reference.shape != decoded.shape or opaque_mask.shape != reference.shape[:2]
            or opaque_mask.dtype != np.bool_ or not 1 <= bits <= 8):
        raise ValueError("uint8 RGB arrays, matching bool mask and1..8 bits required")
    count = int(opaque_mask.sum())
    if not count:
        raise ValueError("nonempty opaque mask required")
    if reference.shape[0] > 4096 or reference.shape[1] > 4096:
        raise ValueError("image budget")
    r, d = reference[opaque_mask].astype(np.int16), decoded[opaque_mask].astype(np.int16)
    raw = r - d
    normalized = (r >> (8-bits)) - (d >> (8-bits))
    return PixelComparison(count, count*3, int((normalized == 0).sum()),
                           float(np.sqrt(np.mean(normalized.astype(float)**2))),
                           int(np.abs(normalized).max()),
                           float(np.sqrt(np.mean(raw.astype(float)**2))), int(np.abs(raw).max()))


def diagonal_grid_matrix(scale_u, scale_v, tx, ty):
    values = (scale_u, scale_v, tx, ty)
    if not all(math.isfinite(v) for v in values) or scale_u <= 0 or scale_v <= 0:
        raise ValueError("finite positive scales required")
    return np.array([[scale_u, -scale_u, tx], [scale_v, scale_v, ty]], dtype=float)


def project_points(points, matrix):
    points, matrix = np.asarray(points, float), np.asarray(matrix, float)
    if (points.ndim != 2 or points.shape[1] != 2 or matrix.shape != (2,3)
            or not np.isfinite(points).all() or not np.isfinite(matrix).all()):
        raise ValueError("finite2D points and2x3 matrix required")
    return points @ matrix[:, :2].T + matrix[:,2]


def landmark_residuals(source, measured, matrix):
    """Matched-point residuals only, not nearest-edge distances or fit confidence."""
    projected = project_points(source, matrix)
    measured = np.asarray(measured, float)
    if measured.shape != projected.shape or not len(measured) or not np.isfinite(measured).all():
        raise ValueError("nonempty matching measured point set required")
    distances = np.linalg.norm(projected-measured, axis=1)
    return {"landmarks":len(distances),"RMS_pixels":float(np.sqrt(np.mean(distances**2))),
            "max_pixels":float(distances.max())}


def nearest_mask_distances(projected_points, target_mask):
    """Nearest target-pixel distances; unobserved/outside points remain null.

    This metric is intentionally distinct from unique matched-landmark residuals.
    """
    import cv2
    points, mask = np.asarray(projected_points, float), np.asarray(target_mask)
    if (points.ndim != 2 or points.shape[1] != 2 or not np.isfinite(points).all()
            or mask.ndim != 2 or mask.dtype != np.bool_ or not mask.any()):
        raise ValueError("finite points and nonempty2D bool mask required")
    if max(mask.shape)>4096:
        raise ValueError("mask budget")
    distance = cv2.distanceTransform((~mask).astype(np.uint8),cv2.DIST_L2,cv2.DIST_MASK_PRECISE)
    result=[]
    for x,y in points:
        # Round nonnegative coordinates to nearest raster point; never clip a
        # point outside the image into an artificial successful correspondence.
        if x < 0 or y < 0 or x >= mask.shape[1] or y >= mask.shape[0]:
            result.append(None);continue
        ix,iy=int(math.floor(x+.5)),int(math.floor(y+.5))
        result.append(float(distance[iy,ix]) if ix < mask.shape[1] and iy < mask.shape[0] else None)
    return result
