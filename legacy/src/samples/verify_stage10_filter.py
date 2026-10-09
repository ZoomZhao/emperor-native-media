#!/usr/bin/env python3
"""Measure guest callback replacement identity against independent Wine probe."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SP,x

def main():
    r=LoaderRuntime();thunk=next(p for p,(dll,sym) in r.imports.items() if sym=='SetUnhandledExceptionFilter');checks=[]
    ref=json.loads((ROOT/'evidence/stage10-FILTER-result.json').read_text())['rows']
    # Wine probe's own function maps to this guest's valid original function.
    translate={0:0,0x40f000:0x76ca77}
    for row in ref:
        pointer=translate[int(row['callback'],16)];r.last_error=0x13579
        call=r.call(thunk,(pointer,));assert call['status']=='returned'
        assert call['eax']==translate[int(row['previous'],16)]
        assert r.last_error==row['last_error'] and r.exception_filter==pointer
        assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(callback=hex(pointer),previous=hex(call['eax']),reference_pointer_identity_relation_preserved=True))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk ABI comparison; not arm64 callback execution',callback_invoked=False,exception_dispatch_implemented=False)
    (ROOT/'evidence/stage10-filter-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
