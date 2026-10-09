import json,ctypes,time
from pathlib import Path
from stage12_state import decode
from stage44_window import NativeWindow
R=Path(__file__).resolve().parents[1];h=decode(json.loads((R/'runs/stage12-state-final43/state.json').read_text())['host']);w=NativeWindow()
try:
 w.restore(h['window_blob']);w.life_restore(h['window_lifecycle_blob']);before=w.diagnostic();from stage37_audio import NativeAudio;audio=NativeAudio();audio.open();audio.start();time.sleep(1.5);audio.stop();audio.dispose()
 cg=ctypes.CDLL('/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics');cf=ctypes.CDLL('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation')
 cg.CGWindowListCopyWindowInfo.restype=ctypes.c_void_p;cg.CGWindowListCopyWindowInfo.argtypes=[ctypes.c_uint32,ctypes.c_uint32];cf.CFArrayGetCount.restype=ctypes.c_long;cf.CFArrayGetCount.argtypes=[ctypes.c_void_p];cf.CFRelease.argtypes=[ctypes.c_void_p]
 # Option 1 returns only windows currently onscreen, scoped to own window ID.
 wid=w.fn['pointer'](w.state,0x58000001)
 # Native helper below gives exact window number and services AppKit.
 lib=ctypes.CDLL(str(R/'runs/stage44-native/libstage44_window.dylib'));f=lib.s44_visible;f.restype=ctypes.c_int;f.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 out=(ctypes.c_uint32*6)();assert f(w.state,out);after=w.diagnostic()
 result=dict(before=before,after=after,window_number=out[0],visible=out[1],key=out[2],active=out[3],CG_onscreen=out[4],CG_owner_pid=out[5],original_instructions_executed=False)
 (R/'evidence/stage44-window-probe.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));assert out[4]==out[1]==1 and out[5]==after['current_pid'] and out[2]==out[3]==0 and after['frontmost_pid']!=after['current_pid']
finally:w.dispose()
