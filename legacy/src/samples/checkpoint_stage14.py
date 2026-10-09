#!/usr/bin/env python3
"""Same original continuation, native message registry, actual CRT slot trace."""
import argparse,hashlib,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage13 import EventRuntime
from checkpoint_stage12 import x,u
from stage7_runtime import Boundary,SCRATCH
from stage12_state import save_state,restore_state
from stage14_messages import NativeMessages
class MessageRuntime(EventRuntime):
 extra_snapshot_fields=EventRuntime.extra_snapshot_fields+['messages_blob','message_calls','initializer_calls']
 def __init__(self,save):
  super().__init__(save);self.messages=NativeMessages();self.message_calls=[];self.initializer_calls=[]
  self.uc.hook_add(u.UC_HOOK_CODE,self.initializer_hook,begin=0x76a097,end=0x76a099)
 @property
 def messages_blob(self):return self.messages.export()
 @messages_blob.setter
 def messages_blob(self,body):self.messages.restore(body)
 def initializer_hook(self,uc,pc,size,data):
  if pc==0x76a097:self.initializer_calls.append(dict(slot=hex(uc.reg_read(x.UC_X86_REG_ESI)),target=hex(uc.reg_read(x.UC_X86_REG_EAX)),returned=False))
  elif pc==0x76a099:
   slot=hex(uc.reg_read(x.UC_X86_REG_ESI))
   if self.initializer_calls and self.initializer_calls[-1]['slot']==slot:self.initializer_calls[-1]['returned']=True
   else:self.initializer_calls.append(dict(slot=slot,returned=True,resumed_inside_initializer=True))
 def api(self,dll,name):
  if dll.lower()=='user32.dll' and name=='RegisterWindowMessageA':
   a=self.args(1);text=self.cstring(a[0]);item=dict(dll=dll,name=name,text=text.decode('ascii',errors='backslashreplace'),guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),input_last_error=self.last_error,scope='own isolated session',captured_Wine_numeric_ID_replayed=False)
   self.message_calls.append(item);self.api_trace.append(item);value,error=self.messages.register(text,self.last_error)
   if value==0xfffffffe:raise Boundary('native_message_registry_finite_scope_refused',**item)
   self.last_error=error;item.update(native_session_ID=value,last_error=error);self.ret(value,1)
  elif dll.lower()=='kernel32.dll' and name=='GetProcessVersion':
   a=self.args(1)
   if a[0] not in [0,2,0xffffffff]:raise Boundary('unmeasured_other_guest_process_version',pid=a[0])
   value,error=self.messages.process_version(self.pe.data,a[0],2,self.last_error);self.last_error=error
   self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),return_value=value,last_error=error,backend='native C reads actual original PE subsystem version; no canned OS version'))
   self.ret(value,1)
  else:super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--tag',default='messages');p.add_argument('--resume-state',default='disk');a=p.parse_args();started=time.monotonic();r=MessageRuntime('ST9BASE');previous=restore_state(r,a.resume_state);phase=previous['phase'];milestones=previous['milestones'];initial_esi=r.uc.reg_read(x.UC_X86_REG_ESI);s=r.segment()
 while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
 milestones.append(dict(name=phase,result=s))
 crt_returned=s['status']=='returned' and phase=='CRT initializer table'
 if crt_returned:
  phase='city load root';s=r.segment(0x534a30,(SCRATCH,1));milestones.append(dict(name='city load root entered',result=s))
  while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
  milestones.append(dict(name='city load root final',result=s))
 save_state(r,a.tag,phase,milestones)
 tables=json.loads((ROOT/'evidence/stage14-CRT-tables.json').read_text())['tables'];esi=r.uc.reg_read(x.UC_X86_REG_ESI)
 table_progress=[]
 # ESI is callee-saved by the walker but may be used inside a constructor.
 # Trace the actual CALL slot rather than confuse its object's ESI with it.
 walker_slot=int(r.initializer_calls[-1]['slot'],16) if r.initializer_calls else initial_esi
 for t in tables:
  rows=t['rows'];finished=t['name']=='C_initializers' or crt_returned
  done=len([v for v in rows if v['target']!='0x0' and (finished or int(v['slot'],16)<walker_slot)]) if (finished or 0x814000<=walker_slot<0x815788) else None
  table_progress.append(dict(name=t['name'],slots=t['slots'],nonnull=t['nonnull'],completed_by_original_walker_position=done,current_slot=hex(walker_slot) if not finished and 0x814000<=walker_slot<0x815788 else None,remaining_including_current=t['nonnull']-done if done is not None else None))
 result=dict(elapsed_seconds=round(time.monotonic()-started,3),source_checkpoint=a.resume_state,final_status=s['status'],final_boundary=s.get('boundary'),final_registers=s['registers'],complete_CRT_initializer_returned=crt_returned,original_city_root_entered=crt_returned,full_city_loaded=False,native_city_passed=False,GUID_count=len(r.guid_calls),initial_ESI=hex(initial_esi),table_progress=table_progress,initializer_calls=r.initializer_calls,message_calls=r.message_calls,message_snapshot_sha256=hashlib.sha256(r.messages_blob).hexdigest(),message_snapshot_size=len(r.messages_blob),natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8),original_instruction_bytes_patched=False,backend='Unicorn original i386 instructions; native C platform helpers only',state_directory=str(ROOT/'runs'/('stage12-state-'+a.tag)))
 (ROOT/f'evidence/stage14-checkpoint-{a.tag}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));r.messages.dispose();r.events.dispose()
if __name__=='__main__':main()
