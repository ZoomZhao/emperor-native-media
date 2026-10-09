import json
from pathlib import Path
from stage12_state import decode
from stage43_window import NativeWindow
from stage43_pump import NativePump
R=Path(__file__).resolve().parents[1];m=json.loads((R/'runs/stage12-state-loop42/state.json').read_text());h=decode(m['host']);w=NativeWindow();p=NativePump(w)
try:
 w.restore(h['window_blob']);w.life_restore(h['window_lifecycle_blob']);before=w.diagnostic();status,msg,d=p.peek(0);after=w.diagnostic();out=dict(before=before,after=after,actual_queue_status=status,event_diagnostic=d,window=w.query(0x58000001),own_native_window_only=True,original_game_instructions_executed=False)
 (R/'evidence/stage43-background-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));assert status==0 and all(q['key_window']==q['activation_requests']==q['NSApp_isActive']==0 and q['frontmost_pid']!=q['current_pid']for q in [before,after])
finally:p.dispose();w.dispose()
