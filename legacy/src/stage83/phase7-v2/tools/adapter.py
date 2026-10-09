"""Own fixture adapter to the verbatim finite ThreadKernel, no original runner."""
from core import *
from models import *
import copy,struct,ast
from thread_kernel import ThreadKernel
from integrate import Clocks,Keyboard,Observer
from unicorn import x86_const as x
import unicorn as u
class Runtime(ThreadKernel):
 def __init__(self,uc,threads,current,events,m,imports,goal,allow_threads=(1,2,4)):
  self.clocks=Clocks();self.keyboard=Keyboard(host()["keyboard_snapshot61"]);self.observer=None;self.uc=uc;self.threads=threads;self.current_tid=current;self.events=events;self.m=m;self.imports=imports;self.goal_contract=goal;self.goal=False;self.allow_threads=allow_threads;self.tls=copy.deepcopy(threads[current]['tls']);self.last_error=threads[current]['last_error'];self.thread_quantum=100000;self.worker_first=True;self.thread_prefer=None;self.thread_switches=0;self.thread_waits=0;self.thread_log=[];self.thread_owners={};self.critical_sections={};self.process_done=False;self.thread_api_yield=False;self.scheduled=[];self.api_log=[];self.last_pc=None;self.caller_sp=None;self.stop_error=None;self.callbacks=Callbacks(m);self.bridge=None;self.callback_contract=None;self.resume_callback=None
  self.uc.hook_add(u.UC_HOOK_CODE,self.code);self.uc.hook_add(u.UC_HOOK_MEM_WRITE,self.guest_write)
 def readword(self,p):return int.from_bytes(self.m.read(p,4),'little')
 def word(self,p,v):self.m.word(p,v,'kernel.word')
 def args(self,n):return list(struct.unpack('<'+str(n)+'I',self.m.read(self.uc.reg_read(x.UC_X86_REG_ESP)+4,n*4)))if n else[]
 def require_region(self,p,n,permission,why):self.m.check(p,n,permission)
 def ret(self,value,n):
  sp=self.uc.reg_read(x.UC_X86_REG_ESP);pc=self.readword(sp);self.uc.reg_write(x.UC_X86_REG_EAX,value&0xffffffff);self.uc.reg_write(x.UC_X86_REG_ESP,sp+4*(n+1));self.uc.reg_write(x.UC_X86_REG_EIP,pc)
 def thread_load(self,tid):
  # Restoring a waiting context for kernel wake is legitimate; scheduling it is not.
  super().thread_load(tid);self.m.tid=tid
 def api(self,dll,name):
  if name in ('CreateThread','QueueUserAPC','SleepEx','SuspendThread','ResumeThread','TerminateThread','SetThreadPriority'):raise Boundary('UNIMPLEMENTED_THREAD_SEMANTICS '+name)
  if name in ('GetTickCount','timeGetTime','QueryPerformanceCounter','QueryPerformanceFrequency'):
   value,error=self.clocks.api(name,self.args(1 if name.startswith('Query')else 0),self.last_error,self.m);self.last_error=error;self.ret(value,1 if name.startswith('Query')else 0);return
  if dll.lower()=='user32.dll'and name=='GetAsyncKeyState':
   value=self.keyboard.query(self.args(1)[0],self.current_tid,self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)));self.ret(value,1);return
  if self.thread_api(dll,name):return
  if dll.lower()=='kernel32.dll'and name in ('SetEvent','ResetEvent'):
   a=self.args(1);value,error=self.events.invoke('set'if name=='SetEvent'else'reset',a,self.last_error);self.last_error=error;self.ret(value,1);return
  if dll=='authored.dd':self.ret(self.bridge.invoke(name,self.args({'2.25':5,'2.32':2,'2.5':6}[name])),{'2.25':5,'2.32':2,'2.5':6}[name]);return
  if dll=='authored.callback':
   sp=self.uc.reg_read(x.UC_X86_REG_ESP);resume=self.readword(sp);target,entry_sp=self.callbacks.enter(self.current_tid,self.callback_contract['target'],self.callback_contract['return_token'],sp+4,self.callback_contract['args'],self.callback_contract);self.resume_callback=resume;self.uc.reg_write(x.UC_X86_REG_ESP,entry_sp);self.uc.reg_write(x.UC_X86_REG_EIP,target);return
  raise Boundary('UNIMPLEMENTED_API '+dll+'!'+name)
 def code(self,uc,pc,n,data):
  try:
   tid=self.current_tid;self.m.tid=tid;self.m.pc=pc
   if tid not in self.allow_threads:raise Boundary('UNAUTHORIZED_RUNNABLE_THREAD '+str(tid))
   if not self.threads[tid]['status']=='runnable':raise Boundary('WAITING_THREAD_CANNOT_EXECUTE')
   if self.callback_contract and pc==self.callback_contract['return_token']:
    self.callbacks.returned(tid,pc,uc.reg_read(x.UC_X86_REG_ESP));uc.reg_write(x.UC_X86_REG_EIP,self.resume_callback);return
   g=self.goal_contract
   if self.observer and self.observer.interested(pc):self.observer.code(tid,pc,uc.reg_read(x.UC_X86_REG_ESP),self.readword(g['phase_address']),self.readword(g['gate_address']),self.m.read(g['screen_address'],1)[0],self.readword(uc.reg_read(x.UC_X86_REG_ESP)))
   if pc==g['entry']:
    sp=uc.reg_read(x.UC_X86_REG_ESP)
    if tid!=1 or self.last_pc!=g['caller']or self.caller_sp!=sp+4 or self.readword(sp)!=g['return']or self.readword(g['phase_address'])!=8:raise Boundary('NATURAL_CALL_TID_PC_SP_PHASE')
    if self.observer:self.observer.goal(tid,self.readword(g['phase_address']))
    self.goal=True;uc.emu_stop();return
   if pc in self.imports:
    self.m.budget.check();dll,name=self.imports[pc];self.api_log.append((tid,pc,dll,name));self.api(dll,name);return
   if not g['authored_low']<=pc<g['authored_high']:raise Boundary('NON_AUTHORED_PC')
   # Fixture has only individual stores; whole multi-store original plans remain closed.
   if self.m.budget.writes>=self.m.budget.write_cap:raise Boundary('SHARED_WRITE_CAP_BEFORE_INSTRUCTION')
   self.m.budget.instruction(tid);self.last_pc=pc
   if pc==g['caller']:
    if bytes(uc.mem_read(pc,n))!=bytes.fromhex(g['caller_bytes']):raise Boundary('CALLER_IS_NOT_PINNED_DIRECT_CALL')
    self.caller_sp=uc.reg_read(x.UC_X86_REG_ESP)
  except Exception as e:self.stop_error=str(e);uc.emu_stop()
 def guest_write(self,uc,access,a,n,v,data):
  try:
   self.m.check(a,n,2);self.m.budget.reserve(1,self.current_tid,'guest.single_store');self.m.trace.append((self.current_tid,self.m.pc,a,n,v&((1<<(8*n))-1),'guest'))
  except Exception as e:self.stop_error=str(e);uc.emu_stop()
 def segment(self,instruction_limit):
  tid=self.current_tid
  if tid not in self.allow_threads:raise Boundary('UNAUTHORIZED_RUNNABLE_THREAD '+str(tid))
  self.scheduled.append(tid);self.uc.emu_start(self.uc.reg_read(x.UC_X86_REG_EIP),0,timeout=1000000,count=instruction_limit)
  status='refused'if self.stop_error else'budget_yield'
  return {'status':status,'end_pc':hex(self.uc.reg_read(x.UC_X86_REG_EIP)),'boundary':self.stop_error}
# Same finite selector/wake/run implementation; freeze before any later wake.
tree=ast.parse((V/'tools/thread_kernel.py').read_text());cls=next(n for n in tree.body if isinstance(n,ast.ClassDef));fn=next(n for n in cls.body if isinstance(n,ast.FunctionDef)and n.name=='thread_run');text=ast.get_source_segment((V/'tools/thread_kernel.py').read_text(),fn)
needle="if s['status']=='refused':";text=text.replace(needle,"if self.goal:return dict(status='natural_authored_call',segments=segments[-12:])\n   "+needle)
scope={'time':__import__('time')};exec(compile(text,'reused_thread_run_with_goal_stop','exec'),scope);Runtime.thread_run=scope['thread_run']
