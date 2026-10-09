#!/usr/bin/env python3
"""Own callback registration replacement/null sequence, no exception injection."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run
NAME='FILTER'

def make():
    data=Data();data.alloc(b'FILTER10');operations=[]
    for pointer in [0,0x40f000,0x40f000,0,0]:
        result=data.alloc(bytes(8));operations.append(dict(name='SetUnhandledExceptionFilter',args=[pointer],result=result))
    meta=build(NAME,data,operations,callback_stub=True);return dict(sha256=meta['sha256'],size=meta['size'])

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        data=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();result['rows']=[]
        for i,pointer in enumerate([0,0x40f000,0x40f000,0,0]):
            previous,error=struct.unpack_from('<II',data,8+i*8);result['rows'].append(dict(callback=hex(pointer),previous=hex(previous),last_error=error))
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
