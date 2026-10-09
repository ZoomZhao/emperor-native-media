import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeINI:
 def __init__(self):
  p=ROOT/'runs/stage22-native/libstage22_ini.dylib';assert hashlib.sha256(p.read_bytes()).hexdigest()==json.loads((ROOT/'evidence/stage22-ini-build.json').read_text())['sha256'];self.lib=ctypes.CDLL(str(p));cp=ctypes.c_char_p;sz=ctypes.c_size_t;v=ctypes.c_uint32;vp=ctypes.POINTER(v);self.lib.s22_string.argtypes=[cp,sz,cp,cp,cp,ctypes.c_void_p,sz,vp,vp];self.lib.s22_integer.argtypes=[cp,sz,cp,cp,v,vp,vp]
 def string(self,body,section,key,fallback,size,initial=None):
  assert 0<=size<=512;buf=ctypes.create_string_buffer(initial if initial is not None else bytes([0xa5])*max(512,size));value=ctypes.c_uint32();error=ctypes.c_uint32();assert self.lib.s22_string(body,len(body),section,key,fallback,buf,size,ctypes.byref(value),ctypes.byref(error))==1;return value.value,error.value,buf.raw[:-1]
 def integer(self,body,section,key,fallback):
  value=ctypes.c_uint32();error=ctypes.c_uint32();assert self.lib.s22_integer(body,len(body),section,key,fallback,ctypes.byref(value),ctypes.byref(error))==1;return value.value,error.value
