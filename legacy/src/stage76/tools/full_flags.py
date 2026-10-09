"""Byte-checked LLVM CFG flag provenance for the full static dependency graph.

Compiler annotations are generated only from identified original producers.
Unknown dynamic calls and undefined flag reads stay explicit. No paths pruned.
"""
import argparse,collections,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage76';sys.path.insert(0,str(ROOT/'stage74/tools'))
from analysis_full_base import PE,Analysis as Base,CC_FLAGS
CC={'o':0,'no':1,'b':2,'ae':3,'e':4,'ne':5,'be':6,'a':7,'s':8,'ns':9,'p':10,'np':11,'l':12,'ge':13,'le':14,'g':15}
class Analysis(Base):
 def successors(self,ins):
  if ins["control"]=="jmp_indirect":
   domain=self.config["jump_domains"].get(hex(ins["pc"]))
   return sorted({int(r["target"],0)for r in domain["rows"]})if domain else []
  return super().successors(ins)
 def scan(self):
  self.functions=set(int(x,0)for x in self.config['full']['function_entries']);proofs=self.config['import_proofs']
  for source in self.config['full']['instructions']:
   r=dict(source);pc=r['pc'];assert self.pe.read(pc,r['end']-pc).hex()==r['bytes'];mn=r['mnemonic'];operand=r['operand'];target=re.match(r'^0x([0-9a-f]+)',operand);target=int(target[1],16)if target else None
   r.update(name=mn,args=[],defines=0,undefined=0,control=None,target=target)
   if mn.startswith('j')and mn!='jmp':r.update(name='jcc',control='jcc',args=[dict(kind='imm',value=CC[mn[1:]])])
   elif mn.startswith('set'):r.update(name='setcc',args=[dict(kind='imm',value=CC[mn[3:]])])
   elif mn=='call':r['control']='call'if target is not None else'call_indirect'
   elif mn=='jmp':r['control']='jmp'if target is not None else'jmp_indirect'
   elif mn.startswith('ret'):r['control']='ret'
   if mn in ('add','sub','cmp','neg','sbb','adc'):r['defines']=0x3f
   elif mn in ('inc','dec'):r['defines']=0x3e
   elif mn in ('test','and','or','xor'):r.update(defines=0x3b,undefined=4)
   elif mn in ('imul','mul'):r.update(defines=0x21,undefined=0x1e)
   elif mn in ('idiv','div'):r['undefined']=0x3f
   elif mn in ('shl','shr','sar'):
    count=operand.rsplit(',',1)[-1].strip()if','in operand else'1'
    if count=='cl':r.update(defines=0x3b,undefined=0x24,conditional_effect=True)
    else:
     n=int(count,0)&31
     if n:r.update(defines=0x3b if n==1 else 0x1b,undefined=4 if n==1 else 0x24)
   elif mn in ('rep','repne')and operand.startswith(('scas','cmps')):r.update(defines=0x3f,conditional_effect=True)
   elif mn not in {'mov','lea','push','pop','movsx','movzx','leave','ret','call','jmp','cdq','cld','std','not','rep','repne'}and not(mn.startswith('j')or mn.startswith('set')):self.record_unknown('unknown_flag_semantics',pc,mnemonic=mn)
   if mn in ('sbb','adc'):r['arithmetic_flag_read_mask']=1
   if mn=='call':
    q=re.fullmatch(r'dword ptr \[(\d+)\]',operand);slot=int(q[1])if q else int(proofs[hex(pc)]['slot'],0)if hex(pc)in proofs else None
    # Fixed reference API ret() changes EAX/ESP, not the CPU EFLAGS. This is
    # compiler dataflow for that reference shim; it is not a completed native
    # implementation or a universal Windows API guarantee.
    if slot in self.config['reference_import_slots']:r['reference_API_preserves_flags']=True
   self.instructions[pc]=r
 def flag_summary(self,entry,summaries):
  incoming=tuple(frozenset(['incoming'])for _ in range(6));states={entry:incoming};q=collections.deque([entry]);returns=[];budget=0
  while q:
   budget+=1
   if budget>100000:self.record_unknown('flag_fixpoint_budget',entry);break
   pc=q.popleft()
   if pc not in self.instructions:self.record_unknown('flags_unknown_successor',pc);continue
   ins=self.instructions[pc];before=states[pc];state=list(before)
   for bit in range(6):
    if ins['defines']&(1<<bit):state[bit]=frozenset([pc])
    if ins['undefined']&(1<<bit):state[bit]=frozenset(['undefined'])
    if ins.get('conditional_effect'):state[bit]|=before[bit]
   if ins['control']in ('call','call_indirect')and not ins.get('reference_API_preserves_flags'):
    summary=summaries.get(ins['target'])if ins['control']=='call'else None
    if summary is None:state=[frozenset(['unknown-call@'+hex(pc)])for _ in range(6)]
    else:state=[frozenset(x for x in summary[b]if x!='incoming')|(state[b]if'incoming'in summary[b]else frozenset())for b in range(6)]
   state=tuple(state)
   if ins['control']=='ret':returns.append(state)
   for nxt in self.successors(ins):
    prior=states.get(nxt);merged=state if prior is None else tuple(prior[b]|state[b]for b in range(6))
    if merged!=prior:states[nxt]=merged;q.append(nxt)
  if not hasattr(self,'flag_states'):self.flag_states={}
  self.flag_states[entry]=states
  return tuple(frozenset().union(*(r[b]for r in returns))for b in range(6))if returns else None
 def flags(self):
  out=super().flags();annotations={int(r['site'],0):dict(r)for r in out['annotations']};leaders=set(self.functions)
  for q in self.instructions.values():
   if q['control']in ('jmp','jcc'):leaders.add(q['target'])
   if q['control']is not None:leaders.add(q['end'])
  leaders=sorted(leaders);reads=[]
  for entry,states in self.flag_states.items():
   for pc,state in states.items():
    ins=self.instructions.get(pc)
    if not ins or not ins.get('arithmetic_flag_read_mask'):continue
    origins=state[0]
    if any(not isinstance(x,int)for x in origins):self.record_unknown('arithmetic_input_flags_unproved',pc,entry=hex(entry),origins=sorted(origins,key=str));continue
    leader=max(p for p in leaders if p<=pc)
    for producer in origins:
     r=annotations.setdefault(producer,dict(site=hex(producer),sr_mask='0x0',sr_input_mask='0x0'));r['sr_mask']=hex(int(r['sr_mask'],0)|1)
    if any(x<leader for x in origins):
     r=annotations.setdefault(leader,dict(site=hex(leader),sr_mask='0x0',sr_input_mask='0x0'));r['sr_input_mask']=hex(int(r['sr_input_mask'],0)|1)
    reads.append(dict(PC=hex(pc),entry=hex(entry),producers=[hex(x)for x in sorted(origins)],region=hex(leader)))
  # SR can move materialisation to a branch at the next region. Its output
  # flags then require corresponding incoming flags, including bits beyond
  # the branch condition. Derive every bit from all original CFG states.
  bridges=[]
  for pc,r in annotations.items():
   if self.instructions[pc]['control']!='jcc':continue
   mask=int(r['sr_mask'],0);leader=max(p for p in leaders if p<=pc)
   for bit in range(6):
    if not mask&(1<<bit):continue
    contexts=[state[pc][bit]for state in self.flag_states.values()if pc in state]
    origins=set().union(*contexts)
    if any(not isinstance(x,int)for x in origins):
     self.record_unknown('annotation_output_incoming_unproved',pc,flag_bit=bit,origins=sorted(origins,key=str));continue
    if any(not leader<=x<pc for x in origins):
     r['sr_input_mask']=hex(int(r['sr_input_mask'],0)|(1<<bit))
     bridges.append(dict(PC=hex(pc),bit=bit,producers=[hex(x)for x in sorted(origins)],region=hex(leader)))
  out['annotations']=[annotations[p]for p in sorted(annotations)];out['arithmetic_carry_read_evidence']=reads;out['output_region_bridge_proofs']=bridges;return out
