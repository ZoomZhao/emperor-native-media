import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeCursorSelection:
 def __init__(self,window):
  m=json.loads((R/'evidence/stage43-cursor-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.window=window;self.state=None;self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;z=ctypes.c_size_t
  for name,(ret,args)in {'new':(v,[v,v]),'select':(ctypes.c_int,[v,w,ctypes.POINTER(w)]),'size':(z,[v]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}.items():f=getattr(self.lib,'s43c_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
 def ensure(self):
  if not self.state:self.state=self.fn['new'](self.window.state,self.window.fn['pointer'](self.window.state,0x58000001));assert self.state
 def select(self,h):
  self.ensure();v=ctypes.c_uint32();status=self.fn['select'](self.state,h,ctypes.byref(v));return status,v.value
 def snapshot(self):
  if not self.state:return None
  n=self.fn['size'](self.state);a=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,a,n);return a.raw
 def restore(self,b):
  if b is None:return
  self.ensure();assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))and self.snapshot()==b
 def dispose(self):
  if self.state:self.fn['delete'](self.state);self.state=None
