#!/usr/bin/env python3
"""SDK-defined reallocation ownership/content/error guarantees, no pointer guesses."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SP,x

def main():
    r=LoaderRuntime();r.heaps[0x3000]=dict(options=0);thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='HeapReAlloc');checks=[]
    for old_size,new_size,flags in [(16,8,0),(16,16,0),(16,24,0),(16,24,8),(128,144,0),(16,0,0)]:
        old=r.allocate(0x3000,0,old_size);pattern=bytes(range(old_size));r.uc.mem_write(old,pattern);r.last_error=0x13579
        call=r.call(thunk,(0x3000,flags,old,new_size));assert call['status']=='returned' and call['eax'] and r.last_error==0x13579
        new=call['eax'];assert new%8==0 and r.allocations[new]==(0x3000,new_size) and old not in r.allocations
        assert bytes(r.uc.mem_read(new,min(old_size,new_size)))==pattern[:new_size]
        if flags&8 and new_size>old_size:assert bytes(r.uc.mem_read(new+old_size,new_size-old_size))==bytes(new_size-old_size)
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(old_size=old_size,new_size=new_size,flags=flags,ownership_prefix_zero_growth_and_error_verified=True))
    old=r.allocate(0x3000,0,8);r.uc.mem_write(old,b'ABCDEFGH');r.last_error=0x13579
    call=r.call(thunk,(0x3000,0,old,0x1000001));assert call['status']=='returned' and call['eax']==0 and r.last_error==0x13579
    assert old in r.allocations and bytes(r.uc.mem_read(old,8))==b'ABCDEFGH'
    checks.append(dict(out_of_arena_failure=True,old_allocation_content_and_last_error_unchanged=True))
    call=r.call(thunk,(0x3000,0x10,old,24));assert call['status']=='refused'
    checks.append(dict(in_place_only_refused=True,boundary=call['boundary']))
    result=dict(all_passed=True,checks=checks,reference='Microsoft HeapReAlloc defined content/ownership/zero-init/failure guarantees',execution_backend='Unicorn i386 shim API-thunk tests',allocator_pointer_reuse_not_asserted_equal_Wine=True)
    (ROOT/'evidence/stage10-realloc-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
