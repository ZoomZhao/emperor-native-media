#!/usr/bin/env python3
"""Bounded observation of the already approved local game copy."""
import json
import subprocess
import sys
import time
from pathlib import Path

root = Path(__file__).resolve().parents[1]
tag = sys.argv[1]
assert tag.isalnum()
budget = int(sys.argv[2]) if len(sys.argv) > 2 else 900
assert 0 < budget <= 1200
contents = root / 'stage8-local/EmperorBaseline.app/Contents'
command = [str(contents / 'MacOS/wineskinlauncher')]
log = root / f'evidence/stage9-{tag}-launcher.log'
started = time.monotonic()
with log.open('w') as output:
    child = subprocess.Popen(command, cwd=contents, stdout=output, stderr=subprocess.STDOUT)
    result = dict(command=command, cwd=str(contents), launcher_pid=child.pid,
                  budget_seconds=budget, tag=tag)
    record = root / f'evidence/stage9-{tag}-launch.json'
    record.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result), flush=True)
    while time.monotonic() - started < budget:
        if (root / f'stage8-local/stage9-complete-{tag}').exists():
            result['end_reason'] = 'observation_complete_marker'
            break
        time.sleep(2)
    else:
        result['end_reason'] = 'observation_budget_reached'
    result.update(launcher_returncode=child.poll(), elapsed_seconds=round(time.monotonic() - started, 2))
    record.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result), flush=True)
