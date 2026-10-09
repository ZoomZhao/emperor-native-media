"""Read-only extraction of historical real guest inputs, without engine imports."""
import hashlib,json,struct
from pathlib import Path
from stage69_static import Saved
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence';OUT=ROOT/'runs/stage70-input';OUT.mkdir(exist_ok=True)
def sha(p):
    h=hashlib.sha256()
    with Path(p).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def main():
    s=Saved('cycle61a');rows=[]
    def extract(name,addr,n):
        body=s.read(addr,n);p=OUT/(name+'.bin');p.write_bytes(body)
        owners=[r for r in s.regions if r['low']<=addr and addr+n<=r['high']+1];assert len(owners)==1
        rows.append(dict(name=name,guest_address=hex(addr),bytes=n,path=str(p),sha256=sha(p),original_region=owners[0]))
    spans=[('component_labels',0xec5a10,51984),('subgroup_labels',0xeb8f00,51984),('work_globals_and_ring',0xc81b70,4020),
        ('top10_pairs',0x1312588,80),('cursor',0x101c560,4),('tile_flags',0xf6a9e0,51984*4),('tile_object_indices',0xfc3750,51984*2),
        ('neighbor_flags',0x13789c0,51984*2),('neighbor_deltas',0x820cd0,16),('grid_dimensions',0x101d0c0,16),('container',0x8c7634,16)]
    for name,a,n in spans:extract(name,a,n)
    base=s.word(0x8c7638);end=s.word(0x8c763c);assert end>=base and (end-base)%4==0
    extract('pointer_vector_full',base,end-base)
    indices=sorted(set(struct.unpack('<51984h',s.read(0xfc3750,51984*2))));objects={};pointers=[]
    for index in indices:
        slot=(base+index*4)&0xffffffff;obj=s.word(slot);owner=s.owner(obj+0x14,2);assert len(owner)==1
        item=owner[0];a=int(item['base'],16);n=item['bytes'];objects[a]=n
        pointers.append(dict(index=index,slot=hex(slot),object=hex(obj),allocated_object_base=hex(a),allocated_object_bytes=n))
    for i,(a,n) in enumerate(sorted(objects.items())):extract('allocated_object_%02d'%i,a,n)
    stack=[r for r in s.regions if r['low']<=0x700ffd68<=r['high']];assert len(stack)==1
    r=stack[0];extract('main_stack_region',r['low'],r['high']+1-r['low'])
    cpu=OUT/'main_tid1_context_OPAQUE.bin';cpu.write_bytes(s.threads[1]['context'][0]);assert cpu.stat().st_size==5608
    memory=ROOT/'runs/stage12-state-cycle61a/memory.bin';state=ROOT/'runs/stage12-state-cycle61a/state.json'
    memory_sha=sha(memory);assert memory_sha=='d4fc811b000eb8923a8d085602e363d5b0c6b2a87e272688f25365d18f088214'
    assert s.state_sha=='0f916a79bc15697d88f0b986edaa6dd3ef454c3962fe5466b88eba7fede5e1ae'
    out=dict(status='REAL_INPUT_BYTES_EXTRACTED_CPU_ADAPTER_PENDING',source_checkpoint='cycle61a',
        immutable_memory=dict(path=str(memory),bytes=memory.stat().st_size,sha256=memory_sha),
        immutable_state=dict(path=str(state),sha256=s.state_sha),all_original_guest_regions=s.regions,
        mapping='Preserve original uint32 guest virtual addresses, bytes and permissions. Full memory backing is immutable; extracted spans are review aids, not fabricated replacement state.',
        current_tid=s.current,main_tid=1,current_worker_registers_not_used=True,
        main_CPU=dict(path=str(cpu),bytes=5608,sha256=sha(cpu),layout_opaque=True,registers_or_segment_bases_not_guessed=True),
        extracted_spans=rows,pointer_indices=len(indices),unique_allocated_objects=len(objects),pointer_chain=pointers,
        native_entry=dict(PC='0x4af311',SP='0x700ffd68',provenance='Stage69 main-thread trace and actual saved stack; CPU-context API verification still required'),
        native_stop=dict(PC='0x4ac333',SP='0x700ffd8c',before_instruction=True,phase_remains=7),
        isolated_return='Restore tid1 CPU and all guest memory; do not initialize registers, replace return PCs, run other threads, or advance scheduler. Original x87/segment state must be carried unchanged.',
        later_snapshot_exact_oracle=False,later_snapshot='cycle61p07 includes phase8 and refresh; secondary consistency check only',
        original_EXE_Unicorn_or_native_city_executed=False)
    (E/'stage70-real-input-contract.json').write_text(json.dumps(out,indent=2)+'\n');s.memory.close()
    print(json.dumps(dict(status=out['status'],spans=len(rows),bytes=sum(r['bytes']for r in rows),indices=len(indices),objects=len(objects))))
if __name__=='__main__':main()
