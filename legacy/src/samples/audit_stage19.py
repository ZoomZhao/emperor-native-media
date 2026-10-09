#!/usr/bin/env python3
import datetime,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];prior=json.loads((ROOT/'evidence/stage18-file-audit.json').read_text());checks=[]
for old in prior['checks']:
 digest=hashlib.sha256(Path(old['path']).read_bytes()).hexdigest();checks.append(dict(path=old['path'],sha256=digest,unchanged=digest==old['sha256']))
new=[]
for old in json.loads((ROOT/'evidence/stage19-resources-hashed.json').read_text())['rows']:
 for role in ['original','copy']:
  digest=hashlib.sha256(Path(old[role+'_path']).read_bytes()).hexdigest();new.append(dict(path=old[role+'_path'],sha256=digest,unchanged_since_stage19_hash=digest==old[role+'_sha256']))
runs=[]
for p in sorted((ROOT/'evidence').glob('stage19-*.result.json')):
 d=json.loads(p.read_text());runs.append(dict(path=str(p.relative_to(ROOT)),**{k:d.get(k)for k in ['returncode','timed_out','duration_seconds']}))
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_original_protected_unchanged=all(c['unchanged']for c in checks),check_count=len(checks),checks=checks,new_resource_hashes_count=len(new),new_resource_hashes_unchanged=all(c['unchanged_since_stage19_hash']for c in new),new_resource_checks=new,bounded_runs=runs,one_corrected_driver_IndentationError=True,no_install_or_system_security_change=True,no_commit_or_publish_or_upload=True)
assert out['all_original_protected_unchanged'] and out['new_resource_hashes_unchanged'];(ROOT/'evidence/stage19-file-audit.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k not in ['checks','new_resource_checks']}))
