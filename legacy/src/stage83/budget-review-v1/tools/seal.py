"""Seal completed read-only review after measurement wrappers exit."""
import json,sys
from pathlib import Path
sys.dont_write_bytecode=True
R=Path(__file__).resolve().parents[3];V=R/'stage83/budget-review-v1';sys.path.insert(0,str(R/'stage83/batch-v1/tools'))
from common import sha,put,verify
from pipeline import accounting
b=json.loads((V/'evidence/protection-baseline.json').read_text());verify(b['hashes']);counts=accounting();assert counts['rounds_used']==1 and counts['role_processes_used']==1
ledger=json.loads((R/'evidence/stage55-batch-ledger.json').read_text());assert len(ledger['executions'])==20 and ledger['remaining_processes']==80 and ledger['goal_closed']
put(V/'evidence/protection-result.json',{'status':'ALL_EXISTING_INPUTS_APPROVALS_CLAIMS_AND_RESULTS_UNCHANGED','protected_hashes_verified':b['count'],'new_original_native_Unicorn_execution':0,'rounds_used':1,'rounds_remaining':9,'old_ledger':'20/80/closed'})
files=[p for p in V.rglob('*')if p.is_file()];files.extend(p for p in(R/'stage83/evidence').glob('budget-review-v1-*')if p.is_file());pins={str(p.relative_to(R)):sha(p)for p in files};put(V/'manifest.json',{'hashes':pins,'protected_old_hashes':b['count'],'proposal_approved':False,'new_original_native_Unicorn_execution':0});verify(pins)
print(json.dumps({'status':'READ_ONLY_BUDGET_REVIEW_SEALED','old_hashes_passed':b['count'],'new_files':len(pins),'manifest_sha256':sha(V/'manifest.json'),'rounds_used':1,'remaining':9}))
