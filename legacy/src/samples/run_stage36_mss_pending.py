"""Do not invoke before exact MSS36 approval is supplied by the parent."""
import argparse,hashlib,json,os,shutil,subprocess,time
from pathlib import Path
R=Path(__file__).resolve().parents[1];APP=R/'stage8-local/EmperorBaseline.app/Contents';SCOPE='MSS36_STARTUP_DEVICE_V1'
def main():
 p=argparse.ArgumentParser();p.add_argument('--approval-file',required=True);a=p.parse_args();approval=json.loads(Path(a.approval_file).read_text());assert approval.get('scope_id')==SCOPE and approval.get('approved')is True and approval.get('source_thread_id')=='01a10af1-425e-75ae-bded-bf47f843a029'and approval.get('user_verbatim'),'Exact original MSS36 DLL execution approval is missing'
 m=json.loads((R/'evidence/stage36-MSS-pending-probe.json').read_text());assert approval['probe_sha256']==m['executable_sha256'];assert approval['DLL_sha256']==[r['sha256']for r in m['fixed_input_rows']];exe=Path(m['executable']);assert hashlib.sha256(exe.read_bytes()).hexdigest()==m['executable_sha256'];prefix=R/'runs/stage36-mss-prefix';folder=prefix/'drive_c/MSS36StartupProbe';assert folder.is_dir();assert set(p.name for p in (prefix/'dosdevices').iterdir())=={'c:'}
 allowed={row['name']for row in m['fixed_input_rows']}|{'mss36-probe.exe','result.bin'};assert not any(p.name not in allowed for p in folder.iterdir())
 for row in m['fixed_input_rows']:
  source=Path(row['copy_path']);assert hashlib.sha256(source.read_bytes()).hexdigest()==row['sha256'];dest=folder/row['name']
  if dest.exists():assert hashlib.sha256(dest.read_bytes()).hexdigest()==row['sha256']
  else:shutil.copyfile(source,dest)
 target=folder/'mss36-probe.exe';shutil.copyfile(exe,target);output=folder/'result.bin';assert not output.exists(),'Use a fresh explicitly named reviewed run; do not overwrite prior evidence'
 prepared=json.loads((R/'evidence/stage36-prefix-prepared.json').read_text())
 for row in prepared['existing_Wine_binaries']:assert hashlib.sha256(Path(row['path']).read_bytes()).hexdigest()==row['sha256']
 wine=APP/'SharedSupport/wine/bin/wine32on64';env=os.environ.copy();env.update(WINEPREFIX=str(prefix),WINEDEBUG='+loaddll,+seh',DYLD_FALLBACK_LIBRARY_PATH=str(APP/'SharedSupport/wine/lib'));command=[str(wine),'C:\\MSS36StartupProbe\\mss36-probe.exe'];start=time.monotonic()
 with (R/'evidence/stage36-MSS-execution.log').open('w')as log:
  child=subprocess.Popen(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT)
  try:child.wait(timeout=18)
  except subprocess.TimeoutExpired:
   child.terminate()
   try:child.wait(timeout=1)
   except subprocess.TimeoutExpired:child.kill();child.wait(timeout=.5)
 subprocess.run([str(APP/'SharedSupport/wine/bin/wineserver'),'-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=5)
 out=dict(command=command,cwd=str(folder),scope_id=SCOPE,returncode=child.returncode,elapsed_seconds=time.monotonic()-start,original_four_DLLs_execution_approved=True,game_EXE_launched=False)
 if output.exists():
  body=output.read_bytes();assert len(body)==m['output_size'];(R/'evidence/stage36-MSS-result.bin').write_bytes(body);out['output_sha256']=hashlib.sha256(body).hexdigest()
 (R/'evidence/stage36-MSS-execution.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
if __name__=='__main__':main()
