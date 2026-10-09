#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='MSG13'
def make():
 d=Data();d.alloc(b'MSG13v01');ops=[];cases=[]
 for text in ['commctrl_DragListMsg','commctrl_DragListMsg','COMMCTRL_DRAGLISTMSG','ST13_Alpha','ST13_Beta','st13_alpha','']:
  p=d.alloc(text.encode()+b'\0');out=d.alloc(bytes(8));ops.append(dict(name='RegisterWindowMessageA',args=[p],result=out));cases.append(dict(text=text,offset=d.offset(out)))
 meta=build(NAME,d,ops,dll_for={'RegisterWindowMessageA':'USER32.dll'});(ROOT/'evidence/stage13-message-cases.json').write_text(json.dumps(cases,indent=2)+'\n');print(meta['sha256'])
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.load(open(ROOT/'evidence/stage13-message-cases.json')):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;(ROOT/'evidence/stage13-message-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
