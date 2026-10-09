from checkpoint_startup43 import InputRuntime,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode,Boundary,NativeInput,NativeCursorSelection,NativePump,JPEGRuntime
from stage44_window import NativeWindow
from stage44_stream import NativeStreams
import ctypes,subprocess
class StreamRuntime(InputRuntime):
 extra_snapshot_fields=InputRuntime.extra_snapshot_fields+['streams_blob44','stream_calls44']
 def __init__(self,save):
  super().__init__(save);self.native_cursor43.dispose();self.pump.dispose();self.window.dispose();self.window=NativeWindow();self.pump=NativePump(self.window);self.native_input43=NativeInput(self.window);self.native_cursor43=NativeCursorSelection(self.window);self.streams44=NativeStreams(self.audio);self.stream_calls44=[];self.visible44=self.window.lib.s44_visible;self.visible44.restype=ctypes.c_int;self.visible44.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 @property
 def streams_blob44(self):return self.streams44.snapshot()
 @streams_blob44.setter
 def streams_blob44(self,b):
  if b is not None:self.streams44.restore(b)
 def segment(self,*a,**kw):
  assert self.streams44.service()==0;out=super().segment(*a,**kw);assert self.streams44.service()==0;info=(ctypes.c_uint32*6)();assert self.visible44(self.window.state,info);return out
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage44-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def dispose(self):
  if self.streams44.state:self.streams44.dispose()
  super().dispose()
 def api(self,dll,name):
  argc={'_AIL_open_stream@12':3,'_AIL_set_stream_loop_count@8':2,'_AIL_set_stream_volume@8':2,'_AIL_start_stream@4':1,'_AIL_stream_status@4':1,'_AIL_close_stream@4':1}.get(name)if dll.lower()=='mss32.dll'else None
  if argc is None:return super().api(dll,name)
  a=self.args(argc);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));before=self.streams44.stats();value=self.uc.reg_read(x.UC_X86_REG_EAX)
  if name=='_AIL_open_stream@12':
   path=self.cstring(a[1],260)
   if pc!=0x762d46 or a[0]!=self.audio.driver or path.lower()!=b'audio\\music\\menu1-san_xuan.mp3' or a[2]!=256000:raise Boundary('Stream_open_parameters_unreviewed44',args=a,path=str(path),return_pc=hex(pc))
   sources=json.loads((R/'evidence/stage43-stream-contract.json').read_text())['music_files'];source=Path(sources[0]['path']);assert hashlib.sha256(source.read_bytes()).hexdigest()==sources[0]['sha256'];status,value=self.streams44.open(source,a[2])
   if status:raise Boundary('Actual_system_MP3_decoder_failed44',OSStatus=status,path=str(source))
  elif name=='_AIL_close_stream@4'and a[0]==0:return super().api(dll,name)
  else:
   if a[0]!=before['handle']or not a[0]:raise Boundary('Unknown_or_stale_stream_handle44',args=a)
   allowed={'_AIL_set_stream_loop_count@8':0x762daf,'_AIL_set_stream_volume@8':0x762dc7,'_AIL_start_stream@4':0x762ddb,'_AIL_close_stream@4':0x762d1c,'_AIL_stream_status@4':0x762e83}
   if name in allowed and pc!=allowed[name]:raise Boundary('Stream_caller_not_reviewed44',name=name,return_pc=hex(pc))
   # Actual caller PCs are captured; only values proven in the frozen caller are accepted.
   if name=='_AIL_set_stream_loop_count@8'and a[1]!=0:raise Boundary('Original_stream_loop_value_unreviewed44',args=a)
   if name=='_AIL_set_stream_volume@8'and a[1]!=20:raise Boundary('Original_stream_volume_unreviewed44',args=a)
   if name=='_AIL_stream_status@4':
    value=self.streams44.fn['status'](self.streams44.state,a[0]);status=0 if value>=0 else value
   else:
    command={'_AIL_set_stream_loop_count@8':1,'_AIL_set_stream_volume@8':2,'_AIL_start_stream@4':3,'_AIL_close_stream@4':5}[name];status=self.streams44.command(a[0],command,a[1]if argc==2 else 0)
   if status:raise Boundary('Actual_native_stream_lifecycle_failed44',name=name,args=a,status=status)
  item=dict(name=name,args=a,return_pc=hex(pc),return_value=value,before=before,after=self.streams44.stats(),backend='actual ExtAudioFile retained file + decoded PCM ring, offline consumption clocked by real silent AudioUnit',source_PCM_sent_to_device=False,original_MSS_export_executed=False,exact_original_decoder_mixer_equivalence=False);self.stream_calls44.append(item);self.api_trace.append(item);self.ret(value,argc)

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='final43');p.add_argument('--tag',default='stream44');p.add_argument('--seconds',type=int,default=16);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=StreamRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
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
   assert r.registry.start()==0 and r.audio.start()==0;r.configure_window_callback();r.configure_dd();result=r.thread_run(a.seconds,max_steps=200000);r.verify_native_geometry();after_diag=r.window.diagnostic();(R/f'evidence/stage44-{a.tag}-focus.json').write_text(json.dumps(dict(before=before_diag,after=after_diag,result=result),indent=2));assert after_diag['key_window']==0 and after_diag['activation_requests']==0 and after_diag['frontmost_pid']!=after_diag['current_pid'];save_state(r,a.tag,'original startup through actual offline MP3 stream lifecycle; background nonactivating onscreen panel; source PCM never device queued',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=result,before_PC=before,after_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),elapsed_seconds=time.monotonic()-start,input_calls=r.input_calls43,startup_counts=r.startup_counts43,startup_events=r.startup_events43,pump_calls=r.pump_calls42,boot_events=r.boot_events,presentation_events=r.presentation_events,dd_calls=r.dd_calls,bink_calls=r.bink_calls42,native_geometry=r.verify_native_geometry(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(gate=r.readword(0x88ec38),phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,original_loop_or_movie_skipped=False,original_MSS_Bink_M3D_exports_executed=False,global_input_events_posted=False,main_menu_passed=False,city_logic_passed=False,city_tick_proven=False,full_native_WinMain_proven=False,Bink_decode_or_sound_factory_equivalence_proven=False)
  out['stream_calls44']=r.stream_calls44;out['stream_stats44']=r.streams44.stats();out['native_before']=before_diag;out['coordinate_roundtrip_cases']=coordinate_cases
  if a.capture:
   # Reconstruct the transient MTKView from the restored real primary framebuffer.
   # This is a host restore presentation, recorded separately from original Blt calls.
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;obj=primary[0];g=r.verify_native_geometry();pixels_before=r.dd.pixels(obj['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,obj['id'],stats,raw,len(raw),None);assert r.dd.pixels(obj['id'])==pixels_before
   out['host_restore_presentation']=dict(reason='MTKView is process-local and absent from serialized DD state; display restored actual primary pixels',primary_surface_id=obj['id'],framebuffer_sha256=hashlib.sha256(pixels_before).hexdigest(),native_stats=list(stats),framebuffer_unchanged=True,original_Blt_call_fabricated=False)
   stability=[]
   for attempt in range(30):
    v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);d=r.window.diagnostic();stability.append(dict(attempt=attempt,visible=list(v),diagnostic=d));assert d['key_window']==d['NSApp_isActive']==d['activation_requests']==0 and d['frontmost_pid']!=d['current_pid'];time.sleep(0.05)
   out['background_stability_observations']=stability
   assert not a.restore_only and r.dd;wid=r.dd.fn['window_id'](r.dd.state);assert wid;info=(ctypes.c_uint32*10)();assert r.dd.fn['capture_prepare'](r.dd.state,info);obs=[]
   for attempt in range(20):
    visible=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,visible);assert r.dd.fn['capture_prepare'](r.dd.state,info);diag=r.window.diagnostic();obs.append(dict(attempt=attempt,visible=list(visible),capture=list(info),diagnostic=diag))
    if info[1]==info[5]==1 and info[6]==diag['current_pid']:break
   (R/f'evidence/stage44-{a.tag}-visibility.json').write_text(json.dumps(obs,indent=2));assert info[1]==info[5]==1 and info[6]==r.window.diagnostic()['current_pid'];png=R/f'evidence/stage44-{a.tag}-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(wid),str(png)],capture_output=True,timeout=10);diag=r.window.diagnostic();out['window_capture']=dict(owned_window_number=wid,owned_window_only=True,path=str(png),returncode=q.returncode,stderr=q.stderr.decode(errors='replace'),native_capture_observation=list(info),native_after=diag)
   assert q.returncode==0 and png.exists()and diag['key_window']==diag['activation_requests']==diag['NSApp_isActive']==0 and diag['frontmost_pid']!=diag['current_pid'];out['window_capture']['sha256']=hashlib.sha256(png.read_bytes()).hexdigest()
  (R/f'evidence/stage44-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,result=out.get('result'),startup_counts=r.startup_counts43,input_calls=len(r.input_calls43),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
