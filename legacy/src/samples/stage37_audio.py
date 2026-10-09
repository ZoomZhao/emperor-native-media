"""Native host PCM FIFO, deliberately independent of unverified Miles APIs."""
import ctypes,hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeAudio:
 def __init__(self):
  m=json.loads((R/'evidence/stage37-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;z=ctypes.c_size_t
  spec={'new':(v,[]),'open':(ctypes.c_int,[v,ctypes.POINTER(w)]),'prepare':(ctypes.c_int,[v,w,v,w,ctypes.POINTER(w)]),'queue':(ctypes.c_int,[v,w,w]),'start':(ctypes.c_int,[v,w]),'stop':(ctypes.c_int,[v,w]),'poll':(ctypes.c_int,[v,w,ctypes.POINTER(w)]),'release':(ctypes.c_int,[v,w,w]),'stats':(ctypes.c_int,[v,ctypes.POINTER(w)]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}
  for name,(ret,args)in spec.items():f=getattr(self.lib,'s37_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.state=self.fn['new']();assert self.state;self.driver=0x5c000101
 def open(self):
  out=ctypes.c_uint32();status=self.fn['open'](self.state,ctypes.byref(out));assert status==0,status;assert out.value==self.driver;return out.value
 def prepare(self,body):
  assert 0<len(body)<=16384*4 and len(body)%4==0;out=ctypes.c_uint32();status=self.fn['prepare'](self.state,self.driver,ctypes.create_string_buffer(body),len(body)//4,ctypes.byref(out));return status,out.value
 def queue(self,h):return self.fn['queue'](self.state,self.driver,h)
 def start(self):return self.fn['start'](self.state,self.driver)
 def stop(self):return self.fn['stop'](self.state,self.driver)
 def stats(self):
  out=(ctypes.c_uint32*16)();assert self.fn['stats'](self.state,out)==0;v=list(out);r=dict(zip(['phase','bad_buffers','active_callbacks','queued_buffers','pending_events','device'],v[:6]));r.update(dict(zip(['callbacks','render_frames','source_frames','source_hash','zero_frames'],[v[6+i*2]|v[7+i*2]<<32 for i in range(5)])));return r
 def snapshot(self):
  n=self.fn['size']();out=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,out,n)==0;return out.raw
 def restore(self,b):assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==0 and self.snapshot()==b
 def poll(self):
  out=(ctypes.c_uint32*4)();status=self.fn['poll'](self.state,self.driver,out);assert status in [0,1];return list(out)if status else None
 def dispose(self):self.fn['delete'](self.state);self.state=None
class GuestABI:
 """Own stdcall ABI: handles/pointers are uint32; no original MSS binding."""
 argc={'open':1,'prepare':4,'queue':2,'start':1,'stop':1,'poll':2,'release':2}
 def __init__(self,native,uc,x):self.native=native;self.uc=uc;self.x=x;self.calls=[]
 def region(self,p,n,permission):
  if not p or p+n>0x100000000:raise ValueError('invalid guest32 extent')
  end=p+n;cursor=p
  for lo,hi,perm in sorted(self.uc.mem_regions()):
   if lo<=cursor<=hi and perm&permission==permission:cursor=min(end,hi+1)
   if cursor==end:return
  raise ValueError('unmapped guest32 memory')
 def api(self,name):
  n=self.argc[name];sp=self.uc.reg_read(self.x.UC_X86_REG_ESP);a=list(struct.unpack('<'+'I'*n,bytes(self.uc.mem_read(sp+4,n*4))));r=self.native;value=1
  if name=='open':self.region(a[0],4,2);h=r.open();self.uc.mem_write(a[0],struct.pack('<I',h));value=0
  elif name=='prepare':
   if a[0]==r.driver and 0<a[2]<=16384:self.region(a[1],a[2]*4,1);self.region(a[3],4,2);value,h=r.prepare(bytes(self.uc.mem_read(a[1],a[2]*4)));self.uc.mem_write(a[3],struct.pack('<I',h))if value==0 else None
  elif name in ['queue','release']:value=r.fn[name](r.state,*a)
  elif name in ['start','stop']:value=r.fn[name](r.state,*a)
  elif name=='poll':
   self.region(a[1],16,2);out=(ctypes.c_uint32*4)();value=r.fn['poll'](r.state,a[0],out);self.uc.mem_write(a[1],struct.pack('<4I',*out))if value==1 else None
  target=struct.unpack('<I',bytes(self.uc.mem_read(sp,4)))[0];self.calls.append(dict(name=name,args=a,return_value=value));self.uc.reg_write(self.x.UC_X86_REG_EAX,value&0xffffffff);self.uc.reg_write(self.x.UC_X86_REG_ESP,sp+4+n*4);self.uc.reg_write(self.x.UC_X86_REG_EIP,target)
