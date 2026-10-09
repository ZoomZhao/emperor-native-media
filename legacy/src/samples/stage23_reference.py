#!/usr/bin/env python3
"""Original i386 byte oracle, isolated buffers, no game entry or Windows imports."""
import json,struct,hashlib
from pathlib import Path
from stage7_runtime import u,x,PE,SOURCE
R=Path(__file__).resolve().parents[1];pe=PE(SOURCE);assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
m=json.loads((R/'evidence/stage18-pixels-dependency.json').read_text());resource=Path(m['copy_path']).read_bytes();assert hashlib.sha256(resource).hexdigest()==m['sha256'];assert Path(m['original_path']).read_bytes()==resource
# 4096 contiguous pixel words from the first original resource block, offset 4.
# This bounded tile is not a decoded SG3 sprite or map.
words=resource[4:4+8192]
uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);uc.mem_map(0x418000,8192);uc.mem_write(0x418000,pe.read(0x418000,8192));uc.mem_map(0x50000000,0x100000);uc.mem_map(0x70000000,0x10000);uc.mem_map(0x62000000,4096)
calls=0
def convert(fmt,row,w):
 global calls
 uc.mem_write(0x50000000,bytes(row));uc.mem_write(0x50010000,b'\xa5'*(w*4));sp=0x7000ff00
 uc.mem_write(sp,struct.pack('<9I',0x62000000,0x50000000,0x50010000,w,1,w,0,0,0));uc.reg_write(x.UC_X86_REG_ESP,sp)
 uc.emu_start(0x418ff0 if fmt==555 else 0x419070,0x62000000,timeout=1000000,count=500000)
 assert uc.reg_read(x.UC_X86_REG_EIP)==0x62000000 and uc.reg_read(x.UC_X86_REG_ESP)==sp+4
 calls+=1;return bytes(uc.mem_read(0x50010000,w*4))
cases=[]
for i,(fmt,w,h,dx,dy,keyed) in enumerate([(555,64,64,2,1,0),(565,64,64,2,1,0),(555,13,7,-3,-2,0),(565,13,7,5,3,1),(555,1,1,0,0,0),(565,1,1,7,5,0),(555,17,9,99,99,0),(565,31,11,-4,2,1)]):
 sp=w*2+8;dw=w+8;dh=h+6;dp=dw*4+16;src=bytearray(b'\x5a'*(sp*h))
 for y in range(h):
  for z in range(w):src[y*sp+z*2:y*sp+z*2+2]=words[((y*w+z)%4096)*2:((y*w+z)%4096)*2+2]
 if i in (4,5):src[:2]=(0x7fff if fmt==555 else 0xffff).to_bytes(2,'little')
 key=int.from_bytes(src[:2],'little');expected=bytearray(b'\xa5'*(dp*dh))
 for y in range(h):
  row=convert(fmt,src[y*sp:y*sp+w*2],w)
  for z in range(w):
   if 0<=dx+z<dw and 0<=dy+y<dh and not(keyed and int.from_bytes(src[y*sp+z*2:y*sp+z*2+2],'little')==key):
    a=(dy+y)*dp+(dx+z)*4;expected[a:a+4]=row[z*4:z*4+4]
 updated=bytearray(expected);mx=max(0,-dx);my=max(0,-dy)
 if mx<w and my<h and 0<=dx+mx<dw and 0<=dy+my<dh:
  v=int.from_bytes(src[my*sp+mx*2:my*sp+mx*2+2],'little')^0x1f
  rgba=convert(fmt,v.to_bytes(2,'little'),1)
  if not(keyed and v==key):
   a=(dy+my)*dp+(dx+mx)*4;updated[a:a+4]=rgba
 cases.append((struct.pack('<12i',fmt,w,h,sp,dw,dh,dp,dx,dy,keyed,key,len(src)),src,expected,updated))
out=R/'runs/stage23-graphics';out.mkdir(exist_ok=True)
with (out/'cases.bin').open('wb')as f:
 f.write(struct.pack('<I',len(cases)))
 for hd,src,ex,updated in cases:f.write(hd);f.write(src);f.write(ex);f.write(updated)
(R/'evidence/stage23-reference.json').write_text(json.dumps(dict(reference='Unicorn 2.1.4 original unmodified i386 bytes',function_addresses=['0x418ff0','0x419070'],cases=len(cases),original_calls=calls,resource_sha256=m['sha256'],resource_slice_offset=4,resource_slice_bytes=8192,resource_note='Six bounded cases use original first-resource words; two 1x1 cases use explicit max-channel boundary controls. The 64x64 tile is not a decoded complete SG3 sprite.',fixture_sha256=hashlib.sha256((out/'cases.bin').read_bytes()).hexdigest()),indent=2))
print('PASS independent original-byte reference',len(cases),'cases',calls,'calls')
