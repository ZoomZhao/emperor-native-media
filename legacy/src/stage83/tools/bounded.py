import json,subprocess,sys,time
from pathlib import Path
E=Path(__file__).resolve().parents[1]/'evidence';ROOT=E.parents[1]
label,*cmd=sys.argv[1:];assert label.replace('-','').replace('_','').isalnum();start=time.monotonic()
with(E/(label+'.stdout.log')).open('xb')as o,(E/(label+'.stderr.log')).open('xb')as e:
 try:q=subprocess.run(cmd,cwd=ROOT,stdout=o,stderr=e,timeout=60);r={'returncode':q.returncode,'timed_out':False}
 except subprocess.TimeoutExpired:r={'returncode':124,'timed_out':True}
r.update(command=cmd,hard_process_seconds=60,duration_seconds=round(time.monotonic()-start,3));(E/(label+'.result.json')).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r));sys.exit(r['returncode'])
