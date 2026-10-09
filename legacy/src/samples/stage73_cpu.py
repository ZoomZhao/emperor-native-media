"""Future approved execution only. No engine import or API call at module import.

The saved CPU is restored as an entire official serialized tuple. Hidden field
offsets are never inspected. Segment probes execute only the authored bytes below
in separate empty engines, preserving the main engine and its CPU.
"""
import hashlib
import importlib
import os
import sys
from pathlib import Path

NAMES = ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI','EIP','EFLAGS',
         'CS','DS','SS','ES','FS','GS','CR0','GDTR','LDTR','FS_BASE','GS_BASE',
         'FPCW','FPSW','FPTAG')

def load_fixed(root, manifest):
    vendor = (root / manifest['unicorn_package']).resolve()
    library = (root / manifest['unicorn_library']).resolve()
    os.environ['LIBUNICORN_PATH'] = str(library.parent)  # This process only.
    if 'unicorn' in sys.modules:
        raise RuntimeError('Refuse a previously imported engine')
    sys.path.insert(0, str(vendor))
    import unicorn as u
    binding = importlib.import_module('unicorn.unicorn_py3.unicorn')
    if not Path(u.__file__).resolve().is_relative_to(vendor):
        raise RuntimeError('Unexpected engine package')
    if Path(binding.uclib._name).resolve() != library:
        raise RuntimeError('Unexpected loaded engine dylib')
    if u.__version__ != '2.1.4' or u.uc_version()[:2] != (2,1):
        raise RuntimeError('Unexpected engine version')
    return u

def restore(u, raw, meta):
    from unicorn.unicorn_py3.unicorn import UcContext
    if len(raw) != meta['size'] or meta['size'] != 5608:
        raise RuntimeError('Saved CPU ABI size mismatch')
    if (meta['arch'],meta['mode']) != (u.UC_ARCH_X86,u.UC_MODE_32):
        raise RuntimeError('Saved CPU architecture mismatch')
    ctx = UcContext.__new__(UcContext)
    ctx.__setstate__((raw,meta['size'],meta['arch'],meta['mode']))
    engine = u.Uc(meta['arch'],meta['mode'])
    if engine.context_save().size != meta['size']:
        raise RuntimeError('Engine CPU ABI size mismatch')
    engine.context_restore(ctx)
    return engine, ctx

def public_state(engine, u):
    from unicorn import x86_const as x
    state = {n:engine.reg_read(getattr(x,'UC_X86_REG_'+n)) for n in NAMES}
    state['FP80'] = [engine.reg_read(getattr(x,'UC_X86_REG_FP'+str(i))) for i in range(8)]
    # Lists give identical JSON representations without changing values.
    return {k:list(v) if isinstance(v,tuple) else
            [list(p) if isinstance(p,tuple) else p for p in v] if isinstance(v,list) else v
            for k,v in state.items()}

def check_entry(state, manifest):
    if state['EIP'] != manifest['entry']['PC'] or state['ESP'] != manifest['entry']['SP']:
        raise RuntimeError('Actual public PC/SP differs from fixed continuation')
    if any(state[n] != v for n,v in {'CS':8,'DS':16,'ES':16,'SS':16}.items()):
        raise RuntimeError('Unexpected segment selectors')
    if state['GDTR'][1] != 0x60000000 or state['GDTR'][2] < 31:
        raise RuntimeError('Unexpected GDT')
    if state['CR0'] & 0x80000000 or state['EFLAGS'] & ((1<<17)|(1<<8)):
        raise RuntimeError('Paging, virtual mode or single stepping outside scope')

def probe_cached_segments(u, raw, meta, ranges):
    from unicorn import x86_const as x
    code_base = 0x6f000000
    results = []
    for name,prefix in [('DS',0x3e),('ES',0x26),('SS',0x36)]:
        low,high = ranges[name]
        for target in (low,high-4):
            for opcode,expected in [(0xa1,u.UC_MEM_READ_UNMAPPED),(0xa3,u.UC_MEM_WRITE_UNMAPPED)]:
                engine,ctx = restore(u,raw,meta)
                engine.mem_map(code_base,4096,u.UC_PROT_ALL)
                body = bytes([prefix,opcode])+target.to_bytes(4,'little')
                engine.mem_write(code_base,body)
                engine.mem_protect(code_base,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
                # Only this empty probe clone is changed. No segment write.
                engine.reg_write(x.UC_X86_REG_EIP,code_base)
                engine.reg_write(x.UC_X86_REG_EAX,0x73535373)
                observed = []
                def invalid(uc,access,address,size,value,data):
                    observed.append((access,address,size))
                    return False
                engine.hook_add(u.UC_HOOK_MEM_INVALID,invalid)
                error = None
                try:
                    engine.emu_start(code_base,code_base+len(body),timeout=100000,count=1)
                except u.UcError as e:
                    error = str(e)
                if error is None or observed != [(expected,target,4)]:
                    raise RuntimeError('Cached segment probe refused: '+str((name,target,observed,error)))
                results.append(dict(segment=name,logical_address=target,linear_address=target,
                                    access='read' if opcode==0xa1 else 'write',bytes=body.hex(),
                                    instructions_maximum=1,expected_unmapped_fault=True))
                del engine,ctx
    return dict(status='PASSED_BOUNDED_CACHED_SEGMENT_ENDPOINT_PROBES',
                authored_instruction_maximum=12,original_probe_instructions=0,
                ranges=ranges,results=results,main_CPU_not_mutated=True,
                general_segment_fault_equivalence=False)
