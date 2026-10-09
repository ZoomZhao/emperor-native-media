# -*- coding: utf-8 -*-
"""Original caller takes capability-backed native 3D-unavailable fallback."""
from checkpoint_startup39 import SampleRuntime,argparse,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode
from stage7_runtime import Boundary
from stage40_registry import NativeRegistry
import os,errno,re
class ProviderRuntime(SampleRuntime):
 extra_snapshot_fields=SampleRuntime.extra_snapshot_fields+['registry_blob40','provider_calls40','provider_events40','missing_model_calls40']
 def __init__(self,save):
  super().__init__(save);self.registry=NativeRegistry(self.audio);self.provider_calls40=[];self.provider_events40=[];self.missing_model_calls40=[]
  for row in json.loads((R/'evidence/stage40-audio-text-catalog.readonly.json').read_text())['rows']+json.loads((R/'evidence/stage40-record-catalog.readonly.json').read_text())['rows']:
   rel=row['relative_path'].replace('/','\\').encode();self.resource_rows[rel.lower()]=row;self.resource_rows[(b'C:\\Emperor\\'+rel).lower()]=row
  for pc,label in [(0x763497,'3D_initializer_return'),(0x5c351c,'audio_3D_choice_complete'),(0x475751,'audio_startup_group_return'),(0x5c3535,'original_music_list_initializer_return'),(0x475756,'post_audio_477e40_return'),(0x4757e1,'post_audio_52b510_return')]:self.uc.hook_add(u.UC_HOOK_CODE,self.provider_hook,user_data=label,begin=pc,end=pc)
 @property
 def registry_blob40(self):return self.registry.snapshot()
 @registry_blob40.setter
 def registry_blob40(self,b):self.registry.restore(b)
 def volume_consumer_paths(self):return super().volume_consumer_paths()|{b'c:\\emperor\\save\\imperialrecord.imr'}
 def provider_hook(self,uc,address,size,label):
  self.provider_events40.append(dict(pc=hex(address),label=label,EAX=hex(uc.reg_read(x.UC_X86_REG_EAX)),original_3D_disabled_flag=self.readword(0x1108b40),original_selected_provider=self.readword(0x1156bc8+356),original_live_2D_driver=self.readword(0x1156bc8+376)))
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name=='CreateFileA':
   a=self.args(7);path=self.cstring(a[0],260)
   if re.fullmatch(rb'Model\\[A-Za-z0-9_]+\.txt',path,re.IGNORECASE)and path.lower()not in self.resource_rows:
    if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('Missing_model_readonly_open_parameters_unreviewed40',args=a)
    if a[3]and(self.readword(a[3])!=12 or self.readword(a[3]+4)!=0):raise Boundary('Missing_model_security_descriptor_unreviewed40')
    basename=path.split(b'\\')[-1].decode();paths=[R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/Model'/basename,Path('<LOCAL_MACHINE_PATH>]/Model')/basename];observed=[]
    for p in paths:
     if any(q.name.casefold()==basename.casefold()for q in p.parent.iterdir()):raise Boundary('Unreviewed_model_file_present40',path=str(p))
     try:fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
     except FileNotFoundError as e:
      assert e.errno==errno.ENOENT and p.parent.is_dir();observed.append(dict(path=str(p),errno=e.errno,exists=False))
     else:os.close(fd);raise Boundary('Unreviewed_model_file_opened_unexpectedly40')
    item=dict(name=name,path=path.decode(),args=a,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),actual_POSIX_readonly_failures=observed,return_value=0xffffffff,Win32_last_error=2,original_branch_patched=False);self.missing_model_calls40.append(item);self.api_trace.append(item);self.last_error=2;self.ret(0xffffffff,7);return
  if dll.lower()!='mss32.dll'or name!='_AIL_enumerate_3D_providers@12':return super().api(dll,name)
  a=self.args(3);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
  if not a[0]or not a[1]or not a[2]or len(set(a))!=3:raise Boundary('3D_enumeration_requires_current_distinct_guest_DWORD_outputs40',args=a)
  for ptr in a:self.require_region(ptr,4,u.UC_PROT_READ|u.UC_PROT_WRITE,'provider iterator/handle/Name guest32')
  before=[self.readword(p)for p in a];caps=self.registry.capabilities()
  if not caps['registered']or not caps['live_audio']:raise Boundary('Provider_registry_requires_actual_native_backend40')
  result,cursor,provider=self.registry.enumerate(b'MSS 3D audio services',before[0])
  if result<0:raise Boundary('Provider_registry_native_failure40',status=result)
  if result!=0:raise Boundary('3D_provider_Name_and_open_contract_not_implemented40')
  self.word(a[0],cursor);self.word(a[1],provider)
  after=[self.readword(p)for p in a];assert cursor==provider==0 and after[2]==before[2]
  item=dict(name=name,args=a,return_pc=hex(pc),return_value=result,before=before,after=after,backend='authored arm64 registry scans actually registered services',native_capabilities=caps,service='MSS 3D audio services',reason='Actual owned backend registers PCM queue service only; no Miles3D listener/sample/position callback table',original_DLL_registry_equivalence=False,original_MSS_or_M3D_export_executed=False,Name_output_unchanged_on_end=True)
  self.provider_calls40.append(item);self.api_trace.append(item);self.ret(result,3)
 def dispose(self):
  if self.registry.state:self.registry.dispose()
  super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='closed39');p.add_argument('--tag',default='fallback40');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=ProviderRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,registry_sha256=hashlib.sha256(r.registry_blob40).hexdigest(),provider_calls=r.provider_calls40,provider_events=r.provider_events40,missing_model_calls=r.missing_model_calls40,capabilities=r.registry.capabilities(),native_audio=r.audio.stats(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),native_diagnostic=r.window.diagnostic())
  else:
   assert struct.unpack('<64I',r.miles.export())[48]==1 and r.audio_state38['opened'];assert r.registry.start()==0
   assert r.audio.start()==0;r.configure_window_callback();r.configure_dd();s=r.thread_run(18,max_steps=200000);r.verify_native_geometry();save_state(r,a.tag,'original startup with authored actual-native-service registry; observed lawful 3D unavailable branch, no original branch patched',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,provider_calls=r.provider_calls40,provider_events=r.provider_events40,missing_model_calls=r.missing_model_calls40,native_capabilities=r.registry.capabilities(),native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),original_startup='Unicorn original unmodified i386 reference calls own arm64 platform adapters',original_code_patched=False,original_MSS_or_M3D_exports_executed=False,old_DLL_provider_registry_equivalent=False,main_menu_passed=False,city_logic_passed=False)
  (R/f'evidence/stage40-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,result=out.get('result'),events=out.get('provider_events'),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
