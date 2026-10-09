#!/usr/bin/env python3
"""Read-only constant-data analysis of the fixed DLL, never execute DLL code."""
import sys,json,hashlib,struct,re
from pathlib import Path
from auto_i386_analysis import PE,decode
R=Path(__file__).resolve().parents[1];path=R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/mss32.dll';p=PE(path)
assert hashlib.sha256(p.data).hexdigest()=='f09e6024ad35eb6b7b46982efb998df26d162391b26fcf56519a4c23fcbc7015'
# Linear, literal-only constructor data assignments. Reject calls, branches,
# pointers and unknown inputs; this reconstructs a table, not a DLL runtime.
pc=0x21101260;regs={};writes=[];defaults=[0]*45
while pc<0x211013ae:
 i=decode(p,pc);pc=i['end'];a=i['args']
 if i['name']=='ret':break
 if i['name']=='xor':assert a[0]==a[1]and a[0]['kind']=='reg';regs[a[0]['reg']]=0
 elif i['name']=='mov':
  v=a[1]['value']if a[1]['kind']=='imm'else regs[a[1]['reg']]
  if a[0]['kind']=='reg':regs[a[0]['reg']]=v
  else:
   t=a[0];assert t['kind']=='mem'and t['base']is None and t['index']is None
   addr=t['disp'];bits=t['bits'];writes.append(dict(pc=hex(i['pc']),address=hex(addr),bits=bits,value=v))
   if 0x21150bec<=addr<0x21150ca0:assert bits==32 and(addr-0x21150bec)%4==0;defaults[(addr-0x21150bec)//4]=v
 elif i['name']=='nop':pass
 else:raise AssertionError('nonconstant initializer instruction '+str(i))
assert defaults[42]==8 and defaults[44]==1
lines=(R/'evidence/stage25-miles-text-readonly.stdout.log').read_text().splitlines()
def window(lo,hi):
 return '\n'.join(l for l in lines if(m:=re.match(r'^([0-9a-f]+):',l))and lo<=int(m[1],16)<hi)+'\n'
for lo,hi,name in [(0x21101260,0x211013ae,'defaults'),(0x21101dd0,0x21101e14,'dllmain'),(0x21101f10,0x21102030,'NT-attach'),(0x21102340,0x21102560,'startup')]:
 (R/'evidence'/('stage25-miles-'+name+'-readonly.log')).write_text(window(lo,hi))
guards={}
for name,addr,n in [('debug',0x21151e18,2),('debug_nested',0x21151e1a,2),('depth',0x21151e1c,2),('mutex',0x21151e20,4),('startup',0x21151e24,4),('dispatcher',0x21151e28,4)]:
 s=p.section(addr);assert s and addr>=s['va']+s['rawsize'] and addr+n<=s['va']+s['size'];guards[name]=dict(address=hex(addr),initial_zero_BSS=True)
out=dict(original_DLL_sha256=hashlib.sha256(p.data).hexdigest(),defaults=defaults,preference42_default=8,mutex_control_preference44=1,
 initializer_writes=writes,initializer_bytes_sha256=hashlib.sha256(p.read(0x21101260,0x14e)).hexdigest(),zero_BSS_guards=guards,
 setter='0x211029e0 wrapper -> mutex -> 0x21101000 exchange-and-return-prior -> mutex release',
 original_DLL_code_executed=False,scope='Attached NT-family library before AIL_startup, logging off, no legacy dispatcher. Debug/dispatcher/startup paths remain unsupported.',
 observation='DllMain process attach 21101de7 calls literal-default initializer; NT platform branch 21101f50/1f56 leaves legacy dispatcher BSS zero; AIL_startup later reinitializes defaults and loads RIB providers, so must not be a fake success.')
(R/'evidence/stage25-miles-semantics.json').write_text(json.dumps(out,indent=2));(R/'runs/stage25-native').mkdir(exist_ok=True);(R/'runs/stage25-native/defaults.bin').write_bytes(struct.pack('<45I',*defaults));print('PASS readonly Miles defaults: preference42=8, preference44=1; no DLL execution')
