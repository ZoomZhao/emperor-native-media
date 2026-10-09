#!/usr/bin/env python3
"""Measure the actual CRT locale-zero casing subset without guessing Unicode."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run

NAME='LCMAP'
def make(locale=0):
    data=Data();data.alloc(b'LCMAP010');ops=[];cases=[]
    mb=json.loads((ROOT/'evidence/stage10-MB936-result.json').read_text())['rows']
    actual=next(r for r in mb if r['label']=='actual-output')
    wide=bytes.fromhex(actual['canary_buffer_hex'])[4:4+512]
    def add(label,source,flags,n=None,size=256):
        src=data.alloc(source);out=data.alloc(bytes([0xa5])*(max(4,size)*2+8));status=data.alloc(bytes(8))
        args=[locale,flags,src,len(source)//2 if n is None else n,0 if size==0 else out+4,size]
        ops.append(dict(name='LCMapStringW',args=args,result=status));cases.append(dict(label=label,LCID=locale,flags=flags,source_hex=source.hex(),length=len(source)//2 if n is None else n,capacity=size,result_offset=data.offset(status),buffer_offset=data.offset(out),buffer_size=max(4,size)*2+8))
    for flags in [0x100,0x200]:
        add('actual-query-'+str(flags),wide,flags,size=0)
        add('actual-output-'+str(flags),wide,flags)
        add('actual-short-'+str(flags),wide,flags,size=1)
        add('nul-query-'+str(flags),b'\0\0',flags,size=0)
        add('nul-output-'+str(flags),b'\0\0',flags,size=1)
        add('negative-'+str(flags),'Aa\0Z'.encode('utf-16le'),flags,n=-1,size=4)
    add('invalid-flags',b'A\0',0xffffffff,size=4)
    add('zero-source-count',b'A\0',0x100,n=0,size=4)
    meta=build(NAME,data,ops);(ROOT/f'evidence/stage10-{NAME}-cases.json').write_text(json.dumps(cases,indent=2)+'\n');return dict(sha256=meta['sha256'],size=meta['size'],case_count=len(cases))

def main():
    global NAME
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);p.add_argument('--locale',type=lambda x:int(x,0),default=0);a=p.parse_args()
    assert a.locale in (0,0x804)
    if a.locale:NAME='LCMAP804'
    if a.action=='build':print(json.dumps(make(a.locale),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
        for case in json.loads((ROOT/f'evidence/stage10-{NAME}-cases.json').read_text()):
            value,error=struct.unpack_from('<II',raw,case['result_offset']);at=case['buffer_offset']
            rows.append(dict(case,return_value=value,last_error=error,canary_buffer_hex=raw[at:at+case['buffer_size']].hex()))
        result['rows']=rows
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2))
    if 'rows' in result:print(json.dumps([{k:v for k,v in r.items() if k not in ('source_hex','canary_buffer_hex')}|{'buffer_prefix':r['canary_buffer_hex'][:48]} for r in rows],indent=2))

if __name__=='__main__':main()
