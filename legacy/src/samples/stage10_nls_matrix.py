#!/usr/bin/env python3
"""Author/build/run own Win32 ABI probe; fixed existing isolated Wine only."""
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
APP=ROOT/'stage8-local/EmperorBaseline.app/Contents'
GAME=APP/'Resources/drive_c/EmperorRotMK[ZeaS]'
BASE=0x400000
CASES=[(936,0),(0,0),(1,0),(2,0),(3,0),(65001,0),(1252,0),(0xffffffff,0),(0xffffffff,1)]
NAMES=['GetACP','GetCPInfo','SetLastError','GetLastError','CreateFileA','WriteFile','CloseHandle','ExitProcess']
HEADER=16
STRIDE=48

def build():
    idata=bytearray(512)
    struct.pack_into('<5I',idata,0,0x4040,0,0,0x40d0,0x4080)
    idata[0xd0:0xdd]=b'KERNEL32.dll\0'
    pos=0xe0
    for i,name in enumerate(NAMES):
        symbol=struct.pack('<H',0)+name.encode()+b'\0'
        for table in [0x40,0x80]:struct.pack_into('<I',idata,table+4*i,0x4000+pos)
        idata[pos:pos+len(symbol)]=symbol;pos=(pos+len(symbol)+1)&~1
    data=bytearray(b'NLSMAT10'+struct.pack('<II',len(CASES),0))
    for cp,null in CASES:data.extend(struct.pack('<4I',cp,null,0,0)+bytes([0xa5])*32)
    assert len(data)<=496
    code=bytearray()
    def push(v):code.extend(b'\x68'+struct.pack('<I',v))
    def call(name):code.extend(b'\xff\x15'+struct.pack('<I',BASE+0x4080+4*NAMES.index(name)))
    def store(addr):code.extend(b'\xa3'+struct.pack('<I',addr))
    call('GetACP');store(BASE+0x300c)
    for i,(cp,null) in enumerate(CASES):
        record=BASE+0x3000+HEADER+STRIDE*i
        push(0x13579);call('SetLastError')
        push(0 if null else record+20);push(cp);call('GetCPInfo');store(record+8)
        call('GetLastError');store(record+12)
    for v in [0,0x80,2,0,0,0x40000000,BASE+0x2000]:push(v)
    call('CreateFileA');code.extend(b'\x89\xc3')
    for v in [0,BASE+0x31f0,len(data),BASE+0x3000]:push(v)
    code.extend(b'\x53');call('WriteFile');code.extend(b'\x53');call('CloseHandle');push(0);call('ExitProcess')
    header=bytearray(1024);header[:2]=b'MZ';struct.pack_into('<I',header,0x3c,0x80)
    header[0x80:0x84]=b'PE\0\0';struct.pack_into('<HHIIIHH',header,0x84,0x14c,4,0,0,0,224,0x10f)
    opt=0x98
    struct.pack_into('<HBB9I6H4I2H6I',header,opt,0x10b,1,0,1024,1536,0,0x1000,0x1000,0x3000,BASE,0x1000,0x200,4,0,0,0,4,0,0,0x5000,1024,0,2,0,0x100000,0x1000,0x100000,0x1000,0,16)
    struct.pack_into('<II',header,opt+96+8,0x4000,40)
    struct.pack_into('<II',header,opt+96+12*8,0x4080,4*(len(NAMES)+1))
    chunks=[('.text',0x1000,code,0x60000020),('.rdata',0x2000,b'C:\\EmperorRotMK[ZeaS]\\ST10-NLS-MATRIX.bin\0',0x40000040),('.data',0x3000,data+bytes(512-len(data)),0xc0000040),('.idata',0x4000,idata,0xc0000040)]
    output=bytearray(header)
    for i,(name,rva,body,flags) in enumerate(chunks):
        raw=len(output);rawsize=(len(body)+511)&~511
        assert len(body)<4096
        struct.pack_into('<8sIIIIIIHHI',output,opt+224+40*i,name.encode().ljust(8,b'\0'),len(body),rva,rawsize,raw,0,0,0,0,flags)
        output.extend(body+bytes(rawsize-len(body)))
    exe=ROOT/'samples/stage10-nls-matrix.exe';exe.write_bytes(output)
    meta=dict(sha256=hashlib.sha256(output).hexdigest(),size=len(output),entirely_own_source=True,cases=CASES,imports=NAMES)
    (ROOT/'evidence/stage10-nls-matrix-source.json').write_text(json.dumps(meta,indent=2)+'\n')
    return exe,meta

def parse(data):
    assert data[:8]==b'NLSMAT10' and len(data)==HEADER+STRIDE*len(CASES)
    assert struct.unpack_from('<I',data,8)[0]==len(CASES)
    rows=[]
    for i in range(len(CASES)):
        at=HEADER+i*STRIDE
        cp,null,value,error=struct.unpack_from('<4I',data,at)
        rows.append(dict(code_page=cp,null_pointer=bool(null),return_value=value,last_error=error,canary_buffer_hex=data[at+16:at+48].hex(),CPINFO_hex=data[at+20:at+40].hex()))
    return dict(ACP=struct.unpack_from('<I',data,12)[0],rows=rows)

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);p.add_argument('--tag',default='first');a=p.parse_args()
    assert a.tag.isalnum()
    if a.action=='build':print(json.dumps(build()[1],indent=2));return
    meta=json.loads((ROOT/'evidence/stage10-nls-matrix-source.json').read_text())
    exe=ROOT/'samples/stage10-nls-matrix.exe'
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==meta['sha256']
    output=GAME/'ST10-NLS-MATRIX.bin';assert not output.exists()
    env=os.environ.copy();env['WINEPREFIX']=str(APP/'Resources');env['WINEDEBUG']='+loaddll'
    env['DYLD_FALLBACK_LIBRARY_PATH']=str(APP/'SharedSupport/wine/lib')
    target=GAME/'ST10-NLS-MATRIX.exe'
    assert not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest()==meta['sha256']
    shutil.copyfile(exe,target)
    command=[str(APP/'SharedSupport/wine/bin/wine32on64'),'C:\\EmperorRotMK[ZeaS]\\ST10-NLS-MATRIX.exe']
    started=time.monotonic();result=dict(command=command,exe_sha256=meta['sha256'],budget_seconds=20)
    with (ROOT/f'evidence/stage10-nls-matrix-{a.tag}-wine.log').open('w') as log:
        child=subprocess.Popen(command,cwd=GAME,env=env,stdout=log,stderr=subprocess.STDOUT);result['pid']=child.pid
        try:child.wait(timeout=18)
        except subprocess.TimeoutExpired:
            result['deadline']=True;child.terminate()
            try:child.wait(timeout=1)
            except subprocess.TimeoutExpired:child.kill();child.wait(timeout=.5)
        result['returncode']=child.returncode
    if output.is_file():
        data=output.read_bytes();(ROOT/'evidence/stage10-nls-matrix-output.bin').write_bytes(data)
        result.update(parse(data));result['output_sha256']=hashlib.sha256(data).hexdigest()
    else:result['failure']='own matrix output missing'
    result['elapsed_seconds']=round(time.monotonic()-started,3)
    (ROOT/f'evidence/stage10-nls-matrix-{a.tag}-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
