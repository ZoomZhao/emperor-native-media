"""Own synthetic hook-state test. No runtime constructor or engine execution."""
import json
from pathlib import Path
from checkpoint_startup51 import MenuRuntime,x,Boundary
R=Path(__file__).resolve().parents[1]
class OwnRegisters:
 def __init__(self):self.values={x.UC_X86_REG_ESP:0x1004,x.UC_X86_REG_EAX:0}
 def reg_read(self,r):return self.values[r]
r=MenuRuntime.__new__(MenuRuntime);r.uc=OwnRegisters();r.current_tid=1;r.city_observations49=[];r._city_frames49=[];r.city_values49=lambda:dict(own_fixture=True)
frame=dict(label='city_load',tid=1,entry_PC=0x534a30,return_PC=0x401aa8,owner_SP=0x1000,before={'own_fixture':True})
r._city_frames49=[dict(frame)];r.uc.values[x.UC_X86_REG_ESP]=0x1000
MenuRuntime.city_return49(r,r.uc,0x401aa8,1,None);assert r._city_frames49 and not r.city_observations49 and not hasattr(r,'original_loader_stop51')
r.uc.values[x.UC_X86_REG_ESP]=0x1004
try:MenuRuntime.city_return49(r,r.uc,0x401aa8,1,None)
except Boundary as e:
 assert e.details==dict(kind='Original_loader_return_stop51',EAX=0,return_PC='0x401aa8',caller_instruction_executed=False)
else:raise AssertionError('successful root return must stop')
assert not r._city_frames49 and r.original_loader_stop51['EAX']==0 and r.original_loader_stop51['original_cdecl_return_stack_verified']
r.city_observations49=[];r._city_frames49=[dict(frame)];del r.original_loader_stop51;r.uc.values[x.UC_X86_REG_EAX]=1
try:MenuRuntime.city_return49(r,r.uc,0x401aa8,1,None)
except Boundary as e:assert e.details['EAX']==1
else:raise AssertionError('natural failed root return must also stop')
out=dict(passed=True,synthetic_cases=3,original_EXE_instructions_executed=False,emulator_instance_created=False,runtime_constructor_called=False,wrong_stack_does_not_stop=True,natural_success_and_failure_return_both_stop=True,caller_instruction_not_executed=True)
(R/'evidence/stage51-loader-stop-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
