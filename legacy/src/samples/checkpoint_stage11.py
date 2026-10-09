#!/usr/bin/env python3
"""Same fixed original initialization, bounded; newly verified GUID boundary."""
import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage10 import LoaderRuntime
from stage7_runtime import Boundary,SCRATCH,GUEST_NAME,SENTINEL,TEB,x,u
from stage11_guid import create_guid

class GuidRuntime(LoaderRuntime):
    def __init__(self,save='ST9BASE',provider=None,model_input=False):
        self.guid_provider=provider;self.guid_calls=[];self.debug_messages=[]
        super().__init__(save)
        self.model_input=model_input;self.model_payload=None
        self.model_guest=b'C:\\Emperor\\Model\\EmperorEventmsg.txt'
        if model_input:
            relative='Contents/Resources/drive_c/EmperorRotMK[ZeaS]/Model/EmperorEventmsg.txt'
            source=Path('<LOCAL_MACHINE_PATH>')/relative;copy=ROOT/'stage8-local/EmperorBaseline.app'/relative
            original=source.read_bytes();self.model_payload=copy.read_bytes()
            assert self.model_payload==original and len(original)<=2*1024*1024
            record=dict(original_path=str(source),copy_path=str(copy),size=len(original),sha256=hashlib.sha256(original).hexdigest(),source_copy_equal=True,read_only=True,guest_path=self.model_guest.decode(),virtual_CWD='C:\\Emperor',contents_not_modified=True)
            (ROOT/'evidence/stage11-model-input.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n')

    def allocate(self,heap,flags,size):
        # HeapAlloc does not SetLastError on failure (SDK). Earlier research
        # base set 8 on bounded OOM; this subclass preserves it correctly.
        error=self.last_error;result=super().allocate(heap,flags,size);self.last_error=error;return result
    def api(self,dll,name):
        if dll.lower()=='ole32.dll' and name=='CoCreateGuid':
            a=self.args(1);item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),deterministic_test_provider=self.guid_provider is not None)
            if not a[0]:value=0x80070057;payload=None
            else:
                self.require_region(a[0],16,u.UC_PROT_WRITE,'CoCreateGuid output')
                if self.guid_provider is None:
                    try:payload=create_guid()
                    except OSError as error:raise Boundary('unmapped_host_entropy_failure',detail=str(error))
                    value=0
                else:value,payload=self.guid_provider()
                if payload is not None:
                    assert len(payload)==16;self.uc.mem_write(a[0],payload)
            item.update(return_value=hex(value),last_error=self.last_error,guid_bytes_le_hex=payload.hex() if payload is not None else None,contract='Same-Wine UUIDv4 layout/NULL behavior; fresh OS entropy in actual checkpoint')
            self.guid_calls.append(item);self.api_trace.append(item);self.ret(value,1)
        elif dll.lower()=='kernel32.dll' and name=='OutputDebugStringA':
            a=self.args(1);text=self.cstring(a[0]);self.debug_messages.append(text)
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],text=text.decode('latin1'),contract='debugger absent; void API, bounded string consumed; no game state synthesized'))
            self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),1)
        elif self.model_input and dll.lower()=='kernel32.dll' and name=='GetFullPathNameA':
            a=self.args(4);path=self.cstring(a[0]);canonical={GUEST_NAME.lower():GUEST_NAME,self.model_guest.lower():self.model_guest,b'model\\emperoreventmsg.txt':self.model_guest}.get(path.lower())
            if canonical is None:raise Boundary('path_outside_reviewed_inputs',api=name,args=[hex(v) for v in a],path=path.decode('latin1'))
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],path=path.decode('latin1'),canonical=canonical.decode(),virtual_CWD='C:\\Emperor'))
            if a[1]<=len(canonical):self.ret(len(canonical)+1,4);return
            self.require_region(a[2],len(canonical)+1,u.UC_PROT_WRITE,'full path buffer')
            if a[3]:self.require_region(a[3],4,u.UC_PROT_WRITE,'file part pointer')
            self.uc.mem_write(a[2],canonical+b'\0')
            if a[3]:self.word(a[3],a[2]+canonical.rfind(b'\\')+1)
            self.ret(len(canonical),4)
        elif self.model_input and dll.lower()=='kernel32.dll' and name=='CreateFileA':
            a=self.args(7);path=self.cstring(a[0]);payload={GUEST_NAME.lower():self.save,self.model_guest.lower():self.model_payload,b'model\\emperoreventmsg.txt':self.model_payload}.get(path.lower())
            if payload is None:raise Boundary('path_outside_reviewed_inputs',api=name,args=[hex(v) for v in a],path=path.decode('latin1'))
            if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('unmeasured_model_file_open',args=a)
            if a[3] and (self.readword(a[3])!=12 or self.readword(a[3]+4)!=0):raise Boundary('unmeasured_model_file_security')
            if any(file.get('path')==path.lower() for file in self.files.values()):raise Boundary('same_file_sharing_not_implemented')
            h=self.next_handle;self.next_handle+=1;self.files[h]=dict(path=path.lower(),payload=payload,position=0)
            self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],canonical_path=path.decode(),read_only_payload_sha256=hashlib.sha256(payload).hexdigest(),return_value=hex(h)))
            self.ret(h,7)
        elif self.model_input and dll.lower()=='kernel32.dll' and name=='ReadFile':
            a=self.args(5);h,buffer,n,out,overlap=a
            if overlap or not out or h not in self.files:raise Boundary('unmeasured_model_file_read',args=a)
            file=self.files[h];at=file['position'];chunk=file['payload'][at:at+n]
            self.require_region(buffer,len(chunk),u.UC_PROT_WRITE,'read file buffer');self.require_region(out,4,u.UC_PROT_WRITE,'read byte count')
            if chunk:self.uc.mem_write(buffer,chunk)
            file['position']+=len(chunk);self.word(out,len(chunk));self.api_trace.append(dict(dll=dll,name=name,args=[hex(v) for v in a],bytes_read=len(chunk),offset=at,read_only=True));self.ret(1,5)
        elif self.model_input and dll.lower()=='kernel32.dll' and name=='SetFilePointer':
            a=self.args(4);h,low,high,origin=a
            if h not in self.files or high or origin not in (0,1,2):raise Boundary('unmeasured_model_file_seek',args=a)
            file=self.files[h];signed=low if low<0x80000000 else low-0x100000000
            at=(0 if origin==0 else file['position'] if origin==1 else len(file['payload']))+signed
            if at<0:self.last_error=131;self.ret(0xffffffff,4)
            else:file['position']=at;self.ret(at,4)
        else:super().api(dll,name)

    def resume_bounded(self):
        # Continue the exact stopped machine, including a partially executed
        # REP instruction. No stack, register, gate or initializer is reset.
        pc=self.uc.reg_read(x.UC_X86_REG_EIP);old_writes=self.writes;before=len(self.api_trace)
        self.count=0;self.trace.clear();self.last_invalid=None
        item=dict(entry=hex(pc),resumed_existing_machine=True)
        try:
            self.uc.emu_start(pc,SENTINEL,timeout=1000000,count=500000)
            if self.uc.reg_read(x.UC_X86_REG_EIP)!=SENTINEL:raise Boundary('engine_budget_before_return')
            item.update(status='returned',eax=self.uc.reg_read(x.UC_X86_REG_EAX),esp=hex(self.uc.reg_read(x.UC_X86_REG_ESP)),seh_head=hex(self.readword(TEB)))
        except (Boundary,u.UcError) as error:
            item.update(status='refused',boundary=error.details if isinstance(error,Boundary) else (self.last_invalid or dict(kind='Unicorn_error',text=str(error))))
        item.update(instructions=self.count,write_events=self.writes-old_writes,api_calls=self.api_trace[before:],last_trace=[hex(v) for v in self.trace])
        return item

