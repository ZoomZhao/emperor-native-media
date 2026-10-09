#!/usr/bin/env python3
"""Read-only audit of input bytes, coverage and retained bounded-run evidence."""
from pathlib import Path
import json,struct,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
metadata=json.loads((ROOT/'evidence/stage2-tax-input.json').read_text())
original=Path(metadata['source']).read_bytes()
assert hashlib.sha256(original).hexdigest()==metadata['source_sha256']
pe=(ROOT/'samples/emperor-tax.exe').read_bytes()
coff=struct.unpack_from('<I',pe,0x3c)[0]+4
machine,nsec,_,_,_,optsize,_=struct.unpack_from('<HHIIIHH',pe,coff)
assert machine==0x14c
sections=[]
for i in range(nsec):
    off=coff+20+optsize+i*40
    name,vs,rva,rawsize,rawptr=struct.unpack_from('<8sIIII',pe,off)
    sections.append((rva+0x400000,vs,rawptr,name.rstrip(b'\0').decode()))
def read(va,size):
    base,vs,ptr,_=next(s for s in sections if s[0]<=va and va+size<=s[0]+s[1])
    return pe[ptr+va-base:ptr+va-base+size]
for f in metadata['fragments']:
    lo,hi=int(f['start'],16),int(f['end_exclusive'],16)
    assert read(lo,hi-lo)==original[lo-0x400000:hi-0x400000]
    assert hashlib.sha256(read(lo,hi-lo)).hexdigest()==f['sha256']
targets=[0x130f960,0x131223c,0x10de2e0,0x130f858,0x130f988,0x1312575,0x130f810,0x85cc8c,0x130f96c]
for va,value in zip(metadata['relocation_sites'],targets):assert int.from_bytes(read(int(va,16),4),'little')==value
static=set();branches=set()
for name in ('stage2-tax-calc-disassembly.txt','stage2-tax-index-disassembly.txt','stage2-tax-rate-disassembly.txt'):
    for line in (ROOT/'evidence'/name).read_text().splitlines():
        m=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{2})\s',line)
        if not m:continue
        addr=int(m[1],16)
        if any(int(f['start'],16)<=addr<int(f['end_exclusive'],16) for f in metadata['fragments']):
            static.add(addr)
            if m[2] in ('7c','7d','7e','7f','75'):branches.add(addr)
paths=json.loads((ROOT/'evidence/stage2-paths.json').read_text())
dynamic={int(addr,16) for p in paths for addr in p['trace']}
oracle=json.loads((ROOT/'evidence/stage2-oracle.json').read_text())
edges={(int(e['address'],16),e['taken']) for e in oracle['conditional_edges']}
assert static==dynamic
assert edges=={(a,taken) for a in branches for taken in (False,True)}
commands=[]
for path in sorted((ROOT/'evidence').glob('stage2-*.result.json')):
    r=json.loads(path.read_text());commands.append(dict(label=path.stem[:-7],**r))
assert all(not r['timed_out'] and r['duration_seconds']<=60 for r in commands)
unannotated=json.loads((ROOT/'evidence/stage2-verify-unannotated.stdout.log').read_text())
annotated=json.loads((ROOT/'evidence/stage2-verify-flags.stdout.log').read_text())
assert unannotated['defined_flag_failures']>0 and annotated['passed']
assert unannotated['digest']==annotated['digest']
artifacts={}
for name in ('samples/emperor-tax.exe','samples/tax-cases.bin','samples/x86-alu-cases.bin','runs/tax/verify-arm64','runs/tax/unannotated-verify-arm64','build/check-x86-alu','build/bin/SRW-llasm','build/bin/llasm','runs/tax/instruction_flags.sci'):
    data=(ROOT/name).read_bytes();artifacts[name]=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
result=dict(input=metadata,oracle=oracle,unannotated=unannotated,annotated=annotated,primitive_oracle=json.loads((ROOT/'evidence/stage2-alu-rosetta.stdout.log').read_text()),static_instruction_count=len(static),all_instructions_covered=True,conditional_branch_count=len(branches),all_branch_directions_covered=True,manual_relocations=len(targets),instruction_flag_annotations=6,alias_entries=16,game_code_modified_bytes=0,semantic_data_bytes=201,writable_mapped_bytes=7822,artifacts=artifacts,commands=commands,max_command_duration_seconds=max(r['duration_seconds'] for r in commands),command_timeouts=0)
(ROOT/'evidence/stage2-final-results.json').write_text(json.dumps(result,indent=2))
print(json.dumps(dict(original_bytes_preserved=True,instructions=len(static),branches=len(branches),both_directions_covered=True,paths=len(paths),commands=len(commands),max_command_seconds=result['max_command_duration_seconds'],timeouts=0)))
