"""Seal after the bounded finalizer closes its logs; no game/engine execution."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage80'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
assert json.loads((S/'evidence/finalize-protection-v1.result.json').read_text())['returncode']==0
assert json.loads((S/'evidence/result.json').read_text())['protected_hashes_passed']==4511
artifacts={str(p.relative_to(ROOT)):sha(p)for p in S.rglob('*')if p.is_file()and p.name!='manifest.json'}
for p in ['stage79/evidence/cpu-evidence-attempt.claim.json','stage79/runs/cpu-evidence-once/result.json']:artifacts[p]=sha(ROOT/p)
manifest=dict(previous_protected_hashes=4511,hashes=artifacts,diagnostic_original_manifest_sha256=sha(ROOT/'stage79/configs/cpu-evidence-manifest.json'),old_ledger_sha256=sha(ROOT/'evidence/stage55-batch-ledger.json'),finalizer_logs_closed_before_seal=True)
(S/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
assert all(sha(ROOT/p)==h for p,h in artifacts.items())
print(json.dumps(dict(status='SEALED_AND_RECHECKED',artifacts=len(artifacts),manifest_sha256=sha(S/'manifest.json'))))
