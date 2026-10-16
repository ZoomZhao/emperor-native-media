"""Exact fixed-base content comparison; no file/ID selector or game rules.

CLI replays explicit private native-decoder caches from a verified data root.
All pixel inputs stay local; only aggregate match counts and metadata are emitted.
"""
from pathlib import Path
import argparse,csv,hashlib,json,os


def diamond_mask():
    import numpy as np
    mask=np.zeros((40,78),bool)
    for y in range(40):
        edge=38-2*y if y<20 else 2*y-40;mask[y,edge:78-edge]=True
    return mask


def reference_base(reference):
    import numpy as np
    reference=np.asarray(reference);mask=diamond_mask()
    if reference.shape!=(80,156,3)or reference.dtype!=np.uint8:raise ValueError('fixed nearest2x uint8 RGB base required')
    logical=reference[::2,::2];expanded=np.repeat(np.repeat(logical,2,0),2,1);physical=np.repeat(np.repeat(mask,2,0),2,1)
    if not np.array_equal(reference[physical],expanded[physical]):raise ValueError('raw nearest2x base repetition required')
    return logical[mask]>>3


def crop_reference_base(frame,center):
    import numpy as np
    frame=np.asarray(frame)
    if (frame.ndim!=3 or frame.shape[-1]!=3 or frame.dtype!=np.uint8 or max(frame.shape[:2])>4096
            or len(center)!=2 or any(type(v)is not int for v in center)):
        raise ValueError('bounded RGB frame and integer center required')
    cx,cy=center
    if not 78<=cx<=frame.shape[1]-78 or not 40<=cy<=frame.shape[0]-40:
        raise ValueError('full base bounds; no wrap or clip')
    return frame[cy-40:cy+40,cx-78:cx+78]


def match_masked_bases(reference,bases,hasher=None):
    """Hash filters only; verify all1600 RGB5 triplets after every hash hit.

    Input bases are already sampled at the public source subbase positions.
    Nonopaque bases remain explicitly unknown for this complete-base test.
    """
    import numpy as np
    query=reference_base(reference);bases=np.asarray(bases)
    if bases.ndim!=3 or bases.shape[1:]!=(1600,4)or bases.dtype!=np.uint8 or len(bases)>65536:
        raise ValueError('bounded uint8 BGRA masked base candidates required')
    digest=hasher or (lambda value:hashlib.sha256(value).digest())
    key=digest(query.tobytes());hits=[];unknown=0;minimum=None
    for i,base in enumerate(bases):
        if not np.all(base[:,3]==255):unknown+=1;continue
        rgb=base[:,2::-1]>>3;bad=int(np.any(rgb!=query,axis=1).sum())
        minimum=bad if minimum is None else min(minimum,bad)
        if digest(rgb.tobytes())==key and np.array_equal(rgb,query):hits.append(i)
    return {'candidate_variants':len(bases),'eligible_opaque_variants':len(bases)-unknown,
            'unknown_nonopaque_variants':unknown,'matching_indices':hits,
            'minimum_logical_mismatches':minimum,'logical_pixels':1600,'physical_pixels':6400,
            'RGB_bits':5,'finite_domain_only':True}


