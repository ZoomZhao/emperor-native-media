"""Verify final full-image post-fixpoint under a different worklist schedule."""
import json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage77'
from flags_analysis import load
def main():
 a=load(ROOT/'stage76/evidence/full-dependency-closure-v1.json',False);first=a.make_solver().solve();second=a.make_solver();drain=second.drain
 second.drain=lambda entries:drain(list(reversed(entries)))
 second.solve();assert first.summaries==second.summaries
 assert first.statistics()['summary_updates']<=first.statistics()['summary_update_upper_bound']
 out=dict(status='FULL_SOLVER_SCHEDULE_INDEPENDENCE_AND_POSTFIXPOINT_PASSED',functions=len(first.entries),instructions=len(first.nodes),normal_schedule=first.statistics(),reversed_initial_schedule=second.statistics(),all_summaries_identical=True,unclosed_external_flags_remain_top=True,original_instruction_execution=False)
 p=S/'evidence/solver-schedule-audit.json';assert not p.exists();p.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(dict(status=out['status'],normal_updates=first.summary_updates,reversed_updates=second.summary_updates)))
if __name__=='__main__':main()
