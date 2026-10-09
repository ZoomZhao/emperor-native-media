"""Concrete A/B/C commands. Original execution gate precedes all engine loads."""
import argparse,json,os,struct,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,sha,put,verify,exclusive_dir
from write_gate import install,sandbox_command,check_path
def preflight(c,path):
 if c['maximum_attempts']!={'capture':1,'reference':1,'native':1}or not c['no_retry']or c['old_ledger_reusable']:raise RuntimeError('Scope mismatch')
 if c['kind']=='original'and not c['pins']:raise RuntimeError('Original config not frozen')
 verify(c['pins']);build=json.loads((ROOT/c['native_driver']).parent.joinpath('result.json').read_text())
 if build['kind']!=c['kind']or build['native_driver_sha256']!=sha(ROOT/c['native_driver']):raise RuntimeError('Native driver kind/pin mismatch')
 if c['kind']=='original':
  ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text())
  if len(ledger['executions'])!=20 or ledger['remaining_processes']!=80 or not ledger['goal_closed']:raise RuntimeError('Protected ledger differs')
 return {'status':'PREFLIGHT_DATA_ONLY_PASSED','configuration_sha256':sha(path),'kind':c['kind'],'original_execution_authorized':False,'natural_input_available':False,'unknowns_stop_attempt':True,'maximum_new_role_processes':3}
def gate(c,config,approval,out):
 if c['kind']!='original':raise RuntimeError('Only --fixture can run authored data')
 if not approval:raise RuntimeError('ORIGINAL_EXECUTION_GATE_CLOSED: new explicit user authorization missing')
 a=json.loads(Path(approval).read_text())
 scope={'id':c['id'],'configuration_sha256':sha(config),'maximum_attempts':c['maximum_attempts'],'old_ledger_reopened':False,'input_or_activity_mutation':False,'worker_scheduling':False,'UI_keyboard_audio_video':False}
 if a.get('status')!='AUTHORIZED_BY_USER'or not a.get('user_message_id')or a.get('scope')!=scope:raise RuntimeError('New concrete approval scope mismatch')
 # Root claim is durable and exclusive BEFORE any engine/game process.
 claim=S/'evidence/original-ABC-attempt.claim.json';put(claim,{'scope':scope,'approval_sha256':sha(approval),'output':str(out),'retry_allowed':False,'role_attempt_accounting':'each immutable original-ROLE-attempt.claim.json is the authoritative charge'});return a
def worker(c,role,source,out,ticket):
 if c['kind']=='original':
  claim=json.loads((S/'evidence/original-ABC-attempt.claim.json').read_text());expected=json.loads((Path(out).parent/'approved-scope.json').read_text());verify(c['pins'])
  if claim['scope']!=expected or claim['scope']['configuration_sha256']!=sha(c['_config']):raise RuntimeError('Worker claim mismatch')
  root=Path(claim['output']).resolve()
  expected_source=(ROOT/c['source']).resolve()if role=='capture'else root/'capture/snapshot.json'
  if Path(out).resolve()!=root/role or Path(source).resolve()!=expected_source:raise RuntimeError('Worker output/input differs from approved chain')
  roleclaim=S/('evidence/original-'+role+'-attempt.claim.json');put(roleclaim,{'scope':expected,'role':role,'input_sha256':sha(source),'retry_allowed':False})
 if role!='capture':
  proof=json.loads((Path(source).parent/'result.json').read_text());snapshot=json.loads(Path(source).read_text());cpu=snapshot['public_CPU'][str(snapshot['current_tid'])]
  if proof['status']!='NATURAL_ENTRY_CAPTURED'or proof['natural_call']['caller']!=c['caller']or proof['natural_call']['return']!=c['return']or cpu['EIP']!=c['entry']or cpu['ESP']!=proof['natural_call']['caller_SP']-4:raise RuntimeError('Missing genuine natural capture proof')
 if role!='native':
  from engine import run
  result=run(c,role,source,out)
  if role=='capture':
   from activity import inspect
   put(out/'activity.json',inspect(out/'snapshot.json')if c['kind']=='original'else {'status':'AUTHORED_FIXTURE','eligible_receivers':1})
  print(json.dumps(result));return
 from common import verify
 verify(json.loads((Path(source).parent/'frozen.json').read_text())['files'])
 limit=c['limits']['native'];lp=out/'limits.bin';lp.write_bytes(struct.pack('<3Q',limit['steps'],limit['writes'],int(limit['guest_seconds']*1e9)));tp=out/'execution-ticket.bin';tp.write_bytes(b'S82T'+struct.pack('<I',1 if c['kind']=='original'else 2)+bytes.fromhex(sha(c['_config']))+bytes.fromhex(sha(source)))
 with(out/'driver.stdout.log').open('xb')as o,(out/'driver.stderr.log').open('xb')as e:q=subprocess.run([str(ROOT/c['native_driver']),str(Path(source).parent),str(out),str(lp),str(tp)],cwd=ROOT,stdout=o,stderr=e,timeout=30)
 if q.returncode:raise RuntimeError('NATIVE_BOUNDARY returncode='+str(q.returncode))
