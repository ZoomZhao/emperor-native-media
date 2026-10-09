"""Thin marshal layer; explicit fixture is separate from native live profile."""
import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeGUI:
 def __init__(self,fixture=None):
  p=ROOT/'runs/stage15-native/libstage15_gui.dylib';m=json.loads((ROOT/'evidence/stage15-gui-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256']
  self.lib=ctypes.CDLL(str(p));v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t;word=ctypes.POINTER(v)
  sigs={'new':(ptr,[word]),'delete':(None,[ptr]),'call':(ctypes.c_int,[ptr,v,v,v,word,word]),'profile':(ctypes.c_int,[ptr,word,sz]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz]),'object_kind':(v,[ptr,v]),'brush_pixel':(ctypes.c_int,[ptr,v,word])}
  for name,(ret,args) in sigs.items():
   f=getattr(self.lib,'s15_gui_'+name);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
  values=(v*18)(*fixture) if fixture is not None else None;self.state=self.new_c(values);assert self.state,'actual AppKit display unavailable; no fixture fallback';self.fixture=fixture is not None
 def invoke(self,kind,a=0,b=0,last_error=0):
  value=ctypes.c_uint32();error=ctypes.c_uint32(last_error);status=self.call_c(self.state,kind,a,b,ctypes.byref(value),ctypes.byref(error));return status,value.value,error.value
 def profile(self):
  a=(ctypes.c_uint32*18)();assert self.profile_c(self.state,a,18)==1;return list(a)
 def export(self):
  b=ctypes.create_string_buffer(self.size_c());assert self.export_c(self.state,b,len(b))==1,'native GUI profile changed';return b.raw
 def restore(self,body):
  b=ctypes.create_string_buffer(body);assert self.import_c(self.state,b,len(body))==1,'native GUI resource/profile checkpoint refuses';assert self.export()==body
 def brush_pixel(self,h):
  value=ctypes.c_uint32();assert self.brush_pixel_c(self.state,h,ctypes.byref(value))==1;return value.value
 def dispose(self):
  if self.state:self.delete_c(self.state);self.state=None