def match_causal_bases(before,after,bases):
    """Localize source content using an observed change, never infer visibility.

    The public base footprint and caller placement are fixed. A candidate must
    explain every changed RGB5 sample. Unchanged positions are still checked and
    reported; they are never turned into alpha, occlusion or a complete layer.
    """
    import numpy as np
    a,b=reference_base(before),reference_base(after);bases=np.asarray(bases)
    if bases.ndim!=3 or bases.shape[1:]!=(1600,4)or bases.dtype!=np.uint8 or len(bases)>65536:
        raise ValueError('bounded uint8 BGRA masked base candidates required')
    mask=diamond_mask();raw_a=np.asarray(before)[::2,::2][mask];raw_b=np.asarray(after)[::2,::2][mask]
    changed=np.any(raw_a!=raw_b,axis=1);discriminating=np.any(a!=b,axis=1)
    if not np.any(discriminating):raise ValueError('a discriminating RGB5 change is required')
    hits=[];unknown=0;complete=[]
    for i,base in enumerate(bases):
        if not np.all(base[:,3]==255):unknown+=1;continue
        bad=np.any((base[:,2::-1]>>3)!=b,axis=1)
        if np.any(bad[changed]):continue
        count=int(bad.sum());hits.append({'index':i,'whole_base_mismatches':count,
                                         'unchanged_base_mismatches':int(bad[~changed].sum())})
        if not count:complete.append(i)
    return {'candidate_count':len(bases),'unknown_nonopaque_candidates':unknown,
            'raw_changed_logical_pixels':int(changed.sum()),'RGB5_changed_logical_pixels':int(discriminating.sum()),
            'logical_pixels':1600,'physical_pixels':6400,'causal_subset_matches':hits,
            'complete_base_matching_indices':complete,'finite_domain_only':True,'RGB_bits':5,
            'visibility_mask_inferred':False,'engine_selected_ID_proven':False}


def run_causal_base(root,manifest):
    """Explicit private paired captures/cache; no asset selector or renderer."""
    import numpy as np
    from PIL import Image
    if Path(manifest['source_root']).resolve()!=root:raise ValueError('cache provenance root differs')
    if manifest['pool']['schema']!='single' or manifest['variant']!='composite':
        raise ValueError('declared single-cell composite pool required')
    bases,labels=load_pool(manifest['pool'])
    if len(bases)!=manifest['expected_variants']:raise ValueError('frozen pool size differs')
    selected=[i for i,label in enumerate(labels)if label['variant']=='composite']
    if len(selected)!=manifest['expected_candidates']:raise ValueError('frozen candidate count differs')
    for archive in {labels[i]['archive']for i in selected}:
        name=Path(archive);asset=(root/name).resolve()
        if name.is_absolute()or '..'in name.parts or not asset.is_relative_to(root)or not asset.is_file():
            raise ValueError('cached archive outside source root')
    references=[]
    for key in ('before','after'):
        with Image.open(manifest[key])as image:
            if max(image.size)>4096:raise ValueError('bounded RGB frame required')
            references.append(crop_reference_base(np.asarray(image.convert('RGB')),manifest['center']))
    result=match_causal_bases(*references,bases[selected])
    for hit in result['causal_subset_matches']:hit['source']=labels[selected[hit.pop('index')]]
    result['complete_base_matches']=[labels[selected[i]]for i in result.pop('complete_base_matching_indices')]
    return result


def source_over_fixed_base(base,layers):
    """One fixed opaque BGRA base, native premultiplied layers, no fitted alpha.

    This is a diagnostic source-over contract, not an original-game rule.
    """
    import numpy as np
    base,layers=map(np.asarray,(base,layers))
    if (base.shape!=(1600,4) or base.dtype!=np.uint8 or layers.ndim!=3
            or layers.shape[1:]!=(1600,4) or layers.dtype!=np.uint8 or len(layers)>1024
            or not np.all(base[:,3]==255)):
        raise ValueError('one opaque base and bounded native BGRA layers required')
    if np.any(layers[:,:,:3]>layers[:,:,3,None]):raise ValueError('native premultiplied channels required')
    src=layers.astype(np.uint32);dst=base.astype(np.uint32)[None];alpha=src[:,:,3,None]
    result=src+(dst*(255-alpha)+127)//255
    if np.any(result>255)or not np.all(result[:,:,3]==255):raise ValueError('source-over invariant')
    return result.astype(np.uint8)


def _cache_bytes(path,label,expected_size=None):
    """Open only complete BGRA storage; check declared extents before mapping."""
    import numpy as np
    size=Path(path).stat().st_size
    if size%4 or (expected_size is not None and size!=expected_size):
        raise ValueError(label+' byte extent')
    return np.memmap(path,np.uint8,mode='r') if size else np.empty(0,np.uint8)


