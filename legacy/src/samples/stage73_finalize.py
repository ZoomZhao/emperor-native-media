"""Build-only manifest, source/byte audit and CLI syntax validation. No engine."""
import ast
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'evidence'

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb')as f:
        for b in iter(lambda:f.read(1<<20),b''):h.update(b)
    return h.hexdigest()

def write(path,data):path.write_text(json.dumps(data,indent=2)+'\n')

def pe_read(body,va,size):
    pe=struct.unpack_from('<I',body,60)[0];n=struct.unpack_from('<H',body,pe+6)[0]
    op=pe+24;imagebase=struct.unpack_from('<I',body,op+28)[0]
    start=op+struct.unpack_from('<H',body,pe+20)[0];rva=va-imagebase
    for i in range(n):
        p=start+i*40;vsize,base,rawsize,raw=struct.unpack_from('<4I',body,p+8)
        if base<=rva and rva+size<=base+rawsize:
            return body[raw+rva-base:raw+rva-base+size]
    raise ValueError('PE window not file-backed '+hex(va))

def main():
    contract=json.loads((E/'stage70-real-input-contract.json').read_text())
    build=json.loads((E/'stage73-bound-build-result.json').read_text())
    meta=json.loads((ROOT/'runs/stage70-city/analysis.json').read_text())
    ir=(ROOT/'runs/stage73-SR-bound/protected.ll').read_text()
    CPUslots=sorted(set(map(int,re.findall(r'getelementptr %_cpu, %_cpu\* %cpu, i32 0, i32 (\d+)',ir))))
    assert CPUslots==list(range(9))+[13],CPUslots
    assert build['guest_memory_load_guards']==54 and build['guest_memory_store_guards_and_logs']==58
    assert ir.count('call ccc void @stage72_guard(')==112
    assert ir.count('call ccc void @stage72_note_write(')==58
    assert build['REP_helper']['actual_store_log_insertions']==4
    # Every original guest dereference, rather than just matched accesses, has
    # a preceding guard. The compiler's CPU-field accesses are separate GEPs.
    ptrs={};audited=0;lines=ir.splitlines()
    for i,line in enumerate(lines):
        if line.startswith('define '):ptrs={}
        q=re.match(r'(%\d+) = inttoptr i64 %\d+ to i(8|16|32)\*$',line)
        if q:ptrs[q[1]]=int(q[2])
        if re.match(r'(?:%\d+ = )?(?:load|store) i(?:8|16|32)[ ,]',line):
            q=re.search(r'i(?:8|16|32)\* (%\d+)',line)
            if q and q[1]in ptrs:
                assert '@stage72_guard('in lines[i-1],line
                audited+=1
    assert audited==112
    static=json.loads((E/'stage69-static-closure.json').read_text())
    original=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
    original_body=original.read_bytes();synthetic=(ROOT/'runs/stage73-SR-bound/guestVA-six.exe').read_bytes()
    windows=[]
    for row in static['functions']:
        low=int(row['entry'],16);high=int(row['exclusive_end'],16)
        a=pe_read(original_body,low,high-low);b=pe_read(synthetic,low,high-low)
        assert a==b and hashlib.sha256(a).hexdigest()==row['sha256']
        windows.append(dict(low=low,exclusive_high=high,bytes=high-low,sha256=row['sha256']))
    assert sum(r['bytes']for r in windows)==728
    # The full tuple metadata is read from the saved JSON using existing own
    # pure-data decoding. No CPU offsets, engine imports or register guesses.
    from stage69_static import Saved
    saved=Saved('cycle61a');raw,size,arch,mode=saved.threads[1]['context'];saved.memory.close()
    assert raw==(ROOT/'runs/stage70-input/main_tid1_context_OPAQUE.bin').read_bytes()
    assert (size,arch,mode)==(5608,4,4)
    tuplemeta=dict(size=size,arch=arch,mode=mode,original_full_tuple_provenance=contract['immutable_state']['path'])
    del saved,raw
    protected=json.loads((E/'stage64-postflight.json').read_text())['checks']
    audit=[]
    for row in protected:
        actual=sha(row['path']);assert actual==row['expected_sha256'],row['path']
        audit.append(dict(path=row['path'],sha256=actual,unchanged=True))
    ledgerpath=E/'stage55-batch-ledger.json';ledger=json.loads(ledgerpath.read_text())
    assert sha(ledgerpath)=='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
    assert len(ledger['executions'])==20 and ledger['remaining_processes']==80 and ledger['goal_closed']
    for p,expected in ledger['pinned_files'].items():assert sha(p)==expected,p
    sources=['samples/stage73_diff.py','samples/stage73_cpu.py','samples/stage73_build.py',
        'samples/stage73_finalize.py','samples/run_bounded.py','samples/stage72_runtime.h',
        'samples/stage72_runtime.c','runs/stage73-SR-bound/protected.ll',
        'runs/stage73-SR-bound/rep-traced.c','runs/stage73-SR-bound/runtime.c',
        'runs/stage73-SR-bound/guestVA-six.exe','runs/stage73-SR-bound/libstage73_SR_diff_UNEXECUTED.dylib',
        'vendor-src/SR/SR/llasm-support/llasm_cpu.h','vendor-src/SR/SR/llasm-support/llasm_stos.c',
        'evidence/stage73-bound-build-result.json','evidence/stage55-batch-ledger.json',
        'runs/stage12-state-cycle61a/memory.bin','runs/stage12-state-cycle61a/state.json',
        'runs/stage70-input/main_tid1_context_OPAQUE.bin']
    sources += [str(p.relative_to(ROOT))for p in sorted((ROOT/'vendor-tools/unicorn-2.1.4/unicorn').rglob('*.py'))]
    sources += ['vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib',str(original)]
    pinned=[dict(path=p,sha256=sha(Path(p)if Path(p).is_absolute()else ROOT/p))for p in sources]
    regions=[dict(low=r['low'],size=r['high']+1-r['low'],permissions=r['permissions'],offset=r['offset'])
             for r in contract['all_original_guest_regions']]
    assert len(regions)==28 and sum(r['size']for r in regions)==297521152
    offsets=0;prior=0
    for r in regions:
        assert r['offset']==offsets and r['low']>=prior and r['low']%4096==0 and r['size']%4096==0
        offsets+=r['size'];prior=r['low']+r['size']
    spans=contract['extracted_spans']
    # A normal flat descriptor with the documented 4GiB limit covers intervals
    # once its cached base/access is checked at each interval's endpoints.
    data=[q for q in spans if q['name']!='full_main_stack']
    ds_low=min(int(q['guest_address'],16)for q in data)
    ds_high=max(int(q['guest_address'],16)+q['bytes']for q in data)
    rep=json.loads((E/'stage72-input-and-preflight-contracts.json').read_text())['normal_REP_access_spans']
    es_low=min(int(q['write_low'],16)for q in rep);es_high=max(int(q['write_exclusive_high'],16)for q in rep)
    instructions=[dict(pc=q['pc'],size=q['end']-q['pc'])for q in sorted(meta['instructions'],key=lambda q:q['pc'])]
    assert len(instructions)==214
    m=dict(version=1,status='BUILD_ONLY_COMMANDS_IMPLEMENTED_UNEXECUTED',
        execution_scope='Stage73 fixed EmperorEN.exe historical cycle61a main tid1 continuation 0x4af311/SP0x700ffd68 to before 0x4ac333/SP0x700ffd8c; six 728-byte original windows, reference once and SR native once; clone-only authored segment probes max12 instructions; no scheduler, worker, import, UI or original DLL',
        CPU_tuple_metadata=tuplemeta,context='runs/stage70-input/main_tid1_context_OPAQUE.bin',
        memory='runs/stage12-state-cycle61a/memory.bin',regions=regions,original_windows=windows,
        original_instructions=instructions,entry=dict(PC=0x4af311,SP=0x700ffd68),
        stop=dict(PC=0x4ac333,SP=0x700ffd8c,before_instruction=True),
        segment_probe_ranges=dict(DS=[ds_low,ds_high],SS=[0x70000000,0x70100000],ES=[es_low,es_high]),
        segment_scope='cached DS/ES/SS endpoint read/write access at original flat normal-data descriptor limit4GiB; paging/VM/TF declined; not a generic CPU or fault equivalence proof',
        unicorn_package='vendor-tools/unicorn-2.1.4',
        unicorn_library='vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib',
        native_library='runs/stage73-SR-bound/libstage73_SR_diff_UNEXECUTED.dylib',
        return_guest_PCs=[int(r['guest_PC'],16)for r in build['return_bindings']],
        limits=dict(guest_seconds=8,hard_process_seconds=60,local_alarm_seconds=45,steps=5000000,writes=2000000,
                    reference_attempts=1,native_attempts=1,authored_probe_instructions_maximum=12),
        CPU_GEP_slots=CPUslots,x87_unchanged_sidecar=True,full_fault_or_mid_REP_resume_equivalence=False,
        pinned_inputs_and_sources=pinned,old_ledger=dict(status='CLOSED',used=20,remaining=80,reopen_allowed=False))
    write(E/'stage73-run-manifest.json',m)
    approval=dict(status='PENDING_USER_APPROVAL',user_message_id='',manifest_sha256=sha(E/'stage73-run-manifest.json'),
                  scope=m['execution_scope'],maximum_attempts=dict(reference=1,native=1))
    write(E/'stage73-run-approval.example.json',approval)
    for name in ['stage73_diff.py','stage73_cpu.py','stage73_build.py','stage73_finalize.py']:
        ast.parse((ROOT/'samples'/name).read_text(),filename=name)
    commands=[
        '/usr/bin/python3 samples/run_bounded.py stage73-reference-once . /usr/bin/python3 samples/stage73_diff.py reference --run --approval evidence/stage73-run-approval.json --output runs/stage73-reference-once',
        '/usr/bin/python3 samples/run_bounded.py stage73-native-once . /usr/bin/python3 samples/stage73_diff.py native --run --approval evidence/stage73-run-approval.json --reference runs/stage73-reference-once/result.json --output runs/stage73-native-once']
    result=dict(status='ACTUAL_SR_BOUND_LIBRARY_AND_DIFFERENTIAL_COMMANDS_BUILT_UNEXECUTED',
        manifest='evidence/stage73-run-manifest.json',manifest_sha256=sha(E/'stage73-run-manifest.json'),
        library=build['artifact'],library_sha256=build['sha256'],commands=commands,
        permission='Await exact approval recorded in evidence/stage73-run-approval.json; example is pending and rejected by CLI',
        source_CPU_tuple_metadata_read_without_engine=True,original_728_bytes_identical=windows,
        guest_loads_guards=54,guest_store_guards_and_trace_sites=58,REP_actual_store_trace_sites=4,
        return_guest_PCs=m['return_guest_PCs'],SR_CPU_GEP_slots=CPUslots,
        syntax_only_passed=True,engine_imported_or_called=False,library_loaded_or_executed=False,
        old_protected_hash_checks=len(audit),old_bound_source_pins=len(ledger['pinned_files']),
        old_ledger_sha256=sha(ledgerpath),old_ledger_closed_20_used_80_remaining=True,
        differential_not_run=True,behavior_equivalence_unknown=True,
        historical_failures=[dict(attempt='v1',failure='SRW rejected stripped relocations without relocations.csv'),
            dict(attempt='v2',failure='Removing relocations produced unsupported commented memory helpers; original-instruction check refused'),
            dict(attempt='v3',failure='llasm returned79 for constant load/store address; fixed via explicit tmpadr'),
            dict(attempt='v4',failure='Build passed; static review found read guards absent due LLVM load comma syntax'),
            dict(attempt='v5',result='Build passed with54 read guards and58 store guards/logs')],
        remaining_runtime_uncertainties=['Opaque saved CPU ABI restoration and public-register consistency',
            'Actual cached segment endpoint probes','Non-overwriting common-offset Mach shadow mapping',
            'Full original write sequence, all mapped bytes and exact public CPU comparison',
            'No generic REP fault semantics, mid-REP or whole-game checkpoint proof'])
    write(E/'stage73-postflight.json',dict(all_passed=True,checks=audit,
          original_source_pins_unchanged=len(ledger['pinned_files']),old_ledger_sha256=sha(ledgerpath)))
    write(E/'stage73-result.json',result)
    print(json.dumps({k:result[k]for k in ['status','manifest_sha256','library_sha256','old_protected_hash_checks','old_bound_source_pins']}))

if __name__=='__main__':main()
