"""Read-only preflight. Never imports an execution engine or claims an original round."""
from core import *
import argparse

def main():
 p=argparse.ArgumentParser()
 p.add_argument('--config',type=Path,default=V/'configs/bounded-probe-proposal.json')
 p.add_argument('--original-run',action='store_true')
 a=p.parse_args()
 if a.original_run:
  raise Boundary('ORIGINAL_PROBE_CLOSED: target ABI / original executor / changed-budget approval incomplete')
 plan=json.loads(a.config.read_text())
 verify({str(SOURCE/n):value for n,value in plan['source_pins'].items()})
 ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text())
 assert sha(ROOT/'evidence/stage55-batch-ledger.json')=='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
 assert len(ledger['executions'])==20 and ledger['remaining_processes']==80 and ledger['goal_closed']
 assert len(list((ROOT/'stage83/runs').glob('batch-v1-round*/round.claim.json')))==1
 callback=json.loads((V/'evidence/callback-proof.json').read_text())
 own=json.loads((V/'runs/own-performance-full-v1/result.json').read_text())
 process=json.loads((V/'runs/own-performance-full-v1/process.json').read_text())
 snapshot=json.loads((V/'evidence/snapshot-cost.json').read_text())
 print(json.dumps({
  'status':'READONLY_PREFLIGHT_PASS_ORIGINAL_PROBE_CLOSED',
  'closures':{'clock_and_Watch':True,'existing_controlled_keyboard':True,'future_callback_target_ABI':False},
  'callback_source_object':callback['current_object'],
  'own_mixed_steps':own['own_instructions'],'own_writes':own['own_write_records'],
  'own_guest_seconds':own['guest_seconds'],'own_whole_role_seconds':process['total_seconds'],
  'readonly_snapshot_process_data_seconds':snapshot['whole_process_data_seconds'],
  'proposed_caps':plan['proposed_shared_caps'],
  'caps_approved':False,'original_executor_implemented':False,
  'original_engine_imported':'unicorn' in sys.modules,'original_instructions':0,
  'original_rounds_used':1,'remaining_rounds':9,'old_ledger':'20 used / 80 remaining / closed'
 },ensure_ascii=False))

if __name__=='__main__':main()
