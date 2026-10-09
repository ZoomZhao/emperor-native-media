#!/usr/bin/env python3
"""Compare the actual arm64 wrapper capture with pre-existing Wine captures."""
import hashlib
import json
import struct
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def main():
    data=(ROOT/'evidence/stage10-native-output.bin').read_bytes()
    nls=json.loads((ROOT/'evidence/stage10-nls-matrix-guestpath-result.json').read_text())
    version=json.loads((ROOT/'evidence/stage10-VERSIONCASES-result.json').read_text())
    for file,expected in [
        ('stage10-nls-matrix-output.bin','738feb4bc3b767c6beb8f990b0819be4d4d52aec1c70ead558651ebdebd2d944'),
        ('stage10-VERSIONCASES-output.bin','53323199e36d825a292f457692c5a50d5aa4b2b8c90a8b8764d8101004580939')]:
        assert hashlib.sha256((ROOT/'evidence'/file).read_bytes()).hexdigest()==expected
    acp,packed=struct.unpack_from('<II',data)
    assert acp==nls['ACP'] and packed==int(version['GetVersion'],16)
    checks=[dict(API='GetACP',value=acp),dict(API='GetVersion',value=hex(packed))]
    offset=8
    for row in nls['rows']:
        returned,error=struct.unpack_from('<II',data,offset);offset+=8
        actual=data[offset:offset+32];offset+=32
        assert (returned,error)==(row['return_value'],row['last_error'])
        assert actual.hex()==row['canary_buffer_hex']
        checks.append(dict(API='GetCPInfo',code_page=row['code_page'],null_pointer=row['null_pointer'],return_error_full_buffer_equal_Wine=True))
    for row in version['rows']:
        returned,error=struct.unpack_from('<II',data,offset);offset+=8
        actual=data[offset:offset+168];offset+=168
        assert (returned,error)==(row['return_value'],row['last_error'])
        assert actual.hex()==row['buffer_hex']
        checks.append(dict(API='GetVersionExA',declared_size=row['declared_size'],return_error_full_buffer_equal_Wine=True))
    assert offset==len(data)==1776
    binary=ROOT/'runs/stage10-native-verify-arm64'
    description=subprocess.run(['/usr/bin/file',str(binary)],capture_output=True,text=True,timeout=5,check=True).stdout.strip()
    assert 'Mach-O 64-bit executable arm64' in description
    abi=json.loads((ROOT/'evidence/stage10-native-capture-v2.stdout.log').read_text())
    assert abi['abi_calls']==56 and abi['stdcall_stack_and_nonvolatile_preserved']
    boundaries=[]
    for mode in ['unknown-cp','null-cp','read-only-cp','truncated-version']:
        process=subprocess.run([str(binary),mode],cwd=ROOT,capture_output=True,text=True,timeout=5)
        assert process.returncode==86 and 'stage10 native boundary:' in process.stderr
        boundaries.append(dict(mode=mode,returncode=process.returncode,stderr=process.stderr.strip(),guest_continuation_refused=True))
    result=dict(all_passed=True,execution_backend=abi['execution_backend'],oracle='Independent existing-Wine PE probes; captured before C implementation',
                checks=checks,abi=abi,boundaries=boundaries,file_description=description,
                binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),capture_sha256=hashlib.sha256(data).hexdigest(),
                original_Emperor_code_linked=False,arm64_city_chain_passed=False,
                note='Six reusable ABI bindings; four measured platform APIs plus thread-local Set/GetLastError. No complete PE32-to-native address bridge, callbacks, heap or city loader port.')
    (ROOT/'evidence/stage10-native-regression.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(all_passed=True,Wine_comparisons=len(checks),ABI_calls=abi['abi_calls'],explicit_boundaries=len(boundaries),binary_sha256=result['binary_sha256'],arm64_city_chain_passed=False)))

if __name__=='__main__':main()
