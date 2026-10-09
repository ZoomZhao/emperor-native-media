import ctypes,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeDD:
 def __init__(self,window,w,h):
  m=json.loads((ROOT/'evidence/stage31-dd-build.json').read_text());p=Path(m['path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn={};v=ctypes.c_uint32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t;vp=ctypes.POINTER(v);i=ctypes.c_int32
  sigs={'new':(ptr,[ptr,v,v]),'delete':(None,[ptr]),'create':(v,[ptr,v]),'addref':(v,[ptr,v]),'release':(v,[ptr,v]),'coop':(ctypes.c_int,[ptr,v,v,v]),'mode':(ctypes.c_int,[ptr,v,v,v,v]),'display':(ctypes.c_int,[ptr,vp]),'restore_mode':(ctypes.c_int,[ptr,v]),'desktop_caps':(ctypes.c_int,[ptr,v,vp]),'surface':(v,[ptr,v,v,v,v,v,v]),'info':(ctypes.c_int,[ptr,v,vp]),'pixel_size':(sz,[ptr,v]),'pixels':(ctypes.c_int,[ptr,v,ptr,sz,v]),'lock':(ctypes.c_int,[ptr,v]),'unlock':(ctypes.c_int,[ptr,v]),'clipper_hwnd':(ctypes.c_int,[ptr,v,v]),'attach_clipper':(ctypes.c_int,[ptr,v,v]),'blt':(ctypes.c_int,[ptr,v,i,i,v,ctypes.POINTER(i)]),'size':(sz,[ptr]),'export':(ctypes.c_int,[ptr,ptr,sz]),'import':(ctypes.c_int,[ptr,ptr,sz])}
  for name,(ret,args)in sigs.items():
   f=getattr(self.lib,'s29_'+name);f.restype=ret;f.argtypes=args;self.fn[name]=f
  f=self.lib.s31_fill;f.restype=ctypes.c_int;f.argtypes=[ptr,v,v,ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_int32)];self.fn['fill']=f
  f=self.lib.s30_storage_report;f.restype=ctypes.c_int;f.argtypes=[ptr,vp];self.fn['storage_report']=f
  self.state=self.fn['new'](window,w,h);assert self.state,'real native DD device/queue initialization failed'
 def storage_report(self):
  a=(ctypes.c_uint32*10)();assert self.fn['storage_report'](self.state,a);return dict(zip(['NSWindowDepth','AppKit_bits_per_pixel','AppKit_bits_per_sample','bitmap_bits_per_pixel','bitmap_bits_per_component','bitmap_bytes_per_row_2_pixels','Metal_pixel_format','texture_width','texture_height','Metal_16_byte_roundtrip_equal'],a))
 def info(self,id):
  a=(ctypes.c_uint32*12)();assert self.fn['info'](self.state,id,a);return dict(zip(['kind','refs','caps','hwnd','coop','clip','w','h','pitch','fmt','locked','reserved'],a))
 def display(self):
  a=(ctypes.c_uint32*4)();assert self.fn['display'](self.state,a);return list(a)
 def pixels(self,id,body=None):
  n=self.fn['pixel_size'](self.state,id);assert n
  a=ctypes.create_string_buffer(body,n)if body is not None else ctypes.create_string_buffer(n);assert len(a)==n and self.fn['pixels'](self.state,id,a,n,body is not None);return a.raw
 def export(self):
  a=ctypes.create_string_buffer(self.fn['size'](self.state));assert self.fn['export'](self.state,a,len(a));return a.raw
 def restore(self,body):assert self.fn['import'](self.state,ctypes.create_string_buffer(body),len(body))and self.export()==body
 def dispose(self):self.fn['delete'](self.state);self.state=None
