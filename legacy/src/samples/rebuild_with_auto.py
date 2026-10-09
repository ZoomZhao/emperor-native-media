#!/usr/bin/env python3
"""Rebuild a reviewed state fixture with ONLY high-evidence auto relocations.

Original fixture .reloc is ignored. Existing alias names and data-layout/model
fixture are retained; this is not automatic recovery of the whole game image.
"""
import sys,json,struct,shutil
from pathlib import Path
from build_pe import ROOT,BASE,write_pe
from auto_i386_analysis import PE
fixture,analysisdir,aliasdir,outname=sys.argv[1:]
analysis=json.loads((ROOT/analysisdir/'analysis.json').read_text())
assert analysis['conversion_ready'],'Unknown code/control-flow prevents automatic conversion'
old=PE(str(ROOT/fixture));sections=[]
for s in old.sections:
    if s['name']=='.reloc':continue
    sections.append(dict(name=s['name'],rva=s['va']-BASE,data=bytearray(old.read(s['va'],s['size'])),flags=s['flags']))
relocs=[]
for a in analysis['address_candidates']:
    if a['initializer_scope'] or a['confidence']!='high':continue
    va=int(a['site'],16)
    if old.section(va):relocs.append(va-BASE)
for b in analysis['runtime_pointer_bindings']:
    if b['confidence']!='high':continue
    va,value=int(b['slot'],16),int(b['target'],16)
    s=next(s for s in sections if s['rva']+BASE<=va<s['rva']+BASE+len(s['data']))
    off=va-BASE-s['rva'];s['data'][off:off+4]=value.to_bytes(4,'little');relocs.append(va-BASE)
symbols={}
for line in (ROOT/aliasdir/'global_aliases.sci').read_text().splitlines():
    address,name=line.split(',',1);symbols[name]=int(address[4:],16)
p=struct.unpack_from('<I',old.data,0x3c)[0];entry=struct.unpack_from('<I',old.data,p+24+16)[0]
write_pe(ROOT/'samples'/f'{outname}.exe',sections,entry,sorted(set(relocs)),symbols)
run=ROOT/'runs'/outname;run.mkdir(parents=True,exist_ok=True)
for name in ('global_aliases.sci','SR.cfg','extern.llinc','macros.llinc'):shutil.copyfile(ROOT/aliasdir/name,run/name)
shutil.copyfile(ROOT/analysisdir/'instruction_flags.sci',run/'instruction_flags.sci')
info=dict(relocations=[hex(BASE+r) for r in sorted(set(relocs))],flags_generated_by='auto_i386_analysis.py',old_relocation_section_ignored=True,pending_candidates_not_rebased=[a for a in analysis['address_candidates'] if a['confidence']=='pending' and not a['initializer_scope']],state_layout_and_aliases_still_reviewed_inputs=True)
(run/'auto-build.json').write_text(json.dumps(info,indent=2));print(json.dumps(info))
