#!/usr/bin/env python3
"""Tiny bounded COFF->PE builder for our one self-contained i386 sample.

No external linker/dependencies and no original program loader. Deliberately
rejects imports, unsupported relocation types, and all undefined symbols.
"""
from pathlib import Path
import struct, json, collections, hashlib

ROOT=Path(__file__).resolve().parents[1]
BASE=0x400000
def align(v,a): return (v+a-1)//a*a
def write_pe(path,sections,entry,relocations,symbols,max_bytes=16384):
    groups=collections.defaultdict(list)
    for rva in relocations: groups[rva&~4095].append(0x3000|(rva&4095))
    rel=bytearray()
    for page,entries in sorted(groups.items()):
        if len(entries)%2: entries.append(0)
        rel+=struct.pack('<II',page,8+len(entries)*2)+struct.pack('<'+'H'*len(entries),*entries)
    reloc_rva=align(max(s['rva']+len(s['data']) for s in sections),4096)
    sections=[*sections,dict(name='.reloc',rva=reloc_rva,data=rel,flags=0x42000040)]
    n=len(sections); headersize=align(128+24+224+40*n,512)
    image=bytearray(headersize)
    image[:2]=b'MZ'; struct.pack_into('<I',image,0x3c,128)
    image[128:132]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',image,132,0x14c,n,0,0,0,224,0x102)
    opt=152
    struct.pack_into('<HBBIIIIII',image,opt,0x10b,1,0,sum(align(len(s['data']),512) for s in sections if s['flags']&0x20),sum(align(len(s['data']),512) for s in sections if not s['flags']&0x20),0,entry,sections[0]['rva'],next(s['rva'] for s in sections if not s['flags']&0x20))
    struct.pack_into('<III',image,opt+28,BASE,4096,512)
    struct.pack_into('<HHHHHH',image,opt+40,4,0,0,0,4,0)
    struct.pack_into('<IIIIHHIIIIII',image,opt+52,0,align(reloc_rva+len(rel),4096),headersize,0,3,0,0x100000,4096,0x100000,4096,0,16)
    struct.pack_into('<II',image,opt+96+8*5,reloc_rva,len(rel))
    for i,s in enumerate(sections):
        raw=align(len(image),512); size=align(len(s['data']),512)
        image+=bytes(raw-len(image))+s['data']+bytes(size-len(s['data']))
        off=opt+224+40*i
        struct.pack_into('<8sIIIIIIHHI',image,off,s['name'].encode(),len(s['data']),s['rva'],size,raw,0,0,0,0,s['flags'])
    assert len(image)<=max_bytes, 'Input exceeded explicit bounded maximum'
    path.write_bytes(image)
    (path.with_suffix('.map.json')).write_text(json.dumps(dict(base=BASE,entry=BASE+entry,symbols=symbols,relocations=[hex(BASE+r) for r in relocations],sha256=hashlib.sha256(image).hexdigest()),indent=2))

def from_coff(obj,path):
    b=obj.read_bytes()
    def get(fmt,off): return struct.unpack_from('<'+fmt,b,off)
    machine,nsec,_,symptr,nsyms,optsize,_=get('HHIIIHH',0)
    assert machine==0x14c and optsize==0
    stringtable=symptr+nsyms*18
    def name(raw):
        if raw[:4]==bytes(4):
            off=int.from_bytes(raw[4:],'little')+stringtable
            return b[off:b.index(0,off)].decode()
        return raw.rstrip(b'\0').decode()
    symbols={}; indexed={}; i=0
    while i<nsyms:
        off=symptr+i*18; value,sec,typ,storage,naux=get('IhHBB',off+8)
        indexed[i]=dict(name=name(b[off:off+8]),value=value,sec=sec,storage=storage)
        i+=1+naux
    secs={}; output=[]; rva=4096
    for i in range(nsec):
        off=20+i*40
        nm=b[off:off+8].rstrip(b'\0').decode()
        _,_,size,ptr,relptr,_,nrel,_,flags=get('IIIIIIHHI',off+8)
        if nm not in ('.text','.data','.rdata','.bss'): continue
        if not size: continue
        data=bytearray(b[ptr:ptr+size] if ptr else bytes(size))
        s=dict(name=nm,rva=rva,data=data,flags=(flags&~0x00f00000)|0x00300000,relptr=relptr,nrel=nrel)
        secs[i+1]=s; output.append(s); rva=align(rva+size,4096)
    relocations=[]
    for index,sym in indexed.items():
        if sym['sec'] in secs: symbols[sym['name']]=BASE+secs[sym['sec']]['rva']+sym['value']
    for s in output:
        for i in range(s['nrel']):
            off,symidx,typ=get('IIH',s['relptr']+10*i)
            sym=indexed[symidx]
            assert sym['sec'] in secs,'Undefined or unsupported symbol '+str(sym)
            addr=BASE+secs[sym['sec']]['rva']+sym['value']
            orig=int.from_bytes(s['data'][off:off+4],'little')
            if typ==6: value=orig+addr; relocations.append(s['rva']+off)
            elif typ==7: value=orig+addr-BASE
            elif typ==20: value=orig+addr-(BASE+s['rva']+off+4)
            else: raise ValueError('Unsupported relocation '+str(typ))
            s['data'][off:off+4]=(value&0xffffffff).to_bytes(4,'little')
    write_pe(path,output,symbols['_guest_run']-BASE,relocations,symbols)
    return symbols

if __name__=='__main__':
    symbols=from_coff(ROOT/'samples'/'guest32.obj',ROOT/'samples'/'guest32.exe')
    run=ROOT/'runs'/'guest';run.mkdir(parents=True,exist_ok=True)
    aliases=[]
    for name,addr in symbols.items():
        if name.startswith('_guest_'): aliases.append('loc_'+format(addr,'X')+','+name[1:])
    (run/'global_aliases.sci').write_text('\n'.join(aliases)+'\n')
    (run/'SR.cfg').write_text('esp_dword_aligned=yes\nebp_dword_aligned=no\n')
    (run/'extern.llinc').write_text('')
    (run/'macros.llinc').write_text('')
    print('Authored sample PE32 ready; no original EXE copied or executed.')
