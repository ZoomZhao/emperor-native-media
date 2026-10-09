#!/usr/bin/env python3
"""Bounded static windows. External callees/indirect jumps remain unknown."""
import collections,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];EXE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'
WINDOWS=[(0x776f00,0x779a00),(0x77d2db,0x77d460),(0x77facf,0x77fb6b),(0x78209a,0x782550),(0x534a30,0x534c20),(0x408c70,0x408c77),(0x4081f0,0x4081f6),(0x40d170,0x40d185),(0x41f1e0,0x41f20b),(0x41f810,0x41f900),(0x4b08a0,0x4b0b00)]
def main():
 listings=[]
 for lo,hi in WINDOWS:
  p=subprocess.run(['xcrun','llvm-objdump','--disassemble',f'--start-address={hex(lo)}',f'--stop-address={hex(hi)}',EXE],capture_output=True,text=True,timeout=10);assert p.returncode==0;listings.append(p.stdout)
 text='\n'.join(listings);(ROOT/'evidence/stage14-original-reviewed-windows.disassembly.log').write_text(text)
 ins={}
 for line in text.splitlines():
  m=re.match(r'\s*([0-9a-f]+): ([0-9a-f ]+)\t(.+)',line)
  if not m:continue
  pc=int(m[1],16);size=len(m[2].split());asm=m[3].split('#')[0].strip();parts=asm.split(None,1);ins[pc]=dict(next=pc+size,op=parts[0],args=parts[1] if len(parts)>1 else '',asm=asm)
 meta=json.loads((ROOT/'evidence/pe-metadata.json').read_text())[0];iat={int(s['iat_va'],16):dict(dll=d['dll'],name=s.get('name',f"ordinal:{s.get('ordinal')}")) for d in meta['imports'] for s in d['symbols']}
 table=json.loads((ROOT/'evidence/stage14-CRT-tables.json').read_text())['tables'][1]['rows'];roots=[int(r['target'],16) for r in table if int(r['slot'],16)>=0x815710 and r['target']!='0x0']+[0x534a30]
 reports=[]
 for root in roots:
  queue=collections.deque([root]);seen=set();apis=[];outside=set();unresolved=[];global_addresses=set()
  while queue and len(seen)<2000:
   pc=queue.popleft()
   if pc in seen:continue
   if pc not in ins:outside.add(pc);continue
   seen.add(pc);item=ins[pc];op=item['op'];args=item['args'];branch=re.search(r'0x([0-9a-f]+)',args);target=int(branch[1],16) if branch else None
   # Absolute IAT calls and IAT loads identify known API dependencies;
   # register calls remain unresolved rather than claiming a complete graph.
   for address in re.findall(r'(?<![\w])([0-9]{6,})(?![\w])',args):
    v=int(address)
    if v in iat:apis.append(dict(at=hex(pc),**iat[v],evidence='absolute IAT slot used by call or register load'))
    elif 0x400000<=v<0x2000000:global_addresses.add(v)
   if op.startswith('ret'):continue
   if op.startswith('jmp'):
    if target is not None:queue.append(target)
    else:unresolved.append(dict(at=hex(pc),asm=item['asm']))
    continue
   if op.startswith('j') and target is not None:queue.append(target)
   if op.startswith('call'):
    if target is not None:queue.append(target)
    elif '*' in args and not re.search(r'\*[0-9]+',args):unresolved.append(dict(at=hex(pc),asm=item['asm']))
   queue.append(item['next'])
  reports.append(dict(root=hex(root),reviewed_instructions=len(seen),API_dependencies=apis,external_callees_or_branches=[hex(v) for v in sorted(outside)],unresolved_indirect_controls=unresolved,absolute_image_addresses=[hex(v) for v in sorted(global_addresses)],complete_dependency_closure=False))
 result=dict(EXE_sha256=hashlib.sha256(Path(EXE).read_bytes()).hexdigest(),original_not_executed=True,windows=[[hex(v) for v in w] for w in WINDOWS],remaining_Cxx_constructor_roots=30,city_root='0x534a30',reports=reports,safe_initializer_separation_proven=False,scope='bounded CFG over trusted objdump listing; IAT load records are conservative candidates, unknown callees/indirect controls prevent a full dependency claim')
 (ROOT/'evidence/stage14-static-dependencies.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(roots=len(reports),API_names=sorted({a['name'] for r in reports for a in r['API_dependencies']}),safe_initializer_separation_proven=False)))
if __name__=='__main__':main()
