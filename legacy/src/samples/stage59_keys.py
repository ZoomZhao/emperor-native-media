"""Own event-domain binding; original scope explicitly refuses unknown hardware."""
import ctypes,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]


class KeyboardScopeError(RuntimeError):
    pass


class NativeKeys59:
    def __init__(self):
        build=json.loads((ROOT/'evidence/stage59-build.json').read_text())
        path=Path(build['library'])
        assert hashlib.sha256(path.read_bytes()).hexdigest()==build['sha256']
        self.lib=ctypes.CDLL(str(path));self.fn={}
        p=ctypes.c_void_p;u=ctypes.c_uint32;n=ctypes.c_size_t;i=ctypes.c_int
        for name,(result,args) in {
             'new':(p,[]),'diag':(i,[p,ctypes.POINTER(ctypes.c_uint64)]),
             'fixture_event':(i,[p,u,u,u,u]),'query_event_state':(i,[p,u,ctypes.POINTER(u)]),
             'query_original_scope':(i,[p,u,ctypes.POINTER(u)]),'size':(n,[]),
             'export':(i,[p,p,n]),'import':(i,[p,p,n,u]),'delete':(None,[p])}.items():
            function=getattr(self.lib,'s59_'+name);function.restype=result;function.argtypes=args
            self.fn[name]=function
        self.state=self.fn['new']();assert self.state

    def diagnostic(self):
        out=(ctypes.c_uint64*16)();assert self.fn['diag'](self.state,out)==0
        return dict(zip(['main_thread','app_active','key_window','can_become_key','visible','window_number',
                         'pid','frontmost_pid','delivered','accepted_events','ignored','foreign',
                         'fixture_events','down_mask','pending_mask','known_mask'],out))

    def fixture_event(self,key_code,down,repeat=False,other_window=False):
        return self.fn['fixture_event'](self.state,key_code,down,repeat,other_window)

    def query_event_state(self,vk):
        if type(vk) is not int or not 0<=vk<=255:raise KeyboardScopeError('invalid virtual key')
        out=ctypes.c_uint32(0xdeadbeef)
        status=self.fn['query_event_state'](self.state,vk,ctypes.byref(out))
        if status:raise KeyboardScopeError('local event state unobservable or unsupported; status='+str(status))
        return out.value

    def query_original_scope(self,vk):
        if type(vk) is not int or not 0<=vk<=255:raise KeyboardScopeError('invalid virtual key')
        out=ctypes.c_uint32(0xdeadbeef)
        status=self.fn['query_original_scope'](self.state,vk,ctypes.byref(out))
        if status:
            assert out.value==0xdeadbeef
            raise KeyboardScopeError('original system async state is unavailable from the task window; status='+str(status))
        return out.value

    def snapshot(self):
        size=self.fn['size']();assert size==48
        out=ctypes.create_string_buffer(size);assert self.fn['export'](self.state,out,size)==0
        return out.raw

    def restore(self,blob,replay=False):
        body=ctypes.create_string_buffer(blob)
        status=self.fn['import'](self.state,body,len(blob),replay)
        if status:raise KeyboardScopeError('invalid keyboard snapshot; status='+str(status))

    def dispose(self):
        if self.state:self.fn['delete'](self.state);self.state=None


class OriginalKeyboardAPI59:
    """Prepared generic stdcall adapter. Never supplies fake success under current scope."""
    def __init__(self,keyboard,guest,x):self.keyboard=keyboard;self.guest=guest;self.x=x
    def call(self):
        # No original caller-address or per-game-key patches.
        import struct
        sp=self.guest.reg_read(self.x.ESP)
        _,vk=struct.unpack('<2I',bytes(self.guest.mem_read(sp,8)))
        value=self.keyboard.query_original_scope(vk)
        self.guest.reg_write(self.x.EAX,ctypes.c_int16(value).value&0xffffffff)
        self.guest.reg_write(self.x.ESP,sp+8)
        self.guest.reg_write(self.x.EIP,struct.unpack('<I',bytes(self.guest.mem_read(sp,4)))[0])
