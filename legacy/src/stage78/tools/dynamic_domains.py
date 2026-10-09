"""CFG reaching IAT definitions, explicit conditional ABI/runtime obligations.

Code-table membership is never an allowed-target proof. Reviewed CRT bounds
remain reviewed assembly facts; this tool checks their bytes/table entries but
does not silently upgrade them to a machine-checked index-range proof.
"""
import collections,hashlib,json,re,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
REGS=('eax','ecx','edx','ebx','esp','ebp','esi','edi');UNKNOWN=frozenset([None])
def main():
 c=json.loads((ROOT/'stage76/evidence/full-dependency-closure-v1.json').read_text());nodes={q['pc']:q for q in c['instructions']};pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()==c['original_sha256']
 jd=json.loads((ROOT/'stage75/configs/crt-jump-domains.json').read_text())['domains'];jump={int(k,0):v for k,v in jd.items() if int(k,0) in nodes}
 def successor(q):
  m=q['mnemonic'];pc=q['pc']
  if m=='ret':return []
  if m=='jmp':
   if pc in jump:return [int(x['target'],0) for x in jump[pc]['rows']]
   z=re.match(r'0x([0-9a-f]+)',q['operand']);return [int(z[1],16)] if z else []
  if m.startswith('j'):
   z=re.match(r'0x([0-9a-f]+)',q['operand']);return [q['end'],int(z[1],16)] if z else []
  return [q['end']]
 imports={}
 opt=struct.unpack_from('<I',pe.data,60)[0]+24;rva=struct.unpack_from('<I',pe.data,opt+104)[0]
 def string(a):
  buf=bytearray()
  for k in range(512):
   v=pe.read(a+k,1)[0]
   if not v:return buf.decode('ascii')
   buf.append(v)
  raise ValueError('string budget')
 for k in range(100):
  oft,t,f,name,iat=struct.unpack('<5I',pe.read(pe.base+rva+k*20,20))
  if not any((oft,t,f,name,iat)):break
  dll=string(pe.base+name)
  for j in range(4096):
   v=struct.unpack('<I',pe.read(pe.base+(oft or iat)+j*4,4))[0]
   if not v:break
   if not v&0x80000000:imports[pe.base+iat+j*4]=dll+'.'+string(pe.base+v+2)
 candidates={pc:q['operand'] for pc,q in nodes.items() if q['mnemonic']=='call' and q['operand'] in ('esi','ebx','ebp')};proofs=collections.defaultdict(list);evals=0
 for entry in map(lambda x:int(x,0),c['function_entries']):
  for reg in sorted(set(candidates.values())):
   states={entry:UNKNOWN};todo=collections.deque([entry]);queued={entry}
   while todo:
    pc=todo.popleft();queued.remove(pc);evals+=1
    if evals>200000:raise RuntimeError('explicit CFG evaluation budget; no proof emitted')
    if pc not in nodes:continue
    q=nodes[pc];v=states[pc];m=q['mnemonic'];op=q['operand'];dest=op.split(',',1)[0].strip();load=re.fullmatch(reg+r', dword ptr \[(\d+)\]',op)
    if m=='mov' and load and int(load[1]) in imports:v=frozenset([(pc,int(load[1]))])
    elif m=='leave' and reg=='ebp':v=UNKNOWN
    elif m not in ('push','cmp','test','call','ret','jmp','rep','repne','std','cld') and not m.startswith('j') and dest in {'esi':('esi','si'),'ebx':('ebx','bx','bh','bl'),'ebp':('ebp','bp')}[reg]:v=UNKNOWN
    elif m in ('rep','repne') and reg=='esi' and op.startswith('movs'):v=UNKNOWN
    if pc in candidates and candidates[pc]==reg:proofs[pc].append(dict(entry=hex(entry),reaching_definitions=v))
    for to in successor(q):
     prior=states.get(to);merged=v if prior is None else prior|v
     if merged!=prior:
      states[to]=merged
      if to not in queued:todo.append(to);queued.add(to)
 results=[]
 for pc,reg in sorted(candidates.items()):
  evidence=proofs[pc];union=frozenset().union(*(r['reaching_definitions'] for r in evidence)) if evidence else UNKNOWN
  slots={x[1] for x in union if x is not None};proved=None not in union and len(slots)==1
  results.append(dict(PC=hex(pc),register=reg,all_CFG_paths_have_single_IAT_source=proved,slot=hex(next(iter(slots))) if proved else None,API=imports[next(iter(slots))] if proved else None,load_PCs=sorted(hex(x[0]) for x in union if x is not None),CFG_entry_count=len(evidence),nonvolatile_register_preservation_across_local_and_platform_calls_is_obligation=True,IAT_value_and_legal_trampoline_binding_verified_at_runtime=False,unconditional_legal_target_proof=False))
 verified=[]
 for pc,row in sorted(jump.items()):
  assert nodes[pc]['bytes']==row['original_instruction_bytes']
  for r in row['rows']:assert struct.unpack('<I',pe.read(int(r['slot'],0),4))[0]==int(r['target'],0) and int(r['target'],0) in nodes
  verified.append(dict(PC=hex(pc),table= row['table'],targets=sorted(set(x['target'] for x in row['rows'])),table_bytes_and_instruction_boundaries_verified=True,index_range_proof=row['proof_scope'],machine_checked_all_path_index_range=False,runtime_table_immutability_verified=False))
 unique_calls={q['pc'] for q in c['indirect_edges'] if q['kind']=='indirect_call'}
 prior_named=set(int(x,0) for api in json.loads((ROOT/'stage76/evidence/full-audit.json').read_text())['expanded_named_APIs'] for x in api['sites'])
 new=[r for r in results if r['all_CFG_paths_have_single_IAT_source'] and int(r['PC'],0) not in prior_named]
 out=dict(status='DYNAMIC_TARGET_DOMAINS_OPEN_FAIL_CLOSED',unique_indirect_CALL_sites=len(unique_calls),conditional_register_IAT_source_proofs=sum(r['all_CFG_paths_have_single_IAT_source'] for r in results),new_conditional_named_API_sites=len(new),remaining_non_IAT_CALL_sites_after_conditional_classification=len(unique_calls-prior_named)-len(new),CFG_evaluations=evals,register_IAT_sources=results,reviewed_jump_domains=verified,fully_machine_proved_per_site_domains=0,structural_code_bindings_are_legal_target_proof=False,unknown_target_policy='reject before native lookup; no fallback to executable/code-table membership',game_executions=0)
 (ROOT/'stage78/evidence/dynamic-domains.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items() if k not in ['register_IAT_sources','reviewed_jump_domains']}))
if __name__=='__main__':main()
