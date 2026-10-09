"""Read-only preservation audit. Writes only independent stage74 evidence."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];HERE=ROOT/'stage74'
def digest(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 parser=argparse.ArgumentParser();parser.add_argument('mode',choices=['baseline','verify']);args=parser.parse_args()
 target=HERE/'evidence/protection-baseline.json';cache={}
 def sha(p):
  p=str(Path(p).resolve())
  if p not in cache:cache[p]=digest(p)
  return cache[p]
 if args.mode=='baseline':
  assert not target.exists()
  pinned={r['path']:r['expected_sha256']for r in json.loads((ROOT/'evidence/stage64-postflight.json').read_text())['checks']}
  ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());pinned.update(ledger['pinned_files'])
  manifest=json.loads((ROOT/'evidence/stage73-run-manifest.json').read_text())
  for r in manifest['pinned_inputs_and_sources']:pinned[str((ROOT/r['path']).resolve())]=r['sha256']
  for folder in ['samples','evidence','runs/stage73-SR-bound','runs/stage73-reference-once','runs/stage73-native-once']:
   for p in (ROOT/folder).rglob('*'):
    if p.is_file()and (folder.startswith('runs/stage73')or 'stage73'in p.name):pinned[str(p.resolve())]=sha(p)
  for p,v in pinned.items():assert sha(p)==v,p
  snapshot=dict(status='PROTECTION_BASELINE_FIXED',hashes=pinned,old_ledger_sha256=sha(ROOT/'evidence/stage55-batch-ledger.json'),old_used=len(ledger['executions']),old_remaining=ledger['remaining_processes'],old_closed=bool(ledger['goal_closed']),Stage73_attempt_claims_present=True)
  assert(snapshot['old_used'],snapshot['old_remaining'],snapshot['old_closed'])==(20,80,True)
  target.write_text(json.dumps(snapshot,indent=2)+'\n')
 else:
  snapshot=json.loads(target.read_text());pinned=snapshot['hashes']
  for p,v in pinned.items():assert sha(p)==v,p
  ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text())
  assert(len(ledger['executions']),ledger['remaining_processes'],bool(ledger['goal_closed']))==(20,80,True)
  (HERE/'evidence/protection-postflight.json').write_text(json.dumps(dict(status='ALL_PROTECTED_FILES_UNCHANGED',checks=len(pinned),ledger_sha256=sha(ROOT/'evidence/stage55-batch-ledger.json'),Stage73_preserved=True,old_used=20,old_remaining=80,old_closed=True),indent=2)+'\n')
 print(json.dumps(dict(mode=args.mode,checked=len(pinned),all_passed=True)))
if __name__=='__main__':main()
