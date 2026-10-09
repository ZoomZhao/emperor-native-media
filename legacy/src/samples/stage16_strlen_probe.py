#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='STRLEN16'
def make():
 d=Data();d.alloc(b'STRLEN16');ops=[];cases=[]
 for text in ['C:\\Emperor\\save-input.sav','ABCDE','']:
  src=d.alloc(text.encode()+b'\0');out=d.alloc(bytes(8));ops.append(dict(name='lstrlenA',args=[src],result=out));cases.append(dict(text=text,offset=d.offset(out)))
 build(NAME,d,ops);(ROOT/'evidence/stage16-strlen-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage16-strlen-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;(ROOT/'evidence/stage16-strlen-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
