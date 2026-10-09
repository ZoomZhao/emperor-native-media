from checkpoint_startup47 import *
from stage48_missing import missing_name,actual_absence
class MenuRuntime(MenuRuntime):
 extra_snapshot_fields=MenuRuntime.extra_snapshot_fields+['missing_calls48']
 def __init__(self,save):super().__init__(save);self.missing_calls48=[]
 def bind_resource(self,path):
  from stage46_find import GAME
  relative=path.lower()
  if relative.startswith(b'c:\\emperor\\'):relative=relative[len(b'c:\\emperor\\'):]
  rows=json.loads((R/'evidence/stage47-existing-save-paths.json').read_text());row=next((q for q in rows if Path(q['path']).parent==(GAME/'Save/zoom').resolve()and relative==b'save\\zoom\\'+Path(q['path']).name.encode('gbk').lower()),None)
  if not row:return super().bind_resource(path)
  p=Path(row['path']);body=p.read_bytes();assert len(body)==row['size']and hashlib.sha256(body).hexdigest()==row['sha256'];canonical=b'C:\\Emperor\\Save\\zoom\\'+p.name.encode('gbk');key=canonical.lower()
  if key in self.startup_files:return
  fd=NativeSavedFile(p);info=fd.info();assert fd.read(0,len(body))==body;self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],row['sha256']);self.startup_aliases[key]=canonical;self.startup_aliases[relative]=canonical
  if hasattr(self,'profile_bindings46'):self.profile_bindings46.append(dict(path='Save/zoom/'+p.name,bytes=len(body),copy_sha256=row['sha256'],readonly=True,stage48_scope='real existing frozen saves; original list header consumer and target load share actual readonly FD backing'))
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{b'c:\\emperor\\campaigns\\audio.pak',b'c:\\emperor\\campaigns\\untitled.pak'}
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage48-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name=='CompareFileTime':
   a=self.args(2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if pc!=0x40149a:raise Boundary('Compare_file_time_consumer_unreviewed48',args=a,return_PC=hex(pc))
   times=[]
   for pointer in a:
    self.require_region(pointer,8,u.UC_PROT_READ,'FILETIME32');times.append(struct.unpack('<Q',bytes(self.uc.mem_read(pointer,8)))[0])
   value=(times[0]>times[1])-(times[0]<times[1]);self.profile_bindings46.append(dict(name=name,return_PC=hex(pc),FILETIME_values=times,signed_return=value,source='https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-comparefiletime',real_file_times_only=True));self.ret(value&0xffffffff,2);return
  if dll.lower()=='kernel32.dll'and name=='GetFileInformationByHandle':
   a=self.args(2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));backing=self.native_for_handle(a[0]);path=self.files.get(a[0],{}).get('path',b'')
   if pc!=0x401424 or not backing or not path.startswith(b'c:\\emperor\\save\\zoom\\')or not path.endswith(b'.sav'):raise Boundary('File_information_consumer_unreviewed48',args=a,return_PC=hex(pc),path=str(path))
   self.require_region(a[1],52,u.UC_PROT_WRITE,'BY_HANDLE_FILE_INFORMATION32');meta=json.loads((R/'evidence/stage48-fileinfo-build.json').read_text());lib=Path(meta['library']);assert hashlib.sha256(lib.read_bytes()).hexdigest()==meta['sha256'];native=ctypes.CDLL(str(lib));f=native.s48_file_info;f.restype=ctypes.c_int;f.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)];v=(ctypes.c_uint32*13)();assert f(backing.state,v);self.uc.mem_write(a[1],bytes(v));self.profile_bindings46.append(dict(name=name,path=path.decode('gbk'),return_PC=hex(pc),return_value=1,actual_native_FD_fstat_fstatfs=list(v),consumer_reads_only_LastWrite_offsets=[20,24],NTFS_serial_attributes_equivalence=False));self.ret(1,2);return
  argc={'GetFullPathNameA':4,'CreateFileA':7,'GetFileAttributesA':1}.get(name)if dll.lower()=='kernel32.dll'else None
  if argc:
   a=self.args(argc);path=self.cstring(a[0]);filename=missing_name(path)
   if filename:
    pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));item=dict(name=name,args=a,path=path.decode(),return_PC=hex(pc),last_error_before=self.last_error,scope='two observed missing campaign packages only; no synthetic data or files')
    if name=='GetFullPathNameA':
     if pc!=0x77c4f9 or a[1]>4096:raise Boundary('Missing_fullpath_consumer_unreviewed48',**item)
     status,value,error,body=self.paths.get(261,bytes(261),self.last_error);assert status==1 and body[:value]==b'C:\\Emperor';cwd=body[:value];canonical=cwd+b'\\Campaigns\\'+filename
     if a[1]<=len(canonical):value=len(canonical)+1;item['buffer_untouched_required_capacity']=value
     else:
      self.require_region(a[2],len(canonical)+1,u.UC_PROT_WRITE,'missing file full path');self.uc.mem_write(a[2],canonical+b'\0')
      if a[3]:self.require_region(a[3],4,u.UC_PROT_WRITE,'file part pointer32');self.word(a[3],a[2]+canonical.rfind(b'\\')+1)
      value=len(canonical)
     item.update(canonical=canonical.decode(),backed_virtual_cwd=cwd.decode(),existence_not_required=True)
    else:
     if name=='CreateFileA':
      if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=128 or a[6]:raise Boundary('Missing_file_open_not_readonly48',**item)
      if a[3]:
       self.require_region(a[3],12,u.UC_PROT_READ,'SECURITY_ATTRIBUTES32');sa=list(struct.unpack('<3I',bytes(self.uc.mem_read(a[3],12))));item['security_attributes']=sa
       if sa[0]!=12 or sa[1]!=0 or sa[2]not in [0,1]:raise Boundary('Missing_open_security_scope48',**item)
     item['actual_missing']=actual_absence(filename);self.last_error=2;value=0xffffffff;item['no_handle_created']=True
    item.update(return_value=value,last_error_after=self.last_error);self.missing_calls48.append(item);self.api_trace.append(item);self.ret(value,argc);return
  return super().api(dll,name)
