#!/usr/bin/env python3
"""Ordered original startup with native guest COM apartment state."""
import argparse,json,time,hashlib,sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from checkpoint_startup25 import MilesRuntime,x,u
from stage7_runtime import Boundary
from stage12_state import save_state,restore_state,decode
from stage26_com import NativeCOM
from stage26_window import NativeWindow
from stage26_resources import PEResources
R=Path(__file__).resolve().parents[1]
class WindowRuntime(MilesRuntime):
 extra_snapshot_fields=MilesRuntime.extra_snapshot_fields+['com_blob','com_calls','window_blob','window_calls','window_pending','window_classes']
 def __init__(self,save):
  super().__init__(save);self.com=NativeCOM();self.com_calls=[];self.window=NativeWindow();self.resources=PEResources(self.pe);self.window_calls=[];self.window_pending=None;self.window_classes={}
 @property
 def com_blob(self):return self.com.export()
 @com_blob.setter
 def com_blob(self,b):self.com.restore(b)
 @property
 def window_blob(self):return self.window.export()
 @window_blob.setter
 def window_blob(self,b):self.window.restore(b)
 def configure_window_callback(self):
  self.window_return=0x61003000;assert self.imports.get(self.window_return)in(None,('authored.window','callback_return'));self.imports[self.window_return]=('authored.window','callback_return');self.uc.mem_write(self.window_return,b'\x0f\x0b')
 def window_record(self,name,args,**fields):
  item=dict(name=name,args=args,tid=self.current_tid,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),**fields);self.window_calls.append(item);self.api_trace.append(item);return item
 def api(self,dll,name):
  if dll=='authored.window':raise Boundary('window_creation_callback_continuation_pending',pending=self.window_pending)
  if dll.lower()=='ole32.dll'and name in ['CoInitialize','CoInitializeEx','CoUninitialize']:
   argc={'CoInitialize':1,'CoInitializeEx':2,'CoUninitialize':0}[name];a=self.args(argc);tid=self.current_tid;pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));before=self.com.query(tid)
   if a and a[0]:raise Boundary('COM_reserved_pointer_unsupported',args=a)
   if name=='CoInitializeEx'and a[1]not in (0,2):raise Boundary('COM_extra_initialization_options_unsupported',flags=a[1])
   if name=='CoUninitialize':
    if before['mode']==2 and any(t!=tid and self.com.query(t)['mode']==2 and self.com.query(t)['count']for t in self.threads):raise Boundary('multiple_STA_teardown_order_unsupported',tid=tid)
    if not self.com.uninitialize(tid):raise Boundary('COM_unbalanced_uninitialize',tid=tid)
    value=self.uc.reg_read(x.UC_X86_REG_EAX)
   else:value=self.com.initialize(tid,2 if name=='CoInitialize'else a[1])
   item=dict(name=name,args=a,tid=tid,return_pc=hex(pc),return_value=value if argc else None,before=before,after=self.com.query(tid),last_error=self.last_error,backend='native arm64 guest apartment state, per guest thread + pthread mutex; not host Windows COM')
   self.com_calls.append(item);self.api_trace.append(item);self.ret(value,argc);return
  if dll.lower()=='user32.dll'and name=='FindWindowA':
   a=self.args(2)
   if a[0]and a[0]<=65535:raise Boundary('FindWindow_class_atom_unimplemented')
   cls=self.cstring(a[0])if a[0]else None;title=self.cstring(a[1])if a[1]else None;v=self.window.find(cls,title);self.window_record(name,a,return_value=v,class_name=cls.decode()if cls else None,title=title.decode()if title else None,namespace='actual native window objects owned by this isolated guest process; external Wine windows are not guest HWNDs');self.ret(v,2);return
  if dll.lower()=='user32.dll'and name in ['LoadIconA','LoadCursorA']and self.args(2)[0]==0x400000:
   a=self.args(2)
   if name=='LoadIconA':body,e=self.resources.icon(a[1]);h=self.window.resource(1,0,0,body)
   else:
    px,py,mask,bits,e=self.resources.cursor(a[1])
    if e['invert_screen_pixel_count']:raise Boundary('native_cursor_invert_screen_pixels_unsupported',resource=e)
    body=mask+bits;h=self.window.resource(3,px,py,body)
   if not h:raise Boundary('native_EXE_image_or_cursor_decode_failed',resource=e)
   self.window_record(name,a,return_value=h,source=e,native_object='NSImage'if name=='LoadIconA'else'NSCursor',source_sha256=hashlib.sha256(body).hexdigest());self.ret(h,2);return
  if dll.lower()=='gdi32.dll'and name=='GetStockObject':
   a=self.args(1)
   if a!=[4]:raise Boundary('stock_GDI_object_unsupported',args=a)
   h=self.window.resource(2,4,0,b'');assert h;self.window_record(name,a,return_value=h,native_object='retained NSColor.blackColor, BLACK_BRUSH role');self.ret(h,1);return
  if dll.lower()=='user32.dll'and name=='RegisterClassA':
   a=self.args(1);self.require_region(a[0],40,u.UC_PROT_READ,'WNDCLASSA');words=struct.unpack('<10I',bytes(self.uc.mem_read(a[0],40)));cls=self.cstring(words[9]);section=self.pe.section(words[1])
   if not section or not section['flags']&0x20000000:raise Boundary('window_procedure_not_original_code')
   atom,error=self.window.register(words[:9],cls,self.last_error)
   if not atom and error==self.last_error:raise Boundary('native_class_attributes_unsupported',words=words,class_name=cls.decode())
   if atom:self.window_classes[cls]=dict(atom=atom,words=words,proc=words[1])
   self.last_error=error;self.window_record(name,list(words),return_value=atom,class_name=cls.decode(),last_error=error,original_WndProc=hex(words[1]));self.ret(atom,1);return
  if dll.lower()=='user32.dll'and name=='CreateWindowExA':
   a=self.args(12);cls=self.cstring(a[1]);title=self.cstring(a[2]);entry=self.window_classes[cls]['proc']
   if self.window_pending:raise Boundary('recursive_window_creation_outside_scope')
   h=self.window.begin(self.current_tid,cls,title,a)
   if not h:raise Boundary('native_window_creation_attributes_unsupported',args=a)
   sp=self.uc.reg_read(x.UC_X86_REG_ESP);create=sp-64;payload=struct.pack('<12I',a[11],a[10],a[9],a[8],a[7],a[6],a[5],a[4],a[3],a[2],a[1],a[0]);self.require_region(create,64,u.UC_PROT_READ|u.UC_PROT_WRITE,'stack CREATESTRUCT');self.uc.mem_write(create,payload)
   self.window_record(name,a,phase='pending real original WM_NCCREATE callback; CreateWindowEx has not returned',native_NSWindow_allocated=True,visible=False,handle=h,original_WndProc=hex(entry),CREATESTRUCT=create)
   self.window_pending=dict(tid=self.current_tid,handle=h,args=a,original_sp=sp,create_struct=create,proc=entry,phase='WM_NCCREATE',messages_dispatched=[0x81],callback_return_stub=self.window_return)
   cbsp=create-20;self.uc.mem_write(cbsp,struct.pack('<5I',self.window_return,h,0x81,0,create));self.uc.reg_write(x.UC_X86_REG_ESP,cbsp);self.uc.reg_write(x.UC_X86_REG_EIP,entry);return
  try:super().api(dll,name)
  except Boundary as e:
   e.details['guest_return_pc']=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)))
   e.details['stack_args_up_to_12']=self.args(12)
   raise
 def dispose(self):self.window.dispose();self.com.dispose();super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='miles25');p.add_argument('--tag',default='window26');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=WindowRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 if a.restore_only:
  expected=decode(m['host'])['guest_threads20'];payloads={}
  for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
  out=dict(source=a.resume,all_memory_CPU_and_four_threads_restored=True,thread_payloads=payloads,native_COM_snapshot_sha256=hashlib.sha256(r.com_blob).hexdigest(),com_threads={str(tid):r.com.query(tid)for tid in r.threads},native_Miles_snapshot_sha256=hashlib.sha256(r.miles_blob).hexdigest(),native_Window_snapshot_sha256=hashlib.sha256(r.window_blob).hexdigest(),window_pending=r.window_pending,main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
  (R/'evidence/stage26-restore.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));r.dispose();return
 r.configure_window_callback();s=r.thread_run(25,max_steps=200000);save_state(r,a.tag,'ordered original startup with native COM apartment state and window resources',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,com_calls=r.com_calls,window_calls=r.window_calls,window_pending=r.window_pending,boot_events=r.boot_events,native_platform='authored arm64 adapters',original_startup='Unicorn original i386 reference, not native WinMain',original_code_patched=False,original_MSS_code_executed=False,full_resource_initializer_returned=False,city_root_returned=False)
 (R/f'evidence/stage26-{a.tag}.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));r.dispose()
if __name__=='__main__':main()
