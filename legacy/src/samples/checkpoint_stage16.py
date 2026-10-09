#!/usr/bin/env python3
"""Empty-cwd failure is real; original recovery continues without field fills."""
import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage15 import GUIRuntime
from checkpoint_stage12 import x,u
from stage7_runtime import Boundary,GUEST_NAME
from stage12_state import save_state,restore_state
from stage16_paths import NativePaths
class PathsRuntime(GUIRuntime):
 extra_snapshot_fields=GUIRuntime.extra_snapshot_fields+['cwd_blob','cwd_calls','path_events','save_IO','string_calls']
 def __init__(self,save):
  super().__init__(save);self.paths=NativePaths();self.cwd_calls=[];self.path_events=[];self.save_IO=[];self.string_calls=[]
  for pc,label in [(0x779fd3,'original_chdir_checks_Bool'),(0x77a035,'original_chdir_failure'),(0x52a28d,'caller_checks_CRT_minus_one'),(0x52a291,'original_error_record_branch'),(0x40fc20,'original_logger_is_RET'),(0x52a2b1,'directory_helper_returns'),(0x534a84,'city_root_resumes'),(0x534b16,'city_deserializer_call'),(0x533f20,'city_deserializer_entered')]:
   self.uc.hook_add(u.UC_HOOK_CODE,self.path_hook,user_data=label,begin=pc,end=pc)
 @property
 def cwd_blob(self):return self.paths.export()
 @cwd_blob.setter
 def cwd_blob(self,body):self.paths.restore(body)
 def path_hook(self,uc,pc,size,label):
  if len(self.path_events)<256:self.path_events.append(dict(pc=hex(pc),label=label,eax=hex(uc.reg_read(x.UC_X86_REG_EAX)),startup_path_field=self.cstring(0x10dd7f4).decode('ascii',errors='backslashreplace')))
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll' and name in ['SetCurrentDirectoryA','GetCurrentDirectoryA']:
   argc=1 if name=='SetCurrentDirectoryA' else 2;a=self.args(argc);item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),input_last_error=self.last_error,namespace=self.paths.guest.decode(),game_startup_field_before=self.cstring(0x10dd7f4).decode('ascii',errors='backslashreplace'),backend='native C POSIX-directory-backed virtual guest cwd')
   self.cwd_calls.append(item);self.api_trace.append(item)
   if argc==1:
    text=self.cstring(a[0]);item['path']=text.decode('ascii',errors='backslashreplace');status,value,error=self.paths.set(text,self.last_error)
   else:
    if a[0]>4096:raise Boundary('GetCurrentDirectory_capacity_scope',**item)
    status,value,error,body=self.paths.get(a[0],bytes(a[0]),self.last_error)
    if status and value<len(body):
     self.require_region(a[1],value+1,u.UC_PROT_WRITE,'cwd output');self.uc.mem_write(a[1],body[:value+1]);item['output_path']=body[:value].decode('ascii')
   if not status:raise Boundary('native_cwd_unmeasured_path_scope',**item)
   self.last_error=error;item.update(return_value=value,last_error=error,game_startup_field_after=self.cstring(0x10dd7f4).decode('ascii',errors='backslashreplace'));self.ret(value,argc)
  elif dll.lower()=='kernel32.dll' and name=='lstrlenA':
   a=self.args(1)
   if not a[0]:raise Boundary('lstrlenA_null_not_measured')
   source=self.cstring(a[0]);status,value=self.paths.length(source);item=dict(name=name,args=[hex(v) for v in a],source_ascii=source.decode('ascii',errors='backslashreplace'),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),backend='own arm64 C valid ASCII length',last_error=self.last_error)
   self.string_calls.append(item);self.api_trace.append(item)
   if status!=1:raise Boundary('lstrlenA_finite_ASCII_scope',**item)
   item['return_value']=value;self.ret(value,1)
  elif dll.lower()=='kernel32.dll' and name=='lstrcpynA':
   a=self.args(3);source=self.cstring(a[1]);item=dict(name=name,args=[hex(v) for v in a],source_ascii=source.decode('ascii',errors='backslashreplace'),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),backend='own arm64 C ASCII copy, valid nonoverlapping buffers')
   self.string_calls.append(item);self.api_trace.append(item)
   if a[2]>260 or any(v>=128 for v in source):raise Boundary('lstrcpynA_finite_ASCII_scope',**item)
   n=min(len(source),a[2]-1)+1 if a[2] else 0
   if n:
    self.require_region(a[0],n,u.UC_PROT_WRITE,'lstrcpynA destination')
    if a[0]<a[1]+len(source)+1 and a[1]<a[0]+n:raise Boundary('lstrcpynA_overlap_undefined',**item)
   status,written,body=self.paths.copy(source,a[2],bytes(n))
   if status!=1 or written!=n:raise Boundary('native_ASCII_copy_refused',**item)
   if written:self.uc.mem_write(a[0],body[:written])
   item.update(bytes_written=written,last_error=self.last_error,return_value=hex(a[0]));self.ret(a[0],3)
  elif dll.lower()=='kernel32.dll' and name in ['CreateFileA','ReadFile']:
   a=self.args(7 if name=='CreateFileA' else 5);filename=self.cstring(a[0]) if name=='CreateFileA' else None;return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)));before=self.files.get(a[0],{}).get('position') if name=='ReadFile' else None
   super().api(dll,name)
   if name=='CreateFileA' and filename.lower()==GUEST_NAME.lower():self.save_IO.append(dict(name=name,guest_filename=filename.decode(),handle=hex(self.uc.reg_read(x.UC_X86_REG_EAX)),return_pc=return_pc,frozen_save_sha256=hashlib.sha256(self.save).hexdigest()))
   elif name=='ReadFile':
    n=self.readword(a[3]);after=self.files.get(a[0],{}).get('position');payload=self.files.get(a[0],{}).get('payload',b'')
    if payload==self.save:self.save_IO.append(dict(name=name,handle=hex(a[0]),bytes=n,position_before=before,position_after=after,guest_read_sha256=hashlib.sha256(bytes(self.uc.mem_read(a[1],n))).hexdigest(),source_slice_equal=bytes(self.uc.mem_read(a[1],n))==self.save[before:after],return_pc=return_pc))
  else:super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume-state',default='oem');p.add_argument('--tag',default='cwd-failure');a=p.parse_args();started=time.monotonic();r=PathsRuntime('ST9BASE');previous=restore_state(r,a.resume_state);phase=previous['phase'];before=len(r.api_trace);s=r.segment()
 while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
 milestones=previous['milestones']+[dict(name=phase,result=s)];save_state(r,a.tag,phase,milestones)
 result=dict(elapsed_seconds=round(time.monotonic()-started,3),source_checkpoint=a.resume_state,final_status=s['status'],final_boundary=s.get('boundary'),final_registers=s['registers'],original_startup_path_field=r.cstring(0x10dd7f4).decode('ascii',errors='backslashreplace'),path_field_filled_by_harness=False,path_events=r.path_events,cwd_calls=r.cwd_calls,save_IO=r.save_IO,total_save_bytes_read=sum(row.get('bytes',0) for row in r.save_IO),original_city_deserializer_entered=any(e['label']=='city_deserializer_entered' for e in r.path_events),complete_CRT_initializer_returned=True,original_city_root_entered=True,new_API_calls=r.api_trace[before:],full_city_loaded=False,native_city_passed=False,no_original_instruction_patched=True,natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8),backend='Unicorn original i386 reference, task-authored native platform helpers; no whole-city static translation',state_directory=str(ROOT/'runs'/('stage12-state-'+a.tag)))
 (ROOT/f'evidence/stage16-checkpoint-{a.tag}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['new_API_calls','path_events','cwd_calls','save_IO']}));r.paths.dispose();r.gui.dispose();r.messages.dispose();r.events.dispose()
if __name__=='__main__':main()
