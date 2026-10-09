"""Format1-compatible encoder and staged checkpoint; no original/runtime imports."""
import collections,hashlib,json,os,time
from pathlib import Path

def fast_encode(value):
    if value is None or isinstance(value,(str,int,float,bool)):return value
    if isinstance(value,bytes):return {'bytes':value.hex()}
    if isinstance(value,collections.deque):return {'deque':[fast_encode(v) for v in value],'maxlen':value.maxlen}
    if isinstance(value,dict):return {'dict':[[fast_encode(k),fast_encode(v)] for k,v in value.items()]}
    if isinstance(value,tuple):return {'tuple':[fast_encode(v) for v in value]}
    if isinstance(value,set):return {'set':[fast_encode(v) for v in value]}
    if isinstance(value,list):return [fast_encode(v) for v in value]
    raise TypeError(type(value).__name__)

def budget_seconds(elapsed,maximum=8,reserve=25):
    if elapsed<0 or not 0<maximum<=8 or reserve<25:raise ValueError('invalid whole-process timing contract')
    return max(0,min(maximum,60-elapsed-reserve))

def digest(b):return hashlib.sha256(b).hexdigest()
def durable_write(path,body):
    with path.open('xb') as f:f.write(body);f.flush();os.fsync(f.fileno())

def save_checkpoint(r,tag,milestones,root,regs,x,field_names,mark,fault=None):
    root=Path(root);assert tag.replace('-','').isalnum()
    final=root/'runs'/('stage12-state-'+tag);pending=final.with_name(final.name+'.stage63-pending')
    if final.exists() or pending.exists():raise ValueError('unique checkpoint destination required')
    pending.mkdir();regions=[];position=0;whole=hashlib.sha256();mark('MEMORY_SAVE_BEGIN')
    with (pending/'memory.bin').open('xb') as f:
        for low,high,permissions in r.uc.mem_regions():
            body=bytes(r.uc.mem_read(low,high-low+1));f.write(body);whole.update(body)
            regions.append(dict(low=low,high=high,permissions=permissions,offset=position,sha256=digest(body)));position+=len(body)
        f.flush();os.fsync(f.fileno())
    mark('MEMORY_SAVE_END',bytes=position)
    if fault=='after_memory':raise RuntimeError('authored interrupted-save fixture')
    context=r.uc.context_save().__getstate__();durable_write(pending/'context.bin',context[0]);mark('CONTEXT_SAVE_END')
    values={};metrics=[]
    for field in field_names(r):
        start=time.monotonic();values[field]=getattr(r,field);metrics.append(dict(field=field,getter_seconds=time.monotonic()-start))
    mark('HOST_GET_END',slow_getters=sorted(metrics,key=lambda q:q['getter_seconds'],reverse=True)[:10])
    encoded=fast_encode(values);mark('HOST_ENCODE_END')
    record=dict(format=1,phase='bounded original scheduler continuation; same state/input; staged complete checkpoint',
        milestones=milestones,regions=regions,memory_bytes=position,
        context=dict(size=context[1],arch=context[2],mode=context[3],sha256=digest(context[0])),
        host=encoded,registers={n:r.uc.reg_read(reg) for n,reg in dict(regs,eflags=x.UC_X86_REG_EFLAGS).items()},
        original_EXE_sha256=digest(r.pe.data),save_sha256=digest(r.save),model_sha256=digest(r.model_payload),sparse=r.sparse)
    body=(json.dumps(record,separators=(',',':'),check_circular=False)+'\n').encode('utf-8');mark('STATE_JSON_END',bytes=len(body))
    durable_write(pending/'state.json',body);mark('STATE_FILE_FSYNC_END')
    directory_fd=os.open(pending,os.O_RDONLY)
    try:os.fsync(directory_fd)
    finally:os.close(directory_fd)
    os.rename(pending,final)
    parent_fd=os.open(final.parent,os.O_RDONLY)
    try:os.fsync(parent_fd)
    finally:os.close(parent_fd)
    mark('CHECKPOINT_DIRECTORY_COMMITTED',path=str(final))
    # Hash emitted bytes already in RAM / in the memory write loop; no second disk scan.
    hashes=[dict(path=str((final/name).relative_to(root)),bytes=n,sha256=h) for name,n,h in
        [('context.bin',len(context[0]),digest(context[0])),('memory.bin',position,whole.hexdigest()),('state.json',len(body),digest(body))]]
    return record,hashes
