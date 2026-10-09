import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeBink:
 def __init__(self,audio):
  m=json.loads((R/'evidence/stage42-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];sm=json.loads((R/'evidence/stage39-native-build.json').read_text());sp=Path(sm['library']);assert hashlib.sha256(sp.read_bytes()).hexdigest()==sm['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;wp=ctypes.POINTER(w);z=ctypes.c_size_t;c=ctypes.c_char_p
  spec={'new':(v,[v,c]),'set_sound':(ctypes.c_int,[v,w,w,wp]),'open_missing':(ctypes.c_int,[v,c,c,w,wp]),'error':(ctypes.c_int,[v,v,w]),'capabilities':(ctypes.c_int,[v,wp]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}
  for name,(ret,args)in spec.items():f=getattr(self.lib,'s42_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.audio=audio;self.state=self.fn['new'](audio.state,str(sp).encode());assert self.state
 def set_sound(self,factory,driver):
  out=ctypes.c_uint32();status=self.fn['set_sound'](self.state,factory,driver,ctypes.byref(out));return status,out.value
 def open_missing(self,root,relative,flags):
  out=ctypes.c_uint32();status=self.fn['open_missing'](self.state,str(root).encode(),relative.encode(),flags,ctypes.byref(out));return status,out.value
 def error(self):
  out=ctypes.create_string_buffer(256);assert self.fn['error'](self.state,out,256)==0;return out.value
 def capabilities(self):
  out=(ctypes.c_uint32*8)();assert self.fn['capabilities'](self.state,out)==0;return dict(zip(['real_audio_live','real_sample_allocate_entry','real_minimum_buffer_size_entry','factory_calls','sound_open_registered','open_calls','readonly_IO_attempts','last_errno'],list(out)))
 def snapshot(self):
  n=self.fn['size']();out=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,out,n)==0;return out.raw
 def restore(self,b):assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==0 and self.snapshot()==b
 def dispose(self):self.fn['delete'](self.state);self.state=None
