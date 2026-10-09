#!/usr/bin/env python3
"""Compare requested-size heap API against independently measured Wine sizes."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SP,x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='HeapSize');r.heaps[0x3000]=dict(options=0);checks=[]
    for row in json.loads((ROOT/'evidence/stage10-HEAPSIZE-result.json').read_text())['rows']:
        pointer=r.allocate(0x3000,0,row['requested']);r.last_error=0x13579
        call=r.call(thunk,(0x3000,0,pointer));assert call['status']=='returned' and call['eax']==row['HeapSize'] and r.last_error==row['last_error']
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(requested=row['requested'],result_error_equal_Wine=True,stdcall_stack_balanced=True))
    for heap,flags,ptr,label in [(0x3000,0x80,pointer,'unknown-flags'),(0x3001,0,pointer,'wrong-heap'),(0x3000,0,pointer+1,'interior-pointer')]:
        call=r.call(thunk,(heap,flags,ptr));assert call['status']=='refused';checks.append(dict(case=label,boundary=call['boundary']))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk comparison; not arm64 city execution')
    (ROOT/'evidence/stage10-heap-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(all_passed=True,checks=len(checks))))

if __name__=='__main__':main()
