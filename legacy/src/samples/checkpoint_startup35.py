#!/usr/bin/env python3
"""Same IJL contract across original callers; unknown parameters still refuse."""
from checkpoint_startup34 import ResourceRuntime,argparse,ctypes,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode
from checkpoint_startup32 import JPEGRuntime
from stage7_runtime import Boundary
class ContractRuntime(ResourceRuntime):
 extra_snapshot_fields=ResourceRuntime.extra_snapshot_fields+['jpeg_contract_checks35','raw_conversion_checks35','raw_conversion_pending35']
 def __init__(self,save):
  super().__init__(save);self.jpeg_contract_checks35=[];self.raw_conversion_checks35=[];self.raw_conversion_pending35=None
  rows=json.loads((R/'evidence/stage35-JPEG-catalog.readonly.json').read_text())['rows'];self.jpeg_catalog35={r['canonical_guest_key'].encode('ascii'):r for r in rows}
  for row in json.loads((R/'evidence/stage35-model-catalog.readonly.json').read_text())['rows']:
   relative=row['relative_path'].replace('/','\\').encode();self.resource_rows[relative.lower()]=row;self.resource_rows[(b'C:\\Emperor\\'+relative).lower()]=row
  self.uc.hook_add(u.UC_HOOK_CODE,self.raw_converted_hook,begin=0x525c68,end=0x525c68)
  self.uc.hook_add(u.UC_HOOK_CODE,self.raw_conversion_entry,begin=0x525c63,end=0x525c63)
  for pc,label in [(0x4756c7,'building_model_initializer_return'),(0x4756e7,'figure_model_initializer_return'),(0x475714,'tax_model_initializer_return'),(0x47572f,'post_model_503700_call'),(0x475734,'post_model_503700_return')]:self.uc.hook_add(u.UC_HOOK_CODE,self.boot_hook,user_data=label,begin=pc,end=pc)
 def jpeg_file(self,path):
  row=self.jpeg_catalog35.get(path.lower())
  if not row:raise Boundary('JPEG_path_outside_fixed_legal_catalog',guest_path=path.decode(errors='replace'))
  if not row['supported_header']:raise Boundary('JPEG_format_outside_baseline_RGB8_scope',header=row['JPEG_SOF'])
  p=Path(row['copy_path']);assert p.resolve().is_relative_to(R);body=p.read_bytes();assert len(body)==row['size'] and hashlib.sha256(body).hexdigest()==row['sha256'];assert hashlib.sha256(Path(row['original_path']).read_bytes()).hexdigest()==row['sha256'];return row,body
 @staticmethod
 def defaults():
  public=[0]*22
  for offset,value in [(20,3),(24,2),(52,3),(56,3),(60,1),(72,1),(76,1),(80,75)]:public[offset//4]=value
  return public
 def live_allocation(self,p,n):
  if not p or n<=0 or p+n>0x100000000:raise Boundary('JPEG_invalid_32bit_output_extent',pointer=hex(p),bytes=n)
  owners=[(base,size)for base,(_,size)in self.allocations.items()if base<=p and p+n<=base+size]
  if len(owners)!=1:raise Boundary('JPEG_output_not_contained_in_live_guest_allocation',pointer=hex(p),bytes=n,owner_count=len(owners))
  self.require_region(p,n,u.UC_PROT_READ|u.UC_PROT_WRITE,'live original CRT BGR buffer');return owners[0]
 def raw_conversion_entry(self,uc,address,size,user_data):
  q=self.jpeg_sessions.get(0x10c1f60)
  if not q or q.get('phase')!='decoded':raise Boundary('raw_JPEG_conversion_without_decode_session')
  sp=uc.reg_read(x.UC_X86_REG_ESP);src,dst,w,h,pitch=struct.unpack('<5I',bytes(uc.mem_read(sp,20)));meta=q['metadata'];n=w*h
  if [w,h,pitch]!=[meta['width'],meta['height'],meta['width']]or src!=q['output_pointer']:raise Boundary('raw_JPEG_RGB555_shape_mismatch',args=[src,dst,w,h,pitch])
  self.live_allocation(src,n*3);self.live_allocation(dst,n*2);self.raw_conversion_pending35=dict(args=[src,dst,w,h,pitch],path=q['path']);return
 def raw_converted_hook(self,uc,address,size,user_data):
  pending=self.raw_conversion_pending35
  if not pending:raise Boundary('raw_JPEG_conversion_return_without_entry')
  src,dst,w,h,pitch=pending['args'];n=w*h;bgr=bytes(uc.mem_read(src,n*3));actual=bytes(uc.mem_read(dst,n*2));expected=bytearray(n*2);words=memoryview(expected).cast('H')
  for i in range(n):b,g,r=bgr[i*3:i*3+3];words[i]=((r>>3)<<10)|((g>>3)<<5)|(b>>3)
  mismatches=sum(a!=b for a,b in zip(actual,expected));check=dict(original_function='0x525910',original_return_PC=hex(address),width=w,height=h,bytes=n*2,source_BGR_sha256=hashlib.sha256(bgr).hexdigest(),actual_RGB555_sha256=hashlib.sha256(actual).hexdigest(),byte_mismatches=mismatches,guest_path=pending['path'].decode(),guest_destination=hex(dst),original_instructions_unpatched=True)
  self.raw_conversion_checks35.append(check)
  if mismatches:raise Boundary('original_RGB555_conversion_differs_from_formula',check=check)
  self.raw_conversion_pending35=None
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage35-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def api(self,dll,name):
  if dll.lower()!='ijl10.dll':return super().api(dll,name)
  argc={'ordinal:2':1,'ordinal:3':1,'ordinal:4':2}.get(name)
  if argc is None:raise Boundary('IJL_API_outside_readonly_decoder_contract',name=name)
  a=self.args(argc);core=a[0];pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));self.require_region(core,20032,u.UC_PROT_READ|u.UC_PROT_WRITE,'JPEG_CORE_PROPERTIES32')
  if core!=0x10c1f60:raise Boundary('IJL_core_outside_original_singleton',core=hex(core))
  item=dict(name=name,args=a,return_pc=hex(pc),return_value=0,original_IJL_executed=False,backend='task-authored arm64 ImageIO',acceptance='validated parameters and session, no return-address whitelist')
  if name=='ordinal:2':
   if core in self.jpeg_sessions or bytes(self.uc.mem_read(core,20032))!=bytes(20032):raise Boundary('IJL_init_requires_cleared_core_and_no_live_session')
   public=self.defaults();self.uc.mem_write(core,struct.pack('<22I',*public));self.jpeg_sessions[core]=dict(phase='initialized');item['public_defaults']=public
  elif name=='ordinal:4':
   session=self.jpeg_sessions.get(core);mode=a[1];public=list(struct.unpack('<22I',bytes(self.uc.mem_read(core,88))))
   if not session or (mode,session['phase'])not in [(0,'initialized'),(2,'header')]:raise Boundary('IJL_read_mode_or_lifecycle_unsupported',mode=mode,phase=session.get('phase')if session else None)
   if not public[8]or public[9]or public[10]:raise Boundary('IJL_requires_file_source_no_memory_buffer',public=public)
   path=self.cstring(public[8],260);row,body=self.jpeg_file(path);info,bgr=self.jpeg.read(body,decode=mode==2);header=row['JPEG_SOF'];w,h=info['width'],info['height']
   if [w,h,info['source_bits_per_component'],info['source_color_components']]!=[header['width'],header['height'],8,3]:raise Boundary('ImageIO_metadata_differs_from_readonly_JPEG_header')
   expected=self.defaults();expected[8]=public[8]
   if mode==0:
    if public!=expected:raise Boundary('IJL_header_public_parameters_unsupported',public=public)
    for offset,value in [(44,w),(48,h),(52,3),(56,3)]:self.word(core+offset,value)
    session.update(phase='header',path=path,sha256=row['sha256'],metadata=info)
   else:
    if session['path'].lower()!=path.lower()or session['sha256']!=row['sha256']:raise Boundary('IJL_file_changed_between_header_and_decode')
    expected[1]=public[1];expected[2:7]=[w,h,0,3,2];expected[11:15]=[w,h,3,3]
    if public!=expected:raise Boundary('IJL_topdown_tight_BGR3_parameters_unsupported',public=public,expected=expected)
    owner=self.live_allocation(public[1],w*h*3)
    if public[1]<core+20032 and core<public[1]+len(bgr):raise Boundary('IJL_output_overlaps_core')
    assert len(bgr)==w*h*3;self.uc.mem_write(public[1],bgr);session.update(phase='decoded',output_pointer=public[1]);self.jpeg_outputs[path]=dict(guest_pointer=public[1],bytes=len(bgr),sha256=hashlib.sha256(bgr).hexdigest(),metadata=info)
    item.update(output_pointer=hex(public[1]),output_bytes=len(bgr),output_sha256=hashlib.sha256(bgr).hexdigest(),live_allocation_owner=[hex(owner[0]),owner[1]],stride=info['packed_BGR_stride'],orientation='top down; independently checked CoreGraphics with authored 2x2 colors',IJL_pixel_equivalence='unmeasured: original DLL execution not authorized')
   item.update(guest_path=path.decode(),source_sha256=row['sha256'],native_metadata=info,mode=mode)
  else:
   if self.jpeg_sessions.get(core,{}).get('phase')!='decoded':raise Boundary('IJL_free_requires_successful_decode_session')
   del self.jpeg_sessions[core]
  self.jpeg_contract_checks35.append(item);self.jpeg_calls.append(item);self.ret(0,argc)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='jpegsecond34');p.add_argument('--tag',default='image35');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=ContractRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20'];payloads={}
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,thread_payloads=payloads,native_DD_sha256=hashlib.sha256(r.dd_native_blob).hexdigest(),native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
  else:
   r.configure_window_callback();r.configure_dd();s=r.thread_run(22,max_steps=500000);r.verify_native_geometry();save_state(r,a.tag,'original resource startup using same IJL contract across callers',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,jpeg_contract_checks=r.jpeg_contract_checks35,raw_conversion_checks=r.raw_conversion_checks35,resource_binding_checks=r.resource_binding_checks,startup_file_calls=r.startup_file_calls,startup_file_IO=r.startup_file_IO,jpeg_calls=r.jpeg_calls,jpeg_output_hashes={k.decode():v for k,v in r.jpeg_outputs.items()},text_calls=r.text_calls,dd_calls=r.dd_calls,dd_objects=r.dd_objects,native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,presentation_events=r.presentation_events,original_startup='Unicorn i386 reference',native_platform='task-authored arm64 adapters',original_code_patched=False,original_IJL_code_executed=False,full_resource_initializer_returned=any(e['label']=='graphics_and_data_initializer_return'for e in r.boot_events),city_root_returned=False)
  (R/f'evidence/stage35-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=out.get('result'),JPEG_contract_calls=len(r.jpeg_contract_checks35),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
