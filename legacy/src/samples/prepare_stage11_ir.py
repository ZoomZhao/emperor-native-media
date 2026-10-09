#!/usr/bin/env python3
"""Unresolved original helpers get abort-only LLVM bodies, never fake returns."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];run=ROOT/'runs/stage11-native'
source=(run/'combined.ll').read_text();out=source
layout=json.loads((ROOT/'evidence/stage11-native-slice-layout.json').read_text())
for va in layout['external_abort_addresses']:
    name='unresolved_'+va[2:].upper()
    out,n=re.subn(r'^declare[^\n]*@'+name+r'\([^\n]*\n','',out,flags=re.M);assert n==1,name
    out+=f'\ndefine hidden fastcc void @{name}(%_cpu* %cpu) nounwind {{\ncall ccc void @stage11_forbidden_api(i32 {int(va,16)})\nunreachable\n}}\n'
(run/'bounded.ll').write_text(out)
(ROOT/'evidence/stage11-native-abort-bindings.json').write_text(json.dumps(dict(original_LLVM_sha256=hashlib.sha256(source.encode()).hexdigest(),bounded_LLVM_sha256=hashlib.sha256(out.encode()).hexdigest(),abort_only_original_helpers=layout['external_abort_addresses'],original_instruction_replacements=0),indent=2)+'\n')
print('Abort-only original boundaries ready.')
