#!/usr/bin/env python3
"""Entirely own PE32 probe: GetACP/GetCPInfo -> one local 28-byte output.

No game instruction bytes, external compiler, downloader, or third-party source.
The output is restricted to the already isolated Wine prefix game directory.
"""
import hashlib
import json
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=0x400000
names=['GetACP','GetCPInfo','CreateFileA','WriteFile','CloseHandle','ExitProcess']
idata=bytearray(512)
struct.pack_into('<5I',idata,0,0x4040,0,0,0x40d0,0x4080)
idata[0xd0:0xd0+13]=b'KERNEL32.dll\0'
at=0xe0
for i,name in enumerate(names):
    raw=struct.pack('<H',0)+name.encode('ascii')+b'\0'
    struct.pack_into('<I',idata,0x40+4*i,0x4000+at)
    struct.pack_into('<I',idata,0x80+4*i,0x4000+at)
    idata[at:at+len(raw)]=raw;at=(at+len(raw)+1)&~1

code=bytearray()
def push(value):code.extend(b'\x68'+struct.pack('<I',value))
def call(name):code.extend(b'\xff\x15'+struct.pack('<I',BASE+0x4080+4*names.index(name)))
def store(va):code.extend(b'\xa3'+struct.pack('<I',va))
call('GetACP');store(BASE+0x3000)
push(BASE+0x3004);code.extend(b'\x50');call('GetCPInfo');store(BASE+0x3018)
for value in [0,0x80,2,0,0,0x40000000,BASE+0x2000]:push(value)
call('CreateFileA');code.extend(b'\x89\xc3')
for value in [0,BASE+0x301c,28,BASE+0x3000]:push(value)
code.extend(b'\x53');call('WriteFile');code.extend(b'\x53');call('CloseHandle');push(0);call('ExitProcess')

header=bytearray(1024);header[:2]=b'MZ';struct.pack_into('<I',header,0x3c,0x80)
header[0x80:0x84]=b'PE\0\0';struct.pack_into('<HHIIIHH',header,0x84,0x14c,4,0,0,0,224,0x10f)
opt=0x98
struct.pack_into('<HBB9I6H4I2H6I',header,opt,0x10b,1,0,512,1536,0,0x1000,0x1000,0x3000,BASE,0x1000,0x200,4,0,0,0,4,0,0,0x5000,1024,0,2,0,0x100000,0x1000,0x100000,0x1000,0,16)
struct.pack_into('<II',header,opt+96+8,0x4000,40)
struct.pack_into('<II',header,opt+96+12*8,0x4080,4*(len(names)+1))
chunks=[('.text',0x1000,bytes(code),0x60000020),('.rdata',0x2000,b'C:\\EmperorRotMK[ZeaS]\\ST10-NLS.bin\0',0x40000040),('.data',0x3000,bytes(32),0xc0000040),('.idata',0x4000,bytes(idata),0xc0000040)]
output=bytearray(header)
for i,(name,rva,data,flags) in enumerate(chunks):
    assert len(data)<=512
    raw=1024+i*512
    struct.pack_into('<8sIIIIIIHHI',output,opt+224+40*i,name.encode().ljust(8,b'\0'),len(data),rva,512,raw,0,0,0,0,flags)
    output.extend(data+bytes(512-len(data)))
path=ROOT/'samples/stage10-nls-probe.exe';path.write_bytes(output)
record=dict(path=str(path),sha256=hashlib.sha256(output).hexdigest(),size=len(output),source='samples/make_stage10_nls_probe.py; all source and machine code authored here',
            operations=names,output_guest_path='C:\\EmperorRotMK[ZeaS]\\ST10-NLS.bin',output_size=28,
            network_or_registry_write=False,game_source_code_used=False,third_party_source_used=False)
(ROOT/'evidence/stage10-nls-probe-source.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
