"""Meaningful own-data roundtrip, corruption, timeout and transition tests."""
import collections,json,os,struct,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,V,put,sha,Boundary
from binary_host import dump,load,prepare,HostStore
from progress_gate import Progress,Limit
from role_process import chain,run_role
from platform_model import Platform
def main():
 out=V/'runs/component-tests';out.mkdir();passed=[]
 def deny(name,fn):
  try:fn()
  except(Exception)as e:passed.append({'name':name,'reason':str(e)});return
  raise AssertionError(name+' did not refuse')
 data={('key',1):[None,True,False,-1,2**64,2**100,0.25,'皇帝',b'\0\xff',tuple([1,2]),{3,4},collections.deque([1,2],maxlen=4)],'u32_list':list(range(100000))};p=out/'typed.bin';info=dump(data,p);assert load(p,info['sha256'])==data;passed.append({'name':'complete_all_types_and_uint32_packed_roundtrip'})
 b=p.read_bytes();bad=out/'corrupt.bin';bad.write_bytes(b[:-1]);deny('truncated',lambda:load(bad));bad2=out/'hash-corrupt.bin';bad2.write_bytes(b[:-1]+bytes([b[-1]^1]));deny('hash_mismatch',lambda:load(bad2,info['sha256']));extra=out/'extra.bin';extra.write_bytes(b+b'x');deny('trailing_payload',lambda:load(extra))
 h={'imports':{1:('dll','name')},'last_error':3,'allocations':{4:(1,8)},'opaque':b'captured\x00','tls':{0:1}};manifest=prepare(h,out/'base');store=HostStore(manifest);deny('immutable_nested_write',lambda:store['imports'].__setitem__(2,('x','y')));store['last_error']=9;store['allocations'][8]=(1,16);frozen=store.freeze(out);restored=HostStore(frozen);assert restored['last_error']==9 and restored['allocations']=={4:(1,8),8:(1,16)}and restored['opaque']==h['opaque'];passed.append({'name':'lazy_delta_roundtrip_immutable_content_pinned'})
 policy={'initial_phase':7,'initial_advances':2,'initial_entries':2,'initial_tail_returns':0,'maximum_total_advances':3,'maximum_total_entries':4,'maximum_total_tail_returns':1,'maximum_writes':2,'cross_cycle_allowed':False,'scheduler_entry':100,'scheduler_return':200,'tail_entry':300}
 g=Progress(policy);g.event('entry',1,100,1000,7,200);g.event('return',1,200,1004,8);assert g.snapshot()['phase']==8;deny('advance_limit',lambda:g.event('entry',1,100,1000,8,200));g.before_write(4);g.before_write(1);deny('write_limit_before_authored_store',lambda:g.before_write(1));deny('worker',lambda:Progress(policy).event('entry',2,100,1000,7,200));deny('wrong_return_SP',lambda:Progress(policy).event('return',1,200,1008,8));p50=dict(policy,initial_phase=50,maximum_total_advances=51);g50=Progress(p50);g50.event('entry',1,100,1000,50,200);deny('tail_cross_cycle',lambda:g50.event('tail',1,300,1000,0));deny('phase_wrap_return',lambda:g50.event('return',1,200,1004,0));deny('enable_cross_cycle',lambda:Progress(dict(policy,cross_cycle_allowed=True)));gskip=Progress(policy);gskip.event('entry',1,100,1000,7,200);deny('phase_skip',lambda:gskip.event('return',1,200,1004,9));passed.append({'name':'accepted_exact_authored_phase7_to8_transition'})
 td=out/'timeout';td.mkdir();cmd=['/usr/bin/python3','-B','-c','import time,subprocess;subprocess.Popen(["/usr/bin/python3","-c","import time;time.sleep(10)"]);time.sleep(10)'];r=run_role('capture',cmd,td,.2);assert r['timed_out']and r['process_group_killed_on_timeout'];deny('duplicate_role_claim',lambda:run_role('capture',cmd,td,.2));deny('wall_over_60',lambda:run_role('native',cmd,td,61));passed.append({'name':'timeout_kills_role_and_descendants'})
 cd=out/'independent';cd.mkdir();start=time.monotonic();rs=chain({k:['/usr/bin/python3','-B','-c','import time;time.sleep(.16)']for k in ('capture','reference','native')},cd,.3);assert len(rs)==3 and all(not r['timed_out']and r['returncode']==0 for r in rs)and time.monotonic()-start>.3;passed.append({'name':'three_roles_do_not_share_one_wall_budget'})
 fd=out/'stop';fd.mkdir();rs=chain({'capture':['/usr/bin/false'],'reference':['/usr/bin/true'],'native':['/usr/bin/true']},fd,.2);assert len(rs)==1 and not(fd/'reference.claim.json').exists();passed.append({'name':'failed_A_cannot_start_B_or_C'})
 platform=Platform({},None);deny('unsupported_API',lambda:platform.invoke(100,[],0));deny('original_pipeline_closed',lambda:__import__('subprocess').run(['/usr/bin/python3','-B',str(V/'tools/pipeline.py'),'--config',str(ROOT/'stage83/batch-v1/configs/round01.json')],capture_output=True,check=True,timeout=10))
 put(V/'evidence/component-tests.json',{'status':'OWN_DATA_AND_PROCESS_GATES_PASSED','cases':passed,'case_count':len(passed),'original_or_Unicorn_execution':0});print(json.dumps({'cases_passed':len(passed),'original_execution':0}))
if __name__=='__main__':main()
