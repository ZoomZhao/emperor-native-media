# -*- coding: utf-8 -*-
"""Original startup continuation using actual native capability and file failure."""
from checkpoint_startup41 import ProgressRuntime,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode,Boundary
from stage42_bink import NativeBink
from stage42_pump import NativePump
import os,re,errno
ROOT42=R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]'
SOURCE42=Path('<LOCAL_MACHINE_PATH>]')
class BinkRuntime(ProgressRuntime):
 extra_snapshot_fields=ProgressRuntime.extra_snapshot_fields+['bink_blob42','bink_calls42','bink_events42','bink_error_guest42','pump_blob42','pump_calls42','message_callback42']
 def __init__(self,save):
  super().__init__(save);self.bink=NativeBink(self.audio);self.bink_calls42=[];self.bink_events42=[];self.bink_error_guest42=0;self.pump=NativePump(self.window);self.pump_calls42=[];self.message_callback42=None
  for pc,label in [(0x41ffb3,'original_movie_open_failed_return'),(0x41ffbd,'original_movie_open_success_branch'),(0x4758af,'original_startup_movie_call_return'),(0x475986,'original_startup_475210_return'),(0x475a12,'original_startup_event_loop_after_tick')]:self.uc.hook_add(u.UC_HOOK_CODE,self.bink_hook,user_data=label,begin=pc,end=pc)
 @property
 def bink_blob42(self):return self.bink.snapshot()
 @bink_blob42.setter
 def bink_blob42(self,b):self.bink.restore(b)
 @property
 def pump_blob42(self):return self.pump.snapshot()
 @pump_blob42.setter
 def pump_blob42(self,b):self.pump.restore(b)
 def callback_return(self):
  q=self.message_callback42
  if not q:return super().callback_return()
  if self.current_tid!=q['tid']or self.uc.reg_read(x.UC_X86_REG_ESP)!=q['owner_sp']:raise Boundary('Message_callback_stack_or_owner_mismatch42')
  value=self.uc.reg_read(x.UC_X86_REG_EAX);self.pump_calls42.append(dict(name='DispatchMessageA_original_WndProc_return',message=q['msg'],return_value=value));self.message_callback42=None;self.ret(value,1)
 def bink_hook(self,uc,address,size,label):
  if len(self.bink_events42)<200:self.bink_events42.append(dict(pc=hex(address),label=label,EAX=hex(uc.reg_read(x.UC_X86_REG_EAX)),AL=uc.reg_read(x.UC_X86_REG_EAX)&255))
 def movie_relative42(self,path):
  # Z is admitted only after checking the fixed DOS namespace root below.
  m=re.fullmatch(rb'(?:(?:C:\\Emperor|Z:)\\)?Binks\\(emperor_intro(?:_hiq)?\.bik)',path,re.IGNORECASE)
  if not m:raise Boundary('Bink_guest_movie_path_namespace_unreviewed42',path=path.decode('ascii','replace'))
  return 'Binks/'+m[1].decode().lower()
 def missing_movie42(self,path):
  rel=self.movie_relative42(path);observed=[];roots=[ROOT42,SOURCE42];mapping=[]
  if path.lower().startswith(b'z:\\'):
   for game in [ROOT42,SOURCE42]:
    link=game.parent/'dosdevices/z:';target=str(link.readlink());assert target=='/';mapping.append(dict(link=str(link),target=target))
   assert self.readword(17686738)&255==ord('Z');roots=[Path('/')]
  for root in roots:
   p=root/rel;assert root.is_dir()
   if p.parent.is_dir()and any(q.name.casefold()==p.name.casefold()for q in p.parent.iterdir()):raise Boundary('Bink_movie_now_present_requires_decoder42',path=str(p))
   try:fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
   except FileNotFoundError as e:assert e.errno==errno.ENOENT;observed.append(dict(path=str(p),native_root=str(root),errno=e.errno,read_only=True,guest_namespace_mapping=mapping))
   else:os.close(fd);raise Boundary('Bink_movie_opened_requires_decoder42',path=str(p))
  return rel,observed
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name=='Sleep':
   a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if a!=[1]or pc!=0x475a35:raise Boundary('Sleep_parameters_unreviewed42',args=a,return_pc=hex(pc))
   started=time.monotonic();time.sleep(a[0]/1000.0);elapsed=time.monotonic()-started;assert elapsed>=0.001
   self.pump_calls42.append(dict(name=name,args=a,return_pc=hex(pc),actual_host_wait_seconds=elapsed,scope='finite own scheduler wait; guest Windows timing equivalence not proven'))
   self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),1);self.thread_api_yield=True;self.uc.emu_stop();return
  if dll.lower()=='user32.dll'and name in ['PeekMessageA','GetMessageA']:
   argc=5 if name=='PeekMessageA'else 4;a=self.args(argc);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));remove=a[4]if argc==5 else 1
   if a[1:4]!=[0,0,0]or remove not in [0,1]or pc not in [0x4759ad,0x4759c4]:raise Boundary('Message_query_parameters_unreviewed42',name=name,args=a,return_pc=hex(pc))
   self.require_region(a[0],28,u.UC_PROT_WRITE,'MSG32');status,msg,diag=self.pump.peek(remove)
   if status<0:raise Boundary('Actual_AppKit_event_or_activation_contract_unimplemented42',status=status,actual_native_event_type=diag[0],actual_native_event_subtype=diag[1])
   if name=='GetMessageA'and status==0:raise Boundary('Blocking_GetMessage_without_pending_owned_event_unimplemented42')
   if status:self.uc.mem_write(a[0],struct.pack('<7I',*msg))
   if len(self.pump_calls42)<500:self.pump_calls42.append(dict(name=name,args=a,return_pc=hex(pc),return_value=status,native_queue_polled=True,MSG32=msg if status else None,native_event_diagnostic=diag,output_untouched_when_empty=status==0,NOACTIVATE=True))
   self.ret(status,argc);return
  if dll.lower()=='user32.dll'and name in ['TranslateMessage','DispatchMessageA']:
   a=self.args(1);self.require_region(a[0],28,u.UC_PROT_READ,'native-owned MSG32');msg=list(struct.unpack('<7I',bytes(self.uc.mem_read(a[0],28))))
   if msg[0]!=0x58000001 or msg[1]not in [0x200,0x201,0x202,0x204,0x205]:raise Boundary('Message_translate_dispatch_type_unreviewed42',MSG32=msg)
   if name=='TranslateMessage':self.ret(0,1);return
   if self.message_callback42 or self.platform_callback or self.position_callback or self.style_callback:raise Boundary('Message_dispatch_nested_callback_unreviewed42')
   sp=self.uc.reg_read(x.UC_X86_REG_ESP);self.message_callback42=dict(tid=self.current_tid,owner_sp=sp,msg=msg);self.uc.mem_write(sp-20,struct.pack('<5I',self.window_return,*msg[:4]));self.uc.reg_write(x.UC_X86_REG_ESP,sp-20);self.uc.reg_write(x.UC_X86_REG_EIP,next(iter(self.window_classes.values()))['proc']);return
  if dll.lower()=='user32.dll'and name=='SendMessageA':
   a=self.args(4);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));registered={q['native_session_ID']:q['text']for q in self.message_calls if 'native_session_ID'in q}
   if a[0]!=0 or a[2:]!=[0,0]or pc not in [0x4752a6,0x4752c6]or registered.get(a[1])not in ['MSH_WHEELSUPPORT_MSG','MSH_SCROLL_LINES_MSG']:raise Boundary('SendMessage_target_or_message_unreviewed42',args=a,return_pc=hex(pc))
   kind=self.window.fn['kind'](self.window.state,a[0]);assert kind==0
   self.last_error=1400;self.window_record(name,a,return_value=0,last_error=1400,actual_native_window_registry_kind=kind,receiver_present=False,registered_message=registered[a[1]],scope='absent legacy MouseZ/Magellan wheel-driver window only',source='Wine 7.0 message.c send_message + win.c GetWindowThreadProcessId invalid-handle path; independent source, fixed Windows dynamic result not measured');self.ret(0,4);return
  if dll.lower()=='user32.dll'and name=='SetWindowPos'and self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))==0x47597c:
   a=self.args(7);sp=self.uc.reg_read(x.UC_X86_REG_ESP)
   if a[0]!=0x58000001 or a[1]!=0 or a[4:7]!=[1024,796,0x14]or self.position_callback or self.platform_callback or self.style_callback:raise Boundary('Post_movie_position_parameters_unreviewed42',args=a)
   # Reuse existing native geometry and unchanged original WndProc pipeline.
   b=self.allocate(next(iter(self.heaps)),8,160);assert b;self.uc.mem_write(b,struct.pack('<7I',*a));self.position_callback=dict(tid=self.current_tid,h=a[0],args=a,owner_sp=sp,parent_API='SetWindowPos',proc=next(iter(self.window_classes.values()))['proc'],buffer=b,old_geometry=self.verify_native_geometry(),messages=[],nccalc=False);self.window_record(name,a,phase='pending original post-movie geometry callbacks',NOACTIVATE=True,NOZORDER=True,official_flags_source='https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos');self.position_dispatch(dict(msg=0x46,wp=0,lp=b));return
  if dll.lower()=='kernel32.dll'and name=='CreateFileA':
   a=self.args(7);path=self.cstring(a[0],260)
   if b'\\binks\\'in path.lower()or path.lower().startswith(b'binks\\'):
    if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('Bink_CRT_missing_readonly_parameters_unreviewed42',args=a,path=path.decode())
    if a[3]and(self.readword(a[3])!=12 or self.readword(a[3]+4)!=0):raise Boundary('Bink_CRT_missing_security_descriptor_unreviewed42')
    rel,observed=self.missing_movie42(path);item=dict(name=name,path=path.decode(),args=a,actual_POSIX_readonly_failures=observed,Win32_return=0xffffffff,Win32_last_error=2,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))));self.bink_calls42.append(item);self.api_trace.append(item);self.last_error=2;self.ret(0xffffffff,7);return
  if dll.lower()!='binkw32.dll':return super().api(dll,name)
  argc={'_BinkSetSoundSystem@8':2,'_BinkOpen@8':2,'_BinkGetSummary@8':2,'_BinkGetError@0':0}.get(name)
  if argc is None:raise Boundary('Bink_next_nonfailure_contract_unimplemented42',name=name)
  a=self.args(argc);item=dict(name=name,args=a,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),original_DLL_executed=False);value=self.uc.reg_read(x.UC_X86_REG_EAX)
  if name=='_BinkSetSoundSystem@8':
   if self.imports.get(a[0])!=('binkw32.dll','_BinkOpenMiles@4')or a[1]!=self.audio.driver:raise Boundary('Bink_sound_factory_or_driver_unreviewed42',args=a)
   status,value=self.bink.set_sound(*a);caps=self.bink.capabilities()
   if status:raise Boundary('Bink_actual_native_factory_failure_unreviewed42',status=status,args=a)
   assert value==0 and caps['real_audio_live']and caps['real_sample_allocate_entry']and not caps['real_minimum_buffer_size_entry']and not caps['sound_open_registered']
   item.update(return_value=value,native_capabilities=caps,reason='real callback factory queried reviewed own sample module; mandatory minimum-buffer-size entry is absent',Windows_original_DLL_registry_equivalence=False,manufactured_sound_open_table=False)
  elif name=='_BinkOpen@8':
   path=self.cstring(a[0],260);rel,observed=self.missing_movie42(path);status,value=self.bink.open_missing(Path(observed[0]['native_root']),rel,a[1])
   if status:raise Boundary('Bink_native_open_contract_unreviewed42',status=status,args=a,path=path.decode())
   assert value==0 and self.bink.error()==b'Error opening file.';self.last_error=2
   item.update(path=path.decode(),return_value=value,actual_POSIX_readonly_failures=observed,native_capabilities=self.bink.capabilities(),error=self.bink.error().decode(),scope='actual missing file only; existing files refuse for unimplemented decoder')
  elif name=='_BinkGetSummary@8':
   if a[0]:raise Boundary('Bink_nonnull_summary_requires_real_movie42',args=a)
   self.require_region(a[1],124,u.UC_PROT_READ|u.UC_PROT_WRITE,'original BinkSummary output');before=bytes(self.uc.mem_read(a[1],124));item.update(scope='NULL movie early return; original void API leaves output untouched',output_sha256_before=hashlib.sha256(before).hexdigest(),output_sha256_after=hashlib.sha256(bytes(self.uc.mem_read(a[1],124))).hexdigest(),EAX_contract='void/undefined preserved')
  else:
   if not self.bink_error_guest42:
    self.bink_error_guest42=self.allocate(next(iter(self.heaps)),0,256)
    if not self.bink_error_guest42:raise Boundary('Bink_error_guest_buffer_allocation_failed42')
   body=self.bink.error()+b'\0';self.uc.mem_write(self.bink_error_guest42,body);value=self.bink_error_guest42;item.update(return_value=hex(value),error=body[:-1].decode())
  self.bink_calls42.append(item);self.api_trace.append(item);self.ret(value,argc)
 def dispose(self):
  self.pump.dispose()
  if self.bink.state:self.bink.dispose()
  super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='stream41');p.add_argument('--tag',default='bink42');p.add_argument('--seconds',type=int,default=18);p.add_argument('--restore-only',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=BinkRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry();before=hex(r.uc.reg_read(x.UC_X86_REG_EIP))
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,main_PC=before,bink_native_sha256=hashlib.sha256(r.bink_blob42).hexdigest(),bink_capabilities=r.bink.capabilities(),native_diagnostic=r.window.diagnostic())
  else:
   assert r.registry.start()==0 and r.audio.start()==0;r.configure_window_callback();r.configure_dd();result=r.thread_run(a.seconds,max_steps=200000);r.verify_native_geometry();save_state(r,a.tag,'original startup continued using actual native factory lookup failure and actual missing movie readonly failure; no branch patched or movie substituted',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=result,before_PC=before,after_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),elapsed_seconds=time.monotonic()-start,bink_calls=r.bink_calls42,bink_events=r.bink_events42,bink_native_capabilities=r.bink.capabilities(),stream_calls=r.stream_calls41,position_calls=r.position_calls,position_callback=r.position_callback,position_frames=r.position_frames,window_calls=r.window_calls,window_callback_results=r.window_callback_results,pump_calls=r.pump_calls42,pump_snapshot_sha256=hashlib.sha256(r.pump_blob42).hexdigest()if r.pump_blob42 else None,native_geometry=r.verify_native_geometry(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),original_code_patched=False,original_loop_or_movie_skipped=False,original_MSS_Bink_M3D_exports_executed=False,main_menu_passed=False,city_logic_passed=False,full_native_WinMain_proven=False,Miles3D_equivalence_proven=False,Bink_decode_or_sound_factory_equivalence_proven=False)
  (R/f'evidence/stage42-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,result=out.get('result'),events=r.bink_events42,elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
