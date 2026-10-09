import ctypes,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeWindow:
 def __init__(self):
  p=ROOT/'runs/stage26-native/libstage26_window.dylib';m=json.loads((ROOT/'evidence/stage26-window-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t;cp=ctypes.c_char_p;vp=ctypes.POINTER(v)
  sigs={'new':(ptr,[]),'delete':(None,[ptr]),'resource':(v,[ptr,v,v,v,ptr,sz]),'kind':(v,[ptr,v]),'register':(v,[ptr,vp,cp,vp]),'find':(v,[ptr,cp,cp]),'begin':(v,[ptr,v,cp,cp,vp]),'pointer':(ptr,[ptr,v]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args)in sigs.items():
   f=getattr(self.lib,'s26_window_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.state=self.fn['new']();assert self.state
 def resource(self,kind,x,y,body):return self.fn['resource'](self.state,kind,x,y,ctypes.create_string_buffer(body),len(body))
 def register(self,words,name,last_error):
  error=ctypes.c_uint32(last_error);value=self.fn['register'](self.state,(ctypes.c_uint32*9)(*words),name,ctypes.byref(error));return value,error.value
 def find(self,cls,title):return self.fn['find'](self.state,cls,title)
 def begin(self,tid,cls,title,args):return self.fn['begin'](self.state,tid,cls,title,(ctypes.c_uint32*12)(*args))
 def export(self):
  a=ctypes.create_string_buffer(self.fn['size']());assert self.fn['export'](self.state,a,len(a));return a.raw
 def restore(self,body):assert self.fn['import'](self.state,ctypes.create_string_buffer(body),len(body))==1 and self.export()==body
 def dispose(self):self.fn['delete'](self.state);self.state=None
