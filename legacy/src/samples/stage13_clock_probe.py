#!/usr/bin/env python3
import argparse,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='CLOCK13'
def make():
 d=Data();d.alloc(b'CLK13v01');ops=[];cases=[]
 def add(name,args,label,buf=None):
  at=d.alloc(bytes(8));ops.append(dict(name=name,args=args,result=at));cases.append(dict(name=name,args=args,label=label,offset=d.offset(at),buffer_offset=d.offset(buf) if buf else None))
 for name,n in [('GetLocalTime',16),('GetSystemTime',16),('GetSystemTimeAsFileTime',8),('GetTimeZoneInformation',172),('QueryPerformanceFrequency',8),('QueryPerformanceCounter',8)]:
  for fill in ([0xa5,0x5a] if name=='GetTimeZoneInformation' else [0xa5]):
   buf=d.alloc(bytes([0xcd])*4+bytes([fill])*n+bytes([0xcd])*4);add(name,[buf+4],name+'-'+str(fill),buf);cases[-1].update(size=n,fill=fill)
 add('GetTickCount',[],'tick-before');add('Sleep',[30],'sleep-30');add('GetTickCount',[],'tick-after')
 buf=d.alloc(bytes([0xa5])*8);add('QueryPerformanceCounter',[buf],'counter-after',buf);cases[-1].update(size=8,unguarded=True)
 meta=build(NAME,d,ops);(ROOT/'evidence/stage13-clock-cases.json').write_text(json.dumps(cases,indent=2)+'\n');print(json.dumps(dict(sha256=meta['sha256'],case_count=len(cases))))
def capture():
 before_ns=time.time_ns();before_monotonic_ns=time.monotonic_ns();result=run(NAME);after_ns=time.time_ns();after_monotonic_ns=time.monotonic_ns();raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for case in json.loads((ROOT/'evidence/stage13-clock-cases.json').read_text()):
  value,error=struct.unpack_from('<II',raw,case['offset']);row=dict(case,return_value=value,last_error=error)
  if case['buffer_offset'] is not None:
   p=case['buffer_offset'];body=raw[p:p+case['size']] if case.get('unguarded') else raw[p+4:p+4+case['size']];row['buffer_hex']=body.hex()
   if not case.get('unguarded'):assert raw[p:p+4]==raw[p+4+case['size']:p+8+case['size']]==bytes([0xcd])*4
  rows.append(row)
 result.update(rows=rows,host_wall_before_ns=before_ns,host_wall_after_ns=after_ns,host_monotonic_before_ns=before_monotonic_ns,host_monotonic_after_ns=after_monotonic_ns);(ROOT/'evidence/stage13-clock-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='rows'}))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
