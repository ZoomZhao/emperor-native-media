"""Thin ctypes wrapper around task-authored native arm64 C ABI state machine."""
import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
REFUSED=0xfffffffe
class NativeEvents:
    def __init__(self):
        binary=ROOT/'runs/stage13-native/libstage13_events.dylib';record=json.loads((ROOT/'evidence/stage13-events-build.json').read_text())
        assert hashlib.sha256(binary.read_bytes()).hexdigest()==record['sha256']
        self.lib=ctypes.CDLL(str(binary));p=ctypes.c_void_p;v=ctypes.c_uint32;err=ctypes.POINTER(v)
        signatures={'new':(p,[]),'delete':(v,[p]),'create':(v,[p,v,v,err]),'set':(v,[p,v,err]),'reset':(v,[p,v,err]),'close':(v,[p,v,err]),'wait':(v,[p,v,v,err]),'query':(v,[p,v,err,err,err]),'snapshot_size':(ctypes.c_size_t,[]),'export':(v,[p,p,ctypes.c_size_t]),'import':(v,[p,p,ctypes.c_size_t])}
        for name,(ret,args) in signatures.items():
            symbol='s13_events_'+name if name in ['new','delete','snapshot_size','export','import'] else 's13_event_'+name
            f=getattr(self.lib,symbol);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
        self.state=self.new_c();assert self.state
        self.wsa_references=ctypes.c_uint32()
        self.wsa_startup_c=self.lib.s13_wsa_startup;self.wsa_startup_c.restype=v;self.wsa_startup_c.argtypes=[err,v,p,ctypes.c_size_t,err]
        self.wsa_cleanup_c=self.lib.s13_wsa_cleanup;self.wsa_cleanup_c.restype=v;self.wsa_cleanup_c.argtypes=[err,err]
        self.clock_get_c=self.lib.s13_clock_get;self.clock_get_c.restype=ctypes.c_int;self.clock_get_c.argtypes=[v,p,ctypes.c_size_t,err,err]
        self.clock_from_unix_c=self.lib.s13_clock_from_unix;self.clock_from_unix_c.restype=ctypes.c_int;self.clock_from_unix_c.argtypes=[ctypes.c_uint64,ctypes.c_int,p,ctypes.c_size_t]
        self.directory_create_c=self.lib.s13_directory_create;self.directory_create_c.restype=ctypes.c_int;self.directory_create_c.argtypes=[ctypes.c_char_p,err,err]
        self.disk_space_c=self.lib.s13_disk_space;self.disk_space_c.restype=ctypes.c_int;self.disk_space_c.argtypes=[ctypes.c_char_p,err,err]
    def invoke(self,name,args,last_error):
        error=ctypes.c_uint32(last_error);value=getattr(self,name+'_c')(self.state,*args,ctypes.byref(error));return value,error.value
    def query(self,h):
        manual=ctypes.c_uint32();signaled=ctypes.c_uint32();waiters=ctypes.c_uint32()
        live=self.query_c(self.state,h,ctypes.byref(manual),ctypes.byref(signaled),ctypes.byref(waiters))
        return dict(live=bool(live),manual=manual.value,signaled=signaled.value,waiters=waiters.value)
    def wsa_startup(self,version,body,last_error):
        error=ctypes.c_uint32(last_error);buf=ctypes.create_string_buffer(body) if body is not None else None
        result=self.wsa_startup_c(ctypes.byref(self.wsa_references),version,buf,len(body) if body is not None else 0,ctypes.byref(error))
        return result,error.value,buf.raw[:-1] if buf is not None else None
    def wsa_cleanup(self,last_error):
        error=ctypes.c_uint32(last_error);result=self.wsa_cleanup_c(ctypes.byref(self.wsa_references),ctypes.byref(error));return result,error.value
    def clock(self,kind,size,last_error):
        buf=ctypes.create_string_buffer(size) if size else None;ret=ctypes.c_uint32();error=ctypes.c_uint32(last_error)
        status=self.clock_get_c(kind,buf,size,ctypes.byref(ret),ctypes.byref(error));assert status==1,'native clock boundary refused'
        return ret.value,error.value,buf.raw if buf is not None else b''
    def directory_create(self,path,last_error):
        ret=ctypes.c_uint32();error=ctypes.c_uint32(last_error);assert self.directory_create_c(str(path).encode(),ctypes.byref(ret),ctypes.byref(error))==1,'native filesystem error outside measured contract'
        return ret.value,error.value
    def disk_space(self,path,last_error):
        out=(ctypes.c_uint32*4)();error=ctypes.c_uint32(last_error);assert self.disk_space_c(str(path).encode(),out,ctypes.byref(error))==1,'native disk geometry outside finite DWORD scope'
        return list(out),error.value
    def export(self):
        size=self.snapshot_size_c();buf=ctypes.create_string_buffer(size);result=self.export_c(self.state,buf,size);assert result==1,('snapshot refused',result);return buf.raw
    def restore(self,body):
        buf=ctypes.create_string_buffer(body);assert self.import_c(self.state,buf,len(body))==1;assert self.export()==body
    def dispose(self):
        if self.state:assert self.delete_c(self.state)==1;self.state=None
