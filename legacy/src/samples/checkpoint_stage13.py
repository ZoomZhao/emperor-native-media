#!/usr/bin/env python3
"""Actual original continuation through the self-authored native event ABI."""
import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage12 import DiagnosticRuntime,REGS
from stage7_runtime import Boundary,SCRATCH,x,u
from stage12_state import save_state,restore_state
from stage13_events import NativeEvents,REFUSED

class EventRuntime(DiagnosticRuntime):
    extra_snapshot_fields=['events_blob','event_calls','pending_callbacks','guest_thread_metadata','winsock_references','winsock_calls','clock_calls','directory_observations','disk_calls']
    def __init__(self,save):
        super().__init__(save);self.events=NativeEvents();self.event_calls=[];self.winsock_calls=[];self.clock_calls=[];self.disk_calls=[];self.pending_callbacks=[];self.guest_thread_metadata=dict(main_id=1,worker_threads={},scheduler_implemented=False);self._directory_observations=[]
    @property
    def events_blob(self):return self.events.export()
    @events_blob.setter
    def events_blob(self,body):self.events.restore(body)
    @property
    def winsock_references(self):return self.events.wsa_references.value
    @winsock_references.setter
    def winsock_references(self,n):assert 0<=n<=65535;self.events.wsa_references.value=n
    @property
    def directory_observations(self):return self._directory_observations
    @directory_observations.setter
    def directory_observations(self,rows):
        base=(ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]').resolve()
        for row in rows:
            p=Path(row['host_path']).resolve();assert p.is_relative_to(base) and p.is_dir()==row['is_directory'], 'checkpoint filesystem observation changed'
        self._directory_observations=rows
    def handle_type(self,h):
        if h&0xff000000==0x51000000:return 'event' if self.events.query(h)['live'] else 'closed-or-invalid-event'
        if h in self.files:return 'file'
        if h in self.heaps:return 'private-heap'
        if h==0xffffffff:return 'current-process-pseudo'
        return 'invalid-untracked'
    def api(self,dll,name):
        if dll.lower()=='kernel32.dll' and name in ['CreateEventA','SetEvent','ResetEvent','WaitForSingleObject','CloseHandle']:
            argc={'CreateEventA':4,'SetEvent':1,'ResetEvent':1,'WaitForSingleObject':2,'CloseHandle':1}[name];a=self.args(argc)
            if name=='CloseHandle' and a[0] in self.files:return super().api(dll,name)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),backend='self-authored Mach-O arm64 C ABI event core',input_last_error=self.last_error)
            self.event_calls.append(item);self.api_trace.append(item)
            if name=='CreateEventA':
                if a[0] or a[3]:raise Boundary('unmeasured_named_or_security_event',**item)
                value,error=self.events.invoke('create',a[1:3],self.last_error)
                if value==REFUSED:raise Boundary('native_event_capacity_budget',**item)
                item['created_event_state']=self.events.query(value)
            else:
                kind=self.handle_type(a[0]);item['handle_type']=kind;item['event_before']=self.events.query(a[0])
                if name=='WaitForSingleObject':
                    if kind in ['file','private-heap','current-process-pseudo']:raise Boundary('unimplemented_waitable_type',**item)
                    if item['event_before']['live'] and not item['event_before']['signaled'] and a[1]==0xffffffff:
                        raise Boundary('requires_guest_thread_or_event_pump_for_infinite_wait',**item,thread_metadata=self.guest_thread_metadata,pending_callbacks=self.pending_callbacks)
                    if item['event_before']['live'] and not item['event_before']['signaled'] and a[1]>100:
                        raise Boundary('finite_wait_exceeds_100ms_callback_budget',**item)
                if name=='CloseHandle' and kind=='current-process-pseudo':raise Boundary('unmeasured_pseudo_process_close',**item)
                action={'SetEvent':'set','ResetEvent':'reset','WaitForSingleObject':'wait','CloseHandle':'close'}[name]
                value,error=self.events.invoke(action,a,self.last_error)
                if value==REFUSED:raise Boundary('native_event_lifecycle_refused',**item)
                item['event_after']=self.events.query(a[0])
            self.last_error=error;item.update(return_value=hex(value),last_error=error);self.ret(value,argc)
        elif dll.lower()=='kernel32.dll' and name=='GetDiskFreeSpaceA':
            a=self.args(5);path=self.cstring(a[0]) if a[0] else None
            if path not in (None,b'C:\\'):raise Boundary('unreviewed_disk_volume',path=repr(path))
            for p in a[1:]:self.require_region(p,4,u.UC_PROT_WRITE,'disk output DWORD')
            base=(ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]').resolve();fields,error=self.events.disk_space(base,self.last_error)
            for p,value in zip(a[1:],fields):self.word(p,value)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),native_statvfs_fields=fields,last_error=error,physical_task_volume=True)
            self.disk_calls.append(item);self.api_trace.append(item);self.last_error=error;self.ret(1,5)
        elif dll.lower()=='kernel32.dll' and name=='CreateDirectoryA':
            a=self.args(2);path=self.cstring(a[0]);item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_path=path.decode('ascii'),guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))))
            if a[1] or path.lower()!=b'cities\\':raise Boundary('unreviewed_directory_creation_scope',**item)
            base=(ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]').resolve();target=(base/'Cities').resolve();assert target.is_relative_to(base)
            value,error=self.events.directory_create(target,self.last_error);self.last_error=error;row=dict(host_path=str(target),guest_path=path.decode(),is_directory=target.is_dir(),return_value=value,last_error=error);self._directory_observations.append(row)
            item.update(row);self.api_trace.append(item);self.ret(value,2)
        elif dll.lower()=='kernel32.dll' and name in ['GetLocalTime','GetSystemTime','GetSystemTimeAsFileTime','GetTimeZoneInformation','QueryPerformanceFrequency','QueryPerformanceCounter','GetTickCount']:
            kind,size={'GetLocalTime':(1,16),'GetSystemTime':(2,16),'GetSystemTimeAsFileTime':(3,8),'GetTimeZoneInformation':(4,172),'QueryPerformanceFrequency':(5,8),'QueryPerformanceCounter':(6,8),'GetTickCount':(7,0)}[name];argc=1 if size else 0;a=self.args(argc)
            if size:self.require_region(a[0],size,u.UC_PROT_WRITE,'native clock output')
            value,error,body=self.events.clock(kind,size,self.last_error)
            if body:self.uc.mem_write(a[0],body)
            item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),native_output_hex=body.hex(),native_return_value=hex(value),last_error=error,backend='self-authored native live wall/boot clock with measured fixed-Wine units/timezone')
            self.clock_calls.append(item);self.api_trace.append(item);self.last_error=error
            self.ret(value if kind>=4 else self.uc.reg_read(x.UC_X86_REG_EAX),argc)
        elif dll.lower()=='wsock32.dll' and name in ['ordinal:115','ordinal:116']:
            argc=2 if name=='ordinal:115' else 0;a=self.args(argc);item=dict(dll=dll,name=name,resolved_name='WSAStartup' if argc else 'WSACleanup',args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),references_before=self.winsock_references)
            self.winsock_calls.append(item);self.api_trace.append(item)
            if argc:
                if a[0]&0xffff not in [0,0x101,0x202]:raise Boundary('unmeasured_WSA_version_negotiation',**item)
                if a[1]:self.require_region(a[1],400,u.UC_PROT_READ|u.UC_PROT_WRITE,'WSADATA original 32-bit structure')
                body=bytes(self.uc.mem_read(a[1],400)) if a[1] else None
                value,error,output=self.events.wsa_startup(a[0],body,self.last_error)
                if value==REFUSED:raise Boundary('native_Winsock_startup_gateway_refused',**item)
                if value==0:
                    for lo,hi in [(0,16),(261,269),(390,394)]:self.uc.mem_write(a[1]+lo,output[lo:hi])
            else:value,error=self.events.wsa_cleanup(self.last_error)
            self.last_error=error;item.update(return_value=hex(value),last_error=error,references_after=self.winsock_references,network_calls_issued=False);self.ret(value,argc)
        elif dll.lower()=='kernel32.dll' and name=='CreateThread':
            a=self.args(6);item=dict(dll=dll,name=name,args=[hex(v) for v in a],guest_return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),thread_metadata=self.guest_thread_metadata,pending_callbacks=self.pending_callbacks)
            self.api_trace.append(item);raise Boundary('requires_original_guest_thread_execution',**item,callback_entry=hex(a[2]),callback_parameter=hex(a[3]),creation_flags=hex(a[4]),note='No thread handle, ID or callback completion fabricated; thread launch semantics not implemented')
        else:super().api(dll,name)

