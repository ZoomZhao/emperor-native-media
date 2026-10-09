#!/usr/bin/env python3
import hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from observe_stage18_fields import Observer,FIELDS,x
from stage12_state import restore_state
r=Observer('ST9BASE');m=restore_state(r,'fields')
for s in r.pe.sections:
 if s['flags']&0x20000000:assert bytes(r.uc.mem_read(s['va'],s['rawsize']))==r.pe.read(s['va'],s['rawsize'])
assert r.uc.reg_read(x.UC_X86_REG_EIP)==0x4081ba and r.readword(0x1e28348+4)==0
assert r.readword(0x7b813c+8*4)==0x5592b0 and r.readword(0x7b8188+8*4)==0x5593f0
assert r.readword(0x130f988)==28 and r.readword(0x130f970)==1633
out=dict(all_passed=True,complete_restore='evidence/stage12-state-restore-fields.json',all_original_executable_bytes_equal=True,original_city_root_returned=False,original_code_patched=False,source_state='fields',static_binding=dict(population=dict(class_name='cPopulationGoal',runtime_class='0x8590f0',creator='0x5591c0',constructor='0x559220',vtable='0x7b813c',slot8='0x5592b0',field='0x130f988'),treasury=dict(class_name='cTreasuryGoal',runtime_class='0x859108',creator='0x559320',constructor='0x559380',vtable='0x7b8188',slot8='0x5593f0',getter='0x591aa0',this='0x130f960',offset=16,field='0x130f970')),getter_probes=[],native_city_passed=False,game_save_restore_passed=False)
for name,pc,expected in [('population',0x5592b0,28),('treasury',0x5593f0,1633)]:
 before={a:bytes(r.uc.mem_read(a,n)) for a,(label,n) in FIELDS.items()};s=r.segment(pc,instruction_limit=100)
 assert s['status']=='returned' and r.uc.reg_read(x.UC_X86_REG_EAX)==expected and before=={a:bytes(r.uc.mem_read(a,n)) for a,(label,n) in FIELDS.items()}
 out['getter_probes'].append(dict(name=name,original_entry=hex(pc),returned=True,value=r.uc.reg_read(x.UC_X86_REG_EAX),contract_equal=True,observed_fields_unchanged=True,backend='original i386 getter executed via approved Unicorn; not native getter translation'))
(ROOT/'evidence/stage18-state-and-getters.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out));r.dispose()
