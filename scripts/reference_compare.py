"""Independent comparison primitives; no assets, camera or MAP selection rules.

Scientific callers supply arrays and measured correspondences. No file lookup,
UI, game execution or artwork export is performed here.
"""
from dataclasses import dataclass
from collections import Counter
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


@dataclass(frozen=True)
class ScaledPixelComparison:
    logical: PixelComparison
    physical: PixelComparison
    raw_pixels_equal_to_block_representative: int

    @property
    def raw_nearest_replication(self):
        return self.raw_pixels_equal_to_block_representative == self.physical.opaque_pixels


@dataclass(frozen=True)
class MaskTranslationCandidate:
    orientation_index: int
    offset_xy: tuple
    heldout_matches: int
    heldout_total: int

    @property
    def heldout_passed(self):
        return self.heldout_matches == self.heldout_total


@dataclass(frozen=True)
class FixedCandidateSupport:
    candidate_index: int
    training_matches: int
    training_total: int
    heldout_matches: int
    heldout_total: int

    @property
    def training_passed(self):
        return self.training_total > 0 and self.training_matches == self.training_total

    @property
    def heldout_passed(self):
        return self.heldout_total > 0 and self.heldout_matches == self.heldout_total


@dataclass(frozen=True)
class CellTintComparison:
    mask_pixels: int
    preview_changed_pixels: int
    red_pixels: int
    green_pixels: int
    neutral_pixels: int
    cancel_changed_pixels: int


def compare_cell_tints(before, preview, after, cell_masks, margin=8):
    """Count tint evidence on caller-supplied masks, including neutral pixels.

    No outline is inferred from changed pixels: a tinted pixel can already have
    the same raw colour. Masks must be distinct/nonoverlapping. The counts do not
    establish road meaning, a MAP coordinate, a selector or an acceptance rule.
    """
    before,preview,after=map(np.asarray,(before,preview,after))
    masks=np.asarray(cell_masks)
    if (before.ndim!=3 or before.shape[-1]!=3 or not all(before.shape)
            or max(before.shape[:2])>4096 or before.shape!=preview.shape or before.shape!=after.shape
            or any(a.dtype!=np.uint8 for a in (before,preview,after))
            or masks.ndim!=3 or masks.dtype!=np.bool_ or masks.shape[1:]!=before.shape[:2]
            or not 1<=len(masks)<=128 or masks.size>8_000_000
            or not isinstance(margin,(int,np.integer)) or isinstance(margin,(bool,np.bool_))
            or not 0<=margin<=255):
        raise ValueError("bounded uint8 RGB frames, bool cell masks and integer margin0..255 required")
    if not np.all(masks.any(axis=(1,2))) or np.any(masks.sum(axis=0,dtype=np.uint16)>1):
        raise ValueError("nonempty nonoverlapping cell masks required")
    changed=np.any(before!=preview,axis=2);cancel=np.any(before!=after,axis=2)
    rgb=preview.astype(np.int16)
    red=(rgb[:,:,0]>rgb[:,:,1]+margin)&(rgb[:,:,0]>rgb[:,:,2]+margin)
    green=(rgb[:,:,1]>rgb[:,:,0]+margin)&(rgb[:,:,1]>rgb[:,:,2]+margin)
    return tuple(CellTintComparison(int(m.sum()),int(changed[m].sum()),int(red[m].sum()),
                                    int(green[m].sum()),int((~red&~green)[m].sum()),int(cancel[m].sum()))
                 for m in masks)


# All square-grid rotations/reflections. No game-camera orientation is selected.
GRID_ORIENTATIONS = (
    ((1,0),(0,1)), ((0,-1),(1,0)), ((-1,0),(0,-1)), ((0,1),(-1,0)),
    ((-1,0),(0,1)), ((1,0),(0,-1)), ((0,1),(1,0)), ((0,-1),(-1,0)),
)


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


