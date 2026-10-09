"""Read-only comparison with the already protected original-input manifest."""
import hashlib,json,datetime
from pathlib import Path
root=Path(__file__).resolve().parents[1]
prior=json.loads((root/'evidence/stage31-file-audit.json').read_text())
checks=[]
for item in prior['checks']:
    path=Path(item['path']);digest=hashlib.sha256(path.read_bytes()).hexdigest()
    checks.append(dict(path=str(path),sha256=digest,unchanged=digest==item['sha256']))
result=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_147_inputs_unchanged=len(checks)==147 and all(row['unchanged']for row in checks),check_count=len(checks),checks=checks)
(root/'evidence/stage32-file-audit.json').write_text(json.dumps(result,indent=2,ensure_ascii=False))
print(json.dumps({key:result[key]for key in ['all_147_inputs_unchanged','check_count']}))
assert result['all_147_inputs_unchanged']
