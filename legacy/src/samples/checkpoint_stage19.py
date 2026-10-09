#!/usr/bin/env python3
"""Continue pre-city CRT, then actual bounded WinMain; no skipped startup calls."""
import argparse,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage18 import PixelsRuntime,x,u
from stage7_runtime import SCRATCH
from stage12_state import restore_state,save_state
from stage7_runtime import Boundary
from stage19_profile import missing_profile,version_file_missing
class BootRuntime(PixelsRuntime):
 extra_snapshot_fields=PixelsRuntime.extra_snapshot_fields+['boot_events']
 def __init__(self,save):
  super().__init__(save);self.boot_events=[]
  for pc,label in [(0x4754f0,'WinMain_entry'),(0x475544,'startup_player_constructor_return'),(0x47554e,'configuration_initializer_call'),(0x47557b,'startup_path_selector_call'),(0x4755f4,'platform_window_initializer_call'),(0x475649,'graphics_and_data_initializer_call'),(0x47564e,'graphics_and_data_initializer_return'),(0x475d65,'startup_graphics_group_loader_call'),(0x476069,'remaining_graphics_group_loader_call'),(0x5cce70,'graphics_named_group_load'),(0x5cd1f0,'graphics_group_parse_entry')]:self.uc.hook_add(u.UC_HOOK_CODE,self.boot_hook,user_data=label,begin=pc,end=pc)
 def boot_hook(self,uc,pc,size,label):
  sp=uc.reg_read(x.UC_X86_REG_ESP);item=dict(pc=hex(pc),label=label,eax=hex(uc.reg_read(x.UC_X86_REG_EAX)),ecx=hex(uc.reg_read(x.UC_X86_REG_ECX)),return_or_top_word=hex(self.readword(sp)))
  if pc==0x5cce70:item.update(group_index=self.readword(uc.reg_read(x.UC_X86_REG_ECX)+2148252),name=self.cstring(self.readword(sp+4)).decode('ascii'))
  self.boot_events.append(item)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll' and name=='CreateFileA' and self.cstring(self.args(7)[0])==b'_wonapiversion.txt':
   a=self.args(7);item=dict(name=name,path='_wonapiversion.txt',args=[hex(v)for v in a],return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
   if a[1]!=0x80000000 or a[2]!=3 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('version_file_unmeasured_scope',**item)
   if a[3] and (self.readword(a[3])!=12 or self.readword(a[3]+4)!=0 or self.readword(a[3]+8) not in (0,1)):raise Boundary('version_file_security_scope',**item)
   version_file_missing();self.last_error=2;item.update(return_value='0xffffffff',last_error=2,backend='own native readonly openat confirms ENOENT in exact game cwd');self.api_trace.append(item);self.ret(0xffffffff,7)
  elif dll.lower()=='kernel32.dll' and name=='GetPrivateProfileStringA':
   a=self.args(6);section=self.cstring(a[0]) if a[0] else None;key=self.cstring(a[1]) if a[1] else None;filename=self.cstring(a[5]) if a[5] else None
   item=dict(name=name,section=section.decode('ascii') if section else None,key=key.decode('ascii') if key else None,filename=filename.decode('ascii') if filename else None,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
   if filename!=b'EOC.ini' or section!=b'Emperor of China' or not key or not all(v<128 for v in key) or not 1<=a[4]<=512:raise Boundary('profile_query_unmeasured_scope',**item)
   default=self.cstring(a[2],512) if a[2] else None;n,error,body,whole=missing_profile(default,a[4]);self.require_region(a[3],len(body),u.UC_PROT_WRITE,'profile query output');self.uc.mem_write(a[3],body);self.last_error=error
   item.update(return_value=n,last_error=error,output_hex=body.hex(),backend='own native readonly Windows-directory missing EOC.ini branch; same-Wine default/truncation/error2 verified');self.api_trace.append(item);self.ret(n,6)
  elif dll.lower()=='winmm.dll' and name=='timeGetTime':
   value,error,body=self.events.clock(7,0,self.last_error);assert not body;self.last_error=error
   item=dict(dll=dll,name=name,return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),return_value=value,last_error=error,backend='existing own arm64 CLOCK_MONOTONIC boot milliseconds, DWORD wrap; no multimedia resolution requests implemented',native_host_clock=True,original_Windows_boot_origin_not_replayed=True)
   self.clock_calls.append(item);self.api_trace.append(item);self.ret(value,0)
  else:super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume-state');p.add_argument('--tag',default='boot');p.add_argument('--crt-only',action='store_true');a=p.parse_args();start=time.monotonic();r=BootRuntime('ST9BASE');previous=restore_state(r,a.resume_state or 'gui');milestones=previous['milestones'];phase=previous['phase']
 if r.first_save_read is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 if a.resume_state:s=r.segment()
 else:
  s=r.segment()
  while s['status']=='budget_yield' and time.monotonic()-start<35:s=r.segment()
  milestones=milestones+[dict(name='complete pre-city CRT',result=s)];assert s['status']=='returned',s
  save_state(r,'prewinmain','complete pre-city CRT',milestones)
  if a.crt_only:phase='complete pre-city CRT'
  else:
   # WinMain ABI is derived from original CRT 0x768995. Empty command line is
   # an explicit self-authored invocation input, never a city-state field fill.
   r.uc.mem_write(SCRATCH+4096,b'\0');phase='bounded original WinMain';s=r.segment(0x4754f0,(r.pe.base,0,SCRATCH+4096,5))
 while s['status']=='budget_yield' and time.monotonic()-start<40:s=r.segment()
 save_state(r,a.tag,phase,milestones+[dict(name=phase,result=s)])
 result=dict(elapsed_seconds=round(time.monotonic()-start,3),source_state=a.resume_state or 'gui',phase=phase,final_result=s,boot_events=r.boot_events,city_root_returned=False,native_city_passed=False,original_executable_patched=False,skipped_WinMain_calls=False,game_fields_filled=False,natural_gate=r.readword(0x88ec38),natural_phase=r.readword(0xc82ef8),population=r.readword(0x130f988),treasury=r.readword(0x130f970),startup_cwd=r.cstring(0x10dd7f4).decode('ascii'),state=a.tag)
 (ROOT/f'evidence/stage19-checkpoint-{a.tag}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='final_result'},ensure_ascii=False));print(json.dumps(dict(status=s['status'],boundary=s.get('boundary'),registers=s['registers'])));r.dispose()
if __name__=='__main__':main()
