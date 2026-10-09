import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeText:
 def __init__(self):
  m=json.loads((R/'evidence/stage33-text-build.json').read_text());p=Path(m['path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));v=ctypes.c_uint32;i=ctypes.c_int32;ptr=ctypes.c_void_p;sz=ctypes.c_size_t
  self.measure_fn=self.lib.s33_measure;self.measure_fn.restype=ctypes.c_int;self.measure_fn.argtypes=[ptr,sz,v,ctypes.POINTER(ctypes.c_double),ptr,sz];self.draw_fn=self.lib.s33_draw;self.draw_fn.restype=ctypes.c_int;self.draw_fn.argtypes=[ptr,v,v,v,i,i,ptr,sz,v,v,ctypes.POINTER(i),ctypes.POINTER(i)]
 def measure(self,body,variant=0):
  values=(ctypes.c_double*10)();name=ctypes.create_string_buffer(256);assert self.measure_fn(ctypes.create_string_buffer(body),len(body),variant,values,name,len(name));return dict(zip(['integer_width','integer_height','ascent','descent','leading','typographic_advance','ink_x','ink_y','ink_w','ink_h'],values),font_postscript_name=name.value.decode(),font_size=13,variant=variant)
 def draw(self,pixels,w,h,pitch,x,y,body,bk,text,clip):
  buf=ctypes.create_string_buffer(pixels,len(pixels));out=(ctypes.c_int32*6)();assert self.draw_fn(buf,w,h,pitch,x,y,ctypes.create_string_buffer(body),len(body),bk,text,(ctypes.c_int32*4)(*clip),out);return buf.raw,list(out)
