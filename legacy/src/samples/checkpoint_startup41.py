# -*- coding: utf-8 -*-
"""Observe bounded original text scans without altering instructions or loops."""
from checkpoint_startup40 import ProviderRuntime,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode
from stage7_runtime import Boundary
class ProgressRuntime(ProviderRuntime):
 extra_snapshot_fields=ProviderRuntime.extra_snapshot_fields+['scan_entries41','scan_returns41','main_segments41','stream_calls41']
 def __init__(self,save):
  super().__init__(save);self.scan_entries41=[];self.scan_returns41=[];self.main_segments41=[];self.stream_calls41=[]
  for pc,label in [(0x5278b0,'entry'),(0x5278f0,'return')]:self.uc.hook_add(u.UC_HOOK_CODE,self.scan_hook,user_data=label,begin=pc,end=pc)
  for pc,label in [(0x4757e1,'post_audio_52b510_return'),(0x47583f,'startup_535bc0_return'),(0x475a85,'startup_return_path')]:self.uc.hook_add(u.UC_HOOK_CODE,self.provider_hook,user_data=label,begin=pc,end=pc)
 def scan_state(self):
  pc=self.uc.reg_read(x.UC_X86_REG_EIP);a=self.uc.reg_read(x.UC_X86_REG_EAX);d=self.uc.reg_read(x.UC_X86_REG_EDX)
  out=dict(pc=hex(pc),EAX=hex(a),EDX=d,current_tid=self.current_tid)
  if 0x5278b0<=pc<=0x5278f0:
   sp=self.uc.reg_read(x.UC_X86_REG_ESP);ret,gid,index=struct.unpack('<3I',bytes(self.uc.mem_read(sp,12)));base=self.readword(17684684)
   if 0<=gid<1000:
    offset,count=struct.unpack('<2I',bytes(self.uc.mem_read(17676440+gid*8,8)));out.update(return_pc=hex(ret),group_id=gid,requested_index=index,group_count=count,group_start=hex(base+offset),pointer_offset_in_group=a-base-offset,within_text_allocation=any(p<=a<p+size for p,(_,size)in self.allocations.items()if p==base))
  return out
 def scan_hook(self,uc,address,size,label):
  out=self.scan_state();out['label']=label;target=self.scan_entries41 if label=='entry'else self.scan_returns41;target.append(out)
 def segment(self,*args,**kwargs):
  before=self.scan_state()if getattr(self,'current_tid',None)==1 else None;out=super().segment(*args,**kwargs)
  if before is not None:self.main_segments41.append(dict(index=len(self.main_segments41),before=before,after=self.scan_state(),status=out['status']))
  return out
 def api(self,dll,name):
  if dll.lower()=='mss32.dll'and name=='_AIL_close_stream@4':
   a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if a[0]!=0:raise Boundary('Nonempty_stream_close_requires_owned_stream_contract41',args=a)
   value=self.uc.reg_read(x.UC_X86_REG_EAX)
   item=dict(name=name,args=a,return_pc=hex(pc),guest_return_value=value,scope='NULL stream only; no owned stream exists',original_core_NULL_test='21121e54 loads arg; 21121e57 test; 21121e59 branches to 21121fdb epilogue; stdcall ret4',EAX_contract='void/undefined preserved; wrapper scratch EAX equivalence not claimed',original_MSS_export_executed=False)
   self.stream_calls41.append(item);self.api_trace.append(item);self.ret(value,1);return
  if dll.lower()=='mss32.dll'and name=='_AIL_open_stream@12':
   a=self.args(3);path=self.cstring(a[1],260);key=path.lower();row=self.resource_rows.get(key)
   item=dict(name=name,args=a,path=path.decode('ascii','replace'),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),buffer_request_bytes=a[2],actual_catalogued_resource=row,driver_is_current_native_audio=a[0]==self.audio.driver)
   self.stream_calls41.append(item)
   raise Boundary('Stream_decode_lifecycle_loop_volume_EOS_contract_unimplemented41',**item)
  return super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='recordvol40');p.add_argument('--tag',default='progress41');p.add_argument('--seconds',type=int,default=12);p.add_argument('--restore-only',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=ProgressRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry();before=r.scan_state()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,main_state=r.scan_state(),native_diagnostic=r.window.diagnostic())
  else:
   assert r.registry.start()==0 and r.audio.start()==0;r.configure_window_callback();r.configure_dd();result=r.thread_run(a.seconds,max_steps=200000);r.verify_native_geometry();save_state(r,a.tag,'original startup continued with observational per-segment text scan progress and original return hooks; no loop patched or skipped',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=result,before=before,after=r.scan_state(),elapsed_seconds=time.monotonic()-start,scan_entries=r.scan_entries41,scan_returns=r.scan_returns41,main_segments=r.main_segments41,stream_calls=r.stream_calls41,provider_events=r.provider_events40,missing_model_calls=r.missing_model_calls40,native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),original_code_patched=False,original_MSS_or_M3D_exports_executed=False,main_menu_passed=False,city_logic_passed=False,full_native_WinMain_proven=False,Miles3D_equivalence_proven=False)
  (R/f'evidence/stage41-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,result=out.get('result'),before=before,after=r.scan_state(),scan_returns=len(r.scan_returns41),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
