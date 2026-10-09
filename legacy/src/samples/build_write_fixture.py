#!/usr/bin/env python3
"""One state-writing holdout; fixture has no manually authored relocations."""
import hashlib,json
from build_pe import ROOT,BASE,write_pe
from auto_i386_analysis import PE
source=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
assert hashlib.sha256(source.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
fragments=[(0x592d10,0x592d1a,'tax_raise'),(0x592c70,0x592c8c,'raise_selector'),(0x592be0,0x592c2a,'nearest_rate')]
old=PE(str(ROOT/'samples/emperor-tax.exe'));sections=[]
for s in old.sections:
    if s['name'] in ('.reloc','.taxcode'):continue
    if s['name']=='.idxcode':
        data=bytearray(0xd1a)
        for lo,hi,_ in fragments:data[lo-s['va']:hi-s['va']]=source.read(lo,hi-lo)
    else:data=bytearray(old.read(s['va'],s['size']))
    sections.append(dict(name=s['name'],rva=s['va']-BASE,data=data,flags=s['flags']))
symbols={}
for line in (ROOT/'runs/tax/global_aliases.sci').read_text().splitlines():
    address,name=line.split(',');va=int(address[4:],16)
    if not source.section(va)['flags']&0x20000000:symbols[name]=va
symbols.update({name:lo for lo,_,name in fragments})
write_pe(ROOT/'samples/emperor-write-fixture.exe',sections,0x192d10,[],symbols)
run=ROOT/'runs/write-fixture';run.mkdir(parents=True,exist_ok=True)
(run/'global_aliases.sci').write_text(''.join(f'loc_{va:X},{name}\n' for name,va in symbols.items()))
for name,text in [('SR.cfg','esp_dword_aligned=yes\nebp_dword_aligned=no\n'),('extern.llinc',''),('macros.llinc','')]: (run/name).write_text(text)
(ROOT/'samples/write-fragments.json').write_text(json.dumps({str(lo):source.read(lo,hi-lo).hex() for lo,hi,_ in fragments},indent=2))
print(json.dumps(dict(original_bytes=sum(hi-lo for lo,hi,_ in fragments),manual_relocations=0,fragments=[dict(start=hex(lo),end_exclusive=hex(hi),bytes=hi-lo,name=name) for lo,hi,name in fragments],semantic_evidence='tax descriptor used by validated tax sentiment module; writer selects next rate and stores through the descriptor current-value pointer')))
