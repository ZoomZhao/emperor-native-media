import ctypes,hashlib,json
from pathlib import Path
from stage39_samples import NativeSamples as Legacy
R=Path(__file__).resolve().parents[1]
class NativeSamples(Legacy):
 def __init__(self,audio):
  m=json.loads((R/'evidence/stage46-native-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  spec={'new':(ptr,[ptr,v,v,ctypes.POINTER(v)]),'delete':(None,[ptr]),'allocate':(ctypes.c_int,[ptr,v,ctypes.POINTER(v)]),'init':(ctypes.c_int,[ptr,v]),'release':(ctypes.c_int,[ptr,v]),'query':(ctypes.c_int,[ptr,v,v,ctypes.POINTER(v)]),'set':(ctypes.c_int,[ptr,v,v,v]),'start_empty':(ctypes.c_int,[ptr,v]),'size':(sz,[]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz]),'full_size':(sz,[]),'full_export':(ctypes.c_int,[ptr,ptr,sz]),'full_import':(ctypes.c_int,[ptr,ptr,sz]),'service':(ctypes.c_int,[ptr]),'pull':(ctypes.c_int,[ptr,v]),'command':(ctypes.c_int,[ptr,v,v]),'drop_file':(ctypes.c_int,[ptr,v]),'bind':(ctypes.c_int,[ptr,v,v,ptr,sz]),'stats':(ctypes.c_int,[ptr,v,ctypes.POINTER(ctypes.c_uint64)])}
  for name,(ret,args)in spec.items():f=getattr(self.lib,'s46_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  f=self.lib.s46_zero_reverb;f.restype=ctypes.c_int;f.argtypes=[ptr,v];self.fn['zero_reverb']=f
  self.evidence=json.loads((R/'evidence/stage39-static.json').read_text());defaults=json.loads((R/'evidence/stage25-miles-semantics.json').read_text())['defaults'];self.audio=audio;self.state=self.fn['new'](audio.state,defaults[1],defaults[2],(v*128)(*self.evidence['pan_table']));assert self.state
 def full_snapshot(self):
  n=self.fn['full_size']();b=ctypes.create_string_buffer(n);assert self.fn['full_export'](self.state,b,n)==0;return b.raw
 def full_restore(self,b):assert self.fn['full_import'](self.state,ctypes.create_string_buffer(b),len(b))==0 and self.full_snapshot()==b
 def bind(self,h,file,body):return self.fn['bind'](self.state,h,file,ctypes.create_string_buffer(body),len(body))
 def stats(self,h):
  v=(ctypes.c_uint64*21)();status=self.fn['stats'](self.state,h,v);return status,dict(zip(['handle','attached','status','file','data_offset','PCM_bytes','frames','rate','loop_count','volume','pan','left_gain','right_gain','queued','source','consumed','nonzero','EOS','PCM_fnv64','mixed_fnv64','audio_clock'],v))
