#!/usr/bin/env python3
"""Two separately approved one-shot differential commands; no execution on import.

--describe is pure data. --run requires a signed-off task-local approval file,
all pinned hashes, and an exclusive one-shot attempt claim before library load.
No source file is writable, no worker is scheduled, no original DLL is loaded.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import struct
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = None
CHUNK = 1<<20
GPRS = ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(CHUNK),b''): h.update(b)
    return h.hexdigest()

def local(path):
    p=(ROOT/path).resolve() if not Path(path).is_absolute() else Path(path).resolve()
    if not p.is_relative_to(ROOT): raise RuntimeError('Path outside independent task: '+str(p))
    return p

def write_json(path,data):
    with path.open('x') as f: json.dump(data,f,indent=2); f.write('\n')

def verify_pins(m):
    for row in m['pinned_inputs_and_sources']:
        p=Path(row['path']) if Path(row['path']).is_absolute() else ROOT/row['path']
        if sha(p)!=row['sha256']: raise RuntimeError('Pin differs: '+str(p))

def authorize(args,m,msha):
    approval=json.loads(local(args.approval).read_text())
    if approval.get('status')!='AUTHORIZED_BY_USER' or not approval.get('user_message_id'):
        raise RuntimeError('This pair has no concrete user approval')
    expected=m['execution_scope']
    if approval.get('scope')!=expected or approval.get('manifest_sha256')!=msha:
        raise RuntimeError('Approval scope or manifest mismatch')
    if approval.get('maximum_attempts')!={'reference':1,'native':1}:
        raise RuntimeError('Approval does not match one attempt per role')
    destination=local(args.output)
    if not destination.is_relative_to(ROOT/'stage74/runs') or destination==ROOT/'stage74/runs':
        raise RuntimeError('Output must be a new task runs subdirectory')
    if destination.exists(): raise RuntimeError('Refuse output overwrite')
    # Exclusive claim is persistent even if any subsequent preflight fails.
    claim=local(m['claim_directory'])/(m['id']+'-'+args.role+'-attempt.claim.json')
    write_json(claim,dict(role=args.role,approval_sha256=sha(local(args.approval)),
                         manifest_sha256=msha,output=str(destination),started_UTC=time.time(),
                         charged_attempt=True,retry_allowed=False,old_ledger_reopened=False))
    destination.mkdir(parents=True,exist_ok=False)
    return destination,approval

def memory_chunks(m):
    with local(m['memory']).open('rb') as source:
        for r in m['regions']:
            source.seek(r['offset'])
            for delta in range(0,r['size'],CHUNK):
                b=source.read(min(CHUNK,r['size']-delta))
                if len(b)!=min(CHUNK,r['size']-delta):raise RuntimeError('Truncated source memory')
                yield r['low']+delta,b

def dump_memory(m,out,reader):
    h=hashlib.sha256();regions=[];offset=0
    with (out/'memory.bin').open('xb') as f:
        for r in m['regions']:
            hr=hashlib.sha256()
            for delta in range(0,r['size'],CHUNK):
                b=bytes(reader(r['low']+delta,min(CHUNK,r['size']-delta)))
                if len(b)!=min(CHUNK,r['size']-delta):raise RuntimeError('Short memory read')
                f.write(b);h.update(b);hr.update(b)
            regions.append(dict(low=r['low'],size=r['size'],offset=offset,sha256=hr.hexdigest()))
            offset+=r['size']
    return dict(path=str((out/'memory.bin').relative_to(ROOT)),bytes=offset,sha256=h.hexdigest(),regions=regions)

def finish_trace(path,count):
    if path.stat().st_size!=count*16:raise RuntimeError('Trace size mismatch')
    return dict(path=str(path.relative_to(ROOT)),records=count,record_layout='<PC,address,size,value:u32>',sha256=sha(path))

def reference(m,out):
    from cpu_adapter import load_fixed,restore,public_state,check_entry,probe_cached_segments
    u=load_fixed(ROOT,m)
    raw=local(m['context']).read_bytes()
    engine,context=restore(u,raw,m['CPU_tuple_metadata'])
    for r in m['regions']:engine.mem_map(r['low'],r['size'],r['permissions'])
    for address,b in memory_chunks(m):engine.mem_write(address,b)
    # Restore a second time after mapping, without changing saved registers.
    engine.context_restore(context)
    initial=public_state(engine,u);check_entry(initial,m)
    probes=probe_cached_segments(u,raw,m['CPU_tuple_metadata'],m['segment_probe_ranges'])
    if public_state(engine,u)!=initial:raise RuntimeError('Main CPU changed during clone probes')
    allowed={q['pc']:q['size'] for q in m['original_instructions']}
    from unicorn import x86_const as x
    tracepath=out/'writes.bin';records=0;steps=0;pc=0;stopped=False;failure=[]
    deadline=time.monotonic()+m['limits']['guest_seconds']
    with tracepath.open('xb') as trace:
        def reject(uc,message):
            failure.append(message);uc.emu_stop()
        def code(uc,address,size,data):
            nonlocal steps,pc,stopped
            pc=address
            if address==m['stop']['PC']:
                if uc.reg_read(x.UC_X86_REG_ESP)!=m['stop']['SP']:
                    return reject(uc,'Wrong stop SP')
                stopped=True;uc.emu_stop();return
            if allowed.get(address)!=size:return reject(uc,'PC outside fixed instruction closure: '+hex(address))
            steps+=1
            if steps>m['limits']['steps'] or time.monotonic()>deadline:
                reject(uc,'Guest execution budget exceeded')
        def write(uc,access,address,size,value,data):
            nonlocal records
            if size not in (1,2,4) or address>0xffffffff or records>=m['limits']['writes']:
                return reject(uc,'Write trace outside cap/width')
            trace.write(struct.pack('<4I',pc,address,size,value&((1<<(size*8))-1)))
            records+=1
        def invalid(uc,access,address,size,value,data):
            reject(uc,'Original memory fault: '+str((access,hex(address),size)))
            return False
        engine.hook_add(u.UC_HOOK_CODE,code)
        engine.hook_add(u.UC_HOOK_MEM_WRITE,write)
        engine.hook_add(u.UC_HOOK_MEM_INVALID,invalid)
        engine.emu_start(m['entry']['PC'],0,timeout=int(m['limits']['guest_seconds']*1000000),
                         count=m['limits']['steps']+1)
    if failure or not stopped:raise RuntimeError('Reference refused: '+str(failure or ['No exact stop']))
    final=public_state(engine,u)
    sidecar=[k for k in initial if k not in GPRS+('EIP','EFLAGS')]
    if any(initial[k]!=final[k]for k in sidecar):raise RuntimeError('Unexpected x87/segment/control state mutation')
    return dict(status='REFERENCE_EXACT_STOP',initial_CPU=initial,final_CPU=final,
                segment_preflight=probes,steps=steps,writes=finish_trace(tracepath,records),
                memory=dump_memory(m,out,engine.mem_read),x87_segments_unchanged=True,
                original_windows_executed=True,workers_imports_UI=False)

def native(m,out,refpath,msha):
    # This import and every CDLL load occurs only after the one-shot authorization.
    import ctypes as c
    refpath=local(refpath);ref=json.loads(refpath.read_text())
    if ref.get('status')!='REFERENCE_EXACT_STOP' or ref.get('manifest_sha256')!=msha:
        raise RuntimeError('Native requires the successful fixed reference result')
    claim=json.loads((local(m['claim_directory'])/(m['id']+'-reference-attempt.claim.json')).read_text())
    if claim['manifest_sha256']!=msha or refpath.parent!=Path(claim['output']).resolve():
        raise RuntimeError('Reference does not belong to the approved attempt')
    for name in ('writes','memory'):
        if sha(local(ref[name]['path']))!=ref[name]['sha256']:raise RuntimeError('Reference output hash differs')
    if ref['segment_preflight']['status']!='PASSED_BOUNDED_CACHED_SEGMENT_ENDPOINT_PROBES':
        raise RuntimeError('Missing actual segment preflight')
    from cpu_adapter import check_entry
    check_entry(ref['initial_CPU'],m)
    class CPU(c.Structure):
        _fields_=[('r',c.c_uint32*9),('st_top',c.c_uint32),('st_sw_cond',c.c_uint32),
                  ('st_cw',c.c_uint32),('st',c.c_double*8),('pointer_offset',c.c_uint64),
                  ('st_result',c.c_int64),('stack_bottom',c.c_void_p),('stack_top',c.c_void_p)]
    class Region(c.Structure):_fields_=[('low',c.c_uint32),('size',c.c_uint32),('permissions',c.c_uint32)]
    class Return(c.Structure):_fields_=[('pc',c.c_uint32),('target',c.c_void_p)]
    class Result(c.Structure):
        _fields_=[('steps',c.c_uint64),('writes',c.c_uint64),('pc',c.c_uint32),
                  ('status',c.c_uint32),('fault_address',c.c_uint32)]
    if c.sizeof(CPU)!=144 or CPU.pointer_offset.offset!=112 or c.sizeof(Return)!=16 or c.sizeof(Result)!=32:
        raise RuntimeError('Native ABI mismatch')
    lib=c.CDLL(str(local(m['native_library'])))
    count=c.c_uint32.in_dll(lib,'stage72_return_bindings_count').value
    if count!=len(m['return_guest_PCs']):raise RuntimeError('Return binding count differs')
    returns=(Return*count).in_dll(lib,'stage72_return_bindings')
    if [v.pc for v in returns]!=m['return_guest_PCs']:raise RuntimeError('Return PC binding differs')
    targets=[v.target for v in returns]
    if None in targets or len(set(targets))!=count:raise RuntimeError('Ambiguous native continuation target')
    # All original guest mappings start at 4MiB. Put the small translated text
    # in the unused 1MiB guest hole so the common offset remains above macOS's
    # 4GiB PAGEZERO. A 1GiB code slot would frequently put shadow pages below it.
    offset=(min(targets)&~16383)-0x00100000
    if offset<=0 or any(not 0<=p-offset<=0xffffffff for p in targets):
        raise RuntimeError('Native function pointers do not fit common uint32 pointer offset')
    if offset<0x100000000 or any(p-offset>=0x00400000 for p in targets):
        raise RuntimeError('Native continuation hole or macOS low-address protection mismatch')
    if any(r['low']<=p-offset<r['low']+r['size'] for p in targets for r in m['regions']):
        raise RuntimeError('Native continuation aliases an original guest mapping')
    cpu=CPU()
    for i,name in enumerate(GPRS):cpu.r[i]=ref['initial_CPU'][name]
    cpu.r[8]=ref['initial_CPU']['EFLAGS'];cpu.pointer_offset=offset
    # x87 is an unchanged opaque/public sidecar. No 80-bit-to-double guess.
    regions=(Region*len(m['regions']))(*(Region(r['low'],r['size'],r['permissions']) for r in m['regions']))
    lib.stage72_configure.argtypes=[c.POINTER(CPU),c.POINTER(Region),c.c_size_t,c.c_uint64,c.c_uint64,c.c_uint64]
    lib.stage72_configure.restype=c.c_int
    lib.stage72_run.argtypes=[];lib.stage72_run.restype=c.c_int
    lib.stage72_result.argtypes=[];lib.stage72_result.restype=c.POINTER(Result)
    lib.stage72_writes.argtypes=[];lib.stage72_writes.restype=c.c_void_p
    lib.stage72_dispose.argtypes=[];lib.stage72_dispose.restype=None
    system=c.CDLL('/usr/lib/libSystem.B.dylib')
    task=c.c_uint32.in_dll(system,'mach_task_self_').value
    system.mach_vm_allocate.argtypes=[c.c_uint32,c.POINTER(c.c_uint64),c.c_uint64,c.c_int]
    system.mach_vm_allocate.restype=c.c_int
    system.mach_vm_deallocate.argtypes=[c.c_uint32,c.c_uint64,c.c_uint64]
    system.mach_vm_deallocate.restype=c.c_int
    # mach_vm_allocate(flags=0) refuses overlap; it never replaces existing memory.
    # Physical pages are RW and non-executable; logical original permissions are
    # checked by the SR guard before every guest load/store.
    physical=[]
    for r in m['regions']:
        lo=r['low']&~16383;hi=(r['low']+r['size']+16383)&~16383
        if physical and lo<=physical[-1][1]:physical[-1][1]=max(hi,physical[-1][1])
        else:physical.append([lo,hi])
    allocated=[];configured=False
    try:
        for lo,hi in physical:
            address=c.c_uint64(offset+lo)
            rc=system.mach_vm_allocate(task,c.byref(address),hi-lo,0)
            if not rc:allocated.append((address.value,hi-lo))
            if rc or address.value!=offset+lo:raise RuntimeError('Non-overwriting shadow allocation refused: '+str((hex(lo),rc)))
        for address,b in memory_chunks(m):c.memmove(offset+address,b,len(b))
        limit=m['limits'];deadline=time.clock_gettime_ns(time.CLOCK_MONOTONIC)+int(limit['guest_seconds']*1000000000)
        rc=lib.stage72_configure(c.byref(cpu),regions,len(regions),limit['steps'],limit['writes'],deadline)
        if rc:raise RuntimeError('Native configuration refused: '+str(rc))
        configured=True;rc=lib.stage72_run();state=lib.stage72_result().contents
        if rc or state.status or state.pc!=m['stop']['PC'] or cpu.r[4]!=m['stop']['SP']:
            raise RuntimeError('Native run failed: '+str((rc,state.pc,state.status,state.fault_address)))
        tracepath=out/'writes.bin';pointer=lib.stage72_writes()
        if state.writes>limit['writes'] or not pointer:raise RuntimeError('Native trace ABI invalid')
        with tracepath.open('xb')as f:
            for at in range(0,state.writes*16,CHUNK):f.write(c.string_at(pointer+at,min(CHUNK,state.writes*16-at)))
        final=dict(ref['initial_CPU'])
        for i,name in enumerate(GPRS):final[name]=cpu.r[i]
        final['EFLAGS']=cpu.r[8];final['EIP']=state.pc
        result=dict(status='NATIVE_COMPLETED_PENDING_COMPARE',initial_CPU=ref['initial_CPU'],final_CPU=final,
                    steps=state.steps,writes=finish_trace(tracepath,state.writes),
                    memory=dump_memory(m,out,lambda a,n:c.string_at(offset+a,n)),
                    guest_pointer_offset=offset,return_bindings=[dict(PC=v.pc,target=v.target)for v in returns],
                    physical_pages_RW_non_executable=True,full_original_logical_permissions_checked=True,
                    x87_segments_sidecar_passthrough_only=True,
                    invalid_REP_declined_before_store=True,generic_REP_fault_equivalence=False,
                    reference_result_sha256=sha(refpath))
        mismatches=[]
        for name in final:
            if final[name]!=ref['final_CPU'][name]:mismatches.append(dict(kind='CPU',name=name,reference=ref['final_CPU'][name],native=final[name]))
        for name in ('writes','memory'):
            if result[name]['sha256']!=ref[name]['sha256']:
                a=local(ref[name]['path']);b=local(result[name]['path']);first=None;at=0
                with a.open('rb')as fa,b.open('rb')as fb:
                    while True:
                        ba=fa.read(CHUNK);bb=fb.read(CHUNK)
                        if ba!=bb:
                            first=at+next((i for i,(x,y)in enumerate(zip(ba,bb))if x!=y),min(len(ba),len(bb)));break
                        if not ba:break
                        at+=len(ba)
                mismatches.append(dict(kind=name,first_different_byte_offset=first))
        result['mismatches']=mismatches
        result['status']='DIFFERENTIAL_MATCH_EXACT_SCOPE'if not mismatches else 'DIFFERENTIAL_MISMATCH'
        return result
    finally:
        if configured:lib.stage72_dispose()
        for address,size in reversed(allocated):system.mach_vm_deallocate(task,address,size)

def main():
    global MANIFEST
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('role',choices=('reference','native'))
    parser.add_argument('--manifest',required=True)
    parser.add_argument('--run',action='store_true')
    parser.add_argument('--describe',action='store_true')
    parser.add_argument('--approval',default='evidence/stage73-run-approval.json')
    parser.add_argument('--output')
    parser.add_argument('--reference')
    args=parser.parse_args();MANIFEST=local(args.manifest);m=json.loads(MANIFEST.read_text());msha=sha(MANIFEST)
    if args.describe:
        print(json.dumps(dict(role=args.role,manifest_sha256=msha,scope=m['execution_scope'],limits=m['limits'],
                             requires_user_approval=True,runtime_eligible=m['runtime_eligible'],engine_or_native_library_loaded=False),indent=2));return 0
    if not args.run or not args.output or args.role=='native'and not args.reference:
        parser.error('Use --describe, or --run --approval PATH --output NEW_PATH (native also --reference PATH)')
    if not m.get('runtime_eligible',False):
        parser.error('Open closure: execution interface refused before any engine or library load')
    def timeout(signum,frame):raise TimeoutError('45-second local process deadline')
    signal.signal(signal.SIGALRM,timeout);signal.alarm(45)
    out=None
    try:
        verify_pins(m)
        out,approval=authorize(args,m,msha)
        result=reference(m,out)if args.role=='reference'else native(m,out,args.reference,msha)
        # Fail closed if any immutable source changed while the run was active.
        verify_pins(m)
        result.update(manifest_sha256=msha,approval_sha256=sha(local(args.approval)),
                      limits=m['limits'],old_closed_ledger_modified=False,
                      one_shot_claimed=True,repeat_allowed=False)
        write_json(out/'result.json',result)
        print(json.dumps(dict(status=result['status'],result=str(out/'result.json'))))
        return 0 if result['status']in ('REFERENCE_EXACT_STOP','DIFFERENTIAL_MATCH_EXACT_SCOPE')else 1
    except Exception as e:
        failure=dict(status='FAILED_CLOSED',role=args.role,error=repr(e),manifest_sha256=msha,
                     partial_CPU_not_a_checkpoint=True,retry_allowed=False)
        if out is not None:write_json(out/'result.json',failure)
        print(json.dumps(failure),file=sys.stderr);return 1
    finally:signal.alarm(0)

if __name__=='__main__':sys.exit(main())
