from checkpoint_startup48 import *
from checkpoint_startup48 import MenuRuntime as Previous
from stage49_readonly import FrozenTree,PolicyError,normalize,os,stat
class MenuRuntime(Previous):
 extra_snapshot_fields=Previous.extra_snapshot_fields+['readonly_calls49','file_open_details49','city_frames49','city_observations49']
 def __init__(self,save):
  super().__init__(save);self.readonly_calls49=[];self.file_open_details49={};self._city_frames49=[];self.city_observations49=[];self._city_return_hooks49={}
  for pc,label in [(0x534a30,'city_load'),(0x4ac2b0,'city_scheduler'),(0x4ac650,'city_cycle_tail'),(0x401cd0,'save_row_select')]:self.uc.hook_add(u.UC_HOOK_CODE,self.city_entry49,user_data=label,begin=pc,end=pc)
 @property
 def city_frames49(self):return self._city_frames49
 @city_frames49.setter
 def city_frames49(self,v):
  self._city_frames49=v
  for q in v:self.watch_city_return49(q['return_PC'])
 def watch_city_return49(self,pc):
  if pc not in self._city_return_hooks49:self._city_return_hooks49[pc]=self.uc.hook_add(u.UC_HOOK_CODE,self.city_return49,begin=pc,end=pc)
 def city_values49(self):return dict(phase=self.readword(0xc82ef8),gate=self.readword(0x88ec38),population=self.readword(0x130f988),treasury=self.readword(0x130f970),original_selected_name=self.cstring(0x10ddcd0).decode('gbk',errors='backslashreplace'),original_selected_index=self.readword(0x88ebe4))
 def city_entry49(self,uc,pc,size,label):
  sp=uc.reg_read(x.UC_X86_REG_ESP);ret=self.readword(sp);self.require_region(ret,1,u.UC_PROT_EXEC,'observed original city return PC');q=dict(label=label,tid=self.current_tid,entry_PC=pc,return_PC=ret,owner_SP=sp,before=self.city_values49());self._city_frames49.append(q);self.watch_city_return49(ret);self.city_observations49.append(dict(kind='entry',**q))
 def city_return49(self,uc,pc,size,user):
  sp=uc.reg_read(x.UC_X86_REG_ESP)
  for q in list(self._city_frames49):
   if q['tid']==self.current_tid and q['return_PC']==pc and sp==q['owner_SP']+4:self.city_observations49.append(dict(kind='returned',**q,EAX=uc.reg_read(x.UC_X86_REG_EAX),after=self.city_values49(),original_cdecl_return_stack_verified=True));self._city_frames49.remove(q)
 def tree49(self):
  if not hasattr(self,'_tree49'):self._tree49=FrozenTree()
  return self._tree49
 def path49(self,raw):
  status,size,error,body=self.paths.get(261,bytes(261),self.last_error)
  if status!=1:raise PolicyError('owned virtual current directory unavailable')
  return normalize(raw,body[:size])
 def bind49(self,raw,res):
  tree=self.tree49();canonical=('C:\\Emperor\\'+'\\'.join(res['actual'])).encode('gbk');key=canonical.lower()
  if key not in self.startup_files:
   fd,body,wanted=tree.bind(res);info=fd.info();self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],wanted)
  else:
   fd,body=self.startup_files[key];s=res['stat'];identity=self._startup_identity[key];assert(s.st_size,s.st_dev,s.st_ino)==identity[:3];assert tree.manifest['/'.join(res['actual']).casefold()]==identity[3];parts=[]
   for offset in range(0,len(body),2*1024*1024):parts.append(fd.read(offset,min(2*1024*1024,len(body)-offset)))
   assert b''.join(parts)==body
  self.startup_aliases[key]=canonical;self.startup_aliases[raw.lower()]=canonical;self.startup_aliases[b'\\'.join(p.encode('gbk')for p in res['actual']).lower()]=canonical;return canonical
 def bind_resource(self,path):
  try:
   parts,full=self.path49(path);res=self.tree49().open(parts)
   try:
    if res['error']:return
    if stat.S_ISREG(res['stat'].st_mode):self.bind49(path,res)
   finally:
    if res['fd']is not None:os.close(res['fd'])
  except PolicyError as e:raise Boundary('Readonly_game_tree_policy49',path=path.decode('gbk',errors='replace'),reason=str(e))
 def present_primary(self,o,pc,name):
  JPEGRuntime.present_primary(self,o,pc,name);src=Path(self.presentation_events[-1]['PNG_path']);dest=R/'evidence/stage49-original-primary.png';src.rename(dest);self.presentation_events[-1]['PNG_path']=str(dest)
 def api(self,dll,name):
  argc={'CreateFileA':7,'GetFullPathNameA':4,'GetFileAttributesA':1,'CreateDirectoryA':2}.get(name)if dll.lower()=='kernel32.dll'else None
  if argc:
   a=self.args(argc);raw=self.cstring(a[0]);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));item=dict(name=name,args=a,path=raw.decode('gbk',errors='replace'),return_PC=hex(pc),last_error_before=self.last_error,contract='one frozen game tree, readonly openat+O_NOFOLLOW, actual errno and data only',Windows_cross_process_share_ACL_short_names_equivalence=False)
   try:
    parts,full=self.path49(raw);item['normalized_virtual_path']=full.decode('gbk')
    if name=='GetFullPathNameA':
     if a[1]>4096:raise PolicyError('fullpath output capacity above bounded scope')
     if a[1]<=len(full):value=len(full)+1;item['output_untouched_required_capacity']=value
     else:
      self.require_region(a[2],len(full)+1,u.UC_PROT_WRITE,'readonly tree full path');self.uc.mem_write(a[2],full+b'\0')
      if a[3]:self.require_region(a[3],4,u.UC_PROT_WRITE,'readonly tree file part32');self.word(a[3],a[2]+full.rfind(b'\\')+1)
      value=len(full)
     item['existence_not_required']=True
    else:
     if name=='CreateFileA':
      if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]not in[0,128]or a[6]:raise PolicyError('write/create/delete/async/directory/unknown access flags refused')
      if a[3]:
       self.require_region(a[3],12,u.UC_PROT_READ,'readonly SECURITY_ATTRIBUTES32');sa=list(struct.unpack('<3I',bytes(self.uc.mem_read(a[3],12))));item['security_attributes']=sa
       if sa[0]!=12 or sa[1]!=0 or sa[2]not in[0,1]:raise PolicyError('unreviewed security descriptor')
     res=self.tree49().open(parts);item['native_path']=str(res['path']);item['actual_openat']=res['trace']
     try:
      if name=='CreateDirectoryA':
       if a[1]or res['error']:raise PolicyError('actual directory creation/security changes refused; only already-existing readonly branch supported')
       self.last_error=183;value=0;item['actual_existing_directory_or_file_no_mkdir']=True;item['reference']='https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya'
      elif res['error']:self.last_error=res['error'];value=0xffffffff;item['no_handle_created']=True
      elif name=='GetFileAttributesA':
       s=res['stat'];value=(16 if stat.S_ISDIR(s.st_mode)else 0)|(0 if s.st_mode&0o222 else 1)|(2 if getattr(s,'st_flags',0)&0x8000 else 0);value=value or 128;item['actual_fstat_attributes']=True
      elif stat.S_ISDIR(res['stat'].st_mode):self.last_error=5;value=0xffffffff;item['directory_open_without_backup_semantics']=True
      else:
       canonical=self.bind49(raw,res);key=canonical.lower();conflict=False
       for h,f in self.files.items():
        if f.get('path')==key:
         older=self.file_open_details49.get(h)
         if older is None:raise PolicyError('existing live guest handle has unreviewed sharing mode')
         conflict=conflict or not(older['share']&1 and a[2]&1)
       if conflict:self.last_error=32;value=0xffffffff;item['guest_read_sharing_violation']=True
       else:
        self._startup_snapshot_identity[key]=self._startup_identity[key];value=self.next_handle;self.next_handle+=1;self.files[value]=dict(path=key,payload=self.startup_files[key][1],position=0);self.file_open_details49[value]=dict(share=a[2],access=a[1],inherit=bool(a[3]and item['security_attributes'][2]),path=key);item['actual_sha256']=self._startup_identity[key][3];item['actual_bytes']=len(self.startup_files[key][1])
     finally:
      if res['fd']is not None:os.close(res['fd'])
    item.update(return_value=value,last_error_after=self.last_error);self.readonly_calls49.append(item);self.api_trace.append(item);self.ret(value,argc);return
   except PolicyError as e:raise Boundary('Readonly_game_tree_policy49',**item,reason=str(e))
  return super().api(dll,name)
 def dispose(self):
  if hasattr(self,'_tree49'):self._tree49.close()
  super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='sorted48');p.add_argument('--tag',default='list49');p.add_argument('--seconds',type=int,default=14);p.add_argument('--click',nargs=2,type=int);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');p.add_argument('--probe-game-window',action='store_true');p.add_argument('--visibility-soak',type=int,default=0);a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=MenuRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.begin_watch47();r.thread_quantum=100000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  before=r.window.diagnostic();before_pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP));result=None;native_reads_before49=len(r.native_file_read_calls);path_events_before49=len(r.path_events)
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
  else:
   assert r.registry.start()==r.audio.start()==0;r.configure_window_callback();r.configure_dd()
   preclick=None
   if a.click:preclick=prepare_before_click48(r,a.tag);r.inject_click(a.click)
   result=r.thread_run(a.seconds,max_steps=200000);after=r.window.diagnostic();assert after['key_window']==after['NSApp_isActive']==after['activation_requests']==0 and after['frontmost_pid']!=after['current_pid'];save_state(r,a.tag,'original menu continuation with owned window NSEvent FIFO; no guest gameplay fields injected',m['milestones'])
  out=dict(source=a.resume,state=a.tag,result=result,current_tid=r.current_tid,current_CPU_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),before_PC=before_pc,elapsed_seconds=time.monotonic()-start,test_input_calls=r.test_input_calls45,original_WndProc_entries=r.menu_events45,route=r.pump.route_diagnostic(),input_calls=r.input_calls43,pump_calls=r.pump_calls42,startup_counts=r.startup_counts43,startup_events=r.startup_events43,boot_events=r.boot_events,stream_calls=r.stream_calls44,stream_stats=r.streams44.stats(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(screen_mode=bytes(r.uc.mem_read(17686597,1))[0],gate=r.readword(0x88ec38),city_phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,game_fields_injected=False,global_events_posted=False,global_cursor_moved=False,files_uploaded=False,original_DLL_exports_executed=False,original_city_update_proven=False,all_memory_CPU_four_threads_native_blobs_restored=bool(a.restore_only))
  out['missing_calls48']=r.missing_calls48;out['readonly_calls49']=r.readonly_calls49;out['file_open_details49']={str(h):{k:(q.decode('gbk')if isinstance(q,bytes)else q)for k,q in v.items()}for h,v in r.file_open_details49.items()};out['native_file_read_calls_this_run49']=r.native_file_read_calls[native_reads_before49:];out['path_events_this_run49']=r.path_events[path_events_before49:];out['city_observations49']=r.city_observations49;out['city_frames49']=r.city_frames49
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
   (R/f'evidence/stage49-{a.tag}.json').write_text(json.dumps(out,indent=2))
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
   info=(ctypes.c_uint32*10)();observations=[]
   def observe(phase,visible=None,prepare_status=None):
    diag=r.window.diagnostic();checks=dict(native_visible=info[1]==1,CG_onscreen=info[5]==1,CG_owner=info[6]==diag['current_pid'],nonkey=diag['key_window']==0,inactive=diag['NSApp_isActive']==0,no_activation_request=diag['activation_requests']==0,other_frontmost=diag['frontmost_pid']!=diag['current_pid']);item=dict(phase=phase,elapsed=time.monotonic()-start,visible=visible,capture=list(info),diagnostic=diag,checks=checks,prepare_status=prepare_status);observations.append(item);(R/f'evidence/stage49-{a.tag}-visibility.json').write_text(json.dumps(observations,indent=2));return checks
   observe('after_primary_present_before_old_single_pass')
   v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);observe('after_old_visible',list(v));status=r.dd.fn['capture_prepare'](r.dd.state,info);checks=observe('after_old_capture_prepare',list(v),status)
   out['old_single_pass_result']=dict(prepare_status=status,checks=checks,passed=bool(status and all(checks.values())))
   for attempt in range(20):
    if status and all(checks.values()):break
    time.sleep(0.05);v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);status=r.dd.fn['capture_prepare'](r.dd.state,info);checks=observe('bounded_retry_'+str(attempt),list(v),status)
   out['visibility_observations']=observations;(R/f'evidence/stage49-{a.tag}.json').write_text(json.dumps(out,indent=2));assert status and all(checks.values()),'native onscreen contract failed; exact per-condition observations saved'
   diag=r.window.diagnostic();png=R/f'evidence/stage49-{a.tag}-window.png';proof=capture_nonempty_window48(r,a.tag+'-main',png);out['window_capture']=dict(**proof,native_capture=list(info),native_diagnostic=diag,restored_framebuffer_unchanged=True,host_restore_presentation_not_original_Blt=True)

  if a.probe_game_window:out['actual_game_window_recovery_probe48']=probe_actual_game_window48(r,a.tag)
  dest=R/f'evidence/stage49-{a.tag if not a.restore_only else "restore"}.json';dest.write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=result,route=out['route'],natural=out['natural_state'],elapsed_seconds=time.monotonic()-start)))
 finally:
  d=r.window.diagnostic();(R/f'evidence/stage49-{a.tag}-before-dispose.json').write_text(json.dumps(dict(elapsed=time.monotonic()-start,native_diagnostic=d,cleanup_close_not_yet_called=True,native_lifecycle=r.lifecycle47()),indent=2));r.dispose();(R/f'evidence/stage49-{a.tag}-after-dispose.json').write_text(json.dumps(dict(elapsed=time.monotonic()-start,explicit_cleanup_closed_window=True,native_lifecycle=r.lifecycle47()),indent=2))
if __name__=='__main__':main()
