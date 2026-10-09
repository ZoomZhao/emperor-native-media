#!/usr/bin/env python3
"""Keep the approved local launcher session alive for bounded GUI observation."""
import json, subprocess, time, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
contents=root/'stage8-local/EmperorBaseline.app/Contents'
command=[str(contents/'MacOS/wineskinlauncher')]
tag=sys.argv[1] if len(sys.argv)>1 else ''
assert not tag or tag.isalnum()
prefix='stage8-supervised'+('-'+tag if tag else '')
log=root/f'evidence/{prefix}-launcher.log'
budget=300 if tag else 900
started=time.monotonic()
with log.open('w') as out:
    child=subprocess.Popen(command,cwd=contents,stdout=out,stderr=subprocess.STDOUT)
    result=dict(command=command,cwd=str(contents),launcher_pid=child.pid,budget_seconds=budget)
    (root/f'evidence/{prefix}-launch.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result),flush=True)
    # Yielding exec keeps this local session reviewable while separate short
    # commands inspect and interact with the game. No long blocking tool wait.
    while time.monotonic()-started<budget:
        if (root/f'stage8-local/observation-complete-{tag or "initial"}').exists():break
        time.sleep(2)
    result.update(launcher_returncode=child.poll(),elapsed_seconds=round(time.monotonic()-started,2))
    (root/f'evidence/{prefix}-launch.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result),flush=True)
