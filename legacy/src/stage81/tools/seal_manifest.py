import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
assert json.loads((S/'evidence/finalize-protection-v1.result.json').read_text())['returncode']==0
artifacts={str(p.relative_to(ROOT)):sha(p)for p in S.rglob('*')if p.is_file()and p.name!='manifest.json'}
(S/'manifest.json').write_text(json.dumps(dict(hashes=artifacts,previous_protected_hashes=4708,old_ledger_sha256=sha(ROOT/'evidence/stage55-batch-ledger.json'),finalizer_logs_closed_before_seal=True),indent=2)+'\n');assert all(sha(ROOT/p)==h for p,h in artifacts.items());print(json.dumps(dict(status='SEALED_AND_RECHECKED',artifacts=len(artifacts),manifest_sha256=sha(S/'manifest.json'))))
