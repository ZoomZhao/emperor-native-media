#!/usr/bin/env python3
"""Own bounded PE probe for a measured CT_CTYPE1 UTF16-unit profile."""
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
BASE=0x400000;DATA=BASE+0x3000;IDATA=0x50000
COUNT=65536;TABLE=16;BOUNDARY=TABLE+COUNT*2+8;STRIDE=40
CASES=[(1,1),(1,0),(1,-1),(1,-2),(0,1),(3,1)]
OUTPUT_SIZE=BOUNDARY+len(CASES)*STRIDE
INPUT=(OUTPUT_SIZE+3)&~3
NAMES=['GetStringTypeW','SetLastError','GetLastError','CreateFileA','WriteFile','CloseHandle','ExitProcess']

def build():
    idata=bytearray(512);struct.pack_into('<5I',idata,0,IDATA+0x40,0,0,IDATA+0xd0,IDATA+0x80)
    idata[0xd0:0xdd]=b'KERNEL32.dll\0';pos=0xe0
    for i,name in enumerate(NAMES):
        sym=struct.pack('<H',0)+name.encode()+b'\0'
        for offset in [0x40,0x80]:struct.pack_into('<I',idata,offset+4*i,IDATA+pos)
        idata[pos:pos+len(sym)]=sym;pos=(pos+len(sym)+1)&~1
    data=bytearray(b'CTYPE110'+bytes(8)+bytes([0xa5])*(OUTPUT_SIZE-16))
    for i,(info,length) in enumerate(CASES):struct.pack_into('<4I',data,BOUNDARY+STRIDE*i,info,length&0xffffffff,0,0)
    data.extend(bytes(INPUT-len(data)))
    data.extend(struct.pack('<65536H',*range(COUNT)))
    sample_offset=len(data);data.extend('A中\0'.encode('utf-16le'))
    data.extend(bytes(8));written_count=len(data)-4
    code=bytearray()
    def push(v):code.extend(b'\x68'+struct.pack('<I',v&0xffffffff))
    def call(name):code.extend(b'\xff\x15'+struct.pack('<I',BASE+IDATA+0x80+4*NAMES.index(name)))
    def store(addr):code.extend(b'\xa3'+struct.pack('<I',addr))
    def classification(info,source,n,out,status):
        push(0x13579);call('SetLastError')
        for v in [out,n,source,info]:push(v)
        call('GetStringTypeW');store(status);call('GetLastError');store(status+4)
    classification(1,DATA+INPUT,COUNT,DATA+TABLE,DATA+8)
    for i,(info,n) in enumerate(CASES):
        at=DATA+BOUNDARY+STRIDE*i
        classification(info,DATA+sample_offset,n,at+20,at+8)
    for v in [0,0x80,2,0,0,0x40000000,BASE+0x2000]:push(v)
    call('CreateFileA');code.extend(b'\x89\xc3')
    for v in [0,DATA+written_count,OUTPUT_SIZE,DATA]:push(v)
    code.extend(b'\x53');call('WriteFile');code.extend(b'\x53');call('CloseHandle');push(0);call('ExitProcess')
    chunks=[('.text',0x1000,code,0x60000020),('.rdata',0x2000,b'C:\\EmperorRotMK[ZeaS]\\ST10-CTYPE1.bin\0',0x40000040),('.data',0x3000,data,0xc0000040),('.idata',IDATA,idata,0xc0000040)]
    header=bytearray(1024);header[:2]=b'MZ';struct.pack_into('<I',header,0x3c,0x80)
    header[0x80:0x84]=b'PE\0\0';struct.pack_into('<HHIIIHH',header,0x84,0x14c,4,0,0,0,224,0x10f)
    opt=0x98
    struct.pack_into('<HBB9I6H4I2H6I',header,opt,0x10b,1,0,512,(len(data)+1023)&~511,0,0x1000,0x1000,0x3000,BASE,0x1000,0x200,4,0,0,0,4,0,0,IDATA+0x1000,1024,0,2,0,0x100000,0x1000,0x100000,0x1000,0,16)
    struct.pack_into('<II',header,opt+96+8,IDATA,40);struct.pack_into('<II',header,opt+96+12*8,IDATA+0x80,4*(len(NAMES)+1))
    output=bytearray(header)
    for i,(name,rva,body,flags) in enumerate(chunks):
        raw=len(output);rawsize=(len(body)+511)&~511
        assert rva!=0x3000 or rva+len(body)<IDATA
        struct.pack_into('<8sIIIIIIHHI',output,opt+224+40*i,name.encode().ljust(8,b'\0'),len(body),rva,rawsize,raw,0,0,0,0,flags)
        output.extend(body+bytes(rawsize-len(body)))
    path=ROOT/'samples/stage10-ctype-probe.exe';path.write_bytes(output)
    meta=dict(sha256=hashlib.sha256(output).hexdigest(),size=len(output),all_source_authored_here=True,input='all 65536 UTF16 code units; no game bytes',output_size=OUTPUT_SIZE,cases=CASES)
    (ROOT/'evidence/stage10-ctype-probe-source.json').write_text(json.dumps(meta,indent=2)+'\n');return meta

