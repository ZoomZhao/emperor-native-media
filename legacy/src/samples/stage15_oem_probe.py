#!/usr/bin/env python3
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='OEM15'
def make():
 d=Data();d.alloc(b'OEM15v01');ops=[];cases=[]
 def op(name,args):
  out=d.alloc(bytes(8));ops.append(dict(name=name,args=args,result=out));cases.append(dict(name=name,offset=d.offset(out)));return out
 oem=op('GetOEMCP',[]);op('GetOEMCP',[]);op('GetACP',[]);buf=d.alloc(bytes([0xa5])*20);op('GetCPInfo',[{'load':oem},buf]);build(NAME,d,ops);(ROOT/'evidence/stage15-oem-cases.json').write_text(json.dumps(dict(cases=cases,buffer_offset=d.offset(buf)),indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();cases=json.loads((ROOT/'evidence/stage15-oem-cases.json').read_text());rows=[]
 for c in cases['cases']:
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result.update(rows=rows,CPINFO_hex=raw[cases['buffer_offset']:cases['buffer_offset']+20].hex());assert rows[0]['return_value']==rows[1]['return_value'];(ROOT/'evidence/stage15-oem-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
