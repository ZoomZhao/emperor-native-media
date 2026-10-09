#!/usr/bin/env python3
"""Measure actual movable HGLOBAL, lock count, pointer identity and release."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run
NAME='GLOBAL';CASES=[(0x2002,1),(0x2002,9),(0x2002,256),(0x2042,256),(0x2002,0)]

def make():
    data=Data();data.alloc(b'GLOBAL10');ops=[];cases=[]
    for flags,size in CASES:
        steps={}
        def op(label,name,args):
            result=data.alloc(bytes(8));ops.append(dict(name=name,args=args,result=result));steps[label]=data.offset(result);return result
        handle=op('alloc','GlobalAlloc',[flags,size]);h=dict(load=handle)
        op('size','GlobalSize',[h]);op('initial_flags','GlobalFlags',[h])
        first=op('lock1','GlobalLock',[h]);op('lock2','GlobalLock',[h]);op('locked_flags','GlobalFlags',[h])
        if size:op('pointer_to_handle','GlobalHandle',[dict(load=first)])
        op('unlock1','GlobalUnlock',[h]);op('unlock2','GlobalUnlock',[h]);op('unlock3','GlobalUnlock',[h]);op('final_flags','GlobalFlags',[h]);op('free','GlobalFree',[h])
        cases.append(dict(flags=flags,size=size,steps=steps))
    meta=build(NAME,data,ops);(ROOT/f'evidence/stage10-{NAME}-cases.json').write_text(json.dumps(cases,indent=2)+'\n');return dict(sha256=meta['sha256'],size=meta['size'])

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();result['cases']=[]
        for case in json.loads((ROOT/f'evidence/stage10-{NAME}-cases.json').read_text()):
            steps={}
            for label,at in case['steps'].items():
                value,error=struct.unpack_from('<II',raw,at);steps[label]=dict(return_value=value,last_error=error)
            result['cases'].append(dict(flags=case['flags'],size=case['size'],steps=steps))
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
