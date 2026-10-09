# -*- coding: utf-8 -*-
from checkpoint_startup42 import BinkRuntime,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode,Boundary
from stage43_input import NativeInput
from stage43_cursor import NativeCursorSelection
from checkpoint_startup32 import JPEGRuntime
from stage43_pump import NativePump
from stage43_window import NativeWindow
import ctypes,subprocess
class InputRuntime(BinkRuntime):
 extra_snapshot_fields=BinkRuntime.extra_snapshot_fields+['input_calls43','startup_events43','startup_counts43','cursor_selection_blob43']
 def __init__(self,save):
  bootstrap=NativeWindow();super().__init__(save);self.window.dispose();self.window=NativeWindow();bootstrap.dispose();self.pump.dispose();self.pump=NativePump(self.window);self.native_input43=NativeInput(self.window);self.native_cursor43=NativeCursorSelection(self.window);self.input_calls43=[];self.startup_events43=[];self.startup_counts43={}
  for pc,label in [(0x475994,'startup_loop_iteration'),(0x475a12,'startup_queued_task_dispatch_return'),(0x4fefc0,'startup_deferred_notification_check_entry'),(0x475a1e,'startup_deferred_notification_check_return'),(0x475b00,'original_UI_refresh_outer_entry'),(0x536b20,'original_UI_refresh_root_entry'),(0x534a30,'city_load_root_entry'),(0x4ac2b0,'original_city_scheduler_entry'),(0x4ac650,'original_city_cycle_tail_entry')]:self.uc.hook_add(u.UC_HOOK_CODE,self.observe43,user_data=label,begin=pc,end=pc)
 def observe43(self,uc,pc,size,label):
  self.startup_counts43[label]=self.startup_counts43.get(label,0)+1
  if len(self.startup_events43)<200:self.startup_events43.append(dict(pc=hex(pc),label=label,tid=self.current_tid,EAX=hex(uc.reg_read(x.UC_X86_REG_EAX)),gate=self.readword(0x88ec38),phase=self.readword(0xc82ef8)))
 @property
 def cursor_selection_blob43(self):return self.native_cursor43.snapshot()
 @cursor_selection_blob43.setter
 def cursor_selection_blob43(self,b):self.native_cursor43.restore(b)
 def dispose(self):self.native_cursor43.dispose();super().dispose()
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage43-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def api(self,dll,name):
  if dll.lower()=='user32.dll'and name in ['PeekMessageA','GetMessageA']:
   argc=5 if name=='PeekMessageA'else 4;a=self.args(argc);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));remove=a[4]if argc==5 else 1
   if a[1]!=0 or a[2:4]not in [[0,0],[0x205,0x205],[0x200,0x209],[0x100,0x108]]or remove not in [0,1]:raise Boundary('Message_filter_or_parameters_unreviewed43',name=name,args=a,return_pc=hex(pc))
   self.require_region(a[0],28,u.UC_PROT_WRITE,'native MSG32');dirty=self.window.fn['paint_dirty'](self.window.state)
   if dirty and(a[2:4]==[0,0]or a[2]<=15<=a[3]):raise Boundary('Actual_invalid_region_requires_WM_PAINT_queue43',paint_dirty=dirty)
   status,msg,diag=self.pump.peek(remove,*a[2:4])
   if status<0:raise Boundary('Actual_AppKit_event_filter_or_activation_unimplemented43',status=status,diagnostic=diag,filters=a[2:4])
   if name=='GetMessageA'and not status:raise Boundary('Blocking_GetMessage_empty_requires_real_wait43',return_pc=hex(pc))
   if status:self.uc.mem_write(a[0],struct.pack('<7I',*msg))
   if len(self.pump_calls42)<1000:self.pump_calls42.append(dict(name=name,args=a,return_pc=hex(pc),return_value=status,native_queue_polled=True,MSG32=msg if status else None,native_event_diagnostic=diag,native_paint_dirty=dirty,output_untouched_when_empty=status==0,NOACTIVATE=True))
   self.ret(status,argc);return
  if dll.lower()=='user32.dll'and name=='SetCursor':
   a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if pc not in [0x52b27a,0x52b28c,0x52b29e,0x52b2b0]or self.window.fn['kind'](self.window.state,a[0])!=3:raise Boundary('Owned_cursor_selection_unreviewed43',args=a,return_pc=hex(pc))
   status,value=self.native_cursor43.select(a[0])
   if not status:raise Boundary('Owned_native_cursor_rectangle_binding_failed43',args=a)
   self.input_calls43.append(dict(name=name,args=a,return_pc=hex(pc),return_value=value,backend='actual NSView cursor rectangle; previous actual native cursor registered in owned guest namespace',global_cursor_set=False,global_events_posted=False,Windows_shared_cursor_stack_equivalence=False));self.ret(value,1);return
  if dll.lower()=='user32.dll'and name in ['GetCursorPos','ClientToScreen','ScreenToClient']:
   argc=1 if name=='GetCursorPos'else 2;a=self.args(argc);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));dest=a[-1]
   if (name=='GetCursorPos'and pc!=0x52b010)or(argc==2 and a[0]!=0x58000001):raise Boundary('Readonly_input_consumer_unreviewed43',name=name,args=a,return_pc=hex(pc))
   self.require_region(dest,8,u.UC_PROT_READ|u.UC_PROT_WRITE,'POINT32 readonly native input');before=list(struct.unpack('<2i',bytes(self.uc.mem_read(dest,8))));status,point,diag=self.native_input43.query(None if argc==1 else before,name=='ClientToScreen')
   if not status:raise Boundary('Native_readonly_input_scope_or_focus_changed43',diagnostic=diag)
   g=self.verify_native_geometry();assert diag['native_client_w']==g['client_w']and diag['native_client_h']==g['client_h']
   if argc==2:
    sign=1 if name=='ClientToScreen'else -1;assert point==[before[0]+sign*g['client_x'],before[1]+sign*g['client_y']]
   self.uc.mem_write(dest,struct.pack('<2i',*point));q=dict(name=name,args=a,return_pc=hex(pc),return_value=1,point_before=before,point_after=point,native_diagnostic=diag,readonly=True,global_events_posted=False,coordinate_policy='one guest pixel per AppKit logical point; actual backing scale measured, not applied twice',Windows_DPI_virtualization_equivalence=False)
   if len(self.input_calls43)<500:self.input_calls43.append(q)
   self.ret(1,argc);return
  return super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='loop42');p.add_argument('--tag',default='input43');p.add_argument('--seconds',type=int,default=16);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=InputRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  before=hex(r.uc.reg_read(x.UC_X86_REG_EIP));r.verify_native_geometry();before_diag=r.window.diagnostic();assert before_diag['key_window']==0 and before_diag['activation_requests']==0
  coordinate_cases=[]
  for point in [[0,0],[1023,767],[-20,900],[500,380]]:
   status,screen,d=r.native_input43.query(point,True);assert status;status,back,d2=r.native_input43.query(screen,False);assert status and back==point;coordinate_cases.append(dict(client=point,screen=screen,roundtrip_client=back,diagnostic=d2))
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,main_PC=before,input_calls=r.input_calls43,startup_counts=r.startup_counts43,native_diagnostic=r.window.diagnostic(),coordinate_roundtrip_cases=coordinate_cases)
  else:
   assert r.registry.start()==0 and r.audio.start()==0;r.configure_window_callback();r.configure_dd();result=r.thread_run(a.seconds,max_steps=200000);r.verify_native_geometry();after_diag=r.window.diagnostic();(R/f'evidence/stage43-{a.tag}-focus.json').write_text(json.dumps(dict(before=before_diag,after=after_diag,result=result),indent=2));assert after_diag['key_window']==0 and after_diag['activation_requests']==0 and after_diag['frontmost_pid']!=after_diag['current_pid'];save_state(r,a.tag,'original startup with readonly actual AppKit cursor and coordinate transforms; no global input generated',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=result,before_PC=before,after_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),elapsed_seconds=time.monotonic()-start,input_calls=r.input_calls43,startup_counts=r.startup_counts43,startup_events=r.startup_events43,pump_calls=r.pump_calls42,boot_events=r.boot_events,presentation_events=r.presentation_events,dd_calls=r.dd_calls,bink_calls=r.bink_calls42,native_geometry=r.verify_native_geometry(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(gate=r.readword(0x88ec38),phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,original_loop_or_movie_skipped=False,original_MSS_Bink_M3D_exports_executed=False,global_input_events_posted=False,main_menu_passed=False,city_logic_passed=False,city_tick_proven=False,full_native_WinMain_proven=False,Bink_decode_or_sound_factory_equivalence_proven=False)
  out['native_before']=before_diag;out['coordinate_roundtrip_cases']=coordinate_cases
  if a.capture:
   assert not a.restore_only and r.dd;wid=r.dd.fn['window_id'](r.dd.state);assert wid;info=(ctypes.c_uint32*10)();assert r.dd.fn['capture_prepare'](r.dd.state,info);png=R/f'evidence/stage43-{a.tag}-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(wid),str(png)],capture_output=True,timeout=10);diag=r.window.diagnostic();out['window_capture']=dict(owned_window_number=wid,owned_window_only=True,path=str(png),returncode=q.returncode,stderr=q.stderr.decode(errors='replace'),native_capture_observation=list(info),native_after=diag)
   assert q.returncode==0 and png.exists()and diag['key_window']==diag['activation_requests']==diag['NSApp_isActive']==0 and diag['frontmost_pid']!=diag['current_pid'];out['window_capture']['sha256']=hashlib.sha256(png.read_bytes()).hexdigest()
  (R/f'evidence/stage43-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,result=out.get('result'),startup_counts=r.startup_counts43,input_calls=len(r.input_calls43),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
