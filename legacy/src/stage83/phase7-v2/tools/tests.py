from core import *
from integrate import *
def main():
 h=host();saved=json.loads(json.dumps(h['keyboard_snapshot61']));k=Keyboard(saved);assert k.snapshot()==saved;prefix=len(k.device.log);values=[k.query(v,1,p)for v,p in ((113,0x477fb9),(116,0x477fce),(117,0x477fe3))];assert values==[0,0,0]and len(k.device.log)==prefix+3 and k.device.log[:prefix]==saved['device']['payload']['log'];s=k.snapshot();assert Keyboard(s).snapshot()==s
 refused=[]
 for label,fn in [('physical',lambda:k.original_physical_query(113)),('mouse_query',lambda:k.query(1,1,0)),('reset_domain',lambda:Keyboard({'device':dict(saved['device'],sha256='0'*64),'meta':saved['meta']}))]:
  try:fn()
  except Exception as e:refused.append({'case':label,'failure':str(e)})
  else:raise AssertionError(label)
 c=Clocks();now=c.deadline_now();py=time.monotonic();frequency=int.from_bytes(c.clock(5,8,123)[2],'little');assert frequency==10000000 and c.clock(7,0,123)[1]==123;after=c.deadline_now();assert after>=now
 seed=json.loads(json.dumps(h['watch_state54']));o=Observer(seed);before=o.w.snapshot();o.code(1,0x5371c4,0x700ffd90,8,0,1,0);after_watch=o.goal(1,8);assert before['entries']==3 and len(before['advances'])==2 and after_watch['entries']==3 and len(after_watch['advances'])==3 and after_watch['tail_returns']==0
 for label,fn in [('wrong_phase_return',lambda:Observer(seed).code(1,0x5371c4,0x700ffd90,9,0,1,0)),('wrong_SP_return',lambda:Observer(seed).code(1,0x5371c4,0x700ffd94,8,0,1,0)),('tail_without_wrap',lambda:Observer(seed).code(1,0x4ac650,0x700ffd8c,0,0,1,0))]:
  try:fn()
  except Exception as e:refused.append({'case':label,'failure':str(e)})
  else:raise AssertionError(label)
 u=fixed();from fixture import make,DATA
 r,m=make(u)
 try:
  result=r.thread_run(3,100);assert result['status']=='natural_authored_call'and r.scheduled==[4,1,2,4,1]and r.m.budget.steps==68 and r.m.budget.writes==124;assert r.readword(DATA+0x30)==10000000 and r.readword(DATA+0x38)==0 and r.readword(DATA+0x108)==0xcc;watch=r.observer.goal(1,8);assert watch['entries']==3 and len(watch['advances'])==3
  actual={'scheduled':r.scheduled,'steps':r.m.budget.steps,'writes':r.m.budget.writes,'api_log':r.api_log,'watch':watch,'input_snapshot_after':r.keyboard.snapshot(),'natural_entry':r.goal_contract['entry']};put(V/'evidence/own-integration.json',actual)
 finally:r.events.close()
 put(V/'evidence/closure-tests.json',{'status':'CLOCK_WATCH_CONTROLLED_INPUT_OWN_END_TO_END_PASS','source_keyboard_restore_exact':True,'source_keyboard_domain':k.device.domain,'source_device_no_new_attach':True,'initial_log_records':prefix,'queried_values_derived_from_saved_bits':values,'hash_chain_retained':True,'negative_cases':refused,'native_global_monotonic_seconds':now,'Python_private_monotonic_seconds':py,'saved_thread2_deadline':h['guest_threads20'][2]['wait']['deadline'],'wait2_is_currently_expired':now>=h['guest_threads20'][2]['wait']['deadline'],'clock_QPC_frequency':frequency,'actual_own_capture':actual,'source_input_not_modified_or_cropped':True,'original_instructions':0});print(json.dumps({'status':'CLOCK_WATCH_KEYBOARD_E2E_PASS','own_steps':68,'own_writes':124,'native_clock':now,'Python_clock':py,'saved_wait2_expired':now>=h['guest_threads20'][2]['wait']['deadline']}))
if __name__=='__main__':main()
