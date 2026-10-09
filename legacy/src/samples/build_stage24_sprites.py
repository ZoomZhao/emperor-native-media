#!/usr/bin/env python3
"""Finite exact original sprite + DirectDraw refresh slice, no startup bypass."""
import hashlib,json,re,struct
from pathlib import Path
from auto_i386_analysis import PE
from build_pe import write_pe,BASE
R=Path(__file__).resolve().parents[1];run=R/'runs/stage24-sprites';run.mkdir(exist_ok=True)
pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
fragments=[(0x408150,0x408170,'sprite_record'),(0x418ff0,0x419070,'convert555'),(0x419070,0x4190f0,'convert565'),
 (0x41bd10,0x41c3c0,'rle_top'),(0x41cc50,0x41cca0,'rle_skip'),(0x44dbd0,0x44dd40,'create_original_surface'),
 (0x44fec0,0x450150,'diamond'),(0x4527d0,0x452a80,'diamond_left'),(0x453cc0,0x453f70,'diamond_right'),
 (0x471bf0,0x471c00,'sprite_count'),(0x5279d0,0x5279f0,'original_copy'),(0x5297f0,0x529950,'clip'),
 (0x52bc10,0x52bc30,'clock_delta'),(0x52be40,0x52c080,'original_refresh'),(0x5cd8a0,0x5ce800,'draw_base'),(0x5ce800,0x5ceae0,'draw_top')]
items=[]
for line in (R/'evidence/stage16-original-text.disassembly.log').open():
 m=re.match(r'\s*([0-9a-f]+):\s+(\S+)\s*(.*)',line)
 if m:items.append((int(m[1],16),m[2],m[3].split('#')[0].strip()))
instructions={}
for i,(va,op,args)in enumerate(items[:-1]):
 if any(lo<=va<hi for lo,hi,_ in fragments):
  n=items[i+1][0]-va;assert 0<n<=15,(hex(va),n)
  instructions[va]=(op,args,pe.read(va,n))
assert sum(len(t[2])for t in instructions.values())==sum(hi-lo for lo,hi,_ in fragments)
external=set();addresses=set();pointer_immediates={17684824,17684712,17687648,8541024,8540980,8540936,8540908}
for va,(op,args,body)in instructions.items():
 if op=='calll':
  m=re.match(r'0x([0-9a-f]+)',args)
  if m:
   t=int(m[1],16)
   if not any(lo<=t<hi for lo,hi,_ in fragments):external.add(t)
 for m in re.finditer(r'(?<![\w$%])([0-9]{6,})(?=,|$)',args):
  t=int(m[1]);assert t>=BASE,(hex(va),t);addresses.add(t)
 for t in pointer_immediates:
  if '$'+str(t)in args:addresses.add(t)
pages={}
def page(va,flags):
 key=va&~4095
 if key not in pages:pages[key]=dict(data=bytearray(b'\xcc'*4096 if flags&0x20 else bytes(4096)),flags=flags)
 assert pages[key]['flags']==flags
 return pages[key]['data'],va-key
for lo,hi,_ in fragments:
 for va in range(lo,hi):p,o=page(va,0x60000020);p[o]=pe.read(va,1)[0]
for va in external:p,o=page(va,0x60000020);p[o]=pe.read(va,1)[0]
for va in addresses:
 p,o=page(va,0xc0000040)
 if va<0xa00000:p[:]=pe.read(va&~4095,4096)
relocations=[]
for va,(op,args,body)in instructions.items():
 for j in range(len(body)-3):
  if int.from_bytes(body[j:j+4],'little')in addresses:relocations.append(va+j-BASE)
symbols={name:lo for lo,hi,name in fragments}
for va in addresses:symbols['g_'+format(va,'x')]=va
sections=[]
for i,(va,item)in enumerate(sorted(pages.items())):
 if sections and sections[-1]['flags']==item['flags']and BASE+sections[-1]['rva']+len(sections[-1]['data'])==va:sections[-1]['data'].extend(item['data'])
 else:sections.append(dict(name='.s%02d'%i,rva=va-BASE,data=item['data'],flags=item['flags']))
write_pe(R/'samples/stage24-sprites-slice.exe',sections,0x1ce800,relocations,symbols,max_bytes=300000)
(run/'global_aliases.sci').write_text(''.join('loc_%X,%s\n'%(v,k)for k,v in symbols.items()))
bindings={0x765058:'dd_sprintf',0x40fc20:'dd_log'}
(run/'external_procedures.sci').write_text(''.join('loc_%X,%s\n'%(v,bindings.get(v,'unresolved_%X'%v))for v in sorted(external)))
(run/'extern.llinc').write_text(''.join('proc unresolved_%X external\n'%v for v in sorted(external)if v not in bindings))
for nm,text in [('SR.cfg','esp_dword_aligned=yes\nebp_dword_aligned=no\n'),('macros.llinc',''),('instruction_flags.sci','')]: (run/nm).write_text(text)
info=dict(original_EXE_sha256=hashlib.sha256(pe.data).hexdigest(),original_instruction_bytes_unchanged=True,
 fragments=[dict(start=hex(lo),end=hex(hi),name=n,bytes=hi-lo,sha256=hashlib.sha256(pe.read(lo,hi-lo)).hexdigest())for lo,hi,n in fragments],
 original_instruction_bytes=sum(hi-lo for lo,hi,_ in fragments),instructions=len(instructions),absolute_operand_addresses=[hex(x)for x in sorted(addresses)],
 relocation_sites=[hex(BASE+x)for x in relocations],external_abort_addresses=[hex(x)for x in sorted(external)if x not in bindings],
 native_platform_bindings={hex(v):n for v,n in bindings.items()},scope='Isolated original graphics entries, not full ordered WinMain or city state.')
(R/'evidence/stage24-extraction.json').write_text(json.dumps(info,indent=2));print(json.dumps(dict(bytes=info['original_instruction_bytes'],instructions=len(instructions),globals=len(addresses),external=len(external))))