def _cache_record(blob,offset,size,label):
    """Validate both bounds before slicing so negative indices never wrap."""
    if (offset<0 or size<0 or offset>len(blob) or size>len(blob)-offset
            or offset%4 or size%4):
        raise ValueError(label+' record extent')
    record=blob[offset:offset+size]
    if len(record)!=size:raise ValueError(label+' incomplete record')
    return record


def load_pool(pool):
    import numpy as np
    mask=diamond_mask();kind=pool['schema'];path=Path(pool['metadata'])
    if kind=='single':
        rows=[r.split('\t')for r in path.read_text().splitlines()]
        raw=_cache_bytes(pool['raw'],'raw single-cache',len(rows)*12480).reshape(len(rows),40,78,4)
        full=_cache_bytes(pool['full'],'single cache');images=[];labels=[]
        for i,r in enumerate(rows):
            width,height,offset=map(int,r[2:5])
            if width!=78 or height<40:raise ValueError('single cache geometry')
            canvas=_cache_record(full,offset,width*height*4,'single cache').reshape(height,width,4)
            for variant,image in (('composite',canvas[-40:]),('raw',raw[i])):
                images.append(image[mask]);labels.append({'archive':r[0],'id':int(r[1]),'resolved_id':int(r[8]),'variant':variant})
        return np.asarray(images,np.uint8),labels
    if kind=='multibase':
        with path.open()as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        bases=_cache_bytes(pool['pixels'],'masked cache',len(rows)*6400).reshape(len(rows),1600,4);labels=[]
        for i,row in enumerate(rows):
            n=int(row['n']);x,y=int(row['x']),int(row['y'])
            if not 2<=n<=6 or not 0<=x<n or not 0<=y<n or int(row['width'])!=80*n-2 or int(row['height'])<40*n or int(row['offset'])!=i*6400:
                raise ValueError('public multibase cache geometry/extent')
            labels.append({k:int(v)if k not in ('archive','variant')else v for k,v in row.items()if k not in ('offset','opaque')})
        return bases,labels
    if kind=='regular':
        rows=[r.split('\t')for r in path.read_text().splitlines()];blob=_cache_bytes(pool['pixels'],'regular cache');images=[];labels=[]
        for row in rows:
            offset=int(row[5]);image=_cache_record(blob,offset,12480,'regular cache').reshape(40,78,4)
            legal=np.array_equal(image[:,:,3],mask.astype(np.uint8)*255)
            if legal!=bool(int(row[6])):raise ValueError('regular alpha geometry metadata disagrees')
            if legal:
                images.append(image[mask]);labels.append({'archive':row[0],'id':int(row[1]),'resolved_id':int(row[2]),'type':int(row[3]),'bitmap_index':int(row[4]),'variant':'regular'})
        return np.asarray(images,np.uint8).reshape(-1,1600,4),labels
    raise ValueError('unknown cache schema')


def run(root,manifest):
    import numpy as np
    from PIL import Image
    if Path(manifest['source_root']).resolve()!=root:raise ValueError('cache provenance root differs')
    frame=np.asarray(Image.open(manifest['reference']).convert('RGB'));results=[]
    for pool in manifest['pools']:
        bases,labels=load_pool(pool)
        if len(bases)!=pool['expected_variants']:raise ValueError('frozen pool size differs')
        for archive in {label['archive']for label in labels}:
            name=Path(archive);asset=(root/name).resolve()
            if name.is_absolute()or '..'in name.parts or not asset.is_relative_to(root)or not asset.is_file():raise ValueError('cached archive outside source root')
        targets={}
        for target in manifest['targets']:
            reference=crop_reference_base(frame,target['center'])
            measured=match_masked_bases(reference,bases);hits=[labels[i]for i in measured.pop('matching_indices')]
            targets[target['name']]={**measured,'matching_candidates':len(hits),'matching_logical_records':len({(h['archive'],h['id'])for h in hits}),'matches':hits}
        results.append({'pool':pool['name'],'targets':targets})
    return {'pools':results,'engine_selected_ID_proven':False,'all_original_resource_uniqueness_proven':False}


