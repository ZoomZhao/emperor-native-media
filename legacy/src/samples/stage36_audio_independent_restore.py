import ctypes,hashlib,json,struct,time
from pathlib import Path
R=Path(__file__).resolve().parents[1];p=R/'runs/stage36-native/libstage36_audio.dylib';lib=ctypes.CDLL(str(p));ptr=ctypes.c_void_p;fn={}
for name,(ret,args)in {'new':(ptr,[]),'import':(ctypes.c_int,[ptr,ptr,ctypes.c_size_t]),'export':(ctypes.c_int,[ptr,ptr,ctypes.c_size_t]),'start':(ctypes.c_int,[ptr]),'stop':(ctypes.c_int,[ptr]),'delete':(None,[ptr])}.items():
 f=getattr(lib,'s36_'+name);f.restype=ret;f.argtypes=args;fn[name]=f
b=(R/'evidence/stage36-native-audio.state.bin').read_bytes();before=struct.unpack('<10I5Q',b);s=fn['new']()
try:
 assert fn['import'](s,ctypes.create_string_buffer(b),len(b));out=ctypes.create_string_buffer(len(b));assert fn['export'](s,out,len(b))and out.raw==b;assert fn['start'](s);time.sleep(.25);assert fn['stop'](s);assert fn['export'](s,out,len(b));after=struct.unpack('<10I5Q',out.raw);assert after[10]>before[10]and after[11]>before[11]and after[9]==0;result=dict(independent_process_restore_passed=True,original_blob_sha256=hashlib.sha256(b).hexdigest(),quiescent_byte_equal=True,device=after[5],callbacks_before=before[10],callbacks_after=after[10],frames_before=before[11],frames_after=after[11],original_MSS_or_provider_code_executed=False,scope='own host audio state only, not original game callback state');(R/'evidence/stage36-native-audio-independent-restore.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:fn['delete'](s)
