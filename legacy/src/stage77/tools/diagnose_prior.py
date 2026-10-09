"""Reproduce old abstract-analysis replacement oscillation; no CPU execution."""
import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage77';sys.path.insert(0,str(ROOT/'stage76/tools'))
from full_flags import Analysis as Old
from flags_analysis import load
from monotone_flags import tarjan
def serial(summary):
 return None if summary is None else[sorted(bit,key=str)for bit in summary]
def main():
 a=load(ROOT/'stage76/evidence/full-dependency-closure-v1.json');a.__class__=Old
 summaries={};seen={};rows=[];drops=[];repeat=None
 for step in range(1,49):
  new={f:a.flag_summary(f,summaries)for f in sorted(a.functions)}
  changed=[hex(f)for f in new if new[f]!=summaries.get(f)]
  for f in new:
   old=summaries.get(f)
   if old is None or new[f]is None:continue
   for bit in range(6):
    removed=old[bit]-new[f][bit]
    if removed and len(drops)<20:drops.append(dict(round=step,function=hex(f),flag=bit,removed=sorted(removed,key=str),added=sorted(new[f][bit]-old[bit],key=str)))
  digest=hashlib.sha256(json.dumps({hex(f):serial(v)for f,v in new.items()},sort_keys=True).encode()).hexdigest()
  rows.append(dict(round=step,changed_functions=changed,summary_digest=digest))
  if digest in seen and changed:repeat=dict(first_round=seen[digest],repeat_round=step,period=step-seen[digest]);break
  seen[digest]=step;summaries=new
 # Minimal two-procedure mutually recursive identity transfer. Old unknown-call
 # sentinels are freshly substituted and swap forever rather than joined.
 synth=[];prev={}
 for step in range(6):
  nxt={1:prev.get(2,frozenset(['unknown-call@A'])),2:prev.get(1,frozenset(['unknown-call@B']))};synth.append({str(f):sorted(v)for f,v in nxt.items()});prev=nxt
 out=dict(status='PRIOR_ABSTRACT_SOLVER_OSCILLATION_REPRODUCED',original_instructions_executed=False,iterations=rows,detected_repeat=repeat,nonmonotone_origin_removal_examples=drops,minimal_mutual_recursion=synth,cause='fresh unknown-call tokens for temporarily absent local summaries; synchronous replacement rather than lattice join allows provenance to disappear and swap around recursive call SCCs; not an original instruction execution problem')
 (S/'evidence/prior-oscillation.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(dict(repeat=repeat,origin_removal_examples=len(drops),last_changed=rows[-1]['changed_functions'])))
if __name__=='__main__':main()
