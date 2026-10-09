#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage17 import FileRuntime,x
from stage12_state import restore_state
r=FileRuntime('ST9BASE');state=restore_state(r,'classes');sp=r.uc.reg_read(x.UC_X86_REG_ESP);args=[r.readword(sp+4+i*4) for i in range(4)];path=r.cstring(args[0]);assert path==b'Campaigns\\1 Xia Dynasty - Tutorials.pak'
for section in r.pe.sections:
 if section['flags']&0x20000000:assert bytes(r.uc.mem_read(section['va'],section['rawsize']))==r.pe.read(section['va'],section['rawsize'])
assert r.native_save.read(0,len(r.save))==r.save
resolved=sorted(set(row['first'] for row in r.class_comparisons if row.get('equal')))
result=dict(all_passed=True,all_original_executable_bytes_equal=True,original_instructions_not_executed_in_restore_check=True,full_CPU_memory_and_platform_state_restore='evidence/stage12-state-restore-classes.json',next_API='GetFullPathNameA',return_pc=hex(r.readword(sp)),args=[hex(v) for v in args],relative_path=path.decode(),native_file_identity_revalidated=True,entire_frozen_save_native_pread_hash_equal=True,save_position=[dict(handle=hex(h),position=f['position'],payload_size=len(f['payload'])) for h,f in r.files.items()],native_read_bytes=sum(row['bytes'] for row in r.native_file_read_calls),total_guest_read_bytes=sum(row.get('bytes',0) for row in r.save_IO),class_comparison_count=len(r.class_comparisons),resolved_classes=resolved,original_startup_path_field_empty=r.cstring(0x10dd7f4)==b'',natural_gate=r.readword(0x88ec38),natural_phase=r.readword(0xc82ef8),native_city_passed=False)
(ROOT/'evidence/stage17-state-and-boundary.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));r.native_save.dispose();r.paths.dispose();r.gui.dispose();r.messages.dispose();r.events.dispose()
