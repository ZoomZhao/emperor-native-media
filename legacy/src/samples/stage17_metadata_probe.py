#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='META17OK'
def make():
 d=Data();d.alloc(b'META17v1');path=d.alloc(b'C:\\EmperorRotMK[ZeaS]\\Save\\zoom\\ST9BASE.sav\0');handle=d.alloc(bytes(8));ops=[dict(name='CreateFileA',args=[path,0x80000000,3,0,3,0x80,0],result=handle)];cases=[]
 buffers=[d.alloc(bytes([0xa5])*8) for _ in range(3)];out=d.alloc(bytes(8));ops.append(dict(name='GetFileTime',args=[{'load':handle}]+buffers,result=out));cases.append(dict(name='GetFileTime',offset=d.offset(out),buffers=[d.offset(v) for v in buffers],size=8))
 out=d.alloc(bytes(8));ops.append(dict(name='GetFileSize',args=[{'load':handle},0],result=out));cases.append(dict(name='GetFileSize',offset=d.offset(out)))
 out=d.alloc(bytes(8));ops.append(dict(name='GetFileAttributesA',args=[path],result=out));cases.append(dict(name='GetFileAttributesA',offset=d.offset(out)))
 for ticks in [0,133536384001234567,134357416891234567,0x8000000000000000]:
  for name,size in [('FileTimeToLocalFileTime',8),('FileTimeToSystemTime',16)]:
   source=d.alloc(struct.pack('<Q',ticks));buf=d.alloc(bytes([0xa5])*size);out=d.alloc(bytes(8));ops.append(dict(name=name,args=[source,buf],result=out));cases.append(dict(name=name,ticks=ticks,offset=d.offset(out),buffers=[d.offset(buf)],size=size))
 out=d.alloc(bytes(8));ops.append(dict(name='CloseHandle',args=[{'load':handle}],result=out))
 build(NAME,d,ops);(ROOT/'evidence/stage17-metadata-cases-ok.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage17-metadata-cases-ok.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error,buffer_hex=[raw[v:v+c['size']].hex() for v in c.get('buffers',[])]))
 result['rows']=rows;(ROOT/'evidence/stage17-metadata-Wine-result-ok.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