def run_single_layer(root,manifest):
    """Replay a previously frozen one-base/one-layer diagnostic, never infer use."""
    import numpy as np
    import struct
    from PIL import Image
    from map_sg3_audit import decode_container,city_fields
    if Path(manifest['source_root']).resolve()!=root:raise ValueError('cache provenance root differs')
    name=Path(manifest['map']);map_path=(root/name).resolve()
    if name.is_absolute() or '..'in name.parts or not map_path.is_relative_to(root):raise ValueError('map root escape')
    payload=decode_container(map_path.read_bytes());city_fields(payload)
    side=struct.unpack_from('<I',payload,84)[0];origin=(228-side)//2
    if manifest['map_raw_width']!=4:raise ValueError('frozen diagnostic u32 only')
    raw_offset=manifest['map_raw_offset']
    if not 0<=raw_offset<=len(payload)-51984*4:raise ValueError('raw span bounds')
    bases,labels=load_pool(manifest['base_pool']);mask=diamond_mask()
    rows=[r.split('\t')for r in Path(manifest['layers_metadata']).read_text().splitlines()]
    blob=_cache_bytes(manifest['layers_pixels'],'layer cache');layers=[];layer_labels=[]
    for row in rows:
        image=_cache_record(blob,int(row[5]),12480,'layer cache').reshape(40,78,4)
        if not int(row[7]):continue
        if np.any(image[:,:,3][~mask]!=0)or not np.any((image[:,:,3]>0)&(image[:,:,3]<255)):
            raise ValueError('frozen partial-alpha support differs')
        name=Path(row[0]);asset=(root/name).resolve()
        if name.is_absolute()or '..'in name.parts or not asset.is_relative_to(root)or not asset.is_file():raise ValueError('layer archive root escape')
        layers.append(image[mask]);layer_labels.append({'archive':row[0],'id':int(row[1]),'bitmap_index':int(row[4])})
    if len(layers)!=manifest['expected_layers']:raise ValueError('frozen layer count differs')
    layers=np.asarray(layers,np.uint8);frame=np.asarray(Image.open(manifest['reference']).convert('RGB'));result={}
    for target in manifest['targets']:
        x,y=target['cell']
        if not 0<=x<side or not 0<=y<side:raise ValueError('canonical map coordinate bounds')
        raw=struct.unpack_from('<I',payload,raw_offset+4*((y+origin)*228+x+origin))[0]
        predicted=raw-manifest['id_base']
        if predicted!=target['id']:raise ValueError('frozen MAP prediction differs')
        found=[i for i,label in enumerate(labels)if label['archive']==target['archive']and label['id']==predicted and label['variant']=='raw']
        if len(found)!=1:raise ValueError('one frozen raw base required')
        base=bases[found[0]];reference=crop_reference_base(frame,target['center'])
        measured=match_masked_bases(reference,source_over_fixed_base(base,layers));hits=measured.pop('matching_indices')
        zero=source_over_fixed_base(base,np.zeros((1,1600,4),np.uint8));control=match_masked_bases(reference,zero);control_hits=control.pop('matching_indices')
        result[target['name']]={**measured,'matching_records':len(hits),'matches':[layer_labels[i]for i in hits],
                              'zero_alpha_matches':len(control_hits),'zero_alpha_minimum_differences':control['minimum_logical_mismatches']}
    return {'single_layer_count':len(layers),'targets':result,'original_rule_proven':False,'fixed_anchor_conditional':True}


def main():
    env=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not env:print(json.dumps({'skipped':True,'reason':'EMPEROR_GAMEDATA_ROOT unset'}));return 0
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--manifest',type=Path,required=True);args=parser.parse_args()
    root=Path(env)
    if not root.is_absolute()or not root.is_dir():parser.error('existing absolute root required')
    try:
        manifest=json.loads(args.manifest.read_text())
        function={'single-layer':run_single_layer,'causal-base':run_causal_base}.get(manifest.get('mode'),run)
        print(json.dumps(function(root.resolve(),manifest),ensure_ascii=False))
    except ValueError as error:print(str(error),file=__import__('sys').stderr);return 1
    return 0

if __name__=='__main__':raise SystemExit(main())
