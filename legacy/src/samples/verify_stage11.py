#!/usr/bin/env python3
"""Independent original-byte reference vs native output; finite platform checks."""
import hashlib,json,struct,subprocess,sys,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage11 import GuidRuntime
from stage7_runtime import TEB,x,u
GUID=uuid.UUID('01234567-89ab-4cde-8f10-213243546576').bytes_le

def main():
    native=(ROOT/'evidence/stage11-native-output.bin').read_bytes();offset=0;checks=[]
    for failure in [False,True]:
        provider=(lambda:(0x8007000e,None)) if failure else (lambda:(0,GUID))
        r=GuidRuntime(provider=provider);h=r.call(0x76db30,(1,));assert h['status']=='returned' and h['eax']==1
        globals=bytes(r.uc.mem_read(0x1e58000,4096));assert native[offset:offset+4096]==globals;offset+=4096
        heap=r.readword(0x1e58d20);obj=r.allocate(heap,0,200);r.word(obj,0xcafebabe);r.last_error=0x13579
        for reg,value in [(x.UC_X86_REG_EBX,0x13579bdf),(x.UC_X86_REG_ESI,0x2468ace0),(x.UC_X86_REG_EDI,0xaabbccdd),(x.UC_X86_REG_EBP,0xdeadbeef)]:r.uc.reg_write(reg,value)
        result=r.call(0x426e60,(),obj);assert result['status']=='returned' and r.readword(TEB)==0xffffffff
        expected=struct.pack('<4I',result['eax'],r.last_error,r.uc.reg_read(x.UC_X86_REG_ESP),len(r.debug_messages))+bytes(r.uc.mem_read(obj,200))
        assert native[offset:offset+216]==expected;offset+=216
        checks.append(dict(case='injected-provider-failure' if failure else 'injected-provider-success',original_CRT_heap_full_4096_byte_BSS_equal=True,original_GUID_constructor_return_error_stack_debug_and_full_200_byte_object_equal=True,deterministic_provider_for_difference_test_only=True))
    guids=[]
    for i in range(32):
        b=native[offset:offset+24];offset+=24
        assert b[:4]==b[20:]==bytes([0xa5])*4
        guid=uuid.UUID(bytes_le=b[4:20]);assert guid.version==4 and guid.variant==uuid.RFC_4122;guids.append(str(guid))
    assert len(set(guids))==32 and offset==len(native)==9392
    run=json.loads((ROOT/'evidence/stage11-native-run-v2.stdout.log').read_text())
    binary=ROOT/'runs/stage11-native/verify-arm64';description=subprocess.run(['/usr/bin/file',str(binary)],capture_output=True,text=True,check=True,timeout=5).stdout.strip();assert 'executable arm64' in description
    negatives=[]
    for mode in ['heap','freed','realloc-flags','tls','pointer-overflow','guid-pointer']:
        p=subprocess.run([str(binary),'--negative',mode],capture_output=True,text=True,timeout=5)
        assert p.returncode==86 and 'stage11 boundary:' in p.stderr
        negatives.append(dict(mode=mode,returncode=p.returncode,stderr=p.stderr.strip()))
    wine=json.loads((ROOT/'evidence/stage11-guid-Wine-result.json').read_text());assert wine['all_passed']
    fixture=json.loads((ROOT/'evidence/stage11-native-slice-layout.json').read_text())
    result=dict(all_passed=True,original_slice_cases=checks,native_runtime=run,production_GUID_samples=guids,production_GUID_contract_equal_Wine=dict(HRESULT_success=0,LastError_preserved=0x13579,NULL_HRESULT='0x80070057',RFC4122_version=4,canaries_preserved=True,random_bytes_cross_run_compared=False),negative_boundaries=negatives,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),file_description=description,fixture=fixture,strict_EFLAGS_compared=False,full_native_CRT_initializer_table=False,full_native_city_loaded=False,note='Actual SRW LLVM arm64 original CRT heap and GUID-consumer slices. Heap/TLS ownership and pointer domain exercised natively; source/host code addresses differ legitimately. Failure entropy provider is test-only. No complete native city runtime.')
    (ROOT/'evidence/stage11-native-regression.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(all_passed=True,original_slice_cases=len(checks),native_runtime=run,negative_boundaries=len(negatives),binary_sha256=result['binary_sha256'])))

if __name__=='__main__':main()
