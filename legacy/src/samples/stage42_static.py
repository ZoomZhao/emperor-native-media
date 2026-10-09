import hashlib,json,re,struct
from pathlib import Path
from auto_i386_analysis import PE
R=Path(__file__).resolve().parents[1];m=json.loads((R/'evidence/stage41-Bink-input.readonly.json').read_text());p=PE(m['copy_path']);assert hashlib.sha256(p.data).hexdigest()==m['sha256']
off=struct.unpack_from('<I',p.data,0x3c)[0]+24;er,es=struct.unpack_from('<II',p.data,off+96);d=p.read(p.base+er,40);nf,nn,fa,na,oa=struct.unpack_from('<IIIII',d,20);exports=[]
for i in range(nn):
 nr=struct.unpack('<I',p.read(p.base+na+4*i,4))[0];name=p.read(p.base+nr,128).split(b'\0')[0].decode();o=struct.unpack('<H',p.read(p.base+oa+2*i,2))[0];fr=struct.unpack('<I',p.read(p.base+fa+o*4,4))[0];exports.append(dict(name=name,va=p.base+fr))
names=['_BinkGetSummary@8','_BinkGetError@0','_BinkSetSoundSystem@8','_BinkOpenMiles@4'];wins=[]
for name in names:
 q=next(q for q in exports if q['name']==name);hi=min([v['va']for v in exports if v['va']>q['va']]+[q['va']+1024]);wins.append((q['va'],hi,name))
wins.extend([(0x3000a1d0,0x3000a290,'IO-open'),(0x30003d90,0x30003e60,'BinkOpen-file-failure')]);lines=(R/'evidence/stage41-Bink-text.readonly.log').read_text().splitlines()
for lo,hi,label in wins:
 out=[l for l in lines if(m:=re.match(r'^\s*([0-9a-f]+):',l))and lo<=int(m[1],16)<hi];(R/f'evidence/stage42-{label}.readonly.log').write_text('\n'.join(out)+'\n')
e=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');strings={hex(a):e.read(a,128).split(b'\0')[0].decode(errors='replace')for a in [0x815cdc,0x81623c,0x816234,0x81621c]};strings['Bink-file-open-error']=p.read(0x300474cc,128).split(b'\0')[0].decode()
out=dict(Bink_sha256=hashlib.sha256(p.data).hexdigest(),selected_exports=[dict(name=q['name'],va=hex(q['va']))for q in exports if q['name']in names],strings=strings,original_Bink_or_Miles_executed=False,official_sources=[dict(url='https://www.radgametools.com/binkfaq.htm',finding='Vendor states DLL versions are not interchangeable; this experiment retains the fixed legal DLL only for static evidence'),dict(url='https://www.radgametools.com/bnkdown.htm',finding='Vendor provides Mac arm64 player but current SDK is available to licensed customers; no public exact legacy callback ABI obtained; no player or SDK downloaded')])
(R/'evidence/stage42-static.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));print(json.dumps(out,ensure_ascii=False,indent=2))