def inspect_png48(path):
 metadata=json.loads((R/'evidence/stage48-game-window-build.json').read_text());lib=Path(metadata['library']);assert hashlib.sha256(lib.read_bytes()).hexdigest()==metadata['sha256'];native=ctypes.CDLL(str(lib));f=native.s48_png;f.restype=ctypes.c_int;f.argtypes=[ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint32)];v=(ctypes.c_uint32*8)();assert f(str(path).encode(),v);return dict(zip(['width','height','bits_per_pixel','channels','grid_samples','nonblack_samples','nontransparent_samples','opaque_samples'],v))
def capture_nonempty_window48(r,tag,png):
 observations=[];primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();body=r.dd.pixels(primary[0]['id']);passed=False
 for attempt in range(8):
  v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);info=(ctypes.c_uint32*10)();assert r.dd.fn['capture_prepare'](r.dd.state,info);d=r.window.diagnostic();assert info[1]==info[5]==1 and info[6]==d['current_pid']and d['key_window']==d['NSApp_isActive']==d['activation_requests']==0 and d['frontmost_pid']!=d['current_pid'];trial=png.with_name(png.stem+'-attempt'+str(attempt)+'.png');q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(info[0]),str(trial)],capture_output=True,timeout=10);assert q.returncode==0;metrics=inspect_png48(trial);passed=metrics['nonblack_samples']>=200 and metrics['opaque_samples']>=200;observations.append(dict(attempt=attempt,metrics=metrics,passed=passed,path=str(trial),native_capture=list(info),diagnostic=d));(R/f'evidence/stage48-{tag}-capture-content.json').write_text(json.dumps(observations,indent=2))
  if passed:png.write_bytes(trial.read_bytes());break
  time.sleep(0.1);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,primary[0]['id'],stats,raw,len(raw),None);assert r.dd.pixels(primary[0]['id'])==body
 assert passed,'refuse transparent/black capture as actual UI evidence';return dict(path=str(png),sha256=hashlib.sha256(png.read_bytes()).hexdigest(),pixel_content=metrics,attempts=len(observations),actual_game_window=True)

def prepare_before_click48(r,tag):
 primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
 observations=[];info=(ctypes.c_uint32*10)()
 for i in range(20):
  v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);status=r.dd.fn['capture_prepare'](r.dd.state,info);d=r.window.diagnostic();ok=status and info[1]==info[5]==1 and info[6]==d['current_pid'];observations.append(dict(attempt=i,visible=list(v),capture=list(info),diagnostic=d,prepare_status=status));(R/f'evidence/stage48-{tag}-preclick-visibility.json').write_text(json.dumps(observations,indent=2));assert d['key_window']==d['NSApp_isActive']==d['activation_requests']==0 and d['frontmost_pid']!=d['current_pid']
  if ok:break
  time.sleep(0.05)
 assert ok,'refuse input before native onscreen ownership is proven';png=R/f'evidence/stage48-{tag}-before-click-window.png';proof=capture_nonempty_window48(r,tag+'-preclick',png);return dict(**proof,framebuffer_unchanged=True,onscreen_owned=True)

