"""Read existing city checkpoint bytes, typed allocator state and save locations.

No restore, engine import, caller invocation or game field writes.
"""
import gc,hashlib,json,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage76';sys.path.insert(0,str(S/'tools'));sys.path.insert(0,str(ROOT/'samples'))
from saved_handlers import field,read
from stage60_controlled_input_plan import decode
def typedfield(path,key):
 # Saved host state is tagged as {dict:[[key,value],...]}, not plain JSON.
 import re
 pattern=re.compile(r'\["'+re.escape(key)+r'",');buf='';found=False
 with path.open()as f:
  while True:
   chunk=f.read(1<<20)
   if not chunk:return None
   buf+=chunk
   if not found:
    m=pattern.search(buf)
    if not m:buf=buf[-128:];continue
    buf=buf[m.end():];found=True
   try:return decode(json.JSONDecoder().raw_decode(buf)[0])
   except json.JSONDecodeError:pass
   if len(buf)>32<<20:raise ValueError('typed field extraction budget '+key)
def main():
 # The immutable image regions precede changing heap/worker mappings. Validate
 # their hashes per candidate before reusing offsets; never assume all regions
 # in different checkpoints have identical layout.
 manifest=json.loads((ROOT/'evidence/stage73-run-manifest.json').read_text());image=[r for r in manifest['regions']if r['low']<0x2000000];rows=[]
 for state in sorted((ROOT/'runs').glob('stage12-state-*/state.json')):
  if not state.parent.name.startswith(('stage12-state-cycle','stage12-state-frame','stage12-state-city')):continue
  regions=field(state,'regions');path=state.parent/'memory.bin'
  if not regions or not path.exists():continue
  try:
   begin,end=struct.unpack('<II',read(path,regions,0x10bc918,8));count=(end-begin)//4;assert end>=begin and count<=100000
   objects=struct.unpack('<I',read(path,regions,0x8c7638))[0];active=[];eligible=[]
   for k in range(1,count):
    record=struct.unpack('<I',read(path,regions,begin+4*k))[0]
    if not record:continue
    header=read(path,regions,record+8,6);index=struct.unpack_from('<h',header)[0]
    if not header[4]:continue
    r=dict(record=hex(record),index=index);active.append(r)
    if index:
     obj=struct.unpack('<I',read(path,regions,(objects+4*index)&0xffffffff))[0];body=read(path,regions,obj,24);status=body[4];r.update(object=hex(obj),vtable=hex(struct.unpack_from('<I',body)[0]),status=status,typeword=struct.unpack_from('<h',body,20)[0])
     if status in(1,3):eligible.append(r)
   rows.append(dict(checkpoint=state.parent.name,vector_count=count,active_records=len(active),eligible_records=len(eligible),eligible=eligible,actual_phase8_entry_CPU_not_assumed=True))
  except (ValueError,AssertionError)as e:rows.append(dict(checkpoint=state.parent.name,not_readable_for_phase8=str(e)))
  gc.collect()
 latest=ROOT/'runs/stage12-state-cycle62p19/state.json';heaps=typedfield(latest,'heaps');allocations=typedfield(latest,'allocations');allocator=dict(heaps=heaps,live_allocation_count=len(allocations)if allocations else None,heap_next=typedfield(latest,'heap_next'),heap_mapped_end34=typedfield(latest,'heap_mapped_end34'),importing_into_old_64KiB_128_block_native_arena_possible=False);del allocations
 saves=[]
 for p in Path('<LOCAL_ORIGINAL_PROJECT>/GameData/Save').rglob('*.sav'):
  h=hashlib.sha256()
  with p.open('rb')as f:
   for b in iter(lambda:f.read(1<<20),b''):h.update(b)
  saves.append(dict(path=str(p),bytes=p.stat().st_size,sha256=h.hexdigest(),game_entity_activity_not_decoded=True,file_loaded_or_executed=False))
 out=dict(status='READ_ONLY_REAL_INPUT_EVIDENCE',checkpoints=rows,checkpoints_with_eligible_objects=[r['checkpoint']for r in rows if r.get('eligible_records',0)>0],latest_allocator=allocator,legal_local_save_candidates=saves,source_save_selection_not_authorized_or_performed=True,new_game_executions=0)
 (S/'evidence/input-evidence.json').write_text(json.dumps(out,indent=2,ensure_ascii=False)+'\n');print(json.dumps(dict(checkpoints=len(rows),eligible_checkpoints=out['checkpoints_with_eligible_objects'],allocator=allocator,local_saves=len(saves))))
if __name__=='__main__':main()
