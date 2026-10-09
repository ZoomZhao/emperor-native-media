#!/usr/bin/env python3
"""Read-only audit of fixed engine bytes and independent output fixtures."""
import json,struct,hashlib,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def records(p,words):
    b=p.read_bytes();h=struct.unpack_from('<4I',b)
    assert h[2]==words and len(b)==16+h[1]*4*words
    return [struct.unpack_from('<'+str(words)+'I',b,16+i*4*words) for i in range(h[1])]
wheel=ROOT/'evidence/unicorn-2.1.4-cp37-abi3-macosx_11_0_arm64.whl'
assert digest(wheel)=='d6c93e0f60328d8f4a1792af3f834137a28050fcc2305f2ec01efe8558a9844e'
pkg=ROOT/'vendor-tools/unicorn-2.1.4';checked=0
with zipfile.ZipFile(wheel) as z:
    for entry in z.infolist():
        if entry.is_dir():continue
        assert (pkg/entry.filename).read_bytes()==z.read(entry.filename),entry.filename
        checked+=1
original=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
assert digest(original)=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
assert digest(ROOT/'samples/tax-cases.bin')=='6521b854a1567d84a774973de9805ae8d038c26a6f2b295317435268f63dce95'
assert digest(ROOT/'samples/write-cases.bin')=='e4457ebadfe905f1c497d70f2061dd186f09c00ae7ef1601588bc82aaefda359'
self=records(ROOT/'samples/tax-cases.bin',36);independent=records(ROOT/'samples/stage4-tax-0-unicorn-cases.bin',36)
assert len(self)==len(independent)==12335
different=[];defined_errors=0;other_errors=0
for i,(a,b) in enumerate(zip(self,independent)):
    columns=[j for j in range(36) if a[j]!=b[j]]
    if columns:
        if columns!=[18]:other_errors+=1
        xor=a[18]^b[18];defined=bool(xor&(a[19]|(0xffffffff^0x8d5)))
        defined_errors+=defined
        different.append(dict(case=i,kind=a[0],self_flags=hex(a[18]),unicorn_flags=hex(b[18]),xor=hex(xor),defined_mask=hex(a[19]),defined_difference=defined,columns=columns))
assert not defined_errors and not other_errors
write_equal=(ROOT/'samples/write-cases.bin').read_bytes()==(ROOT/'samples/stage4-write-0-unicorn-cases.bin').read_bytes()
assert write_equal
paths={m:json.loads((ROOT/f'evidence/stage4-{m}-0-unicorn-paths.json').read_text()) for m in ('leaf','tax','write')}
results={m:json.loads((ROOT/f'evidence/stage4-{m}-0-unicorn.json').read_text()) for m in paths}
for m in paths:
    assert results[m]['passed'] and len(paths[m])==results[m]['unique_paths']
    assert len({a for p in paths[m] for a in p})==results[m]['instruction_addresses']
native={m:json.loads((ROOT/f'evidence/stage4-native-{m}-unicorn.stdout.log').read_text()) for m in paths}
assert all(r['passed'] for r in native.values())
crashpath=Path('<LOCAL_MACHINE_PATH>')
s=crashpath.read_text();decoder=json.JSONDecoder();_,at=decoder.raw_decode(s);crash,_=decoder.raw_decode(s,at+len(s[at:])-len(s[at:].lstrip()))
crash_evidence=dict(report=crashpath.name,translated=crash['translated'],exception=crash['exception'],first_frames=crash['threads'][crash['faultingThread']]['frames'][:4],library_architecture=crash['usedImages'][19]['arch'],decoded_host_instruction='MRS x8, CTR_EL0',guest_instructions_executed=0)
(ROOT/'evidence/stage4-unicorn-crash-diagnostic.json').write_text(json.dumps(crash_evidence,indent=2))
(ROOT/'evidence/stage4-fixture-flag-differences.json').write_text(json.dumps(different,indent=2))
report=dict(wheel_sha256=digest(wheel),wheel_files_unchanged=checked,library_sha256=digest(pkg/'unicorn/lib/libunicorn.2.dylib'),original_exe_sha256=digest(original),self_tax_fixture_unchanged=True,self_write_fixture_unchanged=True,tax_output_differing_records=len(different),tax_defined_or_control_flag_differences=defined_errors,tax_nonflag_output_differences=other_errors,tax_differing_xor_values=sorted({r['xor'] for r in different}),write_output_fixture_byte_identical=write_equal,unicorn_results=results,native_results=native,full_Windows_game_run=False,independent_engine_baseline_passed=True)
(ROOT/'evidence/stage4-final-results.json').write_text(json.dumps(report,indent=2))
print(json.dumps(dict(wheel_files_unchanged=checked,library_sha256=report['library_sha256'],tax_undefined_flag_records=len(different),xor=report['tax_differing_xor_values'],defined_or_control_differences=defined_errors,other_differences=other_errors,write_fixture_identical=write_equal,all_three_native_passed=True)))