def probe_actual_game_window48(r,tag):
 p=prepare_before_click48(r,tag+'-recovery-before');g=r.verify_native_geometry();primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;body=r.dd.pixels(primary[0]['id']);cpu=r.uc.context_save().__getstate__()[0][40:];route=r.input_route_blob45;metadata=json.loads((R/'evidence/stage48-game-window-build.json').read_text());lib=Path(metadata['library']);assert hashlib.sha256(lib.read_bytes()).hexdigest()==metadata['sha256'];native=ctypes.CDLL(str(lib));launch=native.s48_launch;hide=native.s48_hide
 for f in [launch,hide]:f.restype=ctypes.c_int;f.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 pointer=r.window.fn['pointer'](r.window.state,0x58000001);v=(ctypes.c_uint32*8)();status=launch(pointer,v);out=dict(actual_game_window=True,host_visibility_fault_injected=True,launch_status=status,launch_observation=list(v),before_window_capture=p,close_before=r.lifecycle47())
 (R/f'evidence/stage48-{tag}-actual-game-window-recovery.json').write_text(json.dumps(out,indent=2));assert status==1 and v[3]==1,'actual game AppKit lifecycle must finish before recovery probe'
 visible=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,visible);out['before_visible']=list(visible);out['before_diagnostic']=r.window.diagnostic();assert visible[1]==visible[4]==1;hidden=(ctypes.c_uint32*8)();assert hide(pointer,hidden);out['hidden_observation']=list(hidden);out['hidden_diagnostic']=r.window.diagnostic();out['close_while_hidden']=r.lifecycle47();assert out['close_while_hidden']['actual_NSWindowWillClose_count']==0
 assert r.visible44(r.window.state,visible);info=(ctypes.c_uint32*10)();assert r.dd.fn['capture_prepare'](r.dd.state,info);d=r.window.diagnostic();out.update(recovered_visible=list(visible),recovered_capture=list(info),recovered_diagnostic=d,recovery_records=r.visibility_recoveries47,close_after_recovery=r.lifecycle47(),primary_pixels_unchanged=r.dd.pixels(primary[0]['id'])==body,full_current_CPU_unchanged=r.uc.context_save().__getstate__()[0][40:]==cpu,owned_mouse_FIFO_unchanged=r.input_route_blob45==route,geometry_unchanged=r.verify_native_geometry()==g)
 assert info[1]==info[5]==1 and info[6]==d['current_pid'];assert d['key_window']==d['NSApp_isActive']==d['activation_requests']==0 and d['frontmost_pid']==out['before_diagnostic']['frontmost_pid']!=d['current_pid'];assert all(out[q]for q in ['primary_pixels_unchanged','full_current_CPU_unchanged','owned_mouse_FIFO_unchanged','geometry_unchanged']);png=R/f'evidence/stage48-{tag}-actual-game-window-recovered.png';out['recovered_window_png']=capture_nonempty_window48(r,tag+'-recovered',png);out['passed']=True;(R/f'evidence/stage48-{tag}-actual-game-window-recovery.json').write_text(json.dumps(out,indent=2));return out

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='ready47');p.add_argument('--tag',default='missing48');p.add_argument('--seconds',type=int,default=14);p.add_argument('--click',nargs=2,type=int);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');p.add_argument('--probe-game-window',action='store_true');p.add_argument('--visibility-soak',type=int,default=0);a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=MenuRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.begin_watch47();r.thread_quantum=100000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  before=r.window.diagnostic();before_pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP));result=None
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
  else:
   assert r.registry.start()==r.audio.start()==0;r.configure_window_callback();r.configure_dd()
   preclick=None
   if a.click:preclick=prepare_before_click48(r,a.tag);r.inject_click(a.click)
   result=r.thread_run(a.seconds,max_steps=200000);after=r.window.diagnostic();assert after['key_window']==after['NSApp_isActive']==after['activation_requests']==0 and after['frontmost_pid']!=after['current_pid'];save_state(r,a.tag,'original menu continuation with owned window NSEvent FIFO; no guest gameplay fields injected',m['milestones'])
  out=dict(source=a.resume,state=a.tag,result=result,current_tid=r.current_tid,current_CPU_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),before_PC=before_pc,elapsed_seconds=time.monotonic()-start,test_input_calls=r.test_input_calls45,original_WndProc_entries=r.menu_events45,route=r.pump.route_diagnostic(),input_calls=r.input_calls43,pump_calls=r.pump_calls42,startup_counts=r.startup_counts43,startup_events=r.startup_events43,boot_events=r.boot_events,stream_calls=r.stream_calls44,stream_stats=r.streams44.stats(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(screen_mode=bytes(r.uc.mem_read(17686597,1))[0],gate=r.readword(0x88ec38),city_phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,game_fields_injected=False,global_events_posted=False,global_cursor_moved=False,files_uploaded=False,original_DLL_exports_executed=False,original_city_update_proven=False,all_memory_CPU_four_threads_native_blobs_restored=bool(a.restore_only))
  out['missing_calls48']=r.missing_calls48
  out['native_lifecycle47']=r.lifecycle47()
  out['visibility_recoveries47']=r.visibility_recoveries47
  out['preclick_window_capture']=preclick if not a.restore_only else None
  out['miles_file_calls']=r.miles_file_calls45;out['miles_file_allocations']=r.miles_file_allocations45;out['PCM_calls']=r.PCM_calls46;out['PCM_stats']={h:r.sample_pool.stats(h)[1] for h in r.PCM_handles46 if r.sample_pool.stats(h)[0]==0};out['file_releases']=r.file_releases46;out['find_calls']=r.find_calls46;out['profile_bindings']=r.profile_bindings46
  if a.visibility_soak:
   assert 0<=a.visibility_soak<=8
   deadline=time.monotonic()+a.visibility_soak
   while time.monotonic()<deadline:
    v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);time.sleep(0.05)
  if a.capture:
   (R/f'evidence/stage48-{a.tag}.json').write_text(json.dumps(out,indent=2))
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
   info=(ctypes.c_uint32*10)();observations=[]
   def observe(phase,visible=None,prepare_status=None):
    diag=r.window.diagnostic();checks=dict(native_visible=info[1]==1,CG_onscreen=info[5]==1,CG_owner=info[6]==diag['current_pid'],nonkey=diag['key_window']==0,inactive=diag['NSApp_isActive']==0,no_activation_request=diag['activation_requests']==0,other_frontmost=diag['frontmost_pid']!=diag['current_pid']);item=dict(phase=phase,elapsed=time.monotonic()-start,visible=visible,capture=list(info),diagnostic=diag,checks=checks,prepare_status=prepare_status);observations.append(item);(R/f'evidence/stage48-{a.tag}-visibility.json').write_text(json.dumps(observations,indent=2));return checks
   observe('after_primary_present_before_old_single_pass')
   v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);observe('after_old_visible',list(v));status=r.dd.fn['capture_prepare'](r.dd.state,info);checks=observe('after_old_capture_prepare',list(v),status)
   out['old_single_pass_result']=dict(prepare_status=status,checks=checks,passed=bool(status and all(checks.values())))
   for attempt in range(20):
    if status and all(checks.values()):break
    time.sleep(0.05);v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);status=r.dd.fn['capture_prepare'](r.dd.state,info);checks=observe('bounded_retry_'+str(attempt),list(v),status)
   out['visibility_observations']=observations;(R/f'evidence/stage48-{a.tag}.json').write_text(json.dumps(out,indent=2));assert status and all(checks.values()),'native onscreen contract failed; exact per-condition observations saved'
   diag=r.window.diagnostic();png=R/f'evidence/stage48-{a.tag}-window.png';proof=capture_nonempty_window48(r,a.tag+'-main',png);out['window_capture']=dict(**proof,native_capture=list(info),native_diagnostic=diag,restored_framebuffer_unchanged=True,host_restore_presentation_not_original_Blt=True)

  if a.probe_game_window:out['actual_game_window_recovery_probe48']=probe_actual_game_window48(r,a.tag)
  dest=R/f'evidence/stage48-{a.tag if not a.restore_only else "restore"}.json';dest.write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=result,route=out['route'],natural=out['natural_state'],elapsed_seconds=time.monotonic()-start)))
 finally:
  d=r.window.diagnostic();(R/f'evidence/stage48-{a.tag}-before-dispose.json').write_text(json.dumps(dict(elapsed=time.monotonic()-start,native_diagnostic=d,cleanup_close_not_yet_called=True,native_lifecycle=r.lifecycle47()),indent=2));r.dispose();(R/f'evidence/stage48-{a.tag}-after-dispose.json').write_text(json.dumps(dict(elapsed=time.monotonic()-start,explicit_cleanup_closed_window=True,native_lifecycle=r.lifecycle47()),indent=2))
if __name__=='__main__':main()
