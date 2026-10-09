import ctypes,hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeMiles:
 def __init__(self):
  p=R/'runs/stage25-native/libstage25_miles.dylib';m=json.loads((R/'evidence/stage25-native-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256']
  self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  for name,(ret,args)in {'new':(ptr,[ctypes.POINTER(v),sz]),'delete':(None,[ptr]),'preference':(ctypes.c_int,[ptr,v,v,ctypes.POINTER(v)]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}.items():
   f=getattr(self.lib,'s25_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  evidence=json.loads((R/'evidence/stage25-miles-semantics.json').read_text());self.defaults=evidence['defaults'];self.state=self.fn['new']((v*45)(*self.defaults),45);assert self.state
 def preference(self,index,value):
  old=ctypes.c_uint32();assert self.fn['preference'](self.state,index,value,ctypes.byref(old))==1;return old.value
 def export(self):
  b=ctypes.create_string_buffer(self.fn['size']());assert self.fn['export'](self.state,b,len(b));return b.raw
 def restore(self,b):assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==1 and self.export()==b
 def dispose(self):self.fn['delete'](self.state);self.state=None
