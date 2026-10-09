#!/usr/bin/env python3
"""Authored COM ABI wrappers; all guest instruction translations stay intact."""
from pathlib import Path
import json
R=Path(__file__).resolve().parents[1];run=R/'runs/stage24-sprites'
wrappers={'QueryInterface':(3,3),'AddRef':(1,1),'Release':(1,1),'CreateSurface':(4,4),'CreateClipper':(4,4),
 'SetCooperativeLevel':(3,3),'SetDisplayMode':(4,4),'Lock':(5,5),'Unlock':(2,2),'Blt':(6,6),'BltFast':(6,6),
 'GetPixelFormat':(2,2),'GetSurfaceDesc':(2,2),'SetClipper':(2,2),'SetHWnd':(3,3),'GetHWnd':(2,2),
 'clock':(0,0),'sprintf':(4,0),'log':(3,0),'unsupported':(0,0)}
text=['include llasm.llinc','include asm-calls.llinc']
for nm,(n,pop)in wrappers.items():
 text+=['func dd_'+nm+'_c'+(' '+', '.join('a%d'%i for i in range(n))if n else '')]
for nm,(n,pop)in wrappers.items():
 text+=['proc dd_'+nm+' global c_dd_'+nm,'    Call_Asm_Stack%d dd_%s_c'%(n,nm),'    Call_Asm_Return %d'%pop,'endp']
wrapper='\n'.join(text)+'\n';(R/'samples/stage24_wrappers.llasm').write_text(wrapper)
original=(run/'original.llasm').read_text().replace('include llasm.llinc\n','',1);(run/'combined.llasm').write_text(wrapper+'\n'+original)
info=json.loads((R/'evidence/stage24-extraction.json').read_text());hdr=['#include <stdint.h>']
hdr+=['extern unsigned char g_%s[];'%a[2:] for a in info['absolute_operand_addresses']]
hdr+=['extern void c_%s(CPU*);'%p['name']for p in info['fragments']]
hdr+=['extern void dd_%s(void); extern void c_dd_%s(CPU*);'%(n,n)for n in wrappers]
hdr+=['extern void (*ptr_initialize_pointers)(uint64_t);']
(R/'samples/stage24_generated.h').write_text('\n'.join(hdr)+'\n')
aborts='#include <stdio.h>\n#include <stdlib.h>\n'
for a in info['external_abort_addresses']:
 aborts+='void unresolved_%s(void*p){(void)p;fprintf(stderr,"unimplemented original graphics branch %s\\n");exit(71);}\n'%(a[2:].upper(),a)
(R/'samples/stage24_aborts.c').write_text(aborts)
print('prepared',len(wrappers),'COM/platform ABI wrappers and explicit aborts')
