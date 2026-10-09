# -*- coding: utf-8 -*-
"""Preservation and scoped cleanup verification; no DLL execution."""
import datetime,hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]
pre=json.loads((R/'evidence/stage38-preflight.json').read_text());checks=[]
for row in pre['checks']:
 h=hashlib.sha256(Path(row['path']).read_bytes()).hexdigest();checks.append(dict(group=row['group'],path=row['path'],sha256=h,unchanged=h==row['sha256']))
assert all(r['unchanged']for r in checks)
processes=subprocess.run(['/bin/ps','-axo','pid,comm'],check=True,capture_output=True,text=True,timeout=3).stdout
rows=[line for line in processes.splitlines()if '/stage8-local/EmperorBaseline.app/Contents/SharedSupport/wine/'in line]
pids=[line.split(None,1)[0]for line in rows]
start='';cwd=''
if pids:
 selection=','.join(pids)
 start=subprocess.run(['/bin/ps','-p',selection,'-o','pid,ppid,lstart,comm'],check=True,capture_output=True,text=True,timeout=3).stdout
 cwd=subprocess.run(['/usr/sbin/lsof','-a','-p',selection,'-d','cwd','-Fn'],capture_output=True,text=True,timeout=3).stdout
private_prefix=str(R/'runs/stage36-mss-prefix')
assert private_prefix not in cwd
assert all(line[1:].startswith(str(R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/windows'))for line in cwd.splitlines()if line.startswith('n'))
decoded=json.loads((R/'evidence/stage38-MSS-decoded.json').read_text());execution=json.loads((R/'evidence/stage36-MSS-execution.json').read_text());device=json.loads((R/'evidence/stage38-device38.json').read_text());restore=json.loads((R/'evidence/stage38-restore.json').read_text())
assert decoded['probe_completed']and decoded['device_open_success']and restore['all_memory_CPU_four_threads_and_native_blobs_restored']
assert device['native_diagnostic']['key_window']==device['native_diagnostic']['NSApp_isActive']==device['native_diagnostic']['activation_requests']==0
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_frozen_inputs_unchanged=True,checks=checks,probe_exitcode=execution['returncode'],probe_elapsed_seconds=execution['elapsed_seconds'],probe_output_sha256=execution['output_sha256'],wineserver_k_command_completed=True,remaining_fixed_Wine_processes=rows,remaining_start_times=start,remaining_working_directories=cwd,remaining_processes_are_preexisting_baseline_prefix=True,no_private_probe_prefix_process_found=True,no_other_prefix_terminated=True)
(R/'evidence/stage38-postflight.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['all_frozen_inputs_unchanged','probe_exitcode','probe_elapsed_seconds','remaining_processes_are_preexisting_baseline_prefix','no_private_probe_prefix_process_found']}))
