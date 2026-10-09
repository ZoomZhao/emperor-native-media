#!/usr/bin/env python3
"""SDK-defined fixed/zero-initialized LocalAlloc plus correct lifecycle."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import THUNKS,SP,x

def main():
    r=LoaderRuntime();thunks={sym:p for p,(dll,sym) in r.imports.items() if dll.lower()=='kernel32.dll'};checks=[]
    for name in ['LocalAlloc','LocalFree','LocalSize','LocalLock','LocalUnlock']:
        if name not in thunks:
            address=THUNKS+len(r.imports)*16;r.imports[address]=('KERNEL32.dll',name);r.uc.mem_write(address,b'\x0f\x0b');thunks[name]=address
    for size in [0,1,9,0x118]:
        alloc=r.call(thunks['LocalAlloc'],(0x40,size));assert alloc['status']=='returned' and alloc['eax']
        pointer=alloc['eax'];assert pointer%8==0 and bytes(r.uc.mem_read(pointer,size))==bytes(size)
        for name,value in [('LocalSize',size),('LocalLock',pointer),('LocalUnlock',1),('LocalFree',0)]:
            call=r.call(thunks[name],(pointer,));assert call['status']=='returned' and call['eax']==value
            assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        assert pointer not in r.local_memory and pointer not in r.allocations
        checks.append(dict(size=size,zero_init_fixed_pointer_and_lifecycle_verified=True))
    call=r.call(thunks['LocalFree'],(0,));assert call['status']=='returned' and call['eax']==0
    call=r.call(thunks['LocalAlloc'],(0x42,8));assert call['status']=='refused'
    result=dict(all_passed=True,checks=checks,reference='Microsoft LocalAlloc/LocalFree fixed memory and zero-init contracts; no independent Wine buffer capture',execution_backend='Unicorn i386 shim API-thunk test',movable_local_memory_implemented=False)
    (ROOT/'evidence/stage10-local-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
