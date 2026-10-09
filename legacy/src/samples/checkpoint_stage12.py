#!/usr/bin/env python3
"""Diagnose and continue the exact original machine; observations do not patch it."""
import argparse,collections,copy,hashlib,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage11 import GuidRuntime
from stage7_runtime import Boundary,SCRATCH,GUEST_NAME,SP,TEB,THUNKS,SENTINEL,x,u

REGS={'eax':x.UC_X86_REG_EAX,'ecx':x.UC_X86_REG_ECX,'edx':x.UC_X86_REG_EDX,'ebx':x.UC_X86_REG_EBX,'esp':x.UC_X86_REG_ESP,'ebp':x.UC_X86_REG_EBP,'esi':x.UC_X86_REG_ESI,'edi':x.UC_X86_REG_EDI,'eip':x.UC_X86_REG_EIP,'eflags':x.UC_X86_REG_EFLAGS}

class DiagnosticRuntime(GuidRuntime):
    def __init__(self,save):
        self.loop_hits=0;self.samples=[];self.progress=[];self.sparse=False;self.api_hook_events=0;self.parser_returns=[]
        super().__init__(save,model_input=True)
    def code_hook(self,uc,address,size,user):
        super().code_hook(uc,address,size,user)
        if address==0x526668:
            self.loop_hits+=1
            if self.loop_hits%8192==1:self.sample_text('full-trace-loop')
    def sample_text(self,reason):
        regs={n:self.uc.reg_read(reg) for n,reg in REGS.items()};sp=regs['esp'];base=regs['ebp'];object=regs['ebx']
        row=dict(reason=reason,registers={n:hex(v) for n,v in regs.items()},loop_hits=self.loop_hits,api_calls=len(self.api_trace),GUID_count=len(self.guid_calls),files=[dict(handle=hex(h),position=f['position'],payload_size=len(f['payload'])) for h,f in self.files.items()])
        # Only this exact instruction has the unshifted parser stack frame.
        # During strcmp calls even PCs in this function can have pushed args.
        if regs['eip'] in (0x526668,0x526669,0x526738):
            try:
                row.update(parser_object=hex(object),text_base=hex(base),input_index=regs['esi'],pass_flag=self.readword(sp+0x14),output_index=self.readword(sp+0x24),declared_length=self.readword(sp+0x20),raw_stack_slot_28=hex(self.readword(sp+0x28)),object_header_hex=bytes(self.uc.mem_read(object,20)).hex())
                length=row['declared_length'];self.require_region(base,length,u.UC_PROT_READ,'diagnostic text snapshot')
                if length<=2*1024*1024:
                    body=bytes(self.uc.mem_read(base,length));row['text_buffer_sha256']=hashlib.sha256(body).hexdigest();row['input_equal_reviewed_model']=body==self.model_payload
                    changes=[i for i,(actual,original) in enumerate(zip(body,self.model_payload)) if actual!=original]
                    row['tokenized_input_changes']=dict(count=len(changes),all_are_original_whitespace_to_NUL=all(body[i]==0 and self.model_payload[i]<=32 for i in changes),first_changes=[dict(offset=i,original=self.model_payload[i],actual=body[i]) for i in changes[:8]])
                pointer=self.readword(object+8)
                if pointer:
                    size=min(row['output_index']+1,4096);self.require_region(pointer,size,u.UC_PROT_READ,'diagnostic output sample')
                    row['output_prefix_sha256']=hashlib.sha256(bytes(self.uc.mem_read(pointer,size))).hexdigest()
            except (Boundary,u.UcError) as error:row['diagnostic_read_error']=str(error)
        self.samples.append(row)
    def enable_sparse(self):
        assert not self.sparse
        self.uc.hook_del(self.code_hook_handle);self.uc.hook_del(self.write_hook_handle)
        self.uc.hook_add(u.UC_HOOK_CODE,self.sparse_api_hook,begin=THUNKS,end=THUNKS+65535)
        for pc,kind in [(0x7688ba,'forbidden_complete_EXE_entry'),(0x77a9e9,'unimplemented_exception_dispatch')]:
            self.uc.hook_add(u.UC_HOOK_CODE,self.forbidden_hook,user_data=kind,begin=pc,end=pc)
        self.uc.hook_add(u.UC_HOOK_CODE,self.sparse_text_hook,begin=0x526668,end=0x526668)
        self.uc.hook_add(u.UC_HOOK_CODE,self.parser_terminal_hook,begin=0x526738,end=0x526738)
        self.uc.hook_add(u.UC_HOOK_CODE,self.parser_return_hook,begin=0x52674a,end=0x52674a)
        self.uc.hook_add(u.UC_HOOK_BLOCK,self.validate_block)
        self.sparse=True
    def validate_block(self,uc,address,size,user):
        if THUNKS<=address<THUNKS+65536:return
        if address==SENTINEL:return
        section=self.pe.section(address)
        if not section or not section['flags']&0x20000000 or address+size>section['va']+section['size']:
            raise Boundary('invalid_original_code_block',address=hex(address),size=size)
    def sparse_api_hook(self,uc,address,size,user):
        self.api_hook_events+=1
        if address not in self.imports:raise Boundary('unknown_platform_thunk',address=hex(address))
        self.trace.append(address);self.api(*self.imports[address])
    def forbidden_hook(self,uc,address,size,kind):raise Boundary(kind,address=hex(address))
    def sparse_text_hook(self,uc,address,size,user):
        self.loop_hits+=1
        if self.loop_hits%8192==1:self.sample_text('sparse-loop')
    def parser_terminal_hook(self,uc,address,size,user):self.sample_text('original-parser-before-free')
    def parser_return_hook(self,uc,address,size,user):
        object=uc.reg_read(x.UC_X86_REG_EAX);sp=uc.reg_read(x.UC_X86_REG_ESP)
        self.parser_returns.append(dict(original_ret_pc=hex(address),return_object=hex(object),caller_pc=hex(self.readword(sp)),object_header_hex=bytes(uc.mem_read(object,20)).hex(),descriptor_count=self.readword(object+12),output_pointer=hex(self.readword(object+8))))
    def segment(self,entry=None,args=(),this=0,instruction_limit=500000):
        if entry is not None:
            sp=SP-4*(len(args)+1);self.uc.mem_write(sp,struct.pack('<'+str(len(args)+1)+'I',SENTINEL,*args));self.uc.reg_write(x.UC_X86_REG_ESP,sp);self.uc.reg_write(x.UC_X86_REG_ECX,this);pc=entry
        else:pc=self.uc.reg_read(x.UC_X86_REG_EIP)
        self.count=0;self.trace.clear();self.last_invalid=None;before=len(self.api_trace);started=time.monotonic()
        item=dict(start_pc=hex(pc),sparse_hooks=self.sparse,engine_instruction_limit=instruction_limit,engine_timeout_us=1000000)
        try:
            self.uc.emu_start(pc,SENTINEL,timeout=1000000,count=instruction_limit)
            actual=self.uc.reg_read(x.UC_X86_REG_EIP)
            item.update(status='returned' if actual==SENTINEL else 'budget_yield',end_pc=hex(actual),engine_timeout=bool(self.uc.query(u.UC_QUERY_TIMEOUT)))
        except (Boundary,u.UcError) as error:
            item.update(status='refused',boundary=error.details if isinstance(error,Boundary) else (self.last_invalid or dict(kind='Unicorn_error',text=str(error))),end_pc=hex(self.uc.reg_read(x.UC_X86_REG_EIP)))
            pc=self.uc.reg_read(x.UC_X86_REG_EIP)
            if pc in self.imports:
                item['boundary']['guest_return_pc']=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)))
                item['boundary']['platform_import']=list(self.imports[pc])
                if self.imports[pc][1]=='CreateEventA':item['boundary']['args']=[hex(v) for v in self.args(4)]
        item.update(elapsed_seconds=round(time.monotonic()-started,6),full_hook_events=self.count if not self.sparse else None,registers={n:hex(self.uc.reg_read(reg)) for n,reg in REGS.items()},new_API_calls=self.api_trace[before:],last_trace=[hex(v) for v in self.trace])
        self.progress.append(item)
        if 0x52655d<=self.uc.reg_read(x.UC_X86_REG_EIP)<=0x526718:self.sample_text('segment-end')
        return item

    def check_sparse_equivalence(self):
        # In-memory snapshot only. Restore the same CPU, writable guest bytes
        # and semantic host state; no call-stack reset and no reinitialization.
        memory=[(lo,bytes(self.uc.mem_read(lo,hi-lo+1))) for lo,hi,permissions in self.uc.mem_regions() if permissions&u.UC_PROT_WRITE]
        context=self.uc.context_save()
        fields=['api_trace','guid_calls','debug_messages','heaps','allocations','heap_next','files','next_handle','critical_sections','tls','last_error','global_memory','global_next','local_memory','exception_filter','count','trace','visited','writes','last_invalid','loop_hits','samples','progress','api_hook_events']
        host={field:copy.deepcopy(getattr(self,field)) for field in fields};before_apis=len(self.api_trace)
        full=self.segment();after_memory=[(lo,hashlib.sha256(bytes(self.uc.mem_read(lo,len(body)))).hexdigest()) for lo,body in memory]
        reference_fields={field:copy.deepcopy(getattr(self,field)) for field in fields[:16]}
        reference_APIs=[(a['dll'],a['name'],a.get('args')) for a in self.api_trace[before_apis:]]
        assert not any(name in ['CoCreateGuid','GetTickCount','GetSystemTimeAsFileTime','QueryPerformanceCounter'] for dll,name,args in reference_APIs),'nondeterministic input would invalidate trace comparison'
        for lo,body in memory:self.uc.mem_write(lo,body)
        self.uc.context_restore(context)
        for field,value in host.items():setattr(self,field,value)
        self.enable_sparse();sparse=self.segment(instruction_limit=full['full_hook_events'])
        sparse_memory=[(lo,hashlib.sha256(bytes(self.uc.mem_read(lo,len(body)))).hexdigest()) for lo,body in memory]
        actual_APIs=[(a['dll'],a['name'],a.get('args')) for a in self.api_trace[before_apis:]]
        semantic_fields=['heaps','allocations','heap_next','files','next_handle','critical_sections','tls','last_error','global_memory','global_next','local_memory','exception_filter']
        host_equal=all(getattr(self,field)==reference_fields[field] for field in semantic_fields)
        # Trace bookkeeping differs intentionally; compare actual semantic state.
        result=dict(full_segment=full,sparse_segment=sparse,registers_equal=full['registers']==sparse['registers'],writable_memory_sha256_equal=after_memory==sparse_memory,semantic_host_state_equal=host_equal,API_names_and_arguments_equal=reference_APIs==actual_APIs,snapshot_memory_bytes=sum(len(body) for lo,body in memory),instruction_steps_compared=full['full_hook_events'],nondeterministic_API_inputs=False)
        result['all_passed']=all(result[k] for k in ['registers_equal','writable_memory_sha256_equal','semantic_host_state_equal','API_names_and_arguments_equal'])
        (ROOT/'evidence/stage12-sparse-equivalence.json').write_text(json.dumps(result,indent=2)+'\n')
        if not result['all_passed']:raise Boundary('trace_equivalence_failed')
        return sparse

