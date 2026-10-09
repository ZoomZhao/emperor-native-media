"""Local exact Unicorn checkpoint, plain typed JSON and hashed binary blobs.

No pickle, no executable restoration, and no external input paths. Raw context
uses the serialization protocol of the already reviewed Unicorn 2.1.4 package.
This is a research continuation, not the game's save/restore implementation.
"""
import collections,ctypes,hashlib,json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FIELDS=['api_trace','guid_calls','debug_messages','heaps','allocations','heap_next','files','next_handle','critical_sections','tls','last_error','global_memory','global_next','local_memory','exception_filter','count','trace','visited','writes','last_invalid','loop_hits','samples','progress','api_hook_events','imports','wall_clock_ns','monotonic_origin_ns','parser_returns']

class ContextHeader(ctypes.Structure):
    # Exact fixed 2.1.4 source: include/uc_priv.h struct uc_context.
    _fields_=[('context_size',ctypes.c_size_t),('mode',ctypes.c_int),('arch',ctypes.c_int),('snapshot_level',ctypes.c_int),('ramblock_freed',ctypes.c_bool),('last_block',ctypes.c_void_p),('fv',ctypes.c_void_p)]
assert ctypes.sizeof(ContextHeader)==40

def encode(value):
    if isinstance(value,bytes):return {'bytes':value.hex()}
    if isinstance(value,collections.deque):return {'deque':[encode(v) for v in value],'maxlen':value.maxlen}
    if isinstance(value,dict):return {'dict':[[encode(k),encode(v)] for k,v in value.items()]}
    if isinstance(value,tuple):return {'tuple':[encode(v) for v in value]}
    if isinstance(value,set):return {'set':[encode(v) for v in value]}
    if isinstance(value,list):return [encode(v) for v in value]
    assert value is None or isinstance(value,(str,int,float,bool)),type(value)
    return value

def decode(value):
    if isinstance(value,list):return [decode(v) for v in value]
    if not isinstance(value,dict):return value
    if 'bytes' in value:return bytes.fromhex(value['bytes'])
    if 'deque' in value:return collections.deque((decode(v) for v in value['deque']),maxlen=value['maxlen'])
    if 'dict' in value:return {decode(k):decode(v) for k,v in value['dict']}
    if 'tuple' in value:return tuple(decode(v) for v in value['tuple'])
    if 'set' in value:return set(decode(v) for v in value['set'])
    raise ValueError('Unknown local checkpoint type')

def directory(tag):
    assert tag.replace('-','').isalnum()
    return ROOT/'runs'/('stage12-state-'+tag)

def digest(body):return hashlib.sha256(body).hexdigest()

def field_names(r):
    names=FIELDS+list(getattr(r,'extra_snapshot_fields',()))
    assert len(names)==len(set(names));return names

def save_state(r,tag,phase,milestones):
    from checkpoint_stage12 import REGS,x
    target=directory(tag);target.mkdir(exist_ok=True);regions=[];position=0
    with (target/'memory.bin').open('wb') as stream:
        for low,high,permissions in r.uc.mem_regions():
            body=bytes(r.uc.mem_read(low,high-low+1));stream.write(body)
            regions.append(dict(low=low,high=high,permissions=permissions,offset=position,sha256=digest(body)));position+=len(body)
    context=r.uc.context_save().__getstate__();(target/'context.bin').write_bytes(context[0])
    record=dict(format=1,phase=phase,milestones=milestones,regions=regions,memory_bytes=position,context=dict(size=context[1],arch=context[2],mode=context[3],sha256=digest(context[0])),host=encode({field:getattr(r,field) for field in field_names(r)}),registers={n:r.uc.reg_read(reg) for n,reg in dict(REGS,eflags=x.UC_X86_REG_EFLAGS).items()},original_EXE_sha256=digest(r.pe.data),save_sha256=digest(r.save),model_sha256=digest(r.model_payload),sparse=r.sparse)
    (target/'state.json').write_text(json.dumps(record,separators=(',',':'))+'\n')
    return record

def restore_state(r,tag):
    from checkpoint_stage12 import REGS,x,u
    target=directory(tag);record=json.loads((target/'state.json').read_text());assert record['format']==1
    assert record['original_EXE_sha256']==digest(r.pe.data) and record['save_sha256']==digest(r.save) and record['model_sha256']==digest(r.model_payload)
    actual=list(r.uc.mem_regions());assert actual==[(s['low'],s['high'],s['permissions']) for s in record['regions']]
    with (target/'memory.bin').open('rb') as stream:
        for section in record['regions']:
            assert stream.tell()==section['offset'];body=stream.read(section['high']-section['low']+1);assert digest(body)==section['sha256']
            r.uc.mem_write(section['low'],body);assert digest(bytes(r.uc.mem_read(section['low'],len(body))))==section['sha256']
        assert not stream.read(1)
    body=(target/'context.bin').read_bytes();meta=record['context'];assert digest(body)==meta['sha256'] and len(body)==meta['size']
    context_class=type(r.uc.context_save())
    context=context_class.__new__(context_class);context.__setstate__((body,meta['size'],meta['arch'],meta['mode']));r.uc.context_restore(context)
    restored=decode(record['host'])
    for field,value in restored.items():assert field in field_names(r);setattr(r,field,value)
    assert all(getattr(r,field)==value for field,value in restored.items()),'all semantic host fields must restore exactly'
    assert record['registers']=={n:r.uc.reg_read(reg) for n,reg in dict(REGS,eflags=x.UC_X86_REG_EFLAGS).items()}
    # In CPU-only mode, source uc.c copies exactly context->data. Header
    # ramblock_freed/last_block are not initialized by context_alloc/save;
    # they are never CPU state. Compare every byte of the entire CPU payload.
    actual_context=bytes(r.uc.context_save())
    header=ContextHeader.from_buffer_copy(body[:40]);actual_header=ContextHeader.from_buffer_copy(actual_context[:40])
    assert header.context_size==actual_header.context_size==len(body)-40
    assert header.mode==actual_header.mode and header.arch==actual_header.arch and not header.fv and not actual_header.fv
    if actual_context[40:]!=body[40:]:
        difference=[i for i,(a,b) in enumerate(zip(actual_context,body)) if a!=b]
        (ROOT/f'evidence/stage12-context-difference-{tag}.json').write_text(json.dumps(dict(size=len(body),difference_offsets=difference,expected_ranges=[dict(offset=i,expected=body[max(0,i-16):i+17].hex(),actual=actual_context[max(0,i-16):i+17].hex()) for i in difference[:16]],registers_equal=True),indent=2)+'\n')
        raise AssertionError('complete CPU payload differs; inspect exact offsets before continuation')
    if record['sparse']:r.enable_sparse()
    (ROOT/f'evidence/stage12-state-restore-{tag}.json').write_text(json.dumps(dict(all_passed=True,registers_equal=True,entire_CPU_context_equal=True,CPU_payload_bytes=len(body)-40,all_mapped_memory_hashes_equal=True,memory_bytes=record['memory_bytes'],restored_pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),phase=record['phase'],EXE_save_model_hashes_equal=True,game_save_restore=False,context_header_note='40-byte engine metadata excluded; CPU-only source leaves ramblock_freed/last_block uninitialized. Entire CPU payload including segments/FPU/internal state matches.',source='https://github.com/unicorn-engine/unicorn/blob/2.1.4/include/uc_priv.h#L402'),indent=2)+'\n')
    return record
