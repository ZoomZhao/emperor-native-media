#!/usr/bin/env python3
"""Isolate three original tax routines without altering any instruction byte."""
from pathlib import Path
import hashlib,json,re,struct
from build_pe import ROOT,BASE,write_pe

original=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
model=Path('<LOCAL_ORIGINAL_PROJECT>/GameData/Model/EmperorTaxSentimentModel.txt')
b=original.read_bytes()
assert hashlib.sha256(b).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
raw=model.read_bytes()
rows=[]
for line in raw.decode('ascii').splitlines():
    if '{' in line:
        row=[int(x) for x in re.findall(r'-?\d+',line.split('{')[1])]
        assert len(row)==6
        rows.append(row)
assert len(rows)==7
fragments=[(0x591180,0x5911c4,'tax_contribution'),(0x592be0,0x592c2a,'nearest_rate'),(0x592cf0,0x592cfa,'tax_index')]
sections=[]
for name,va,end,executable in [('.taxcode',0x591000,0x5911c4,True),('.idxcode',0x592000,0x592cfa,True),('.rates',0x85c000,0x85cca8,False),('.diff',0x10de000,0x10de2e4,False),('.taxdata',0x130f000,0x130f98c,False),('.city',0x1312000,0x1312576,False)]:
    data=bytearray(end-va)
    for start,stop,_ in fragments:
        if va<=start<end: data[start-va:stop-va]=b[start-BASE:stop-BASE]
    sections.append(dict(name=name,rva=va-BASE,data=data,flags=0x60000020 if executable else 0xc0000040))
def put(va,data):
    s=next(s for s in sections if s['rva']+BASE<=va<s['rva']+BASE+len(s['data']))
    off=va-BASE-s['rva'];s['data'][off:off+len(data)]=data
put(0x85cc8c,struct.pack('<7i',*[r[0] for r in rows]))
put(0x130f858,struct.pack('<35i',*[v for r in rows for v in r[1:]]))
put(0x130f810,struct.pack('<4I',0x85cc8c,7,0x11e,0x130f96c))
symbols={name:lo for lo,_,name in fragments}
symbols.update(tax_rates=0x85cc8c,tax_difficulty=0x10de2e0,tax_descriptor=0x130f810,tax_matrix=0x130f858,tax_object=0x130f960,tax_current=0x130f96c,tax_metric=0x130f988,tax_coverage=0x131223c,tax_guard=0x1312575)
symbols.update({s['name'][1:]+'_segment':BASE+s['rva'] for s in sections if not s['flags']&0x20})
reloc_va=[0x591181,0x59118c,0x591197,0x5911a3,0x5911ad,0x5911b9,0x592cf1,0x130f810,0x130f81c]
write_pe(ROOT/'samples/emperor-tax.exe',sections,0x191180,[r-BASE for r in reloc_va],symbols)
run=ROOT/'runs/tax';run.mkdir(parents=True,exist_ok=True)
(run/'global_aliases.sci').write_text(''.join(f'loc_{va:X},{name}\n' for name,va in symbols.items()))
(run/'SR.cfg').write_text('esp_dword_aligned=yes\nebp_dword_aligned=no\n')
for f in ('extern.llinc','macros.llinc'): (run/f).write_text('')
metadata=dict(source=str(original),source_sha256=hashlib.sha256(b).hexdigest(),model=str(model),model_sha256=hashlib.sha256(raw).hexdigest(),rows=rows,fragments=[dict(start=hex(lo),end_exclusive=hex(hi),bytes=hi-lo,name=name,sha256=hashlib.sha256(b[lo-BASE:hi-BASE]).hexdigest()) for lo,hi,name in fragments],relocation_sites=[hex(x) for x in reloc_va],segments=[dict(name=s['name'],va=BASE+s['rva'],size=len(s['data'])) for s in sections],original_instruction_bytes_unchanged=True)
(ROOT/'evidence/stage2-tax-input.json').write_text(json.dumps(metadata,indent=2))
(ROOT/'samples/tax-fragments.json').write_text(json.dumps({str(lo):b[lo-BASE:hi-BASE].hex() for lo,hi,_ in fragments},indent=2))
(ROOT/'evidence/stage2-model.txt').write_bytes(raw)
print(json.dumps(dict(instruction_bytes=sum(hi-lo for lo,hi,_ in fragments),pe_bytes=(ROOT/'samples/emperor-tax.exe').stat().st_size,manual_relocations=len(reloc_va),rows=rows)))
