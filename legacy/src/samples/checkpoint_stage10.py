#!/usr/bin/env python3
"""Bounded fixed-English original loader checkpoint, no fabricated game state."""
import argparse
import hashlib
import json
import struct
import sys
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'samples'))
from stage7_runtime import Runtime, Boundary, SCRATCH, GUEST_NAME, TEB, x, u
from stage10_platform import NlsProfile, UnsupportedNls, VersionProfile


class LoaderRuntime(Runtime):
    def __init__(self, save='ST9BASE'):
        record=json.loads((ROOT/f'evidence/stage9-{save.lower()}-file.json').read_text())
        self.profile=json.loads((ROOT/'evidence/stage10-platform-profile.json').read_text())
        self.nls=NlsProfile()
        self.version=VersionProfile()
        self.profile.update(OS_version_measured_same_Wine=True,OS_version_fields=self.version.fields,GetVersion=hex(self.version.packed),OS_service_pack='Service Pack 1',old_research_XP_profile_replaced=True)
        self.exception_filter=0
        self.profile.update(Wine_GetACP_return_directly_instrumented=True,GetCPInfo_measured_matrix_sha256='738feb4bc3b767c6beb8f990b0819be4d4d52aec1c70ead558651ebdebd2d944')
        super().__init__(ROOT/f'stage9-data/{save}.sav', record['sha256'])
        self.global_memory={};self.global_next=0x64000002;self.global_heap=0x3f00
        self.local_memory={}
        self.heaps[self.global_heap]=dict(options=0)

    def require_region(self,start,size,permission,operation):
        if size==0:return
        end=start+size;cursor=start
        for lo,hi,permissions in sorted(self.uc.mem_regions()):
            if lo<=cursor<=hi and (permissions & permission)==permission:
                cursor=min(end,hi+1)
                if cursor==end:break
        if not start or end>0x100000000 or cursor!=end:
            raise Boundary('unsupported_invalid_NLS_pointer',operation=operation,address=hex(start),size=size,note='Invalid-pointer exception semantics are not fabricated')

    def api(self,dll,name):
        if dll.lower()=='ole32.dll' and name=='CoCreateGuid':
            a=self.args(1)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),contract='Unimplemented; no GUID, return value or success path fabricated')
            self.api_trace.append(item);raise Boundary('unimplemented_API',**item)
        elif dll.lower()=='kernel32.dll' and name=='GetVersion':
            self.api_trace.append(dict(dll=dll,name=name,args=[],return_value=hex(self.version.packed),contract='measured same-Wine 6.1 build7601; agrees with GetVersionExA'))
            self.ret(self.version.packed,0)
        elif dll.lower()=='kernel32.dll' and name=='GetVersionExA':
            a=self.args(1);self.require_region(a[0],4,u.UC_PROT_READ,'GetVersionExA size')
            size=self.readword(a[0]);outcome=self.version.info(size)
            for offset,payload in outcome['writes']:self.require_region(a[0]+offset,len(payload),u.UC_PROT_WRITE,'GetVersionExA output')
            for offset,payload in outcome['writes']:self.uc.mem_write(a[0]+offset,payload)
            if outcome['last_error'] is not None:self.last_error=outcome['last_error']
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(a[0])],declared_size=size,return_value=outcome['return_value'],last_error=self.last_error,writes=[dict(offset=offset,size=len(payload)) for offset,payload in outcome['writes']],contract='measured same-Wine fields, exact structure variants and preserved unused tail'))
            self.ret(outcome['return_value'],1)
        elif dll.lower()=='kernel32.dll' and name=='InterlockedExchange':
            a=self.args(2)
            if a[0]&3:raise Boundary('unaligned_InterlockedExchange',address=hex(a[0]))
            self.require_region(a[0],4,u.UC_PROT_READ|u.UC_PROT_WRITE,'InterlockedExchange LONG')
            previous=self.readword(a[0]);self.word(a[0],a[1])
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),return_value=hex(previous),new_value=hex(a[1]),contract='32-bit atomic exchange in explicitly single-thread guest domain'))
            self.ret(previous,2)
        elif dll.lower()=='kernel32.dll' and name=='HeapReAlloc':
            a=self.args(4);item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
            self.api_trace.append(item);heap,flags,pointer,size=a
            if flags&~9:raise Boundary('unimplemented_HeapReAlloc_flags',**item)
            old=self.allocations.get(pointer)
            if not old or old[0]!=heap:raise Boundary('invalid_HeapReAlloc_allocation',**item)
            # This bounded allocator deliberately permits relocation. Copy all
            # retained bytes and free the old ownership only after success.
            saved_error=self.last_error;new=self.allocate(heap,flags,size);self.last_error=saved_error
            if new:
                keep=min(old[1],size)
                if keep:self.uc.mem_write(new,bytes(self.uc.mem_read(pointer,keep)))
                del self.allocations[pointer]
            item.update(return_value=hex(new),old_size=old[1],new_size=size,content_prefix_preserved=bool(new),last_error=self.last_error,contract='SDK-defined relocation, preserved prefix, zero growth if requested; failure keeps old ownership/error')
            self.ret(new,4)
        elif dll.lower()=='kernel32.dll' and name=='HeapSize':
            a=self.args(3);allocation=self.allocations.get(a[2])
            if a[1]&~1:raise Boundary('unsupported_HeapSize_flags',args=[hex(v) for v in a])
            if a[0] not in self.heaps or not allocation or allocation[0]!=a[0]:raise Boundary('undefined_HeapSize_allocation',args=[hex(v) for v in a])
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),return_value=allocation[1],contract='requested size of tracked live private-heap allocation, last error unchanged'))
            self.ret(allocation[1],3)
        elif dll.lower()=='kernel32.dll' and name=='SetUnhandledExceptionFilter':
            a=self.args(1);previous=self.exception_filter
            if a[0]:self.require_region(a[0],1,u.UC_PROT_EXEC,'SetUnhandledExceptionFilter callback')
            self.exception_filter=a[0]
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(a[0])],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),previous_callback=hex(previous),registered_callback=hex(self.exception_filter),contract='single-process guest 32-bit callback registration; exception dispatch remains unsupported'))
            self.ret(previous,1)
        elif dll.lower()=='kernel32.dll' and name=='GetStringTypeW':
            a=self.args(4)
            signed=a[2] if a[2]<0x80000000 else a[2]-0x100000000
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),signed_length=signed)
            self.api_trace.append(item)
            if signed < -1:raise Boundary('unmeasured_negative_NLS_length',length=signed)
            if signed>65536:raise Boundary('NLS_source_budget',length=signed)
            if a[0] in (0,3):source=b''
            elif signed==-1:
                source=bytearray()
                for i in range(4096):
                    self.require_region(a[1]+2*i,2,u.UC_PROT_READ,'GetStringTypeW source')
                    word=bytes(self.uc.mem_read(a[1]+2*i,2));source.extend(word)
                    if word==b'\0\0':break
                else:raise Boundary('unterminated_NLS_source')
                source=bytes(source)
            else:
                self.require_region(a[1],2*signed,u.UC_PROT_READ,'GetStringTypeW source')
                source=bytes(self.uc.mem_read(a[1],2*signed)) if signed else b''
            item['source_UTF16LE_hex']=source.hex() if len(source)<=1024 else None
            if a[1]==a[3]:raise Boundary('unmeasured_NLS_aliased_buffers')
            try:outcome=self.nls.string_type(a[0],source)
            except UnsupportedNls as error:raise Boundary('unsupported_NLS_character_type',detail=str(error))
            self.require_region(a[3],len(outcome['payload']),u.UC_PROT_WRITE,'GetStringTypeW output')
            if outcome['payload']:self.uc.mem_write(a[3],outcome['payload'])
            if outcome['last_error'] is not None:self.last_error=outcome['last_error']
            item.update(return_value=outcome['return_value'],last_error=self.last_error,write_size=len(outcome['payload']))
            self.ret(outcome['return_value'],4)
        elif dll.lower()=='kernel32.dll' and name=='MultiByteToWideChar':
            a=self.args(6)
            signed=a[3] if a[3]<0x80000000 else a[3]-0x100000000
            capacity=a[5] if a[5]<0x80000000 else a[5]-0x100000000
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),signed_length=signed,capacity=capacity)
            self.api_trace.append(item)
            if signed < -1:raise Boundary('unmeasured_negative_NLS_length',length=signed)
            if signed>65536:raise Boundary('NLS_source_budget',length=signed)
            if signed==-1:
                source=bytearray()
                for i in range(4096):
                    self.require_region(a[2]+i,1,u.UC_PROT_READ,'MultiByteToWideChar source')
                    byte=bytes(self.uc.mem_read(a[2]+i,1));source.extend(byte)
                    if byte==b'\0':break
                else:raise Boundary('unterminated_NLS_source')
                source=bytes(source)
            else:
                self.require_region(a[2],signed,u.UC_PROT_READ,'MultiByteToWideChar source')
                source=bytes(self.uc.mem_read(a[2],signed)) if signed else b''
            item['source_hex']=source.hex() if len(source)<=2048 else None
            if a[2]==a[4] and a[4]:raise Boundary('unmeasured_NLS_aliased_buffers')
            try:outcome=self.nls.mb_to_wide(a[0],a[1],source,capacity)
            except UnsupportedNls as error:raise Boundary('unsupported_NLS_conversion',detail=str(error),**item)
            self.require_region(a[4],len(outcome['payload']),u.UC_PROT_WRITE,'MultiByteToWideChar output')
            if outcome['payload']:self.uc.mem_write(a[4],outcome['payload'])
            if outcome['last_error'] is not None:self.last_error=outcome['last_error']
            item.update(return_value=outcome['return_value'],last_error=self.last_error,write_size=len(outcome['payload']))
            self.ret(outcome['return_value'],6)
        elif dll.lower()=='kernel32.dll' and name=='LCMapStringW':
            a=self.args(6);signed=a[3] if a[3]<0x80000000 else a[3]-0x100000000
            capacity=a[5] if a[5]<0x80000000 else a[5]-0x100000000
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),signed_length=signed,capacity=capacity);self.api_trace.append(item)
            if signed < -1:raise Boundary('unmeasured_negative_NLS_length',length=signed)
            if signed>65536:raise Boundary('NLS_source_budget',length=signed)
            if signed==-1:
                source=bytearray()
                for i in range(4096):
                    self.require_region(a[2]+2*i,2,u.UC_PROT_READ,'LCMapStringW source')
                    word=bytes(self.uc.mem_read(a[2]+2*i,2));source.extend(word)
                    if word==b'\0\0':break
                else:raise Boundary('unterminated_NLS_source')
                source=bytes(source)
            else:
                self.require_region(a[2],2*signed,u.UC_PROT_READ,'LCMapStringW source')
                source=bytes(self.uc.mem_read(a[2],2*signed)) if signed else b''
            item['source_hex']=source.hex() if len(source)<=2048 else None
            if a[2]==a[4] and a[4]:raise Boundary('unmeasured_NLS_aliased_buffers')
            try:outcome=self.nls.lc_map(a[0],a[1],source,capacity)
            except UnsupportedNls as error:raise Boundary('unsupported_NLS_mapping',detail=str(error),**item)
            self.require_region(a[4],len(outcome['payload']),u.UC_PROT_WRITE,'LCMapStringW output')
            if outcome['payload']:self.uc.mem_write(a[4],outcome['payload'])
            if outcome['last_error'] is not None:self.last_error=outcome['last_error']
            item.update(return_value=outcome['return_value'],last_error=self.last_error,write_size=len(outcome['payload']));self.ret(outcome['return_value'],6)
        elif dll.lower()=='kernel32.dll' and name=='WideCharToMultiByte':
            a=self.args(8);signed=a[3] if a[3]<0x80000000 else a[3]-0x100000000
            capacity=a[5] if a[5]<0x80000000 else a[5]-0x100000000
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),signed_length=signed,capacity=capacity);self.api_trace.append(item)
            if a[6]:raise Boundary('unmeasured_NLS_custom_default_character')
            if signed < -1:raise Boundary('unmeasured_negative_NLS_length',length=signed)
            if signed>65536:raise Boundary('NLS_source_budget',length=signed)
            if signed==-1:
                source=bytearray()
                for i in range(4096):
                    self.require_region(a[2]+2*i,2,u.UC_PROT_READ,'WideCharToMultiByte source')
                    word=bytes(self.uc.mem_read(a[2]+2*i,2));source.extend(word)
                    if word==b'\0\0':break
                else:raise Boundary('unterminated_NLS_source')
                source=bytes(source)
            else:
                self.require_region(a[2],2*signed,u.UC_PROT_READ,'WideCharToMultiByte source')
                source=bytes(self.uc.mem_read(a[2],2*signed)) if signed else b''
            item['source_hex']=source.hex() if len(source)<=2048 else None
            if a[2]==a[4] and a[4]:raise Boundary('unmeasured_NLS_aliased_buffers')
            try:outcome=self.nls.wide_to_mb(a[0],a[1],source,capacity)
            except UnsupportedNls as error:raise Boundary('unsupported_NLS_reverse_conversion',detail=str(error),**item)
            if a[7] and outcome['used_default'] is None:raise Boundary('unmeasured_NLS_used_default_on_failure_or_query')
            self.require_region(a[4],len(outcome['payload']),u.UC_PROT_WRITE,'WideCharToMultiByte output')
            if a[7]:self.require_region(a[7],4,u.UC_PROT_WRITE,'WideCharToMultiByte used-default flag')
            if outcome['payload']:self.uc.mem_write(a[4],outcome['payload'])
            if a[7]:self.word(a[7],outcome['used_default'])
            if outcome['last_error'] is not None:self.last_error=outcome['last_error']
            item.update(return_value=outcome['return_value'],last_error=self.last_error,write_size=len(outcome['payload']),used_default=outcome['used_default']);self.ret(outcome['return_value'],8)
        elif dll.lower()=='kernel32.dll' and name in ('LCMapStringA','GetLocaleInfoA','GetLocaleInfoW','GetModuleFileNameA'):
            a=self.args(6 if name=='LCMapStringA' else 4 if name.startswith('GetLocaleInfo') else 3)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
            if name=='LCMapStringA' and 0<a[3]<=1024:item['source_hex']=bytes(self.uc.mem_read(a[2],a[3])).hex()
            self.api_trace.append(item);raise Boundary('unimplemented_API',**item)
        elif dll.lower()=='kernel32.dll' and name=='GetCPInfo':
            a=self.args(2)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),contract='measured finite Wine NLS profile; two-argument stdcall; no flags parameter')
            self.api_trace.append(item)
            try:outcome=self.nls.get_cp_info(a[0])
            except UnsupportedNls as error:raise Boundary('unsupported_NLS_code_page',code_page=hex(a[0]),detail=str(error))
            if outcome['payload']:
                start=a[1]
                self.require_region(start,len(outcome['payload']),u.UC_PROT_WRITE,'GetCPInfo output')
                self.uc.mem_write(start,outcome['payload'])
            if outcome['last_error'] is not None:self.last_error=outcome['last_error']
            item.update(return_value=outcome['return_value'],last_error=self.last_error,write_size=len(outcome['payload']))
            self.ret(outcome['return_value'],2)
        elif dll.lower()=='kernel32.dll' and name=='GetACP':
            self.api_trace.append(dict(dll=dll,name=name,args=[],contract='explicit existing Wine Nls ACP profile',return_value=self.profile['ACP'],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)))))
            self.ret(self.profile['ACP'],0)
        elif dll.lower()=='kernel32.dll' and name in ('LocalAlloc','LocalFree','LocalSize','LocalLock','LocalUnlock'):
            a=self.args(2 if name=='LocalAlloc' else 1)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
            self.api_trace.append(item)
            if name=='LocalAlloc':
                if a[0] not in (0,0x40):raise Boundary('unimplemented_movable_LocalAlloc',**item)
                pointer=self.allocate(self.global_heap,8 if a[0]&0x40 else 0,a[1])
                if pointer:self.local_memory[pointer]=a[1]
                item.update(return_value=hex(pointer),fixed_memory=True,zero_initialized=bool(a[0]&0x40));self.ret(pointer,2);return
            pointer=a[0]
            if name=='LocalFree' and not pointer:value=0
            elif pointer not in self.local_memory:raise Boundary('unmeasured_invalid_HLOCAL',**item)
            elif name=='LocalFree':del self.local_memory[pointer];del self.allocations[pointer];value=0
            elif name=='LocalSize':value=self.local_memory[pointer]
            elif name=='LocalLock':value=pointer
            elif name=='LocalUnlock':value=1
            item.update(return_value=hex(value));self.ret(value,1)
        elif dll.lower()=='kernel32.dll' and name in ('GlobalAlloc','GlobalFree','GlobalSize','GlobalLock','GlobalUnlock','GlobalFlags','GlobalHandle'):
            a=self.args(2 if name=='GlobalAlloc' else 1)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
            self.api_trace.append(item)
            if name=='GlobalAlloc':
                flags,size=a
                if flags not in (0x2002,0x2042):raise Boundary('unmeasured_GlobalAlloc_flags',**item)
                pointer=self.allocate(self.global_heap,8 if flags&0x40 else 0,size) if size else 0
                if size and not pointer:self.ret(0,2);return
                handle=self.global_next;self.global_next+=8
                self.global_memory[handle]=dict(pointer=pointer,size=size,locks=0,flags=flags)
                item.update(return_value=hex(handle),memory_pointer=hex(pointer),initial_contents='zero per GMEM_ZEROINIT' if flags&0x40 else 'unspecified memory poisoned cd for research; not asserted equal to Wine contents')
                self.ret(handle,2);return
            if name=='GlobalHandle':
                handle=next((h for h,b in self.global_memory.items() if b['pointer']==a[0] and a[0]),None)
                if handle is None:raise Boundary('unmeasured_GlobalHandle_pointer',**item)
                value=handle
            else:
                handle=a[0]
                if handle not in self.global_memory:raise Boundary('unmeasured_invalid_HGLOBAL',**item)
                block=self.global_memory[handle]
                if name=='GlobalSize':value=block['size']
                elif name=='GlobalFlags':value=0x2000|(0x4000 if not block['size'] else block['locks'])
                elif name=='GlobalLock':
                    if not block['size']:self.last_error=157;value=0
                    else:
                        if block['locks']>=255:raise Boundary('unmeasured_GlobalLock_saturation')
                        block['locks']+=1;value=block['pointer']
                elif name=='GlobalUnlock':
                    if not block['locks']:self.last_error=158;value=0
                    else:
                        block['locks']-=1;value=1 if block['locks'] else 0
                        if not value:self.last_error=0
                elif name=='GlobalFree':
                    if block['locks']:raise Boundary('unmeasured_free_locked_HGLOBAL')
                    if block['pointer']:del self.allocations[block['pointer']]
                    del self.global_memory[handle];value=0
            item.update(return_value=hex(value),last_error=self.last_error);self.ret(value,1)
        else:
            super().api(dll,name)


