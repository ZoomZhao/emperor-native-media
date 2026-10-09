"""Own x86 four-thread input; no original CPU bytes or executable mapping."""
from core import *
import struct,copy
CODE=0x6f100000;DATA=0x6f200000;STACK=0x6f300000;IMPORT=CODE+0x1000
def make(u):
 from unicorn import x86_const as x
 from models import Events,Memory,SharedBudget
 from adapter import Runtime
 uc=u.Uc(4,4);regions=[(CODE,CODE+0x2000,7),(DATA,DATA+0x4000,3),(STACK,STACK+0x4000,3),(0x60000000,0x60006000,3)]
 for lo,hi,p in regions:uc.mem_map(lo,hi-lo,p)
 def descriptor(base,limit,access,flags):return struct.pack('<HHBBBB',limit&65535,base&65535,(base>>16)&255,access,((limit>>16)&15)|(flags<<4),(base>>24)&255)
 uc.mem_write(0x60000008,descriptor(0,0xfffff,0x9b,12));uc.mem_write(0x60000010,descriptor(0,0xfffff,0x93,12));tebs={1:0x60001000,2:0x60003000,3:0x60004000,4:0x60005000}
 for tid,teb in tebs.items():uc.mem_write(0x60000000+24+8*(tid-1),descriptor(teb,4095,0x93,4));uc.mem_write(teb+0x18,struct.pack('<I',teb))
 uc.reg_write(x.UC_X86_REG_GDTR,(0,0x60000000,55,0));uc.reg_write(x.UC_X86_REG_CS,8)
 for reg in ('DS','SS','ES'):uc.reg_write(getattr(x,'UC_X86_REG_'+reg),16)
 events=Events(clock=lambda:10.0);start,_=events.invoke('create',[0,0],0);done,_=events.invoke('create',[1,0],0);never,_=events.invoke('create',[1,0],0)
 names=[('kernel32.dll','WaitForSingleObjectEx'),('kernel32.dll','SetEvent'),('kernel32.dll','GetCurrentThreadId'),('kernel32.dll','TlsSetValue'),('kernel32.dll','TlsGetValue'),('authored.dd','2.25'),('authored.dd','2.32'),('authored.dd','2.5'),('authored.callback','invoke')];imports={IMPORT+16*i:r for i,r in enumerate(names)};addr={name:IMPORT+16*i for i,(dll,name)in enumerate(names)}
 for a in imports:uc.mem_write(a,b'\x0f\x0b')
 def program(base):
  b=bytearray()
  def emit(q):b.extend(q)
  def push(v):emit(b'\x68'+struct.pack('<I',v))
  def call(target):emit(b'\xe8'+struct.pack('<i',target-(base+len(b)+5)))
  def api(name,args):
   for v in reversed(args):push(v)
   call(addr[name])
  def store(a):emit(b'\xa3'+struct.pack('<I',a))
  return b,emit,api,store,call
 phase=DATA+0x100;src=DATA+0x1000;dst=DATA+0x2000;srcobj=0x30001020;dstobj=0x30001030;desc=DATA+0x200;sr=DATA+0x300;dr=DATA+0x310
 uc.mem_write(phase,struct.pack('<I',7));uc.mem_write(desc,struct.pack('<I',108));uc.mem_write(sr,struct.pack('<4i',0,0,3,2));uc.mem_write(dr,struct.pack('<4i',1,1,4,3));pixels=b''.join(struct.pack('<I',0x11000000+i)for i in range(6));uc.mem_write(src,pixels);uc.mem_write(dst,b'\xa5'*80)
 main=CODE;worker2=CODE+0x400;worker4=CODE+0x500;callback=CODE+0x700;entry=CODE+0x800;token=IMPORT+0x300
 b,e,api,store,call=program(main);api('WaitForSingleObjectEx',[done,200,1]);api('GetCurrentThreadId',[]);store(DATA+4);api('2.25',[srcobj,0,desc,1,0]);e(b'\xc7\x05'+struct.pack('<II',src,0x12345678));api('2.32',[srcobj,0]);api('2.5',[dstobj,dr,srcobj,sr,0,0]);api('invoke',[]);caller=main+len(b);call(entry);uc.mem_write(main,bytes(b));uc.mem_write(entry,b'\x90')
 b,e,api,store,call=program(worker2);api('GetCurrentThreadId',[]);store(DATA+8);api('TlsSetValue',[0,0x2222]);api('TlsGetValue',[0]);store(DATA+0x18);api('SetEvent',[done]);api('WaitForSingleObjectEx',[never,0xffffffff,1]);uc.mem_write(worker2,bytes(b))
 b,e,api,store,call=program(worker4);e(bytes.fromhex('64a118000000'));store(DATA+0x10);api('GetCurrentThreadId',[]);store(DATA+0x14);api('SetEvent',[start]);api('WaitForSingleObjectEx',[done,200,1]);api('WaitForSingleObjectEx',[never,0xffffffff,1]);uc.mem_write(worker4,bytes(b))
 uc.mem_write(callback,b'\xc7\x05'+struct.pack('<II',phase,8)+bytes.fromhex('c20400'))
 threads={}
 for tid,pc in ((1,main),(2,addr['WaitForSingleObjectEx']),(3,addr['WaitForSingleObjectEx']),(4,worker4)):
  sp=STACK+4096*tid-256;uc.reg_write(x.UC_X86_REG_FS,24+8*(tid-1));uc.reg_write(x.UC_X86_REG_ESP,sp);uc.reg_write(x.UC_X86_REG_EIP,pc);uc.reg_write(x.UC_X86_REG_EFLAGS,0x202)
  wait=None
  if tid in(2,3):uc.mem_write(sp,struct.pack('<4I',worker2 if tid==2 else worker4,start if tid==2 else never,200,1));wait={'kind':'event','handle':start if tid==2 else never,'deadline':10.2 if tid==2 else None,'argc':3,'alertable':True}
  threads[tid]={'context':uc.context_save(),'status':'waiting'if wait else'runnable','wait':wait,'tls':{0:tid*0x1111},'last_error':tid,'teb':tebs[tid],'stack':STACK+4096*(tid-1),'handle':None,'exit_code':None}
 uc.context_restore(threads[2]['context']);budget=SharedBudget(1000,500,3);m=Memory(uc,regions,budget);goal={'caller':caller,'caller_bytes':bytes(uc.mem_read(caller,5)).hex(),'entry':entry,'return':caller+5,'phase_address':phase,'authored_low':CODE,'authored_high':CODE+0x1000};r=Runtime(uc,threads,2,events,m,imports,goal)
 header=struct.pack('<12I',0x44443239,1,3,2,32,0,5,4,32,0,0,0);objects=[]
 for id in range(1,65):objects.append(struct.pack('<12I',2,1,0,0,0,0,3 if id==1 else 5,2 if id==1 else 4,12 if id==1 else 20,32,0,0)if id<=2 else bytes(48))
 host={'dd_objects':{srcobj:{'id':1,'kind':2,'guest_pixels':src},dstobj:{'id':2,'kind':2,'guest_pixels':dst}},'dd_native_blob':header+b''.join(objects)+pixels+b'\xa5'*80};from models import SurfaceBridge
 r.bridge=SurfaceBridge(host,m);r.callback_contract={'target':callback,'targets':[callback],'return_token':token,'argc':1,'args':[dst]};uc.mem_write(token,b'\x0f\x0b')
 return r,{'src':src,'dst':dst,'phase':phase,'expected_source':struct.pack('<I',0x12345678)+pixels[4:],'done':done,'start':start,'never':never,'source_object':srcobj,'destination_object':dstobj,'descriptor':desc,'source_rect':sr,'destination_rect':dr}
