#!/usr/bin/env python3
"""Finite original window lifecycle and actual original WM_PAINT continuation."""
import argparse,json,time,hashlib,sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from checkpoint_startup27 import LifecycleRuntime,WindowRuntime,x,u,R
from stage28_window import NativeWindow
from stage7_runtime import Boundary
from stage12_state import save_state,restore_state,decode
class StartupRuntime(LifecycleRuntime):
 extra_snapshot_fields=LifecycleRuntime.extra_snapshot_fields+['platform_callback','paint_calls','activation_decisions']
 def __init__(self,save):
  WindowRuntime.__init__(self,save);self.window.dispose();self.window=NativeWindow();self.window_sequence=None;self.window_index=0;self.window_callback_results=[];self.input_calls=[];self.native_window_dc=0;self.paint_records={};self.platform_callback=None;self.paint_calls=[];self.activation_decisions=[]
 def migrate_old_creation_paint(self,source):
  # Stage27 did not yet persist invalid regions. Restoring its visible creation
  # exposes the full native client area; explicitly retain that invalid region.
  if source in ['activation27','visible27']and self.window_pending and not self.paint_calls:
   h=self.window_pending['handle'];assert self.window.fn['paint_invalidate'](self.window.state,h,1);self.activation_decisions.append(dict(kind='old_creation_snapshot_full_native_exposure',source=source,dirty=True,erase_pending=True,original_code_unchanged=True))
 def dispatch_next(self):
  if self.window_sequence[self.window_index]['msg']==0x1c:
   h=self.window_pending['handle'];status=self.window.action('activate',h);observation=self.window.diagnostic();self.activation_decisions.append(dict(kind='native_activation',result=status,diagnostic=observation))
   if not status:
    # A valid visible window can be created without foreground activation.
    # Do not synthesize positive activation/focus messages for that case.
    self.window_sequence=[packet for packet in self.window_sequence if packet['msg']not in [0x1c,0x86,6,7]];self.activation_decisions.append(dict(kind='creation_continues_without_activation',removed_messages=[0x1c,0x86,6,7],native_state=observation))
  super().dispatch_next()
 def callback_return(self):
  q=self.platform_callback
  if not q:
   q=self.window_pending
   if not q or self.current_tid!=q['tid']or self.uc.reg_read(x.UC_X86_REG_ESP)!=q['create_struct']:raise Boundary('window_callback_owner_or_stack_mismatch')
   if self.window_sequence is not None and q.get('current_packet')!=self.window_sequence[self.window_index]:self.dispatch_next();return
   if self.window_sequence is None:self.creation_sequence()
   packet=self.window_sequence[self.window_index];value=self.uc.reg_read(x.UC_X86_REG_EAX);self.window_callback_results.append(dict(message=packet['msg'],return_value=value,tid=self.current_tid,original_WndProc=hex(q['proc'])))
   if(packet['msg']==0x81 and value==0)or(packet['msg']==1 and value==0xffffffff):raise Boundary('original_WndProc_rejected_window_creation',message=packet['msg'],return_value=value)
   self.window_index+=1
   if self.window_index<len(self.window_sequence):self.dispatch_next();return
   h=q['handle'];observation=self.window.query(h)
   if not observation[9]:raise Boundary('native_visible_window_creation_not_complete',native_observation=observation)
   self.uc.reg_write(x.UC_X86_REG_ESP,q['original_sp']);self.window_record('CreateWindowExA_completed',q['args'],return_value=h,actual_native_window=observation,original_callback_count=len(self.window_callback_results),activation_required_for_valid_creation=False);self.ret(h,12);self.window_pending=None;return
  if self.window_pending or self.current_tid!=q['tid']or self.uc.reg_read(x.UC_X86_REG_ESP)!=q['original_sp']:raise Boundary('platform_paint_callback_stack_or_owner_mismatch',pending=q)
  value=self.uc.reg_read(x.UC_X86_REG_EAX)
  if self.paint_records or self.window.fn['paint_dirty'](self.window.state):raise Boundary('original_WM_PAINT_did_not_complete_validation',pending=q,paint_records=self.paint_records)
  self.window_callback_results.append(dict(message=15,return_value=value,tid=self.current_tid,original_WndProc=hex(q['proc']),parent_API='UpdateWindow'));self.window_record('UpdateWindow_completed',q['args'],return_value=1,original_callback_return=value,actual_native_window=self.window.query(q['h']));self.platform_callback=None;self.ret(1,1)
 def api(self,dll,name):
  if dll.lower()=='user32.dll'and name=='SetFocus':
   a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if a!=[0x58000001]or pc!=0x52b893:raise Boundary('SetFocus_consumer_outside_reviewed_original_startup',args=a,return_pc=hex(pc))
   status,value=self.window.focus(self.current_tid,a[0]);self.window_record(name,a,return_value=value if status else 0,native_focus_succeeded=bool(status),actual_native_window=self.window.query(a[0]),original_ignores_return_at_52b893=True,last_error=self.last_error,last_error_equivalence_not_verified=True);self.ret(value if status else 0,1);return
  if dll.lower()=='user32.dll'and name=='UpdateWindow':
   a=self.args(1);h=a[0];state=self.window.query(h)
   if self.window_pending or self.platform_callback:raise Boundary('recursive_window_update_outside_reviewed_scope')
   dirty=self.window.fn['paint_dirty'](self.window.state)
   if not dirty:self.window_record(name,a,return_value=1,invalid_region_empty=True);self.ret(1,1);return
   entry=next(iter(self.window_classes.values()))['proc'];sp=self.uc.reg_read(x.UC_X86_REG_ESP);self.platform_callback=dict(tid=self.current_tid,h=h,args=a,proc=entry,original_sp=sp,original_return_pc=hex(self.readword(sp)),phase='WM_PAINT');self.window_record(name,a,phase='pending actual original WM_PAINT',invalid_region_empty=False,native_observation=state);self.require_region(sp-20,20,u.UC_PROT_WRITE,'nested original callback stack');self.uc.mem_write(sp-20,struct.pack('<5I',self.window_return,h,15,0,0));self.uc.reg_write(x.UC_X86_REG_ESP,sp-20);self.uc.reg_write(x.UC_X86_REG_EIP,entry);return
  if dll.lower()=='user32.dll'and name in ['BeginPaint','EndPaint']:
   a=self.args(2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));q=self.platform_callback
   if not q or a[0]!=q['h']or pc!={'BeginPaint':0x47692d,'EndPaint':0x476939}[name]:raise Boundary('paint_consumer_outside_reviewed_original_callback',name=name,args=a,return_pc=hex(pc))
   self.require_region(a[1],64,u.UC_PROT_READ|u.UC_PROT_WRITE,'32-bit PAINTSTRUCT')
   if name=='BeginPaint':
    if self.paint_records:raise Boundary('nested_BeginPaint_unsupported')
    value,body=self.window.paint_begin(a[0])
    if not value:raise Boundary('native_BeginPaint_unavailable',diagnostic=self.window.diagnostic())
    baseline=json.loads((R/'evidence/stage28-window-paint-api-Wine.json').read_text());expected=bytes.fromhex(next(p for p in baseline['messages']if p['message']==15)['paintstruct_hex']);assert body[4:]==expected[4:],'all PAINTSTRUCT non-handle fields must match own Wine probe';self.uc.mem_write(a[1],body);self.paint_records[a[1]]=dict(h=a[0],body=body)
   else:
    record=self.paint_records.get(a[1]);body=bytes(self.uc.mem_read(a[1],64))
    if not record or record['h']!=a[0]or body!=record['body']:raise Boundary('EndPaint_without_matching_native_BeginPaint')
    value=self.window.fn['paint_end'](self.window.state,a[0],struct.unpack_from('<I',body)[0])
    if not value:raise Boundary('native_EndPaint_failed')
    del self.paint_records[a[1]]
   self.paint_calls.append(dict(name=name,args=a,return_pc=hex(pc),return_value=value,PAINTSTRUCT_hex=body.hex(),backend='actual native BGRA CoreGraphics context, saved/clip/restored native state, persisted invalid region'));self.window_record(name,a,return_value=value,PAINTSTRUCT_hex=body.hex());self.ret(value,2);return
  super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='activation27');p.add_argument('--tag',default='paint28');p.add_argument('--restore-only',action='store_true');p.add_argument('--background-test',action='store_true');a=p.parse_args();start=time.monotonic();r=StartupRuntime('ST9BASE');r.start_threads(worker_capacity=3)
 if a.background_test:assert r.window.fn['background_test'](r.window.state,1)
 m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20'];payloads={}
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
   out=dict(source=a.resume,all_memory_CPU_and_four_threads_restored=True,thread_payloads=payloads,window_snapshot_sha256=hashlib.sha256(r.window_blob).hexdigest(),window_lifecycle_snapshot_sha256=hashlib.sha256(r.window_lifecycle_blob).hexdigest(),native_COM_snapshot_sha256=hashlib.sha256(r.com_blob).hexdigest(),native_Miles_snapshot_sha256=hashlib.sha256(r.miles_blob).hexdigest(),native_window_observation=r.window.query(0x58000001),native_diagnostic=r.window.diagnostic(),window_pending=r.window_pending,platform_callback=r.platform_callback,completed_original_callback_count=len(r.window_callback_results),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)));(R/'evidence/stage28-restore.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));return
  r.migrate_old_creation_paint(a.resume);r.configure_window_callback();s=r.thread_run(25,max_steps=200000);save_state(r,a.tag,'original WndProc creation and paired original WM_PAINT with real native lifecycle',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,input_calls=r.input_calls,window_callback_results=r.window_callback_results,window_calls=r.window_calls,paint_calls=r.paint_calls,window_pending=r.window_pending,platform_callback=r.platform_callback,activation_decisions=r.activation_decisions,native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,native_platform='authored arm64 AppKit/CoreGraphics',original_startup='Unicorn i386 reference, not native WinMain',original_code_patched=False,original_MSS_code_executed=False,full_resource_initializer_returned=False,city_root_returned=False);(R/f'evidence/stage28-{a.tag}.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
 finally:r.dispose()
if __name__=='__main__':main()