def main():
 p=argparse.ArgumentParser();p.add_argument('--config',type=Path,required=True);p.add_argument('--preflight',action='store_true');p.add_argument('--fixture',action='store_true');p.add_argument('--approval',type=Path);p.add_argument('--out',type=Path);p.add_argument('--worker',choices=['capture','reference','native']);p.add_argument('--source',type=Path);p.add_argument('--sandboxed',action='store_true',help=argparse.SUPPRESS);a=p.parse_args();c=json.loads(a.config.read_text());c['_config']=str(a.config.resolve())
 if not a.preflight:
  if not a.out:raise RuntimeError('Need exact approved output')
  expected=Path(c['approved_output_root']).resolve();actual=a.out.absolute()
  if '..'in actual.parts or(actual.resolve()!=expected if not a.worker else actual.resolve()!=expected/a.worker):raise RuntimeError('ABC_OUTPUT_ROOT_MISMATCH')
  check_path(actual,expected)
  if not a.sandboxed:
   install(expected);cmd=sandbox_command(expected,['/usr/bin/python3','-B',str(S/'tools/pipeline.py'),*sys.argv[1:],'--sandboxed']);q=subprocess.run(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=190 if not a.worker else 60)
   # Child stdio never inherits a regular caller file outside the output tree.
   if not expected.exists():expected.mkdir(exist_ok=False)
   if not a.worker:
    with(expected/'controller.stdout.log').open('xb')as f:f.write(q.stdout)
    with(expected/'controller.stderr.log').open('xb')as f:f.write(q.stderr)
   import stat
   if not stat.S_ISREG(os.fstat(1).st_mode):sys.stdout.buffer.write(q.stdout)
   if not stat.S_ISREG(os.fstat(2).st_mode):sys.stderr.buffer.write(q.stderr)
   raise SystemExit(q.returncode)
  install(expected)
 if a.worker:
  # Original workers cannot create their own root authority; fixtures use only
  # the separate authored binary, with original mode impossible there.
  if c['kind']=='authored'and not a.fixture:raise RuntimeError('Authored worker requires --fixture')
  worker(c,a.worker,a.source,a.out,None);return
 ready=preflight(c,a.config)
 if a.preflight:print(json.dumps(ready));return
 if not a.out:raise RuntimeError('Need new exclusive output')
 out=a.out.resolve()
 if not out.is_relative_to(S/'runs')or out==S/'runs'or out.exists():raise RuntimeError('Need unused Stage82 runs subdirectory')
 if c['kind']=='original':gate(c,a.config,a.approval,out)
 elif not a.fixture or a.approval:raise RuntimeError('Authored fixture gate mismatch')
 out=exclusive_dir(out)
 if c['kind']=='original':put(out/'approved-scope.json',json.loads((S/'evidence/original-ABC-attempt.claim.json').read_text())['scope'])
 for role in ('capture','reference','native'):
  d=out/role;d.mkdir();source=ROOT/c['source']if role=='capture'else out/'capture/snapshot.json';cmd=['/usr/bin/python3','-B',str(S/'tools/pipeline.py'),'--config',str(a.config.resolve()),'--worker',role,'--source',str(source),'--out',str(d)]
  if c['kind']=='authored':cmd+=['--fixture']
  cmd+=['--sandboxed'] # inherits the controller's kernel write policy
  with(d/'worker.stdout.log').open('xb')as o,(d/'worker.stderr.log').open('xb')as e:
   try:q=subprocess.run(cmd,cwd=ROOT,stdout=o,stderr=e,timeout=60);rc=q.returncode
   except subprocess.TimeoutExpired:rc=124
  put(d/'process.json',{'returncode':rc,'hard_process_seconds':60,'retry_allowed':False})
  if rc:put(out/'result.json',{'status':'BOUNDARY_NOT_SUCCESS','role':role,'returncode':rc});raise RuntimeError('Stopped at '+role)
  verify(c['pins'])
 from compare import compare
 r=compare(out/'capture',out/'reference',out/'native',out/'differential.json');activity=json.loads((out/'capture/activity.json').read_text());r['scope_label']='AUTHORED_DRIVER_PROTOCOL_ONLY'if c['kind']=='authored'else('ACTIVITY_PATH_DIFFERENTIAL'if activity.get('eligible_receivers',0)>0 else 'EMPTY_PATH_ONLY');put(out/'result.json',r);print(json.dumps(r))
 if r['mismatches']:raise RuntimeError('Differential mismatch')
if __name__=='__main__':main()
