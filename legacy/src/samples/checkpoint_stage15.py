#!/usr/bin/env python3
"""Actual original continuation with real live native metadata/resource ABI."""
import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage14 import MessageRuntime
from checkpoint_stage12 import x,u
from stage7_runtime import Boundary,SCRATCH
from stage12_state import save_state,restore_state
from stage15_gui import NativeGUI
class GUIRuntime(MessageRuntime):
 extra_snapshot_fields=MessageRuntime.extra_snapshot_fields+['gui_blob','gui_calls','gui_cache_reads','gui_phase','oem_code_page']
 def __init__(self,save):
  super().__init__(save);self.gui=NativeGUI();assert not self.gui.fixture;self.gui_calls=[];self.gui_cache_reads=[];self.gui_phase='CRT initializer table';self.oem_code_page=None
  record_path=ROOT/'evidence/stage15-oem-config.json'
  if record_path.exists():
   record=json.loads(record_path.read_text());raw=(ROOT/'evidence/stage10-OEM15-output.bin').read_bytes();assert hashlib.sha256(raw).hexdigest()==record['output_sha256'];assert record['OEMCP']==int.from_bytes(raw[8:12],'little');self.oem_code_page=record['OEMCP']
  self.uc.hook_add(u.UC_HOOK_MEM_READ,self.cache_read,begin=0x1e580a0,end=0x1e5810b)
 @property
 def gui_blob(self):return self.gui.export()
 @gui_blob.setter
 def gui_blob(self,body):self.gui.restore(body)
 def cache_read(self,uc,access,address,size,value,data):
  if len(self.gui_cache_reads)<256:self.gui_cache_reads.append(dict(phase=self.gui_phase,pc=hex(uc.reg_read(x.UC_X86_REG_EIP)),address=hex(address),size=size,bytes=bytes(uc.mem_read(address,size)).hex()))
 def api(self,dll,name):
  signatures={'GetSystemMetrics':(1,1),'GetDC':(2,1),'GetDeviceCaps':(3,2),'ReleaseDC':(4,2),'GetSysColor':(5,1),'GetSysColorBrush':(6,1),'LoadCursorA':(7,2)}
  if (dll.lower()=='user32.dll' and name in signatures) or (dll.lower()=='gdi32.dll' and name=='GetDeviceCaps'):
   kind,argc=signatures[name];a=self.args(argc);item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),native_live_profile=self.gui.profile(),fixture=False,backend='task-authored native AppKit/CoreGraphics metadata/resource core',input_last_error=self.last_error)
   self.gui_calls.append(item);self.api_trace.append(item);status,value,error=self.gui.invoke(kind,*a,last_error=self.last_error)
   if not status:raise Boundary('native_GUI_scope_or_changed_display_refused',**item)
   self.last_error=error;item.update(return_value=hex(value),last_error=error)
   if name in ['GetDC','GetSysColorBrush','LoadCursorA']:item['native_object_kind']=self.gui.object_kind_c(self.gui.state,value)
   if name=='LoadCursorA' and a[1]==32514:item['presentation_note']='retained real NSCursor, task-authored hourglass fallback; not exact Windows pixels, not set on screen'
   self.ret(value,argc)
  elif dll.lower()=='kernel32.dll' and name=='GetOEMCP':
   if self.oem_code_page is None:raise Boundary('unmeasured_guest_OEM_locale')
   self.api_trace.append(dict(dll=dll,name=name,args=[],return_value=self.oem_code_page,last_error=self.last_error,guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),backend='fixed measured virtual Windows NLS configuration; not macOS display backend',native_macos_locale_claimed=False));self.ret(self.oem_code_page,0)
  else:super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--tag',default='gui');p.add_argument('--resume-state',default='process');a=p.parse_args();started=time.monotonic();r=GUIRuntime('ST9BASE');previous=restore_state(r,a.resume_state);phase=previous['phase'];crt_already_returned=phase=='city load root';r.gui_phase=phase;milestones=previous['milestones'];before=len(r.gui_calls);s=r.segment()
 while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
 milestones.append(dict(name=phase,result=s));crt_newly_returned=s['status']=='returned' and phase=='CRT initializer table';crt_returned=crt_already_returned or crt_newly_returned
 if crt_newly_returned:
  phase='city load root';r.gui_phase=phase;s=r.segment(0x534a30,(SCRATCH,1));milestones.append(dict(name='city load root entered',result=s))
  while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
  milestones.append(dict(name='city load root final',result=s))
 save_state(r,a.tag,phase,milestones);slot=int(r.initializer_calls[-1]['slot'],16);table=json.loads((ROOT/'evidence/stage14-CRT-tables.json').read_text())['tables'][1]['rows'];done=1505 if crt_returned else sum(t['target']!='0x0' and int(t['slot'],16)<slot for t in table)
 result=dict(elapsed_seconds=round(time.monotonic()-started,3),source_checkpoint=a.resume_state,final_status=s['status'],final_boundary=s.get('boundary'),final_registers=s['registers'],complete_CRT_initializer_returned=crt_returned,original_city_root_entered=crt_returned,total_nonempty_initializers=1511,completed_nonempty_initializers=done+6,remaining_nonempty_initializers=1505-done,current_table_slot=hex(slot) if not crt_returned else None,new_GUI_calls=r.gui_calls[before:],gui_cache_reads=r.gui_cache_reads,gui_snapshot_sha256=hashlib.sha256(r.gui_blob).hexdigest(),gui_snapshot_bytes=len(r.gui_blob),native_live_profile=r.gui.profile(),fixture_not_used_in_original_continuation=True,no_original_instruction_patched=True,full_city_loaded=False,native_city_passed=False,backend='Unicorn original i386 instructions with native arm64 platform helpers; not whole-city static translation',natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8),state_directory=str(ROOT/'runs'/('stage12-state-'+a.tag)))
 (ROOT/f'evidence/stage15-checkpoint-{a.tag}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['new_GUI_calls','gui_cache_reads']}));r.gui.dispose();r.messages.dispose();r.events.dispose()
if __name__=='__main__':main()
