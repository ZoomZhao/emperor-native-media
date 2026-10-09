from checkpoint_profiles46 import *
class MenuRuntime(MenuRuntime):
 extra_snapshot_fields=MenuRuntime.extra_snapshot_fields+['visibility_recoveries47']
 def __init__(self,save):
  super().__init__(save);self.visibility_recoveries47=[];meta=json.loads((R/'evidence/stage47-visibility-build.json').read_text());lib=Path(meta['library']);assert hashlib.sha256(lib.read_bytes()).hexdigest()==meta['sha256'];self.visibility_lib47=ctypes.CDLL(str(lib));self.recover47=self.visibility_lib47.s47_recover;self.recover47.restype=ctypes.c_int;self.recover47.argtypes=[ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];self.original_visible44=self.visible44;self.visible44=self.fixed_visible47
 @property
 def find_sessions46(self):return self._find_sessions46
 @find_sessions46.setter
 def find_sessions46(self,v):
  from stage47_find import enumerate_saves,enumerate_campaigns
  from stage46_find import enumerate_profiles,os
  for fd in self._find_fds46.values():os.close(fd)
  self._find_fds46={};self._find_sessions46=v
  for h,session in v.items():
   fd,records=(enumerate_saves if session['pattern'].lower()==b'save\\zoom\\*.sav'else enumerate_campaigns if session['pattern'].lower()==b'campaigns\\*'else enumerate_profiles)();assert [(r['name'],r['sha256'],r['device'],r['inode'])for r in records]==[(r['name'],r['sha256'],r['device'],r['inode'])for r in session['records']];self._find_fds46[h]=fd
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name=='FindFirstFileA':
   a=self.args(2);pattern=self.cstring(a[0]);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if pattern.lower()in [b'save\\zoom\\*.sav',b'campaigns\\*']:
    from stage47_find import enumerate_saves,enumerate_campaigns
    from stage46_find import os
    if pc not in ([0x43bbcd]if pattern.lower()==b'save\\zoom\\*.sav'else [0x43bd5e,0x43beb3]):raise Boundary('Directory_search_caller_unreviewed47',return_PC=hex(pc))
    self.require_region(a[1],320,u.UC_PROT_WRITE,'WIN32_FIND_DATAA32');fd,records=(enumerate_saves if pattern.lower()==b'save\\zoom\\*.sav'else enumerate_campaigns)()
    if not records:os.close(fd);self.last_error=2;value=0xffffffff
    else:
     value=self.find_next_handle46;self.find_next_handle46+=0x100;self._find_fds46[value]=fd;self._find_sessions46[value]=dict(pattern=pattern,records=records,index=1);self.uc.mem_write(a[1],records[0]['WIN32_FIND_DATAA'])
    self.find_calls46.append(dict(name=name,args=a,return_PC=hex(pc),pattern=pattern.decode('gbk'),actual_name=records[0]['name']if records else None,actual_all_names=[q['name']for q in records],actual_owned_directory_FD=True,readonly=True,no_target_filter_or_synthetic_entries=True,return_value=value,last_error=self.last_error));self.ret(value,2);return
  if dll.lower()=='kernel32.dll'and name=='GetFileAttributesA':
   a=self.args(1);path=self.cstring(a[0]);rows=json.loads((R/'evidence/stage47-campaign-files.json').read_text());row=next((q for q in rows if path.lower()in [b'campaigns\\'+q['name'].encode('gbk').lower(),b'campaigns\\\\'+q['name'].encode('gbk').lower()]),None)
   if row:
    from stage47_find import campaign_metadata
    v,value=campaign_metadata(row);self.profile_bindings46.append(dict(name=name,path=path.decode('gbk'),return_PC=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),return_value=value,readonly=True,actual_fstat=True,Windows_DOS_metadata_full_equivalence=False));self.ret(value,1);return
  return super().api(dll,name)
 def bind_resource(self,path):
  from stage46_find import GAME
  relative=path.lower()
  if relative.startswith(b'c:\\emperor\\'):relative=relative[len(b'c:\\emperor\\'):]
  rows=json.loads((R/'evidence/stage47-campaign-files.json').read_text());row=next((q for q in rows if not q['is_directory']and relative==b'campaigns\\'+q['name'].encode('gbk').lower()),None)
  if row:
   p=Path(row['path']);body=p.read_bytes();assert len(body)==row['size']and hashlib.sha256(body).hexdigest()==row['sha256'];canonical=b'C:\\Emperor\\Campaigns\\'+row['name'].encode('gbk');key=canonical.lower()
   if key in self.startup_files:return
   fd=NativeSavedFile(p);info=fd.info();assert fd.read(0,len(body))==body;self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],row['sha256']);self.startup_aliases[key]=canonical;self.startup_aliases[relative]=canonical
   if hasattr(self,'profile_bindings46'):self.profile_bindings46.append(dict(path='Campaigns/'+row['name'],bytes=len(body),copy_sha256=row['sha256'],readonly=True,stage47_scope='actual frozen authored campaign data, no original implementation reused'))
   return
  allowed={b'save\\zoom\\st9aftr.sav':'ST9AFTR.sav',b'save\\zoom\\st9base.sav':'ST9BASE.sav'};relative=path.lower()
  if relative.startswith(b'c:\\emperor\\'):relative=relative[len(b'c:\\emperor\\'):]
  name=allowed.get(relative)
  if not name:return super().bind_resource(path)
  p=GAME/'Save/zoom'/name;row=next(q for q in json.loads((R/'evidence/stage47-existing-save-paths.json').read_text())if Path(q['path'])==p.resolve());body=p.read_bytes();assert len(body)==row['size']and hashlib.sha256(body).hexdigest()==row['sha256'];canonical=b'C:\\Emperor\\Save\\zoom\\'+name.encode();key=canonical.lower()
  if key in self.startup_files:return
  fd=NativeSavedFile(p);info=fd.info();assert fd.read(0,len(body))==body;self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],row['sha256']);self.startup_aliases[key]=canonical;self.startup_aliases[relative]=canonical
  if hasattr(self,'profile_bindings46'):self.profile_bindings46.append(dict(path='Save/zoom/'+name,bytes=len(body),copy_sha256=row['sha256'],readonly=True,stage47_scope='existing frozen baseline test save; no remap of ST9AFTR to ST9BASE, no profile/save edits'))
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{b'c:\\emperor\\save\\zoom\\st9aftr.sav',b'c:\\emperor\\save\\zoom\\st9base.sav'}|{b'c:\\emperor\\campaigns\\'+q['name'].encode('gbk').lower()for q in json.loads((R/'evidence/stage47-campaign-files.json').read_text())if not q['is_directory']}
 def begin_watch47(self):
  f=self.visibility_lib47.s47_watch;f.restype=ctypes.c_int;f.argtypes=[ctypes.c_void_p];assert f(self.window.fn['pointer'](self.window.state,0x58000001))
 def lifecycle47(self):
  f=self.visibility_lib47.s47_lifecycle;f.restype=ctypes.c_int;f.argtypes=[ctypes.POINTER(ctypes.c_uint32)];a=(ctypes.c_uint32*2)();assert f(a);return dict(actual_NSWindowWillClose_count=a[0],owned_window_object_alive=bool(a[1]))
 def fixed_visible47(self,state,out):
  status=self.original_visible44(state,out)
  if not status:return status
  life=struct.unpack('<16I',self.window.life_export()[:64]);diag=(ctypes.c_uint32*12)();pointer=self.window.fn['pointer'](self.window.state,0x58000001);assert self.recover47(pointer,life[3],diag)
  if diag[3]:
   self.visibility_recoveries47.append(dict(diagnostic=list(diag),guest_visible=life[3],action='orderFront only current task NSPanel after late launch hide; no activate or makeKey'));return self.original_visible44(state,out)
  return status
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage47-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
def prepare_before_click47(r,tag):
 primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
 observations=[];info=(ctypes.c_uint32*10)()
 for i in range(20):
  v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);status=r.dd.fn['capture_prepare'](r.dd.state,info);d=r.window.diagnostic();ok=status and info[1]==info[5]==1 and info[6]==d['current_pid'];observations.append(dict(attempt=i,visible=list(v),capture=list(info),diagnostic=d,prepare_status=status));(R/f'evidence/stage47-{tag}-preclick-visibility.json').write_text(json.dumps(observations,indent=2));assert d['key_window']==d['NSApp_isActive']==d['activation_requests']==0 and d['frontmost_pid']!=d['current_pid']
  if ok:break
  time.sleep(0.05)
 assert ok,'refuse input before native onscreen ownership is proven';png=R/f'evidence/stage47-{tag}-before-click-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(info[0]),str(png)],capture_output=True,timeout=10);assert q.returncode==0;return dict(path=str(png),sha256=hashlib.sha256(png.read_bytes()).hexdigest(),framebuffer_unchanged=True,onscreen_owned=True)

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='stable46');p.add_argument('--tag',default='visible47');p.add_argument('--seconds',type=int,default=14);p.add_argument('--click',nargs=2,type=int);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');p.add_argument('--visibility-soak',type=int,default=0);a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=MenuRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
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
   if a.click:preclick=prepare_before_click47(r,a.tag);r.inject_click(a.click)
   result=r.thread_run(a.seconds,max_steps=200000);after=r.window.diagnostic();assert after['key_window']==after['NSApp_isActive']==after['activation_requests']==0 and after['frontmost_pid']!=after['current_pid'];save_state(r,a.tag,'original menu continuation with owned window NSEvent FIFO; no guest gameplay fields injected',m['milestones'])
  out=dict(source=a.resume,state=a.tag,result=result,current_tid=r.current_tid,current_CPU_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),before_PC=before_pc,elapsed_seconds=time.monotonic()-start,test_input_calls=r.test_input_calls45,original_WndProc_entries=r.menu_events45,route=r.pump.route_diagnostic(),input_calls=r.input_calls43,pump_calls=r.pump_calls42,startup_counts=r.startup_counts43,startup_events=r.startup_events43,boot_events=r.boot_events,stream_calls=r.stream_calls44,stream_stats=r.streams44.stats(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(screen_mode=bytes(r.uc.mem_read(17686597,1))[0],gate=r.readword(0x88ec38),city_phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,game_fields_injected=False,global_events_posted=False,global_cursor_moved=False,files_uploaded=False,original_DLL_exports_executed=False,original_city_update_proven=False,all_memory_CPU_four_threads_native_blobs_restored=bool(a.restore_only))
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
   (R/f'evidence/stage47-{a.tag}.json').write_text(json.dumps(out,indent=2))
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
   info=(ctypes.c_uint32*10)();observations=[]
   def observe(phase,visible=None,prepare_status=None):
    diag=r.window.diagnostic();checks=dict(native_visible=info[1]==1,CG_onscreen=info[5]==1,CG_owner=info[6]==diag['current_pid'],nonkey=diag['key_window']==0,inactive=diag['NSApp_isActive']==0,no_activation_request=diag['activation_requests']==0,other_frontmost=diag['frontmost_pid']!=diag['current_pid']);item=dict(phase=phase,elapsed=time.monotonic()-start,visible=visible,capture=list(info),diagnostic=diag,checks=checks,prepare_status=prepare_status);observations.append(item);(R/f'evidence/stage47-{a.tag}-visibility.json').write_text(json.dumps(observations,indent=2));return checks
   observe('after_primary_present_before_old_single_pass')
   v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);observe('after_old_visible',list(v));status=r.dd.fn['capture_prepare'](r.dd.state,info);checks=observe('after_old_capture_prepare',list(v),status)
   out['old_single_pass_result']=dict(prepare_status=status,checks=checks,passed=bool(status and all(checks.values())))
   for attempt in range(20):
    if status and all(checks.values()):break
    time.sleep(0.05);v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);status=r.dd.fn['capture_prepare'](r.dd.state,info);checks=observe('bounded_retry_'+str(attempt),list(v),status)
   out['visibility_observations']=observations;(R/f'evidence/stage47-{a.tag}.json').write_text(json.dumps(out,indent=2));assert status and all(checks.values()),'native onscreen contract failed; exact per-condition observations saved'
   diag=r.window.diagnostic();png=R/f'evidence/stage47-{a.tag}-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(info[0]),str(png)],capture_output=True,timeout=10);assert q.returncode==0;out['window_capture']=dict(path=str(png),sha256=hashlib.sha256(png.read_bytes()).hexdigest(),native_capture=list(info),native_diagnostic=diag,restored_framebuffer_unchanged=True,host_restore_presentation_not_original_Blt=True)

  dest=R/f'evidence/stage47-{a.tag if not a.restore_only else "restore"}.json';dest.write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=result,route=out['route'],natural=out['natural_state'],elapsed_seconds=time.monotonic()-start)))
 finally:
  d=r.window.diagnostic();(R/f'evidence/stage47-{a.tag}-before-dispose.json').write_text(json.dumps(dict(elapsed=time.monotonic()-start,native_diagnostic=d,cleanup_close_not_yet_called=True,native_lifecycle=r.lifecycle47()),indent=2));r.dispose();(R/f'evidence/stage47-{a.tag}-after-dispose.json').write_text(json.dumps(dict(elapsed=time.monotonic()-start,explicit_cleanup_closed_window=True,native_lifecycle=r.lifecycle47()),indent=2))
if __name__=='__main__':main()
