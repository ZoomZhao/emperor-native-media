#!/usr/bin/env python3
"""Integrated original window geometry, DD and resource startup continuation."""
import argparse,ctypes,json,time,hashlib,sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from checkpoint_startup30 import DrawRuntime,x,u,R,BASE,SIZE
from stage31_window import NativeWindow
from stage31_dd import NativeDD
from stage17_file import NativeSavedFile
from stage16_paths import GAME
from stage7_runtime import Boundary
from stage12_state import save_state,restore_state,decode
class IntegratedRuntime(DrawRuntime):
 extra_snapshot_fields=DrawRuntime.extra_snapshot_fields+['position_callback','position_frames','frame_style_pending','position_calls']
 def __init__(self,save):
  super().__init__(save);self.window.dispose();self.window=NativeWindow();self.position_callback=None;self.position_frames=[];self.frame_style_pending=True;self.position_calls=[];self.resource_rows={}
  for row in json.loads((R/'evidence/stage19-resources-hashed.json').read_text())['rows']:
   canonical=b'C:\\Emperor\\'+row['relative_path'].replace('/','\\').encode();self.resource_rows[canonical.lower()]=row;self.resource_rows[row['relative_path'].replace('/','\\').encode().lower()]=row
 def bind_resource(self,path):
  row=self.resource_rows.get(path.lower())
  if not row:return
  canonical=b'C:\\Emperor\\'+row['relative_path'].replace('/','\\').encode();key=canonical.lower()
  if key in self.startup_files:return
  copy=Path(row['copy_path']);assert copy.resolve().is_relative_to(R);body=copy.read_bytes();assert len(body)==row['copy_size']and hashlib.sha256(body).hexdigest()==row['copy_sha256'];assert hashlib.sha256(Path(row['original_path']).read_bytes()).hexdigest()==row['original_sha256'];fd=NativeSavedFile(copy);info=fd.info();assert fd.read(0,len(body))==body;self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],row['copy_sha256']);self.startup_aliases[key]=canonical;self.startup_aliases[row['relative_path'].replace('/','\\').encode().lower()]=canonical
 def prepare_resource_restore(self,prior):
  for path in decode(prior['host']).get('startup_file_identity',{}):self.bind_resource(path)
 def native_for_path(self,path):self.bind_resource(path);return super().native_for_path(path)
 def ensure_dd(self):
  if self.dd is None:
   h=0x58000001;state=self.window.query(h);ptr=self.window.fn['pointer'](self.window.state,h);self.dd=NativeDD(ptr,state[3],state[4])
 def position_dispatch(self,packet,frame=None):
  q=self.position_callback;parent=frame or q;parent['packet']=packet;sp=parent['owner_sp'];self.require_region(sp-20,20,u.UC_PROT_WRITE,'nested geometry callback32 stack');self.uc.mem_write(sp-20,struct.pack('<5I',self.window_return,q['h'],packet['msg'],packet['wp'],packet['lp']));self.uc.reg_write(x.UC_X86_REG_ESP,sp-20);self.uc.reg_write(x.UC_X86_REG_EIP,q['proc']);self.position_calls.append(dict(kind='WndProc_entry',message=packet['msg'],tid=self.current_tid,parent_API=parent['parent_API'],stack_owner=hex(sp)))
 def position_finish(self):
  q=self.position_callback;self.window_record('SetWindowPos_completed',q['args'],return_value=1,actual_native_geometry=self.verify_native_geometry(),callback_messages=q['messages'],native_applied=q.get('native_applied',False),NOACTIVATE=True);self.position_callback=None;self.ret(1,7)
 def position_apply(self):
  q=self.position_callback;a=list(struct.unpack('<7I',bytes(self.uc.mem_read(q['buffer'],28))))
  if not self.window.fn['position'](self.window.state,q['h'],(ctypes.c_uint32*7)(*a)):raise Boundary('native_position_or_NOACTIVATE_failed',WINDOWPOS=a)
  g=self.verify_native_geometry();q['native_applied']=True;q['new_geometry']=g
  if q.get('nccalc'):
   rect=list(struct.unpack('<4i',bytes(self.uc.mem_read(q['buffer']+80,16))))
   if rect!=[g['client_x'],g['client_y'],g['client_x']+g['client_w'],g['client_y']+g['client_h']]:raise Boundary('original_custom_NCCALCSIZE_not_representable_in_native_window',original_rect=rect,native_geometry=g)
  old=q['old_geometry'];moved=self.frame_style_pending or [old['client_x'],old['client_y']]!=[g['client_x'],g['client_y']];sized=[old['client_w'],old['client_h']]!=[g['client_w'],g['client_h']];self.frame_style_pending=False;q['client_moved']=moved;q['client_sized']=sized
  flags=a[6]|(0 if moved else 0x800)|(0 if sized else 0x1000);self.uc.mem_write(q['buffer'],struct.pack('<7I',q['h'],a[1],g['frame_x'],g['frame_y'],g['frame_w'],g['frame_h'],flags))
  if not(moved or sized):self.position_finish();return
  packets=[]
  if sized:packets.extend([dict(msg=0x85,wp=1,lp=0),dict(msg=0x14,wp=self.window.dc(q['h']),lp=0)])
  packets.append(dict(msg=0x47,wp=0,lp=q['buffer']));q['after_packets']=packets;q['after_index']=0;self.position_dispatch(packets[0])
 def callback_return(self):
  if self.platform_callback:return super().callback_return()
  q=self.position_callback
  if not q:return super().callback_return()
  parent=self.position_frames[-1]if self.position_frames else q
  if self.current_tid!=q['tid']or self.uc.reg_read(x.UC_X86_REG_ESP)!=parent['owner_sp']:raise Boundary('position_callback_stack_or_thread_mismatch',pending=q,nested=self.position_frames)
  packet=parent['packet'];msg=packet['msg'];value=self.uc.reg_read(x.UC_X86_REG_EAX);self.window_callback_results.append(dict(message=msg,return_value=value,tid=self.current_tid,original_WndProc=hex(q['proc']),parent_API=parent['parent_API'],nested=bool(self.position_frames)));q['messages'].append(msg)
  if self.position_frames:
   parent['index']+=1
   if parent['index']<len(parent['packets']):self.position_dispatch(parent['packets'][parent['index']],parent);return
   self.position_frames.pop();self.window_record('DefWindowProcA_nested_geometry_completed',parent['args'],return_value=0,children=[p['msg']for p in parent['packets']]);self.ret(0,4);return
  if msg==0x46:
   a=list(struct.unpack('<7I',bytes(self.uc.mem_read(q['buffer'],28))));old=q['old_geometry']
   if a[0]!=q['h']or not a[6]&0x10 or a[6]&~0x17:raise Boundary('original_WINDOWPOS_modification_outside_reviewed_scope',WINDOWPOS=a)
   if a[6]&2:a[2:4]=[old['frame_x'],old['frame_y']]
   if a[6]&1:a[4:6]=[old['frame_w'],old['frame_h']]
   self.uc.mem_write(q['buffer'],struct.pack('<7I',*a));q['requested']=a
   if a[4:6]!=[old['frame_w'],old['frame_h']]or self.frame_style_pending:
    limits=(ctypes.c_int32*10)();assert self.window.fn['minmax'](self.window.state,q['h'],limits);self.uc.mem_write(q['buffer']+32,bytes(limits));self.position_dispatch(dict(msg=0x24,wp=0,lp=q['buffer']+32));return
   self.position_apply();return
  if msg==0x24:
   a=q['requested'];limits=struct.unpack('<10i',bytes(self.uc.mem_read(q['buffer']+32,40)))
   if not(limits[6]<=a[4]<=limits[8]and limits[7]<=a[5]<=limits[9]):raise Boundary('requested_size_outside_original_MINMAXINFO',size=a[4:6],MINMAXINFO=limits)
   old=q['old_geometry'];rect=[a[2],a[3],a[2]+a[4],a[3]+a[5]];prior=[old['frame_x'],old['frame_y'],old['frame_x']+old['frame_w'],old['frame_y']+old['frame_h']];client=[old['client_x'],old['client_y'],old['client_x']+old['client_w'],old['client_y']+old['client_h']];self.uc.mem_write(q['buffer']+80,struct.pack('<13I',*[v&0xffffffff for v in rect+prior+client+[q['buffer']]]));q['nccalc']=True;self.position_dispatch(dict(msg=0x83,wp=1,lp=q['buffer']+80));return
  if msg==0x83:
   if value:raise Boundary('original_NCCALCSIZE_preservation_flags_not_implemented',value=value)
   self.position_apply();return
  q['after_index']+=1
  if q['after_index']<len(q['after_packets']):self.position_dispatch(q['after_packets'][q['after_index']]);return
  self.position_finish()
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name in ['CreateFileA','GetFullPathNameA','GetFileAttributesA']:self.bind_resource(self.cstring(self.args(1)[0]))
  if dll.lower()=='kernel32.dll'and name=='CreateDirectoryA':
   a=self.args(2);path=self.cstring(a[0]);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));child=path.rstrip(b'\\')
   if child not in [b'Save',b'Campaigns']or a[1]or pc not in [0x475bb5,0x475c01]:raise Boundary('directory_outside_reviewed_existing_baseline_scope',path=path.decode(),args=a,return_pc=hex(pc))
   error=ctypes.c_uint32()
   if not self.window.fn['existing_directory'](str(GAME).encode(),child,ctypes.byref(error)):raise Boundary('original_baseline_directory_missing_or_not_readable',path=path.decode())
   self.last_error=error.value;self.window_record(name,a,guest_path=path.decode(),return_value=0,last_error=self.last_error,backend='readonly openat confirms actual baseline directory already exists; ERROR_ALREADY_EXISTS, no mkdir');self.ret(0,2);return
  if dll=='authored.dd'and name=='2.5':
   a=self.args(6);o=self.lookup(a[0],2);di=self.dd.info(o['id']);flags=a[4]
   if flags&0x400:
    if a[2]or a[3]or flags not in [0x400,0x1000400]or not a[5]:raise Boundary('unsupported_DDBLT_COLORFILL_contract',args=a)
    self.require_region(a[5],100,u.UC_PROT_READ,'DDBLTFX32 color fill');fx=bytes(self.uc.mem_read(a[5],100))
    if struct.unpack_from('<I',fx)[0]!=100:raise Boundary('DDBLTFX32_wrong_size')
    color=struct.unpack_from('<I',fx,80)[0];rect=list(struct.unpack('<4i',bytes(self.uc.mem_read(a[1],16))))if a[1]else[0,0,di['w'],di['h']];out=(ctypes.c_int32*5)();self.sync_to_native(o);value=0 if self.dd.fn['fill'](self.dd.state,o['id'],color,(ctypes.c_int32*4)(*rect),out)else 0x80004005
    if not value:self.sync_from_native(o)
    self.record_dd('2.5.colorfill',a,value,fill_color=color,requested_rect=rect,native_clipped_rect=list(out)[:4],pixels_filled=out[4],pixel_sha256=hashlib.sha256(self.dd.pixels(o['id'])).hexdigest(),synchronous_WAIT=bool(flags&0x1000000),DDBLTFX_size=100);self.ret(value,6);return
   if flags not in [0,0x1000000]or a[5]:raise Boundary('DD_Blt_copy_effects_or_flags_not_supported',args=a)
   src=self.lookup(a[2],2);si=self.dd.info(src['id']);rect=list(struct.unpack('<4i',bytes(self.uc.mem_read(a[3],16))))if a[3]else[0,0,si['w'],si['h']];dr=list(struct.unpack('<4i',bytes(self.uc.mem_read(a[1],16))))if a[1]else[0,0,di['w'],di['h']]
   if[dr[2]-dr[0],dr[3]-dr[1]]!=[rect[2]-rect[0],rect[3]-rect[1]]:raise Boundary('DD_stretched_copy_Blt_not_supported',args=a)
   self.sync_to_native(src);self.sync_to_native(o);value=0 if self.dd.fn['blt'](self.dd.state,o['id'],dr[0],dr[1],src['id'],(ctypes.c_int32*4)(*rect))else 0x80004005
   if not value:self.sync_from_native(o)
   self.record_dd('2.5.copy',a,value,source_rect=rect,destination_rect=dr,synchronous_WAIT=bool(flags&0x1000000));self.ret(value,6);return
  if dll.lower()=='user32.dll'and name in ['BeginPaint','EndPaint']:
   a=self.args(2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));q=self.platform_callback
   if not q or a[0]!=q['h']or pc!={'BeginPaint':0x47692d,'EndPaint':0x476939}[name]:raise Boundary('paint_outside_original_UpdateWindow_callback',name=name,args=a,return_pc=hex(pc))
   self.require_region(a[1],64,u.UC_PROT_READ|u.UC_PROT_WRITE,'PAINTSTRUCT32')
   if name=='BeginPaint':
    if self.paint_records:raise Boundary('recursive_native_BeginPaint')
    dirty=self.window.fn['paint_dirty'](self.window.state);g=self.verify_native_geometry();value,body=self.window.paint_begin(a[0])
    if not value:raise Boundary('native_paint_begin_failed',geometry=g)
    expected=[0]*16;expected[0]=value
    if dirty:expected[4:6]=[g['client_w'],g['client_h']]
    if body!=struct.pack('<16I',*expected):raise Boundary('native_PAINTSTRUCT_inconsistent_with_actual_invalid_client_area',body=body.hex(),expected=expected)
    self.uc.mem_write(a[1],body);self.paint_records[a[1]]=dict(h=a[0],body=body)
   else:
    rec=self.paint_records.get(a[1]);body=bytes(self.uc.mem_read(a[1],64))
    if not rec or rec['h']!=a[0]or body!=rec['body']:raise Boundary('native_EndPaint_record_mismatch')
    value=self.window.fn['paint_end'](self.window.state,a[0],struct.unpack_from('<I',body)[0])
    if not value:raise Boundary('native_EndPaint_failed')
    del self.paint_records[a[1]]
   self.paint_calls.append(dict(name=name,args=a,return_pc=hex(pc),return_value=value,PAINTSTRUCT_hex=body.hex(),backend='real resized CoreGraphics bitmap and actual invalid client rectangle'));self.window_record(name,a,return_value=value,PAINTSTRUCT_hex=body.hex());self.ret(value,2);return
  if dll.lower()=='user32.dll'and name=='SetWindowPos':
   a=self.args(7);sp=self.uc.reg_read(x.UC_X86_REG_ESP);pc=self.readword(sp)
   if a[0]!=0x58000001 or pc not in [0x44d560,0x44d575,0x44d5ca]or a[6]not in [0x16,0x13,0x15]or self.position_callback or self.platform_callback or self.style_callback:raise Boundary('SetWindowPos_consumer_or_flags_outside_reviewed_scope',args=a,return_pc=hex(pc))
   b=self.allocate(next(iter(self.heaps)),8,160);assert b;self.uc.mem_write(b,struct.pack('<7I',*a));self.position_callback=dict(tid=self.current_tid,h=a[0],args=a,owner_sp=sp,parent_API='SetWindowPos',proc=next(iter(self.window_classes.values()))['proc'],buffer=b,old_geometry=self.verify_native_geometry(),messages=[],nccalc=False);self.window_record(name,a,phase='pending original geometry callbacks',NOACTIVATE=True);self.position_dispatch(dict(msg=0x46,wp=0,lp=b));return
  if dll.lower()=='user32.dll'and name=='DefWindowProcA'and self.args(4)[1]in [0x24,0x46,0x47,0x83]:
   a=self.args(4);h,msg,wp,lp=a;size=40 if msg==0x24 else 52 if msg==0x83 and wp else 16 if msg==0x83 else 28;self.require_region(lp,size,u.UC_PROT_READ|u.UC_PROT_WRITE,'finite Win32 geometry structure');before=bytes(self.uc.mem_read(lp,size));status,value,after=self.window.default(h,msg,wp,before)
   if not status:raise Boundary('native_finite_geometry_default_failed',args=a)
   self.uc.mem_write(lp,after);self.window_record(name,a,return_value=value,before_hex=before.hex(),after_hex=after.hex(),backend='actual native frame metrics and scoped virtual Win32 geometry defaults')
   q=self.position_callback
   if msg==0x47 and q and q['packet']['msg']==0x47:
    if self.position_frames or lp!=q['buffer']:raise Boundary('unexpected_recursive_WINDOWPOSCHANGED',args=a)
    g=q['new_geometry'];packets=[]
    if q['client_moved']:packets.append(dict(msg=3,wp=0,lp=(g['client_x']&65535)|((g['client_y']&65535)<<16)))
    if q['client_sized']:packets.append(dict(msg=5,wp=0,lp=(g['client_w']&65535)|((g['client_h']&65535)<<16)))
    if packets:
     frame=dict(owner_sp=self.uc.reg_read(x.UC_X86_REG_ESP),parent_API='DefWindowProcA(WM_WINDOWPOSCHANGED)',args=a,packets=packets,index=0);self.position_frames.append(frame);self.position_dispatch(packets[0],frame);return
   self.ret(value,4);return
  if dll.lower()=='user32.dll'and name=='SystemParametersInfoA':
   a=self.args(4)
   if a[0]!=48 or a[1]or a[3]:raise Boundary('SystemParametersInfo_outside_readonly_workarea_scope',args=a)
   self.require_region(a[2],16,u.UC_PROT_WRITE,'native visible workarea RECT');out=(ctypes.c_int32*4)()
   if not self.window.fn['work_area'](self.window.state,out):raise Boundary('native_workarea_unavailable')
   self.uc.mem_write(a[2],bytes(out));self.window_record(name,a,return_value=1,actual_native_workarea=list(out));self.ret(1,4);return
  super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='style30final');p.add_argument('--tag',default='integrated31');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=IntegratedRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20'];payloads={}
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
   out=dict(source=a.resume,all_memory_CPU_and_four_threads_restored=True,thread_payloads=payloads,native_DD_sha256=hashlib.sha256(r.dd_native_blob).hexdigest(),native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),position_callback=r.position_callback,position_frames=r.position_frames,main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)));(R/'evidence/stage31-restore.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));return
  r.configure_window_callback();r.configure_dd();s=r.thread_run(24,max_steps=500000);r.verify_native_geometry();save_state(r,a.tag,'integrated original window geometry/DD/resource startup',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,dd_calls=r.dd_calls,dd_objects=r.dd_objects,native_virtual_display=r.dd.display()if r.dd else None,native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),paint_calls=r.paint_calls,position_calls=r.position_calls,position_callback=r.position_callback,position_frames=r.position_frames,window_callback_results=r.window_callback_results,window_calls=r.window_calls,boot_events=r.boot_events,startup_file_calls=r.startup_file_calls,startup_file_IO=r.startup_file_IO,original_startup='Unicorn i386 reference',native_platform='authored arm64 AppKit/CoreGraphics/Metal',original_code_patched=False,original_MSS_code_executed=False,full_resource_initializer_returned=any(e['label']=='graphics_and_data_initializer_return'for e in r.boot_events),city_root_returned=False);(R/f'evidence/stage31-{a.tag}.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
 finally:r.dispose()
if __name__=='__main__':main()
