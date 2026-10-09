"""Checkpoint JSON data costs only; no engine import or game logic execution."""
import hashlib,json,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
R=Path(__file__).resolve().parents[3];V=R/'stage83/budget-review-v1';sys.path.insert(0,str(R/'stage83/batch-v1/tools'))
from common import decode,encode,put
class Count:
 def __init__(self):self.bytes=0;self.parts=0
 def write(self,s):self.bytes+=len(s);self.parts+=1
start=time.monotonic();source=R/'runs/stage12-state-cycle62p19/state.json';s=json.loads(source.read_text());parsed=time.monotonic();host=decode(s['host']);decoded=time.monotonic();encoded=encode(host);encoded_at=time.monotonic();sink=Count();json.dump(encoded,sink,indent=2);done=time.monotonic()
put(V/'evidence/host-data-cost.json',{'status':'AUTHORED_DATA_ONLY_COST_MEASURED','source_bytes':source.stat().st_size,'parse_seconds':parsed-start,'decode_seconds':decoded-parsed,'encode_seconds':encoded_at-decoded,'pretty_JSON_dump_to_counting_sink_seconds':done-encoded_at,'serialized_host_ASCII_characters':sink.bytes,'JSON_write_chunks':sink.parts,'full_guest_memory_bytes':sum(r['high']-r['low']+1 for r in s['regions']),'filesystem_snapshot_write_cost_measured':False,'Unicorn_restore_cost_measured':False,'opaque_context_serialization_cost_measured':False,'game_logic_executed':0,'guest_or_input_modified':False})
print(json.dumps({'parse_decode_seconds':decoded-start,'encode_dump_seconds':done-decoded,'serialized_host_characters':sink.bytes,'original_execution':0}))
