"""Own behavioral failure cases through production sandbox/controller/engine/freeze."""
from probe_common import *
import subprocess
from binary_host import load
EXPECTED={
 'natural':None,'unknown-callback':'UNPROVEN_INDIRECT_TARGET_BEFORE_CALL',
 'unknown-instruction':'UNPROVEN_PREFIX_MULTI_STORE_OR_SEGMENT',
 'write-cap':'WRITE_CAP_BEFORE_INSTRUCTION_OR_API','invalid-address':'UNMAPPED_RANGE',
 'second-advance':'ONE_ADVANCE_NO_WRAP_BEFORE_PHASE_WRITE',
 'heap-realloc-cap':'WRITE_CAP_BEFORE_INSTRUCTION_OR_API',
 'thread3-ready':'THREAD3_NATURALLY_READY_NO_EXECUTION',
 'soft-role-timeout':'ROLE_SOFT50_FREEZE_RESERVE10',
 'soft-guest-timeout':'SHARED_GUEST_WALL_CAP','step-cap':'INSTRUCTION_CAP_BEFORE_INSTRUCTION',
 'changed-code':'IMMUTABLE_EXECUTABLE_BLOCK_CHANGED',
 'unknown-api':'UNSUPPORTED_API_BEFORE_IMPLEMENTATION','phase-wrap':'ONE_ADVANCE_NO_WRAP_BEFORE_PHASE_WRITE'}

def host_field(snapshot,key):
 manifest=json.loads((ROOT/snapshot['host_manifest']).read_text())
 row=next(r for r in manifest['fields']if r['key']==key)
 return load(ROOT/row['path'],row['sha256'])
def memory(snapshot,out,a,n):
 row=next(r for r in snapshot['regions']if r['low']<=a and a+n<=r['high']+1)
 with(out/'memory.bin').open('rb')as f:f.seek(row['offset']+a-row['low']);return f.read(n)

