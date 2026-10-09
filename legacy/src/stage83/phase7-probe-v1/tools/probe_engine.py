"""Bounded four-context diagnostic engine. Imported only after an execution gate."""
from probe_common import *
from memory_plan import Budget,Memory,PackedTrace,CodeView,Planner
from thread_kernel import ThreadKernel
from models import Events,SurfaceBridge
from integrate import Clocks,Keyboard,Observer
import unicorn as u
from unicorn import x86_const as x
from binary_host import MUTABLE,prepare

SYNC_FIELDS={
 'guest_threads20','current_tid','tls','last_error','thread_log','thread_switches','thread_waits',
 'thread_prefer','thread_quantum','worker_first','worker_capacity','next_tid','process_done',
 'thread_active','critical_sections','thread_owners','events_blob','event_calls','clock_calls',
 'input_calls','keyboard_snapshot61','watch_state54','dd_calls','dd_native_blob','api_trace',
 'count','writes','trace','visited','api_hook_events','heap_growth_events34'}
MUTABLE_FIELDS=MUTABLE|SYNC_FIELDS

def mutable_host():
 c=source_config();return HostStore(ROOT/c['host_manifest'],mutable=MUTABLE_FIELDS)

def restore_source(config):
 meta=source_config();h=mutable_host()
 from stage73_cpu import restore
 raw=(SOURCE/'context.bin').read_bytes();uc,ctx=restore(u,raw,meta['context'])
 code=[]
 with(SOURCE/'memory.bin').open('rb')as f:
  for r in meta['regions']:
   n=r['high']-r['low']+1;f.seek(r['offset']);b=f.read(n)
   if hashlib.sha256(b).hexdigest()!=r['sha256']:raise Boundary('SOURCE_REGION_HASH')
   uc.mem_map(r['low'],n,r['permissions']);uc.mem_write(r['low'],b)
   if r['permissions']&4:code.append((r['low'],b))
 uc.context_restore(ctx)
 from unicorn.unicorn_py3.unicorn import UcContext
 threads=h['guest_threads20'];cur=h['current_tid']
 for tid,t in threads.items():
  b=raw if tid==cur else t['context'][0];c=UcContext.__new__(UcContext);c.__setstate__((b,5608,4,4))
  uc.context_restore(c)
  if uc.context_save().__getstate__()[0][40:]!=b[40:]:raise Boundary('COMPLETE_CONTEXT_RESTORE',tid=tid)
  if uc.reg_read(x.UC_X86_REG_FS_BASE)!=t['teb']:raise Boundary('CONTEXT_FS_TEB',tid=tid)
  if [uc.reg_read(getattr(x,'UC_X86_REG_'+q))for q in ('CS','DS','ES','SS')]!=[8,16,16,16]:raise Boundary('NONFLAT_SOURCE_SELECTORS_UNPROVEN',tid=tid)
  if uc.reg_read(x.UC_X86_REG_CR0)&0x80000000 or uc.reg_read(x.UC_X86_REG_EFLAGS)&((1<<17)|(1<<8)):raise Boundary('PAGING_VM_OR_SINGLESTEP_UNPROVEN',tid=tid)
  t['context']=c
 uc.context_restore(threads[cur]['context'])
 return uc,h,meta['regions'],CodeView(code)

