import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeSamples:
 def __init__(self,audio):
  m=json.loads((R/'evidence/stage39-native-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  spec={'new':(ptr,[ptr,v,v,ctypes.POINTER(v)]),'delete':(None,[ptr]),'allocate':(ctypes.c_int,[ptr,v,ctypes.POINTER(v)]),'init':(ctypes.c_int,[ptr,v]),'release':(ctypes.c_int,[ptr,v]),'query':(ctypes.c_int,[ptr,v,v,ctypes.POINTER(v)]),'set':(ctypes.c_int,[ptr,v,v,v]),'start_empty':(ctypes.c_int,[ptr,v]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args)in spec.items():f=getattr(self.lib,'s39_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.evidence=json.loads((R/'evidence/stage39-static.json').read_text());defaults=json.loads((R/'evidence/stage25-miles-semantics.json').read_text())['defaults'];assert self.evidence['volume_preference_index']==2 and defaults[1]==64
  self.audio=audio;self.state=self.fn['new'](audio.state,defaults[1],defaults[2],(v*128)(*self.evidence['pan_table']));assert self.state
 def allocate(self,driver):
  out=ctypes.c_uint32();status=self.fn['allocate'](self.state,driver,ctypes.byref(out));return status,out.value
 def query(self,h,field):
  out=ctypes.c_uint32();status=self.fn['query'](self.state,h,field,ctypes.byref(out));return status,out.value
 def description(self,h):
  fields={'status':2,'loop_count':17,'rate':22,'volume':23,'pan':24,'left_gain':25,'right_gain':26,'ASI_active':79,'filter_active':105,'mixer_active':131,'mixer_provider':132,'mono_gain':160}
  out={}
  for key,i in fields.items():status,v=self.query(h,i);assert status==0;out[key]=v
  return out
 def call(self,name,*args):return self.fn[name](self.state,*args)
 def snapshot(self):
  n=self.fn['size']();out=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,out,n)==0;return out.raw
 def restore(self,b):assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==0 and self.snapshot()==b
 def dispose(self):self.fn['delete'](self.state);self.state=None
