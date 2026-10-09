import json,struct
from pathlib import Path
from stage12_state import decode
from stage44_window import NativeWindow
from stage45_pump import NativePump
R=Path(__file__).resolve().parents[1];h=decode(json.loads((R/'runs/stage12-state-stable44/state.json').read_text())['host']);w=NativeWindow();p=NativePump(w)
try:
 w.restore(h['window_blob']);w.life_restore(h['window_lifecycle_blob']);events=[];before=w.diagnostic()
 for msg in [0x200,0x201,0x202]:assert p.inject(msg,[360,363])==0
 blob=p.route_snapshot();p.route_restore(blob);assert p.route_diagnostic()['owned_pending_events']==3
 for msg in [0x200,0x201,0x202]:
  status,actual,d=p.peek(1);events.append(dict(status=status,MSG32=actual,diagnostic=d));print(json.dumps(dict(events=events,route=p.route_diagnostic())),flush=True);assert status==1 and actual[1]==msg and actual[3]==360|(363<<16)
 status,_,d=p.peek(1);assert status==0;route=p.route_diagnostic();assert route['screen']==[568,457]and route['client']==[360,363];blob=p.route_snapshot();p.route_restore(blob);after=w.diagnostic();assert before['frontmost_pid']==after['frontmost_pid']!=after['current_pid']and after['key_window']==after['NSApp_isActive']==after['activation_requests']==0
 out=dict(events=events,route=route,before=before,after=after,owned_window_NSEvent_FIFO=True,global_events_posted=False,global_cursor_moved=False,original_game_code_executed=False)
 (R/'evidence/stage45-pump-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
finally:p.dispose();w.dispose()
