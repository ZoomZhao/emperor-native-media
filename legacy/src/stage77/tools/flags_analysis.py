"""Byte-backed scanner plus monotone flags solver; reuse reviewed SCI emitter."""
import argparse,hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage77'
sys.path.insert(0,str(ROOT/'stage76/tools'))
from full_flags import PE,Analysis as Prior
from monotone_flags import Solver,TOP
class Analysis(Prior):
 def scan(self):
  super().scan()
  unknown={int(q['address'],0)for q in self.unknown if q['kind']=='unknown_flag_semantics'}
  for pc,r in self.instructions.items():
   r['semantic_unknown']=pc in unknown
   # x86 shift counts are masked to five bits. CF is undefined for SHL/SHR
   # at counts >= destination width; variable counts include this possibility.
   if r['mnemonic']in ('shl','shr','sar'):
    operand=r['operand'];dst=operand.split(',')[0].strip()
    width=8 if 'byte ptr'in dst or dst in ('al','ah','bl','bh','cl','ch','dl','dh')else 16 if 'word ptr'in dst and'dword ptr'not in dst or dst in ('ax','bx','cx','dx','si','di','sp','bp')else 32
    count=operand.rsplit(',',1)[-1].strip()if','in operand else'1'
    if count=='cl':
     r.update(defines=0x1b,undefined=0x24,conditional_effect=True)
     if width<32 and r['mnemonic']!='sar':r['undefined']|=1
    else:
     n=int(count,0)&31;r.update(defines=0,undefined=0,conditional_effect=False)
     if n:
      r.update(defines=0x3b if n==1 else 0x1b,undefined=4 if n==1 else 0x24)
      if n>=width and r['mnemonic']!='sar':r['defines']&=~1;r['undefined']|=1
 def make_solver(self):
  nodes={}
  for pc,r in self.instructions.items():
   n=dict(r,successors=self.successors(r),normal_return=r['control']=='ret')
   if r['control']=='call'and r['target']in self.functions:n['local_callee']=r['target']
   elif r['control']in ('call','call_indirect')and not r.get('reference_API_preserves_flags'):n['call_unknown']=True
   if r['control']=='jmp_indirect'and not n['successors']:n['open_exit']=True
   nodes[pc]=n
  return Solver(nodes,sorted(self.functions))
 def flag_summary(self,entry,ignored):
  assert self.solution.converged
  return self.solution.summaries[entry]
 def flags(self):
  self.solution=self.make_solver().solve();self.flag_states=self.solution.states
  for entry,pc in self.solution.diagnostics:self.record_unknown('open_control_flow_top',pc,entry=hex(entry))
  for entry in self.solution.no_return_promotions:self.record_unknown('unclosed_normal_return_promoted_top',entry)
  # Prior emitter's round loop now sees immutable, already proved post-fixpoint
  # summaries. It cannot replace them or recreate temporary unknown-call tokens.
  return super().flags()
def load(path,reference_contract=True):
 full=json.loads(Path(path).read_text());proofs=json.loads((ROOT/'stage76/configs/proved-register-IAT.json').read_text());pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
 assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 # Membership comes from the actual imported API inventory, not an address
 # range. These reference contracts are explicit and optional, never a claim
 # that arbitrary native Windows API code preserves CPU EFLAGS.
 selected={int(q['pc'])for q in full['instructions']};audit=json.loads((ROOT/'stage76/evidence/full-audit.json').read_text())
 sites={int(pc,0)for q in audit['expanded_named_APIs']for pc in q['sites']}
 slots=set()
 for q in full['instructions']:
  if q['pc']in sites:
   m=re.fullmatch(r'dword ptr \[(\d+)\]',q['operand'])
   if m:slots.add(int(m[1]))
 slots|={int(p['slot'],0)for pc,p in proofs.items()if int(pc,0)in selected}
 if not reference_contract:slots=set()
 a=Analysis(pe,dict(roots=full['function_entries'],windows=[],full=full,import_proofs=proofs,reference_import_slots=slots,jump_domains=json.loads((ROOT/'stage75/configs/crt-jump-domains.json').read_text())['domains']));a.scan();return a
def main():
 p=argparse.ArgumentParser();p.add_argument('--closure',required=True);p.add_argument('--output',required=True);p.add_argument('--external-flags',choices=['unknown','fixed-reference'],default='unknown');args=p.parse_args()
 output=(ROOT/args.output).resolve();assert output.is_relative_to(S/'evidence')and not output.exists()
 a=load(ROOT/args.closure,args.external_flags=='fixed-reference');flags=a.flags()
 result=dict(status='CONSERVATIVE_MONOTONE_FLAGS_POSTFIXPOINT',statistics=a.solution.statistics(),flags=flags,unknowns=a.unknown,decoded_instructions=len(a.instructions),functions=len(a.functions),external_flag_contract=args.external_flags,unknown_dynamic_calls_are_top=True,undefined_flag_effects_are_top=True,no_unclosed_returns_marked_dead=True,source_instruction_bytes_changed=0,native_flag_equivalence_proved=False)
 output.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(dict(status=result['status'],statistics=result['statistics'],annotations=len(flags['annotations']),explicit_unproved_contracts=len(flags['incoming_or_unknown_contracts']),unknowns=len(a.unknown))))
if __name__=='__main__':main()
