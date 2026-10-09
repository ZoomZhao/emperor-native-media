#!/usr/bin/env python3
"""Own private-heap size probe; requested vs allocation rounding is measured."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run
NAME='HEAPSIZE';SIZES=[0,1,7,8,9,0x80,0x480]

def make():
    data=Data();data.alloc(b'HEAP0010');ops=[];heap=data.alloc(bytes(8))
    ops.append(dict(name='HeapCreate',args=[0,0,0],result=heap));cases=[]
    for size in SIZES:
        allocation=data.alloc(bytes(8));result=data.alloc(bytes(8));freed=data.alloc(bytes(8))
        ops.extend([dict(name='HeapAlloc',args=[dict(load=heap),0,size],result=allocation),dict(name='HeapSize',args=[dict(load=heap),0,dict(load=allocation)],result=result),dict(name='HeapFree',args=[dict(load=heap),0,dict(load=allocation)],result=freed)])
        cases.append(dict(size=size,allocation_offset=data.offset(allocation),result_offset=data.offset(result),free_offset=data.offset(freed)))
    meta=build(NAME,data,ops);(ROOT/f'evidence/stage10-{NAME}-cases.json').write_text(json.dumps(cases,indent=2)+'\n');return dict(sha256=meta['sha256'],size=meta['size'])

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();result['rows']=[]
        for case in json.loads((ROOT/f'evidence/stage10-{NAME}-cases.json').read_text()):
            allocation=struct.unpack_from('<I',raw,case['allocation_offset'])[0];value,error=struct.unpack_from('<II',raw,case['result_offset']);freed=struct.unpack_from('<I',raw,case['free_offset'])[0]
            assert allocation and freed==1;result['rows'].append(dict(requested=case['size'],HeapSize=value,last_error=error,HeapFree=freed))
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
