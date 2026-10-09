#!/usr/bin/env python3
"""Compare casing API adapter against independent Wine cases and boundaries."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH,SP,x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='LCMapStringW');checks=[]
    rows=[]
    for name in ['LCMAP','LCMAP804']:rows.extend(json.loads((ROOT/f'evidence/stage10-{name}-result.json').read_text())['rows'])
    for row in rows:
        source=bytes.fromhex(row['source_hex']);out=SCRATCH+0x1000
        r.uc.mem_write(SCRATCH,source);r.uc.mem_write(out,bytes([0xa5])*row['buffer_size']);r.last_error=0x13579
        call=r.call(thunk,(row['LCID'],row['flags'],SCRATCH,row['length']&0xffffffff,0 if row['capacity']==0 else out+4,row['capacity']))
        assert call['status']=='returned' and call['eax']==row['return_value'] and r.last_error==row['last_error'],row['label']
        assert bytes(r.uc.mem_read(out,row['buffer_size'])).hex()==row['canary_buffer_hex'],row['label']
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(label=row['label'],LCID=row['LCID'],result_error_buffer_equal_Wine=True,stdcall_stack_balanced=True))
    for locale,flags,source,label in [(0x41f,0x100,b'A\0','unmeasured locale'),(0,0x400,b'A\0','unmeasured sort-key flags'),(0,0x100,'中'.encode('utf-16le'),'unmeasured Unicode casing')]:
        r.uc.mem_write(SCRATCH,source);call=r.call(thunk,(locale,flags,SCRATCH,len(source)//2,SCRATCH+0x1000,8));assert call['status']=='refused'
        checks.append(dict(case=label,boundary=call['boundary']))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk ABI comparison; not arm64 city execution')
    (ROOT/'evidence/stage10-lcmap-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(all_passed=True,checks=len(checks))))

if __name__=='__main__':main()
