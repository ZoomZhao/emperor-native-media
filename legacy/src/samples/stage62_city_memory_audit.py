"""Readonly city globals and tracked guest blocks; no assumed entity schema."""
import hashlib,json,struct
from pathlib import Path
from stage60_controlled_input_plan import decode
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def sha(b):return hashlib.sha256(b).hexdigest()
class Snapshot:
    def __init__(self,tag):
        self.tag=tag;self.folder=ROOT/f'runs/stage12-state-{tag}'
        q=json.loads((self.folder/'state.json').read_text());self.regions=q['regions'];h=dict(q['host']['dict'])
        self.allocations=decode(h['allocations']);self.dd=decode(h['dd_objects'])
        self.regs=q['registers'];self.source_SHA=q['original_EXE_sha256'];self.stream=(self.folder/'memory.bin').open('rb')
    def read(self,addr,n):
        for r in self.regions:
            if r['low']<=addr and addr+n<=r['high']+1:
                self.stream.seek(r['offset']+addr-r['low']);b=self.stream.read(n);assert len(b)==n;return b
        raise ValueError(hex(addr))
def main():
    before=Snapshot('cycle61p15');after=Snapshot('cycle62p19')
    assert before.source_SHA==after.source_SHA=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
    pe=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe').read_bytes()
    e=struct.unpack_from('<I',pe,60)[0];n=struct.unpack_from('<H',pe,e+6)[0];osize=struct.unpack_from('<H',pe,e+20)[0]
    base=struct.unpack_from('<I',pe,e+52)[0];sections={}
    for i in range(n):
        o=e+24+osize+i*40;name=pe[o:o+8].rstrip(b'\0').decode();vs,va,rs,ro=struct.unpack_from('<4I',pe,o+8)
        sections[name]=dict(addr=base+va,virtual=vs,raw=rs,offset=ro)
    text=sections['.text'];expected=pe[text['offset']:text['offset']+text['raw']]
    assert before.read(text['addr'],len(expected))==after.read(text['addr'],len(expected))==expected
    data=sections['.data'];b=before.read(data['addr'],data['virtual']);a=after.read(data['addr'],data['virtual'])
    pages=[];changed_bytes=0
    for start in range(0,len(b),4096):
        old=b[start:start+4096];new=a[start:start+4096]
        if old==new:continue
        offsets=[j for j,(x,y) in enumerate(zip(old,new)) if x!=y];changed_bytes+=len(offsets)
        samples=[dict(address=hex(data['addr']+start+j),old=old[j],new=new[j]) for j in offsets[:8]]
        pages.append(dict(start=hex(data['addr']+start),changed_bytes=len(offsets),old_sha256=sha(old),new_sha256=sha(new),samples=samples))
    fields=[]
    for name,addr in [('phase',0xc82ef8),('cycle_counter_c82ef4',0xc82ef4),('cycle_counter_c82ef0',0xc82ef0),('population',0x130f988),('treasury',0x130f970)]:
        fields.append(dict(name=name,address=hex(addr),before=struct.unpack('<I',before.read(addr,4))[0],after=struct.unpack('<I',after.read(addr,4))[0]))
    common=[p for p in before.allocations if p in after.allocations and before.allocations[p]==after.allocations[p]]
    changed_small=[];checked=0
    for p in common:
        size=before.allocations[p][1]
        if not 0<size<=4096:continue
        old=before.read(p,size);new=after.read(p,size);checked+=1
        if old!=new:
            offsets=[i for i,(x,y) in enumerate(zip(old,new)) if x!=y]
            changed_small.append(dict(address=hex(p),bytes=size,changed_bytes=len(offsets),
                old_sha256=sha(old),new_sha256=sha(new),changed_offsets=offsets[:32]))
    out=dict(readonly=True,original_instructions_executed=0,before=before.tag,after=after.tag,
        original_text_equals_fixed_EXE=True,original_data_section_start=hex(data['addr']),original_data_section_bytes=len(b),
        original_data_changed_bytes=changed_bytes,original_data_changed_pages=len(pages),changed_pages=pages,
        known_city_fields=fields,
        guest_live_heap_blocks_before=len(before.allocations),guest_live_heap_blocks_after=len(after.allocations),
        guest_live_heap_bytes_before=sum(x[1] for x in before.allocations.values()),guest_live_heap_bytes_after=sum(x[1] for x in after.allocations.values()),
        owned_DirectDraw_COM_objects_before=len(before.dd),owned_DirectDraw_COM_objects_after=len(after.dd),
        guest_allocation_metadata_unchanged=before.allocations==after.allocations,owned_DirectDraw_metadata_unchanged=before.dd==after.dd,
        common_small_guest_blocks_checked=checked,changed_small_guest_blocks=len(changed_small),changed_small_blocks=changed_small,
        semantic_city_entity_schema_fully_decoded=False,
        note='Actual saved guest bytes and tracked allocations; raw block changes are not interpreted as a complete city entity graph or exact actor counts.')
    (E/'stage62-city-memory-audit.json').write_text(json.dumps(out,indent=2)+'\n')
    before.stream.close();after.stream.close()
    print(json.dumps({k:v for k,v in out.items() if k not in ['changed_pages','changed_small_blocks']}))
if __name__=='__main__':main()
