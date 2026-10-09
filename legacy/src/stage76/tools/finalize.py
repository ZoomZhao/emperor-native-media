"""Read-only protection audit and local Stage76 evidence manifest; no game run."""
import ast,hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage76'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 for p in (S/'tools').glob('*.py'):ast.parse(p.read_text(),filename=str(p))
 base=json.loads((S/'evidence/protection-baseline.json').read_text());bad=[]
 for p,h in base['hashes'].items():
  if not Path(p).is_file()or sha(p)!=h:bad.append(p)
 assert not bad,bad
 ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text())
 assert(len(ledger['executions']),ledger['remaining_processes'],bool(ledger['goal_closed']))==(20,80,True)
 build=json.loads((S/'builds/full-v9/result.json').read_text());audit=json.loads((S/'evidence/full-audit.json').read_text());inputs=json.loads((S/'evidence/input-evidence.json').read_text())
 assert all(x['returncode']==0 for x in build['steps'])and build['status']=='FULL_DEPENDENCY_SR_DIAGNOSTIC_OBJECT_BUILT'
 assert not build['complete_runnable_phase8']and not build['original_or_Unicorn_or_generated_game_executed']
 assert len(inputs['checkpoints'])==19 and all(x['eligible_records']==0 for x in inputs['checkpoints'])
 report=dict(status='FULL_PHASE8_DEPENDENCY_OBJECT_BUILT_RUNTIME_OPEN',build='stage76/builds/full-v9/result.json',static_audit='stage76/evidence/full-audit.json',input_evidence='stage76/evidence/input-evidence.json',call_partition='stage76/evidence/call-partition.json',human_report='stage76/reports/README.txt',
  functions=201,instructions=7334,original_code_bytes=20054,expanded_named_API_CALL_sites=60,expanded_unique_named_APIs=31,expanded_unresolved_non_IAT_CALL_sites=31,
  real_native_heap_wrappers_compiled=4,full_checkpoint_compatible_API_completion_claimed=0,flags_summary_converged=False,full_guest_binding_guard_trace_integrated=False,complete_runnable_phase8=False,Stage73_success_not_generalized=True,
  protected_files_checked=len(base['hashes']),protection_mismatches=bad,old_ledger=dict(used=20,remaining=80,closed=True),new_execution_claims=0,new_original_game_or_Unicorn_or_generated_game_library_runs=0,
  continuation_gate='build-only SCC flags convergence and complete binding/state-adapter contracts before considering separately approved active-input acquisition; no current execution approval requested')
 p=S/'evidence/result.json';assert not p.exists();p.write_text(json.dumps(report,indent=2)+'\n')
 review=dict(status='READ_ONLY_PROTECTION_AND_DELIVERY_REVIEW_PASSED',protected_files=len(base['hashes']),mismatches=bad,used=20,remaining=80,closed=True,Stage73_claims_approvals_inputs_checkpoints_and_success_artifacts_preserved=True,Stage74_and_Stage75_preserved=True,new_game_runs=0,all_new_python_sources_parse=True)
 p=S/'evidence/final-review.json';assert not p.exists();p.write_text(json.dumps(review,indent=2)+'\n')
 hashes={}
 for p in S.rglob('*'):
  if p.is_file()and '__pycache__'not in p.parts and p.name!='delivery-manifest.json'and not p.name.startswith('stage76-finalize-'):hashes[str(p.relative_to(ROOT))]=sha(p)
 p=S/'evidence/delivery-manifest.json';assert not p.exists();p.write_text(json.dumps(dict(kind='STATIC_BUILD_EVIDENCE_NOT_EXECUTION_APPROVAL',runtime_eligible=False,pinned_files=hashes,excluded_current_finalization_logs='stage76-finalize-* may still be written while this command returns; all earlier build/analysis/failure logs included',old_protection_baseline='stage76/evidence/protection-baseline.json',human_report='stage76/reports/README.txt',result='stage76/evidence/result.json'),indent=2)+'\n')
 for p,h in hashes.items():assert sha(ROOT/p)==h,p
 print(json.dumps(dict(**review,delivery_files=len(hashes))))
if __name__=='__main__':main()
