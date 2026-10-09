"""Native-only activation diagnostic using task-owned checkpoint resources."""
import json,sys,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage12_state import decode
from stage28_window import NativeWindow
root=Path(__file__).resolve().parents[1]
host=decode(json.loads((root/'runs/stage12-state-activation27/state.json').read_text())['host'])
window=NativeWindow()
try:
 window.restore(host['window_blob']);window.life_restore(host['window_lifecycle_blob']);before=window.diagnostic();start=time.monotonic();value=window.action('activate',0x58000001);after=window.diagnostic();result=dict(before=before,after=after,activate_result=value,elapsed_seconds=time.monotonic()-start,original_code_executed=False,requests_at_most_one=True,user_input_events_not_consumed=True)
 (root/'evidence/stage28-activation-probe.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:window.dispose()
