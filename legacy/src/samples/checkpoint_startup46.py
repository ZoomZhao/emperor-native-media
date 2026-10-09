from checkpoint_startup45 import MenuRuntime as Previous,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode,Boundary,ctypes,subprocess,JPEGRuntime
from stage46_samples import NativeSamples
class MenuRuntime(Previous):
 extra_snapshot_fields=Previous.extra_snapshot_fields+['PCM_blob46','PCM_calls46','PCM_handles46','file_releases46']
 def __init__(self,save):
  super().__init__(save);self.sample_pool.dispose();self.sample_pool=NativeSamples(self.audio);self.PCM_calls46=[];self.PCM_handles46=[];self.file_releases46=[]
 @property
 def PCM_blob46(self):return self.sample_pool.full_snapshot()
 @PCM_blob46.setter
 def PCM_blob46(self,b):self.sample_pool.full_restore(b)
 def segment(self,*a,**kw):
  assert self.sample_pool.call('service')==0;out=super().segment(*a,**kw);assert self.sample_pool.call('service')==0;return out
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage46-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def api(self,dll,name):
  if dll.lower()!='mss32.dll':return super().api(dll,name)
  pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));eax=self.uc.reg_read(x.UC_X86_REG_EAX)
  if name=='_AIL_mem_free_lock@4':
   a=self.args(1);ptr=a[0]
   if not ptr:self.ret(eax,1);return
   owner=self.miles_file_allocations45.get(ptr)
   if not owner or ptr not in self.allocations:raise Boundary('Miles_file_free_unknown_owner46',args=a,guest_return_pc=hex(pc))
   status=self.sample_pool.call('drop_file',ptr)
   if status:raise Boundary('Miles_file_free_while_live_PCM46',args=a,status=status,guest_return_pc=hex(pc))
   del self.allocations[ptr];del self.miles_file_allocations45[ptr];self.file_releases46.append(dict(pointer=ptr,bytes=owner['allocation_bytes'],return_PC=hex(pc),actual_owned_guest_allocation_released=True));self.ret(eax,1);return
  n={'_AIL_set_sample_file@12':3,'_AIL_set_sample_loop_count@8':2,'_AIL_set_sample_playback_rate@8':2,'_AIL_set_sample_volume@8':2,'_AIL_set_sample_pan@8':2,'_AIL_set_sample_reverb@16':4,'_AIL_start_sample@4':1,'_AIL_end_sample@4':1,'_AIL_stop_sample@4':1,'_AIL_resume_sample@4':1,'_AIL_sample_status@4':1,'_AIL_sample_loop_count@4':1,'_AIL_init_sample@4':1,'_AIL_release_sample_handle@4':1}.get(name)
  if n is None:return super().api(dll,name)
  a=self.args(n);status,before=self.sample_pool.stats(a[0])
  if status:raise Boundary('PCM_sample_unknown_owned_token46',name=name,args=a,guest_return_pc=hex(pc))
  if name=='_AIL_set_sample_file@12':
   allocation=self.miles_file_allocations45.get(a[1]);review=json.loads((R/'evidence/stage45-button-wav-review.json').read_text())
   if not allocation or a[2]!=0 or pc not in [0x76275e,0x7629ce]:raise Boundary('PCM_attachment_outside_reviewed_button46',args=a,guest_return_pc=hex(pc))
   body=bytes(self.uc.mem_read(a[1],allocation['file_bytes']));assert hashlib.sha256(body).hexdigest()==review['sha256'];status=self.sample_pool.bind(a[0],a[1],body);value=1
   if a[0]not in self.PCM_handles46:self.PCM_handles46.append(a[0])
  elif name in ['_AIL_sample_status@4','_AIL_sample_loop_count@4']:
   status,value=self.sample_pool.query(a[0],2 if name=='_AIL_sample_status@4'else 17)
  elif name=='_AIL_set_sample_reverb@16':
   if any(a[1:]):raise Boundary('Nonzero_sample_reverb_unimplemented46',args=a,guest_return_pc=hex(pc))
   status=self.sample_pool.call('zero_reverb',a[0]);value=eax
  elif name in ['_AIL_init_sample@4','_AIL_release_sample_handle@4','_AIL_start_sample@4']:
   status=self.sample_pool.call({'_AIL_init_sample@4':'init','_AIL_release_sample_handle@4':'release','_AIL_start_sample@4':'start_empty'}[name],a[0]);value=eax
  elif name in ['_AIL_stop_sample@4','_AIL_resume_sample@4','_AIL_end_sample@4']:
   status=self.sample_pool.call('command',a[0],{'_AIL_stop_sample@4':1,'_AIL_resume_sample@4':2,'_AIL_end_sample@4':3}[name]);value=eax
  else:
   status=self.sample_pool.call('set',a[0],{'_AIL_set_sample_loop_count@8':17,'_AIL_set_sample_playback_rate@8':22,'_AIL_set_sample_volume@8':23,'_AIL_set_sample_pan@8':24}[name],a[1]);value=eax
  if status:raise Boundary('Real_PCM_sample_contract_refused46',name=name,args=a,status=status,guest_return_pc=hex(pc))
  after_status,after=self.sample_pool.stats(a[0]);self.PCM_calls46.append(dict(name=name,args=a,return_PC=hex(pc),guest_return_value=value,before=before,after=after if not after_status else None,scope='actual owned PCM and real offline ring; frame clock from silent AudioUnit; no callbacks/providers/codecs beyond validated mono22050 S16 input',source_PCM_sent_to_device=False,original_DLL_exports_executed=False,exact_original_Miles_mixer_equivalence=False));self.ret(value,n);return

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='pcm45');p.add_argument('--tag',default='menu46');p.add_argument('--seconds',type=int,default=14);p.add_argument('--click',nargs=2,type=int);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=MenuRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=100000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  before=r.window.diagnostic();before_pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP));result=None
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
  else:
   assert r.registry.start()==r.audio.start()==0;r.configure_window_callback();r.configure_dd()
   if a.click:r.inject_click(a.click)
   result=r.thread_run(a.seconds,max_steps=200000);after=r.window.diagnostic();assert after['key_window']==after['NSApp_isActive']==after['activation_requests']==0 and after['frontmost_pid']!=after['current_pid'];save_state(r,a.tag,'original menu continuation with owned window NSEvent FIFO; no guest gameplay fields injected',m['milestones'])
  out=dict(source=a.resume,state=a.tag,result=result,current_tid=r.current_tid,current_CPU_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),before_PC=before_pc,elapsed_seconds=time.monotonic()-start,test_input_calls=r.test_input_calls45,original_WndProc_entries=r.menu_events45,route=r.pump.route_diagnostic(),input_calls=r.input_calls43,pump_calls=r.pump_calls42,startup_counts=r.startup_counts43,startup_events=r.startup_events43,boot_events=r.boot_events,stream_calls=r.stream_calls44,stream_stats=r.streams44.stats(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(screen_mode=bytes(r.uc.mem_read(17686597,1))[0],gate=r.readword(0x88ec38),city_phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,game_fields_injected=False,global_events_posted=False,global_cursor_moved=False,files_uploaded=False,original_DLL_exports_executed=False,original_city_update_proven=False,all_memory_CPU_four_threads_native_blobs_restored=bool(a.restore_only))
  out['miles_file_calls']=r.miles_file_calls45;out['miles_file_allocations']=r.miles_file_allocations45;out['PCM_calls']=r.PCM_calls46;out['PCM_stats']={h:r.sample_pool.stats(h)[1] for h in r.PCM_handles46 if r.sample_pool.stats(h)[0]==0};out['file_releases']=r.file_releases46
  if a.capture:
   (R/f'evidence/stage46-{a.tag}.json').write_text(json.dumps(out,indent=2))
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
   info=(ctypes.c_uint32*10)();v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);assert r.dd.fn['capture_prepare'](r.dd.state,info);diag=r.window.diagnostic();assert info[1]==info[5]==1 and info[6]==diag['current_pid']and diag['key_window']==diag['NSApp_isActive']==diag['activation_requests']==0 and diag['frontmost_pid']!=diag['current_pid'];png=R/f'evidence/stage46-{a.tag}-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(info[0]),str(png)],capture_output=True,timeout=10);assert q.returncode==0;out['window_capture']=dict(path=str(png),sha256=hashlib.sha256(png.read_bytes()).hexdigest(),native_capture=list(info),native_diagnostic=diag,restored_framebuffer_unchanged=True,host_restore_presentation_not_original_Blt=True)
  dest=R/f'evidence/stage46-{a.tag if not a.restore_only else "restore"}.json';dest.write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=result,route=out['route'],natural=out['natural_state'],elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
