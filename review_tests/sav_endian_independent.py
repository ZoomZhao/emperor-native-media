"""Scalar byte oracle for the fixed SAV layout search, authored inputs only."""
from pathlib import Path
import argparse,importlib.util,json
import numpy as np
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
spec=importlib.util.spec_from_file_location('review_sav',args.source_root/'scripts/sav_surface_search.py')
s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
grid=np.arange(228*228,dtype='uint32').reshape(228,228)
grid=((grid+1)<<16)|((grid*17+3)&65535);crop=grid[106:122,106:122]
def oracle(a,tile,width,shift,endian):
    return b''.join(((int(a[y,x])>>shift)&((1<<(8*width))-1)).to_bytes(width,'little'if endian=='<'else'big')
        for by in range(0,len(a),tile)for bx in range(0,len(a),tile)
        for y in range(by,min(by+tile,len(a)))for x in range(bx,min(bx+tile,len(a))))
cases=[]
for a in (grid,crop,grid.T,crop.T):
    for width,shift,endian in s.ENCODINGS:
        for tile in (8,16):
            cases.append({'side':len(a),'width':width,'shift':shift,'endian':endian,'tile':tile,
                'correct':s.tiles(s.projected(a,width,shift,endian),tile).tobytes()==oracle(a,tile,width,shift,endian)})
controls=[]
for width,shift in ((4,0),(2,0),(2,16)):
    for tile in (8,16):
        pattern=oracle(crop,tile,width,shift,'>');payload=b'header!'+pattern+b'tail'
        r=s.search_surface(grid,16,payload)
        controls.append({'width':width,'shift':shift,'tile':tile,'scalar_pattern_offset':payload.find(pattern),
            'hits':r['families']['block_major']['hits']})
passed=all(c['correct']for c in cases)and all(c['hits']==1 and c['scalar_pattern_offset']==7 for c in controls)
args.output.write_text(json.dumps({'passed':passed,'serialization_cases':cases,'big_endian_controls':controls,
    'no_monkeypatch':True,'original_data_reads':0},indent=2))
print(json.dumps({'passed':passed,'serialization_correct':sum(c['correct']for c in cases),'serialization_total':len(cases),
    'big_endian_positive_controls':sum(c['hits']==1 for c in controls),'controls_total':len(controls)}))
raise SystemExit(0 if passed else 1)
