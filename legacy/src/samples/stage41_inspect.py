import json,struct,hashlib
from pathlib import Path
from stage12_state import decode
R=Path(__file__).resolve().parents[1];tag='recordvol40';p=R/f'runs/stage12-state-{tag}';d=json.loads((p/'state.json').read_text());regs=d['registers'];fields=dict(d['host']['dict'])
def mem(addr,n):
 r=next(s for s in d['regions']if s['low']<=addr and addr+n<=s['high']+1)
 with(p/'memory.bin').open('rb')as f:f.seek(r['offset']+addr-r['low']);return f.read(n)
def word(a):return struct.unpack('<I',mem(a,4))[0]
sp=regs['esp'];ret,gid,index=struct.unpack('<3I',mem(sp,12));base=word(17684684);offset,length=struct.unpack('<2I',mem(17676440+gid*8,8));start=base+offset;allocs=decode(fields['allocations']);owner=[dict(base=hex(a),heap=hex(v[0]),bytes=v[1],end=hex(a+v[1]))for a,v in allocs.items()if a<=regs['eax']<a+v[1]];sources=decode(fields['startup_file_calls']);textcalls=[c for c in sources if any(w in str(c).casefold()for w in ['.eng','.txt','.dat'])];out=dict(source=tag,registers={k:hex(v)for k,v in regs.items()},return_pc=hex(ret),group_id=gid,requested_item_index=index,remaining=regs['edx'],text_global_base=hex(base),table_group_offset=offset,table_group_second_word=length,group_pointer=hex(start),current_pointer=hex(regs['eax']),current_offset_in_group=regs['eax']-start,live_allocation_owners=owner,bytes_near_pointer_hex=mem(regs['eax']-16,64).hex(),recent_text_file_calls=textcalls[-20:]);(R/'evidence/stage41-inspection.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
