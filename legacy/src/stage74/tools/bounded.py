"""Independent stage74 60-second command/log wrapper; never overwrites logs."""
import json,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];E=ROOT/'stage74/evidence'
label,*command=sys.argv[1:];assert label.replace('-','').replace('_','').isalnum()
start=time.monotonic()
with (E/(label+'.stdout.log')).open('xb')as out,(E/(label+'.stderr.log')).open('xb')as err:
 try:
  q=subprocess.run(command,cwd=ROOT,stdout=out,stderr=err,timeout=60)
  r=dict(returncode=q.returncode,timed_out=False)
 except subprocess.TimeoutExpired:r=dict(returncode=124,timed_out=True)
r.update(command=command,hard_process_seconds=60,duration_seconds=round(time.monotonic()-start,3))
(E/(label+'.result.json')).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r));sys.exit(r['returncode'])
