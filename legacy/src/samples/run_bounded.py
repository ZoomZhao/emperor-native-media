#!/usr/bin/env python3
"""Execute one approved command with a hard 60-second timeout and audit logs."""
import sys, subprocess, json, time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
label,cwd,*cmd=sys.argv[1:]
assert label.replace('-','').replace('_','').isalnum()
start=time.monotonic()
with (root/'evidence'/(label+'.stdout.log')).open('wb') as out, (root/'evidence'/(label+'.stderr.log')).open('wb') as err:
    try:
        r=subprocess.run(cmd,cwd=root/cwd,stdout=out,stderr=err,timeout=60)
        result=dict(command=cmd,cwd=str(root/cwd),returncode=r.returncode,timed_out=False)
    except subprocess.TimeoutExpired:
        result=dict(command=cmd,cwd=str(root/cwd),returncode=None,timed_out=True)
result['duration_seconds']=round(time.monotonic()-start,3)
(root/'evidence'/(label+'.result.json')).write_text(json.dumps(result,indent=2))
print(json.dumps(result))
sys.exit(124 if result['timed_out'] else result['returncode'])
