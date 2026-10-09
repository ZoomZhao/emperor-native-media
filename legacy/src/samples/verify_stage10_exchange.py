#!/usr/bin/env python3
"""SDK-defined atomic exchange sequence, pointer width and guards verified."""
import json,sys,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH,SP,x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='InterlockedExchange');checks=[]
    initial=0x11223344;r.uc.mem_write(SCRATCH,struct.pack('<3I',0xa5a5a5a5,initial,0xa5a5a5a5))
    for value in [0,1,0xffffffff,0x80000000,0x7fffffff,0x11223344]:
        call=r.call(thunk,(SCRATCH+4,value));assert call['status']=='returned' and call['eax']==initial
        assert bytes(r.uc.mem_read(SCRATCH,12))==struct.pack('<3I',0xa5a5a5a5,value,0xa5a5a5a5)
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP;checks.append(dict(previous=hex(initial),new_value=hex(value),atomic_single_thread_transition_and_guards_verified=True));initial=value
    result=dict(all_passed=True,checks=checks,reference='Microsoft InterlockedExchange contract; no additional Wine numeric oracle needed',execution_backend='Unicorn i386 shim ABI',multithread_atomicity_tested=False)
    (ROOT/'evidence/stage10-exchange-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
