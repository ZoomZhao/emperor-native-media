import ctypes
from stage16_paths import NativePaths
class NativeVolume(NativePaths):
 def __init__(self,guest=b'C:\\Emperor'):
  super().__init__(guest,binary='runs/stage17-native/libstage17_volume.dylib',build_record='evidence/stage17-volume-build.json')
  self.volume_c=self.lib.s17_volume_read;self.volume_c.restype=ctypes.c_int;self.volume_c.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t]
 def volume(self,root,mask=6,capacity=24):
  body=ctypes.create_string_buffer(bytes([0xa5])*capacity);status=self.volume_c(self.state,root,mask,body,capacity);return status,body.raw[:-1]
