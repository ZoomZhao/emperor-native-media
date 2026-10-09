import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativePump:
 def __init__(self,window):
  m=json.loads((R/'evidence/stage42-pump-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;wp=ctypes.POINTER(w);z=ctypes.c_size_t
  spec={'new':(v,[v]),'peek':(ctypes.c_int,[v,w,wp,wp]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}
  for name,(ret,args)in spec.items():f=getattr(self.lib,'s42p_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.window=window;self.state=None
 def ensure(self):
  if not self.state:self.state=self.fn['new'](self.window.fn['pointer'](self.window.state,0x58000001));assert self.state
 def peek(self,remove):
  self.ensure();out=(ctypes.c_uint32*7)();diag=(ctypes.c_uint32*2)();status=self.fn['peek'](self.state,remove,out,diag);return status,list(out),list(diag)
 def snapshot(self):
  if not self.state:return None
  n=self.fn['size']();out=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,out,n)==1;return out.raw
 def restore(self,b):
  if b is None:return
  self.ensure();assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==1 and self.snapshot()==b
 def dispose(self):
  if self.state:self.fn['delete'](self.state);self.state=None
