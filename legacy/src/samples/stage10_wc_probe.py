#!/usr/bin/env python3
"""Own inverse-conversion probe for the actual CRT's measured character subset."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run
NAME='WC936'

def make():
    data=Data();data.alloc(b'WC936010');ops=[];cases=[]
    def add(label,source,flags=0x220,n=None,size=4,used=False):
        src=data.alloc(source);out=data.alloc(bytes([0xa5])*(max(4,size)+8));used_ptr=data.alloc(bytes([0xa5])*4);status=data.alloc(bytes(8))
        count=len(source)//2 if n is None else n
        ops.append(dict(name='WideCharToMultiByte',args=[936,flags,src,count,0 if size==0 else out+4,size,0,used_ptr if used else 0],result=status))
        cases.append(dict(label=label,source_hex=source.hex(),flags=flags,length=count,capacity=size,used_default_pointer=used,result_offset=data.offset(status),buffer_offset=data.offset(out),buffer_size=max(4,size)+8,used_offset=data.offset(used_ptr)))
    for char in list(range(128))+[0x20ac,0xf8f5]:add('single-'+str(char),char.to_bytes(2,'little'),used=True)
    boundary=json.loads((ROOT/'evidence/stage10-checkpoint-city-lcmap804.json').read_text())['first_actual_refusal']
    source=bytes.fromhex(boundary['source_hex']);assert len(source)==512
    add('actual-query',source,size=0)
    add('actual-output',source,size=256,used=True)
    add('actual-short-output',source,size=1)
    add('negative-null-terminated','Aa\0Z'.encode('utf-16le'),n=-1)
    add('zero-input-length',b'A\0',n=0)
    add('negative-capacity',b'A\0',size=-1)
    add('invalid-flags',b'A\0',flags=0xffffffff)
    meta=build(NAME,data,ops);(ROOT/f'evidence/stage10-{NAME}-cases.json').write_text(json.dumps(cases,indent=2)+'\n');return dict(sha256=meta['sha256'],size=meta['size'],case_count=len(cases))

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
        for case in json.loads((ROOT/f'evidence/stage10-{NAME}-cases.json').read_text()):
            value,error=struct.unpack_from('<II',raw,case['result_offset']);at=case['buffer_offset']
            rows.append(dict(case,return_value=value,last_error=error,canary_buffer_hex=raw[at:at+case['buffer_size']].hex(),used_default=int.from_bytes(raw[case['used_offset']:case['used_offset']+4],'little')))
        result['rows']=rows
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2))
    if 'rows' in result:print(json.dumps([{k:v for k,v in r.items() if k not in ('source_hex','canary_buffer_hex')}|{'buffer_prefix':r['canary_buffer_hex'][:40]} for r in rows if not r['label'].startswith('single-')],indent=2))

if __name__=='__main__':main()
