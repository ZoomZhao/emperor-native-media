"""One independent process group per role. No shared ABC wall budget."""
import json,os,signal,subprocess,time
from pathlib import Path
from common import put
def run_role(role,command,out,seconds=60):
 if role not in ('capture','reference','native')or not 0<seconds<=60:raise ValueError('ROLE_OR_WALL_SCOPE')
 out=Path(out);put(out/(role+'.claim.json'),{'role':role,'maximum_executions':1,'hard_wall_seconds':seconds,'retry_allowed':False})
 start=time.monotonic();timed=False
 with(out/(role+'.stdout.log')).open('xb')as o,(out/(role+'.stderr.log')).open('xb')as e:
  p=subprocess.Popen(command,stdout=o,stderr=e,start_new_session=True)
  try:rc=p.wait(timeout=seconds)
  except subprocess.TimeoutExpired:
   timed=True;os.killpg(p.pid,signal.SIGKILL);rc=p.wait()
 result={'role':role,'returncode':rc,'timed_out':timed,'hard_wall_seconds':seconds,'elapsed_seconds':time.monotonic()-start,'process_group_killed_on_timeout':timed};put(out/(role+'.process.json'),result);return result
def chain(commands,out,seconds=60):
 results=[]
 for role in ('capture','reference','native'):
  r=run_role(role,commands[role],out,seconds);results.append(r)
  if r['returncode']or r['timed_out']:break
 return results
