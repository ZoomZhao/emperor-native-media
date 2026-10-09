import ctypes,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class NativeInput:
 def __init__(self,window):
  m=json.loads((R/'evidence/stage43-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.window=window;i=ctypes.c_int32;d=ctypes.c_double;v=ctypes.c_void_p;self.cursor_fn=self.lib.s43_cursor;self.cursor_fn.restype=ctypes.c_int;self.cursor_fn.argtypes=[v,ctypes.POINTER(i),ctypes.POINTER(d)];self.convert_fn=self.lib.s43_convert;self.convert_fn.restype=ctypes.c_int;self.convert_fn.argtypes=[v,ctypes.c_uint32,ctypes.POINTER(i),ctypes.POINTER(i),ctypes.POINTER(d)]
 def pointer(self):return self.window.fn['pointer'](self.window.state,0x58000001)
 def query(self,point=None,to_screen=False):
  out=(ctypes.c_int32*2)();diag=(ctypes.c_double*14)()
  status=self.cursor_fn(self.pointer(),out,diag)if point is None else self.convert_fn(self.pointer(),to_screen,(ctypes.c_int32*2)(*point),out,diag)
  names=['screen_top','native_client_x','native_client_bottom','native_client_w','native_client_h','backing_scale','backing_client_w','backing_client_h','app_active','key_window','screen_count','view_flipped','raw_result_x','raw_result_y'];return status,list(out),dict(zip(names,diag))
