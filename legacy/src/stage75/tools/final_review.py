"""Freeze a complete local delivery manifest and verify all recorded hashes."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage75'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 first=json.loads((S/'evidence/delivery-manifest.json').read_text())
 for p,h in first['pinned_files'].items():assert sha(ROOT/p)==h,p
 report=json.loads((S/'evidence/result.json').read_text());assert report['original_game_or_Unicorn_or_derived_library_executions']==0
 # First manifest deliberately excluded the report under construction, but its
 # broad basename filter also excluded build result.json. The final manifest
 # explicitly includes all build results and reports; preserve the first file.
 paths=[]
 for p in S.rglob('*'):
  if p.is_file()and '__pycache__'not in p.parts and p.name not in ('delivery-manifest-v2.json','final-review.json')and not p.name.startswith('final-review-build-'):paths.append(p)
 hashes={str(p.relative_to(ROOT)):sha(p)for p in paths}
 for p,h in first['pinned_files'].items():hashes[p]=h
 assert 'stage75/builds/type53-family-v5/result.json'in hashes and 'stage75/reports/README.txt'in hashes
 # Recheck old protection hashes at final handoff; old mutable bookkeeping is
 # never reset, reconciled or edited by this task.
 base=json.loads((S/'evidence/protection-baseline.json').read_text())
 for p,h in base['hashes'].items():assert sha(p)==h,p
 ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert(len(ledger['executions']),ledger['remaining_processes'],bool(ledger['goal_closed']))==(20,80,True)
 final=dict(status='FINAL_READ_ONLY_REVIEW_PASSED',protected_files=len(base['hashes']),delivery_files=len(hashes),new_game_or_Unicorn_or_derived_library_executions=0,used=20,remaining=80,closed=True,all_delivery_hashes_passed=True,all_build_results_and_reports_included=True)
 out=S/'evidence/final-review.json';assert not out.exists();out.write_text(json.dumps(final,indent=2)+'\n');hashes[str(out.relative_to(ROOT))]=sha(out)
 manifest=S/'evidence/delivery-manifest-v2.json';assert not manifest.exists();manifest.write_text(json.dumps(dict(kind='STATIC_BUILD_DELIVERY_NOT_EXECUTION_AUTHORIZATION',pinned_files=hashes,runtime_eligible=False,new_execution_claims=0,report='stage75/evidence/result.json',human_report='stage75/reports/README.txt',supersedes_incomplete_pin_list='stage75/evidence/delivery-manifest.json'),indent=2)+'\n')
 for p,h in hashes.items():assert sha(ROOT/p)==h,p
 print(json.dumps(final))
if __name__=='__main__':main()
