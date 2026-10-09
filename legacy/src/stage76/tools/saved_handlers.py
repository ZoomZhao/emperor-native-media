"""Read saved callback values and allocator metadata, without restoring engines."""
import gc,json,re,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage76';sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
def field(path,key):
 pattern=re.compile(r'(?<!\\)"'+re.escape(key)+r'":\s*');buf='';found=False
 with Path(path).open()as f:
  while True:
   chunk=f.read(1<<20)
   if not chunk:return None
   buf+=chunk
   if not found:
    m=pattern.search(buf)
    if not m:buf=buf[-128:];continue
    buf=buf[m.end():];found=True
   try:return json.JSONDecoder().raw_decode(buf)[0]
   except json.JSONDecodeError:pass
   if len(buf)>32<<20:raise ValueError('metadata extraction budget '+key)
def read(path,regions,address,size=4):
 for r in regions:
  n=r.get('size',r.get('high',0)+1-r['low'])
  if r['low']<=address and address+size<=r['low']+n:
   with Path(path).open('rb')as f:f.seek(r['offset']+address-r['low']);b=f.read(size);assert len(b)==size;return b
 raise ValueError('not mapped '+hex(address))
def main():
 part=json.loads((S/'evidence/call-partition.json').read_text());manifest=json.loads((ROOT/'evidence/stage73-run-manifest.json').read_text());snapshots=[('Stage73_phase7_return',ROOT/'runs/stage73-reference-once/memory.bin',manifest['regions'],None)]
 for tag in ['cycle61a','cycle61p07','cycle62p19']:
  state=ROOT/('runs/stage12-state-'+tag+'/state.json');regions=field(state,'regions')
  assert regions and all('low'in r and 'offset'in r for r in regions)
  snapshots.append((tag,state.parent/'memory.bin',regions,state))
 rows=[];targets=set()
 for tag,path,regions,state in snapshots:
  globals=[]
  for r in part['CRT_global_callbacks']:
   value=struct.unpack('<I',read(path,regions,int(r['slot'],0)))[0];globals.append(dict(PC=r['PC'],slot=r['slot'],saved_target=hex(value),classification=r['classification'],observed_value_not_universal_proof=True))
   if value:targets.add(value)
  heaps=field(state,'heaps')if state else None;alloc=field(state,'allocations')if state else None
  rows.append(dict(tag=tag,globals=globals,heaps=heaps,live_allocation_entries=len(alloc)if alloc else None,heap_next=field(state,'heap_next')if state else None,heap_mapped_end34=field(state,'heap_mapped_end34')if state else None));del alloc;gc.collect()
 out=dict(status='SAVED_METADATA_READ_ONLY',snapshots=rows,observed_nonzero_original_targets=[hex(x)for x in sorted(targets)],no_original_runs=0)
 (S/'evidence/saved-handler-values.json').write_text(json.dumps(out,indent=2)+'\n')
 vt=json.loads((ROOT/'stage75/evidence/vtable-inventory.json').read_text());filetargets={int(r['file_initial_value'],0)for r in part['CRT_global_callbacks']if int(r['file_initial_value'],0)};roots={0x4aeeb0}|targets|filetargets|{int(x,0)for x in vt['candidate_target200']+vt['candidate_target612']}
 cfg=dict(id='phase8-full-dependency-superset',roots=[hex(x)for x in sorted(roots)],boundary_functions={},resolved_indirect_edges={},proved_branch_targets={},runtime_eligible=False,root_virtual_types_pruned=False,note='All structural virtual candidates and observed/default CRT handler targets are analysis roots; remaining arbitrary registered handlers/factory targets stay unknown')
 (S/'configs/phase8-full.json').write_text(json.dumps(cfg,indent=2)+'\n');print(json.dumps(dict(snapshots=rows,roots=len(roots))))
if __name__=='__main__':main()
