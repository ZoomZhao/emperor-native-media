import hashlib,json,re,struct,subprocess
from pathlib import Path
from auto_i386_analysis import PE
R=Path(__file__).resolve().parents[1]
path=R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/Binkw32.dll'
p=PE(path);digest=hashlib.sha256(p.data).hexdigest()
source=Path('<LOCAL_MACHINE_PATH>]/Binkw32.dll')
source_digest=hashlib.sha256(source.read_bytes()).hexdigest();assert source_digest==digest
manifest=dict(original_path=str(source),copy_path=str(path),sha256=digest,bytes=len(p.data),source_and_copy_equal=True,read_only=True,executed=False)
(R/'evidence/stage41-Bink-input.readonly.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2))
off=struct.unpack_from('<I',p.data,0x3c)[0]+24;entry=p.base+struct.unpack_from('<I',p.data,off+16)[0]
er,es=struct.unpack_from('<II',p.data,off+96);d=p.read(p.base+er,40);nfunc,nnames,fa,na,oa=struct.unpack_from('<IIIII',d,20);exports=[]
for i in range(nnames):
 nr=struct.unpack('<I',p.read(p.base+na+4*i,4))[0];name=p.read(p.base+nr,128).split(b'\0')[0].decode();ordinal=struct.unpack('<H',p.read(p.base+oa+2*i,2))[0];fr=struct.unpack('<I',p.read(p.base+fa+ordinal*4,4))[0];exports.append(dict(name=name,va=hex(p.base+fr)))
for flag,label in [('-d','text'),('-p','headers')]:
 result=subprocess.run(['<LOCAL_MACHINE_PATH>',flag,str(path)],capture_output=True,timeout=20);assert result.returncode==0
 (R/f'evidence/stage41-Bink-{label}.readonly.log').write_bytes(result.stdout)
selected=[q for q in exports if any(s in q['name']for s in ['SetSoundSystem','OpenMiles','BinkOpen@'])]
lines=(R/'evidence/stage41-Bink-text.readonly.log').read_text().splitlines()
for q in selected:
 lo=int(q['va'],16);hi=lo+384
 out=[l for l in lines if(m:=re.match(r'^\s*([0-9a-f]+):',l))and lo<=int(m[1],16)<hi]
 (R/f'evidence/stage41-Bink-{q["name"]}.readonly.log').write_text('\n'.join(out)+'\n')
lines=(R/'evidence/stage16-original-text.disassembly.log').read_text().splitlines()
for lo,hi,name in [(0x41fed0,0x4203e0,'movie-caller'),(0x762c80,0x762e50,'stream-caller')]:
 out=[l for l in lines if(m:=re.match(r'^\s*([0-9a-f]+):',l))and lo<=int(m[1],16)<hi]
 (R/f'evidence/stage41-{name}.readonly.log').write_text('\n'.join(out)+'\n')
e=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
strings={hex(a):e.read(a,128).split(b'\0')[0].decode(errors='replace')for a in [8563804,8563776]}
out=dict(path=str(path),bytes=len(p.data),sha256=digest,machine='i386',base=hex(p.base),entry=hex(entry),selected_exports=selected,original_startup_movie_strings=strings,original_Bink_DLL_executed=False,input_manifest=manifest)
(R/'evidence/stage41-Bink-static.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
