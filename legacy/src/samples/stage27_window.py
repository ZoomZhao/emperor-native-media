import ctypes,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeWindow:
 def __init__(self):
  p=ROOT/'runs/stage27-native/libstage27_window.dylib';m=json.loads((ROOT/'evidence/stage27-window-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t;cp=ctypes.c_char_p;vp=ctypes.POINTER(v)
  sigs={'new':(ptr,[]),'delete':(None,[ptr]),'resource':(v,[ptr,v,v,v,ptr,sz]),'kind':(v,[ptr,v]),'register':(v,[ptr,vp,cp,vp]),'find':(v,[ptr,cp,cp]),'begin':(v,[ptr,v,cp,cp,vp]),'pointer':(ptr,[ptr,v]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args)in sigs.items():
   f=getattr(self.lib,'s26_window_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.state=self.fn['new']();assert self.state
  extra={'alt_state':(ctypes.c_int,[vp,ctypes.POINTER(ctypes.c_uint64)]),'window_created':(ctypes.c_int,[ptr,v]),'window_show':(ctypes.c_int,[ptr,v]),'window_activate':(ctypes.c_int,[ptr,v]),'window_focus':(ctypes.c_int,[ptr,v,v,vp]),'window_query':(ctypes.c_int,[ptr,v,vp,sz]),'window_foreground':(v,[ptr]),'window_rect':(ctypes.c_int,[ptr,v,v,ctypes.POINTER(ctypes.c_int32)]),'window_dc':(v,[ptr,v]),'window_default':(ctypes.c_int,[ptr,v,v,v,ptr,sz,vp]),'life_size':(sz,[ptr]),'life_export':(ctypes.c_int,[ptr,ptr,sz]),'life_import':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args)in extra.items():
   f=getattr(self.lib,'s27_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
 def resource(self,kind,x,y,body):return self.fn['resource'](self.state,kind,x,y,ctypes.create_string_buffer(body),len(body))
 def register(self,words,name,last_error):
  error=ctypes.c_uint32(last_error);value=self.fn['register'](self.state,(ctypes.c_uint32*9)(*words),name,ctypes.byref(error));return value,error.value
 def find(self,cls,title):return self.fn['find'](self.state,cls,title)
 def begin(self,tid,cls,title,args):return self.fn['begin'](self.state,tid,cls,title,(ctypes.c_uint32*12)(*args))
 def export(self):
  a=ctypes.create_string_buffer(self.fn['size']());assert self.fn['export'](self.state,a,len(a));return a.raw
 def restore(self,body):assert self.fn['import'](self.state,ctypes.create_string_buffer(body),len(body))==1 and self.export()==body
 def alt(self):
  value=ctypes.c_uint32();flags=(ctypes.c_uint64*4)();status=self.fn['alt_state'](ctypes.byref(value),flags);return status,value.value,list(flags)
 def query(self,h):
  a=(ctypes.c_uint32*12)();assert self.fn['window_query'](self.state,h,a,12);return list(a)
 def rect(self,h,client):
  a=(ctypes.c_int32*4)();assert self.fn['window_rect'](self.state,h,client,a);return list(a)
 def action(self,name,h):return self.fn['window_'+name](self.state,h)
 def focus(self,tid,h):
  old=ctypes.c_uint32();status=self.fn['window_focus'](self.state,tid,h,ctypes.byref(old));return status,old.value
 def dc(self,h):return self.fn['window_dc'](self.state,h)
 def default(self,h,msg,wp,body):
  a=ctypes.create_string_buffer(body);v=ctypes.c_uint32();status=self.fn['window_default'](self.state,h,msg,wp,a,len(body),ctypes.byref(v));return status,v.value,a.raw[:len(body)]
 def life_export(self):
  a=ctypes.create_string_buffer(self.fn['life_size'](self.state));assert self.fn['life_export'](self.state,a,len(a));return a.raw
 def life_restore(self,body):assert self.fn['life_import'](self.state,ctypes.create_string_buffer(body),len(body))==1 and self.life_export()==body
 def dispose(self):self.fn['delete'](self.state);self.state=None
