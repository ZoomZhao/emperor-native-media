#!/usr/bin/env python3
"""Measure single-byte conversion and actual CRT input, never guess DBCS."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run

NAME='MB936'
def make():
    data=Data();data.alloc(b'MB936010')
    operations=[];cases=[]
    def add(label,source,flags=1,n=None,size=4):
        src=data.alloc(source)
        out=data.alloc(bytes([0xa5])*(max(4,size)*2+8));status=data.alloc(bytes(8))
        op=dict(name='MultiByteToWideChar',args=[936,flags,src,len(source) if n is None else n,0 if size==0 else out+4,size],result=status)
        operations.append(op);cases.append(dict(label=label,source_hex=source.hex(),flags=flags,length=len(source) if n is None else n,capacity=size,result_offset=data.offset(status),buffer_offset=data.offset(out),buffer_size=max(4,size)*2+8))
    for b in range(256):add('single-'+str(b),bytes([b]))
    actual=json.loads((ROOT/'evidence/stage10-checkpoint-city-ctype.json').read_text())['first_actual_refusal']
    source=bytes.fromhex(actual['source_hex'])
    assert len(source)==256 and actual['args'][:2]==['0x3a8','0x1']
    add('actual-query',source,size=0)
    add('actual-output',source,size=256)
    add('actual-short-output',source,size=1)
    add('negative-null-terminated',b'A\0Z',n=-1)
    add('zero-input-length',b'A',n=0)
    add('negative-capacity',b'A',size=-1)
    add('invalid-flags',b'A',flags=0xffffffff)
    add('strict-invalid-byte',b'\xff',flags=8)
    add('default-invalid-byte',b'\xff',flags=0)
    meta=build(NAME,data,operations);(ROOT/f'evidence/stage10-{NAME}-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
    return dict(sha256=meta['sha256'],size=meta['size'],case_count=len(cases))

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
        for case in json.loads((ROOT/f'evidence/stage10-{NAME}-cases.json').read_text()):
            value,error=struct.unpack_from('<II',raw,case['result_offset'])
            at=case['buffer_offset'];row=dict(case,return_value=value,last_error=error,canary_buffer_hex=raw[at:at+case['buffer_size']].hex());rows.append(row)
        result['rows']=rows
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2))
    if 'rows' in result:print(json.dumps([r for r in rows if not r['label'].startswith('single-') or r['label'] in ['single-128','single-129','single-255']],indent=2))

if __name__=='__main__':main()
