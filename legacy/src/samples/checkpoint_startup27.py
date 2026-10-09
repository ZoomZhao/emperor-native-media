#!/usr/bin/env python3
"""Continue the real original WndProc and native finite window lifecycle."""
import argparse,json,time,hashlib,sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from checkpoint_startup26 import WindowRuntime,x,u,R
from stage27_window import NativeWindow
from stage7_runtime import Boundary
from stage12_state import save_state,restore_state,decode
class LifecycleRuntime(WindowRuntime):
 extra_snapshot_fields=WindowRuntime.extra_snapshot_fields+['window_lifecycle_blob','window_sequence','window_index','window_callback_results','input_calls','native_window_dc','paint_records']
 def __init__(self,save):
  super().__init__(save);self.window.dispose();self.window=NativeWindow();self.window_sequence=None;self.window_index=0;self.window_callback_results=[];self.input_calls=[];self.native_window_dc=0;self.paint_records={}
 @property
 def window_lifecycle_blob(self):return self.window.life_export()
 @window_lifecycle_blob.setter
 def window_lifecycle_blob(self,body):self.window.life_restore(body)
 def creation_sequence(self):
  q=self.window_pending;h=q['handle'];a=q['args'];heap=next(iter(self.heaps));base=self.allocate(heap,8,128)
  if not base:raise Boundary('window_platform_callback_buffer_allocation_failed')
  self.uc.mem_write(base,struct.pack('<4i',*self.window.rect(h,0)));self.uc.mem_write(base+32,struct.pack('<7I',h,0,0,0,0,0,0x43));self.uc.mem_write(base+64,struct.pack('<7I',h,0,0,0,a[6],a[7],0x1847))
  self.native_window_dc=self.window.dc(h)
  if not self.native_window_dc:raise Boundary('native_window_BGRA_DC_allocation_failed')
  self.window_sequence=[dict(msg=0x81,wp=0,lp=q['create_struct']),dict(msg=0x83,wp=0,lp=base),dict(msg=1,wp=0,lp=q['create_struct']),dict(msg=5,wp=0,lp=(a[7]<<16)|a[6]),dict(msg=3,wp=0,lp=(a[5]<<16)|a[4]),dict(msg=0x18,wp=1,lp=0),dict(msg=0x46,wp=0,lp=base+32),dict(msg=0x30f,wp=0,lp=0),dict(msg=0x1c,wp=1,lp=0),dict(msg=0x86,wp=1,lp=0),dict(msg=6,wp=1,lp=0),dict(msg=7,wp=0,lp=0),dict(msg=0x85,wp=1,lp=0),dict(msg=0x14,wp=self.native_window_dc,lp=0),dict(msg=0x47,wp=0,lp=base+64)]
  baseline=json.loads((R/'evidence/stage27-window-defaults-Wine.json').read_text());assert [p['msg']for p in self.window_sequence]==[p['message']for p in baseline['creation_messages']]
 def dispatch_next(self):
  q=self.window_pending;packet=self.window_sequence[self.window_index];h=q['handle'];msg=packet['msg']
  if msg==5 and not self.window.action('created',h):raise Boundary('native_window_complete_object_failed')
  if msg==0x18 and not self.window.action('show',h):raise Boundary('actual_native_window_show_failed')
  if msg==0x1c and not self.window.action('activate',h):raise Boundary('actual_native_window_activation_failed',native_observation=self.window.query(h),native_foreground=self.window.fn['window_foreground'](self.window.state))
  if msg==7:
   status,old=self.window.focus(self.current_tid,h)
   if not status:raise Boundary('actual_native_window_focus_failed')
   packet['wp']=old
  q['phase']=hex(msg);q['messages_dispatched'].append(msg);q['current_packet']=packet;sp=q['create_struct']-20;self.uc.mem_write(sp,struct.pack('<5I',self.window_return,h,msg,packet['wp'],packet['lp']));self.uc.reg_write(x.UC_X86_REG_ESP,sp);self.uc.reg_write(x.UC_X86_REG_EIP,q['proc'])
 def callback_return(self):
  q=self.window_pending
  if not q or self.current_tid!=q['tid']or self.uc.reg_read(x.UC_X86_REG_ESP)!=q['create_struct']:raise Boundary('window_callback_owner_or_stack_mismatch')
  if self.window_sequence is not None and q.get('current_packet')!=self.window_sequence[self.window_index]:
   # A native pre-dispatch action may fail after the preceding callback was
   # already recorded. Retry that action without inventing another return.
   self.dispatch_next();return
  if self.window_sequence is None:self.creation_sequence()
  packet=self.window_sequence[self.window_index];value=self.uc.reg_read(x.UC_X86_REG_EAX);self.window_callback_results.append(dict(message=packet['msg'],return_value=value,tid=self.current_tid,original_WndProc=hex(q['proc'])))
  if(packet['msg']==0x81 and value==0)or(packet['msg']==1 and value==0xffffffff):raise Boundary('original_WndProc_rejected_window_creation',message=packet['msg'],return_value=value)
  self.window_index+=1
  if self.window_index<len(self.window_sequence):self.dispatch_next();return
  h=q['handle'];observation=self.window.query(h);assert observation[9]and observation[10]and observation[7:9]==[h,h];self.uc.reg_write(x.UC_X86_REG_ESP,q['original_sp']);self.window_record('CreateWindowExA_completed',q['args'],return_value=h,actual_native_window=observation,original_callback_count=len(self.window_callback_results));self.ret(h,12);self.window_pending=None
 def api(self,dll,name):
  if dll=='authored.window'and name=='callback_return':self.callback_return();return
  if dll.lower()=='user32.dll'and name=='GetAsyncKeyState':
   a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if a!=[18]or pc!=0x43cfaa or bytes(self.uc.mem_read(0x43cfad,6))!=self.pe.read(0x43cfad,6):raise Boundary('keyboard_consumer_not_reviewed',args=a,return_pc=hex(pc))
   status,value,flags=self.window.alt();item=dict(name=name,args=a,return_pc=hex(pc),return_value=value,raw_NSEvent_flags=flags[0],raw_CG_session_flags=flags[1],left_Option=flags[2],right_Option=flags[3],backend='live native NSEvent/CGEventSource combined-session Option to reviewed VK_MENU high SHORT bit',low_byte_unused_by_original=True,system_permissions_changed=False);self.input_calls.append(item);self.api_trace.append(item)
   if not status:raise Boundary('native_modifier_sources_disagree_or_change',**item)
   self.ret(value,1);return
  if dll.lower()=='user32.dll'and name=='DefWindowProcA':
   a=self.args(4);h,msg,wp,lp=a;size=48 if msg in [0x81,1]else 16 if msg==0x83 and not wp else 28 if msg in [0x46,0x47]else 0;body=bytes(self.uc.mem_read(lp,size))if size else b'';status,value,after=self.window.default(h,msg,wp,body)
   if not status:raise Boundary('native_default_window_message_not_supported',args=a)
   if size:self.uc.mem_write(lp,after)
   self.window_record(name,a,return_value=value,before_hex=body.hex(),after_hex=after.hex(),backend='native real window + checked creation/default notification semantics');self.ret(value,4);return
  if dll.lower()=='user32.dll'and name=='GetForegroundWindow':
   value=self.window.fn['window_foreground'](self.window.state);self.window_record(name,[],return_value=value,backend='actual AppKit application active and window visible/key, projected into owned guest HWND namespace');self.ret(value,0);return
  if dll.lower()=='user32.dll'and name in ['GetActiveWindow','GetFocus','IsIconic','GetClientRect','GetWindowRect','ClientToScreen','ScreenToClient','SetFocus','UpdateWindow']:
   argc={'GetActiveWindow':0,'GetFocus':0,'IsIconic':1,'GetClientRect':2,'GetWindowRect':2,'ClientToScreen':2,'ScreenToClient':2,'SetFocus':1,'UpdateWindow':1}[name];a=self.args(argc);h=a[0]if a else 0x58000001;state=self.window.query(h)
   if name in ['GetActiveWindow','GetFocus']:value=state[7 if name=='GetActiveWindow'else 8]
   elif name=='IsIconic':value=state[11]
   elif name in ['GetClientRect','GetWindowRect']:
    self.require_region(a[1],16,u.UC_PROT_WRITE,'native window rectangle');self.uc.mem_write(a[1],struct.pack('<4i',*self.window.rect(h,name=='GetClientRect')));value=1
   elif name in ['ClientToScreen','ScreenToClient']:
    self.require_region(a[1],8,u.UC_PROT_READ|u.UC_PROT_WRITE,'POINT');px,py=struct.unpack('<2i',bytes(self.uc.mem_read(a[1],8)));sign=1 if name=='ClientToScreen'else-1;self.uc.mem_write(a[1],struct.pack('<2i',px+sign*state[1],py+sign*state[2]));value=1
   elif name=='SetFocus':
    if state[8]!=h:raise Boundary('new_focus_callback_required',args=a,state=state)
    status,value=self.window.focus(self.current_tid,h)
    if not status:raise Boundary('native_window_focus_request_failed',args=a)
   else:
    raise Boundary('window_update_needs_reviewed_original_paint_callback',args=a)
   self.window_record(name,a,return_value=value,native_observation=state);self.ret(value,argc);return
  super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='window26');p.add_argument('--tag',default='lifecycle27');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=LifecycleRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 if a.restore_only:
  expected=decode(m['host'])['guest_threads20'];payloads={}
  for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
  out=dict(source=a.resume,all_memory_CPU_and_four_threads_restored=True,thread_payloads=payloads,window_snapshot_sha256=hashlib.sha256(r.window_blob).hexdigest(),window_lifecycle_snapshot_sha256=hashlib.sha256(r.window_lifecycle_blob).hexdigest(),native_COM_snapshot_sha256=hashlib.sha256(r.com_blob).hexdigest(),native_Miles_snapshot_sha256=hashlib.sha256(r.miles_blob).hexdigest(),native_window_observation=r.window.query(0x58000001),native_foreground=r.window.fn['window_foreground'](r.window.state),window_pending=r.window_pending,window_index=r.window_index,completed_original_callback_count=len(r.window_callback_results),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
  (R/'evidence/stage27-restore.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));r.dispose();return
 r.configure_window_callback();s=r.thread_run(30,max_steps=200000);save_state(r,a.tag,'ordered original WndProc, native live modifier input and finite native window lifecycle',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,input_calls=r.input_calls,window_callback_results=r.window_callback_results,window_calls=r.window_calls,window_pending=r.window_pending,boot_events=r.boot_events,native_platform='authored arm64 AppKit/CoreGraphics adapters',original_startup='Unicorn original i386 reference, not native WinMain',original_code_patched=False,original_MSS_code_executed=False,full_resource_initializer_returned=False,city_root_returned=False)
 (R/f'evidence/stage27-{a.tag}.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));r.dispose()
if __name__=='__main__':main()