def isometric_subbase_centres(canvas_width, canvas_height):
    """Public Emperor base-layout coordinates, not an original MAP placement.

    Callers must independently verify type30 and uncompressedLength=n*n*3200.
    Each tuple is(squareX,squareY,sourceCenterX,sourceCenterY), in encoded row
    order. No image lookup, candidate selection or pixel-offset search occurs.
    """
    if any(not isinstance(v,(int,np.integer)) or isinstance(v,(bool,np.bool_))
           or not 1 <= v <= 4096 for v in (canvas_width,canvas_height)):
        raise ValueError("bounded integer canvas dimensions required")
    n=(int(canvas_width)+2)//80
    if not 1 <= n <= 16 or canvas_width!=80*n-2 or canvas_height<40*n:
        raise ValueError("declared Emperor base dimensions required")
    padding=int(canvas_height)-40*n
    return tuple((x,row-x,(n-1+x-(row-x))*40+39,padding+row*20+20)
                 for row in range(2*n-1)
                 for x in range(max(0,row-n+1),min(n-1,row)+1))


def bottom_base_canvas_bounds(center_xy, canvas_size, base_size, scale=2):
    """Integer sampling rectangle for an explicitly bottom-aligned base.

    Sizes are logical(width,height); centre is in physical reference pixels.
    Callers choose this decoded-geometry convention. It is not an original MAP
    draw-anchor rule, and animation offsets are never used as static origins.
    """
    sizes = (*canvas_size, *base_size)
    if (len(canvas_size) != 2 or len(base_size) != 2
            or any(not isinstance(v,(int,np.integer)) or isinstance(v,(bool,np.bool_))
                   or not 1 <= v <= 4096 for v in sizes)
            or not isinstance(scale,(int,np.integer)) or isinstance(scale,(bool,np.bool_))
            or not 1 <= scale <= 8 or len(center_xy) != 2):
        raise ValueError("positive integer sizes, finite centre and integer scale1..8 required")
    w,h,bw,bh = sizes
    if bw > w or bh > h or max(w,h)*scale > 4096:
        raise ValueError("base must fit the bounded canvas")
    cx,cy = map(float,center_xy)
    if not math.isfinite(cx) or not math.isfinite(cy):
        raise ValueError("finite centre required")
    bounds = (cx-w*scale/2, cy-(h-bh/2)*scale,
              cx+w*scale/2, cy+bh*scale/2)
    if any(not v.is_integer() for v in bounds):
        raise ValueError("sampling bounds must lie on integer physical pixels")
    return tuple(map(int,bounds))


def compare_nearest_scaled_rgb(reference, decoded, opaque_mask, scale=2, bits=5):
    """Report logical and physical denominators separately, checking every pixel.

    The logical comparison uses one block representative. Physical equality and
    raw block replication are independent checks; neither is inferred from it.
    """
    reference, decoded, mask = map(np.asarray, (reference, decoded, opaque_mask))
    if (not isinstance(scale, (int,np.integer)) or isinstance(scale, (bool,np.bool_))
            or not 1 <= scale <= 8 or decoded.ndim != 3 or decoded.shape[-1] != 3
            or reference.shape != (decoded.shape[0]*scale, decoded.shape[1]*scale, 3)
            or max(reference.shape[:2]) > 4096):
        raise ValueError("matching nearest-scale image dimensions and integer scale1..8 required")
    logical = compare_rgb(reference[::scale,::scale], decoded, mask, bits)
    expanded_mask = np.repeat(np.repeat(mask,scale,axis=0),scale,axis=1)
    expanded = np.repeat(np.repeat(decoded,scale,axis=0),scale,axis=1)
    physical = compare_rgb(reference, expanded, expanded_mask, bits)
    representative = np.repeat(np.repeat(reference[::scale,::scale],scale,axis=0),scale,axis=1)
    repeated = int(np.all(reference == representative,axis=2)[expanded_mask].sum())
    return ScaledPixelComparison(logical,physical,repeated)


