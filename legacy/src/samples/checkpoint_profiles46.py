from checkpoint_startup46 import MenuRuntime as Previous,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode,Boundary,ctypes,subprocess,JPEGRuntime
from stage46_find import enumerate_profiles,profile_rows,verify,os
from stage17_file import NativeSavedFile
class MenuRuntime(Previous):
 extra_snapshot_fields=Previous.extra_snapshot_fields+['find_sessions46','find_next_handle46','find_calls46','profile_bindings46']
 def __init__(self,save):
  super().__init__(save);self._find_sessions46={};self._find_fds46={};self.find_next_handle46=0x66000101;self.find_calls46=[];self.profile_bindings46=[]
 @property
 def find_sessions46(self):return self._find_sessions46
 @find_sessions46.setter
 def find_sessions46(self,v):
  for fd in self._find_fds46.values():os.close(fd)
  self._find_fds46={};self._find_sessions46=v
  for h,session in v.items():
   fd,records=enumerate_profiles();assert [(r['name'],r['sha256'],r['device'],r['inode'])for r in records]==[(r['name'],r['sha256'],r['device'],r['inode'])for r in session['records']];self._find_fds46[h]=fd
 def bind_resource(self,path):
  row=next((r for r in profile_rows()if path.lower()in [r['relative_path'].replace('/','\\').encode('gbk').lower(),b'c:\\emperor\\'+r['relative_path'].replace('/','\\').encode('gbk').lower()]),None)
  if not row:return super().bind_resource(path)
  canonical=b'C:\\Emperor\\'+row['relative_path'].replace('/','\\').encode('gbk');key=canonical.lower()
  if key in self.startup_files:return
  p=verify(row);body=p.read_bytes();fd=NativeSavedFile(p);info=fd.info();assert fd.read(0,len(body))==body;self.startup_files[key]=(fd,body);self._startup_identity[key]=(info[0],info[4],info[5],row['copy_sha256']);self.startup_aliases[key]=canonical;self.startup_aliases[row['relative_path'].replace('/','\\').encode('gbk').lower()]=canonical
  if hasattr(self,'profile_bindings46'):self.profile_bindings46.append(dict(path=row['relative_path'],bytes=len(body),copy_sha256=row['copy_sha256'],installed_source_sha256=row['original_sha256'],copy_equals_installed_source=row['copy_sha256']==row['original_sha256'],scope='fixed legitimate isolated baseline profile; both source and frozen copy independently unchanged'))
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{b'c:\\emperor\\save\\zoom.dat'}
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name=='GetFileAttributesA':
   a=self.args(1);path=self.cstring(a[0]);relative=path.lower();names=[r['relative_path'][:-4].replace('/','\\').encode('gbk').lower()for r in profile_rows()]
   if relative in names:
    from stage46_find import GAME
    i=names.index(relative);native_path=GAME/profile_rows()[i]['relative_path'][:-4];fd=os.open(native_path,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
    try:metadata=os.fstat(fd);value=16|(1 if not metadata.st_mode&0o222 else 0)|(2 if metadata.st_flags&0x8000 else 0)
    finally:os.close(fd)
    self.profile_bindings46.append(dict(name=name,path=path.decode('gbk'),return_PC=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),return_value=value,backend='actual readonly open O_DIRECTORY/O_NOFOLLOW and fstat of matching existing baseline profile directory',native_device=metadata.st_dev,native_inode=metadata.st_ino,readonly=True,Windows_DOS_ACL_attribute_equivalence=False));self.ret(value,1);return
  if dll.lower()=='kernel32.dll'and name in ['FindFirstFileA','FindNextFileA','FindClose']:
   n=1 if name=='FindClose'else 2;a=self.args(n);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));item=dict(name=name,args=a,return_PC=hex(pc),backend='actual owned readonly POSIX directory FD + native readonly metadata; observed full-name consumer only',NTFS_short_names_and_DOS_metadata_equivalence=False)
   if name=='FindFirstFileA':
    pattern=self.cstring(a[0]);item['pattern']=pattern.decode('gbk')
    if pattern.lower()!=b'save\\*.dat'or pc!=0x43bbcd:raise Boundary('Directory_search_outside_reviewed_profiles46',**item)
    self.require_region(a[1],320,u.UC_PROT_WRITE,'WIN32_FIND_DATAA32');fd,records=enumerate_profiles()
    if not records:os.close(fd);self.last_error=2;value=0xffffffff
    else:
     value=self.find_next_handle46;self.find_next_handle46+=0x100;self._find_fds46[value]=fd;self._find_sessions46[value]=dict(pattern=pattern,records=records,index=1);self.uc.mem_write(a[1],records[0]['WIN32_FIND_DATAA']);item['actual_name']=records[0]['name']
   else:
    session=self._find_sessions46.get(a[0])
    if session is None or a[0]not in self._find_fds46:raise Boundary('Find_handle_unknown_or_closed46',**item)
    if name=='FindClose':os.close(self._find_fds46.pop(a[0]));del self._find_sessions46[a[0]];value=1
    else:
     self.require_region(a[1],320,u.UC_PROT_WRITE,'WIN32_FIND_DATAA32');i=session['index']
     if i>=len(session['records']):self.last_error=18;value=0;item['output_untouched_on_EOF']=True
     else:record=session['records'][i];self.uc.mem_write(a[1],record['WIN32_FIND_DATAA']);session['index']+=1;value=1;item['actual_name']=record['name']
   item.update(return_value=value,last_error=self.last_error);self.find_calls46.append(item);self.ret(value,n);return
  if dll.lower()=='kernel32.dll'and name in ['CreateFileA','GetFullPathNameA']:
   n=7 if name=='CreateFileA'else 4;a=self.args(n);path=self.cstring(a[0]);self.bind_resource(path);canonical=self.startup_aliases.get(path.lower())
   if canonical and canonical.lower().startswith(b'c:\\emperor\\save\\'):
    key=canonical.lower();self._startup_snapshot_identity[key]=self._startup_identity[key];pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
    if name=='CreateFileA':
     if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('Profile_file_outside_readonly_scope46',args=a,path=path.decode('gbk'),guest_return_pc=hex(pc))
     if a[3]:
      self.require_region(a[3],12,u.UC_PROT_READ,'SECURITY_ATTRIBUTES32');sa=list(struct.unpack('<3I',bytes(self.uc.mem_read(a[3],12))))
      if sa[0]!=12 or sa[1]!=0 or sa[2]not in [0,1]:raise Boundary('Profile_file_security_attributes_unreviewed46',security_attributes=sa)
     value=self.next_handle;self.next_handle+=1;self.files[value]=dict(path=key,payload=self.startup_files[key][1],position=0)
    else:
     if a[1]<=len(canonical):self.ret(len(canonical)+1,4);return
     self.require_region(a[2],len(canonical)+1,u.UC_PROT_WRITE,'profile full path');self.uc.mem_write(a[2],canonical+b'\0')
     if a[3]:self.word(a[3],a[2]+canonical.rfind(b'\\')+1)
     value=len(canonical)
    self.profile_bindings46.append(dict(name=name,path=path.decode('gbk'),return_PC=hex(pc),return_value=value,readonly=True));self.ret(value,n);return
  return super().api(dll,name)
 def dispose(self):
  for fd in self._find_fds46.values():os.close(fd)
  self._find_fds46={};super().dispose()

