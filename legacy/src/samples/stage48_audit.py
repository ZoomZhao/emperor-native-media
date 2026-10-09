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
prior=json.loads((R/'evidence/stage47-postflight.json').read_text())
for q in prior['checks']:check(q['path'],q['sha256'])
for q in json.loads((R/'evidence/stage47-result.json').read_text())['current_checkpoint_files']:check(R/q['path'],q['sha256'])
for n,src in [('stage46-native-build.json','stage46_samples.c')]:
 q=json.loads((R/'evidence'/n).read_text());check(q['library'],q['sha256']);check(R/'samples'/src,q['source_sha256'])
for row in json.loads((R/'evidence/stage46-profile-files.json').read_text())['rows']:
 check(row['copy_path'],row['copy_sha256']);check(row['original_path'],row['original_sha256'])
for q in json.loads((R/'evidence/stage47-existing-save-paths.json').read_text()):check(q['path'],q['sha256'])
q=json.loads((R/'evidence/stage47-visibility-build.json').read_text());check(q['library'],q['sha256']);check(R/'samples/stage47_visibility.m',q['source_sha256'])
for q in json.loads((R/'evidence/stage47-campaign-files.json').read_text()):
 if not q['is_directory']:check(q['path'],q['sha256'])
 else:
  p=Path(q['path']);st=p.stat();assert p.is_dir()and(st.st_dev,st.st_ino,st.st_size)==(q['device'],q['inode'],q['size'])
q=json.loads((R/'evidence/stage47-directory-build.json').read_text());check(q['library'],q['sha256']);check(R/'samples/stage47_directory.c',q['source_sha256'])
for q in json.loads((R/'evidence/stage48-frozen-stage47-sources.json').read_text()):check(q['path'],q['sha256'])
q=json.loads((R/'evidence/stage48-game-window-build.json').read_text());check(q['library'],q['sha256']);check(R/'samples/stage48_game_window.m',q['source_sha256'])
q=json.loads((R/'evidence/stage48-fileinfo-build.json').read_text());check(q['library'],q['sha256']);check(R/'samples/stage48_fileinfo.c',q['source_sha256'])
absences=[]
for q in prior['absences']+json.loads((R/'evidence/stage47-next-missing-campaign-paths.json').read_text()):
 p=Path(q['path']);absences.append(dict(path=str(p),still_missing=not p.exists()and(not p.parent.is_dir()or not any(c.name.casefold()==p.name.casefold()for c in p.parent.iterdir()))))
links=[]
for q in prior['DOS_namespace_symlinks']:
 p=Path(q['path']);target=str(p.readlink());links.append(dict(path=str(p),target=target,unchanged=target==q['target']))
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),checks=checks,hash_checks=len(checks),unique_paths=len(cache),all_frozen_inputs_unchanged=all(q['unchanged']for q in checks),absences=absences,all_observed_missing_inputs_still_missing=all(q['still_missing']for q in absences),DOS_namespace_symlinks=links,all_namespace_symlinks_unchanged=all(q['unchanged']for q in links))
(R/'evidence/stage48-postflight.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['hash_checks','unique_paths','all_frozen_inputs_unchanged','all_observed_missing_inputs_still_missing','all_namespace_symlinks_unchanged']}));assert out['all_frozen_inputs_unchanged']and out['all_observed_missing_inputs_still_missing']and out['all_namespace_symlinks_unchanged']
