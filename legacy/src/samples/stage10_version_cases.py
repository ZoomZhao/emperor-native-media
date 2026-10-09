#!/usr/bin/env python3
"""Own GetVersion/GetVersionExA success/error/canary baseline probe."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run
NAME='VERSIONCASES';SIZES=[0,4,147,148,149,156,157,0xffffffff]

def make():
    data=Data();data.alloc(b'VERCASE1');plain=data.alloc(bytes(8));ops=[dict(name='GetVersion',args=[],result=plain)];cases=[]
    for size in SIZES:
        buffer=data.alloc(bytes([0xa5])*4+struct.pack('<I',size)+bytes([0xa5])*160)
        result=data.alloc(bytes(8));ops.append(dict(name='GetVersionExA',args=[buffer+4],result=result))
        cases.append(dict(declared_size=size,buffer_offset=data.offset(buffer),buffer_size=168,result_offset=data.offset(result)))
    meta=build(NAME,data,ops);(ROOT/f'evidence/stage10-{NAME}-cases.json').write_text(json.dumps(cases,indent=2)+'\n');return dict(sha256=meta['sha256'],size=meta['size'])

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();result['GetVersion']=hex(struct.unpack_from('<I',raw,8)[0]);result['rows']=[]
        for case in json.loads((ROOT/f'evidence/stage10-{NAME}-cases.json').read_text()):
            value,error=struct.unpack_from('<II',raw,case['result_offset']);at=case['buffer_offset']
            result['rows'].append(dict(case,return_value=value,last_error=error,buffer_hex=raw[at:at+168].hex()))
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2))
    if 'rows' in result:print(json.dumps(result['rows'],indent=2))

if __name__=='__main__':main()
