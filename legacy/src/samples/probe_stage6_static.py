#!/usr/bin/env python3
"""Finite read-only original-byte xref/table probes; raw hits are candidates."""
import sys,json,struct,hashlib,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
textsec=pe.section(0x401000);b=pe.read(textsec['va'],textsec['rawsize']);base=textsec['va']
targets=[0x42e430,0x430ef0,0x4ad4a0,0x5177c0,0x5177d0,0x517810,0x516e00,0x431850,0x4314a0,0x436560,0x517de0,0x4ada10,0x4adc90]
xrefs={hex(t):[] for t in targets}
for i,op in enumerate(b[:-4]):
    if op in (0xe8,0xe9):
        target=base+i+5+struct.unpack_from('<i',b,i+1)[0]
        if target in targets:xrefs[hex(target)].append(dict(site=hex(base+i),opcode='call' if op==0xe8 else 'jump',classification='raw rel32 candidate; independent instruction-boundary verification required'))
global_targets=[0x88ec38,0x10c6f60,0x10bfef0,0x10bfed8,0x10de045,0x10de12c,0x10c6f64]
globals={}
for t in global_targets:
    pattern=struct.pack('<I',t);hits=[];start=0
    while True:
        i=b.find(pattern,start)
        if i<0:break
        hits.append(dict(site=hex(base+i),before=b[max(0,i-8):i].hex(),after=b[i+4:i+12].hex()));start=i+1
    globals[hex(t)]=dict(count=len(hits),hits=hits,classification='raw operand candidates; not complete semantic read/write set')
tables={hex(va):[hex(v) for v in struct.unpack('<'+str(n)+'I',pe.read(va,n*4))] for va,n in [(0x42e59c,7)]}
parsed={}
for label in ('gate-setter','gate-reset','feedback-dispatcher','lifecycle-create','lifecycle-remove','collection-origin'):
    text=(ROOT/f'evidence/stage6-{label}.stdout.log').read_text();items=[]
    for line in text.splitlines():
        m=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s*([a-z][a-z0-9]*)\s*(.*)',line)
        if not m:continue
        pc=int(m[1],16);mn=m[3];operand=m[4].split(' <')[0]
        if mn.startswith('call') or mn.startswith('jmp') and '*' in operand or '%fs:' in operand or mn.startswith(('fild','fmul','fist','fld','fst','fadd','fsub','fdiv')):
            items.append(dict(pc=hex(pc),mnemonic=mn,operand=operand))
    parsed[label]=items
result=dict(source_hash=hashlib.sha256(pe.data).hexdigest(),direct_xref_candidates=xrefs,global_operand_candidates=globals,read_only_original_tables=tables,independently_disassembled_edges=parsed)
(ROOT/'evidence/stage6-static-probes.json').write_text(json.dumps(result,indent=2))
print(json.dumps(dict(direct_xref_candidates=xrefs,global_candidate_counts={k:v['count'] for k,v in globals.items()},tables=tables,edges=parsed)))
