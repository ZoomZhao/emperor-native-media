#!/usr/bin/env python3
"""Own PE writer and bounded runner; only task-authored operations/data."""
import hashlib,json,os,shutil,struct,subprocess,time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
APP=ROOT/'stage8-local/EmperorBaseline.app/Contents'
GAME=APP/'Resources/drive_c/EmperorRotMK[ZeaS]'
BASE=0x400000;DATA=BASE+0x20000;IDATA=0x200000

class Data:
    def __init__(self):self.data=bytearray()
    def alloc(self,body):
        at=len(self.data);self.data.extend(body);return DATA+at
    def offset(self,address):return address-DATA

def build(name,data,operations,callback_stub=False,dll_for=None):
    assert name.isalnum() and len(data.data)<0x1d0000
    names=list(dict.fromkeys([op['name'] for op in operations]+['SetLastError','GetLastError','CreateFileA','WriteFile','CloseHandle','ExitProcess']))
    idata=bytearray(1024);struct.pack_into('<5I',idata,0,IDATA+0x40,0,0,IDATA+0xd0,IDATA+0x80)
    assert len(names)<=15
    idata[0xd0:0xdd]=b'KERNEL32.dll\0';pos=0xe0
    for i,symbol in enumerate(names):
        sym=struct.pack('<H',0)+symbol.encode()+b'\0'
        for tab in [0x40,0x80]:struct.pack_into('<I',idata,tab+4*i,IDATA+pos)
        idata[pos:pos+len(sym)]=sym;pos=(pos+len(sym)+1)&~1
    slots={symbol:BASE+IDATA+0x80+4*i for i,symbol in enumerate(names)}
    import_bytes=40;iat_rva=IDATA+0x80;iat_size=4*(len(names)+1)
    if dll_for:
        # Task-authored multi-DLL import layout for bounded GUID probes.
        # Existing single-DLL probes and captured binaries are untouched.
        groups={}
        for symbol in names:groups.setdefault(dll_for.get(symbol,'KERNEL32.dll'),[]).append(symbol)
        idata=bytearray(4096);position=((len(groups)+1)*20+15)&~15;first_iat=None;last_iat=0
        for index,(dll,symbols) in enumerate(groups.items()):
            ilt=position;position+=4*(len(symbols)+1)
            iat=position;position+=4*(len(symbols)+1)
            first_iat=iat if first_iat is None else first_iat;last_iat=position
            dll_name=position;encoded=dll.encode()+b'\0';idata[position:position+len(encoded)]=encoded;position=(position+len(encoded)+1)&~1
            struct.pack_into('<5I',idata,index*20,IDATA+ilt,0,0,IDATA+dll_name,IDATA+iat)
            for j,symbol in enumerate(symbols):
                record=struct.pack('<H',0)+symbol.encode()+b'\0'
                for table in [ilt,iat]:struct.pack_into('<I',idata,table+j*4,IDATA+position)
                slots[symbol]=BASE+IDATA+iat+j*4
                idata[position:position+len(record)]=record;position=(position+len(record)+1)&~1
        assert position<len(idata)
        import_bytes=(len(groups)+1)*20;iat_rva=IDATA+first_iat;iat_size=last_iat-first_iat
    code=bytearray()
    def push(v):code.extend(b'\x68'+struct.pack('<I',v&0xffffffff))
    def call(n):code.extend(b'\xff\x15'+struct.pack('<I',slots[n]))
    def store(addr):code.extend(b'\xa3'+struct.pack('<I',addr))
    for op in operations:
        push(0x13579);call('SetLastError')
        for v in reversed(op['args']):
            if isinstance(v,dict):code.extend(b'\xff\x35'+struct.pack('<I',v['load']))
            else:push(v)
        call(op['name']);store(op['result']);call('GetLastError');store(op['result']+4)
    output_size=len(data.data);written=data.alloc(bytes(4))
    for v in [0,0x80,2,0,0,0x40000000,BASE+0x10000]:push(v)
    call('CreateFileA');code.extend(b'\x89\xc3')
    for v in [0,written,output_size,DATA]:push(v)
    code.extend(b'\x53');call('WriteFile');code.extend(b'\x53');call('CloseHandle');push(0);call('ExitProcess')
    if callback_stub:
        assert len(code)<0xe000
        code.extend(bytes(0xe000-len(code)))
        code.extend(b'\x31\xc0\xc2\x04\x00')
    assert 0x1000+len(code)<0x10000
    chunks=[('.text',0x1000,code,0x60000020),('.rdata',0x10000,('C:\\EmperorRotMK[ZeaS]\\ST10-'+name+'.bin\0').encode(),0x40000040),('.data',0x20000,data.data,0xc0000040),('.idata',IDATA,idata,0xc0000040)]
    header=bytearray(1024);header[:2]=b'MZ';struct.pack_into('<I',header,0x3c,0x80);header[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',header,0x84,0x14c,4,0,0,0,224,0x10f);opt=0x98
    struct.pack_into('<HBB9I6H4I2H6I',header,opt,0x10b,1,0,(len(code)+511)&~511,(len(data.data)+2047)&~511,0,0x1000,0x1000,0x20000,BASE,0x1000,0x200,4,0,0,0,4,0,0,IDATA+0x1000,1024,0,2,0,0x100000,0x1000,0x100000,0x1000,0,16)
    struct.pack_into('<II',header,opt+96+8,IDATA,import_bytes);struct.pack_into('<II',header,opt+96+12*8,iat_rva,iat_size)
    output=bytearray(header)
    for i,(section,rva,body,flags) in enumerate(chunks):
        raw=len(output);rawsize=(len(body)+511)&~511
        struct.pack_into('<8sIIIIIIHHI',output,opt+224+40*i,section.encode().ljust(8,b'\0'),len(body),rva,rawsize,raw,0,0,0,0,flags)
        output.extend(body+bytes(rawsize-len(body)))
    exe=ROOT/f'samples/stage10-{name}.exe';exe.write_bytes(output)
    meta=dict(name=name,sha256=hashlib.sha256(output).hexdigest(),size=len(output),output_size=output_size,all_code_and_input_authored_here=True,imports=names,operations=operations,dll_for=dll_for)
    (ROOT/f'evidence/stage10-{name}-source.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta

def run(name):
    meta=json.loads((ROOT/f'evidence/stage10-{name}-source.json').read_text())
    exe=ROOT/f'samples/stage10-{name}.exe';assert hashlib.sha256(exe.read_bytes()).hexdigest()==meta['sha256']
    target=GAME/f'ST10-{name}.exe';assert not target.exists()
    output=GAME/f'ST10-{name}.bin';assert not output.exists();shutil.copyfile(exe,target)
    env=os.environ.copy();env.update(WINEPREFIX=str(APP/'Resources'),WINEDEBUG='+loaddll,+seh',DYLD_FALLBACK_LIBRARY_PATH=str(APP/'SharedSupport/wine/lib'))
    command=[str(APP/'SharedSupport/wine/bin/wine32on64'),f'C:\\EmperorRotMK[ZeaS]\\ST10-{name}.exe']
    start=time.monotonic();result=dict(command=command,budget_seconds=20,probe_sha256=meta['sha256'])
    with (ROOT/f'evidence/stage10-{name}-wine.log').open('w') as log:
        child=subprocess.Popen(command,cwd=GAME,env=env,stdout=log,stderr=subprocess.STDOUT);result['pid']=child.pid
        try:child.wait(timeout=18)
        except subprocess.TimeoutExpired:
            result['deadline']=True;child.terminate()
            try:child.wait(timeout=1)
            except subprocess.TimeoutExpired:child.kill();child.wait(timeout=.5)
        result['returncode']=child.returncode
    if output.is_file():
        data=output.read_bytes();assert len(data)==meta['output_size']
        (ROOT/f'evidence/stage10-{name}-output.bin').write_bytes(data)
        result.update(output_size=len(data),output_sha256=hashlib.sha256(data).hexdigest())
    else:result['failure']='own bounded probe output missing'
    result['elapsed_seconds']=round(time.monotonic()-start,3)
    return result