def parse(data):
    assert len(data)==OUTPUT_SIZE and data[:8]==b'CTYPE110'
    status,error=struct.unpack_from('<II',data,8)
    table=data[TABLE:TABLE+COUNT*2]
    assert data[TABLE+COUNT*2:BOUNDARY]==bytes([0xa5])*8
    rows=[]
    for i in range(len(CASES)):
        at=BOUNDARY+STRIDE*i;info,n,value,last=struct.unpack_from('<4I',data,at)
        rows.append(dict(info_type=info,signed_length=n if n<0x80000000 else n-0x100000000,return_value=value,last_error=last,canary_buffer_hex=data[at+16:at+40].hex(),output_hex=data[at+20:at+36].hex()))
    return dict(return_value=status,last_error=error,table_sha256=hashlib.sha256(table).hexdigest(),table_size=len(table),rows=rows,table_end_guard_unchanged=True)

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);p.add_argument('--tag',default='first');a=p.parse_args()
    assert a.tag.isalnum()
    if a.action=='build':print(json.dumps(build(),indent=2));return
    meta=json.loads((ROOT/'evidence/stage10-ctype-probe-source.json').read_text())
    exe=ROOT/'samples/stage10-ctype-probe.exe';assert hashlib.sha256(exe.read_bytes()).hexdigest()==meta['sha256']
    target=GAME/'ST10-CTYPE-PROBE.exe';assert not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest()==meta['sha256']
    output=GAME/'ST10-CTYPE1.bin';assert not output.exists();shutil.copyfile(exe,target)
    env=os.environ.copy();env.update(WINEPREFIX=str(APP/'Resources'),WINEDEBUG='+loaddll,+seh',DYLD_FALLBACK_LIBRARY_PATH=str(APP/'SharedSupport/wine/lib'))
    command=[str(APP/'SharedSupport/wine/bin/wine32on64'),'C:\\EmperorRotMK[ZeaS]\\ST10-CTYPE-PROBE.exe']
    start=time.monotonic();result=dict(command=command,budget_seconds=20,probe_sha256=meta['sha256'])
    with (ROOT/f'evidence/stage10-ctype-probe-{a.tag}-wine.log').open('w') as log:
        child=subprocess.Popen(command,cwd=GAME,env=env,stdout=log,stderr=subprocess.STDOUT);result['pid']=child.pid
        try:child.wait(timeout=18)
        except subprocess.TimeoutExpired:
            result['deadline']=True;child.terminate()
            try:child.wait(timeout=1)
            except subprocess.TimeoutExpired:child.kill();child.wait(timeout=.5)
        result['returncode']=child.returncode
    if output.is_file():
        data=output.read_bytes();(ROOT/'evidence/stage10-ctype-probe-output.bin').write_bytes(data)
        result.update(parse(data));result['output_sha256']=hashlib.sha256(data).hexdigest()
        (ROOT/'evidence/stage10-ctype1-table.bin').write_bytes(data[TABLE:TABLE+COUNT*2])
    else:result['failure']='missing bounded classification output'
    result['elapsed_seconds']=round(time.monotonic()-start,3)
    (ROOT/f'evidence/stage10-ctype-probe-{a.tag}-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
