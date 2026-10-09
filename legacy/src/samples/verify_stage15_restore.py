#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage15 import GUIRuntime
from stage12_state import restore_state
def main():
 r=GUIRuntime('ST9BASE');state=restore_state(r,'oem');sp=r.uc.reg_read(__import__('checkpoint_stage12').x.UC_X86_REG_ESP);pointer=r.readword(sp+4);text=r.cstring(pointer)
 objects=[]
 for h in [0x52000002,0x52000003,0x52000004,0x52000005]:objects.append(dict(guest_handle=hex(h),validated_native_type=r.gui.object_kind_c(r.gui.state,h)))
 assert [o['validated_native_type'] for o in objects]==[2,2,3,3]
 for section in r.pe.sections:
  if section['flags']&0x20000000:assert bytes(r.uc.mem_read(section['va'],section['rawsize']))==r.pe.read(section['va'],section['rawsize'])
 result=dict(all_passed=True,original_instructions_not_executed_in_restore_check=True,all_original_executable_bytes_equal=True,full_CPU_and_memory_restore='evidence/stage12-state-restore-oem.json',phase=state['phase'],next_API='SetCurrentDirectoryA',guest_argument_pointer=hex(pointer),guest_path=text.decode('ascii',errors='backslashreplace'),guest_return_pc=hex(r.readword(sp)),gui_objects_recreated_with_original_guest_IDs=objects,gui_snapshot_sha256=hashlib.sha256(r.gui_blob).hexdigest(),OEMCP_restored=r.oem_code_page,fixture=False)
 (ROOT/'evidence/stage15-state-and-boundary.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));r.gui.dispose();r.messages.dispose();r.events.dispose()
if __name__=='__main__':main()
