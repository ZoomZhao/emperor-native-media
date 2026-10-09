"""Read existing saved memory only: phase8 objects, vtables and helper precondition.

No guest instructions, engine restore, or generated game-library calls occur.
"""
import collections,gc,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from stage60_controlled_input_plan import decode
from auto_i386_analysis import PE
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
class Memory:
 def __init__(self,path,regions):self.path=Path(path);self.regions=regions;self.file=self.path.open('rb')
 def read(self,a,n):
  for r in self.regions:
   size=r.get('size',r.get('high',0)+1-r['low'])
   if r['low']<=a and a+n<=r['low']+size:
    self.file.seek(r['offset']+a-r['low']);b=self.file.read(n);assert len(b)==n;return b
  raise ValueError('Unmapped saved read '+hex(a))
 def u32(self,a):return struct.unpack('<I',self.read(a,4))[0]
 def u16(self,a):return struct.unpack('<H',self.read(a,2))[0]
 def i16(self,a):return struct.unpack('<h',self.read(a,2))[0]
def inspect(tag,path,regions,pe):
 mem=Memory(path,regions);header=0x10bc910
 vector_base=mem.u32(header+8);vector_end=mem.u32(header+12)
 count=(vector_end-vector_base)//4 if vector_end>=vector_base else None
 assert count is not None and count<100000 and (vector_end-vector_base)%4==0
 helper=dict(receiver=hex(header),index=1,member_vector_receiver=hex(header+4),begin=hex(vector_base),end=hex(vector_end),count=count,
  original_554c00_result=count,original_413a50_branch_to_413a80=count<=1,
  exact_condition='unsigned((end-begin)>>2) > argument1 takes JA and bypasses413a80; ordinary count interpreted only after saved validity checks')
 root_slots=[];selected=[];all_objects={};byvt=collections.Counter();unmapped=[]
 container_begin=mem.u32(0x8c7638);container_end=mem.u32(0x8c763c)
 assert container_end>=container_begin and (container_end-container_begin)%4==0 and container_end-container_begin<1048576
 for index in range((container_end-container_begin)//4):
  obj=mem.u32(container_begin+4*index)
  if not obj:continue
  try:
   vt=mem.u32(obj);s=pe.section(vt)
   if not s or s['flags']&0x20000000:continue
   t200=mem.u32(vt+200);t612=mem.u32(vt+612)
   all_objects[obj]=dict(index=index,address=hex(obj),vtable=hex(vt),target200=hex(t200),target612=hex(t612),
    active_state=mem.read(obj+4,1)[0],type_word=mem.i16(obj+20),field72=mem.read(obj+72,1)[0],field36=mem.i16(obj+36))
   if all_objects[obj]['type_word']==53:
    child=mem.u32(obj+336);child_vtable=mem.u32(child)if child else 0
    all_objects[obj]['child336']=dict(pointer=hex(child),vtable=hex(child_vtable),slot0_target=hex(mem.u32(child_vtable))if child_vtable else None,array340=hex(mem.u32(obj+340)))
   byvt[hex(vt)]+=1
  except ValueError as e:unmapped.append(dict(index=index,address=hex(obj),error=str(e)))
 for i in range(1,count):
  record=mem.u32(vector_base+4*i)
  if not record:root_slots.append(dict(index=i,record=0,null_record=True));continue
  try:
   active=mem.read(record+12,1)[0];index=mem.i16(record+8)
   q=dict(index=i,record=hex(record),active_byte=active,signed_object_index=index)
   if active and index:
    obj=mem.u32((container_begin+index*4)&0xffffffff);row=all_objects.get(obj)
    q['object']=hex(obj);q['object_metadata']=row
    if row and row['active_state']in (1,3):selected.append(q)
   root_slots.append(q)
  except ValueError as e:root_slots.append(dict(index=i,record=hex(record),error=str(e)))
 targets=sorted({r['object_metadata']['target200']for r in selected});t612=sorted({r['object_metadata']['target612']for r in selected})
 result=dict(tag=tag,path=str(Path(path).relative_to(ROOT)),sha256=sha(path),helper_413a50=helper,
  root_vector_records=root_slots,root_condition_426d10_eligible_records=selected,
  snapshot_target_sets=dict(call4aef47=targets,call4aef92=t612,call4aefaa=t612),
  all_container_objects=list(all_objects.values()),container_object_count=len(all_objects),vtable_counts=dict(byvt),unmapped_object_reads=unmapped,
  snapshot_targets_are_domain_facts_not_actual_call_trace=True,no_universal_type_or_runtime_claim=True)
 mem.file.close();return result
def main():
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 inputs=json.loads((ROOT/'evidence/stage73-run-manifest.json').read_text());rows=[]
 rows.append(inspect('Stage73_exact_phase7_return',ROOT/'runs/stage73-reference-once/memory.bin',inputs['regions'],pe))
 for tag in ['cycle61a','cycle61p07','cycle62p19']:
  d=json.loads((ROOT/('runs/stage12-state-'+tag+'/state.json')).read_text());regions=d['regions'];del d;gc.collect()
  rows.append(inspect(tag,ROOT/('runs/stage12-state-'+tag+'/memory.bin'),regions,pe))
 result=dict(status='SAVED_BYTES_TARGET_AND_PRECONDITION_INSPECTION',snapshots=rows,new_guest_or_Unicorn_executions=0)
 (ROOT/'stage75/evidence/snapshot-targets.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps([dict(tag=r['tag'],helper_count=r['helper_413a50']['count'],eligible_records=len(r['root_condition_426d10_eligible_records']),targets=r['snapshot_target_sets'],objects=r['container_object_count'],vtables=len(r['vtable_counts']))for r in rows]))
if __name__=='__main__':main()
