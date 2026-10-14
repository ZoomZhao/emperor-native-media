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


def load_pool(pool):
    import numpy as np
    mask=diamond_mask();kind=pool['schema'];path=Path(pool['metadata'])
    if kind=='single':
        rows=[r.split('\t')for r in path.read_text().splitlines()]
        raw=np.memmap(pool['raw'],np.uint8,mode='r',shape=(len(rows),40,78,4))
        if Path(pool['raw']).stat().st_size!=len(rows)*12480:raise ValueError('raw single-cache byte extent')
        full=np.memmap(pool['full'],np.uint8,mode='r');images=[];labels=[]
        for i,r in enumerate(rows):
            width,height,offset=map(int,r[2:5])
            if width!=78 or height<40 or offset+width*height*4>len(full):raise ValueError('single cache extent')
            canvas=full[offset:offset+width*height*4].reshape(height,width,4)
            for variant,image in (('composite',canvas[-40:]),('raw',raw[i])):
                images.append(image[mask]);labels.append({'archive':r[0],'id':int(r[1]),'resolved_id':int(r[8]),'variant':variant})
        return np.asarray(images,np.uint8),labels
    if kind=='multibase':
        rows=list(csv.DictReader(path.open(),delimiter='\t'))
        if Path(pool['pixels']).stat().st_size!=len(rows)*6400:raise ValueError('masked cache byte extent')
        bases=np.memmap(pool['pixels'],np.uint8,mode='r',shape=(len(rows),1600,4));labels=[]
        for i,row in enumerate(rows):
            n=int(row['n']);x,y=int(row['x']),int(row['y'])
            if not 2<=n<=6 or not 0<=x<n or not 0<=y<n or int(row['width'])!=80*n-2 or int(row['height'])<40*n or int(row['offset'])!=i*6400:
                raise ValueError('public multibase cache geometry/extent')
            labels.append({k:int(v)if k not in ('archive','variant')else v for k,v in row.items()if k not in ('offset','opaque')})
        return bases,labels
    if kind=='regular':
        rows=[r.split('\t')for r in path.read_text().splitlines()];blob=np.memmap(pool['pixels'],np.uint8,mode='r');images=[];labels=[]
        for row in rows:
            offset=int(row[5]);image=blob[offset:offset+12480].reshape(40,78,4)
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


def main():
    env=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not env:print(json.dumps({'skipped':True,'reason':'EMPEROR_GAMEDATA_ROOT unset'}));return 0
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--manifest',type=Path,required=True);args=parser.parse_args()
    root=Path(env)
    if not root.is_absolute()or not root.is_dir():parser.error('existing absolute root required')
    try:print(json.dumps(run(root.resolve(),json.loads(args.manifest.read_text())),ensure_ascii=False))
    except ValueError as error:print(str(error),file=__import__('sys').stderr);return 1
    return 0

if __name__=='__main__':raise SystemExit(main())
