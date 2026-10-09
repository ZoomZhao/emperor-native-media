import json,struct,hashlib
from pathlib import Path
R=Path(__file__).resolve().parents[1];d=json.loads((R/'runs/stage12-state-recordvol40/state.json').read_text());p=R/'runs/stage12-state-recordvol40/memory.bin'
def mem(a,n):
 r=next(s for s in d['regions']if s['low']<=a and a+n<=s['high']+1)
 with p.open('rb')as f:f.seek(r['offset']+a-r['low']);return f.read(n)
base=struct.unpack('<I',mem(17684684,4))[0];table=mem(17676440,8000);blob=mem(base,201408);b=(R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/EmperorText.eng').read_bytes();assert hashlib.sha256(b).hexdigest()=='9b9f642342ea34c7ab8a27a726467bdc3ae1ad073f6fa6a4af5ba9c01f343cbf'
header_matches=table==b[28:8028];payload=b[8028:];same=blob[:len(payload)]==payload;gid=56;offset,count=struct.unpack_from('<2I',table,8*gid);cursor=offset;remaining=64;steps=0;budget=len(payload)-offset
while remaining>0 and steps<budget:
 if blob[cursor]==0 and(blob[cursor-1]>=32 or blob[cursor-1]==0):remaining-=1
 cursor+=1;steps+=1
if remaining==0:
 while cursor<len(blob)and blob[cursor]<32:cursor+=1
out=dict(header_bytes=28,group_table_bytes=8000,original_file_bytes=len(b),text_allocation_bytes=len(blob),text_file_payload_bytes=len(payload),entire_guest_group_table_equals_original_file=header_matches,entire_defined_guest_text_payload_equals_original_file=same,unused_allocation_tail_hex=blob[len(payload):].hex(),allocation_tail_contract='two uninitialized allocation bytes are outside the defined file payload; not NUL sentinels',guest_defined_payload_sha256=hashlib.sha256(blob[:len(payload)]).hexdigest(),expected_payload_sha256=hashlib.sha256(payload).hexdigest(),group_id=gid,group_offset=offset,group_count=count,requested_index=64,shadow_original_predicates_remaining=remaining,shadow_expected_pointer=hex(base+cursor),shadow_loop_byte_steps=steps,observed_pointer=hex(d['registers']['eax']),observed_remaining=d['registers']['edx'],source='fixed original file bytes + readonly checkpoint + own shadow of visible original predicates; no loop or input modified')
(R/'evidence/stage41-text-contract-corrected.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));assert same and header_matches and remaining==0
