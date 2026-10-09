#!/usr/bin/env python3
"""Continue original startup after its first primary/Metal frame."""
from checkpoint_startup33 import TextRuntime,argparse,ctypes,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode
from checkpoint_startup32 import JPEGRuntime
from stage17_file import NativeSavedFile
from stage7_runtime import Boundary
from stage34_dd import NativeDD
class ChunkedSavedFile(NativeSavedFile):
 def read(self,offset,size):
  if not 0<=size<=64*1024*1024:raise Boundary('readonly_file_request_above_bounded_64MiB_scope',offset=offset,bytes=size,path=str(self.path))
  if not size:return super().read(offset,0)
  parts=[];done=0
  while done<size:
   n=min(size-done,2*1024*1024);body=super().read(offset+done,n);parts.append(body);done+=len(body)
   if len(body)<n:break
  return b''.join(parts)
class ResourceRuntime(TextRuntime):
 extra_snapshot_fields=TextRuntime.extra_snapshot_fields+['resource_binding_checks','heap_mapped_end34','heap_growth_events34']
 def __init__(self,save):
  super().__init__(save);self.resource_binding_checks=[];self.heap_mapped_end34=0x11000000;self.heap_growth_events34=[]
  for pc,label in [(0x5cd066,'graphics_group_parse_return'),(0x5cce17,'graphics_named_group_return')]:self.uc.hook_add(u.UC_HOOK_CODE,self.boot_hook,user_data=label,begin=pc,end=pc)
 def allocate(self,heap,flags,size):
  if heap not in self.heaps:raise Boundary('invalid_platform_heap',handle=hex(heap))
  if flags&~0x9:raise Boundary('unsupported_HeapAlloc_flags',flags=hex(flags))
  actual=max(8,(size+7)&~7);p=self.heap_next;end=p+actual
  if end>0x20000000:return 0
  if end>self.heap_mapped_end34:
   new_end=(end+0xffffff)&~0xffffff;old_end=self.heap_mapped_end34;self.map(old_end,new_end-old_end,u.UC_PROT_READ|u.UC_PROT_WRITE);self.heap_mapped_end34=new_end;self.heap_growth_events34.append(dict(requested_size=size,pointer=hex(p),mapped_low=hex(old_end),mapped_high_exclusive=hex(new_end),actual_mapped_bytes=new_end-old_end,guest_domain_limit='0x20000000, before scratch',original_code_patched=False))
  self.heap_next=end;self.allocations[p]=(heap,size);self.uc.mem_write(p,bytes(actual)if flags&8 else bytes([0xcd])*actual);return p
 def prepare_heap_restore(self,prior):
  for section in prior['regions']:
   if 0x11000000<=section['low'] and section['high']<0x20000000:self.map(section['low'],section['high']-section['low']+1,section['permissions'])
 def ensure_dd(self):
  if self.dd is None:
   h=0x58000001;state=self.window.query(h);ptr=self.window.fn['pointer'](self.window.state,h);self.dd=NativeDD(ptr,state[3],state[4])
 def jpeg_file(self,path):
  if path==b'data\\china_load1.jpg':return super().jpeg_file(path)
  rows=json.loads((R/'evidence/stage34-next-JPEG-inputs.json').read_text())['rows'];row=next((r for r in rows if r['guest_path'].encode()==path),None)
  if not row:raise Boundary('JPEG_file_outside_stage34_reviewed_baseline',guest_path=path.decode(errors='replace'))
  p=Path(row['copy_path']);assert p.resolve().is_relative_to(R);body=p.read_bytes();assert len(body)==row['size'] and hashlib.sha256(body).hexdigest()==row['sha256'];assert hashlib.sha256(Path(row['original_path']).read_bytes()).hexdigest()==row['sha256'];assert row['JPEG_SOF']==dict(marker='0xc0',bits=8,height=600,width=800,components=3);return row,body
 def bind_resource(self,path):
  row=self.resource_rows.get(path.lower())
  if not row:return
  canonical=b'C:\\Emperor\\'+row['relative_path'].replace('/','\\').encode();key=canonical.lower()
  if key in self.startup_files:return
  copy=Path(row['copy_path']);assert copy.resolve().is_relative_to(R);body=copy.read_bytes();assert len(body)==row['copy_size']and hashlib.sha256(body).hexdigest()==row['copy_sha256'];assert hashlib.sha256(Path(row['original_path']).read_bytes()).hexdigest()==row['original_sha256'];fd=ChunkedSavedFile(copy);info=fd.info()
  for offset in range(0,len(body),2*1024*1024):assert fd.read(offset,min(2*1024*1024,len(body)-offset))==body[offset:offset+2*1024*1024]
  self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],row['copy_sha256']);self.startup_aliases[key]=canonical;self.startup_aliases[row['relative_path'].replace('/','\\').encode().lower()]=canonical;self.resource_binding_checks.append(dict(path=row['relative_path'],bytes=len(body),native_read_chunks=(len(body)+2*1024*1024-1)//(2*1024*1024),sha256=row['copy_sha256'],all_chunks_equal_original_copy=True))
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name)
  src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage34-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def api(self,dll,name):
  if dll=='authored.dd'and name=='2.24':
   a=self.args(1);o=self.lookup(a[0],2);out=(ctypes.c_uint32*7)();assert self.dd.fn['is_lost'](self.dd.state,o['id'],out);value=0 if out[0]else 0x887601c2;self.record_dd('2.24.IsLost',a,value,actual_native_owned_surface=dict(zip(['resident','alive','w','h','pitch','locked','Metal_device_exists'],out)));self.ret(value,1);return
  if dll.lower()=='kernel32.dll'and name=='ReadFile':
   a=self.args(5);h,buffer,n,out,overlap=a;backing=self.native_for_handle(h)
   if n>2*1024*1024 and isinstance(backing,ChunkedSavedFile):
    if overlap or not out or n>64*1024*1024:raise Boundary('large_readonly_resource_ReadFile_scope',args=a)
    f=self.files[h];before=f['position'];body=backing.read(before,n);assert body==f['payload'][before:before+n];self.require_region(buffer,len(body),u.UC_PROT_WRITE,'large native resource read buffer');self.require_region(out,4,u.UC_PROT_WRITE,'ReadFile actual byte count')
    if body:self.uc.mem_write(buffer,body)
    f['position']+=len(body);self.word(out,len(body));item=dict(name=name,handle=hex(h),requested_bytes=n,bytes=len(body),position_before=before,position_after=f['position'],guest_path=f['path'].decode(),guest_read_sha256=hashlib.sha256(bytes(self.uc.mem_read(buffer,len(body)))).hexdigest(),source_slice_equal=bytes(self.uc.mem_read(buffer,len(body)))==body,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),backend='actual readonly native pread chunks <=2MiB, original aggregate size/EOF and byte count preserved');self.record_native_read(item,backing);self.native_file_read_calls.append(item);self.api_trace.append(item);self.ret(1,5);return
  super().api(dll,name)

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='text33fixed');p.add_argument('--tag',default='resources34');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=ResourceRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20'];payloads={}
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:];payloads[tid]=hashlib.sha256(t['context'].__getstate__()[0][40:]).hexdigest()
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,thread_payloads=payloads,native_DD_sha256=hashlib.sha256(r.dd_native_blob).hexdigest(),jpeg_sessions={hex(k):dict(v,path=v['path'].decode())if'path'in v else v for k,v in r.jpeg_sessions.items()},jpeg_output_hashes={k.decode():v['sha256']for k,v in r.jpeg_outputs.items()},native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
  else:
   r.configure_window_callback();r.configure_dd();s=r.thread_run(22,max_steps=500000);r.verify_native_geometry();save_state(r,a.tag,'original resource startup continuation after first original frame',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,resource_binding_checks=r.resource_binding_checks,startup_file_calls=r.startup_file_calls,startup_file_IO=r.startup_file_IO,jpeg_calls=r.jpeg_calls,jpeg_output_hashes={k.decode():v for k,v in r.jpeg_outputs.items()},text_calls=r.text_calls,text_dc_state=r.text_dc_state,dd_calls=r.dd_calls,dd_objects=r.dd_objects,native_geometry=r.verify_native_geometry(),native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,presentation_events=r.presentation_events,original_startup='Unicorn i386 reference',native_platform='task-authored arm64 AppKit/CoreGraphics/ImageIO/Metal',original_code_patched=False,original_IJL_code_executed=False,full_resource_initializer_returned=any(e['label']=='graphics_and_data_initializer_return'for e in r.boot_events),city_root_returned=False)
  (R/f'evidence/stage34-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=out.get('result'),JPEG_calls=len(r.jpeg_calls),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
