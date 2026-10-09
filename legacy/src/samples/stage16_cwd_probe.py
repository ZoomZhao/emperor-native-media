#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='CWD16'
def make():
 d=Data();d.alloc(b'CWD16v01');ops=[];cases=[]
 def get(label,size):
  buf=d.alloc(bytes([0xa5])*269);out=d.alloc(bytes(8));ops.append(dict(name='GetCurrentDirectoryA',args=[size,buf+4],result=out));cases.append(dict(label=label,name='GetCurrentDirectoryA',size=size,buffer_offset=d.offset(buf),offset=d.offset(out)))
 def setdir(label,path):
  p=d.alloc(path.encode()+b'\0');out=d.alloc(bytes(8));ops.append(dict(name='SetCurrentDirectoryA',args=[p],result=out));cases.append(dict(label=label,name='SetCurrentDirectoryA',path=path,offset=d.offset(out)))
 get('before',261);setdir('empty','');get('after_empty',261);setdir('empty_repeat','');setdir('missing','C:\\ST16NeverExists\\Missing');get('after_missing',261);setdir('dot','.');get('after_dot',261);get('zero_size',0);get('short_size',1)
 build(NAME,d,ops);(ROOT/'evidence/stage16-cwd-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage16-cwd-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);row=dict(c,return_value=value,last_error=error)
  if 'buffer_offset' in c:row['buffer_hex']=raw[c['buffer_offset']:c['buffer_offset']+269].hex()
  rows.append(row)
 result['rows']=rows;(ROOT/'evidence/stage16-cwd-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps([{k:v for k,v in r.items() if k!='buffer_hex'} for r in rows]))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
