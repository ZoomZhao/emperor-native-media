import ctypes,hashlib,json,struct
from pathlib import Path
from stage16_paths import ROOT
class NativeSavedFile:
 def __init__(self,path):
  self.path=Path(path);p=ROOT/'runs/stage17-native/libstage17_file.dylib';m=json.loads((ROOT/'evidence/stage17-file-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));ptr=ctypes.c_void_p;sz=ctypes.c_size_t;q=ctypes.c_uint64
  signatures={'new':(ptr,[ctypes.c_char_p]),'delete':(None,[ptr]),'read':(ctypes.c_int,[ptr,q,ptr,sz,ctypes.POINTER(sz)]),'info':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args) in signatures.items():
   f=getattr(self.lib,'s17_file_'+name);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
  self.convert_c=self.lib.s17_time_convert;self.convert_c.restype=ctypes.c_int;self.convert_c.argtypes=[ctypes.c_uint32,q,ptr,sz]
  self.attributes_c=self.lib.s17_file_basic_attributes;self.attributes_c.restype=ctypes.c_int;self.attributes_c.argtypes=[ptr,ctypes.POINTER(ctypes.c_uint32)]
  self.equal_c=self.lib.s17_identifier_equal;self.equal_c.restype=ctypes.c_int;self.equal_c.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint32)]
  self.state=self.new_c(str(self.path).encode());assert self.state
 def info(self):
  b=ctypes.create_string_buffer(48);assert self.info_c(self.state,b,len(b))==1;return struct.unpack('<6Q',b.raw)
 def read(self,offset,size):
  b=ctypes.create_string_buffer(max(size,1));n=ctypes.c_size_t();assert self.read_c(self.state,offset,b,size,ctypes.byref(n))==1;return b.raw[:n.value]
 def convert(self,kind,ticks,size):
  b=ctypes.create_string_buffer(bytes([0xa5])*size);status=self.convert_c(kind,ticks,b,size);return status,b.raw[:-1]
 def attributes(self):
  v=ctypes.c_uint32();assert self.attributes_c(self.state,ctypes.byref(v))==1;return v.value
 def identifier_equal(self,a,b):
  v=ctypes.c_uint32();status=self.equal_c(a,b,ctypes.byref(v));return status,bool(v.value)
 def dispose(self):
  if self.state:self.delete_c(self.state);self.state=None
