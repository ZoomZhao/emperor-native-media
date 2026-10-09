"""Authored-only integration. Original execution is closed in this repair task."""
import argparse,json,struct,subprocess,sys,os,stat
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,V,sha,put,verify
from role_process import run_role
from binary_host import HostStore
from write_gate import install,sandbox_command
ROLES=('capture','reference','native')
def main():
 p=argparse.ArgumentParser();p.add_argument('--config',type=Path);p.add_argument('--role',choices=ROLES);p.add_argument('--worker',choices=ROLES);p.add_argument('--all-fixture',action='store_true');p.add_argument('--describe-original',action='store_true');p.add_argument('--sandboxed',action='store_true',help=argparse.SUPPRESS);a=p.parse_args()
 if a.describe_original:print(json.dumps({'status':'ORIGINAL_EXECUTION_CLOSED_NO_NEW_AUTHORIZATION','approved_capture_seconds':8,'approved_capture_steps':6000000,'used_rounds':1,'remaining_rounds':9,'current_phase':22,'target_phase':8,'cross_cycle_allowed':False,'target_requires_cross_cycle':True}));return
 cfg=a.config.resolve();c=json.loads(cfg.read_text());c['_config']=str(cfg)
 if c['kind']!='authored':raise RuntimeError('ORIGINAL_EXECUTION_CLOSED_DURING_TOOL_REPAIR')
 verify(c['pins']);out=Path(c['approved_output_root']);expected=S/'runs/repair-v2-authored-ready'
 if out!=expected:raise RuntimeError('OUTPUT_SCOPE')
 if not a.sandboxed:
  install(expected);q=subprocess.run(sandbox_command(expected,['/usr/bin/python3','-B',str(V/'tools/pipeline_ready.py'),*sys.argv[1:],'--sandboxed']),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=190)
  if out.exists():
   with(out/'controller.stdout.log').open('xb')as f:f.write(q.stdout)
   with(out/'controller.stderr.log').open('xb')as f:f.write(q.stderr)
  if not stat.S_ISREG(os.fstat(1).st_mode):sys.stdout.buffer.write(q.stdout)
  if not stat.S_ISREG(os.fstat(2).st_mode):sys.stderr.buffer.write(q.stderr)
  raise SystemExit(q.returncode)
 install(expected)
 if a.worker:
  role=a.worker;d=out/role;claim=json.loads((out/(role+'.claim.json')).read_text());assert claim['role']==role
  source=ROOT/c['source']if role=='capture'else out/'capture/snapshot.json'
  if role!='capture':
   proof=json.loads((source.parent/'result.json').read_text());s=json.loads(source.read_text());assert proof['status']=='NATURAL_ENTRY_CAPTURED'and s['public_CPU'][str(s['current_tid'])]['EIP']==c['entry'];verify(json.loads((source.parent/'frozen.json').read_text())['files'])
  if role!='native':
   from engine_ready import run
   result=run(c,role,source,d);print(json.dumps({'role':role,'status':result['status']}));return
  limits=c['limits']['native'];lp=d/'limits.bin';lp.write_bytes(struct.pack('<3Q',limits['steps'],limits['writes'],int(limits['guest_seconds']*1e9)));tp=d/'ticket.bin';tp.write_bytes(b'S82T'+struct.pack('<I',2)+bytes.fromhex(sha(cfg))+bytes.fromhex(sha(source)))
  with(d/'driver.stdout.log').open('xb')as o,(d/'driver.stderr.log').open('xb')as e:q=subprocess.run([str(ROOT/c['native_driver']),str(source.parent),str(d),str(lp),str(tp)],stdout=o,stderr=e,timeout=30)
  if q.returncode:raise RuntimeError('AUTHORED_NATIVE_BOUNDARY '+str(q.returncode))
  return
 if not out.exists():out.mkdir();put(out/'scope.json',{'configuration_sha256':sha(cfg),'kind':'authored','original_execution_authority':False})
 if json.loads((out/'scope.json').read_text())['configuration_sha256']!=sha(cfg):raise RuntimeError('FROZEN_SCOPE_MISMATCH')
 roles=ROLES if a.all_fixture else(a.role,)
 for role in roles:
  if role not in ROLES:raise RuntimeError('ROLE_REQUIRED')
  index=ROLES.index(role)
  if index:
   prior=json.loads((out/(ROLES[index-1]+'.process.json')).read_text())
   if prior['returncode']or prior['timed_out']:raise RuntimeError('PREVIOUS_ROLE_FAILED_NO_CONTINUATION')
  d=out/role;d.mkdir(exist_ok=False)
  cmd=['/usr/bin/python3','-B',str(V/'tools/pipeline_ready.py'),'--config',str(cfg),'--worker',role,'--sandboxed']
  r=run_role(role,cmd,out,60)
  if r['returncode']or r['timed_out']:raise RuntimeError('STOP_FAILED_ROLE '+role)
 if(out/'native.process.json').exists():
  from compare import compare
  r=compare(out/'capture',out/'reference',out/'native',out/'differential.json');put(out/'result.json',r);print(json.dumps(r))
  if r['mismatches']:raise RuntimeError('AUTHORED_DIFFERENTIAL')
if __name__=='__main__':main()
