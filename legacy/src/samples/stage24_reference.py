#!/usr/bin/env python3
"""Independent original-i386 graphics oracle, metadata from original loader."""
import json,struct,hashlib,collections
from pathlib import Path
from stage7_runtime import u,x,PE,SOURCE
R=Path(__file__).resolve().parents[1];run=R/'runs/stage24-sprites';pe=PE(SOURCE)
info=json.loads((R/'evidence/stage24-extraction.json').read_text())
s=json.loads((R/'runs/stage12-state-fields/state.json').read_text());memory=(R/'runs/stage12-state-fields/memory.bin').open('rb')
def old(a,n):
 for r in s['regions']:
  if r['low']<=a and a+n-1<=r['high']:memory.seek(r['offset']+a-r['low']);return memory.read(n)
 raise ValueError(hex(a))
group=0x1d0f570;recbase=struct.unpack('<I',old(group,4))[0];pixelbase=struct.unpack('<I',old(group+52268,4))[0]
records=bytearray(old(recbase,172*72))
deps=[json.loads((R/'evidence'/n).read_text())for n in ['stage18-elevation-dependency.json','stage18-pixels-dependency.json']]
sg3,resource=[Path(d['copy_path']).read_bytes()for d in deps]
for d,b in zip(deps,[sg3,resource]):assert hashlib.sha256(b).hexdigest()==d['sha256'] and Path(d['original_path']).read_bytes()==b
record_checks=0
for i in range(172):
 raw=sg3[40680+(201+i)*64:40680+(202+i)*64];live=records[i*72:(i+1)*72]
 source,size=struct.unpack_from('<II',raw)
 assert source-39214==struct.unpack_from('<I',live)[0]-pixelbase
 assert raw[4:12]==live[4:12] and raw[16:28]==live[16:28] and raw[32:64]==live[32:64]
 assert old(struct.unpack_from('<I',live)[0],size)==resource[source:source+size]
 struct.pack_into('<I',records,i*72,0x10000000+source);record_checks+=1
uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);pages=set()
def map_(a,n):
 for b in range(a&~4095,(a+n+4095)&~4095,4096):
  if b not in pages:uc.mem_map(b,4096);pages.add(b)
for fragment in info['fragments']:
 lo,hi=int(fragment['start'],16),int(fragment['end'],16);map_(lo,hi-lo);uc.mem_write(lo,pe.read(lo,hi-lo))
for a in info['absolute_operand_addresses']:
 v=int(a,16);map_(v,4)
 if v<0xa00000:uc.mem_write(v&~4095,pe.read(v&~4095,4096))
for a,n in [(0x10000000,0x220000),(0x10300000,0x10000),(0x11000000,0x100000),(0x12000000,0x200000),(0x1c42000,0x100000),(0x70000000,0x10000),(0x62000000,4096)]:map_(a,n)
uc.mem_write(0x10000000,resource);uc.mem_write(0x10300000,bytes(records))
def word(a,v):uc.mem_write(a,struct.pack('<I',v&0xffffffff))
word(group,0x10300000);word(group+52276,172)
seen=collections.Counter();calls=0
def hook(uc,addr,n,_):seen[addr]+=1
uc.hook_add(u.UC_HOOK_BLOCK,hook)
def call(a,args,this=0,stdcall=False):
 global calls
 sp=0x7000fe00;uc.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),0x62000000,*[v&0xffffffff for v in args]))
 uc.reg_write(x.UC_X86_REG_ESP,sp);uc.reg_write(x.UC_X86_REG_ECX,this);uc.reg_write(x.UC_X86_REG_EFLAGS,0x3202)
 for r,v in [(x.UC_X86_REG_EBX,0x12345678),(x.UC_X86_REG_ESI,0x23456789),(x.UC_X86_REG_EDI,0x3456789a),(x.UC_X86_REG_EBP,0x456789ab)]:uc.reg_write(r,v)
 uc.emu_start(a,0x62000000,timeout=5000000,count=10000000)
 assert uc.reg_read(x.UC_X86_REG_EIP)==0x62000000,(hex(a),hex(uc.reg_read(x.UC_X86_REG_EIP)))
 assert uc.reg_read(x.UC_X86_REG_ESP)==sp+4+(len(args)*4 if stdcall else 0)
 for r,v in [(x.UC_X86_REG_EBX,0x12345678),(x.UC_X86_REG_ESI,0x23456789),(x.UC_X86_REG_EDI,0x3456789a),(x.UC_X86_REG_EBP,0x456789ab)]:assert uc.reg_read(r)==v,(hex(a),r)
 calls+=1;return uc.reg_read(x.UC_X86_REG_EAX)
def render(w,h,cw,ch,draws):
 for a,v in {17686528:w*2,17686532:h,17686536:w,18181624:0x11000000,17593124:0,17593120:cw,17593116:0,17593112:ch,17687676:0x555,11109004:0,17686597:0}.items():word(a,v)
 for idx,px,py in draws:
  call(0x5cd8a0,[16*16384+idx,px,py,0,0,0],0x1c42130,True)
  call(0x5ce800,[16*16384+idx,px,py],0x1c42130,True)
 src=bytes(uc.mem_read(0x11000000,w*h*2));uc.mem_write(0x12000000,b'\xa5'*(w*h*4))
 call(0x418ff0,[0x11000000,0x12000000,w,h,w,0,0,0])
 return src,bytes(uc.mem_read(0x12000000,w*h*4))
cases=[(640,360,640,360,[(i,50+140*(z%4),150+155*(z//4))for z,i in enumerate([0,3,8,12,16,20,23,24])],[(6,470,280)])]
for px,py in [(35,90),(-23,90),(120,90),(35,12),(35,130),(250,90),(35,-100),(-77,90),(159,90)]:cases.append((164,150,160,150,[(0,px,py)],[(2,px+4,py+3)]))
cases.append((164,150,160,150,[(172,35,90)],[(0,35,90)]))
with(run/'cases.bin').open('wb')as f:
 f.write(struct.pack('<I',len(cases)))
 for i,(w,h,cw,ch,first,second)in enumerate(cases):
  uc.mem_write(0x11000000,struct.pack('<H',0x2947)*(w*h))
  a,b=render(w,h,cw,ch,first);c,d=render(w,h,cw,ch,second)
  f.write(struct.pack('<6I',w,h,cw,ch,len(first),len(second)))
  for draws in[first,second]:
   for q in draws:f.write(struct.pack('<3i',*q))
  f.write(a);f.write(b);f.write(c);f.write(d)
  print('original-byte case',i,'PASS',hashlib.sha256(d).hexdigest(),flush=True)
summary=dict(reference='Unicorn 2.1.4 executing unmodified original i386 sprite/clip/pixel instructions',cases=len(cases),original_function_calls=calls,
 original_loader_records_validated=record_checks,source_images=[0,3,8,12,16,20,23,24,6,2],source_data_offset=39214,first_complete_sprite_file_offset=39218,
 original_EXE_sha256=hashlib.sha256(pe.data).hexdigest(),resources=deps,fixture_sha256=hashlib.sha256((run/'cases.bin').read_bytes()).hexdigest(),
 selected_original_entry_hits={p['name']:seen[int(p['start'],16)]for p in info['fragments']},
 unsupported_hit_counts={a:seen[int(a,16)]for a in info['external_abort_addresses']if seen[int(a,16)]},
 scope='Reference starts isolated graphics calls; original loader checkpoint only supplies parsed asset records. No full startup or city progress claim.')
(R/'evidence/stage24-reference.json').write_text(json.dumps(summary,indent=2));print('PASS',calls,'original calls;',record_checks,'loader records exact asset comparison')
