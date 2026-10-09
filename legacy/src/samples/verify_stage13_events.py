#!/usr/bin/env python3
"""Independent same-Wine oracle versus the native arm64 C event ABI."""
import ctypes,hashlib,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage13_events import NativeEvents,REFUSED

def main():
    oracle=json.loads((ROOT/'evidence/stage13-event-Wine-result.json').read_text());state=NativeEvents();loaded={};rows=[];skipped=[]
    for row in oracle['rows']:
        name=row['name'];args=[loaded[a['load']] if isinstance(a,dict) else a for a in row['args']];label=row['label'];at=row['result_offset']+0x420000
        if name in ['CreateMutexA','CreateFileA']:
            loaded[at]=0x72000000+len(loaded);skipped.append(dict(label=label,reason='foreign object creation belongs to its own typed owner'));continue
        if name=='GetTickCount' or label=='current-process-wait' or label in ['mutex-close','file-close']:
            skipped.append(dict(label=label,reason='clock or valid non-event object lifecycle outside event core'));continue
        native_name={'CreateEventA':'create','SetEvent':'set','ResetEvent':'reset','WaitForSingleObject':'wait','CloseHandle':'close'}[name]
        actual_args=args[1:3] if name=='CreateEventA' else args
        started=time.monotonic();value,error=state.invoke(native_name,actual_args,0x13579);elapsed=time.monotonic()-started
        if name=='CreateEventA':assert value not in [0,REFUSED];loaded[at]=value;return_equal=bool(row['return_value'])
        else:return_equal=value==row['return_value']
        assert return_equal and error==row['last_error'],(row,value,error)
        if label=='wait-30ms':assert .025<=elapsed<1
        rows.append(dict(label=label,name=name,return_equal=return_equal,last_error_equal=True,native_return_value=value,native_last_error=error,elapsed_seconds=round(elapsed,6)))
    # Export/import preserves handles, live ownership, signal state, next ID.
    auto,error=state.invoke('create',[0,1],77);manual,error=state.invoke('create',[1,1],77)
    dead,error=state.invoke('create',[0,0],77);state.invoke('close',[dead],77)
    image=state.export();other=NativeEvents();other.restore(image);assert other.export()==image
    continuation=[('wait',[auto,0]),('wait',[auto,0]),('wait',[manual,0]),('reset',[manual]),('wait',[manual,0]),('set',[auto]),('close',[manual]),('wait',[dead,0]),('create',[0,0])]
    for name,args in continuation:assert state.invoke(name,args,0x13579)==other.invoke(name,args,0x13579)
    assert state.export()==other.export()
    # Malformed snapshot import must not mutate the live registry.
    good=state.export();bad=bytearray(good);struct.pack_into('<I',bad,0,0)
    buf=ctypes.create_string_buffer(bytes(bad));assert state.import_c(state.state,buf,len(bad))==0 and state.export()==good
    # Finite table exhaustion refuses before inventing an OS success/failure.
    limited=NativeEvents();ids=[]
    for i in range(128):h,e=limited.invoke('create',[0,0],77);assert e==0;ids.append(h)
    h,e=limited.invoke('create',[0,0],77);assert h==REFUSED and e==77
    limited.invoke('close',[ids[0]],77);fresh,e=limited.invoke('create',[0,0],77);assert fresh!=ids[0]
    assert limited.invoke('wait',[ids[0],0],77)==(0xffffffff,6)
    result=dict(all_passed=True,same_Wine_operation_comparisons=len(rows),rows=rows,outside_core=skipped,snapshot_bytes=len(image),snapshot_continuation_operations=len(continuation),entire_registry_equal_after_continuation=True,corrupt_import_rejected_without_mutation=True,stale_handles_rejected=True,capacity_boundary_refuses=True,native_backend='compiled Mach-O arm64 C ABI via ctypes, not Python state logic')
    (ROOT/'evidence/stage13-event-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['rows','outside_core']}))
    state.dispose();other.dispose();limited.dispose()

if __name__=='__main__':main()
