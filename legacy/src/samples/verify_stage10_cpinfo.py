#!/usr/bin/env python3
"""Compare the actual API adapter with independent captured Wine output."""
import json
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH, SENTINEL, SP, u, x

def main():
    r=LoaderRuntime()
    reference=json.loads((ROOT/'evidence/stage10-nls-matrix-guestpath-result.json').read_text())
    thunk=next(p for p,(dll,sym) in r.imports.items() if dll.lower()=='kernel32.dll' and sym=='GetCPInfo')
    checks=[]
    for row in reference['rows']:
        r.uc.mem_write(SCRATCH,bytes([0xa5])*32);r.last_error=0x13579
        before_flags=0x246;r.uc.reg_write(x.UC_X86_REG_EFLAGS,before_flags)
        before_nonvolatile={reg:0x11223344+i for i,reg in enumerate([x.UC_X86_REG_EBX,x.UC_X86_REG_ESI,x.UC_X86_REG_EDI,x.UC_X86_REG_EBP])}
        for reg,value in before_nonvolatile.items():r.uc.reg_write(reg,value)
        result=r.call(thunk,(row['code_page'],0 if row['null_pointer'] else SCRATCH+4))
        data=bytes(r.uc.mem_read(SCRATCH,32))
        assert result['status']=='returned' and result['eax']==row['return_value']
        assert r.last_error==row['last_error'] and data.hex()==row['canary_buffer_hex']
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        assert all(r.uc.reg_read(reg)==value for reg,value in before_nonvolatile.items())
        checks.append(dict(code_page=row['code_page'],null_pointer=row['null_pointer'],return_value=result['eax'],last_error=r.last_error,buffer_equal_Wine=True,stdcall_stack_balanced=True,nonvolatile_registers_preserved=True))
    # Writable but unaligned pointers are supported by the measured x86 ABI.
    for offset in [0,1,2,3,4,13]:
        r.uc.mem_write(SCRATCH,bytes([0x5a])*64);r.last_error=0x13579
        out=r.call(thunk,(936,SCRATCH+offset));assert out['status']=='returned' and out['eax']==1
        after=bytes(r.uc.mem_read(SCRATCH,64));payload=r.nls.success[936]
        assert after==bytes([0x5a])*offset+payload+bytes([0x5a])*(64-offset-len(payload))
        checks.append(dict(unaligned_offset=offset,only_18_output_bytes_written=True))
    # Bad buffers are explicit unsupported boundaries, not fabricated Win32 returns.
    for cp,pointer,label in [(936,0,'null'),(936,0x30000000,'unmapped'),(936,r.pe.base,'read-only'),(936,SCRATCH+0x40000-17,'range-crosses-unmapped'),(932,SCRATCH,'valid-unmeasured-code-page')]:
        r.uc.mem_write(SCRATCH,bytes([0x7d])*32)
        out=r.call(thunk,(cp,pointer))
        assert out['status']=='refused'
        assert bytes(r.uc.mem_read(SCRATCH,32))==bytes([0x7d])*32
        checks.append(dict(case=label,boundary=out['boundary'],no_claim_of_Windows_exception_emulation=True))
    result=dict(execution_backend='Unicorn i386 API-thunk ABI test on arm64 host',GetCPInfo_has_flags_parameter=False,reference='captured independent same-Wine own PE matrix',checks=checks,all_passed=True,arm64_city_chain_passed=False)
    (ROOT/'evidence/stage10-cpinfo-regression.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__':main()
