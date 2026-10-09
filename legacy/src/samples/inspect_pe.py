#!/usr/bin/env python3
"""Read-only PE32 metadata/import inspection. No loader, no execution, stdlib only."""
import sys, struct, hashlib, json, math, collections
from pathlib import Path

def inspect(path):
    data = Path(path).read_bytes()
    def unpack(fmt, off):
        return struct.unpack_from('<' + fmt, data, off)
    def cstr(off):
        end = data.index(b'\0', off)
        return data[off:end].decode('ascii', 'replace')
    if data[:2] != b'MZ':
        raise ValueError('not MZ')
    pe = unpack('I', 0x3c)[0]
    if data[pe:pe+4] != b'PE\0\0':
        raise ValueError('not PE')
    machine, nsec, timestamp, _, _, optsize, characteristics = unpack('HHIIIHH', pe+4)
    opt = pe+24
    magic = unpack('H', opt)[0]
    if magic != 0x10b:
        raise ValueError('only PE32 supported by this inspector')
    entry, base = unpack('I', opt+16)[0], unpack('I', opt+28)[0]
    sections = []
    for i in range(nsec):
        off = opt+optsize+40*i
        name = data[off:off+8].rstrip(b'\0').decode('ascii','replace')
        vsize, va, rawsize, raw, _, _, _, _, flags = unpack('IIIIIIHHI', off+8)
        chunk = data[raw:raw+rawsize]
        counts = collections.Counter(chunk)
        entropy = -sum((n/len(chunk))*math.log2(n/len(chunk)) for n in counts.values()) if chunk else 0
        sections.append(dict(name=name,rva=hex(va),virtual_size=vsize,raw_offset=raw,raw_size=rawsize,flags=hex(flags),entropy=round(entropy,3)))
    def rvaoff(rva):
        if rva < unpack('I', opt+60)[0]:
            return rva
        for sec in sections:
            va = int(sec['rva'],16)
            if va <= rva < va+max(sec['virtual_size'],sec['raw_size']):
                delta=rva-va
                if delta >= sec['raw_size']:
                    raise ValueError('RVA has no file backing')
                return sec['raw_offset']+delta
        raise ValueError('unmapped RVA '+hex(rva))
    names = ['export','import','resource','exception','certificate','relocation','debug','architecture','globalptr','tls','load_config','bound_import','iat','delay_import','clr','reserved']
    dirs = {}
    for i in range(min(16,unpack('I',opt+92)[0])):
        rva,size=unpack('II',opt+96+8*i)
        if rva or size:
            dirs[names[i]]=dict(rva=hex(rva),size=size)
    imports=[]
    if 'import' in dirs:
        off = rvaoff(int(dirs['import']['rva'],16))
        for i in range(4096):
            ilt,stamp,chain,name,iat=unpack('IIIII',off+20*i)
            if not any((ilt,stamp,chain,name,iat)):
                break
            symbols=[]
            thunk=rvaoff(ilt or iat)
            for j in range(65536):
                val=unpack('I',thunk+4*j)[0]
                if not val:
                    break
                sym=dict(iat_va=hex(base+iat+4*j))
                if val&0x80000000:
                    sym['ordinal']=val&0xffff
                else:
                    s=rvaoff(val)
                    sym.update(hint=unpack('H',s)[0],name=cstr(s+2))
                symbols.append(sym)
            imports.append(dict(dll=cstr(rvaoff(name)),symbols=symbols))
    exports=[]
    if 'export' in dirs:
        off=rvaoff(int(dirs['export']['rva'],16))
        fields=unpack('IIHHIIIIIII',off)
        ord_base,nfunc,nnames,functions,enames,ordinals=fields[5:]
        for i in range(nnames):
            s=unpack('I',rvaoff(enames)+4*i)[0]
            ordinal=unpack('H',rvaoff(ordinals)+2*i)[0]
            addr=unpack('I',rvaoff(functions)+4*ordinal)[0]
            exports.append(dict(name=cstr(rvaoff(s)),ordinal=ord_base+ordinal,rva=hex(addr)))
    reloc_count=0
    if 'relocation' in dirs:
        start=rvaoff(int(dirs['relocation']['rva'],16))
        off=start
        end=start+dirs['relocation']['size']
        while off+8<=end:
            va,size=unpack('II',off)
            if size<8 or off+size>end: break
            for e in unpack(str((size-8)//2)+'H',off+8):
                if e>>12: reloc_count+=1
            off+=size
    image_end=max(s['raw_offset']+s['raw_size'] for s in sections)
    return dict(path=str(Path(path).resolve()),size=len(data),sha256=hashlib.sha256(data).hexdigest(),machine=hex(machine),pe_magic=hex(magic),timestamp=timestamp,characteristics=hex(characteristics),entry_rva=hex(entry),image_base=hex(base),image_size=unpack('I',opt+56)[0],subsystem=unpack('H',opt+68)[0],dll_characteristics=hex(unpack('H',opt+70)[0]),sections=sections,directories=dirs,relocation_entries=reloc_count,overlay_bytes=max(0,len(data)-image_end),imports=imports,import_count=sum(len(d['symbols']) for d in imports),exports=exports)

if __name__=='__main__':
    json.dump([inspect(p) for p in sys.argv[1:]],sys.stdout,indent=2,ensure_ascii=False)
    print()
