from checkpoint_startup44 import StreamRuntime,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode,Boundary,ctypes,subprocess,JPEGRuntime
from stage45_pump import NativePump
class MenuRuntime(StreamRuntime):
 extra_snapshot_fields=StreamRuntime.extra_snapshot_fields+['input_route_blob45','test_input_calls45','menu_events45','miles_file_allocations45','miles_file_calls45']
 def __init__(self,save):
  super().__init__(save);self.pump.dispose();self.pump=NativePump(self.window);self.test_input_calls45=[];self.menu_events45=[];self.miles_file_allocations45={};self.miles_file_calls45=[]
  self.uc.hook_add(u.UC_HOOK_CODE,self.wnd45,begin=0x4766c0,end=0x4766c0)
 @property
 def input_route_blob45(self):return self.pump.route_snapshot()
 @input_route_blob45.setter
 def input_route_blob45(self,b):self.pump.route_restore(b)
 def wnd45(self,uc,pc,size,user):
  sp=uc.reg_read(x.UC_X86_REG_ESP);a=list(struct.unpack('<4I',bytes(uc.mem_read(sp+4,16))));self.menu_events45.append(dict(kind='original_WndProc_entry',pc=hex(pc),args=a,tid=self.current_tid,return_PC=hex(self.readword(sp))))
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage45-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def inject_click(self,point):
  before=self.window.diagnostic()
  for msg in [0x200,0x201,0x202]:
   status=self.pump.inject(msg,point);assert status==0;self.test_input_calls45.append(dict(message=msg,client_point=point,native_status=status,transport='actual NSEvent in owned window FIFO; serialized pending inputs',global_event=False))
  after=self.window.diagnostic();assert after['key_window']==after['NSApp_isActive']==after['activation_requests']==0 and after['frontmost_pid']==before['frontmost_pid']!=after['current_pid']
 def api(self,dll,name):
  if dll.lower()=='mss32.dll'and name=='_AIL_file_read@8':
   a=self.args(2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));path=self.cstring(a[0]);review=json.loads((R/'evidence/stage45-button-wav-review.json').read_text())
   if pc!=0x7626d6 or a[1]!=0 or path.lower()!=b'audio\\wavs\\button.wav':raise Boundary('Miles_file_read_outside_reviewed_button45',args=a,path=path.decode(errors='replace'),return_PC=hex(pc))
   body=Path(review['copy_path']).read_bytes();assert len(body)==review['bytes']and hashlib.sha256(body).hexdigest()==review['sha256']and Path(review['original_path']).read_bytes()==body
   from stage17_file import NativeSavedFile
   fd=NativeSavedFile(review['copy_path'])
   try:actual=fd.read(0,len(body));assert actual==body;identity=list(fd.info())
   finally:fd.dispose()
   ptr=self.allocate(self.global_heap,8,len(body)+4);assert ptr;self.uc.mem_write(ptr,actual);assert bytes(self.uc.mem_read(ptr,len(body)))==body
   item=dict(name=name,args=a,return_PC=hex(pc),guest_pointer=ptr,allocation_bytes=len(body)+4,file_bytes=len(body),sha256=review['sha256'],native_read_identity=identity,backend='actual readonly native pread; owned guest32 allocation; raw RIFF at base for destination0',Windows_Miles_GlobalAlloc_lock_registry_equivalence=False,ownership='owned guest allocation serialized; release remains unbound',original_MSS_code_executed=False)
   self.miles_file_allocations45[ptr]=item;self.miles_file_calls45.append(item);self.ret(ptr,2);return
  if dll.lower()=='mss32.dll'and name=='_AIL_set_sample_file@12':
   a=self.args(3);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));allocation=self.miles_file_allocations45.get(a[1]);assert allocation and bytes(self.uc.mem_read(a[1],4))==b'RIFF'
   raise Boundary('Nonempty_button_PCM_sample_attachment_unimplemented45',name=name,args=a,guest_return_pc=hex(pc),file_sha256=allocation['sha256'],reason='real 3001-frame PCM present; sample binding, playback, EOS and ownership contracts require real offline implementation; no empty-sample or silent-success substitution')
  if dll.lower()=='user32.dll'and name=='DefWindowProcA':
   a=self.args(4);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if pc==0x476cfa and a[1]in [0x200,0x201,0x202]:
    out=ctypes.c_uint32();assert self.pump.fn['default'](self.pump.state,*a,ctypes.byref(out));self.test_input_calls45.append(dict(name=name,args=a,return_PC=hex(pc),return_value=out.value,scope='own actual mouse MSG32 validated; client default MOVE/UP no action, DOWN no Alt/F10 latches in mouse-only input domain',source='readonly Wine7 defwnd.c switch and Microsoft client mouse message docs; fixed Windows reference not executed'));self.ret(out.value,4);return
  if dll.lower()=='user32.dll'and name=='GetCursorPos':
   point=self.pump.pointer()
   if point is not None:
    a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
    if pc!=0x52b010:raise Boundary('Window_local_test_pointer_caller_unreviewed45',return_pc=hex(pc))
    self.require_region(a[0],8,u.UC_PROT_WRITE,'window-local routed POINT32');self.uc.mem_write(a[0],struct.pack('<2i',*point));self.test_input_calls45.append(dict(name=name,args=a,return_PC=hex(pc),screen_point=point,backend='actual own NSEvent position in isolated guest input domain; global physical cursor unchanged',Windows_global_cursor_equivalence=False));self.ret(1,1);return
  return super().api(dll,name)

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='stable44');p.add_argument('--tag',default='menu45');p.add_argument('--seconds',type=int,default=14);p.add_argument('--click',nargs=2,type=int);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=MenuRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
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
  out['miles_file_calls']=r.miles_file_calls45;out['miles_file_allocations']=r.miles_file_allocations45
  if a.capture:
   (R/f'evidence/stage45-{a.tag}.json').write_text(json.dumps(out,indent=2))
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
   info=(ctypes.c_uint32*10)();v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);assert r.dd.fn['capture_prepare'](r.dd.state,info);diag=r.window.diagnostic();assert info[1]==info[5]==1 and info[6]==diag['current_pid']and diag['key_window']==diag['NSApp_isActive']==diag['activation_requests']==0 and diag['frontmost_pid']!=diag['current_pid'];png=R/f'evidence/stage45-{a.tag}-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(info[0]),str(png)],capture_output=True,timeout=10);assert q.returncode==0;out['window_capture']=dict(path=str(png),sha256=hashlib.sha256(png.read_bytes()).hexdigest(),native_capture=list(info),native_diagnostic=diag,restored_framebuffer_unchanged=True,host_restore_presentation_not_original_Blt=True)
  dest=R/f'evidence/stage45-{a.tag if not a.restore_only else "restore"}.json';dest.write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=result,route=out['route'],natural=out['natural_state'],elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
