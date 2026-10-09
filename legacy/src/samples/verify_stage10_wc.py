#!/usr/bin/env python3
"""Independent captured Wine reverse-conversion adapter regression."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH,SP,x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='WideCharToMultiByte');checks=[]
    for row in json.loads((ROOT/'evidence/stage10-WC936-result.json').read_text())['rows']:
        source=bytes.fromhex(row['source_hex']);out=SCRATCH+0x1000;used=SCRATCH+0x2000
        r.uc.mem_write(SCRATCH,source);r.uc.mem_write(out,bytes([0xa5])*row['buffer_size']);r.uc.mem_write(used,bytes([0xa5])*4);r.last_error=0x13579
        call=r.call(thunk,(936,row['flags'],SCRATCH,row['length']&0xffffffff,0 if row['capacity']==0 else out+4,row['capacity']&0xffffffff,0,used if row['used_default_pointer'] else 0))
        assert call['status']=='returned' and call['eax']==row['return_value'] and r.last_error==row['last_error'],row['label']
        assert bytes(r.uc.mem_read(out,row['buffer_size'])).hex()==row['canary_buffer_hex'],row['label']
        assert r.readword(used)==row['used_default'] and r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(label=row['label'],result_error_buffer_used_default_equal_Wine=True,stdcall_stack_balanced=True))
    for flags,source,default,label in [(0x220,'中'.encode('utf-16le'),0,'unmeasured DBCS output'),(0,b'A\0',0,'unmeasured flags'),(0x220,b'A\0',SCRATCH+0x3000,'custom default character')]:
        r.uc.mem_write(SCRATCH,source);call=r.call(thunk,(936,flags,SCRATCH,len(source)//2,SCRATCH+0x1000,8,default,0));assert call['status']=='refused'
        checks.append(dict(case=label,boundary=call['boundary']))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk ABI comparison; not arm64 city logic')
    (ROOT/'evidence/stage10-wc-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(all_passed=True,checks=len(checks))))

if __name__=='__main__':main()
