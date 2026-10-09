import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def missing_profile(default,capacity):
 p=ROOT/'runs/stage19-native/libstage19_profile.dylib';m=json.loads((ROOT/'evidence/stage19-profile-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256']
 f=ctypes.CDLL(str(p)).s19_missing_profile;f.restype=ctypes.c_int;f.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
 buf=ctypes.create_string_buffer(bytes([0xa5])*512);n=ctypes.c_uint32();err=ctypes.c_uint32();directory=ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/windows';assert f(str(directory).encode(),default,buf,capacity,ctypes.byref(n),ctypes.byref(err))==1
 return n.value,err.value,buf.raw[:n.value+1],buf.raw[:512]
def version_file_missing():
 p=ROOT/'runs/stage19-native/libstage19_profile.dylib';m=json.loads((ROOT/'evidence/stage19-profile-build.json').read_text());assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256']
 f=ctypes.CDLL(str(p)).s19_missing_version_file;f.restype=ctypes.c_int;f.argtypes=[ctypes.c_char_p]
 directory=ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]';assert f(str(directory).encode())==1
