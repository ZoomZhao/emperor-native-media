"""Pre-instruction single-store admission; unknown/fault-prone forms stop first."""
from probe_common import *
sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import decode as decode_i386

class Budget:
 def __init__(self,steps,writes,seconds,clock=time.monotonic):
  self.step_cap=steps;self.write_cap=writes;self.seconds=seconds;self.clock=clock
  self.started=clock();self.steps=0;self.writes=0;self.reserved=0;self.by_thread={}
 def check(self):
  if self.clock()-self.started>=self.seconds:raise Boundary('SHARED_GUEST_WALL_CAP')
 def reserve(self,n):
  self.check()
  if n<0 or n>self.write_cap-self.writes-self.reserved:raise Boundary('WRITE_CAP_BEFORE_INSTRUCTION_OR_API')
  self.reserved+=n
 def instruction(self,tid):
  self.check()
  if self.steps>=self.step_cap:raise Boundary('INSTRUCTION_CAP_BEFORE_INSTRUCTION')
  self.steps+=1;self.by_thread[tid]=self.by_thread.get(tid,0)+1
 def consume(self,n=1):
  if n>self.reserved:raise Boundary('WRITE_WITHOUT_PRECOMMIT_RESERVATION')
  self.reserved-=n;self.writes+=n

class PackedTrace:
 def __init__(self,path):self.file=Path(path).open('xb');self.buffer=bytearray();self.records=0
 def append(self,tid,pc,a,n,v,kind):
  self.buffer.extend(struct.pack('<6I',tid,pc,a,n,v&0xffffffff,kind));self.records+=1
  if len(self.buffer)>=262144:self.flush()
 def flush(self):
  if self.buffer:self.file.write(self.buffer);self.buffer.clear()
 def close(self):self.flush();self.file.close()

