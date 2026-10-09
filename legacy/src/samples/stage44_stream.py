import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeStreams:
 def __init__(self,audio):
  m=json.loads((R/'evidence/stage44-stream-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_void_p;w=ctypes.c_uint32;z=ctypes.c_size_t
  for name,(ret,args)in {'new':(v,[v]),'open':(ctypes.c_int,[v,ctypes.c_char_p,w,ctypes.POINTER(w)]),'command':(ctypes.c_int,[v,w,w,w]),'status':(ctypes.c_int,[v,w]),'service':(ctypes.c_int,[v]),'pull':(ctypes.c_int,[v,w]),'stats':(ctypes.c_int,[v,ctypes.POINTER(ctypes.c_uint64)]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}.items():
   f=getattr(self.lib,'s44_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  self.audio=audio;self.state=self.fn['new'](audio.state);assert self.state
 def open(self,path,bytes=256000):
  out=ctypes.c_uint32();status=self.fn['open'](self.state,str(path).encode(),bytes,ctypes.byref(out));return status,out.value
 def command(self,h,c,v=0):return self.fn['command'](self.state,h,c,v)
 def service(self):return self.fn['service'](self.state)
 def stats(self):
  out=(ctypes.c_uint64*14)();assert self.fn['stats'](self.state,out)==0;return dict(zip(['handle','status','loop_count','volume','decoded_frames','decoded_pcm_fnv64','consumed_frames','mixed_pcm_fnv64','queued_frames','source_cursor','queued_passes','actual_audio_render_clock','nonzero_mixed_samples','decoder_file_open'],list(out)))
 def snapshot(self):
  n=self.fn['size']();out=ctypes.create_string_buffer(n);assert self.fn['export'](self.state,out,n)==0;return out.raw
 def restore(self,b):
  assert self.fn['import'](self.state,ctypes.create_string_buffer(b),len(b))==0
  assert self.snapshot()==b
 def dispose(self):
  if self.state:self.fn['delete'](self.state);self.state=None
