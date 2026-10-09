"""Entry/block-aligned static LLVM disassembly of the fixed PE; never executes it.

Jump-table runs are structural supersets, not proofs of index bounds. The
result retains every unresolved dynamic edge and decode/budget gap.
"""
import argparse,collections,hashlib,json,re,struct,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
EXE=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
LLVM='<LOCAL_MACHINE_PATH>'
PAT=re.compile(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*([a-z][a-z0-9]*)\s*(.*)$')
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--config');p.add_argument('--jump-domains');p.add_argument('--inventory',action='store_true');p.add_argument('--structural-jumps',action='store_true');p.add_argument('--seconds',type=float,default=40);p.add_argument('--cap',type=int,default=18000);a=p.parse_args()
 out=(ROOT/a.output).resolve();assert out.is_relative_to(ROOT/'stage76');assert not out.exists()
 pe=PE(str(EXE));assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 cfg=json.loads((ROOT/a.config).read_text())if a.config else {};roots=[int(x,0)for x in cfg.get('roots',['0x4aeeb0'])]
 if a.inventory:
  vt=json.loads((ROOT/'stage75/evidence/vtable-inventory.json').read_text());roots+=list({int(x,0)for x in vt['candidate_target200']+vt['candidate_target612']})
 roots=sorted(set(roots));declined={int(x,0)for x in cfg.get('boundary_functions',{})};resolved=cfg.get('resolved_indirect_edges',{});proved=cfg.get('proved_branch_targets',{});jumpdomains=json.loads((ROOT/a.jump_domains).read_text())['domains']if a.jump_domains else{}
 start=time.monotonic();queue=collections.deque(roots);nodes={};blocks={};edges=[];gaps=[];tables=[];processes=0;entries=set(roots)
 def iscode(pc):
  s=pe.section(pc);return bool(s and s['flags']&0x20000000)
 def decode(pc):
  nonlocal processes
  args=[LLVM,'--disassemble','--x86-asm-syntax=intel','--start-address='+hex(pc),'--stop-address='+hex(pc+768),str(EXE)]
  q=subprocess.run(args,capture_output=True,text=True,timeout=5);processes+=1
  if q.returncode:raise ValueError(q.stderr[:256])
  rows=[];at=pc
  for line in q.stdout.splitlines():
   m=PAT.match(line)
   if not m:continue
   here=int(m[1],16)
   if here<pc:continue
   if here!=at:break
   b=bytes.fromhex(m[2]);assert pe.read(here,len(b))==b
   row=dict(pc=here,end=here+len(b),bytes=b.hex(),mnemonic=m[3],operand=m[4].split('#')[0].strip());rows.append(row);at=row['end']
   if row['mnemonic'].startswith(('j','ret','call'))or row['mnemonic']in ('int3','ud2','hlt'):break
  if not rows:raise ValueError('no aligned instruction at '+hex(pc))
  return rows
 while queue:
  pc=queue.popleft()
  if pc in nodes or pc in declined:continue
  if time.monotonic()-start>a.seconds or len(nodes)>=a.cap:
   gaps.append(dict(kind='analysis_budget',PC=hex(pc),pending=len(queue)+1));queue.appendleft(pc);break
  if not iscode(pc):gaps.append(dict(kind='outside_original_text',PC=hex(pc)));continue
  try:rows=decode(pc)
  except (ValueError,subprocess.TimeoutExpired)as error:gaps.append(dict(kind='aligned_decode_failure',PC=hex(pc),reason=str(error)));continue
  blocks[pc]=[r['pc']for r in rows]
  for r in rows:
   if r['pc']in nodes:
    assert nodes[r['pc']]==r
   nodes[r['pc']]=r
  r=rows[-1];mn=r['mnemonic'];m=re.match(r'^0x([0-9a-f]+)(?:\s|$)',r['operand']);target=int(m[1],16)if m else None;nexts=[]
  if mn.startswith('ret'):pass
  elif mn in ('int3','ud2','hlt'):edges.append(dict(pc=r['pc'],kind='trap',operand=mn))
  elif mn=='call':
   nexts=[r['end']]
   if target is not None:
    edges.append(dict(pc=r['pc'],kind='direct_call',target=target));entries.add(target)
    if target not in declined:nexts.append(target)
   else:
    proof=resolved.get(hex(r['pc']));row=dict(pc=r['pc'],kind='indirect_call',operand=r['operand'],resolved=bool(proof))
    if proof:row['targets']=[int(proof['target'],0)];nexts+=row['targets'];entries.update(row['targets'])
    edges.append(row)
  elif mn=='jmp':
   if target is not None:nexts=[target];edges.append(dict(pc=r['pc'],kind='direct_jump',target=target))
   else:
    proof=resolved.get(hex(r['pc']));row=dict(pc=r['pc'],kind='indirect_jump',operand=r['operand'],resolved=bool(proof))
    if proof:row['targets']=[int(proof['target'],0)];nexts+=row['targets']
    elif hex(r['pc'])in jumpdomains:
     domain=jumpdomains[hex(r['pc'])];assert r['bytes']==domain['original_instruction_bytes'];targets=[]
     for slotrow in domain['rows']:
      dest=struct.unpack('<I',pe.read(int(slotrow['slot'],0),4))[0];assert dest==int(slotrow['target'],0)and iscode(dest);targets.append(dest)
     row['resolved']=True;row['targets']=sorted(set(targets));row['reviewed_index_domain']=domain;nexts+=row['targets']
    elif a.structural_jumps:
     table=re.fullmatch(r'dword ptr \[4\*(\w+) \+ (\d+)\]',r['operand'])
     if table:
      base=int(table[2]);targets=[]
      for i in range(256):
       try:dest=struct.unpack('<I',pe.read(base+4*i,4))[0]
       except ValueError:break
       if not iscode(dest):break
       targets.append(dest)
      if targets:
       row['structural_targets']=sorted(set(targets));nexts+=row['structural_targets'];tables.append(dict(pc=r['pc'],base=base,index_register=table[1],slots=len(targets),targets=targets,index_bound_proved=False))
    edges.append(row)
  elif mn.startswith('j'):
   proof=proved.get(hex(r['pc']));nexts=[int(proof['target'],0)]if proof else[r['end'],target]
   edges.append(dict(pc=r['pc'],kind='conditional',target=target,fallthrough=r['end'],guarded_always_taken=bool(proof)))
  else:nexts=[r['end']]
  queue.extend(x for x in nexts if x is not None and x not in nodes)
 # Every node is decoded from a real CFG seed. Overlaps can represent deliberate
 # instruction-entry overlap; report them rather than deriving linear counts.
 ranges=sorted((pc,r['end'])for pc,r in nodes.items());overlaps=[]
 for old,new in zip(ranges,ranges[1:]):
  if old[1]>new[0]:overlaps.append([hex(x)for x in old+new])
 dynamic=[e for e in edges if e['kind'].startswith('indirect')];result=dict(status='GUARDED_ALIGNED_CFG_CLOSED'if not gaps and not any(not e['resolved']for e in dynamic)else'ALIGNED_CFG_OPEN',roots=[hex(x)for x in roots],declined_boundaries=[hex(x)for x in sorted(declined)],function_entries=[hex(x)for x in sorted(entries)],instruction_count=len(nodes),code_bytes=sum(r['end']-r['pc']for r in nodes.values()),instructions=[nodes[x]for x in sorted(nodes)],edges=edges,indirect_edges=dynamic,structural_jump_tables=tables,gaps=gaps,pending_PC=[hex(x)for x in list(queue)],instruction_overlaps=overlaps,LLVM_processes=processes,duration_seconds=round(time.monotonic()-start,3),original_sha256=hashlib.sha256(pe.data).hexdigest(),original_game_executed=False,structural_inventory_is_phase8_domain_proof=False)
 out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k]for k in ['status','instruction_count','code_bytes','LLVM_processes','duration_seconds']}|dict(entries=len(entries),dynamic=len(dynamic),structural_tables=len(tables),gaps=len(gaps))))
if __name__=='__main__':main()
