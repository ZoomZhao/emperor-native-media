import hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];checks=[]
for fname in ['stage35-JPEG-catalog.readonly.json','stage35-model-catalog.readonly.json']:
 for row in json.loads((R/'evidence'/fname).read_text())['rows']:
  for prefix in ['original','copy']:
   path=Path(row[prefix+'_path']);expected=row.get(prefix+'_sha256',row.get('sha256'));actual=hashlib.sha256(path.read_bytes()).hexdigest();checks.append(dict(path=str(path),sha256=actual,unchanged=expected==actual))
prior=json.loads((R/'evidence/stage34-extra-input-audit.json').read_text());previous=[]
for row in prior['checks']:
 path=Path(row['path']);actual=hashlib.sha256(path.read_bytes()).hexdigest();previous.append(dict(path=str(path),sha256=actual,unchanged=actual==row['sha256']))
out=dict(count=len(checks),all_182_JPEG_model_paths_unchanged=len(checks)==182 and all(r['unchanged']for r in checks),previous8_unchanged=len(previous)==8 and all(r['unchanged']for r in previous),previous_checks=previous,checks=checks);(R/'evidence/stage35-extra-input-audit.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['count','all_182_JPEG_model_paths_unchanged','previous8_unchanged']}));assert out['all_182_JPEG_model_paths_unchanged']and out['previous8_unchanged']
