"""Joint own state, thread, surface, callback and shared-budget validation."""
from core import *
from models import *
import copy,struct
def main():
 u=fixed();from fixture import make,DATA
 from unicorn import x86_const as x
 cases=[]
 def case(name,fn):
  r,meta=make(u)
  try:details=fn(r,meta);cases.append({'name':name,'passed':True,'details':details})
  finally:r.events.close()
 def rejects(fn,needle):
  try:fn()
  except Exception as e:
   if needle not in str(e):raise
   return str(e)
  raise AssertionError('Expected boundary '+needle)
 def e2e(r,m):
  assert r.current_tid==2 and r.threads[2]['status']=='waiting'and r.threads[4]['status']=='runnable';initial={str(k):v['status']for k,v in r.threads.items()};t=time.monotonic();q=r.thread_run(3,100)
  assert q['status']=='natural_authored_call'and r.goal and r.scheduled==[4,1,2,4,1]and r.callbacks.pending is None
  assert r.readword(DATA+4)==1 and r.readword(DATA+8)==2 and r.readword(DATA+0x10)==0x60005000 and r.readword(DATA+0x14)==4 and r.readword(DATA+0x18)==0x2222
  source=r.m.read(m['src'],24);assert source==m['expected_source'];expected=bytearray(b'\xa5'*80);expected[24:36]=source[:12];expected[44:56]=source[12:];assert r.m.read(m['dst'],80)==expected
  assert len(r.m.trace)==r.m.budget.writes and sum(r.m.budget.by_thread.values())==r.m.budget.steps and r.threads[3]['status']=='waiting'
  result={'status':'REAL_THREAD_EVENT_LOCK_PIXEL_WRITE_UNLOCK_BLT_CALLBACK_NATURAL_CALL_PASS','seconds':time.monotonic()-t,'initial_current_tid':2,'initial_statuses':initial,'scheduled_threads':r.scheduled,'steps':r.m.budget.steps,'writes':r.m.budget.writes,'steps_by_thread':r.m.budget.by_thread,'write_reservations':r.m.budget.reservations,'API_log':r.api_log,'natural_call':{'caller':r.goal_contract['caller'],'entry':r.uc.reg_read(x.UC_X86_REG_EIP),'return':r.readword(r.uc.reg_read(x.UC_X86_REG_ESP)),'entry_SP':r.uc.reg_read(x.UC_X86_REG_ESP),'caller_SP':r.caller_sp},'final_thread_statuses':{str(k):v['status']for k,v in r.threads.items()},'source_pixel_sha256':hashlib.sha256(source).hexdigest(),'destination_pixel_sha256':hashlib.sha256(expected).hexdigest(),'original_instructions':0,'input_CPU_from_original_context':False,'original_windows_or_SR_library_loaded':False}
  put(V/'evidence/authored-end-to-end-v2.json',result);put(V/'evidence/authored-ordered-writes-v2.json',r.m.trace);return {'steps':result['steps'],'writes':result['writes']}
 case('end_to_end_four_threads_real_surface_and_callback',e2e)
 def timeout(r,m):
  sp=r.uc.reg_read(x.UC_X86_REG_ESP);r.events.clock_source=lambda:11.0;r.thread_wake();t=r.threads[2];r.uc.context_restore(t['context']);assert t['status']=='runnable'and t['wait']is None and r.uc.reg_read(x.UC_X86_REG_EAX)==258 and r.uc.reg_read(x.UC_X86_REG_ESP)==sp+16;return 'Timeout258 returns through captured 3-argument stack; no instruction executed'
 case('waiting_thread_timeout_real_return_ABI',timeout)
 def infinite(r,m):
  r.threads[2]['wait']['deadline']=None;r.events.clock_source=lambda:1e12;r.thread_wake();assert r.threads[2]['status']=='waiting'and r.threads[3]['status']=='waiting'and not r.m.budget.steps;return 'No artificial timeout or completion'
 case('infinite_wait_does_not_fake_activity',infinite)
 def event_semantics(r,m):
  e=r.events;auto,_=e.invoke('create',[0,1],55);assert e.invoke('wait',[auto,0],55)==(0,55)and not e.query(auto)['signaled']and e.invoke('wait',[auto,0],55)==(258,55);manual,_=e.invoke('create',[1,1],55);assert e.invoke('wait',[manual,0],55)==(0,55)and e.invoke('wait',[manual,0],55)==(0,55);blob=e.export();f=Events(blob);assert f.export()==blob;f.close();return 'Real C auto/manual reset, error preservation and snapshot roundtrip'
 case('native_event_auto_manual_reset_and_snapshot',event_semantics)
 def bad_event(r,m):
  assert r.events.invoke('wait',[0x5100ffff,0],123)==(0xffffffff,6);return rejects(lambda:r.events.invoke('wait',[m['start'],10],0),'NONBLOCKING')
 case('invalid_event_and_host_blocking_wait_refused',bad_event)
 def step_cap(r,m):
  r.m.budget.step_cap=24;q=r.thread_run(3,100);assert q['status']=='refused'and 'SHARED_INSTRUCTION_CAP'in str(q)and r.m.budget.steps==24 and len(r.m.budget.by_thread)>1;return r.m.budget.by_thread
 case('instruction_budget_shared_between_workers_and_main',step_cap)
 def write_cap(r,m):
  before=r.m.read(m['src'],24);blob=r.bridge.host['dd_native_blob'];r.m.budget.write_cap=32;msg=rejects(lambda:r.bridge.invoke('2.25',[m['source_object'],0,m['descriptor'],1,0]),'SHARED_WRITE_CAP');assert r.m.read(m['src'],24)==before and r.bridge.host['dd_native_blob']==blob and r.m.budget.writes==0;return msg
 case('whole_Lock_write_plan_rejected_before_pixels_descriptor_or_blob_mutation',write_cap)
 def cross_write(r,m):
  r.m.tid=4;r.m.budget.write_cap=1;r.m.word(DATA,1,'worker4');r.m.tid=1;before=r.m.read(DATA+4,4);msg=rejects(lambda:r.m.word(DATA+4,2,'main1'),'SHARED_WRITE_CAP');assert r.m.read(DATA+4,4)==before and r.m.budget.writes==1;return msg
 case('write_budget_shared_not_reset_at_thread_switch',cross_write)
 def wall(r,m):
  now=[0.0];b=SharedBudget(10,10,.1,lambda:now[0]);b.instruction(4);now[0]=.11;return rejects(lambda:b.instruction(1),'SHARED_GUEST_WALL_CAP')
 case('guest_wall_budget_shared_across_threads',wall)
 def thread3(r,m):
  r.threads[3].update(status='runnable',wait=None);msg=rejects(lambda:r.thread_run(3,100),'UNAUTHORIZED_RUNNABLE_THREAD');assert r.m.budget.steps==0 and r.threads[3]['status']=='runnable';return msg
 case('unlicensed_thread3_natural_readiness_stops_without_fake_wait',thread3)
 def apc(r,m):return rejects(lambda:r.api('kernel32.dll','QueueUserAPC'),'UNIMPLEMENTED_THREAD_SEMANTICS')
 case('APC_is_explicit_failure_not_success_stub',apc)
 def critical(r,m):
  p=DATA+0x380;r.critical_sections[p]=1;r.thread_owners[p]=4;r.thread_load(1);sp=r.uc.reg_read(x.UC_X86_REG_ESP);r.uc.mem_write(sp,struct.pack('<2I',r.goal_contract['caller'],p));assert r.thread_api('kernel32.dll','EnterCriticalSection')and r.threads[1]['wait']=={'kind':'critical','address':p};r.thread_owners[p]=None;r.critical_sections[p]=0;r.thread_wake();assert r.threads[1]['status']=='runnable'and r.uc.reg_read(x.UC_X86_REG_ESP)==sp;return 'Wake re-enters real blocked EnterCriticalSection; no fake lock acquisition'
 case('critical_contention_wait_and_wake',critical)
 def recursive(r,m):
  p=DATA+0x380;r.critical_sections[p]=2;r.thread_owners[p]=4;r.thread_load(1);sp=r.uc.reg_read(x.UC_X86_REG_ESP);r.uc.mem_write(sp,struct.pack('<2I',r.goal_contract['caller'],p));return rejects(lambda:r.thread_api('kernel32.dll','LeaveCriticalSection'),'wrong_thread_leave')
 case('wrong_thread_recursive_lock_leave_refused',recursive)
 def lock(r,m):
  assert r.bridge.invoke('2.25',[m['source_object'],0,m['descriptor'],1,0])==0;n=r.m.budget.writes;before=r.bridge.host['dd_native_blob'];assert r.bridge.invoke('2.25',[m['source_object'],0,m['descriptor'],1,0])==0x80004005 and r.m.budget.writes==n and r.bridge.host['dd_native_blob']==before;r.m.word(m['src'],0x11223344,'own_draw');assert r.bridge.invoke('2.32',[m['source_object'],0])==0;new=r.bridge.host['dd_native_blob'];assert new[3120:3124]==bytes.fromhex('44332211')and struct.unpack_from('<I',new,48+40)[0]==0;assert r.bridge.invoke('2.32',[m['source_object'],0])==0x80004005;return 'Pixels persist in native blob; lock->unlock transition is real'
 case('Lock_descriptor_pixel_sync_and_Unlock_persistence',lock)
 def bltclip(r,m):
  r.m.word(m['destination_rect'],0xffffffff,'own_rect');r.m.word(m['destination_rect']+8,2,'own_rect');assert r.bridge.invoke('2.5',[m['destination_object'],m['destination_rect'],m['source_object'],m['source_rect'],0,0])==0;source=r.m.read(m['src'],24);dest=r.m.read(m['dst'],80);assert dest[20:28]==source[4:12]and dest[40:48]==source[16:24]and dest[:20]==b'\xa5'*20;return 'Negative destination clipped with source advance; padding preserved'
 case('Blt32_clipping_and_row_pitch',bltclip)
 def overlap(r,m):
  r.m.write_many([(m['source_rect'],struct.pack('<4i',0,0,2,1)),(m['destination_rect'],struct.pack('<4i',1,1,3,2))],'own_rect');before=r.m.read(m['src'],24);assert r.bridge.invoke('2.5',[m['source_object'],m['destination_rect'],m['source_object'],m['source_rect'],0,0])==0;assert r.m.read(m['src']+16,8)==before[:8];return 'Snapshot protects same-surface overlapping copy'
 case('same_surface_overlap_snapshot',overlap)
 def flags(r,m):return rejects(lambda:r.bridge.invoke('2.5',[m['destination_object'],m['destination_rect'],m['source_object'],m['source_rect'],1,0]),'BLT_FLAGS_EFFECTS')
 case('unsupported_Blt_flags_refused',flags)
 def stretch(r,m):
  r.m.word(m['destination_rect']+8,5,'own_rect');return rejects(lambda:r.bridge.invoke('2.5',[m['destination_object'],m['destination_rect'],m['source_object'],m['source_rect'],0,0]),'BLT_STRETCH')
 case('unsupported_stretch_refused',stretch)
 def sixteen(r,m):
  b=bytearray(r.bridge.host['dd_native_blob']);struct.pack_into('<I',b,48+36,565);r.bridge.host['dd_native_blob']=bytes(b);before=r.m.read(m['dst'],80);msg=rejects(lambda:r.bridge.invoke('2.5',[m['destination_object'],m['destination_rect'],m['source_object'],m['source_rect'],0,0]),'BLT16_REQUIRES');assert r.m.read(m['dst'],80)==before;return msg
 case('16bit_SR_dependent_Blt_refused_before_guest_store',sixteen)
 def callback(r,m):
  sp=r.uc.reg_read(x.UC_X86_REG_ESP);c=r.callback_contract;q=r.callbacks.enter(2,c['target'],c['return_token'],sp,c['args'],c);assert r.m.read(q[1],8)==struct.pack('<2I',c['return_token'],m['dst']);msg=rejects(lambda:r.callbacks.returned(4,c['return_token'],sp),'CALLBACK_RETURN_OWNER');assert r.callbacks.pending is not None;r.callbacks.returned(2,c['return_token'],sp);return msg
 case('callback_uint32_stack_frame_and_owner_return',callback)
 def badcb(r,m):
  c=r.callback_contract;n=r.m.budget.writes;msg=rejects(lambda:r.callbacks.enter(1,c['target']+1,c['return_token'],STACK if False else DATA+0x800,c['args'],c),'CALLBACK_TARGET');assert r.m.budget.writes==n;return msg
 case('unknown_callback_target_rejected_before_stack_write',badcb)
 def falsecall(r,m):
  r.thread_load(1);r.uc.reg_write(x.UC_X86_REG_EIP,r.goal_contract['entry']);r.code(r.uc,r.goal_contract['entry'],1,None);assert not r.goal and 'NATURAL_CALL'in r.stop_error;return r.stop_error
 case('forced_entry_PC_does_not_pass_natural_call_gate',falsecall)

 def resident(r,m):
  before=r.bridge.host["dd_native_blob"]; assert r.bridge.invoke("2.24",[m["destination_object"]])==0 and r.bridge.last_residency==[1,1,5,4,20,0,0] and r.m.budget.writes==0 and before==r.bridge.host["dd_native_blob"]; return rejects(lambda:r.bridge.invoke("2.24",[0]),"INVALID_COM32_SURFACE")
 case("IsLost_uses_actual_allocated_resident_buffer_GPU_diagnostic_zero",resident)
 def native_token(r,m):
  c=r.callback_contract; return rejects(lambda:r.callbacks.enter(1,0x100000001,c["return_token"],DATA+0x800,c["args"],c),"CALLBACK_UINT32_DOMAIN")
 case("64bit_native_pointer_cannot_be_used_as_guest_callback",native_token)
 # Old pending scheduler frame is preserved and validated; this is a data-only event test.
 sys.path.insert(0,str(ROOT/'samples'));from stage54_watch import Watch
 host=HostStore(V/'prepared-host/host.manifest.json');w=Watch('cycle',copy.deepcopy(host['watch_state54']));initial=w.snapshot();w.returns(1,0x5371c4,0x700ffd90,8);assert w.state['entries']==3 and len(w.state['advances'])==3 and w.state['phase']==8 and w.state['tail_entries']==0;cases.append({'name':'source_pending_scheduler_frame_and_counters_preserved','passed':True,'details':{'initial_entries':initial['entries'],'initial_advances':len(initial['advances']),'after_entries':w.state['entries'],'after_advances':len(w.state['advances']),'original_execution':0,'data_event_only':True}})
 put(V/'evidence/component-tests-v2.json',{'status':'ALL_JOINT_OWN_CASES_PASS','cases':cases,'case_count':len(cases),'original_instructions':0,'new_original_claims':0,'tests_are_authored_data_and_code_only':True});print(json.dumps({'status':'ALL_JOINT_OWN_CASES_PASS','case_count':len(cases),'original_execution':0}))
if __name__=='__main__':main()