def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='menu46');p.add_argument('--tag',default='profiles46');p.add_argument('--seconds',type=int,default=14);p.add_argument('--click',nargs=2,type=int);p.add_argument('--restore-only',action='store_true');p.add_argument('--capture',action='store_true');a=p.parse_args();assert 1<=a.seconds<=18;start=time.monotonic();r=MenuRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=100000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  before=r.window.diagnostic();before_pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP));result=None
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
  else:
   assert r.registry.start()==r.audio.start()==0;r.configure_window_callback();r.configure_dd()
   if a.click:r.inject_click(a.click)
   result=r.thread_run(a.seconds,max_steps=200000);after=r.window.diagnostic();assert after['key_window']==after['NSApp_isActive']==after['activation_requests']==0 and after['frontmost_pid']!=after['current_pid'];save_state(r,a.tag,'original menu continuation with owned window NSEvent FIFO; no guest gameplay fields injected',m['milestones'])
  out=dict(source=a.resume,state=a.tag,result=result,current_tid=r.current_tid,current_CPU_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),before_PC=before_pc,elapsed_seconds=time.monotonic()-start,test_input_calls=r.test_input_calls45,original_WndProc_entries=r.menu_events45,route=r.pump.route_diagnostic(),input_calls=r.input_calls43,pump_calls=r.pump_calls42,startup_counts=r.startup_counts43,startup_events=r.startup_events43,boot_events=r.boot_events,stream_calls=r.stream_calls44,stream_stats=r.streams44.stats(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),natural_state=dict(screen_mode=bytes(r.uc.mem_read(17686597,1))[0],gate=r.readword(0x88ec38),city_phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970)),original_code_patched=False,game_fields_injected=False,global_events_posted=False,global_cursor_moved=False,files_uploaded=False,original_DLL_exports_executed=False,original_city_update_proven=False,all_memory_CPU_four_threads_native_blobs_restored=bool(a.restore_only))
  out['miles_file_calls']=r.miles_file_calls45;out['miles_file_allocations']=r.miles_file_allocations45;out['PCM_calls']=r.PCM_calls46;out['PCM_stats']={h:r.sample_pool.stats(h)[1] for h in r.PCM_handles46 if r.sample_pool.stats(h)[0]==0};out['file_releases']=r.file_releases46;out['find_calls']=r.find_calls46;out['profile_bindings']=r.profile_bindings46
  if a.capture:
   (R/f'evidence/stage46-{a.tag}.json').write_text(json.dumps(out,indent=2))
   primary=[o for o in r.dd_objects.values()if o['kind']==2 and r.dd.info(o['id'])['caps']&0x200];assert len(primary)==1;g=r.verify_native_geometry();o=primary[0];body=r.dd.pixels(o['id']);raw=ctypes.create_string_buffer(g['client_w']*g['client_h']*4);stats=(ctypes.c_uint32*12)();assert r.dd.fn['present'](r.dd.state,o['id'],stats,raw,len(raw),None);assert r.dd.pixels(o['id'])==body
   info=(ctypes.c_uint32*10)();v=(ctypes.c_uint32*6)();assert r.visible44(r.window.state,v);assert r.dd.fn['capture_prepare'](r.dd.state,info);diag=r.window.diagnostic();assert info[1]==info[5]==1 and info[6]==diag['current_pid']and diag['key_window']==diag['NSApp_isActive']==diag['activation_requests']==0 and diag['frontmost_pid']!=diag['current_pid'];png=R/f'evidence/stage46-{a.tag}-window.png';q=subprocess.run(['/usr/sbin/screencapture','-x','-l',str(info[0]),str(png)],capture_output=True,timeout=10);assert q.returncode==0;out['window_capture']=dict(path=str(png),sha256=hashlib.sha256(png.read_bytes()).hexdigest(),native_capture=list(info),native_diagnostic=diag,restored_framebuffer_unchanged=True,host_restore_presentation_not_original_Blt=True)
  dest=R/f'evidence/stage46-{a.tag if not a.restore_only else "restore"}.json';dest.write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,state=a.tag,result=result,route=out['route'],natural=out['natural_state'],elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
