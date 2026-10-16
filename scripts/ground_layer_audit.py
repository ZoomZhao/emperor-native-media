"""Read-only bounded ground discovery and conditional scene-edit replay.

Explicit private manifest; no asset selector, scene renderer or input automation.
Published SG3 base/RLE bytes and complete source alpha only. All image inputs,
profiles, caches and output evidence stay outside Git. Geometry is frozen below;
this is spatially separated discovery validation, not a blind engine-rule test.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import struct
import subprocess
import tempfile

ARCHIVE = 'DATA/China_Terrain.sg3'
CENTRES = ((718, 1256), (878, 1256), (798, 1296))
OFFSETS = ((-80, -40), (80, -40), (-80, 40), (80, 40),
           (0, 80), (-160, 0), (160, 0), (0, -80))
CONTEXT = (620, 1120, 980, 1360)
VALIDATION = (700, 1120, 900, 1280)
ROAD = (720, 1176, 876, 1256)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def decode_components(record, pixels):
    """Only the declared internal, nonmirrored type30 78-wide base/RLE subset."""
    import numpy as np
    from base_content_audit import diamond_mask
    require(len(record) == 64, 'SG3 record extent')
    offset, length, raw_length = struct.unpack_from('<III', record)
    width, height = struct.unpack_from('<hh', record, 20)
    require(width == 78 and 40 <= height <= 59 and raw_length == 3200
            and record[50] == 30 and record[51] == 0 and record[52] == 0
            and struct.unpack_from('<i', record, 16)[0] == 0,
            'unsupported ground record')
    require(length >= raw_length and offset <= len(pixels)
            and length <= len(pixels) - offset, 'SG3 pixel extent')

    def bgra(words):
        words = np.asarray(words, np.uint16)
        q = np.stack((words & 31, (words >> 5) & 31, (words >> 10) & 31), -1).astype(np.uint8)
        result = np.empty((*words.shape, 4), np.uint8)
        result[..., :3] = (q << 3) | (q >> 2)
        result[..., 3] = 255
        return result

    base = np.zeros((40, 78, 4), np.uint8)
    base[diamond_mask()] = bgra(np.frombuffer(pixels, '<u2', 1600, offset))
    top = np.zeros((height * width, 4), np.uint8)
    stream = pixels[offset + raw_length:offset + length]
    i = j = 0
    while i < len(stream):
        run = stream[i]
        i += 1
        if run == 255:
            require(i < len(stream), 'truncated RLE skip')
            j += stream[i]
            i += 1
        else:
            require(i + run * 2 <= len(stream) and j + run <= len(top), 'RLE literal extent')
            top[j:j + run] = bgra(np.frombuffer(stream, '<u2', run, i))
            i += run * 2
            j += run
        require(j <= len(top), 'RLE canvas extent')
    top = top.reshape(height, width, 4)
    full = np.zeros_like(top)
    full[-40:] = base
    full[top[..., 3] == 255] = top[top[..., 3] == 255]
    require(not np.any((top[-40:, :, 3] == 255) & diamond_mask()), 'base/top overlap')
    return base, top, full


def place_mask(shape, mask, centre):
    import numpy as np
    from reference_compare import bottom_base_canvas_bounds
    h, w = mask.shape
    bounds = bottom_base_canvas_bounds(centre, (w, h), (78, 40), 2)
    x0, y0, x1, y1 = bounds
    require(0 <= x0 < x1 <= shape[1] and 0 <= y0 < y1 <= shape[0], 'full bounds; no clipping')
    result = np.zeros(shape, bool)
    result[y0:y1, x0:x1] = mask.repeat(2, 0).repeat(2, 1)
    return result, bounds


def rectangle(shape, bounds):
    import numpy as np
    x0, y0, x1, y1 = bounds
    require(0 <= x0 < x1 <= shape[1] and 0 <= y0 < y1 <= shape[0], 'rectangle extent')
    result = np.zeros(shape, bool)
    result[y0:y1, x0:x1] = True
    return result


def exposed_geometry(shape, union, public, centre):
    """Source-alpha geometry only; never consult target colours or changes."""
    import numpy as np
    other = np.zeros(shape, bool)
    for dx, dy in OFFSETS:
        support, _ = place_mask(shape, union, (centre[0] + dx, centre[1] + dy))
        other |= support
    base, _ = place_mask(shape, public, centre)
    exposed = ~rectangle(shape, VALIDATION) & ~other
    identification = base & exposed
    require(np.any(identification), 'zero-information identification')
    return identification, exposed


def score(rgb, actual, mask):
    import numpy as np
    require(rgb.shape == actual.shape and mask.shape == rgb.shape[:2], 'score shapes')
    bad = np.any((rgb >> 3) != (actual >> 3), 2) & mask
    rawbad = np.any(rgb != actual, 2) & mask
    delta = np.abs(rgb.astype(np.int16) - actual.astype(np.int16)).max(2)
    return {'pixels': int(mask.sum()), 'RGB5_equal': int(mask.sum() - bad.sum()),
            'RGB8_equal': int(mask.sum() - rawbad.sum()), 'RGB5_errors': int(bad.sum()),
            'small_raw_errors': int(np.count_nonzero(bad & (delta <= 1))),
            'max_raw_error': int(delta[bad].max()) if bad.any() else 0}


def identify(frame, fulls, centre, identification, exposed):
    """Every complete exposed paint is scored; minima never become hits."""
    rows = []
    for image_id, full in fulls.items():
        paint, bounds = place_mask(frame.shape[:2], full[..., 3] == 255, centre)
        x0, y0, x1, y1 = bounds
        local = paint[y0:y1, x0:x1] & exposed[y0:y1, x0:x1]
        require(not (identification & ~paint).any(), 'public base missing source paint')
        rgb = full[..., 2::-1].repeat(2, 0).repeat(2, 1)
        measured = score(rgb, frame[y0:y1, x0:x1], local)
        measured['ID'] = image_id
        measured['exact'] = measured['pixels'] > 0 and measured['RGB5_errors'] == 0
        rows.append(measured)
    return rows


def centre_removal_necessary(pre, post, control, domain, fulls, tops, nonpaint, outside, geometry_out=None):
    """Pre-state identity before a removal geometry test; never erase from post.

    Own public base and own top are structurally disjoint. A geometric success
    cannot select among base aliases. Unknown reveal sources produce no pixels.
    """
    import numpy as np
    from base_content_audit import diamond_mask
    shape = pre.shape[:2]
    centre = (798, 1216)
    union = np.zeros((59, 78), bool)
    for image_id in domain:
        full = fulls[image_id]
        union[-len(full):] |= full[..., 3] == 255
    other = np.zeros(shape, bool)
    for dx, dy in OFFSETS:
        support, _ = place_mask(shape, union, (centre[0] + dx, centre[1] + dy))
        other |= support
    public, _ = place_mask(shape, diamond_mask(), centre)
    identification = public & ~other
    require(identification.any(), 'centre identification has no information')
    # Identity reads pre only. Every candidate paints the same complete I region.
    rows = identify(pre, {i: fulls[i] for i in domain}, centre, identification, identification)
    exact = [r['ID'] for r in rows if r['exact']]
    branches = []
    for image_id in domain:
        support, bounds = place_mask(shape, tops[image_id][..., 3] == 255, centre)
        require(not np.any(support & identification), 'centre top overlaps identification')
        if image_id not in exact:
            continue
        x0, y0, x1, y1 = bounds
        local = support[y0:y1, x0:x1]
        rgb = tops[image_id][..., 2::-1].repeat(2, 0).repeat(2, 1)
        reach_N, reach_O = int(np.count_nonzero(support & nonpaint)), int(np.count_nonzero(support & outside))
        branches.append({'ID': image_id, 'bounds': bounds, 'top_pixels': int(support.sum()),
                         'has_top_information': bool(support.any()),
                         'nonpaint_reach': [reach_N, int(nonpaint.sum())],
                         'outside_reach': [reach_O, int(outside.sum())],
                         'geometry_necessary_pass': reach_N == int(nonpaint.sum()) and reach_O == int(outside.sum()),
                         'whole_top_pre': score(rgb, pre[y0:y1, x0:x1], local),
                         'whole_top_post': score(rgb, post[y0:y1, x0:x1], local)})
    require(not np.any(identification & (nonpaint | outside)), 'centre identity/validation overlap')
    if geometry_out is not None:
        geometry_out['centre'] = identification
    return {'role': 'within-discovery necessary check;not independent scene validation',
            'centre': centre, 'identification_pixels': int(identification.sum()),
            'exact_base_IDs': exact, 'all_candidate_errors': rows,
            'control1_identification': score(control, pre, identification),
            'post2_identification_diagnostic_only': score(post, pre, identification),
            'identification_disjoint_every_centre_top': True,
            'identification_validation_overlap': 0, 'base_compatible_branches': branches,
            'unique_identity_proved': len(exact) == 1,
            'geometry_never_selects_identity': True,
            'reveal_sources': 'unknown;no independent source assignment supplied',
            'removal_prediction_constructed': False,
            'stop_reason': 'base identity ambiguous or absent' if len(exact) != 1 else 'reveal sources unknown',
            'post_state_used_as_erase_backdrop': False}


def rear_source_identification(pre, post, control, domain, fulls, centre_branches=(241, 325), geometry_out=None):
    """Three frozen rear positions; complete possible occlusion, pre-state only.

    Centre aliases coexist in the occlusion union. Neither near matches nor post
    pixels fill missing reveal material. Empty exposure is unknown, never a hit.
    """
    import numpy as np
    from base_content_audit import diamond_mask
    shape = pre.shape[:2]
    union = np.zeros((59, 78), bool)
    for image_id in domain:
        full = fulls[image_id]
        union[-len(full):] |= full[..., 3] == 255
    centre_union = np.zeros(shape, bool)
    for image_id in centre_branches:
        support, _ = place_mask(shape, fulls[image_id][..., 3] == 255, (798, 1216))
        centre_union |= support
    geometry = []
    for cx, cy in ((718, 1176), (878, 1176), (798, 1136)):
        other = np.zeros(shape, bool)
        for dx, dy in OFFSETS:
            position = (cx + dx, cy + dy)
            if position == (798, 1216):
                other |= centre_union
            else:
                support, _ = place_mask(shape, union, position)
                other |= support
        public, _ = place_mask(shape, diamond_mask(), (cx, cy))
        region = public & ~other
        require(not np.any(region & centre_union), 'rear identification overlaps centre branches')
        geometry.append(((cx, cy), region, ~other))
    if geometry_out is not None:
        geometry_out['rear'] = [region for _, region, _ in geometry]
    rows = []
    for centre, region, exposed in geometry:
        candidates = identify(pre, {i: fulls[i] for i in domain}, centre, region, exposed) if region.any() else []
        exact = [r['ID'] for r in candidates if r['exact']]
        minimum = min((r['RGB5_errors'] for r in candidates), default=None)
        state = 'zero_information' if not region.any() else ('no_exact_source' if not exact else 'unique' if len(exact) == 1 else 'ambiguous')
        rows.append({'centre': centre, 'identification_pixels': int(region.sum()),
                     'state': state, 'exact_IDs': exact, 'all_candidate_errors': candidates,
                     'minimum_errors_diagnostic_only': minimum,
                     'minimum_IDs_not_promoted': [r['ID'] for r in candidates if r['RGB5_errors'] == minimum],
                     'control1_stability': score(control, pre, region),
                     'post2_stability_diagnostic_only': score(post, pre, region),
                     'centre_branch_opacity_overlap': 0})
    return {'role': 'within-discovery rear-material identification,not scene validation',
            'centre_branches_retained': list(centre_branches), 'domain_per_cell': len(domain),
            'rear_sources': rows, 'post_state_used_for_identity': False,
            'conditional_prediction_gate_passed': all(r['state'] == 'unique' for r in rows),
            'conditional_predictions_constructed': False,
            'reveal_material': 'unknown unless independently identified;no pixels fabricated',
            'no_minimum_promoted_or_colour_tolerance': True}


def paint_complete(rgb, known, source, centre):
    import numpy as np
    require(source.dtype==np.uint8 and source.ndim==3 and source.shape[1:]==(78,4)
                                    and 40<=source.shape[0]<=59 and np.all(np.isin(source[...,3],[0,255])), 'complete natural binary source required')
    support,bounds=place_mask(known.shape,source[...,3]==255,centre)
    x0,y0,x1,y1=bounds;local=support[y0:y1,x0:x1];target=rgb[y0:y1,x0:x1]
    target[local]=source[...,2::-1].repeat(2,0).repeat(2,1)[local];known[support]=True
    return support,bounds

def restore_removed_top(pre, old_top, under, under_known):
    import numpy as np
    require(pre.dtype==under.dtype==np.uint8 and pre.shape==under.shape
                                    and old_top.shape==under_known.shape==pre.shape[:2]
                                    and old_top.dtype==under_known.dtype==bool,'fixed pre/source-alpha underlay geometry')
    rgb=pre.copy();known=np.ones(old_top.shape,bool)
    rgb[old_top]=under[old_top];known[old_top]=under_known[old_top]
    return rgb,known

def score_known(rgb, actual, known, region):
    import numpy as np
    require(known.dtype==region.dtype==bool and known.shape==region.shape==actual.shape[:2], 'known/region geometry')
    measured=score(rgb,actual,known&region);predictable=measured.pop('pixels');denominator=int(region.sum())
    measured['RGB5_errors_known']=measured.pop('RGB5_errors')
    return {'denominator':denominator,'predictable':predictable,'unknown':denominator-predictable,
                                    **measured,'RGB5_not_explained':denominator-measured['RGB5_equal'],
                                    'RGB8_errors_known':predictable-measured['RGB8_equal'],
                                    'RGB8_not_explained':denominator-measured['RGB8_equal']}

def conditional_models(pre, post, fulls, tops, identification):
    import numpy as np
    shape=pre.shape[:2];under=np.zeros_like(pre);under_known=np.zeros(shape,bool)
    require(identification.dtype == bool and identification.shape == shape, 'source-only identification mask')
    sources=[(240,(798,1136)),(244,(718,1176)),(246,(878,1176))]
    full_source_controls=[]
    for image_id,centre in sources:
        support,bounds=paint_complete(under,under_known,fulls[image_id],centre)
        x0,y0,x1,y1=bounds;source=fulls[image_id];rgb=source[...,2::-1].repeat(2,0).repeat(2,1);local=support[y0:y1,x0:x1]
        full_source_controls.append({'ID':image_id,'centre':centre,'exact':False,'bounds':bounds,
                    'before':score(rgb,pre[y0:y1,x0:x1],local),'after':score(rgb,post[y0:y1,x0:x1],local)})
    old241,_=place_mask(shape,tops[241][...,3]==255,(798,1216));R=rectangle(shape,ROAD);V=rectangle(shape,VALIDATION);W=rectangle(shape,CONTEXT)
    require(not np.any(identification&old241),'old top and identification must be separate')
    road,_=place_mask(shape,fulls[794][...,3]==255,(798,1216));P=np.zeros(shape,bool)
    from base_content_audit import diamond_mask
    P,_=place_mask(shape,diamond_mask(),(798,1216));changed=np.any(pre!=post,2);changed5=np.any((pre>>3)!=(post>>3),2)
    U=P&~changed;N=R&~road&changed5;O=W&~R&changed5
    regions={'road':R,'validation':V,'context':W,'separated_validation':V&~identification,
                                        'separated_context':W&~identification,'old241_top':old241,'nonpaint':N,'outside':O,'unchanged_public':U}
    models=[]
    for old_id in (241,325):
        old,_=place_mask(shape,tops[old_id][...,3]==255,(798,1216))
        for optional in (False,True):
            predicted,known=restore_removed_top(pre,old,under,under_known)
            paint_complete(predicted,known,fulls[794],(798,1216))
            for image_id,centre in ((240,(718,1256)),(242,(878,1256))):paint_complete(predicted,known,tops[image_id],centre)
            if optional:paint_complete(predicted,known,tops[246],(798,1296))
            name='centre%d-%s'%(old_id,'three-front246-hypothesis'if optional else'two-front')
            models.append({'name':name,'centre_branch':old_id,'optional_front246_exact':False if optional else None,
                                                                        'scores':{key:score_known(predicted,post,known,region)for key,region in regions.items()}})
    optional=tops[246];support,bounds=place_mask(shape,optional[...,3]==255,(798,1296));x0,y0,x1,y1=bounds
    rgb=optional[...,2::-1].repeat(2,0).repeat(2,1);local=support[y0:y1,x0:x1]
    result={'role':'four frozen conditional edit hypotheses,not confirmed scene/source rule',
                                    'conditional_predictions_constructed':True,'source_identity_or_selection_rule_confirmed':False,
            'strict_identity_results_preserved':True,
            'rear_hypotheses_exact':False,'rear_pre_RGB5_errors_retained':[16,20,12],
            'rear_hypotheses':[{'ID':244,'centre':[718,1176],'exact':False,'pre_errors':[16,2844]},
                               {'ID':246,'centre':[878,1176],'exact':False,'pre_errors':[20,2688]},
                               {'ID':240,'centre':[798,1136],'exact':False,'pre_errors':[12,1824]}],
                                    'centre_branches_retained':[241,325],'single_order':'rear240,244,246;pre-state minus old-top alpha;road794;front240,242;optionalfront246',
                                    'underlay_opaque':int(under_known.sum()),'underlay_covers_old241_top':[int(np.count_nonzero(old241&under_known)),int(old241.sum())],
                                    'identification_pixels_in_context':int(np.count_nonzero(identification&W)),
                                    'identification_pixels_in_validation':int(np.count_nonzero(identification&V)),
                                    'full_rear_source_controls':full_source_controls,
                                    'optional_front246_whole_top':{'exact':False,'identification_errors':[8,624],'bounds':bounds,'before':score(rgb,pre[y0:y1,x0:x1],local),'after':score(rgb,post[y0:y1,x0:x1],local)},
                                    'models':models,'post_used_to_erase_or_fill':False,'source_alpha_or_order_fit':False,
                                    'unknown_counted_as_unexplained_in_full_denominator':True}
    return result


def read_sources(root, manifest):
    import numpy as np
    from base_content_audit import _cache_bytes, _cache_record
    index = (root / ARCHIVE).read_bytes()
    require(len(index) >= 40680 and len(index) < 128 * 1024 * 1024, 'SG3 index budget')
    declared, version = struct.unpack_from('<II', index)
    capacity, count, bitmaps = struct.unpack_from('<III', index, 12)
    require(version == 213 and declared in (len(index), 74480) and 0 < count < capacity
            and 6 < bitmaps <= 200 and 40680 + (count + 1) * 64 <= len(index), 'SG3 header')
    for slot, name in ((1, b'China_land1.bmp'), (6, b'China_land2.bmp')):
        require(index[680 + slot * 200:745 + slot * 200].split(b'\0')[0] == name, 'bitmap family')
    require((root / 'DATA/China_Terrain.555').stat().st_size < 128 * 1024 * 1024, 'pixel budget')
    pixels = (root / 'DATA/China_Terrain.555').read_bytes()
    records = {i: index[40680 + i * 64:40680 + (i + 1) * 64] for i in range(1, count + 1)}
    domain = [i for i, r in records.items() if struct.unpack_from('<hh', r, 20)[0] == 78
              and r[50:53] == bytes((30, 0, 0)) and r[56] in (1, 6)
              and struct.unpack_from('<i', r, 16)[0] == 0]
    require(len(domain) == 585, 'frozen domain differs')
    cache = manifest['native_cache']
    rows = [line.split('\t') for line in Path(cache['metadata']).read_text().splitlines()]
    require(0 < len(rows) <= 65536 and all(len(r) == 11 for r in rows), 'cache metadata')
    lookup = {(r[0], int(r[1])): (n, r) for n, r in enumerate(rows)}
    require(len(lookup) == len(rows), 'duplicate cache ID')
    fullcache = _cache_bytes(cache['full'], 'native full')
    rawcache = _cache_bytes(cache['raw'], 'native raw', len(rows) * 12480)
    fulls, tops = {}, {}
    for image_id in domain + [794]:
        base, top, full = decode_components(records[image_id], pixels)
        n, row = lookup[(ARCHIVE, image_id)]
        require((int(row[2]), int(row[3]), int(row[8]), int(row[7])) == (78, len(full), image_id, 0),
                'native cache geometry/identity')
        cached = _cache_record(fullcache, int(row[4]), full.size, 'native full')
        raw = _cache_record(rawcache, n * 12480, 12480, 'native raw')
        require(np.array_equal(full.reshape(-1), cached), 'native full bytes differ')
        require(np.array_equal(base.reshape(-1), raw), 'native base bytes differ')
        fulls[image_id], tops[image_id] = full, top
    require(sum(bool(np.any(tops[i][..., 3])) for i in domain) == 228, 'top domain differs')
    return domain, fulls, tops


def convert_sources(fulls, tops, manifest):
    """One frozen public ColorSync RGB8 contract; no colour-specific patch."""
    import numpy as np
    cmm = manifest['colorsync']
    for name in ('source_profile', 'target_profile'):
        require(hashlib.sha256(Path(cmm[name]).read_bytes()).hexdigest() == cmm[name + '_sha256'],
                'frozen profile bytes differ')

    def codes(image):
        q = (image[..., 2::-1] >> 3).astype(np.uint16)
        return (q[..., 0] << 10) | (q[..., 1] << 5) | q[..., 2]

    palette = np.unique(np.concatenate([codes(a)[a[..., 3] == 255] for a in fulls.values()]))
    points = np.stack(((palette >> 10) & 31, (palette >> 5) & 31, palette & 31), -1).astype(np.uint8) << 3
    with tempfile.TemporaryDirectory(prefix='emperor-ground-rgb8-') as tmp:
        source, target = Path(tmp) / 'input.rgb8', Path(tmp) / 'output.rgb8'
        points.tofile(source)
        subprocess.run([cmm['executable'], cmm['source_profile'], cmm['target_profile'],
                        str(source), str(target)], check=True, capture_output=True, text=True)
        output = np.frombuffer(target.read_bytes(), np.uint8)
        require(output.size == points.size, 'ColorSync output extent')
    table = np.zeros((32768, 3), np.uint8)
    table[palette] = output.reshape(-1, 3)
    old = np.fromfile(cmm['control_input'], np.uint8)
    expected = np.fromfile(cmm['control_output'], np.uint8)
    require(old.size == expected.size == 841 * 3, 'fixed colour control extent')
    old = old.reshape(-1, 3)
    require(not np.any(old % 8), 'RGB555 zero-filled control')
    q = (old >> 3).astype(np.uint16)
    oldcodes = (q[:, 0] << 10) | (q[:, 1] << 5) | q[:, 2]
    require(np.all(np.isin(oldcodes, palette))
            and np.array_equal(table[oldcodes].reshape(-1), expected), 'ColourSync controls differ')
    for images in (fulls, tops):
        for key, image in images.items():
            result = image.copy()
            result[..., :3] = table[codes(image)][..., ::-1]
            result[image[..., 3] == 0, :3] = 0
            images[key] = result
    return {'source_colours': len(palette), 'old_controls': [841, 841]}


def run(root, manifest):
    import numpy as np
    from PIL import Image
    from base_content_audit import diamond_mask
    require(Path(manifest['source_root']).resolve() == root, 'source provenance root differs')
    domain, fulls, tops = read_sources(root, manifest)
    conversion = convert_sources(fulls, tops, manifest)
    frames = []
    for name in ('control1', 'after2', 'before3'):
        with Image.open(manifest[name]) as image:
            require(image.size == (2048, 1592), 'frozen frame geometry')
            frames.append(np.asarray(image.convert('RGB')))
    control, post, pre = frames
    shape = pre.shape[:2]
    public = diamond_mask()
    union = np.zeros((59, 78), bool)
    for image_id in domain:
        full = fulls[image_id]
        union[-len(full):] |= full[..., 3] == 255
    identification, geometries = [], []
    for centre in CENTRES:
        region, exposed = exposed_geometry(shape, union, public, centre)
        rows = identify(pre, {i: fulls[i] for i in domain}, centre, region, exposed)
        identification.append({'centre': centre, 'I_pixels': int(region.sum()), 'candidates': rows,
                               'exact_IDs': [r['ID'] for r in rows if r['exact']],
                               'stable1_3': score(control, pre, region), 'stable2_3': score(post, pre, region)})
        geometries.append(region)
    require([r['exact_IDs'] for r in identification] == [[240], [242], []],
            'identification gate differs; no fallback to minimum')
    components = []
    for image_id, centre in zip((240, 242), CENTRES):
        top = tops[image_id]
        mask, bounds = place_mask(shape, top[..., 3] == 255, centre)
        require(not np.any(mask & np.logical_or.reduce(geometries)), 'top/identification overlap')
        x0, y0, x1, y1 = bounds
        rgb = top[..., 2::-1].repeat(2, 0).repeat(2, 1)
        local = mask[y0:y1, x0:x1]
        components.append({'ID': image_id, 'bounds': bounds, 'mask': mask, 'rgb': rgb,
                           'before': score(rgb, pre[y0:y1, x0:x1], local),
                           'after': score(rgb, post[y0:y1, x0:x1], local)})
    require(not np.any(components[0]['mask'] & components[1]['mask']), 'two-top order ambiguous')
    road, _ = place_mask(shape, fulls[794][..., 3] == 255, (798, 1216))
    P, _ = place_mask(shape, public, (798, 1216))
    R, V, W = (rectangle(shape, b) for b in (ROAD, VALIDATION, CONTEXT))
    changed = np.any(pre != post, 2)
    changed5 = np.any((pre >> 3) != (post >> 3), 2)
    U, N, outside = P & ~changed, R & ~road & changed5, W & ~R & changed5
    require((int(U.sum()), int(N.sum()), int(outside.sum())) == (1856, 1512, 496), 'partitions differ')
    alltop = np.zeros(shape, bool)
    for image_id in domain:
        for centre in CENTRES:
            support, _ = place_mask(shape, tops[image_id][..., 3] == 255, centre)
            alltop |= support
    x0, y0, x1, y1 = CONTEXT
    crop = lambda a: a[y0:y1, x0:x1]
    reference, backdrop = crop(post), crop(pre)
    models = []
    for label, selected in (('before-only', None), ('road794-only', []),
                            ('road794-plus240', [0]), ('road794-plus242', [1]),
                            ('road794-plus-both', [0, 1])):
        predicted = backdrop.copy()
        def paint(bounds, rgb, mask):
            a, b, c, d = bounds
            require(x0 <= a < c <= x1 and y0 <= b < d <= y1, 'whole component outside context')
            target = predicted[b-y0:d-y0, a-x0:c-x0]
            target[mask] = rgb[mask]
        if selected is not None:
            paint(ROAD, fulls[794][..., 2::-1].repeat(2, 0).repeat(2, 1), road[1176:1256, 720:876])
            for n in selected:
                component = components[n]
                a, b, c, d = component['bounds']
                paint(component['bounds'], component['rgb'], component['mask'][b:d, a:c])
        models.append({'name': label, 'context': score(predicted, reference, crop(W)),
                       'validation': score(predicted, reference, crop(V)),
                       'road': score(predicted, reference, crop(R)),
                       'unchanged_public': score(predicted, reference, crop(U)),
                       'changed_nonpaint': score(predicted, reference, crop(N))})
    selected_support = components[0]['mask'] | components[1]['mask']
    geometry_masks = {}
    centre_result = centre_removal_necessary(pre, post, control, domain, fulls, tops, N, outside, geometry_out=geometry_masks)
    rear_result = rear_source_identification(pre, post, control, domain, fulls, geometry_out=geometry_masks)
    all_identification = np.logical_or.reduce(geometries + [geometry_masks['centre']] + geometry_masks['rear'])
    conditional_result = conditional_models(pre, post, fulls, tops, all_identification)
    return {'role': 'spatially separated discovery,not blind scene validation',
            'frames': 'before3 to after2;control1 stability only', 'domain': len(domain),
            'direct_full_and_base_native_checks': [586, 586], 'conversion': conversion,
            'identification': identification,
            'whole_components': [{k: c[k] for k in ('ID', 'bounds', 'before', 'after')} for c in components],
            'models': models,
            'selected_top_reach_U': [int(np.count_nonzero(selected_support & U)), int(U.sum())],
            'selected_top_reach_N': [int(np.count_nonzero(selected_support & N)), int(N.sum())],
            'opacity_bound': {'placements': len(domain) * len(CENTRES),
                              'U_reachable': [int(np.count_nonzero(alltop & U)), int(U.sum())],
                              'N_reachable': [int(np.count_nonzero(alltop & N)), int(N.sum())],
                              'outside_reachable': [int(np.count_nonzero(alltop & outside)), int(outside.sum())],
                              'mutually_exclusive_ID_union_is_not_a_composition': True},
            'centre_removal': centre_result,
            'rear_material': rear_result,
            'conditional_models': conditional_result,
            'engine_source_rule_or_absolute_saved_cell_binding_proved': False}


def main():
    value = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not value:
        print(json.dumps({'skipped': True, 'reason': 'EMPEROR_GAMEDATA_ROOT unset'}))
        return 0
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True)
    parser.add_argument('--output', required=True, help='private JSON evidence outside Git/data root')
    args = parser.parse_args()
    try:
        root = Path(value).resolve(strict=True)
        output = Path(args.output).resolve()
        repo = Path(__file__).resolve().parents[1]
        require(not output.is_relative_to(root) and not output.is_relative_to(repo), 'output must stay outside data/Git')
        result = run(root, json.loads(Path(args.manifest).read_text()))
        output.write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps({'completed': True, 'domain': result['domain'],
                          'exact_neighbours': [r['exact_IDs'] for r in result['identification']],
                          'models': result['models'], 'opacity_bound': result['opacity_bound'],
                          'centre_base_IDs': result['centre_removal']['exact_base_IDs'],
                          'centre_removal_prediction': result['centre_removal']['removal_prediction_constructed'],
                          'rear_exact_IDs': [r['exact_IDs'] for r in result['rear_material']['rear_sources']],
                          'conditional_models': len(result['conditional_models']['models'])}))
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print(json.dumps({'error': str(error)}))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