def main():
 import argparse
 ap=argparse.ArgumentParser();ap.add_argument('--version',required=True);ap.add_argument('--audit-existing',action='store_true');args=ap.parse_args();version=args.version
 assert version.isalnum()
 rows=[];check_old();before=accounting()
 for case,expected in EXPECTED.items():
  out=ROOT/'stage83/runs'/('phase7-probe-own-'+case+'-'+version)
  cmd=['/usr/bin/python3','-B',str(N/'tools/probe.py'),'--self-test','--case',case,'--output',str(out)]
  if args.audit_existing:
   from types import SimpleNamespace
   old=json.loads((out/'process.json').read_text());q=SimpleNamespace(returncode=0 if old['returncode']==0 and not old['timed_out']else 2)
  else:q=subprocess.run(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=65)
  s=json.loads((out/'snapshot.json').read_text());r=json.loads((out/'result.json').read_text());p=json.loads((out/'process.json').read_text())
  verify(json.loads((out/'frozen.json').read_text())['files'])
  assert r['snapshot_complete']and not p['timed_out']and s['original_source_instructions']==0
  if expected is None:
   assert q.returncode==0 and r['status']=='NATURAL_ENTRY_CAPTURED'
   assert s['qualified_natural_input']and len(host_field(s,'watch_state54')['advances'])==3
   assert host_field(s,'thread_switches')>71 and host_field(s,'thread_waits')>=9
   assert host_field(s,'thread_log')[0]=={'own_prior_prefix':'retain'}
   assert host_field(s,'api_trace')[0]=={'own_prior_prefix':'retain'}
   assert host_field(s,'guest_threads20')[2]['tls'][0]==0x2222
   assert host_field(s,'heap_next')==0x10001020 and host_field(s,'allocations')[0x10001000]==(0x3001,32)
   assert memory(s,out,0x10001000,32)==bytes(32)and memory(s,out,0x6f200240,4)==struct.pack('<I',1)
  else:
   assert q.returncode==2 and r['status']=='DIAGNOSTIC_ONLY_UNQUALIFIED'and not s['qualified_natural_input']
   assert r['boundary']['kind']==expected,(case,r)
  if case=='unknown-callback':
   assert memory(s,out,0x6f200350,4)==bytes(4)
   assert int.from_bytes(memory(s,out,0x6f200300,4),'little')==0x6f200400 # NULL became a real object naturally in own code.
   assert memory(s,out,s['public_CPU'][str(s['current_tid'])]['ESP'],4)==struct.pack('<I',1)
   assert r['boundary']['details']['target']==0x6f100700
  if case in ('write-cap','invalid-address','phase-wrap','second-advance'):
   assert s['budget']['admitted_steps']==0 and s['budget']['writes']==0
   assert memory(s,out,0x6f200200,4)==bytes(4)
  if case=='heap-realloc-cap':
   assert host_field(s,'heap_next')==0x10001000
   assert host_field(s,'allocations')=={0x6f200600:(0x3001,8)}
   assert memory(s,out,0x10001000,16)==bytes(16)
  if case=='unknown-api':assert len(host_field(s,'keyboard_snapshot61')['device']['payload']['log'])==7
  if case in ('thread3-ready','soft-role-timeout','soft-guest-timeout'):assert s['budget']['admitted_steps']==0
  rows.append({'case':case,'passed':True,'boundary':expected,'admitted_steps':s['budget']['admitted_steps'],'writes':s['budget']['writes'],'snapshot_complete':True,'process_seconds':p['elapsed_seconds']})
 # Hard controller timeout: preserve and explicitly distinguish the earlier complete checkpoint.
 out=ROOT/'stage83/runs'/('phase7-probe-own-hard-timeout-'+version)
 if args.audit_existing:
  from types import SimpleNamespace
  q=SimpleNamespace(returncode=2)
 else:q=subprocess.run(['/usr/bin/python3','-B',str(N/'tools/probe.py'),'--self-test','--case','hard-timeout','--output',str(out)],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
 p=json.loads((out/'process.json').read_text());assert q.returncode==2 and p['timed_out']and p['process_group_killed']and not p['kill_time_state_snapshot_complete']and p['earlier_complete_snapshot_retained']
 rows.append({'case':'hard-timeout','passed':True,'hard_seconds':2,'kill_time_snapshot_complete':False,'earlier_complete_snapshot_retained':True})
 # Missing and obsolete genuine approvals refuse before any engine import or original claim.
 gatecmd="import sys;from pathlib import Path;sys.path.insert(0,str(Path('stage83/phase7-probe-v1/tools').resolve()));import probe;sys.argv=['probe','--run'];\ntry:probe.main()\nexcept Exception as e:print(str(e));print('unicorn_imported='+str('unicorn' in sys.modules));raise SystemExit(2)"
 q=subprocess.run(['/usr/bin/python3','-B','-c',gatecmd],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
 assert q.returncode==2 and b'unicorn_imported=False'in q.stdout
 q=subprocess.run(['/usr/bin/python3','-B',str(N/'tools/probe.py'),'--run','--approval',str(ROOT/'stage83/batch-v1/evidence/authorization.json')],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
 assert q.returncode and b'OLD_8SEC_APPROVAL'in q.stderr
 assert accounting()==before;check_old()
 put(N/('evidence/own-executor-tests-'+version+'.json'),{'status':'PRODUCTION_EXECUTOR_OWN_BEHAVIORAL_CASES_PASS','cases':rows,'case_count':len(rows),'missing_approval_engine_imported':False,'old_approval_rejected':True,'original_rounds_before':before,'original_rounds_after':accounting(),'original_instructions':0,'audit_existing_without_guest_reexecution':args.audit_existing})
 print(json.dumps({'status':'OWN_EXECUTOR_TESTS_PASS','cases':len(rows),'original_rounds':before}))

if __name__=='__main__':main()
