#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage19 import BootRuntime,x
from stage12_state import restore_state
r=BootRuntime('ST9BASE');m=restore_state(r,'prewinmain');assert r.uc.reg_read(x.UC_X86_REG_EIP)==0x62000000;assert r.first_save_read is None
for s in r.pe.sections:
 if s['flags']&0x20000000:assert bytes(r.uc.mem_read(s['va'],s['rawsize']))==r.pe.read(s['va'],s['rawsize'])
groups=[]
for i in range(41):
 record=0x1c42130+4096+i*52292;values={str(offset):r.readword(record+offset)for offset in [0,4,52268,52272,52276,52280]};assert not any(values.values());groups.append(dict(index=i,record=hex(record),values=values))
out=dict(all_passed=True,entire_CPU_and_memory_restore='evidence/stage12-state-restore-prewinmain.json',original_code_equal=True,natural_CRT_return=True,original_city_not_loaded=True,graphics_groups=groups,graphics_counter=r.readword(0x1c42130+2148252),population=r.readword(0x130f988),treasury=r.readword(0x130f970),fields_state_equivalence_proven=False,game_fields_filled=False,no_original_code_executed_by_this_verification=True)
assert out['graphics_counter']==0 and out['population']==out['treasury']==0;(ROOT/'evidence/stage19-prewinmain-verified.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k!='graphics_groups'}));r.dispose()