def main():
    p=argparse.ArgumentParser();p.add_argument('--tag',default='acp');p.add_argument('--save',choices=['ST9BASE','ST9AFTR'],default='ST9BASE');p.add_argument('--mode',choices=['city','archive'],default='city');a=p.parse_args()
    assert a.tag.replace('-','').isalnum()
    started=time.monotonic();r=LoaderRuntime(a.save);steps=[]
    r.uc.mem_write(SCRATCH,GUEST_NAME+b'\0')
    calls=[('original CRT heap',0x76db30,(1,),0),('original CRT thread/TLS/locks',0x76c7fc,(),0),('original CRT IO',0x76efc1,(),0),('original CRT initializer table',0x769f80,(),0)]
    if a.mode=='city':calls.append(('original city load root',0x534a30,(SCRATCH,1),0))
    else:calls.extend([('original archive constructor',0x447b70,(),SCRATCH+0x1000),('original archive open',0x447c90,(SCRATCH,0,0),SCRATCH+0x1000)])
    for label,entry,args,this in calls:
        if time.monotonic()-started>40:raise Boundary('stage10 whole-process budget')
        item=r.call(entry,args,this);item['label']=label;steps.append(item)
        if item['status']!='returned':break
    result=dict(save=a.save,mode=a.mode,tag=a.tag,steps=steps,elapsed_seconds=round(time.monotonic()-started,3),
                original_EXE_sha256=hashlib.sha256(r.pe.data).hexdigest(),save_sha256=hashlib.sha256(r.save).hexdigest(),
                platform_profile=r.profile,original_instruction_bytes_patched=False,IAT_only_platform_binding=True,
                hook_instruction_events=sum(item['instructions'] for item in steps),
                execution_backend='Unicorn 2.1.4 i386 reference execution on arm64 host; not arm64 translated city execution',
                natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8),
                guest_state_note='Gate/phase read only. Zero initial BSS is not evidence of a loaded playable city.',
                full_city_loaded=False,complete_scheduler_cycle=False,original_save_and_restore=False,arm64_chain_compared=False,
                complete_chain_passed=False,first_actual_refusal=next((s['boundary'] for s in steps if s['status']=='refused'),None))
    result.update(registered_exception_filter=hex(r.exception_filter),exception_filter_dispatch_implemented=False)
    out=ROOT/f'evidence/stage10-checkpoint-{a.mode}-{a.tag}.json';out.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(result,ensure_ascii=False),flush=True)


if __name__=='__main__':main()
