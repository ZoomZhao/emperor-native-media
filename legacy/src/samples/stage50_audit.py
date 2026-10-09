import hashlib,json,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1];cache={};checks=[]
def sha(p):
 p=Path(p)
 if str(p)not in cache:
  h=hashlib.sha256()
  with p.open('rb')as f:
   for b in iter(lambda:f.read(1048576),b''):h.update(b)
  cache[str(p)]=h.hexdigest()
 return cache[str(p)]
def check(p,wanted):
 got=sha(p);checks.append(dict(path=str(p),expected_sha256=wanted,sha256=got,unchanged=got==wanted))
prior=json.loads((R/'evidence/stage49-postflight.json').read_text())
for q in prior['checks']:check(q['path'],q['sha256'])
for q in json.loads((R/'evidence/stage49-result.json').read_text())['current_checkpoint_files']:check(R/q['path'],q['sha256'])
for q in json.loads((R/'evidence/stage50-frozen-stage49-sources.json').read_text()):check(q['path'],q['sha256'])
q=json.loads((R/'evidence/stage50-stream-close-build.json').read_text());check(q['library'],q['sha256']);check(R/'samples/stage50_close_stream.c',q['source_sha256']);check(R/'samples/stage44_stream.c',q['included_source_sha256'])
absences=[]
for q in prior['absences']:
 p=Path(q['path']);absences.append(dict(path=str(p),still_missing=not p.exists()and(not p.parent.is_dir()or not any(c.name.casefold()==p.name.casefold()for c in p.parent.iterdir()))))
links=[]
for q in prior['DOS_namespace_symlinks']:
 p=Path(q['path']);target=str(p.readlink());links.append(dict(path=str(p),target=target,unchanged=target==q['target']))
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),checks=checks,hash_checks=len(checks),unique_paths=len(cache),all_frozen_inputs_unchanged=all(q['unchanged']for q in checks),absences=absences,all_observed_missing_inputs_still_missing=all(q['still_missing']for q in absences),DOS_namespace_symlinks=links,all_namespace_symlinks_unchanged=all(q['unchanged']for q in links))
(R/'evidence/stage50-postflight.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['hash_checks','unique_paths','all_frozen_inputs_unchanged','all_observed_missing_inputs_still_missing','all_namespace_symlinks_unchanged']}));assert out['all_frozen_inputs_unchanged']and out['all_observed_missing_inputs_still_missing']and out['all_namespace_symlinks_unchanged']
