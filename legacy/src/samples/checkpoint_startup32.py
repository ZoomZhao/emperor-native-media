#!/usr/bin/env python3
"""Original startup with a scoped, task-authored ImageIO IJL ABI adapter."""
import argparse,ctypes,hashlib,json,struct,time
from pathlib import Path
from checkpoint_startup31 import IntegratedRuntime,x,u,R,BASE,restore_state,save_state,decode
from stage32_jpeg import NativeJPEG
from stage32_dd import NativeDD
from stage7_runtime import Boundary

class JPEGRuntime(IntegratedRuntime):
 extra_snapshot_fields=IntegratedRuntime.extra_snapshot_fields+['jpeg_sessions','jpeg_calls','jpeg_outputs','presentation_events']
 def __init__(self,save):
  super().__init__(save);self.jpeg=NativeJPEG();self.jpeg_sessions={};self.jpeg_calls=[];self.jpeg_outputs={};self.presentation_events=[]
 def jpeg_file(self,path):
  row=json.loads((R/'evidence/stage31-next-JPEG-file.json').read_text())
  if path!=row['guest_path'].encode():raise Boundary('JPEG_file_outside_reviewed_baseline',guest_path=path.decode(errors='replace'))
  p=Path(row['copy_path']);assert p.resolve().is_relative_to(R);body=p.read_bytes();assert len(body)==row['size'] and hashlib.sha256(body).hexdigest()==row['sha256'];assert hashlib.sha256(Path(row['original_path']).read_bytes()).hexdigest()==row['sha256'];return row,body
 def ensure_dd(self):
  if self.dd is None:
   h=0x58000001;state=self.window.query(h);ptr=self.window.fn['pointer'](self.window.state,h);self.dd=NativeDD(ptr,state[3],state[4])
 def present_primary(self,o,pc,name):
  g=self.verify_native_geometry();n=g['client_w']*g['client_h']*4;raw=ctypes.create_string_buffer(n);out=(ctypes.c_uint32*12)();png=R/'evidence/stage32-original-primary.png';status=self.dd.fn['present'](self.dd.state,o['id'],out,raw,n,str(png).encode());stats=dict(zip(['width','height','desktop_crop_x','desktop_crop_y','GPU_readback_pitch','GPU_byte_mismatches','actual_drawable_presentations','visible','key_before','key_after','active_before','active_after'],out))
  if not status:raise Boundary('native_primary_Metal_present_failed',stats=stats)
  self.presentation_events.append(dict(original_call_return_pc=hex(pc),method=name,surface=o['id'],GPU_readback_sha256=hashlib.sha256(raw.raw).hexdigest(),stats=stats,source='only original successful primary Blt pixels',PNG_path=str(png)))
 def api(self,dll,name):
  if dll=='authored.dd'and name in ['2.5','2.7']:
   a=self.args(6);o=self.lookup(a[0],2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));info=self.dd.info(o['id'])
   if name=='2.5'and a[4]==0x1000200:
    if pc!=0x526005 or a[5]:raise Boundary('ASYNC_WAIT_copy_consumer_outside_reviewed_scope',return_pc=hex(pc),args=a)
    src=self.lookup(a[2],2);si=self.dd.info(src['id']);sr=list(struct.unpack('<4i',bytes(self.uc.mem_read(a[3],16))))if a[3]else[0,0,si['w'],si['h']];dr=list(struct.unpack('<4i',bytes(self.uc.mem_read(a[1],16))))if a[1]else[0,0,info['w'],info['h']]
    if [dr[2]-dr[0],dr[3]-dr[1]]!=[sr[2]-sr[0],sr[3]-sr[1]]:raise Boundary('ASYNC_WAIT_stretch_not_supported',source=sr,destination=dr)
    self.sync_to_native(src);self.sync_to_native(o);value=0 if self.dd.fn['blt'](self.dd.state,o['id'],dr[0],dr[1],src['id'],(ctypes.c_int32*4)(*sr))else 0x80004005
    if not value:self.sync_from_native(o)
    self.record_dd('2.5.ASYNC_WAIT_copy',a,value,source_rect=sr,destination_rect=dr,backend='idle FIFO copy completes before return; WAIT satisfied; no busy concurrency tested');self.ret(value,6)
   else:super().api(dll,name)
   if info['caps']&0x200 and not self.uc.reg_read(x.UC_X86_REG_EAX) and a[2 if name=='2.5'else 3]:self.present_primary(o,pc,name)
   return
  if dll.lower()=='ijl10.dll':
   argc={'ordinal:2':1,'ordinal:3':1,'ordinal:4':2}.get(name)
   if argc is None:raise Boundary('IJL_API_outside_readonly_decoder_scope',name=name)
   a=self.args(argc);core=a[0];pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));self.require_region(core,20032,u.UC_PROT_READ|u.UC_PROT_WRITE,'original JPEG_CORE_PROPERTIES32')
   if core!=0x10c1f60:raise Boundary('IJL_core_outside_original_loader',core=hex(core))
   item=dict(name=name,args=a,return_pc=hex(pc),return_value=0,original_IJL_executed=False,backend='task-authored arm64 ImageIO')
   if name=='ordinal:2':
    if pc!=0x525d4c or core in self.jpeg_sessions:raise Boundary('IJL_init_consumer_or_lifecycle_mismatch',return_pc=hex(pc))
    public=[0]*22
    for offset,value in [(20,3),(24,2),(52,3),(56,3),(60,1),(72,1),(76,1),(80,75)]:public[offset//4]=value
    self.uc.mem_write(core,struct.pack('<22I',*public));self.jpeg_sessions[core]=dict(phase='initialized');item['public_defaults']=public
   elif name=='ordinal:4':
    session=self.jpeg_sessions.get(core);mode=a[1];public=list(struct.unpack('<22I',bytes(self.uc.mem_read(core,88))))
    if not session:raise Boundary('IJL_read_without_scoped_init')
    path=self.cstring(public[8]);row,body=self.jpeg_file(path);info,bgr=self.jpeg.read(body,decode=mode==2)
    if mode==0 and pc==0x525d82 and session['phase']=='initialized':
     self.word(core+44,info['width']);self.word(core+48,info['height']);self.word(core+52,3);self.word(core+56,3);session.update(phase='header',path=path,sha256=row['sha256'],metadata=info)
    elif mode==2 and pc==0x525e2e and session['phase']=='header':
     if public[2:7]!=[info['width'],info['height'],0,3,2] or public[11:15]!=[info['width'],info['height'],3,3]:raise Boundary('IJL_BGR_contract_not_supported',public=public)
     self.require_region(public[1],len(bgr),u.UC_PROT_WRITE,'original malloc JPEG BGR32 buffer');self.uc.mem_write(public[1],bgr);session['phase']='decoded';self.jpeg_outputs[path]=dict(guest_pointer=public[1],bytes=len(bgr),sha256=hashlib.sha256(bgr).hexdigest(),metadata=info)
     item.update(output_pointer=hex(public[1]),output_bytes=len(bgr),output_sha256=hashlib.sha256(bgr).hexdigest(),stride=info['packed_BGR_stride'],orientation='top down; independently checked CoreGraphics with authored 2x2 colors',IJL_pixel_equivalence='unmeasured: original DLL execution not authorized')
    else:raise Boundary('IJL_read_mode_or_consumer_outside_reviewed_scope',mode=mode,return_pc=hex(pc),phase=session['phase'])
    item.update(guest_path=path.decode(),source_sha256=row['sha256'],native_metadata=info,mode=mode)
   else:
    if pc!=0x526021 or self.jpeg_sessions.get(core,{}).get('phase')!='decoded':raise Boundary('IJL_free_outside_original_success_lifecycle',return_pc=hex(pc))
    del self.jpeg_sessions[core]
   self.jpeg_calls.append(item);self.ret(0,argc);return
  super().api(dll,name)

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='groups31');p.add_argument('--tag',default='jpeg32');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=JPEGRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20'];payloads={}
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,thread_payloads=payloads,native_DD_sha256=hashlib.sha256(r.dd_native_blob).hexdigest(),jpeg_sessions={hex(k):dict(v,path=v['path'].decode())if'path'in v else v for k,v in r.jpeg_sessions.items()},jpeg_output_hashes={k.decode():v['sha256']for k,v in r.jpeg_outputs.items()},native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
  else:
   r.configure_window_callback();r.configure_dd();s=r.thread_run(24,max_steps=500000);r.verify_native_geometry();save_state(r,a.tag,'original JPEG loading/conversion/display continuation using own ImageIO ABI',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,jpeg_calls=r.jpeg_calls,jpeg_output_hashes={k.decode():v for k,v in r.jpeg_outputs.items()},dd_calls=r.dd_calls,dd_objects=r.dd_objects,native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,presentation_events=r.presentation_events,original_startup='Unicorn i386 reference',native_platform='task-authored arm64 AppKit/CoreGraphics/ImageIO/Metal',original_code_patched=False,original_IJL_code_executed=False,full_resource_initializer_returned=any(e['label']=='graphics_and_data_initializer_return'for e in r.boot_events),city_root_returned=False)
  (R/f'evidence/stage32-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=out.get('result'),JPEG_calls=len(r.jpeg_calls),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
