"""Real native state transitions and atomic canonical guest writes."""
from core import *
import ctypes,struct,copy
class SharedBudget:
 def __init__(self,steps,writes,seconds,clock=time.monotonic):self.step_cap=steps;self.write_cap=writes;self.seconds=seconds;self.clock=clock;self.started=clock();self.steps=0;self.writes=0;self.by_thread={};self.reservations=[]
 def check(self):
  if self.clock()-self.started>=self.seconds:raise Boundary('SHARED_GUEST_WALL_CAP')
 def instruction(self,tid):
  self.check()
  if self.steps>=self.step_cap:raise Boundary('SHARED_INSTRUCTION_CAP')
  self.steps+=1;self.by_thread[tid]=self.by_thread.get(tid,0)+1
 def reserve(self,n,tid,kind):
  self.check()
  if n<0 or n>self.write_cap-self.writes:raise Boundary('SHARED_WRITE_CAP_BEFORE_STORE')
  self.writes+=n;self.reservations.append((tid,kind,n))
class Memory:
 def __init__(self,uc,regions,budget):self.uc=uc;self.regions=regions;self.budget=budget;self.tid=0;self.pc=0;self.trace=[]
 def check(self,a,n,permission):
  if a<0 or n<0 or a+n>0x100000000:raise Boundary('ADDRESS_WRAP')
  end=a+n;pos=a
  for lo,hi,p in self.regions:
   if lo<=pos<hi:
    if p&permission!=permission:raise Boundary('MEMORY_PERMISSION')
    pos=min(hi,end)
    if pos==end:return
  if n:raise Boundary('UNMAPPED_RANGE')
 def read(self,a,n):self.check(a,n,1);return bytes(self.uc.mem_read(a,n))
 def write_many(self,rows,kind):
  for a,b in rows:self.check(a,len(b),2)
  count=sum((len(b)+3)//4 for a,b in rows);self.budget.reserve(count,self.tid,kind)
  for a,b in rows:
   for i in range(0,len(b),4):
    q=b[i:i+4];self.uc.mem_write(a+i,q);self.trace.append((self.tid,self.pc,a+i,len(q),int.from_bytes(q,'little'),kind))
 def word(self,a,v,kind):self.write_many([(a,struct.pack('<I',v))],kind)
class Events:
 def __init__(self,blob=None,clock=None):
  from integrate import Clocks
  build=json.loads((OLD/'evidence/build.json').read_text());verify(build['library_pins']);self.lib=ctypes.CDLL(str(OLD/'builds/events.dylib'));self.clock_source=clock if clock is not None else Clocks().deadline_now;self.wsa_references=0;p=ctypes.c_void_p;v=ctypes.c_uint32;err=ctypes.POINTER(v)
  sig={'new':(p,[]),'delete':(v,[p]),'create':(v,[p,v,v,err]),'set':(v,[p,v,err]),'reset':(v,[p,v,err]),'close':(v,[p,v,err]),'wait':(v,[p,v,v,err]),'query':(v,[p,v,err,err,err]),'snapshot_size':(ctypes.c_size_t,[]),'export':(v,[p,p,ctypes.c_size_t]),'import':(v,[p,p,ctypes.c_size_t])}
  for name,(ret,args)in sig.items():
   prefix='s13_events_'if name in ('new','delete','snapshot_size','export','import')else 's13_event_';f=getattr(self.lib,prefix+name);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
  self.state=self.new_c()
  if not self.state:raise Boundary('EVENT_ALLOC')
  if blob is not None:
   b=ctypes.create_string_buffer(blob,len(blob))
   if self.import_c(self.state,b,len(blob))!=1 or self.export()!=blob:raise Boundary('EVENT_SNAPSHOT')
 def invoke(self,name,args,error):
  if name=='wait'and args[1]!=0:raise Boundary('ONLY_SCHEDULER_MEDIATED_NONBLOCKING_EVENT_WAIT')
  e=ctypes.c_uint32(error);v=getattr(self,name+'_c')(self.state,*args,ctypes.byref(e));return v,e.value
 def query(self,h):
  a,b,c=(ctypes.c_uint32()for _ in range(3));live=self.query_c(self.state,h,ctypes.byref(a),ctypes.byref(b),ctypes.byref(c));return {'live':bool(live),'manual':a.value,'signaled':b.value,'waiters':c.value}
 def clock(self,kind,size,error):
  if(kind,size)!=(6,8):raise Boundary('CLOCK_PROFILE_UNSUPPORTED')
  return 1,error,struct.pack('<Q',int(self.clock_source()*10000000))
 def export(self):
  n=self.snapshot_size_c();b=ctypes.create_string_buffer(n)
  if self.export_c(self.state,b,n)!=1:raise Boundary('EVENT_EXPORT')
  return b.raw
 def close(self):
  if self.state:
   if self.delete_c(self.state)!=1:raise Boundary('EVENT_ACTIVE_WAITERS')
   self.state=None
class SurfaceBridge:
 def __init__(self,host,memory):self.host=host;self.m=memory
 def lookup(self,p):
  o=self.host['dd_objects'].get(p)
  if not o or o['kind']!=2:raise Boundary('INVALID_COM32_SURFACE')
  blob=self.host['dd_native_blob'];id=o['id']
  if not 1<=id<=64 or len(blob)<3120:raise Boundary('DD_BLOB_ABI')
  q=struct.unpack_from('<12I',blob,48+(id-1)*48)
  if q[0]!=2 or not q[1]or q[11]:raise Boundary('DD_OBJECT_HEADER')
  return o,dict(zip(('kind','refs','caps','hwnd','coop','clip','w','h','pitch','fmt','locked','reserved'),q))
 def descriptor(self,info,guest,caps):
  a=[0]*27;a[0]=108;a[1]=0x180f if guest else 0x100e;a[2]=info['h'];a[3]=info['w'];a[4]=info['pitch'];a[9]=guest;a[18]=32;a[19]=64;a[21]=32 if info['fmt']==32 else 16;a[22:26]=[0xff0000,0xff00,255,0]if info['fmt']==32 else[0x7c00,0x3e0,31,0]if info['fmt']==555 else[0xf800,0x7e0,31,0];a[26]=caps;return struct.pack('<27I',*a)
 def invoke(self,name,args):
  counts={'2.25':5,'2.32':2,'2.5':6,'2.24':1}
  if name not in counts or len(args)!=counts[name]:raise Boundary('UNSUPPORTED_SURFACE_METHOD '+name)
  obj,info=self.lookup(args[0]);blob=self.host['dd_native_blob'];sources=[(obj,info)]
  if name=='2.25':
   if args[1]or args[4]or args[3]&~1 or int.from_bytes(self.m.read(args[2],4),'little')!=108:raise Boundary('LOCK_ARGUMENT_SCOPE')
  elif name=='2.32':
   if args[1]not in(0,obj['guest_pixels']):raise Boundary('UNLOCK_POINTER')
  elif name=='2.5':
   if args[4]or args[5]:raise Boundary('BLT_FLAGS_EFFECTS')
   src,si=self.lookup(args[2]);sources.append((src,si))
   if info['fmt']!=32 or si['fmt']!=32:raise Boundary('BLT16_REQUIRES_ORIGINAL_SR_CONVERSION_NOT_ALLOWED')
   rect=struct.unpack('<4i',self.m.read(args[3],16))if args[3]else(0,0,si['w'],si['h']);dr=struct.unpack('<4i',self.m.read(args[1],16))if args[1]else(0,0,rect[2]-rect[0],rect[3]-rect[1])
   if dr[2]-dr[0]!=rect[2]-rect[0]or dr[3]-dr[1]!=rect[3]-rect[1]:raise Boundary('BLT_STRETCH_UNSUPPORTED')
  pins=json.loads((OLD/'evidence/build-v2.json').read_text())['library_pins'];verify(pins);lib=ctypes.CDLL(str(OLD/'builds/surfaces-v2.dylib'));p=ctypes.c_void_p;v=ctypes.c_uint32
  sig={'s84_import':(p,[p,ctypes.c_size_t]),'s84_delete':(None,[p]),'s84_export':(ctypes.c_int,[p,p,ctypes.c_size_t]),'p7_sync':(ctypes.c_int,[p,v,p,ctypes.c_size_t]),'p7_lock':(ctypes.c_int,[p,v,v]),'s29_unlock':(ctypes.c_int,[p,v]),'p7_blt32':(ctypes.c_int,[p,v,ctypes.c_int,ctypes.c_int,v,p]),'s34_is_lost':(ctypes.c_int,[p,v,p])}
  for name_,(ret,types)in sig.items():f=getattr(lib,name_);f.restype=ret;f.argtypes=types
  b=ctypes.create_string_buffer(blob,len(blob));state=lib.s84_import(b,len(blob))
  if not state:raise Boundary('DD_IMPORT')
  try:
   if name=='2.24':
    result=(ctypes.c_uint32*7)()
    if not lib.s34_is_lost(state,obj['id'],result):raise Boundary('IS_LOST_OBJECT')
    self.last_residency=list(result);self.m.budget.check();return 0 if result[0]else 0x887601c2
   for o,i in sources:
    n=i['pitch']*i['h'];pix=self.m.read(o['guest_pixels'],n);pb=ctypes.create_string_buffer(pix,n)
    if not lib.p7_sync(state,o['id'],pb,n):raise Boundary('DD_SYNC')
   if name=='2.25':ok=lib.p7_lock(state,obj['id'],obj['guest_pixels'])
   elif name=='2.32':ok=lib.s29_unlock(state,obj['id'])
   else:ok=lib.p7_blt32(state,obj['id'],dr[0],dr[1],src['id'],(ctypes.c_int32*4)(*rect))
   if not ok:return 0x80004005
   output=ctypes.create_string_buffer(len(blob))
   if not lib.s84_export(state,output,len(blob)):raise Boundary('DD_EXPORT')
   new=output.raw;rows=[]
   if name in ('2.25','2.5'):
    offset=3120
    for id in range(1,65):
     q=struct.unpack_from('<12I',new,48+(id-1)*48)
     if q[0]==2 and q[1]:
      n=q[8]*q[7]
      if id==obj['id']:rows.append((obj['guest_pixels'],new[offset:offset+n]))
      offset+=n
    if name=='2.25':rows.append((args[2],self.descriptor(info,obj['guest_pixels'],info['caps'])))
   self.m.write_many(rows,name);self.host['dd_native_blob']=new;return 0
  finally:lib.s84_delete(state)
class Callbacks:
 def __init__(self,m):self.m=m;self.pending=None
 def enter(self,tid,target,return_token,sp,args,contract):
  if any(not 0<=i<=0xffffffff for i in(target,return_token,sp,*args)):raise Boundary('CALLBACK_UINT32_DOMAIN')
  if self.pending is not None or target not in contract['targets']or return_token!=contract['return_token']or len(args)!=contract['argc']:raise Boundary('CALLBACK_TARGET_OR_TOKEN_DOMAIN')
  entry_sp=sp-4*(len(args)+1);data=struct.pack('<'+str(len(args)+1)+'I',return_token,*args);self.m.write_many([(entry_sp,data)],'callback.stack');self.pending={'tid':tid,'target':target,'return_token':return_token,'original_sp':sp,'entry_sp':entry_sp,'argc':len(args)};return target,entry_sp
 def returned(self,tid,pc,sp):
  q=self.pending
  if q is None or(tid,pc,sp)!=(q['tid'],q['return_token'],q['original_sp']):raise Boundary('CALLBACK_RETURN_OWNER_PC_SP')
  self.pending=None
