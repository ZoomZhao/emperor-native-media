"""Query only this Python caller's security session; no AppKit/game construction."""
import ctypes,json,os,threading
native=ctypes.CDLL('/System/Library/Frameworks/Security.framework/Security')
call=native.SessionGetInfo
call.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
call.restype=ctypes.c_int32
sid=ctypes.c_uint32();attrs=ctypes.c_uint32()
status=call(0xffffffff,ctypes.byref(sid),ctypes.byref(attrs))
print(json.dumps(dict(kind='PYTHON_OWN_SESSION_READ_ONLY',pid=os.getpid(),ppid=os.getppid(),
    uid=os.getuid(),euid=os.geteuid(),main_thread=threading.current_thread() is threading.main_thread(),
    status=status,session=sid.value,attributes=attrs.value,graphic_access=bool(attrs.value&0x10),
    remote=bool(attrs.value&0x1000),root_session=bool(attrs.value&1),session_has_TTY=bool(attrs.value&0x20),
    process_stdin_is_TTY=os.isatty(0),process_stdout_is_TTY=os.isatty(1),
    AppKit_constructed=False,original_game_loaded=False)),flush=True)
