"""Real extracted C Unlock state transition; no window or rendering backend."""
import ctypes,struct
from common import V,sha,Boundary
def unlock(host,memory,object_pointer,pixel_pointer):
 obj=host['dd_objects'].get(object_pointer)
 if not obj or obj['kind']!=2:raise Boundary('UNLOCK_INVALID_COM32_OBJECT')
 if pixel_pointer not in(0,obj['guest_pixels']):raise Boundary('UNLOCK_POINTER_NOT_MATCHING_SURFACE')
 blob=host['dd_native_blob'];id=obj['id'];offset=48+(id-1)*48
 if not 1<=id<=64 or len(blob)<3120:raise Boundary('UNLOCK_CAPTURED_BLOB_ABI')
 fields=struct.unpack_from('<12I',blob,offset);kind,refs,caps,hwnd,coop,clip,w,h,pitch,fmt,locked,reserved=fields
 if kind!=2 or not refs or reserved:raise Boundary('UNLOCK_CAPTURED_OBJECT_MISMATCH')
 # Full source byte copy happens before C's tested locked->unlocked transition.
 pixels=memory.read(obj['guest_pixels'],pitch*h);lib=ctypes.CDLL(str(V/'builds/unlock-headless-v2.dylib'));ptr=ctypes.c_void_p
 lib.s84_import.argtypes=[ptr,ctypes.c_size_t];lib.s84_import.restype=ptr;lib.s84_delete.argtypes=[ptr];lib.s84_sync_unlock.argtypes=[ptr,ctypes.c_uint32,ptr,ctypes.c_size_t];lib.s84_sync_unlock.restype=ctypes.c_int;lib.s84_export.argtypes=[ptr,ptr,ctypes.c_size_t];lib.s84_export.restype=ctypes.c_int
 data=ctypes.create_string_buffer(blob,len(blob));state=lib.s84_import(data,len(blob))
 if not state:raise Boundary('UNLOCK_HEADLESS_IMPORT_FAILED')
 try:
  src=ctypes.create_string_buffer(pixels,len(pixels));ok=lib.s84_sync_unlock(state,id,src,len(pixels));out=ctypes.create_string_buffer(len(blob))
  if not lib.s84_export(state,out,len(blob)):raise Boundary('UNLOCK_HEADLESS_EXPORT_FAILED')
  host['dd_native_blob']=out.raw;return 0 if ok else 0x80004005
 finally:lib.s84_delete(state)
def dispatch(host,memory,dll,name,args):
 if(dll,name)!=('authored.dd','2.32')or len(args)!=2:raise Boundary('UNAPPROVED_PLATFORM_CALL '+dll+'!'+name)
 return unlock(host,memory,*args)