def main():
 p=argparse.ArgumentParser();p.add_argument('--closure',default='stage76/evidence/full-dependency-closure-v1.json');p.add_argument('--output',required=True);args=p.parse_args();output=(ROOT/args.output).resolve();assert output.is_relative_to(S/'evidence')and not output.exists()
 full=json.loads((ROOT/args.closure).read_text());part=json.loads((S/'evidence/call-partition.json').read_text());proofs=json.loads((S/'configs/proved-register-IAT.json').read_text());pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
 # All named kernel32 IAT sites identified from original direct operand bytes.
 slots={int(re.fullmatch(r'dword ptr \[(\d+)\]',r['operand'])[1])for r in full['indirect_edges']if re.fullmatch(r'dword ptr \[(\d+)\]',r['operand'])and 0x7a9000<=int(re.fullmatch(r'dword ptr \[(\d+)\]',r['operand'])[1])<0x7aa000}
 slots|={int(p['slot'],0)for p in proofs.values()}
 a=Analysis(pe,dict(roots=full['function_entries'],windows=[],full=full,import_proofs=proofs,reference_import_slots=slots,jump_domains=json.loads((ROOT/"stage75/configs/crt-jump-domains.json").read_text())["domains"]));a.scan();flags=a.flags()
 out=dict(status='FULL_CFG_FLAGS_METADATA_WITH_EXPLICIT_UNPROVED_CONTRACTS',flags=flags,unknowns=a.unknown,summary_rounds=a.summary_rounds,decoded_instructions=len(a.instructions),functions=len(a.functions),source_instructions_unchanged=True,reference_API_flag_contract='fixed existing reference shim ret() preserves CPU arithmetic EFLAGS; unsupported native APIs remain unresolved',native_flag_equivalence_proved=False)
 output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(dict(annotations=len(flags['annotations']),unknowns=len(a.unknown),incoming_or_unknown_contracts=len(flags['incoming_or_unknown_contracts']),conditional_proofs=len(flags['conditional_read_evidence']),carry_reads=len(flags['arithmetic_carry_read_evidence']))))
if __name__=='__main__':main()
