"""Future-only public Unicorn CPU adapter. Import has no Unicorn side effects.

This module does not emulate, load an SR library, or decode opaque CPU offsets.
Its read function must not be invoked under the current build-only scope.
"""
import hashlib

class PreflightRefusal(RuntimeError):pass

REQUIRED_PROOFS=('DS_SS_ES_cached_flat_bases','original_guest_to_native_data_binding',
    'guest_return_PC_to_SR_continuation_binding','ordered_memory_write_trace',
    'SR_REP_fault_and_persistence_contract','exact_CPU_and_x87_sidecar_contract')

def require_execution_proofs(proofs):
    """Refuse guessed bases or incomplete SR checkpoint ABI before any execution."""
    missing=[name for name in REQUIRED_PROOFS if not proofs.get(name)]
    if missing:raise PreflightRefusal('Execution refused; unproven contracts: '+', '.join(missing))

def read_tid1_public(saved_context):
    """Restore a complete saved tuple and read architectural state through APIs.

    Future explicit engine-use scope only. No emu_start, DLL loading, register
    initialization, direct ctypes buffer parsing or opaque-field offset reads.
    Returned selectors/GDT do not establish hidden cached DS/SS/ES bases.
    """
    raw,size,arch,mode=saved_context
    if len(raw)!=size or size!=5608:raise PreflightRefusal('Saved CPU size mismatch')
    if hashlib.sha256(raw).hexdigest()!='b54a9cd33652de5047cda6fbb2288045e8c2cf76805117e02507b6a34690acb8':
        raise PreflightRefusal('Not the fixed historical main tid1 context')
    # Lazy imports: this file can be read/compiled without importing Unicorn.
    import unicorn
    from unicorn import x86_const as x
    if unicorn.__version__!='2.1.4':raise PreflightRefusal('Unicorn version mismatch')
    if arch!=unicorn.UC_ARCH_X86 or mode!=unicorn.UC_MODE_32:raise PreflightRefusal('Saved CPU architecture/mode mismatch')
    context=unicorn.UcContext.__new__(unicorn.UcContext)
    context.__setstate__((raw,size,arch,mode)) # Entire official binding serialized state.
    engine=unicorn.Uc(arch,mode)
    if engine.context_save().size!=size:raise PreflightRefusal('Engine context ABI size mismatch')
    engine.context_restore(context)
    names=('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI','EIP','EFLAGS',
        'CS','DS','SS','ES','FS','GS','CR0','GDTR','LDTR','FS_BASE','GS_BASE','FPCW','FPSW','FPTAG')
    state={name:engine.reg_read(getattr(x,'UC_X86_REG_'+name)) for name in names}
    state['FP80']=[engine.reg_read(getattr(x,'UC_X86_REG_FP'+str(i)))for i in range(8)]
    if state['EIP']!=0x4af311 or state['ESP']!=0x700ffd68:raise PreflightRefusal('Public PC/SP differs from saved continuation')
    return dict(public_architectural_state=state,DF=bool(state['EFLAGS']&0x400),
        SR_integer_prefix=[state[name]for name in names[:8]]+[state['EFLAGS']],
        original_opaque_sidecar=bytes(raw),cached_DS_SS_ES_bases_verified=False,
        engine_not_emulated=True,original_guest_instructions_executed=0)

def native_execution(*args,**kwargs):
    raise PreflightRefusal('No native execution implementation: SR data/return/fault ABI remains unproven')
