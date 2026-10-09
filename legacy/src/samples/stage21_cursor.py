import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeCursor:
 def __init__(self):
  p=ROOT/'runs/stage21-native/libstage21_cursor.dylib';assert hashlib.sha256(p.read_bytes()).hexdigest()==json.loads((ROOT/'evidence/stage21-cursor-build.json').read_text())['sha256'];self.lib=ctypes.CDLL(str(p));v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  for n,(ret,args)in {'new':(ptr,[]),'delete':(None,[ptr]),'create':(v,[ptr,v,v,v,v,ptr,ptr]),'load':(v,[ptr,v]),'destroy':(ctypes.c_int,[ptr,v]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz]),'pixels':(ctypes.c_int,[ptr,v,ptr,sz,ptr])}.items():
   f=getattr(self.lib,'s21_'+n);f.restype=ret;f.argtypes=args;setattr(self,n+'_c',f)
  self.state=self.new_c();assert self.state
 def create(self,x,y,w,h,a,b):return self.create_c(self.state,x,y,w,h,ctypes.create_string_buffer(a),ctypes.create_string_buffer(b))
 def export(self):
  b=ctypes.create_string_buffer(self.size_c());assert self.export_c(self.state,b,len(b));return b.raw
 def restore(self,b):assert self.import_c(self.state,ctypes.create_string_buffer(b),len(b))and self.export()==b
 def pixels(self,h):
  b=ctypes.create_string_buffer(4096);xy=(ctypes.c_uint32*2)();assert self.pixels_c(self.state,h,b,len(b),xy);return b.raw,list(xy)
 def dispose(self):self.delete_c(self.state);self.state=None
