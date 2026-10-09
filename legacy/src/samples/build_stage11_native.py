#!/usr/bin/env python3
"""Exact original CRT heap and GUID constructor; other branches abort.

Known IAT bindings, original BSS and decoded address fields only. No original
instruction is rewritten and no game state is seeded to force a branch.
"""
import hashlib,json,re,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
from build_pe import write_pe,BASE
pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
fragments=[(0x426e60,0x426ea0,'guid_consumer'),(0x7656b0,0x7656df,'crt_stack_probe'),(0x76d9e8,0x76db30,'crt_heap_choice'),(0x76db30,0x76db8d,'crt_heap_init')]
logs=['stage11-guid-consumer-disasm.stdout.log','stage11-stack-probe-disasm.stdout.log','stage11-crt-heap-disasm.stdout.log']
instructions={}
for filename in logs:
 for line in (ROOT/'evidence'/filename).read_text().splitlines():
  m=re.match(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s*(\S+)(.*)$',line)
  if not m:continue
  va=int(m[1],16)
  if not any(lo<=va<hi for lo,hi,_ in fragments):continue
  body=bytes.fromhex(m[2]);assert pe.read(va,len(body))==body
  instructions[va]=dict(bytes=body,mnemonic=m[3],operand=m[4])
assert sum(len(x['bytes']) for x in instructions.values())==sum(hi-lo for lo,hi,_ in fragments)
external=set()
for ins in instructions.values():
 m=re.match(r'\s+0x([0-9a-f]+)',ins['operand']) if ins['mnemonic']=='calll' else None
 if m:
  target=int(m[1],16)
  if not any(lo<=target<hi for lo,hi,_ in fragments):external.add(target)
pages={}
def page(va,flags):
 key=va&~4095
 if key not in pages:pages[key]=dict(data=bytearray(4096),flags=flags)
 assert pages[key]['flags']==flags
 return pages[key]['data'],va-key
for lo,hi,_ in fragments:
 for va in range(lo,hi):p,o=page(va,0x60000020);p[o]=pe.read(va,1)[0]
for va in external:p,o=page(va,0x60000020);p[o]=pe.read(va,1)[0]
for va in [0x7ce000,0x817000]:
 p,o=page(va,0x40000040);p[:]=pe.read(va,4096)
page(0x1e58000,0xc0000040)
iat,_=page(0x7a9000,0xc0000040)
groups=[('KERNEL32.dll',[(0x7a91f8,'HeapCreate'),(0x7a91fc,'HeapDestroy')]),('KERNEL32.dll',[(0x7a92d8,'GetVersionExA')]),('KERNEL32.dll',[(0x7a9200,'GetEnvironmentVariableA')]),('KERNEL32.dll',[(0x7a9188,'GetModuleFileNameA')]),('KERNEL32.dll',[(0x7a9180,'OutputDebugStringA')]),('ole32.dll',[(0x7a96f8,'CoCreateGuid')])]
import_rva=0x440000;import_data=bytearray(4096);pos=(len(groups)+1)*20
for i,(dll,entries) in enumerate(groups):
 ilt=pos;pos+=4*(len(entries)+1);nm=pos;encoded=dll.encode()+b'\0';import_data[pos:pos+len(encoded)]=encoded;pos=(pos+len(encoded)+1)&~1
 struct.pack_into('<5I',import_data,i*20,import_rva+ilt,0,0,import_rva+nm,entries[0][0]-BASE)
 for j,(slot,symbol) in enumerate(entries):
  assert slot==entries[0][0]+4*j
  record=struct.pack('<H',0)+symbol.encode()+b'\0';import_data[pos:pos+len(record)]=record
  struct.pack_into('<I',import_data,ilt+4*j,import_rva+pos);struct.pack_into('<I',iat,slot-0x7a9000,import_rva+pos);pos=(pos+len(record)+1)&~1
addresses={0x1e58d20,0x1e58d24,0x7ce188,0x7ce170,0x817768}|{slot for _,entries in groups for slot,_ in entries}
relocations=[]
for va,ins in instructions.items():
 body=ins['bytes']
 for offset in range(len(body)-3):
  if int.from_bytes(body[offset:offset+4],'little') in addresses:relocations.append(va+offset-BASE)
sections=[]
for i,(va,item) in enumerate(sorted(pages.items())):
 if sections and sections[-1]['flags']==item['flags'] and BASE+sections[-1]['rva']+len(sections[-1]['data'])==va:sections[-1]['data'].extend(item['data'])
 else:sections.append(dict(name=f'.s{i:02d}',rva=va-BASE,data=item['data'],flags=item['flags']))
sections.append(dict(name='.import',rva=import_rva,data=import_data,flags=0xc0000040))
symbols={name:lo for lo,hi,name in fragments};symbols.update(crt_heap_handle=0x1e58d20,crt_heap_mode=0x1e58d24,crt_globals_page=0x1e58000,debug_string_page=0x817000)
exe=ROOT/'samples/stage11-native-slice.exe';write_pe(exe,sections,0x26e60,relocations,symbols,max_bytes=262144)
body=bytearray(exe.read_bytes());opt=152;struct.pack_into('<II',body,opt+96+8,import_rva,(len(groups)+1)*20);exe.write_bytes(body)
run=ROOT/'runs/stage11-native';run.mkdir(parents=True,exist_ok=True)
(run/'global_aliases.sci').write_text(''.join(f'loc_{va:X},{name}\n' for name,va in symbols.items()))
(run/'external_procedures.sci').write_text(''.join(f'loc_{va:X},unresolved_{va:X}\n' for va in sorted(external)))
(run/'extern.llinc').write_text(''.join(f'proc unresolved_{va:X} external\n' for va in sorted(external)))
(run/'macros.llinc').write_text('');(run/'SR.cfg').write_text('esp_dword_aligned=yes\nebp_dword_aligned=no\n')
flags=[]
for va,ins in sorted(instructions.items()):
 op=ins['mnemonic'].rstrip('l')
 mask=0x3b if op in ['test','xor','and','or'] else 0x3e if op in ['inc','dec'] else 0x3f if op in ['cmp','sub','add','sbb'] else 0
 if mask:flags.append(f'loc_{va:X},0x{mask:x},0\n')
(run/'instruction_flags.sci').write_text('')
info=dict(fragments=[dict(start=hex(lo),end=hex(hi),name=name,sha256=hashlib.sha256(pe.read(lo,hi-lo)).hexdigest()) for lo,hi,name in fragments],original_instruction_bytes=sum(hi-lo for lo,hi,_ in fragments),instruction_count=len(instructions),fixture_sha256=hashlib.sha256(body).hexdigest(),external_abort_addresses=[hex(v) for v in sorted(external)],relocation_sites=[hex(BASE+r) for r in relocations],relocation_evidence='Decoded absolute IAT/global-memory operands; known old-path/debug string pointer arguments. No general image-integer scan.',original_code_patched=False,initial_BSS_zero=True,artificial_game_gates=False,scope='Finite measured NT6.1 CRT path and exact GUID consumer. Old OS helpers abort rather than fake implementations.',strict_EFLAGS_comparison=False,flag_note='No blanket force-flags annotations. Compare functional branches, output bytes and caller ABI; strict flag dataflow remains outside this test.')
(ROOT/'evidence/stage11-native-slice-layout.json').write_text(json.dumps(info,indent=2)+'\n');print(json.dumps(info))
