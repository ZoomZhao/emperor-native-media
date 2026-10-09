import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeCOM:
 def __init__(self):
  p=ROOT/'runs/stage26-native/libstage26_com.dylib';m=json.loads((ROOT/'evidence/stage26-com-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  for name,(ret,args)in {'new':(ptr,[]),'delete':(None,[ptr]),'initialize':(v,[ptr,v,v]),'uninitialize':(ctypes.c_int,[ptr,v]),'query':(ctypes.c_int,[ptr,v,ctypes.POINTER(v)]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}.items():
   f=getattr(self.lib,'s26_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.state=self.fn['new']();assert self.state
 def initialize(self,tid,flags):return self.fn['initialize'](self.state,tid,flags)
 def uninitialize(self,tid):return self.fn['uninitialize'](self.state,tid)
 def query(self,tid):
  a=(ctypes.c_uint32*2)();assert self.fn['query'](self.state,tid,a);return dict(mode=a[0],count=a[1])
 def export(self):
  a=ctypes.create_string_buffer(self.fn['size']());assert self.fn['export'](self.state,a,len(a));return a.raw
 def restore(self,body):assert self.fn['import'](self.state,ctypes.create_string_buffer(body),len(body))==1 and self.export()==body
 def dispose(self):self.fn['delete'](self.state);self.state=None
