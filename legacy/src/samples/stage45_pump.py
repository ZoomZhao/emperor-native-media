from stage43_pump import NativePump as Base,R,ctypes,json,hashlib
class NativePump(Base):
 def __init__(self,window):
  m=json.loads((R/'evidence/stage45-pump-build.json').read_text());p=R/'runs/stage45-native/libstage45_pump.dylib';assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;z=ctypes.c_size_t
  for name,(ret,args)in {'new':(v,[v]),'peek':(ctypes.c_int,[v,w,w,w,ctypes.POINTER(w),ctypes.POINTER(w)]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v]),'inject':(ctypes.c_int,[v,w,ctypes.c_int32,ctypes.c_int32]),'pointer':(ctypes.c_int,[v,ctypes.POINTER(ctypes.c_int32)]),'route_size':(z,[]),'route_export':(ctypes.c_int,[v,v,z]),'route_import':(ctypes.c_int,[v,v,z])}.items():
   f=getattr(self.lib,'s45p_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.window=window;self.state=None
  f=self.lib.s45p_default;f.restype=ctypes.c_int;f.argtypes=[v,w,w,w,w,ctypes.POINTER(w)];self.fn['default']=f
 def inject(self,message,point):self.ensure();return self.fn['inject'](self.state,message,*point)
 def pointer(self):
  self.ensure();out=(ctypes.c_int32*2)();status=self.fn['pointer'](self.state,out);return list(out)if status else None
 def route_snapshot(self):
  self.ensure();n=self.fn['route_size']();out=ctypes.create_string_buffer(n);assert self.fn['route_export'](self.state,out,n);return out.raw
 def route_restore(self,b):self.ensure();assert self.fn['route_import'](self.state,ctypes.create_string_buffer(b),len(b))and self.route_snapshot()[:len(b)]==b
 def route_diagnostic(self):
  import struct
  words=struct.unpack('<'+str(self.fn['route_size']()//4)+'i',self.route_snapshot());return dict(route_enabled=words[2],sequence=words[3],screen=list(words[4:6]),client=list(words[6:8]),framework_events=words[8],input_events=words[9],owned_pending_events=words[650],traces=[list(words[10+i*10:20+i*10])for i in range(min(64,words[8]+words[9]))])
