#!/usr/bin/env python3
"""Actual adapter comparison with captured independent Wine conversion cases."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH,SP,x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='MultiByteToWideChar');checks=[]
    rows=json.loads((ROOT/'evidence/stage10-MB936-result.json').read_text())['rows']
    for row in rows:
        source=bytes.fromhex(row['source_hex']);out=SCRATCH+0x2000
        r.uc.mem_write(SCRATCH,source);r.uc.mem_write(out,bytes([0xa5])*row['buffer_size']);r.last_error=0x13579
        call=r.call(thunk,(936,row['flags'],SCRATCH,row['length']&0xffffffff,0 if row['capacity']==0 else out+4,row['capacity']&0xffffffff))
        assert call['status']=='returned' and call['eax']==row['return_value'] and r.last_error==row['last_error'],row['label']
        assert bytes(r.uc.mem_read(out,row['buffer_size'])).hex()==row['canary_buffer_hex'],row['label']
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(label=row['label'],result_error_buffer_equal_Wine=True,stdcall_stack_balanced=True))
    for cp,flags,src,label in [(936,1,b'\x81\x40','DBCS pair'),(65001,0,b'A','unmeasured UTF8'),(936,4,b'A','unmeasured glyph flags')]:
        r.uc.mem_write(SCRATCH,src);call=r.call(thunk,(cp,flags,SCRATCH,len(src),SCRATCH+0x1000,8));assert call['status']=='refused'
        checks.append(dict(case=label,boundary=call['boundary']))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk comparison; not arm64 city logic',generic_DBCS_implemented=False)
    (ROOT/'evidence/stage10-mb-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(all_passed=True,checks=len(checks))))

if __name__=='__main__':main()