class Memory:
 def __init__(self,uc,regions,budget,trace):
  self.uc=uc;self.regions=sorted(copy.deepcopy(regions),key=lambda q:q['low']);self.budget=budget;self.trace=trace;self.tid=0;self.pc=0
 def check(self,a,n,perm):
  if a<0 or n<0 or a+n>0x100000000:raise Boundary('ADDRESS_WRAP')
  end=a+n;pos=a
  if not n:return
  for r in self.regions:
   if r['low']<=pos<=r['high']:
    if r['permissions']&perm!=perm:raise Boundary('MEMORY_PERMISSION',address=a,size=n)
    pos=min(end,r['high']+1)
    if pos==end:return
  raise Boundary('UNMAPPED_RANGE',address=a,size=n)
 def read(self,a,n):self.check(a,n,1);return bytes(self.uc.mem_read(a,n))if n else b''
 def write_many(self,rows,kind):
  for a,b in rows:self.check(a,len(b),2)
  count=sum((len(b)+3)//4 for _,b in rows);self.budget.reserve(count)
  self.commit_reserved(rows,kind)
 def commit_reserved(self,rows,kind):
  # Prepare the full canonical record body before any API memory commit. Native
  # API writes can use one checked mem_write per row; guest instruction hooks
  # remain individual stores. Time rejection leaves the guest bytes untouched.
  body=bytearray();count=0
  for a,b in rows:
   for i in range(0,len(b),4):
    q=b[i:i+4];body.extend(struct.pack('<6I',self.tid,self.pc,a+i,len(q),int.from_bytes(q,'little'),1));count+=1
  self.budget.check()
  for a,b in rows:
   if b:self.uc.mem_write(a,b)
  self.budget.consume(count);self.trace.flush();self.trace.file.write(body);self.trace.records+=count
 def word(self,a,v,kind):self.write_many([(a,struct.pack('<I',v&0xffffffff))],kind)
 def grow(self,a,end):
  if a&0xffffff or end&0xffffff or not 0x10000000<=a<end<=0x20000000 or any(a<=r['high']and r['low']<end for r in self.regions):raise Boundary('HEAP_MAPPING')
  self.uc.mem_map(a,end-a,3);self.regions.append({'low':a,'high':end-1,'permissions':3,'offset':0});self.regions.sort(key=lambda q:q['low'])

class CodeView:
 def __init__(self,regions):self.regions=regions
 def read(self,a,n):
  for lo,b in self.regions:
   if lo<=a and a+n<=lo+len(b):return b[a-lo:a-lo+n]
  raise Boundary('PC_OUTSIDE_PINNED_EXECUTABLE_MAPPING',pc=a)

class Planner:
 def __init__(self,uc,m,view,imports,phase_address):
  self.uc=uc;self.m=m;self.view=view;self.imports=imports;self.phase=phase_address;self.cache={}
  from unicorn import x86_const as x
  self.x=x;self.regs=[getattr(x,'UC_X86_REG_'+n)for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
 def reg(self,i):return self.uc.reg_read(self.regs[i])
 def ea(self,o,segment=None):
  v=o['disp']+(self.reg(o['base'])if o['base']is not None else 0)+(self.reg(o['index'])*o['scale']if o['index']is not None else 0)
  return ((v&0xffffffff)+(self.uc.reg_read(self.x.UC_X86_REG_FS_BASE)if segment=='FS'else 0))&0xffffffff
 def value(self,o,segment=None):
  if o['kind']=='imm':return o['value']&0xffffffff
  if o['kind']=='reg':
   v=self.reg(o['reg']if o.get('bits',32)!=8 or o['reg']<4 else o['reg']-4)
   return (v>>(8 if o.get('bits',32)==8 and o['reg']>=4 else 0))&((1<<o.get('bits',32))-1)
  return int.from_bytes(self.m.read(self.ea(o,segment),o.get('bits',32)//8),'little')
 def decoded(self,pc,size):
  if pc not in self.cache:
   first=self.view.read(pc,1)[0];segment='FS'if first==0x64 else None
   if first in (0x66,0x67,0xf0,0xf2,0xf3,0x26,0x2e,0x36,0x3e,0x65):raise Boundary('UNPROVEN_PREFIX_MULTI_STORE_OR_SEGMENT',pc=pc,byte=first)
   try:d=decode_i386(self.view,pc+1 if segment else pc)
   except Exception as e:raise Boundary('UNPROVEN_INSTRUCTION_BEFORE_EXECUTION',pc=pc,reason=str(e))
   if d['end']-pc!=size:raise Boundary('DECODER_ENGINE_SIZE_MISMATCH',pc=pc)
   self.cache[pc]=(d,segment,self.view.read(pc,size))
  return self.cache[pc]
 def prepare(self,pc,size):
  d,segment,body=self.decoded(pc,size);name=d['name'];args=d['args'];writes=[];sp=self.reg(4)
  for i,o in enumerate(args):
   if o['kind']=='mem'and name!='lea'and not(i==0 and name in ('mov','setcc')):self.m.check(self.ea(o,segment),o.get('bits',32)//8,1)
  if name in ('call_indirect','jmp_indirect'):
   target=self.value(args[0],segment)
   if target not in self.imports:
    slot=self.ea(args[0],segment)if args[0]['kind']=='mem'else None
    raise Boundary('UNPROVEN_INDIRECT_TARGET_BEFORE_CALL',pc=pc,target=target,slot=slot,
     object_ESI=self.reg(6),object_ECX=self.reg(1),table_EDX=self.reg(2),SP=sp)
  if name in ('push','call','call_indirect'):writes=[((sp-4)&0xffffffff,4)]
  elif name in ('pop','ret'):
   self.m.check(sp,4,1)
   if name=='pop'and args[0]['reg']==4:raise Boundary('POP_ESP_UNPROVEN')
  elif args and args[0]['kind']=='mem'and name not in ('cmp','test','lea','jmp_indirect'):
   if name not in ('mov','add','sub','xor','and','or','shl','shr','sar','neg','inc','dec','setcc'):raise Boundary('UNPROVEN_MEMORY_DESTINATION')
   writes=[(self.ea(args[0],segment),args[0].get('bits',32)//8)]
  for a,n in writes:
   if n not in (1,2,4):raise Boundary('UNPROVEN_STORE_WIDTH')
   self.m.check(a,n,2)
   if self.phase is not None and a<self.phase+4 and self.phase<a+n:
    if a!=self.phase or n!=4:raise Boundary('UNPROVEN_PARTIAL_PHASE_WRITE')
    old=int.from_bytes(self.m.read(a,4),'little')
    if name=='mov':new=self.value(args[1],segment)
    elif name=='inc':new=(old+1)&0xffffffff
    elif name=='dec':new=(old-1)&0xffffffff
    elif name=='add':new=(old+self.value(args[1],segment))&0xffffffff
    else:raise Boundary('UNPROVEN_PHASE_WRITE_BEFORE_EXECUTION')
    if old not in (7,8)or new not in (old,8):raise Boundary('ONE_ADVANCE_NO_WRAP_BEFORE_PHASE_WRITE',old=old,new=new)
  if name=='idiv':raise Boundary('DIVIDE_FAULT_SEMANTICS_UNPROVEN_BEFORE_EXECUTION')
  return d,writes,body