def main():
    from stage12_state import save_state,restore_state
    p=argparse.ArgumentParser();p.add_argument('--tag',default='diagnostic');p.add_argument('--save',choices=['ST9BASE','ST9AFTR'],default='ST9BASE');p.add_argument('--diagnostic-only',action='store_true');p.add_argument('--resume-state');a=p.parse_args();assert a.tag.replace('-','').isalnum()
    start=time.monotonic();r=DiagnosticRuntime(a.save);r.uc.mem_write(SCRATCH,GUEST_NAME+b'\0');milestones=[];resumed=False;phase='CRT initializer table'
    if a.resume_state:
        state=restore_state(r,a.resume_state);resumed=True;phase=state['phase'];milestones=state['milestones'];s=r.segment()
    else:
        for name,entry,args in [('CRT heap',0x76db30,(1,)),('CRT TLS/locks',0x76c7fc,()),('CRT IO',0x76efc1,())]:
            s=r.segment(entry,args);milestones.append(dict(name=name,result=s));assert s['status']=='returned'
        s=r.segment(0x769f80);segments=1
        while s['status']=='budget_yield' and segments<13:
            assert time.monotonic()-start<30;s=r.segment();segments+=1
    # Before changing instrumentation or budget, inspect actual sampled input
    # indices. A change is justified only by an observed progressing parser.
    seen=[sample for sample in r.samples if 'input_index' in sample];normal=any(b['input_index']>a['input_index'] and b['pass_flag']==a['pass_flag'] for a,b in zip(seen,seen[1:]))
    diagnostic=dict(parser_samples=len(seen),observed_input_index_forward_progress=normal,read_file_bytes=sum(a.get('bytes_read',0) for a in r.api_trace if a['name']=='ReadFile'),current_pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP)))
    (ROOT/f'evidence/stage12-diagnostic-{a.tag}.json').write_text(json.dumps(dict(diagnostic=diagnostic,samples=r.samples,progress=r.progress),indent=2)+'\n')
    if not a.diagnostic_only and s['status']=='budget_yield':
        if not normal:raise Boundary('no_proven_parser_progress',diagnostic=diagnostic)
        if not r.sparse:s=r.check_sparse_equivalence()
        last_fingerprint=None;same=0
        for segment in range(1024):
            if time.monotonic()-start>40:break
            s=r.segment();fingerprint=(s['end_pc'],tuple(s['registers'].values()),len(r.api_trace),r.loop_hits)
            same=same+1 if fingerprint==last_fingerprint else 0;last_fingerprint=fingerprint
            if same>=3:raise Boundary('no_machine_progress_across_sparse_segments')
            if s['status']!='budget_yield':break
    milestones.append(dict(name=phase,result=s))
    if s['status']=='returned' and phase=='CRT initializer table':
        phase='city load root'
        s=r.segment(0x534a30,(SCRATCH,1));milestones.append(dict(name='city load root entered',result=s))
        for segment in range(64):
            if s['status']!='budget_yield' or time.monotonic()-start>40:break
            s=r.segment()
        milestones.append(dict(name='city load root final',result=s))
    if s['status']=='budget_yield' or (s['status']=='refused' and s.get('boundary',{}).get('kind')=='unimplemented_API'):
        save_state(r,a.tag,phase,milestones)
    result=dict(save=a.save,elapsed_seconds=round(time.monotonic()-start,3),resumed_disk_state=resumed,backend='Unicorn i386 original-byte reference; complete native initialization not implemented',original_instruction_bytes_patched=False,deterministic_GUID_injected=False,GUID_count=len(r.guid_calls),diagnostic=diagnostic,trace_optimization=r.sparse,samples=r.samples,parser_returns=r.parser_returns,segments=r.progress,milestones=milestones,final_status=s['status'],final_boundary=s.get('boundary'),final_registers=s['registers'],original_city_root_entered=any(m['name']=='city load root entered' for m in milestones),full_city_loaded=False,original_save_restore_passed=False,native_city_passed=False,natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8),note='Budget yields are continuations of the exact CPU/stack/REP state. Sparse trace hooks still trap IAT, forbidden EXE entry, exception dispatch and invalid memory; no logical shortcut.')
    (ROOT/f'evidence/stage12-checkpoint-{a.tag}.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['elapsed_seconds','GUID_count','diagnostic','trace_optimization','final_status','final_boundary','original_city_root_entered','full_city_loaded','native_city_passed']},ensure_ascii=False))

if __name__=='__main__':main()
