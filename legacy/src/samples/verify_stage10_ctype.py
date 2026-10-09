#!/usr/bin/env python3
"""API-thunk comparison against independent same-Wine character probe."""
import json
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH, SP, x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='GetStringTypeW')
    reference=json.loads((ROOT/'evidence/stage10-ctype-probe-safe-result.json').read_text())
    checks=[];sample='A中\0'.encode('utf-16le');r.uc.mem_write(SCRATCH,sample)
    for row in reference['rows']:
        out=SCRATCH+0x100;r.uc.mem_write(out,bytes([0xa5])*24);r.last_error=0x13579
        result=r.call(thunk,(row['info_type'],SCRATCH,row['signed_length']&0xffffffff,out+4))
        assert result['status']=='returned' and result['eax']==row['return_value'] and r.last_error==row['last_error']
        assert bytes(r.uc.mem_read(out,24)).hex()==row['canary_buffer_hex']
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(info_type=row['info_type'],signed_length=row['signed_length'],buffer_return_error_equal_Wine=True,stdcall_stack_balanced=True))
    # Include explicit NUL, digits, whitespace, CJK, and non-ASCII BMP units.
    source=bytes().join(c.to_bytes(2,'little') for c in [0,9,10,32,48,65,97,127,255,0x4e2d,0xff21])
    r.uc.mem_write(SCRATCH,source);r.uc.mem_write(SCRATCH+0x100,bytes([0xcc])*64)
    result=r.call(thunk,(1,SCRATCH,len(source)//2,SCRATCH+0x104));assert result['status']=='returned'
    expected=b''.join(r.nls.ctype[c*2:c*2+2] for c in [0,9,10,32,48,65,97,127,255,0x4e2d,0xff21])
    assert bytes(r.uc.mem_read(SCRATCH+0x100,64))==bytes([0xcc])*4+expected+bytes([0xcc])*(60-len(expected))
    checks.append(dict(explicit_NUL_and_BMP_input=True,output_word_count_matches_input=True,output_canaries_unchanged=True))
    for info,src,count,out,label in [(2,SCRATCH,1,SCRATCH+0x100,'unmeasured-valid-ctype2'),(4,SCRATCH,1,SCRATCH+0x100,'unmeasured-valid-ctype3'),(1,SCRATCH,0xfffffffe,SCRATCH+0x100,'negative-minus-two'),(1,0x30000000,1,SCRATCH+0x100,'unmapped-source'),(1,SCRATCH,1,0,'null-output'),(1,SCRATCH,1,r.pe.base,'read-only-output'),(1,SCRATCH,1,SCRATCH,'aliased-buffers')]:
        result=r.call(thunk,(info,src,count,out));assert result['status']=='refused'
        checks.append(dict(case=label,boundary=result['boundary']))
    r.uc.mem_write(SCRATCH,b'\x00\xd8')
    result=r.call(thunk,(1,SCRATCH,1,SCRATCH+0x100));assert result['status']=='refused'
    checks.append(dict(case='unverified-surrogate-context',boundary=result['boundary']))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk ABI tests; not translated arm64 city execution')
    (ROOT/'evidence/stage10-ctype-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
