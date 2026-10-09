#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='STR16'
def make():
 d=Data();d.alloc(b'STR16v01');ops=[];cases=[]
 for text,count in [('C:\\Emperor\\save-input.sav',260),('ABCDE',0),('ABCDE',1),('ABCDE',3),('ABCDE',6),('',260)]:
  src=d.alloc(text.encode()+b'\0');buf=d.alloc(bytes([0xa5])*269);out=d.alloc(bytes(8));dest=buf+4
  ops.append(dict(name='lstrcpynA',args=[dest,src,count],result=out));cases.append(dict(text=text,count=count,dest=dest,buffer_offset=d.offset(buf),offset=d.offset(out)))
 build(NAME,d,ops);(ROOT/'evidence/stage16-string-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage16-string-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,returns_destination=value==c['dest'],return_value=value,last_error=error,buffer_hex=raw[c['buffer_offset']:c['buffer_offset']+269].hex()))
 result['rows']=rows;(ROOT/'evidence/stage16-string-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