class Runtime(ThreadKernel):
 def __init__(self,uc,h,m,view,goal,role_start):
  self.uc=uc;self.h=h;self.m=m;self.goal_contract=goal;self.role_start=role_start
  self.threads=h['guest_threads20'];self.current_tid=h['current_tid']
  for name in ('tls','last_error','thread_quantum','worker_first','thread_prefer','thread_switches',
   'thread_waits','thread_log','thread_owners','critical_sections','process_done','worker_capacity','next_tid'):
   setattr(self,name,h[name])
  self.thread_api_yield=False;self.guest_thread_metadata=h['guest_thread_metadata']
  self.clocks=Clocks();self.events=Events(h['events_blob'],self.clocks.deadline_now)
  self.keyboard=Keyboard(h['keyboard_snapshot61']);self.observer=Observer(h['watch_state54'],scheduler=goal['scheduler'])
  self.imports=h['imports'];self.planner=Planner(uc,m,view,self.imports,goal['phase_address'])
  self.bridge=SurfaceBridge(h,m);self.pending_store=[];self.current_plan=None
  self.last_pc=None;self.last_tid=None;self.caller_sp=None;self.goal=False;self.stop_error=None
  self.scheduled=[];self.diagnostic=None;self.api_log=[];self.invalid=None
  self.base_count=h['count'];self.base_writes=h['writes'];self.base_hook_events=h['api_hook_events']
  self.audit_trace=h['trace'];self.visited=h['visited'];self.new_api_count=0
  self.initial_watch=copy.deepcopy(self.observer.w.snapshot());self.deadline_check_count=0
  uc.hook_add(u.UC_HOOK_CODE,self.code);uc.hook_add(u.UC_HOOK_BLOCK,self.block)
  uc.hook_add(u.UC_HOOK_MEM_WRITE,self.guest_write);uc.hook_add(u.UC_HOOK_MEM_INVALID,self.invalid_memory)
 def readword(self,p):return int.from_bytes(self.m.read(p,4),'little')
 def word(self,p,v):self.m.word(p,v,'kernel.word')
 def require_region(self,p,n,permission,why):self.m.check(p,n,permission)
 def args(self,n):return list(struct.unpack('<'+str(n)+'I',self.m.read(self.uc.reg_read(x.UC_X86_REG_ESP)+4,n*4)))if n else[]
 def ret(self,value,n):
  sp=self.uc.reg_read(x.UC_X86_REG_ESP);pc=self.readword(sp)
  self.m.check(pc,1,4);self.uc.reg_write(x.UC_X86_REG_EAX,value&0xffffffff)
  self.uc.reg_write(x.UC_X86_REG_ESP,(sp+4*(n+1))&0xffffffff);self.uc.reg_write(x.UC_X86_REG_EIP,pc)
 def thread_load(self,tid):super().thread_load(tid);self.m.tid=tid
 def thread_wake(self):
  t=self.threads[3]
  if t['status']=='runnable':raise Boundary('THREAD3_NATURALLY_READY_NO_EXECUTION')
  if t['status']=='waiting':
   w=t['wait'];ready=self.thread_owners.get(w['address'])in(None,3)if w['kind']=='critical'else self.events.query(w['handle'])['signaled']or w['deadline']is not None and self.thread_now()>=w['deadline']
   if ready:raise Boundary('THREAD3_NATURALLY_READY_NO_FORCED_WAITING',wait=w)
  super().thread_wake()
 def heap_api(self,name,args):
  h=self.h;m=self.m;heap,flags=args[:2]
  if heap not in h['heaps']:raise Boundary('HEAP_HANDLE')
  if name in ('HeapFree','HeapSize'):
   p=args[2]
   if flags&~1 or p not in h['allocations']or h['allocations'][p][0]!=heap:raise Boundary('HEAP_OWNERSHIP_OR_FLAGS')
   if name=='HeapSize':return h['allocations'][p][1]
   del h['allocations'][p];return 1
  if flags&~9:raise Boundary('HEAP_FLAGS')
  oldptr=args[2]if name=='HeapReAlloc'else None;size=args[-1]
  if size>64*1024*1024:raise Boundary('HEAP_FINITE_SIZE')
  if oldptr is not None:
   if oldptr not in h['allocations']or h['allocations'][oldptr][0]!=heap:raise Boundary('HEAP_REALLOC_OWNERSHIP')
   keep=min(size,h['allocations'][oldptr][1]);body=m.read(oldptr,keep)
  else:body=b''
  actual=max(8,(size+7)&~7);p=h['heap_next'];end=p+actual
  if end>0x20000000:return 0
  new=(end+0xffffff)&~0xffffff if end>h['heap_mapped_end34']else h['heap_mapped_end34']
  rows=[(p,bytes([0 if flags&8 else 0xcd])*actual)]+([(p,body)]if body else[])
  mapped=h['heap_mapped_end34']
  if not 0x10000000<=p<end<=0x20000000:raise Boundary('HEAP_POINTER_DOMAIN')
  if new>mapped and (mapped&0xffffff or new&0xffffff or any(mapped<=r['high']and r['low']<new for r in m.regions)):raise Boundary('HEAP_MAPPING_PLAN')
  for a,b in rows:
   if a<mapped:m.check(a,min(len(b),mapped-a),2)
  # All fills and preserved bytes are admitted together, before mapping or metadata.
  m.budget.reserve(sum((len(b)+3)//4 for _,b in rows))
  if new>h['heap_mapped_end34']:
   old=h['heap_mapped_end34'];m.grow(old,new);h['heap_mapped_end34']=new
   h['heap_growth_events34'].append({'requested_size':size,'pointer':hex(p),'mapped_low':hex(old),'mapped_high_exclusive':hex(new),'actual_mapped_bytes':new-old,'probe_model':True})
  for a,b in rows:m.check(a,len(b),2)
  m.commit_reserved(rows,name);h['heap_next']=end;h['allocations'][p]=(heap,size)
  if oldptr is not None:del h['allocations'][oldptr]
  return p
 def api(self,dll,name):
  h=self.h;sp=self.uc.reg_read(x.UC_X86_REG_ESP);retpc=self.readword(sp);self.m.check(retpc,1,4)
  # Nothing mutates state until its exact reviewed implementation is chosen.
  if name in ('CreateThread','QueueUserAPC','SleepEx','Sleep','SuspendThread','ResumeThread','TerminateThread','SetThreadPriority'):
   raise Boundary('UNSUPPORTED_THREAD_OR_SLEEP_API_BEFORE_IMPLEMENTATION',name=name)
  if name in ('GetTickCount','timeGetTime','QueryPerformanceCounter','QueryPerformanceFrequency')and dll.lower()in ('kernel32.dll','winmm.dll'):
   n=1 if name.startswith('Query')else 0;a=self.args(n);value,error=self.clocks.api(name,a,self.last_error,self.m);self.last_error=error;self.ret(value,n)
   h['clock_calls'].append({'api':name,'args':a,'return_pc':hex(retpc),'result':value,'probe_model':True})
  elif dll.lower()=='user32.dll'and name=='GetAsyncKeyState':
   a=self.args(1);value=self.keyboard.query(a[0],self.current_tid,retpc);self.ret(value,1)
   h['input_calls'].append({'name':name,'args':a,'return_pc':hex(retpc),'return_value':value,'domain':self.keyboard.device.domain,'backend':'existing controlled Device61'})
  elif dll.lower()=='kernel32.dll'and name in ('InterlockedIncrement','InterlockedDecrement'):
   a=self.args(1);value=(self.readword(a[0])+(1 if name.endswith('Increment')else -1))&0xffffffff;self.m.word(a[0],value,name);self.ret(value,1)
  elif dll.lower()=='kernel32.dll'and name in ('HeapAlloc','HeapReAlloc','HeapFree','HeapSize'):
   n=4 if name=='HeapReAlloc'else 3;a=self.args(n);value=self.heap_api(name,a);self.ret(value,n)
  elif dll.lower()=='kernel32.dll'and name in ('SetEvent','ResetEvent'):
   a=self.args(1);value,error=self.events.invoke('set'if name=='SetEvent'else'reset',a,self.last_error);self.last_error=error;self.ret(value,1)
   h['event_calls'].append({'name':name,'args':a,'return_pc':hex(retpc),'result':value,'last_error':error})
  elif dll=='authored.dd'and name in ('2.25','2.32','2.5','2.24'):
   n={'2.25':5,'2.32':2,'2.5':6,'2.24':1}[name];a=self.args(n);value=self.bridge.invoke(name,a);self.ret(value,n)
   h['dd_calls'].append({'name':name,'args':a,'return_pc':hex(retpc),'return_value':value,'probe_model':True,'presentation':False})
  elif dll.lower()=='kernel32.dll':
   if name=='EnterCriticalSection':
    p=self.args(1)[0]
    if self.critical_sections.get(p)==0xffffffff:raise Boundary('CRITICAL_RECURSION_OVERFLOW')
   if not self.thread_api(dll,name):raise Boundary('UNSUPPORTED_API_BEFORE_IMPLEMENTATION',dll=dll,name=name)
  else:raise Boundary('UNSUPPORTED_API_BEFORE_IMPLEMENTATION',dll=dll,name=name)
  row={'dll':dll,'name':name,'tid':self.current_tid,'PC':self.m.pc,'return_pc':retpc,'probe_model':True}
  self.api_log.append(row);h['api_trace'].append(row);self.new_api_count+=1
 def stop(self,e):
  if self.stop_error is None:
   self.stop_error=str(e);self.diagnostic={'kind':getattr(e,'kind',type(e).__name__),'details':getattr(e,'details',{}),'text':str(e),'tid':self.current_tid,'PC':self.uc.reg_read(x.UC_X86_REG_EIP),'SP':self.uc.reg_read(x.UC_X86_REG_ESP)}
  self.uc.emu_stop()
 def check_deadline(self):
  self.m.budget.check()
  if time.monotonic()-self.role_start>=50:raise Boundary('ROLE_SOFT50_FREEZE_RESERVE10')
 def block(self,uc,pc,n,data):
  try:
   self.check_deadline()
   if pc not in self.imports and bytes(uc.mem_read(pc,n))!=self.planner.view.read(pc,n):raise Boundary('IMMUTABLE_EXECUTABLE_BLOCK_CHANGED')
  except Exception as e:self.stop(e)
 def code(self,uc,pc,n,data):
  try:
   if self.stop_error:return uc.emu_stop()
   if self.pending_store:raise Boundary('PLANNED_STORE_NOT_OBSERVED_BEFORE_NEXT_INSTRUCTION')
   tid=self.current_tid;self.m.tid=tid;self.m.pc=pc
   if tid not in (1,2,4):raise Boundary('THREAD3_EXECUTION_FORBIDDEN')
   if self.threads[tid]['status']!='runnable':raise Boundary('WAITING_THREAD_CANNOT_EXECUTE')
   g=self.goal_contract
   if pc in (0x534a30,0x401cd0):raise Boundary('NEW_CITY_LOADER_OR_SAVE_SELECTION_OUTSIDE_PROBE_BEFORE_EXECUTION',pc=pc)
   if pc in (g['wrap'],g['tail']):raise Boundary('NO_WRAP_OR_TAIL_BEFORE_EXECUTION')
   if self.observer.interested(pc):
    self.observer.code(tid,pc,uc.reg_read(x.UC_X86_REG_ESP),self.readword(g['phase_address']),self.readword(g['gate_address']),self.m.read(g['screen_address'],1)[0],self.readword(uc.reg_read(x.UC_X86_REG_ESP)))
    s=self.observer.w.state
    if len(s['advances'])>len(self.initial_watch['advances'])+1:raise Boundary('MORE_THAN_ONE_PRODUCTIVE_ADVANCE')
   if pc==g['entry']:
    sp=uc.reg_read(x.UC_X86_REG_ESP)
    if(tid,self.last_tid,self.last_pc)!=(1,1,g['caller'])or self.caller_sp!=sp+4 or self.readword(sp)!=g['return']:raise Boundary('NOT_GENUINE_NATURAL_TID_CALL_STACK')
    self.observer.goal(tid,self.readword(g['phase_address']));self.goal=True;uc.emu_stop();return
   if pc in self.imports:
    self.check_deadline();dll,name=self.imports[pc];self.api(dll,name);return
   d,writes,body=self.planner.prepare(pc,n)
   # CALL/RET read and destination checks occur before stack admission.
   if d['name']in ('call','jmp','jcc'):self.m.check(d['target'],1,4)
   if d['name']=='ret':self.m.check(self.readword(uc.reg_read(x.UC_X86_REG_ESP)),1,4)
   if pc==g['caller']and body!=bytes.fromhex(g['caller_bytes']):raise Boundary('PINNED_NATURAL_CALL_BYTES_CHANGED')
   if self.m.budget.steps>=self.m.budget.step_cap:raise Boundary('INSTRUCTION_CAP_BEFORE_INSTRUCTION')
   self.m.budget.reserve(len(writes));self.m.budget.instruction(tid);self.pending_store=list(writes);self.current_plan={'pc':pc,'writes':writes}
   self.last_pc=pc;self.last_tid=tid;self.visited.add(pc);self.audit_trace.append(pc)
   if pc==g['caller']:self.caller_sp=uc.reg_read(x.UC_X86_REG_ESP)
  except Exception as e:self.stop(e)
 def guest_write(self,uc,access,a,n,v,data):
  try:
   if self.pending_store!=[(a,n)]:raise Boundary('ENGINE_WRITE_DISAGREES_WITH_PRECOMMIT_PLAN',actual=[a,n],plan=self.pending_store)
   self.pending_store.clear();self.m.budget.consume();self.m.trace.append(self.current_tid,self.m.pc,a,n,v,0)
  except Exception as e:self.stop(e)
 def invalid_memory(self,uc,access,a,n,v,data):
  self.invalid={'access':access,'address':a,'size':n,'value':v};self.stop(Boundary('ENGINE_MEMORY_FAULT_DIAGNOSTIC',**self.invalid));return False
 def segment(self,instruction_limit):
  self.check_deadline();self.scheduled.append(self.current_tid)
  try:self.uc.emu_start(self.uc.reg_read(x.UC_X86_REG_EIP),0,timeout=1000000,count=instruction_limit)
  except u.UcError as e:self.stop(Boundary('ENGINE_FAULT_DIAGNOSTIC',error=str(e)))
  return {'status':'refused'if self.stop_error else'budget_yield','end_pc':hex(self.uc.reg_read(x.UC_X86_REG_EIP)),'boundary':self.stop_error}
 def run(self):
  start=time.monotonic();last=1;segments=[]
  try:
   while not self.stop_error and not self.goal:
    self.check_deadline();self.thread_wake()
    runnable=[tid for tid,t in self.threads.items()if t['status']=='runnable']
    if self.process_done:raise Boundary('PROCESS_COMPLETE_BEFORE_TARGET')
    if not runnable:
     finite=[t['wait']['deadline']for t in self.threads.values()if t['status']=='waiting'and t['wait'].get('deadline')is not None]
     if not finite:raise Boundary('DEADLOCK_OR_ALL_EXITED')
     time.sleep(min(.002,max(0,min(finite)-self.thread_now())));continue
    later=[tid for tid in runnable if tid>last];tid=min(later)if later else min(runnable)
    if self.thread_prefer in runnable:tid=self.thread_prefer;self.thread_prefer=None
    self.thread_load(tid);last=tid;self.thread_api_yield=False
    s=self.segment(self.thread_quantum);self.thread_save();segments.append({'tid':tid,**s})
   if not self.goal and not self.stop_error:raise Boundary('NO_NATURAL_ENTRY')
  except Exception as e:self.stop(e)
  return {'status':'NATURAL_ENTRY_CAPTURED'if self.goal and not self.stop_error else'DIAGNOSTIC_ONLY_UNQUALIFIED','boundary':self.diagnostic,'scheduled':self.scheduled[-64:],'segments':segments[-12:],'guest_seconds':time.monotonic()-start}
 def synchronize(self):
  self.thread_save();h=self.h
  for name in ('current_tid','tls','last_error','thread_quantum','worker_first','thread_prefer','thread_switches','thread_waits','thread_log','thread_owners','critical_sections','process_done','worker_capacity','next_tid'):
   h[name]=copy.deepcopy(getattr(self,name))
  h['guest_threads20']={tid:{**copy.deepcopy({k:v for k,v in t.items()if k!='context'}),'context':t['context'].__getstate__()}for tid,t in self.threads.items()}
  h['events_blob']=self.events.export();h['keyboard_snapshot61']=self.keyboard.snapshot();h['watch_state54']=self.observer.w.snapshot()
  h['count']=self.base_count+self.m.budget.steps;h['writes']=self.base_writes+self.m.budget.writes;h['api_hook_events']=self.base_hook_events+self.new_api_count
  h['trace']=self.audit_trace;h['visited']=self.visited
  return h

def freeze(r,out,result,kind):
 """Official CPU + all mapped bytes + complete host fields, diagnostic provenance."""
 from stage73_cpu import public_state
 uc=r.uc;h=r.synchronize();cur=r.current_tid;pub={};contexts={}
 for tid,t in r.threads.items():
  uc.context_restore(t['context']);pub[str(tid)]=public_state(uc,u);contexts[tid]=uc.context_save().__getstate__()[0]
 uc.context_restore(r.threads[cur]['context'])
 rows=[];offset=0;start=time.monotonic()
 with(out/'memory.bin').open('xb')as f:
  for row in sorted(r.m.regions,key=lambda q:q['low']):
   n=row['high']-row['low']+1;digest=hashlib.sha256()
   for i in range(0,n,1<<20):
    b=bytes(uc.mem_read(row['low']+i,min(n-i,1<<20)));f.write(b);digest.update(b)
   rows.append({**row,'offset':offset,'sha256':digest.hexdigest()});offset+=n
 for tid,b in contexts.items():(out/('context-%d.bin'%tid)).write_bytes(b)
 hostpath=h.freeze(out)if isinstance(h,HostStore)else prepare(h,out/'host-base')
 r.m.trace.close()
 stack=r.uc.reg_read(x.UC_X86_REG_ESP);diagnostic={'stop':result.get('boundary'),'current_PC':r.uc.reg_read(x.UC_X86_REG_EIP),'current_SP':stack,'invalid':r.invalid,'admitted_instruction_current_plan':r.current_plan,'pending_store_plan':r.pending_store}
 for name,a,n in [('stack',stack,64),('object_ESI',r.uc.reg_read(x.UC_X86_REG_ESI),64),('object_ECX',r.uc.reg_read(x.UC_X86_REG_ECX),64),('table_EDX',r.uc.reg_read(x.UC_X86_REG_EDX),32)]:
  try:diagnostic[name]={'address':a,'bytes':r.m.read(a,n).hex()}
  except Exception as e:diagnostic[name]={'address':a,'unreadable':str(e)}
 put(out/'diagnostic.json',diagnostic)
 record={'format':'S83_PHASE7_PROBE_FROZEN_V1','kind':kind,'qualified_natural_input':result['status']=='NATURAL_ENTRY_CAPTURED',
  'regions':rows,'memory_bytes':offset,'current_tid':cur,'public_CPU':pub,'host_manifest':str(hostpath.relative_to(ROOT)),
  'host_manifest_sha256':sha(hostpath),'complete_host_fields':len(h),'context':{'size':5608,'arch':4,'mode':4},
  'source':str(SOURCE),'budget':{'admitted_steps':r.m.budget.steps,'writes':r.m.budget.writes,'reserved_unconsumed':r.m.budget.reserved,'by_thread':r.m.budget.by_thread},
  'status':result['status'],'natural_caller_SP':r.caller_sp,'original_source_instructions':0 if kind=='authored'else r.m.budget.steps,
  'GS_BASE_read_is_unknown_not_proven':True,'no_input_reset_or_forced_PC':True,'snapshot_seconds':time.monotonic()-start}
 put(out/'snapshot.json',record);put(out/'result.json',{**result,'snapshot_complete':True,'snapshot':str((out/'snapshot.json').relative_to(ROOT))})
 pins={str(p.relative_to(ROOT)):sha(p)for p in out.rglob('*')if p.is_file()and p.name not in ('frozen.json','stdout.log','stderr.log')}
 for row in json.loads(hostpath.read_text())['fields']:pins[row['path']]=row['sha256']
 put(out/'frozen.json',{'files':pins,'snapshot_complete':True,'diagnostic_is_not_qualified':not record['qualified_natural_input']})
 return record
