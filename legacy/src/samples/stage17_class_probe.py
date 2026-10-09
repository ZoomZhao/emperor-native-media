#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='CLS17'
def make():
 p=ROOT/'runs/stage12-state-metadata';state=json.loads((p/'state.json').read_text());memory=(p/'memory.bin').read_bytes()
 def read(at,n):return next(memory[s['offset']+at-s['low']:s['offset']+at-s['low']+n] for s in state['regions'] if s['low']<=at and at+n-1<=s['high'])
 node=state['registers']['esi'];nodes=[];seen=set()
 while node:
  assert node not in seen and len(nodes)<4096;seen.add(node);fields=struct.unpack('<6I',read(node,24));name=read(fields[0],64).split(b'\0')[0].decode('ascii');assert name and all(c.isalnum() or c=='_' for c in name);nodes.append(dict(node=hex(node),name=name,create_function=hex(fields[3]),next=hex(fields[5])));node=fields[5]
 d=Data();d.alloc(b'CLS17v01');ops=[];cases=[]
 pairs=[('cHousingGoal',n['name']) for n in nodes]+[(n['name'],n['name']) for n in nodes]+[('cHousingGoal','CHousingGoal'),('cHousingGoal','chousinggoal')]
 for first,second in pairs:
  a=d.alloc(first.encode()+b'\0');b=d.alloc(second.encode()+b'\0');out=d.alloc(bytes(8));ops.append(dict(name='lstrcmpA',args=[a,b],result=out));cases.append(dict(first=first,second=second,offset=d.offset(out)))
 build(NAME,d,ops);(ROOT/'evidence/stage17-class-cases.json').write_text(json.dumps(dict(registry_nodes=nodes,source_checkpoint='metadata',cases=cases),indent=2)+'\n')
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.loads((ROOT/'evidence/stage17-class-cases.json').read_text())['cases']:
  value,error=struct.unpack_from('<iI',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;(ROOT/'evidence/stage17-class-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(comparisons=len(rows),all_equality_matches_bytes=all((r['return_value']==0)==(r['first']==r['second']) for r in rows))))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
