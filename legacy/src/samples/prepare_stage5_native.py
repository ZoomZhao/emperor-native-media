#!/usr/bin/env python3
"""Add test-only entry counters and exact original/native continuation mapping."""
import json,re,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];run=ROOT/'runs/stage5-feedback'
layout=json.loads((ROOT/'evidence/stage5-slice-layout.json').read_text())
src=(run/'auto.ll').read_text();out=src
for i,f in enumerate(layout['fragments']):
    pattern=r'(define protected fastcc void @'+f['symbol']+r'\(%_cpu\* %cpu\) nounwind \{\n)'
    out,n=re.subn(pattern,r'\1call ccc void @stage5_observe(i32 '+str(i)+')\n',out);assert n==1
out+='\ndeclare ccc void @stage5_observe(i32) nounwind\ndeclare ccc void @stage5_forbidden(i32) nounwind\n'
for va in layout['boundaries']:
    name=f'unresolved_{va:X}'
    pattern=r'^declare[^\n]*@'+name+r'\([^\n]*\n'
    out,n=re.subn(pattern,'',out,flags=re.M);assert n==1
    out+=f'define hidden fastcc void @{name}(%_cpu* %cpu) nounwind {{\ncall ccc void @stage5_forbidden(i32 {va})\nunreachable\n}}\n'
aliases={f['symbol']:f['start'] for f in layout['fragments']};mapped=[]
for name in re.findall(r'^define (?:protected|private) fastcc void @([^ (]+)\(%_cpu\*',src,re.M):
    va=int(name[4:],16) if name.startswith('loc_') else 0xfffffffe if name=='__return_procedure' else aliases.get(name)
    assert va is not None,name;mapped.append((name,va))
count=len(mapped)
out+=f'@stage5_code_count = protected constant i32 {count}\n'
out+=f'@stage5_code_sources = protected constant [{count} x i32] ['+', '.join(f'i32 {va}' for name,va in mapped)+']\n'
out+=f'@stage5_code_pointers = protected constant [{count} x i8*] ['+', '.join(f'i8* bitcast (void (%_cpu*)* @{name} to i8*)' for name,va in mapped)+']\n'
(run/'observed.ll').write_text(out)
pages=layout['data_sections'];state=layout['state']
header='#include <stdint.h>\n'
header+=''.join(f'extern uint8_t {s["symbol"]};\n' for s in pages)
header+='static uint8_t *const stage5_pages[4]={'+','.join('&'+s['symbol'] for s in pages)+'};\n'
for k,va in state.items():
    i=next(i for i,s in enumerate(pages) if s['va']<=va<s['va']+4096);offset=va-pages[i]['va']
    header+=f'#define S5_{k} (*(uint32_t*)(stage5_pages[{i}]+{offset}))\n'
(ROOT/'samples/stage5_layout.h').write_text(header)
result=dict(original_llvm_sha256=hashlib.sha256(src.encode()).hexdigest(),instrumented_llvm_sha256=hashlib.sha256(out.encode()).hexdigest(),changes=['read-only test entry counters at four original functions','nine unresolved-boundary abort-only definitions; no return','exact source/native code continuation lookup table'],mapped_native_functions=count,original_instruction_replacements=0)
(ROOT/'evidence/stage5-native-instrumentation.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
