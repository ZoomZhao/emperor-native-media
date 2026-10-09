#!/usr/bin/env python3
"""One bounded diagnostic using the same approved Wine engine and own PE."""
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
APP=ROOT/'stage8-local/EmperorBaseline.app/Contents'
GAME=APP/'Resources/drive_c/EmperorRotMK[ZeaS]'
engine=APP/'SharedSupport/wine'
exe=GAME/'ST10-NLS-PROBE.exe'
output=GAME/'ST10-NLS.bin'
meta=json.loads((ROOT/'evidence/stage10-nls-probe-source.json').read_text())
assert hashlib.sha256(exe.read_bytes()).hexdigest()==meta['sha256']
assert not output.exists()
command=[str(engine/'bin/wine32on64'),str(exe)]
env=os.environ.copy()
env['WINEPREFIX']=str(APP/'Resources')
env['WINEDEBUG']='+loaddll'
env['DYLD_FALLBACK_LIBRARY_PATH']=str(engine/'lib')
started=time.monotonic()
result=dict(command=command,budget_seconds=20,source='same approved isolated Wine engine; entirely own PE probe')
with (ROOT/'evidence/stage10-nls-direct.log').open('w') as log:
    child=subprocess.Popen(command,cwd=GAME,env=env,stdout=log,stderr=subprocess.STDOUT)
    result['pid']=child.pid
    try:child.wait(timeout=18)
    except subprocess.TimeoutExpired:
        result['deadline']=True
        child.terminate()
        try:child.wait(timeout=1)
        except subprocess.TimeoutExpired:child.kill();child.wait(timeout=.5)
    result['returncode']=child.returncode
if output.is_file():
    data=output.read_bytes()
    assert len(data)==28
    (ROOT/'evidence/stage10-nls-probe-output.bin').write_bytes(data)
    result.update(output_hex=data.hex(),actual_GetACP=int.from_bytes(data[:4],'little'),GetCPInfo_bytes=data[4:24].hex(),actual_GetCPInfo_success=int.from_bytes(data[24:28],'little'),output_sha256=hashlib.sha256(data).hexdigest())
else:result['failure']='no 28-byte output'
result['elapsed_seconds']=round(time.monotonic()-started,3)
(ROOT/'evidence/stage10-nls-direct-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
