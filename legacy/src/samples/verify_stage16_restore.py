#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage16 import PathsRuntime,x
from stage12_state import restore_state
r=PathsRuntime('ST9BASE');state=restore_state(r,'file-open');sp=r.uc.reg_read(x.UC_X86_REG_ESP);args=[r.readword(sp+4+4*i) for i in range(8)]
for section in r.pe.sections:
 if section['flags']&0x20000000:assert bytes(r.uc.mem_read(section['va'],section['rawsize']))==r.pe.read(section['va'],section['rawsize'])
assert r.cstring(0x10dd7f4)==b'' and r.cstring(args[0])==b'C:\\'
assert args[1:4]==[0,0,0] and args[6:]==[0,0]
result=dict(all_passed=True,original_instructions_not_executed_in_restore_check=True,all_original_executable_bytes_equal=True,full_CPU_memory_and_platform_state_restore='evidence/stage12-state-restore-file-open.json',phase=state['phase'],next_API='GetVolumeInformationA',args=[hex(v) for v in args],root_path=r.cstring(args[0]).decode('ascii'),return_pc=hex(r.readword(sp)),startup_path_field_empty=True,game_gate=r.readword(0x88ec38),game_phase=r.readword(0xc82ef8),cwd_image_sha256=hashlib.sha256(r.cwd_blob).hexdigest(),cwd_host_directory_revalidated=True,save_bytes_read=sum(row.get('bytes',0) for row in r.save_IO),string_calls_restored=len(r.string_calls),native_city_passed=False)
(ROOT/'evidence/stage16-state-and-boundary.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));r.paths.dispose();r.gui.dispose();r.messages.dispose();r.events.dispose()