def main():
    p=argparse.ArgumentParser();p.add_argument('--tag',default='events');p.add_argument('--resume-state',default='eventproof');p.add_argument('--save',choices=['ST9BASE','ST9AFTR'],default='ST9BASE');a=p.parse_args();assert a.tag.replace('-','').isalnum()
    started=time.monotonic();r=EventRuntime(a.save);previous=restore_state(r,a.resume_state);phase=previous['phase'];milestones=previous['milestones'];before=len(r.progress);first_event=len(r.event_calls);s=r.segment()
    while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
    milestones.append(dict(name=phase,result=s))
    if s['status']=='returned' and phase=='CRT initializer table':
        phase='city load root';s=r.segment(0x534a30,(SCRATCH,1));milestones.append(dict(name='city load root entered',result=s))
        while s['status']=='budget_yield' and time.monotonic()-started<40:s=r.segment()
        milestones.append(dict(name='city load root final',result=s))
    # Refusals occur before a fabricated return. Preserve CPU/IAT stack and all
    # native event/guest handle/callback state for a later supported adapter.
    stored=save_state(r,a.tag,phase,milestones)
    result=dict(save=a.save,elapsed_seconds=round(time.monotonic()-started,3),source_checkpoint=a.resume_state,final_status=s['status'],final_boundary=s.get('boundary'),final_registers=s['registers'],milestones=milestones,new_segments=r.progress[before:],new_event_calls=r.event_calls[first_event:],GUID_count=len(r.guid_calls),event_snapshot_sha256=hashlib.sha256(r.events_blob).hexdigest(),event_snapshot_bytes=len(r.events_blob),callbacks=dict(unhandled_exception=hex(r.exception_filter),pending=r.pending_callbacks,thread_metadata=r.guest_thread_metadata),native_event_state_persisted=True,original_instruction_bytes_patched=False,complete_CRT_initializer_returned=any(m['name']=='CRT initializer table' and m['result']['status']=='returned' for m in milestones),original_city_root_entered=any(m['name']=='city load root entered' for m in milestones),full_city_loaded=False,original_save_restore_passed=False,native_city_passed=False,backend='Unicorn original i386 instructions call task-authored native arm64 event ABI; whole original city is not translated',natural_gate=hex(r.readword(0x88ec38)),natural_phase=r.readword(0xc82ef8),state_directory=str(ROOT/'runs'/('stage12-state-'+a.tag)))
    (ROOT/f'evidence/stage13-checkpoint-{a.tag}.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['new_segments','new_event_calls','milestones']},ensure_ascii=False));r.events.dispose()

if __name__=='__main__':main()
