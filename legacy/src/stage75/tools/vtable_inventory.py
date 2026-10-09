"""Original instruction-byte constructor references and finite virtual slots.

Candidate table length uses a contiguous run of original code pointers. This is
structural evidence, not a recovered RTTI/inheritance or completeness theorem.
"""
import collections,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
PAT=re.compile(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*([a-z][a-z0-9]*)\s*(.*)$')
def main():
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 snapshots=json.loads((ROOT/'stage75/evidence/snapshot-targets.json').read_text());known={int(o['vtable'],16)for s in snapshots['snapshots']for o in s['all_container_objects']}
 def codeptr(p):
  s=pe.section(p);return bool(s and s['flags']&0x20000000)
 def u32(p):return struct.unpack('<I',pe.read(p,4))[0]
 tables={};references=collections.defaultdict(list)
 for line in (ROOT/'runs/stage69-static/fixed-text.disasm').open():
  m=PAT.match(line)
  if not m or m[3]!='mov':continue
  operand=m[4].split('#')[0].strip()
  q=re.fullmatch(r'dword ptr \[(eax|ecx|edx|ebx|esi|edi|ebp)\], (\d+)',operand)
  if not q:continue
  vt=int(q[2]);section=pe.section(vt)
  if not section or section['flags']&0x20000000:continue
  try:
   if not codeptr(u32(vt)):continue
   pc=int(m[1],16);b=bytes.fromhex(m[2]);assert pe.read(pc,len(b))==b
   references[vt].append(dict(PC=hex(pc),bytes=b.hex(),operand=operand))
  except ValueError:continue
 for vt in sorted(set(references)|known):
  pointers=[]
  for n in range(1024):
   try:target=u32(vt+n*4)
   except ValueError:break
   if not codeptr(target):break
   pointers.append(target)
  if len(pointers)<154:continue
  tables[vt]=dict(vtable=hex(vt),contiguous_original_code_pointer_slots=len(pointers),constructor_or_destructor_vptr_store_references=references[vt],
   saved_container_observed=vt in known,target200=hex(pointers[50]),target612=hex(pointers[153]),first16=[hex(p)for p in pointers[:16]],
   full_slot_pointers=[hex(p)for p in pointers])
 result=dict(status='STRUCTURAL_VTABLE_INVENTORY_NOT_UNIVERSAL_TYPE_PROOF',tables=list(tables.values()),
  snapshot_vtable_count=len(known),structural_candidate_count=len(tables),
  candidate_target200=sorted({t['target200']for t in tables.values()}),candidate_target612=sorted({t['target612']for t in tables.values()}),
  unknown='No inference that every long table is a valid phase8 receiver; no proof that constructor reachability covers all indirect factory paths',new_guest_or_Unicorn_executions=0)
 (ROOT/'stage75/evidence/vtable-inventory.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(dict(candidates=len(tables),observed_vtables=len(known),targets200=len(result['candidate_target200']),targets612=len(result['candidate_target612']),saved_tables=[{k:t[k]for k in ['vtable','target200','target612','constructor_or_destructor_vptr_store_references']}for t in tables.values()if t['saved_container_observed']])) )
if __name__=='__main__':main()
