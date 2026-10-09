"""Prepare a private COW Wine prefix. No Wine/DLL process is started."""
import hashlib,json,shutil,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];APP=R/'stage8-local/EmperorBaseline.app/Contents';dest=R/'runs/stage36-mss-prefix';assert not dest.exists();subprocess.run(['/bin/cp','-cR',str(APP/'Resources'),str(dest)],check=True,timeout=35)
removed=[]
for p in (dest/'dosdevices').iterdir():
 if p.name!='c:':removed.append(dict(name=p.name,target=str(p.readlink())if p.is_symlink()else None));assert p.is_symlink();p.unlink()
assert (dest/'dosdevices/c:').readlink()==Path('../drive_c');folder=dest/'drive_c/MSS36StartupProbe';folder.mkdir();m=json.loads((R/'evidence/stage36-MSS-pending-probe.json').read_text());checks=[]
for row in m['fixed_input_rows']:
 source=Path(row['copy_path']);body=source.read_bytes();assert hashlib.sha256(body).hexdigest()==row['sha256'];target=folder/row['name'];target.write_bytes(body);checks.append(dict(path=str(target),sha256=row['sha256']))
exe=Path(m['executable']);shutil.copyfile(exe,folder/'mss36-probe.exe');checks.append(dict(path=str(folder/'mss36-probe.exe'),sha256=m['executable_sha256']));out=dict(prefix=str(dest),isolated_working_directory=str(folder),prefix_is_private_COW_copy=True,removed_external_drive_links=removed,files=checks,Wine_started=False,original_DLL_executed=False,not_an_OS_security_sandbox=True);(R/'evidence/stage36-prefix-prepared.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['prefix','isolated_working_directory','Wine_started','original_DLL_executed']}))
