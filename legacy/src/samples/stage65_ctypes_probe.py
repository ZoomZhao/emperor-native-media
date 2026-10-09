"""Own main-thread AppKit-only diagnostic; no emulator, game, adapter or resource."""
import ctypes,hashlib,json,os,sys,threading
from pathlib import Path
root=Path(__file__).resolve().parents[1]
library=root/'runs/stage65-appkit-raw.dylib'
print(json.dumps(dict(kind='PYTHON_BEFORE_OWN_LIBRARY',pid=os.getpid(),ppid=os.getppid(),uid=os.getuid(),
    euid=os.geteuid(),sid=os.getsid(0),main_thread=threading.current_thread() is threading.main_thread(),
    executable=sys.executable,library_path=str(library),library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),
    environment={k:os.environ[k] for k in ['TERM','SSH_TTY','XPC_SERVICE_NAME','XPC_FLAGS','__CFBundleIdentifier','COMMAND_MODE'] if k in os.environ},
    original_game_loaded=False)),flush=True)
native=ctypes.CDLL(str(library));call=native.s65_construct;call.argtypes=[];call.restype=ctypes.c_int
status=call()
print(json.dumps(dict(kind='PYTHON_NORMAL_RETURN',status=status)),flush=True)
raise SystemExit(status)
