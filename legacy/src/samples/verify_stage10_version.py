#!/usr/bin/env python3
"""Compare corrected version adapter and invalid sizes against Wine evidence."""
import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import SCRATCH,SP,x

def main():
    r=LoaderRuntime();thunks={sym:p for p,(dll,sym) in r.imports.items() if dll.lower()=='kernel32.dll'};checks=[]
    reference=json.loads((ROOT/'evidence/stage10-VERSIONCASES-result.json').read_text())
    for row in reference['rows']:
        size=row['declared_size'];before=bytes([0xa5])*4+struct.pack('<I',size)+bytes([0xa5])*160
        r.uc.mem_write(SCRATCH,before);r.last_error=0x13579
        call=r.call(thunks['GetVersionExA'],(SCRATCH+4,))
        assert call['status']=='returned' and call['eax']==row['return_value'] and r.last_error==row['last_error']
        assert bytes(r.uc.mem_read(SCRATCH,168)).hex()==row['buffer_hex'] and r.uc.reg_read(x.UC_X86_REG_ESP)==SP
        checks.append(dict(declared_size=size,return_error_full_buffer_equal_independent_Wine=True))
    call=r.call(thunks['GetVersion'],());assert call['status']=='returned' and call['eax']==int(reference['GetVersion'],16)
    assert r.uc.reg_read(x.UC_X86_REG_ESP)==SP
    checks.append(dict(GetVersion=hex(call['eax']),agrees_with_GetVersionExA_fields=True))
    result=dict(all_passed=True,checks=checks,execution_backend='Unicorn i386 API-thunk regression; not arm64 runtime',old_XP_version_replaced=True)
    (ROOT/'evidence/stage10-version-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
