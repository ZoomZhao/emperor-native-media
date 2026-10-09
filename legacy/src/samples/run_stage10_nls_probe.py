#!/usr/bin/env python3
"""Run only this task's own NLS probe via the already approved Wine launcher."""
import hashlib
import json
import plistlib
import shutil
import subprocess
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
APP=ROOT/'stage8-local/EmperorBaseline.app/Contents'
GAME=APP/'Resources/drive_c/EmperorRotMK[ZeaS]'
info=APP/'Info.plist';before=info.read_bytes()
probe=ROOT/'samples/stage10-nls-probe.exe'
target=GAME/'ST10-NLS-PROBE.exe';output=GAME/'ST10-NLS.bin'
assert not target.exists() and not output.exists()
record=json.loads((ROOT/'evidence/stage10-nls-probe-source.json').read_text())
assert hashlib.sha256(probe.read_bytes()).hexdigest()==record['sha256']
shutil.copyfile(probe,target)
v=plistlib.loads(before);v['Program Name and Path']='/EmperorRotMK[ZeaS]/ST10-NLS-PROBE.exe'
info.write_bytes(plistlib.dumps(v))
command=[str(APP/'MacOS/wineskinlauncher')]
started=time.monotonic();child=None
result=dict(command=command,cwd=str(APP),budget_seconds=60,own_probe_sha256=record['sha256'],source='all probe bytes authored in samples/make_stage10_nls_probe.py')
try:
    with (ROOT/'evidence/stage10-nls-probe-launcher.log').open('w') as log:
        child=subprocess.Popen(command,cwd=APP,stdout=log,stderr=subprocess.STDOUT)
        result['launcher_pid']=child.pid
        child.wait(timeout=55)
        result['launcher_returncode']=child.returncode
    if output.is_file():
        data=output.read_bytes();assert len(data)==28
        frozen=ROOT/'evidence/stage10-nls-probe-output.bin';frozen.write_bytes(data)
        result.update(output_sha256=hashlib.sha256(data).hexdigest(),output_hex=data.hex(),output_size=len(data),
                      actual_GetACP=int.from_bytes(data[:4],'little'),GetCPInfo_bytes=data[4:24].hex(),
                      actual_GetCPInfo_success=int.from_bytes(data[24:28],'little'))
    else:result['failure']='own probe did not produce the bounded output'
except subprocess.TimeoutExpired:
    result['failure']='55-second own probe deadline'
    if child:child.terminate();child.wait(timeout=2)
finally:
    info.write_bytes(before)
    result.update(elapsed_seconds=round(time.monotonic()-started,3),copy_config_restored=info.read_bytes()==before)
    (ROOT/'evidence/stage10-nls-probe-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2),flush=True)
