"""Static metadata under explicit guarded domains; original bytes stay unchanged.

Only singleton indirect edges and proved always-taken conditional edges are
supported. Domain proofs are configuration obligations, not generic inference.
"""
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'stage74/tools'))
from analysis_v2 import PE,Analysis as BaseAnalysis
class Analysis(BaseAnalysis):
 def flag_summary(self,entry,summaries):
  # This explicit boundary never returns. It is outside the guarded reachable
  # graph, and its adapter fails rather than inventing a flag contract.
  if entry in {int(p,0)for p in self.config.get('declined_boundary_entries',[])}:
   return None
  return super().flag_summary(entry,summaries)
 def successors(self,ins):
  proof=self.config.get('proved_branch_targets',{}).get(hex(ins['pc']))
  if proof is not None:return [int(proof['target'],0)]
  return super().successors(ins)
 def scan(self):
  # Decode every byte in the reviewed build windows, including the declined
  # branch. Only metadata dataflow uses the explicit branch-domain proof.
  original=self.config.get('proved_branch_targets',{})
  self.config['proved_branch_targets']={}
  super().scan()
  self.config['proved_branch_targets']=original
  for key,proof in self.config.get('resolved_indirect_edges',{}).items():
   pc=int(key,0);ins=self.instructions[pc];target=int(proof['target'],0)
   assert ins['control']in ('call_indirect','jmp_indirect')
   ins['original_control']=ins['control'];ins['profile_target']=target
   ins['control']='call'if ins['control']=='call_indirect'else'jmp';ins['target']=target
  allowed={int(p,0)for p in self.config.get('declined_boundary_entries',[])}
  indirect={int(p,0)for p in self.config.get('resolved_indirect_edges',{})}
  self.unknown=[r for r in self.unknown if not(r['kind']=='outside_reviewed_code_window'and int(r['address'],0)in allowed)and not(r['kind']=='indirect_control_target'and int(r['address'],0)in indirect)]