def mask_translation_candidates(mask, relative_offsets, training_indices, heldout_indices,
                                orientations=GRID_ORIENTATIONS):
    """Exhaustively retain every training fit and separately count held-out hits.

    Supplied observations must all refer to true mask cells. This conditional
    search neither establishes that semantic assumption nor picks a camera,
    sprite ID, preferred orientation or winner among ambiguous translations.
    """
    mask, offsets = np.asarray(mask), np.asarray(relative_offsets)
    if (mask.ndim != 2 or mask.dtype != np.bool_ or not all(mask.shape)
            or max(mask.shape)>512 or offsets.ndim!=2 or offsets.shape[1]!=2
            or not 2<=len(offsets)<=4096 or offsets.dtype.kind not in 'iu'
            or np.any(offsets>512) or np.any(offsets < -512)):
        raise ValueError("bounded bool mask and integer offsets required")
    offsets=offsets.astype(np.int64)
    if len(np.unique(offsets,axis=0))!=len(offsets):
        raise ValueError("duplicate observations inflate the evidence count")
    splits=[]
    for indices in (training_indices,heldout_indices):
        indices=np.asarray(indices)
        if (indices.ndim!=1 or indices.dtype.kind not in 'iu' or not len(indices)
                or np.any(indices>=len(offsets)) or np.any(indices<0)
                or len(np.unique(indices))!=len(indices)):
            raise ValueError("nonempty unique observation indices required")
        splits.append(indices.astype(np.int64))
    train,hold=splits
    if set(train)&set(hold) or len(train)+len(hold)!=len(offsets):
        raise ValueError("disjoint training/heldout partition required")
    matrices=np.asarray(orientations)
    if (matrices.ndim!=3 or matrices.shape[1:]!=(2,2) or not 1<=len(matrices)<=8
            or matrices.dtype.kind not in 'iu' or np.any(matrices>1) or np.any(matrices< -1)
            or not np.all(np.abs(matrices).sum(axis=1)==1)
            or not np.all(np.abs(matrices).sum(axis=2)==1)
            or len(np.unique(matrices.reshape(-1,4),axis=0))!=len(matrices)):
        raise ValueError("unique square-grid signed-permutation orientations required")
    if int(mask.sum())*len(offsets)*len(matrices)>8_000_000:
        raise ValueError("candidate-operation budget")
    ys,xs=np.where(mask);anchor_locations=np.column_stack((xs,ys))
    candidates=[]
    for index,matrix in enumerate(matrices.astype(np.int64)):
        relative=offsets@matrix.T
        # Enumerate from a TRAINING observation, even when relative origin is
        # held out or outside the target mask. It must not leak holdout truth.
        origins=anchor_locations-relative[train[0]]
        x=origins[:,0,None]+relative[None,:,0]
        y=origins[:,1,None]+relative[None,:,1]
        inside=(x>=0)&(y>=0)&(x<mask.shape[1])&(y<mask.shape[0])
        # Lookup only in-bounds coordinates. No clipping into a successful cell.
        hits=np.zeros(inside.shape,bool)
        hits[inside]=mask[y[inside],x[inside]]
        fitting=np.flatnonzero(hits[:,train].all(axis=1))
        for row in fitting:
            candidates.append(MaskTranslationCandidate(index,tuple(map(int,origins[row])),
                                                       int(hits[row,hold].sum()),len(hold)))
    return tuple(candidates)


def check_fixed_candidates(mask, candidate_coordinates, training_indices, heldout_indices):
    """Check supplied feature cells without refitting any candidate.

    Coordinates have shape(candidates, observations, xy). Rows stay in caller
    order, including failures; held-out success cannot override failed training.
    The caller owns the feature/field assumption and any acceptance decision.
    """
    mask, coords=np.asarray(mask),np.asarray(candidate_coordinates)
    if (mask.ndim!=2 or mask.dtype!=np.bool_ or not all(mask.shape) or max(mask.shape)>512
            or coords.ndim!=3 or coords.shape[2]!=2 or not 1<=coords.shape[0]<=4096
            or not 2<=coords.shape[1]<=4096 or coords.dtype.kind not in 'iu'
            or np.any(coords>8192) or np.any(coords < -8192)):
        raise ValueError("bounded bool mask and fixed integer candidate coordinates required")
    if coords.shape[0]*coords.shape[1]>8_000_000:
        raise ValueError("fixed-candidate operation budget")
    if any(len(np.unique(row,axis=0))!=coords.shape[1] for row in coords):
        raise ValueError("duplicate feature coordinates inflate the evidence count")
    splits=[]
    for indices in (training_indices,heldout_indices):
        indices=np.asarray(indices)
        if (indices.ndim!=1 or indices.dtype.kind not in 'iu' or not len(indices)
                or np.any(indices>=coords.shape[1]) or np.any(indices<0)
                or len(np.unique(indices))!=len(indices)):
            raise ValueError("nonempty unique feature indices required")
        splits.append(indices.astype(np.int64))
    train,hold=splits
    if set(train)&set(hold) or len(train)+len(hold)!=coords.shape[1]:
        raise ValueError("disjoint training/heldout partition required")
    coords=coords.astype(np.int64)
    x,y=coords[:,:,0],coords[:,:,1]
    inside=(x>=0)&(y>=0)&(x<mask.shape[1])&(y<mask.shape[0])
    hits=np.zeros(inside.shape,bool)
    hits[inside]=mask[y[inside],x[inside]]
    return tuple(FixedCandidateSupport(i,int(row[train].sum()),len(train),
                                      int(row[hold].sum()),len(hold))
                 for i,row in enumerate(hits))


