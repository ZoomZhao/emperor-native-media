import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class NativeJPEG:
 def __init__(self):
  m=json.loads((ROOT/'evidence/stage32-jpeg-build.json').read_text());p=Path(m['path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.fn=self.lib.s32_jpeg;self.fn.restype=ctypes.c_int;self.fn.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32)]
 def read(self,data,decode=False):
  src=ctypes.create_string_buffer(data);info=(ctypes.c_uint32*10)();assert self.fn(src,len(data),None,0,info),'ImageIO JPEG scope/format/orientation unsupported';n=info[0]*info[1]*3;out=ctypes.create_string_buffer(n)if decode else None
  if decode:assert self.fn(src,len(data),out,n,info)
  return dict(zip(['width','height','source_bits_per_component','source_bits_per_pixel','source_bytes_per_row','source_bitmap_info','color_model','authored_orientation_test_flip','source_color_components','packed_BGR_stride'],info)),out.raw if decode else None
