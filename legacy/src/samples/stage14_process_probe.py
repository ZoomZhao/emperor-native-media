#!/usr/bin/env python3
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
def make(name,os_version,subsystem):
 d=Data();d.alloc(b'PV14v001');ops=[];cases=[];pid=d.alloc(bytes(8));ops.append(dict(name='GetCurrentProcessId',args=[],result=pid));cases.append(dict(kind='current_process_id',offset=d.offset(pid)))
 for kind,p in [('zero',0),('repeat',0),('actual_current',{'load':pid}),('invalid',0xffffffff)]:
  out=d.alloc(bytes(8));ops.append(dict(name='GetProcessVersion',args=[p],result=out));cases.append(dict(kind=kind,offset=d.offset(out)))
 meta=build(name,d,ops);p=ROOT/f'samples/stage10-{name}.exe';body=bytearray(p.read_bytes());struct.pack_into('<HH',body,0x98+40,*os_version);struct.pack_into('<HH',body,0x98+48,*subsystem);p.write_bytes(body);meta['sha256']=hashlib.sha256(body).hexdigest();meta['OS_header_version']=os_version;meta['subsystem_header_version']=subsystem;(ROOT/f'evidence/stage10-{name}-source.json').write_text(json.dumps(meta,indent=2)+'\n');(ROOT/f'evidence/stage14-{name}-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture(name):
 result=run(name);raw=(ROOT/f'evidence/stage10-{name}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/f'evidence/stage14-{name}-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;(ROOT/f'evidence/stage14-{name}-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);p.add_argument('name',choices=['PV14A','PV14B']);a=p.parse_args()
 if a.action=='build':make(a.name,(3,7) if a.name=='PV14A' else (5,9),(4,0) if a.name=='PV14A' else (6,3))
 else:capture(a.name)
