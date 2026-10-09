import copy,struct,time
from unicorn import x86_const as x
import unicorn as u
from core import Boundary
GDT=0x60000000;TEB=0x60001000;STACK=0x70000000;STACK_SIZE=0x100000;SENTINEL=0x62000000
class ThreadKernel:
 def thread_now(self):
  value,error,body=self.events.clock(6,8,self.last_error);assert value==1 and error==self.last_error;return struct.unpack('<Q',body)[0]/10000000
 def activate_threads(self,quantum=1000,worker_first=False,worker_capacity=2):
  assert quantum in (100,1000,10000)and worker_capacity in (2,3);self.worker_capacity=worker_capacity;self.thread_quantum=quantum;self.worker_first=worker_first;self.current_tid=1;self.next_tid=2;self.thread_log=[];self.threads={1:dict(status='runnable',context=self.uc.context_save(),tls=copy.deepcopy(self.tls),last_error=self.last_error,teb=TEB,stack=STACK,handle=None,exit_code=None,wait=None)};self.thread_owners={p:(1 if depth else None)for p,depth in self.critical_sections.items()};self.thread_api_yield=False;self.thread_waits=0;self.thread_switches=0;self.process_done=False;self.thread_prefer=None
  for index in range(worker_capacity):
   teb=0x60003000+index*4096;stack=0x71000000+index*0x200000
   self.map(teb,4096,u.UC_PROT_READ|u.UC_PROT_WRITE);self.map(stack+STACK_SIZE-4096,4096,u.UC_PROT_READ|u.UC_PROT_WRITE)
   self.uc.mem_write(GDT+32+8*index,self.descriptor(teb,4095,0x93,0x4))
  self.uc.reg_write(x.UC_X86_REG_GDTR,(0,GDT,31+8*worker_capacity,0));self.guest_thread_metadata=dict(main_id=1,worker_threads={},scheduler_implemented=True,policy='finite round-robin instruction quanta, APIs and waits; no priority or APC execution',quantum=quantum,worker_first=worker_first)
 def extend_to_reviewed_three_workers(self):
  assert self.worker_capacity==2 and self.next_tid==4;old=self.current_tid;index=2;teb=0x60005000;stack=0x71400000;self.map(teb,4096,u.UC_PROT_READ|u.UC_PROT_WRITE);self.map(stack+STACK_SIZE-4096,4096,u.UC_PROT_READ|u.UC_PROT_WRITE);self.uc.mem_write(GDT+48,self.descriptor(teb,4095,0x93,0x4))
  for tid in self.threads:
   self.thread_load(tid);self.uc.reg_write(x.UC_X86_REG_GDTR,(0,GDT,55,0));self.thread_save()
  self.thread_load(old);self.worker_capacity=3;self.thread_log.append(dict(action='platform_capacity_extend',previous=2,current=3,reason='original 0x6122c0 fixed three startup objects; third actual CreateThread pending'))
 def thread_save(self):
  t=self.threads[self.current_tid];t['context']=self.uc.context_save();t['tls']=copy.deepcopy(self.tls);t['last_error']=self.last_error
 def thread_load(self,tid):
  self.thread_save();t=self.threads[tid];self.current_tid=tid;self.uc.context_restore(t['context']);self.tls=copy.deepcopy(t['tls']);self.last_error=t['last_error'];self.thread_switches+=1
 def thread_finish(self,code):
  if self.current_tid==1:raise Boundary('main_thread_exit_outside_finite_worker_scope')
  t=self.threads[self.current_tid];t.update(status='exited',exit_code=code,wait=None);v,e=self.events.invoke('set',[t['handle']],self.last_error);assert v==1;self.thread_log.append(dict(action='exit',tid=self.current_tid,exit_code=code));self.uc.emu_stop();self.thread_api_yield=True
 def thread_api(self,dll,name):
  if dll.lower()!='kernel32.dll':return False
  sp=self.uc.reg_read(x.UC_X86_REG_ESP);ret=self.readword(sp);t=self.threads[self.current_tid]
  if name=='CreateThread':
   a=self.args(6)
   if a[0] or a[1] or a[4] or self.next_tid>self.worker_capacity+1:raise Boundary('finite_thread_creation_scope',args=[hex(v)for v in a])
   if a[5]:self.require_region(a[5],4,u.UC_PROT_WRITE,'thread ID output')
   section=self.thread_code_section(a[2])
   if not section:raise Boundary('thread_start_code_or_exception_scope',entry=hex(a[2]))
   tid=self.next_tid;self.next_tid+=1;index=tid-2;teb=0x60003000+index*4096;stack=0x71000000+index*0x200000;self.uc.mem_write(teb,bytes(4096))
   for offset,val in [(0,0xffffffff),(4,stack+STACK_SIZE),(8,stack+STACK_SIZE-4096),(0x18,teb)]:self.word(teb+offset,val)
   context=self.uc.context_save();self.uc.reg_write(x.UC_X86_REG_FS,32+8*index)
   for reg in [x.UC_X86_REG_EAX,x.UC_X86_REG_EBX,x.UC_X86_REG_ECX,x.UC_X86_REG_EDX,x.UC_X86_REG_ESI,x.UC_X86_REG_EDI,x.UC_X86_REG_EBP]:self.uc.reg_write(reg,0)
   worker_sp=stack+STACK_SIZE-0x100-8;self.uc.mem_write(worker_sp,struct.pack('<2I',SENTINEL,a[3]));self.uc.reg_write(x.UC_X86_REG_ESP,worker_sp);self.uc.reg_write(x.UC_X86_REG_EIP,a[2]);self.uc.reg_write(x.UC_X86_REG_EFLAGS,0x202);worker_context=self.uc.context_save();self.uc.context_restore(context)
   handle,error=self.events.invoke('create',[1,0],self.last_error);assert handle not in (0,0xfffffffe)
   self.threads[tid]=dict(status='runnable',context=worker_context,tls={k:0 for k in self.tls},last_error=0,teb=teb,stack=stack,handle=handle,exit_code=None,wait=None,entry=a[2],parameter=a[3],closed=False)
   self.guest_thread_metadata['worker_threads'][tid]=dict(handle=hex(handle),entry=hex(a[2]),teb=hex(teb),stack=hex(stack),context_real=True);self.thread_log.append(dict(action='create',tid=tid,handle=hex(handle),entry=hex(a[2]),parameter=hex(a[3]),return_pc=hex(ret)))
   if a[5]:self.word(a[5],tid)
   self.ret(handle,6);self.thread_prefer=tid if self.worker_first else self.current_tid;self.thread_api_yield=True;self.uc.emu_stop();return True
  if name in ['TlsAlloc','TlsSetValue','TlsGetValue','TlsFree','GetCurrentThreadId','GetLastError','SetLastError']:
   if name=='GetCurrentThreadId':self.ret(self.current_tid,0)
   elif name=='GetLastError':self.ret(self.last_error,0)
   elif name=='SetLastError':self.last_error=self.args(1)[0];self.ret(0,1)
   elif name=='TlsAlloc':
    index=next((i for i in range(64)if i not in self.tls),None)
    if index is None:raise Boundary('finite_TLS_slot_capacity')
    self.tls[index]=0
    for other in self.threads.values():other['tls'][index]=0
    self.ret(index,0)
   else:
    a=self.args(2 if name=='TlsSetValue' else 1)
    if a[0]not in self.tls:raise Boundary('unallocated_thread_TLS_index',index=a[0])
    if name=='TlsGetValue':self.last_error=0;self.ret(self.tls[a[0]],1)
    elif name=='TlsSetValue':self.tls[a[0]]=a[1];self.ret(1,2)
    else:
     del self.tls[a[0]]
     for other in self.threads.values():other['tls'].pop(a[0],None)
     self.ret(1,1)
   return True
  if name=='InitializeCriticalSection':
   p=self.args(1)[0]
   if p in self.critical_sections:raise Boundary('critical_section_reinitialization')
   self.require_region(p,24,u.UC_PROT_READ|u.UC_PROT_WRITE,'critical section');self.critical_sections[p]=0;self.thread_owners[p]=None;self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),1);return True
  if name in ['EnterCriticalSection','LeaveCriticalSection','DeleteCriticalSection']:
   p=self.args(1)[0]
   if p not in self.critical_sections:raise Boundary('uninitialized_critical_section',address=hex(p))
   owner=self.thread_owners[p]
   if name=='EnterCriticalSection':
    if owner not in (None,self.current_tid):
     t.update(status='waiting',wait=dict(kind='critical',address=p));self.thread_log.append(dict(action='critical_block',tid=self.current_tid,address=hex(p),owner=owner));self.thread_api_yield=True;self.uc.emu_stop();return True
    self.thread_owners[p]=self.current_tid;self.critical_sections[p]+=1
   elif name=='LeaveCriticalSection':
    if owner!=self.current_tid or self.critical_sections[p]<1:raise Boundary('critical_section_wrong_thread_leave',address=hex(p))
    self.critical_sections[p]-=1
    if not self.critical_sections[p]:self.thread_owners[p]=None
   else:
    if owner is not None:raise Boundary('delete_owned_critical_section')
    del self.critical_sections[p];del self.thread_owners[p]
   self.ret(self.uc.reg_read(x.UC_X86_REG_EAX),1);return True
  if name in ['WaitForSingleObject','WaitForSingleObjectEx']:
   argc=3 if name.endswith('Ex')else 2;a=self.args(argc)
   if argc==3 and a[2]not in (0,1):raise Boundary('alertable_wait_boolean_scope')
   q=self.events.query(a[0]);thread=next((v for v in self.threads.values()if v.get('handle')==a[0]and not v.get('closed')),None)
   if not q['live']:raise Boundary('thread_wait_type_or_invalid_handle',handle=hex(a[0]))
   if q['signaled']or a[1]==0:
    v,e=self.events.invoke('wait',[a[0],0],self.last_error);self.last_error=e;self.ret(v,argc)
   else:
    t.update(status='waiting',wait=dict(kind='event',handle=a[0],deadline=None if a[1]==0xffffffff else self.thread_now()+a[1]/1000,argc=argc,alertable=bool(argc==3 and a[2])))
    self.thread_waits+=1;self.thread_log.append(dict(action='wait',tid=self.current_tid,handle=hex(a[0]),milliseconds=a[1],alertable=bool(argc==3 and a[2]),return_pc=hex(ret)));self.uc.emu_stop()
   self.thread_api_yield=True;return True
  if name=='ExitThread':self.thread_finish(self.args(1)[0]);return True
  if name=='GetExitCodeThread':
   a=self.args(2);target=next((v for v in self.threads.values()if v.get('handle')==a[0]and not v.get('closed')),None)
   if target is None:raise Boundary('untracked_thread_exitcode')
   self.require_region(a[1],4,u.UC_PROT_WRITE,'thread exitcode');self.word(a[1],target['exit_code']if target['status']=='exited'else 259);self.ret(1,2);return True
  if name=='CloseHandle':
   handle=self.args(1)[0];target=next((v for v in self.threads.values()if v.get('handle')==handle),None)
   if target:
    if target.get('closed'):raise Boundary('closed_thread_handle')
    if any(v['wait']and v['wait'].get('handle')==handle for v in self.threads.values()):raise Boundary('close_handle_during_wait_undefined')
    # Finite scope retains live thread handles; closing exited handle preserves
    # completed context metadata but releases native waitable object.
    if target['status']!='exited':raise Boundary('live_thread_handle_close_outside_finite_scope')
    v,e=self.events.invoke('close',[handle],self.last_error);assert v==1;target['closed']=True;self.last_error=e;self.ret(1,1);return True
  if name in ['QueueUserAPC','SuspendThread','ResumeThread','SetThreadPriority','TerminateThread','SleepEx']:raise Boundary('thread_APC_priority_suspend_or_termination_unimplemented',api=name)
  return False
 def thread_wake(self):
  saved=self.current_tid
  for tid,t in self.threads.items():
   if t['status']!='waiting':continue
   w=t['wait'];ready=False;value=None
   if w['kind']=='critical':ready=self.thread_owners.get(w['address'])in(None,tid)
   elif self.events.query(w['handle'])['signaled']:
    self.thread_load(tid);value,error=self.events.invoke('wait',[w['handle'],0],self.last_error);self.last_error=error;ready=value==0
   elif w['deadline']is not None and self.thread_now()>=w['deadline']:ready=True;value=258
   if ready:
    self.thread_load(tid)
    if value is not None:self.ret(value,w['argc'])
    t.update(status='runnable',wait=None);self.thread_log.append(dict(action='wake',tid=tid,value=value));self.thread_save()
  self.thread_load(saved)
 def thread_run(self,seconds=30,max_steps=20000):
  started=time.monotonic();last=1;segments=[]
  for step in range(max_steps):
   if time.monotonic()-started>=seconds:return dict(status='wall_budget',segments=segments[-12:])
   self.thread_wake();runnable=[tid for tid,t in self.threads.items()if t['status']=='runnable']
   if self.process_done:return dict(status='process_complete',segments=segments[-12:])
   if not runnable:
    finite=[t['wait']['deadline']for t in self.threads.values()if t['status']=='waiting'and t['wait'].get('deadline')is not None]
    if not finite:return dict(status='deadlock_or_all_exited',segments=segments[-12:])
    time.sleep(min(.002,max(0,min(finite)-self.thread_now())));continue
   later=[tid for tid in runnable if tid>last];tid=min(later)if later else min(runnable)
   if self.thread_prefer in runnable:tid=self.thread_prefer;self.thread_prefer=None
   if step==0 and not self.worker_first:tid=1
   self.thread_load(tid);last=tid;self.thread_api_yield=False;s=self.segment(instruction_limit=self.thread_quantum);segments.append(dict(tid=tid,status=s['status'],end_pc=s['end_pc'],boundary=s.get('boundary')));self.thread_save()
   if s['status']=='refused':return dict(status='refused',tid=tid,boundary=s.get('boundary'),segments=segments[-12:])
   if s['status']=='returned':
    if tid==1:return dict(status='main_returned',segments=segments[-12:])
    self.thread_finish(self.uc.reg_read(x.UC_X86_REG_EAX));self.thread_save()
  return dict(status='step_budget',segments=segments[-12:])
 def thread_summary(self):return {tid:{k:v for k,v in t.items()if k!='context'}for tid,t in self.threads.items()}
