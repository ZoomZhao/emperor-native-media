"""Only own bytes/contexts. Exact production engine and freeze are used."""
from probe_common import *
def subject(case,out,limits):
 from probe_engine import Runtime
 from memory_plan import Budget,Memory,PackedTrace,CodeView
 from integrate import Keyboard
 from models import Events
 import unicorn as u
 from unicorn import x86_const as x
 from core import host
 import collections
 code=0x6f100000;data=0x6f200000;stack=0x6f300000;imports=code+0x1000
 uc=u.Uc(4,4);regions=[{'low':a,'high':a+n-1,'permissions':p,'offset':0}for a,n,p in ((code,8192,7),(data,16384,3),(stack,16384,3),(0x60000000,24576,3),(0x10000000,16384,3))]
 for row in regions:uc.mem_map(row['low'],row['high']-row['low']+1,row['permissions'])
 def desc(base,limit,access,flags):return struct.pack('<HHBBBB',limit&65535,base&65535,(base>>16)&255,access,((limit>>16)&15)|(flags<<4),(base>>24)&255)
 uc.mem_write(0x60000008,desc(0,0xfffff,0x9b,12));uc.mem_write(0x60000010,desc(0,0xfffff,0x93,12))
 tebs={1:0x60001000,2:0x60003000,3:0x60004000,4:0x60005000}
 for tid,teb in tebs.items():uc.mem_write(0x60000000+24+8*(tid-1),desc(teb,4095,0x93,4));uc.mem_write(teb+0x18,struct.pack('<I',teb))
 uc.reg_write(x.UC_X86_REG_GDTR,(0,0x60000000,55,0));uc.reg_write(x.UC_X86_REG_CS,8)
 for r in ('DS','ES','SS'):uc.reg_write(getattr(x,'UC_X86_REG_'+r),16)
 names=['WaitForSingleObjectEx','SetEvent','GetCurrentThreadId','TlsSetValue','TlsGetValue','GetTickCount','GetAsyncKeyState','InterlockedIncrement','HeapAlloc','HeapReAlloc','GetKeyState']
 imp={imports+16*i:('user32.dll'if n in ('GetAsyncKeyState','GetKeyState')else'kernel32.dll',n)for i,n in enumerate(names)};addresses={name:a for a,(_,name)in imp.items()}
 for a in imp:uc.mem_write(a,b'\x0f\x0b')
 def program(base):
  b=bytearray()
  def emit(q):b.extend(q)
  def call(a):emit(b'\xe8'+struct.pack('<i',a-(base+len(b)+5)))
  def api(n,args):
   for v in reversed(args):emit(b'\x68'+struct.pack('<I',v))
   call(addresses[n])
  return b,emit,call,api
 phase=data+0x100;uc.mem_write(phase,struct.pack('<I',7));uc.mem_write(data+0x10c,b'\x01')
 events=Events();start,_=events.invoke('create',[0,0],0);never,_=events.invoke('create',[1,0],0)
 main=code;resume=code+0x900;worker4=code+0x500;worker2=code+0x400;entry=code+0x800
 b,emit,call,api=program(main)
 if case in ('natural','full-performance'):
  api('GetTickCount',[]);api('GetAsyncKeyState',[113]);api('TlsSetValue',[0,0x1212]);api('TlsGetValue',[0])
  api('InterlockedIncrement',[data+0x240]);api('HeapAlloc',[0x3001,8,32])
  if case=='full-performance':
   pixels=786432;n=pixels*4;src=0x40000000;dst=0x41000000
   for a,p in ((src,1),(dst,3)):
    uc.mem_map(a,n,p);regions.append({'low':a,'high':a+n-1,'permissions':p,'offset':0})
   emit(b'\xbe'+struct.pack('<I',src)+b'\xbf'+struct.pack('<I',dst)+b'\xb9'+struct.pack('<I',pixels))
   loop=b''.join(bytes.fromhex(q)for q in ('8b06','03c2','33c3','c1e005','c60711','c6470122','c6470233','c6470344','83c604','83c704','03d8','33d3','f7d8','40','83f800','90','90','49'))
   emit(loop+b'\x75'+struct.pack('<b',-len(loop)-2))
  caller=main+len(b);call(entry)
 elif case=='unknown-api':api('GetKeyState',[113]);caller=main+len(b);emit(b'\xf4')
 elif case=='phase-wrap':caller=main;emit(b'\xc7\x05'+struct.pack('<II',phase,0))
 elif case=='unknown-callback':
  obj=data+0x400;vt=data+0x500;target=code+0x700;slot=data+0x300
  uc.mem_write(vt,struct.pack('<I',target));uc.mem_write(obj,struct.pack('<I',vt));uc.mem_write(slot,bytes(4))
  emit(b'\xc7\x05'+struct.pack('<II',slot,obj));emit(b'\xbe'+struct.pack('<I',obj));emit(b'\x8b\x16\x6a\x01\x8b\xce');caller=main+len(b);emit(b'\xff\x12')
  uc.mem_write(target,b'\xc7\x05'+struct.pack('<II',data+0x350,0xBAD)+b'\xc2\x04\x00')
 elif case=='unknown-instruction':caller=main;emit(b'\xf3\xab')
 elif case=='write-cap':caller=main;emit(b'\xc7\x05'+struct.pack('<II',data+0x200,0xBAD))
 elif case=='invalid-address':caller=main;emit(b'\xc7\x05'+struct.pack('<II',0x12345000,0xBAD))
 elif case=='second-advance':caller=main;emit(b'\x83\x05'+struct.pack('<I',phase)+b'\x01')
 elif case=='heap-realloc-cap':
  api('HeapReAlloc',[0x3001,8, data+0x600,16]);caller=main+len(b);emit(b'\xf4')
 else:caller=main;emit(b'\xeb\xfe')
 uc.mem_write(main,bytes(b));uc.mem_write(entry,b'\x90')
 b,emit,call,api=program(resume);api('WaitForSingleObjectEx',[start,0xffffffff,1]);emit(b'\xc7\x05'+struct.pack('<II',phase,8));emit(b'\xc3');uc.mem_write(resume,bytes(b))
 b,emit,call,api=program(worker4);api('GetCurrentThreadId',[]);api('SetEvent',[start]);api('WaitForSingleObjectEx',[never,0xffffffff,1]);uc.mem_write(worker4,bytes(b))
 b,emit,call,api=program(worker2);api('TlsSetValue',[0,0x2222]);api('WaitForSingleObjectEx',[never,0xffffffff,1]);uc.mem_write(worker2,bytes(b))
 threads={};now=events.clock_source()
 for tid,pc in ((1,resume),(2,addresses['WaitForSingleObjectEx']),(3,addresses['WaitForSingleObjectEx']),(4,worker4)):
  sp=stack+4096*tid-256;wait=None;uc.reg_write(x.UC_X86_REG_FS,24+8*(tid-1))
  if tid==1:sp-=4;uc.mem_write(sp,struct.pack('<I',main))
  if tid in (2,3):
   uc.mem_write(sp,struct.pack('<4I',worker2,start if tid==2 else never,20,1));wait={'kind':'event','handle':start if tid==2 else never,'deadline':now-1 if tid==2 else None,'argc':3,'alertable':True}
  if case=='thread3-ready'and tid==3:wait=None;pc=worker2
  uc.reg_write(x.UC_X86_REG_ESP,sp);uc.reg_write(x.UC_X86_REG_EIP,pc);uc.reg_write(x.UC_X86_REG_EFLAGS,0x202)
  threads[tid]={'context':uc.context_save(),'status':'waiting'if wait else'runnable','wait':wait,'tls':{0:tid*0x1111},'last_error':tid,'teb':tebs[tid],'stack':stack+4096*(tid-1),'handle':None,'exit_code':None}
 if case in ('write-cap','invalid-address','second-advance','unknown-instruction','heap-realloc-cap','phase-wrap'):
  uc.context_restore(threads[1]['context']);uc.reg_write(x.UC_X86_REG_EIP,main);uc.reg_write(x.UC_X86_REG_ESP,stack+4096-256);threads[1]['context']=uc.context_save()
  for tid in (2,4):threads[tid]['status']='waiting';threads[tid]['wait']={'kind':'event','handle':never,'deadline':None,'argc':3,'alertable':True}
 uc.context_restore(threads[2]['context'])
 seed=json.loads(json.dumps(host()['watch_state54']));seed['frames'][-1]['sp']=stack+4096-260;seed['frames'][-1]['ret']=main
 source_keys=json.loads(json.dumps(host()['keyboard_snapshot61']))
 h={'guest_threads20':threads,'current_tid':2,'tls':copy.deepcopy(threads[2]['tls']),'last_error':2,'thread_quantum':100000,'worker_first':True,'thread_prefer':None,'thread_switches':71,'thread_waits':9,'thread_log':[{'own_prior_prefix':'retain'}],
  'thread_owners':{},'critical_sections':{},'process_done':False,'worker_capacity':3,'next_tid':5,'thread_active':True,'guest_thread_metadata':{'main_id':1,'worker_threads':{}},'events_blob':events.export(),
  'event_calls':[],'clock_calls':[],'input_calls':[],'dd_calls':[],'api_trace':[{'own_prior_prefix':'retain'}],'count':11,'writes':17,'api_hook_events':5,'trace':collections.deque([1,2,3],maxlen=10),'visited':{1,2,3},
  'watch_state54':seed,'keyboard_snapshot61':source_keys,'imports':imp,'dd_objects':{},'dd_native_blob':bytes(3120),'heap_growth_events34':[],
  'heaps':{0x3001:{'options':0}},'allocations':{data+0x600:(0x3001,8)},'heap_next':0x10001000,'heap_mapped_end34':0x10004000}
 events.close();view=CodeView([(code,bytes(uc.mem_read(code,8192)))]);uc.mem_protect(code,8192,5);regions[0]['permissions']=5
 clock=time.monotonic
 budget=Budget(limits['steps'],limits['writes'],limits['guest_seconds'],clock)
 if case=='write-cap':budget.write_cap=0
 if case=='heap-realloc-cap':budget.write_cap=6 # CALL push + fill4 + preserved2 cannot fit atomically after argument pushes
 if case=='second-advance':uc.mem_write(phase,struct.pack('<I',8))
 trace=PackedTrace(out/'writes.bin');m=Memory(uc,regions,budget,trace)
 natural_caller=caller if case in ('natural','full-performance')else code+0xb00
 goal={'caller':natural_caller,'caller_bytes':bytes(uc.mem_read(natural_caller,5)).hex(),'entry':entry,'return':natural_caller+5,'phase_address':phase,'gate_address':data+0x104,'screen_address':data+0x10c,'scheduler':0x4ac2b0,'wrap':0x4ac4e2,'tail':0x4ac650}
 r=Runtime(uc,h,m,view,goal,time.monotonic())
 if case=='changed-code':uc.mem_write(worker2,b'\x90')
 if case=='soft-role-timeout':r.role_start-=51
 if case=='soft-guest-timeout':budget.started-=limits['guest_seconds']+1
 if case=='step-cap':budget.step_cap=2
 return r,{'code':code,'data':data,'stack':stack,'unknown_call':caller,'target_marker':data+0x350,'initial_phase':7,'heap_pointer':h['heap_next']}
