import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeRegistry:
 def __init__(self,audio):
  m=json.loads((R/'evidence/stage40-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;wp=ctypes.POINTER(w);z=ctypes.c_size_t;c=ctypes.c_char_p
  spec={'new':(v,[v]),'start':(ctypes.c_int,[v]),'shutdown':(ctypes.c_int,[v]),'enumerate':(ctypes.c_int,[v,c,wp,wp]),'name':(ctypes.c_int,[v,w,ctypes.c_int32,v,w]),'register_mss3d':(ctypes.c_int,[v]),'capabilities':(ctypes.c_int,[v,wp,v,w]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}
  for name,(ret,args)in spec.items():f=getattr(self.lib,'s40_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.audio=audio;self.state=self.fn['new'](audio.state);assert self.state
 def start(self):return self.fn['start'](self.state)
 def shutdown(self):return self.fn['shutdown'](self.state)
 def enumerate(self,service,cursor):
  cur=ctypes.c_uint32(cursor);out=ctypes.c_uint32();result=self.fn['enumerate'](self.state,service,ctypes.byref(cur),ctypes.byref(out));return result,cur.value,out.value
 def name(self,h,token=-100):
  out=ctypes.create_string_buffer(96);status=self.fn['name'](self.state,h,token,out,96);return status,out.value.decode()
 def capabilities(self):
  out=(ctypes.c_uint32*12)();name=ctypes.create_string_buffer(256);assert self.fn['capabilities'](self.state,out,name,256)==0
  d=dict(zip(['registered','device','service_mask','MSS3D_interface_exposed','live_audio','PCM_provider_handle','Apple_SpatialMixer_components','Apple_SpatialMixer_version'],list(out)[:8]));d.update(Apple_SpatialMixer_name=name.value.decode(),SpatialMixer_instantiated=False,Windows_DirectSound_or_RSX_provider_loaded=False);return d
 def snapshot(self):
  n=self.fn['size']();out=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,out,n)==0;return out.raw
 def restore(self,b):assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==0 and self.snapshot()==b
 def dispose(self):self.fn['delete'](self.state);self.state=None
