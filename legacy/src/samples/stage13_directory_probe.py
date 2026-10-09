#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='DIR13'
def make():
 d=Data();d.alloc(b'DIR13v01');ops=[];cases=[]
 for label,path in [('existing','Cities\\'),('fresh','ST13ProbeDir'),('repeat','ST13ProbeDir'),('trailing','ST13ProbeDir\\'),('parent-missing','ST13AbsentParent\\child')]:
  p=d.alloc(path.encode()+b'\0');out=d.alloc(bytes(8));ops.append(dict(name='CreateDirectoryA',args=[p,0],result=out));cases.append(dict(label=label,path=path,offset=d.offset(out)))
 meta=build(NAME,d,ops);(ROOT/'evidence/stage13-directory-cases.json').write_text(json.dumps(cases,indent=2)+'\n');print(meta['sha256'])
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage13-directory-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;(ROOT/'evidence/stage13-directory-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