def main():
    p=argparse.ArgumentParser();p.add_argument('--tag',default='guid');p.add_argument('--save',choices=['ST9BASE','ST9AFTR'],default='ST9BASE');p.add_argument('--resume-budget-chunks',type=int,default=0);p.add_argument('--model-input',action='store_true');a=p.parse_args();assert a.tag.replace('-','').isalnum() and 0<=a.resume_budget_chunks<=12
    started=time.monotonic();r=GuidRuntime(a.save,model_input=a.model_input);r.uc.mem_write(SCRATCH,GUEST_NAME+b'\0');steps=[]
    for label,entry,args in [('CRT heap',0x76db30,(1,)),('CRT TLS/locks',0x76c7fc,()),('CRT IO',0x76efc1,()),('CRT initializer table',0x769f80,()),('city load root',0x534a30,(SCRATCH,1))]:
        if time.monotonic()-started>40:raise Boundary('stage11 whole-process budget')
        s=r.call(entry,args);s['label']=label;steps.append(s)
        for index in range(a.resume_budget_chunks):
            if s['status']!='refused' or s['boundary']['kind']!='engine_budget_before_return':break
            if time.monotonic()-started>35:break
            s=r.resume_bounded();s.update(label=label,resume_chunk=index+1);steps.append(s)
        if s['status']!='returned':break
    result=dict(save=a.save,steps=steps,elapsed_seconds=round(time.monotonic()-started,3),original_EXE_sha256=hashlib.sha256(r.pe.data).hexdigest(),save_sha256=hashlib.sha256(r.save).hexdigest(),platform_version=r.profile,GUID_calls=r.guid_calls,deterministic_GUID_injected=False,model_input_enabled=r.model_input,debug_messages=[t.decode('latin1') for t in r.debug_messages],execution_backend='Unicorn i386 reference; not native full initialization',original_instruction_bytes_patched=False,IAT_only_platform_binding=True,hook_instruction_events=sum(s['instructions'] for s in steps),first_actual_refusal=next((s['boundary'] for s in reversed(steps) if s['status']=='refused' and s['boundary']['kind']!='engine_budget_before_return'),steps[-1].get('boundary')),full_city_loaded=False,complete_scheduler_cycle=False,original_save_and_restore=False,arm64_city_chain_passed=False,natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8))
    (ROOT/f'evidence/stage11-checkpoint-{a.tag}.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(dict(elapsed_seconds=result['elapsed_seconds'],GUID_call_count=len(r.guid_calls),chunks=len(steps),last_status=steps[-1]['status'],first_actual_refusal=steps[-1].get('boundary'),whole_events=result['hook_instruction_events'],city_root_entered=any(s.get('label')=='city load root' for s in steps)),ensure_ascii=False))

if __name__=='__main__':main()
