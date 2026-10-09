#!/usr/bin/env python3
"""Movable guest handle state machine against independent same-Wine cases."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SP,x,THUNKS

def main():
    r=LoaderRuntime();thunks={sym:p for p,(dll,sym) in r.imports.items() if dll.lower()=='kernel32.dll'};checks=[]
    for name in ['GlobalAlloc','GlobalFree','GlobalSize','GlobalLock','GlobalUnlock','GlobalFlags','GlobalHandle']:
        if name not in thunks:
            address=THUNKS+len(r.imports)*16;r.imports[address]=('KERNEL32.dll',name)
            r.uc.mem_write(address,b'\x0f\x0b');thunks[name]=address
    for case in json.loads((ROOT/'evidence/stage10-GLOBAL-result.json').read_text())['cases']:
        handle=None;pointer=None;mapping={0:0}
        sequence=[('alloc','GlobalAlloc',(case['flags'],case['size'])),('size','GlobalSize',()),('initial_flags','GlobalFlags',()),('lock1','GlobalLock',()),('lock2','GlobalLock',()),('locked_flags','GlobalFlags',())]
        if case['size']:sequence.append(('pointer_to_handle','GlobalHandle',()))
        sequence.extend([('unlock1','GlobalUnlock',()),('unlock2','GlobalUnlock',()),('unlock3','GlobalUnlock',()),('final_flags','GlobalFlags',()),('free','GlobalFree',())])
        for label,name,args in sequence:
            r.last_error=0x13579
            if not args:args=(pointer if name=='GlobalHandle' else handle,)
            call=r.call(thunks[name],args);assert call['status']=='returned',label
            ref=case['steps'][label]
            if label=='alloc':handle=call['eax'];mapping[ref['return_value']]=handle
            if label=='lock1':
                pointer=call['eax'];mapping[ref['return_value']]=pointer
                if case['size']:assert pointer!=handle and pointer%8==0
                if case['flags']&0x40:assert bytes(r.uc.mem_read(pointer,case['size']))==bytes(case['size'])
            expected=mapping.get(ref['return_value'],ref['return_value'])
            assert call['eax']==expected and r.last_error==ref['last_error'],label
            assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
            checks.append(dict(flags=hex(case['flags']),size=case['size'],step=label,reference_result_error_and_pointer_identity_relation_verified=True))
    assert not r.global_memory
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 shim API-thunk comparison; not arm64 MFC/global-memory execution',uninitialized_contents_compared=False,relocation_and_GlobalReAlloc_implemented=False)
    (ROOT/'evidence/stage10-global-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(all_passed=True,checks=len(checks))))

if __name__=='__main__':main()
