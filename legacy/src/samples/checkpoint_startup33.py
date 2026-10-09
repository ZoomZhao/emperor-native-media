#!/usr/bin/env python3
"""Original ASCII loading text through original primary Blt and Metal."""
import argparse,ctypes,hashlib,json,struct,time,subprocess
from pathlib import Path
from checkpoint_startup32 import JPEGRuntime,x,u,R,BASE,restore_state,save_state,decode
from stage33_text import NativeText
from stage33_dd import NativeDD
from stage7_runtime import Boundary
class TextRuntime(JPEGRuntime):
 extra_snapshot_fields=JPEGRuntime.extra_snapshot_fields+['text_calls','text_dc_state']
 def __init__(self,save):super().__init__(save);self.native_text=NativeText();self.text_calls=[];self.text_dc_state={};self.capture_first_frame=True
 def ensure_dd(self):
  if self.dd is None:
   h=0x58000001;state=self.window.query(h);ptr=self.window.fn['pointer'](self.window.state,h);self.dd=NativeDD(ptr,state[3],state[4])
 def text_bytes(self,p,n):
  if not 0<n<=1024:raise Boundary('GDI_text_length_outside_finite_scope',length=n)
  self.require_region(p,n,u.UC_PROT_READ,'counted ASCII GDI text');body=bytes(self.uc.mem_read(p,n))
  if any(v<32 or v>126 for v in body):raise Boundary('GDI_non_ASCII_encoding_requires_separate_adapter',body_hex=body.hex())
  return body
 def present_primary(self,o,pc,name):
  super().present_primary(o,pc,name)
  src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage33-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
  if self.capture_first_frame:
   self.capture_first_frame=False;wid=self.dd.fn['window_id'](self.dd.state);assert wid;info=(ctypes.c_uint32*10)();assert self.dd.fn['capture_prepare'](self.dd.state,info),'capture preparation changed focus/activation';png=R/'evidence/stage33-window.png';result=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(wid),str(png)],capture_output=True,timeout=10);self.presentation_events[-1].update(owned_window_number=wid,capture_observation=dict(zip(['window_number','visible','key','active','finished_launching','CG_onscreen','CG_owner_pid','CG_layer','CG_window_found','total_drawable_presentations'],info)),window_screenshot=str(png),screenshot_returncode=result.returncode,screenshot_stderr=result.stderr.decode(errors='replace'))
   if result.returncode or not png.exists():raise Boundary('owned_window_screenshot_failed',window_number=wid,stderr=result.stderr.decode(errors='replace'))
   raise Boundary('checkpoint_first_original_primary_Blt_Metal_frame',original_return_pc=hex(pc),presentation=self.presentation_events[-1],intentional_milestone_stop=True)
 def api(self,dll,name):
  if dll.lower()=='gdi32.dll'and name in ['GetTextExtentPointA','SetBkColor','SetTextColor','TextOutA']:
   counts={'GetTextExtentPointA':4,'SetBkColor':2,'SetTextColor':2,'TextOutA':5};a=self.args(counts[name]);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));expected={'GetTextExtentPointA':0x52b499,'SetBkColor':0x52b4dc,'SetTextColor':0x52b4e8,'TextOutA':0x52b500}
   if pc!=expected[name]or self.gui.object_kind_c(self.gui.state,a[0])!=1:raise Boundary('GDI_text_consumer_or_live_DC_outside_reviewed_scope',name=name,return_pc=hex(pc),args=a)
   dc=self.text_dc_state.setdefault(a[0],dict(background=0xffffff,text=0,align='TA_LEFT|TA_TOP|TA_NOUPDATECP',background_mode='OPAQUE',font='installed macOS system UI font 13; Windows SYSTEM_FONT equivalence unmeasured'));item=dict(name=name,args=a,return_pc=hex(pc),encoding='strict ASCII, identical byte interpretation within this scope',backend='actual arm64 CoreText/CoreGraphics; owned virtual desktop surface')
   if name=='GetTextExtentPointA':
    body=self.text_bytes(a[1],a[2]);m=self.native_text.measure(body);alt=self.native_text.measure(body,1);self.require_region(a[3],8,u.UC_PROT_WRITE,'GDI SIZE32');self.uc.mem_write(a[3],struct.pack('<2i',int(m['integer_width']),int(m['integer_height'])));value=1;item.update(text=body.decode(),measured=m,alternative_native_font=alt,native_font_sensitivity_width_delta=alt['integer_width']-m['integer_width'],Windows_original_font_delta='unmeasured; no pixel equivalence claimed')
   elif name in ['SetBkColor','SetTextColor']:
    if a[1]>0xffffff:raise Boundary('GDI_palette_color_not_supported',args=a)
    key='background'if name=='SetBkColor'else'text';value=dc[key];dc[key]=a[1];item.update(previous_COLORREF=value,new_COLORREF=a[1])
   else:
    body=self.text_bytes(a[3],a[4]);candidates=[q for q in self.dd_objects.values()if q['kind']==2 and self.dd.info(q['id'])['caps']&0x200]
    if len(candidates)!=1:raise Boundary('GDI_virtual_desktop_primary_ownership_ambiguous')
    o=candidates[0];di=self.dd.info(o['id']);g=self.verify_native_geometry();clip=[g['client_x'],g['client_y'],g['client_x']+g['client_w'],g['client_y']+g['client_h']];before=bytes(self.uc.mem_read(o['guest_pixels'],di['pitch']*di['h']));px=ctypes.c_int32(a[1]).value;py=ctypes.c_int32(a[2]).value;after,box=self.native_text.draw(before,di['w'],di['h'],di['pitch'],px,py,body,dc['background'],dc['text'],clip);changed=[]
    for y in range(di['h']):
     for xx in range(di['w']):
      k=y*di['pitch']+xx*4
      if before[k:k+4]!=after[k:k+4]:
       if not(clip[0]<=xx<clip[2]and clip[1]<=y<clip[3]):raise Boundary('GDI_native_text_write_outside_owned_client_clip',x=xx,y=y)
       changed.append((xx,y))
    if not changed:raise Boundary('GDI_original_text_produced_no_pixels')
    self.uc.mem_write(o['guest_pixels'],after);self.sync_to_native(o);value=1;item.update(text=body.decode(),actual_position=[px,py],logical_background_and_baseline=box,measured=self.native_text.measure(body),native_owned_client_clip=clip,changed_pixels=len(changed),actual_changed_rect=[min(xy[0]for xy in changed),min(xy[1]for xy in changed),max(xy[0]for xy in changed)+1,max(xy[1]for xy in changed)+1],pixel_sha256_before=hashlib.sha256(before).hexdigest(),pixel_sha256_after=hashlib.sha256(after).hexdigest(),Windows_font_pixel_equivalence=False)
   item['return_value']=value;self.text_calls.append(item);self.ret(value,counts[name]);return
  if dll.lower()=='user32.dll'and name=='ReleaseDC':
   a=self.args(2);super().api(dll,name);self.text_dc_state.pop(a[1],None);return
  super().api(dll,name)

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='present32');p.add_argument('--tag',default='text33');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=TextRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.capture_first_frame=not bool(r.presentation_events);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20'];payloads={}
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,thread_payloads=payloads,native_DD_sha256=hashlib.sha256(r.dd_native_blob).hexdigest(),jpeg_sessions={hex(k):dict(v,path=v['path'].decode())if'path'in v else v for k,v in r.jpeg_sessions.items()},jpeg_output_hashes={k.decode():v['sha256']for k,v in r.jpeg_outputs.items()},native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
  else:
   r.configure_window_callback();r.configure_dd();s=r.thread_run(22,max_steps=500000);r.verify_native_geometry();save_state(r,a.tag,'original JPEG loading/conversion/display continuation using own ImageIO ABI',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,jpeg_calls=r.jpeg_calls,jpeg_output_hashes={k.decode():v for k,v in r.jpeg_outputs.items()},text_calls=r.text_calls,text_dc_state=r.text_dc_state,dd_calls=r.dd_calls,dd_objects=r.dd_objects,native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,presentation_events=r.presentation_events,original_startup='Unicorn i386 reference',native_platform='task-authored arm64 AppKit/CoreGraphics/ImageIO/Metal',original_code_patched=False,original_IJL_code_executed=False,full_resource_initializer_returned=any(e['label']=='graphics_and_data_initializer_return'for e in r.boot_events),city_root_returned=False)
  (R/f'evidence/stage33-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=out.get('result'),JPEG_calls=len(r.jpeg_calls),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
