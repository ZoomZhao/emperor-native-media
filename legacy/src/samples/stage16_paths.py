import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
GAME=(ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]').resolve()
class NativePaths:
 def __init__(self,guest=b'C:\\Emperor',binary='runs/stage16-native/libstage16_paths.dylib',build_record='evidence/stage16-paths-build.json'):
  p=ROOT/binary;m=json.loads((ROOT/build_record).read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];assert GAME.is_relative_to(ROOT)
  self.lib=ctypes.CDLL(str(p));v=ctypes.c_uint32;ptr=ctypes.c_void_p;word=ctypes.POINTER(v);sz=ctypes.c_size_t
  sigs={'new':(ptr,[ctypes.c_char_p,ctypes.c_char_p]),'delete':(None,[ptr]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args) in sigs.items():
   f=getattr(self.lib,'s16_paths_'+name);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
  self.set_c=self.lib.s16_cwd_set;self.set_c.restype=ctypes.c_int;self.set_c.argtypes=[ptr,ctypes.c_char_p,word,word]
  self.get_c=self.lib.s16_cwd_get;self.get_c.restype=ctypes.c_int;self.get_c.argtypes=[ptr,v,ptr,word,word]
  self.copy_c=self.lib.s16_string_copy;self.copy_c.restype=ctypes.c_int;self.copy_c.argtypes=[ptr,sz,ctypes.c_char_p,sz,v,ctypes.POINTER(sz)]
  self.length_c=self.lib.s16_string_length;self.length_c.restype=ctypes.c_int;self.length_c.argtypes=[ctypes.c_char_p,sz,word]
  self.state=self.new_c(guest,str(GAME).encode());assert self.state;self.guest=guest
 def set(self,path,last_error):
  value=ctypes.c_uint32();error=ctypes.c_uint32(last_error);status=self.set_c(self.state,path,ctypes.byref(value),ctypes.byref(error));return status,value.value,error.value
 def get(self,capacity,body,last_error):
  b=ctypes.create_string_buffer(body) if body is not None else None;value=ctypes.c_uint32();error=ctypes.c_uint32(last_error);status=self.get_c(self.state,capacity,b,ctypes.byref(value),ctypes.byref(error));return status,value.value,error.value,b.raw[:-1] if b is not None else None
 def export(self):
  b=ctypes.create_string_buffer(self.size_c());assert self.export_c(self.state,b,len(b))==1;return b.raw
 def copy(self,source,count,body):
  b=ctypes.create_string_buffer(body);written=ctypes.c_size_t();status=self.copy_c(b,len(body),source,len(source),count,ctypes.byref(written));return status,written.value,b.raw[:-1]
 def length(self,source):
  value=ctypes.c_uint32();status=self.length_c(source,len(source),ctypes.byref(value));return status,value.value
 def restore(self,body):
  b=ctypes.create_string_buffer(body);assert self.import_c(self.state,b,len(body))==1;assert self.export()==body
 def dispose(self):
  if self.state:self.delete_c(self.state);self.state=None
