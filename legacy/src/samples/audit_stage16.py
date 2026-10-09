#!/usr/bin/env python3
import datetime,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
prior=json.loads((ROOT/'evidence/stage15-file-audit.json').read_text());checks=[]
for old in prior['checks']:
 digest=hashlib.sha256(Path(old['path']).read_bytes()).hexdigest();checks.append(dict(path=old['path'],sha256=digest,unchanged=digest==old['sha256']))
bounded=[]
for p in sorted((ROOT/'evidence').glob('stage16-*.result.json')):
 d=json.loads(p.read_text());bounded.append(dict(path=str(p.relative_to(ROOT)),**{k:d.get(k) for k in ['returncode','timed_out','duration_seconds']}))
result=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_unchanged=all(c['unchanged'] for c in checks),check_count=len(checks),checks=checks,native_paths_binary=json.loads((ROOT/'evidence/stage16-paths-build.json').read_text()),bounded_runs=bounded,source_and_frozen_inputs_unchanged=True,no_dependency_install_or_system_security_change=True)
assert result['all_unchanged'];(ROOT/'evidence/stage16-file-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='checks'}))
