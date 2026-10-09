import ctypes,hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeMiles:
 def __init__(self):
  m=json.loads((R/'evidence/stage38-native-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  for name,(ret,args)in {'new':(ptr,[ctypes.POINTER(v),sz]),'delete':(None,[ptr]),'preference':(ctypes.c_int,[ptr,v,v,ctypes.POINTER(v)]),'get':(ctypes.c_int,[ptr,v,ctypes.POINTER(v)]),'startup':(ctypes.c_int,[ptr,ctypes.POINTER(v)]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}.items():
   f=getattr(self.lib,'s38_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.defaults=json.loads((R/'evidence/stage25-miles-semantics.json').read_text())['defaults'];self.state=self.fn['new']((v*45)(*self.defaults),45);assert self.state;self.legacy_blob=None
 def invoke(self,name,*args):
  out=ctypes.c_uint32();assert self.fn[name](self.state,*args,ctypes.byref(out))==1;self.legacy_blob=None;return out.value
 def preference(self,index,value):return self.invoke('preference',index,value)
 def startup(self):return self.invoke('startup')
 def get(self,index):return self.invoke('get',index)
 def export(self):
  if self.legacy_blob is not None:return self.legacy_blob
  b=ctypes.create_string_buffer(256);assert self.fn['export'](self.state,b,256)==1;return b.raw
 def restore(self,b):
  assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==1;self.legacy_blob=b if struct.unpack_from('<I',b)[0]==0x5332354d else None;assert self.export()==b
 def dispose(self):self.fn['delete'](self.state);self.state=None