def audit_static_labels(keys, labels, training_indices, heldout_indices):
    """Falsify a single label per supplied static key; never return a selector.

    Reports are conditional on caller-supplied coordinates/features/observations.
    Consistency cannot prove a game rule. Unseen keys and ambiguous training keys
    remain explicit instead of being memorized or treated as successful tests.
    """
    keys,labels=list(keys),list(labels)
    if not 2<=len(keys)<=4096 or len(keys)!=len(labels):
        raise ValueError("matching bounded categorical observations required")

    def category(value,depth=0):
        if isinstance(value,(int,np.integer)) and not isinstance(value,(bool,np.bool_)):
            value=int(value)
            if -(2**63)<=value<2**63:return value
        if isinstance(value,str) and 1<=len(value)<=256:return value
        if isinstance(value,tuple) and depth<3 and 1<=len(value)<=32:
            return tuple(category(v,depth+1)for v in value)
        raise ValueError("known bounded integer/string/tuple categories required")

    keys=[category(k)for k in keys];labels=[category(v)for v in labels]
    partitions=[]
    for indices in (training_indices,heldout_indices):
        indices=np.asarray(indices)
        if (indices.ndim!=1 or indices.dtype.kind not in 'iu' or not len(indices)
                or np.any(indices<0) or np.any(indices>=len(keys))
                or len(np.unique(indices))!=len(indices)):
            raise ValueError("nonempty unique categorical indices required")
        partitions.append(indices.astype(np.int64))
    train,hold=partitions
    if set(train)&set(hold) or len(train)+len(hold)!=len(keys):
        raise ValueError("disjoint categorical training/heldout partition required")

    def groups(indices):
        result={}
        for i in indices:result.setdefault(keys[i],Counter())[labels[i]]+=1
        return result

    def summarize(group):
        return {'distinct_keys':len(group),'ambiguous_keys':sum(len(v)>1 for v in group.values()),
                'max_distinct_labels_per_key':max(map(len,group.values())),
                'minimum_single_label_disagreements':sum(sum(v.values())-max(v.values())for v in group.values())}

    tg,hg=groups(train),groups(hold)
    result={'training_observations':len(train),'heldout_observations':len(hold),
            'training':summarize(tg),'heldout':summarize(hg),
            'joint':summarize(groups(range(len(keys)))),
            'heldout_unseen_keys':0,'heldout_with_ambiguous_training_key':0,
            'heldout_with_unique_training_key':0,'heldout_unique_key_disagreements':0,
            'heldout_new_labels_for_seen_key':0,'first_training_counterexample_indices':None}
    first={}
    for i in train:
        prior=first.setdefault(keys[i],int(i))
        if labels[prior]!=labels[i] and result['first_training_counterexample_indices'] is None:
            result['first_training_counterexample_indices']=[prior,int(i)]
    for i in hold:
        if keys[i] not in tg:
            result['heldout_unseen_keys']+=1;continue
        prior=tg[keys[i]]
        if len(prior)>1:result['heldout_with_ambiguous_training_key']+=1
        else:
            result['heldout_with_unique_training_key']+=1
            result['heldout_unique_key_disagreements']+=int(labels[i] not in prior)
        result['heldout_new_labels_for_seen_key']+=int(labels[i] not in prior)
    return result


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
