"""Marshaling only; session identity and persistence implemented in native C."""
import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeMessages:
 def __init__(self):
  p=ROOT/'runs/stage14-native/libstage14_messages.dylib';m=json.loads((ROOT/'evidence/stage14-messages-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256']
  self.lib=ctypes.CDLL(str(p));v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  for name,(ret,args) in {'new':(ptr,[]),'delete':(None,[ptr]),'size':(sz,[]),'export':(v,[ptr,ptr,sz]),'import':(v,[ptr,ptr,sz])}.items():
   f=getattr(self.lib,'s14_messages_'+name);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
  self.register_c=self.lib.s14_message_register;self.register_c.restype=v;self.register_c.argtypes=[ptr,ctypes.c_char_p,ctypes.POINTER(v)]
  self.process_version_c=self.lib.s14_process_version;self.process_version_c.restype=ctypes.c_int;self.process_version_c.argtypes=[ptr,sz,v,v,ctypes.POINTER(v),ctypes.POINTER(v)]
  self.state=self.new_c();assert self.state
 def register(self,text,last_error):
  e=ctypes.c_uint32(last_error);v=self.register_c(self.state,text,ctypes.byref(e));return v,e.value
 def export(self):
  b=ctypes.create_string_buffer(self.size_c());assert self.export_c(self.state,b,len(b))==1;return b.raw
 def process_version(self,image,pid,current,last_error):
  b=ctypes.create_string_buffer(image);value=ctypes.c_uint32();error=ctypes.c_uint32(last_error)
  assert self.process_version_c(b,len(image),pid,current,ctypes.byref(value),ctypes.byref(error))==1,'unmeasured process lookup or invalid original PE header'
  return value.value,error.value
 def restore(self,body):
  b=ctypes.create_string_buffer(body);assert self.import_c(self.state,b,len(body))==1;assert self.export()==body
 def dispose(self):
  if self.state:self.delete_c(self.state);self.state=None
