import hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];prior=json.loads((R/'evidence/stage35-file-audit.json').read_text());checks=[]
for row in prior['checks']:
 p=Path(row['path']);actual=hashlib.sha256(p.read_bytes()).hexdigest();checks.append(dict(path=str(p),sha256=actual,unchanged=actual==row['sha256']))
extra=json.loads((R/'evidence/stage35-extra-input-audit.json').read_text());additional=[]
for row in extra['checks']+extra['previous_checks']:
 p=Path(row['path']);actual=hashlib.sha256(p.read_bytes()).hexdigest();additional.append(dict(path=str(p),sha256=actual,unchanged=actual==row['sha256']))
providers=[]
for row in json.loads((R/'evidence/stage36-provider-inputs.readonly.json').read_text()):
 for prefix in ['original','copy']:
  p=Path(row[prefix+'_path']);actual=hashlib.sha256(p.read_bytes()).hexdigest();providers.append(dict(path=str(p),sha256=actual,unchanged=actual==row['sha256']))
out=dict(protected147_unchanged=len(checks)==147 and all(r['unchanged']for r in checks),previous190_extra_checks_unchanged=len(additional)==190 and all(r['unchanged']for r in additional),MSS_provider8_paths_unchanged=len(providers)==8 and all(r['unchanged']for r in providers),checks=checks,additional=additional,providers=providers);(R/'evidence/stage36-file-audit.json').write_text(json.dumps(out,indent=2));assert all(out[k]for k in ['protected147_unchanged','previous190_extra_checks_unchanged','MSS_provider8_paths_unchanged']);print(json.dumps({k:out[k]for k in ['protected147_unchanged','previous190_extra_checks_unchanged','MSS_provider8_paths_unchanged']}))
