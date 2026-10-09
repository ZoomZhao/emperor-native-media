#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='VOL16'
def make():
 d=Data();d.alloc(b'VOL16v01');ops=[];cases=[]
 for root in ['C:\\',None]:
  path=d.alloc(root.encode()+b'\0') if root is not None else 0;maximum=d.alloc(bytes([0xa5])*4);flags=d.alloc(bytes([0xa5])*4);out=d.alloc(bytes(8));ops.append(dict(name='GetVolumeInformationA',args=[path,0,0,0,maximum,flags,0,0],result=out));cases.append(dict(root=root,maximum_offset=d.offset(maximum),flags_offset=d.offset(flags),offset=d.offset(out)))
 build(NAME,d,ops);(ROOT/'evidence/stage16-volume-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage16-volume-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);maximum=struct.unpack_from('<I',raw,c['maximum_offset'])[0];flags=struct.unpack_from('<I',raw,c['flags_offset'])[0];rows.append(dict(c,return_value=value,last_error=error,maximum_component=maximum,flags=hex(flags)))
 result['rows']=rows;(ROOT/'evidence/stage16-volume-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
