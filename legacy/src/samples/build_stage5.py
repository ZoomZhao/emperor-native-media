#!/usr/bin/env python3
"""Exact four original functions; excluded notification callees abort, never stub."""
import sys,json,hashlib,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from build_pe import ROOT,BASE,write_pe
from auto_i386_analysis import PE
pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
analysis=json.loads((ROOT/'runs/stage5-analysis/analysis.json').read_text());whole=json.loads((ROOT/'evidence/stage5-unclosed-analysis.json').read_text())
assert analysis['conversion_ready'] and not whole['conversion_ready']
fragments=[(0x5917e0,0x5918f9,'feedback_update'),(0x59a1b0,0x59a1c5,'feedback_scale'),(0x43b860,0x43b887,'integer_rounding'),(0x548340,0x5486d4,'notification_original')]
boundaries=[int(u['address'],16) for u in whole['unknowns'] if u['kind']=='outside_reviewed_code_window']
pages={}
def page(address,code):
    base=address&~4095
    if base not in pages:pages[base]=dict(data=bytearray(4096),code=code)
    assert pages[base]['code']==code
    return pages[base]['data'],address-base
for lo,hi,name in fragments:
    for addr in range(lo,hi):
        data,off=page(addr,True);data[off]=pe.read(addr,1)[0]
# These are SCI runtime boundary addresses, not fabricated game implementations.
# One original byte identifies each entry; no such entry may execute in the test.
for va in boundaries:
    data,off=page(va,True);data[off]=pe.read(va,1)[0]
relocs=[]
for c in whole['address_candidates']:
    if c['confidence']!='high':continue
    target=int(c['target'],16);assert pe.section(target)
    page(target,False);relocs.append(int(c['site'],16)-BASE)
symbols={name:lo for lo,hi,name in fragments}
state={'score':0x130f974,'metric':0x130f988,'restriction':0x1312564,'feedback':0x1311fbc,'positive':0x1311f7c,'negative':0x1311f80,'negative_cooldown':0x1311fc4,'positive_cooldown':0x1311fc8,'gate':0x88ec38}
symbols.update(state)
sections=[];data_sections=[]
for i,(va,item) in enumerate(sorted(pages.items())):
    name=f'.s{i:02d}';flags=0x60000020 if item['code'] else 0xc0000040
    # SR emits an end label at a section's exact end. Adjacent sections
    # therefore duplicate that label. Merge adjacent pages with equal flags;
    # bytes, source VAs, entry points and all original control flow are unchanged.
    if sections and sections[-1]['flags']==flags and BASE+sections[-1]['rva']+len(sections[-1]['data'])==va:
        sections[-1]['data'].extend(item['data'])
    else:sections.append(dict(name=name,rva=va-BASE,data=bytearray(item['data']),flags=flags))
    if not item['code']:
        label=f'data_page_{i}';symbols[label]=va;data_sections.append(dict(va=va,size=4096,symbol=label))
write_pe(ROOT/'samples/stage5-feedback.exe',sections,0x1917e0,sorted(set(relocs)),symbols,max_bytes=65536)
run=ROOT/'runs/stage5-feedback';run.mkdir(parents=True,exist_ok=True)
(run/'global_aliases.sci').write_text(''.join(f'loc_{va:X},{name}\n' for name,va in symbols.items()))
(run/'external_procedures.sci').write_text(''.join(f'loc_{va:X},unresolved_{va:X}\n' for va in boundaries))
(run/'extern.llinc').write_text(''.join(f'proc unresolved_{va:X} external\n' for va in boundaries))
(run/'macros.llinc').write_text('');(run/'SR.cfg').write_text('esp_dword_aligned=yes\nebp_dword_aligned=no\n')
(run/'instruction_flags.sci').write_bytes((ROOT/'runs/stage5-analysis/instruction_flags.sci').read_bytes())
info=dict(fragments=[dict(start=lo,end=hi,symbol=name,sha256=hashlib.sha256(pe.read(lo,hi-lo)).hexdigest()) for lo,hi,name in fragments],original_code_bytes=sum(hi-lo for lo,hi,_ in fragments),guarded_reachable_instruction_bytes=sum(len(bytes.fromhex(i['bytes'])) for i in analysis['instructions']),data_sections=data_sections,state=state,boundaries=boundaries,automatic_relocations=len(set(relocs)),guarded_relocations=len(analysis['address_candidates']),flags=analysis['flags']['annotations'],manual_conditions=[analysis['manual_input_contract']],full_callgraph_closed=False,fixture_bytes=(ROOT/'samples/stage5-feedback.exe').stat().st_size,test_data='self-authored zero-filled mapped pages plus explicit controlled state; not a loaded city save',maximum_fixture_bytes=65536)
(ROOT/'evidence/stage5-slice-layout.json').write_text(json.dumps(info,indent=2))
print(json.dumps(dict(original_code_bytes=info['original_code_bytes'],guarded_code_bytes=info['guarded_reachable_instruction_bytes'],fixture_bytes=info['fixture_bytes'],data_bytes=4096*len(data_sections),auto_relocations=info['automatic_relocations'],guarded_relocations=info['guarded_relocations'],unresolved_boundaries=len(boundaries))))
