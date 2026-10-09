#!/usr/bin/env python3
"""Own finite read-only GUI-contract probe, no game or window creation."""
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='GUI14'
def make():
 d=Data();d.alloc(b'GUI14v01');ops=[];cases=[]
 def op(name,args,kind):
  out=d.alloc(bytes(8));ops.append(dict(name=name,args=args,result=out));cases.append(dict(kind=kind,name=name,offset=d.offset(out)));return out
 for metric in [11,12,2,3,0,1]:op('GetSystemMetrics',[metric],f'metric_{metric}')
 dc=op('GetDC',[0],'screen_DC')
 for cap in [88,90]:op('GetDeviceCaps',[{'load':dc},cap],f'caps_{cap}')
 op('ReleaseDC',[0,{'load':dc}],'release_screen_DC')
 for resource in [32514,32512]:op('LoadCursorA',[0,resource],f'cursor_{resource}')
 for color in [15,16,20,18,6]:op('GetSysColor',[color],f'color_{color}')
 for brush in [15,6]:op('GetSysColorBrush',[brush],f'brush_{brush}')
 build(NAME,d,ops,dll_for={'GetSystemMetrics':'USER32.dll','GetDC':'USER32.dll','ReleaseDC':'USER32.dll','LoadCursorA':'USER32.dll','GetSysColor':'USER32.dll','GetSysColorBrush':'USER32.dll','GetDeviceCaps':'GDI32.dll'})
 (ROOT/'evidence/stage14-gui-cases.json').write_text(json.dumps(cases,indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage14-gui-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;result['native_backend_implemented']=False;result['no_window_created_by_probe']=True
 (ROOT/'evidence/stage14-gui-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
