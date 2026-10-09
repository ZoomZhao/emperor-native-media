# -*- coding: utf-8 -*-
import hashlib,json,re,struct,subprocess
from pathlib import Path
from auto_i386_analysis import PE
R=Path(__file__).resolve().parents[1];manifest=json.loads((R/'evidence/stage36-provider-inputs.readonly.json').read_text());meta=[]
for row in manifest:
 if not row['name'].lower().endswith('.m3d'):continue
 path=Path(row['copy_path']);p=PE(path);assert hashlib.sha256(p.data).hexdigest()==row['sha256']
 peoff=struct.unpack_from('<I',p.data,0x3c)[0];opt=peoff+24;entry=p.base+struct.unpack_from('<I',p.data,opt+16)[0];er,es=struct.unpack_from('<II',p.data,opt+96);d=p.read(p.base+er,40) if er else bytes(40);nfunc,nnames,fa,na,oa=struct.unpack_from('<IIIII',d,20);exports=[]
 for i in range(nnames):
  nr=struct.unpack('<I',p.read(p.base+na+4*i,4))[0];name=p.read(p.base+nr,128).split(b'\0')[0].decode();ordinal=struct.unpack('<H',p.read(p.base+oa+2*i,2))[0];fr=struct.unpack('<I',p.read(p.base+fa+ordinal*4,4))[0];exports.append(dict(name=name,va=hex(p.base+fr)))
 strings=[]
 for sec in p.sections:
  data=p.data[sec['rawptr']:sec['rawptr']+sec['rawsize']]
  for m in re.finditer(rb'[\x20-\x7e]{4,}\x00',data):
   txt=m[0][:-1].decode()
   if any(w.lower() in txt.lower()for w in ['MSS','RIB','3D','Name','provider','DirectSound','RSX']):strings.append(dict(va=hex(sec['va']+m.start()),text=txt))
 for flag,label in [('-d','text'),('-p','headers')]:
  result=subprocess.run(['<LOCAL_MACHINE_PATH>',flag,str(path)],capture_output=True,timeout=20);assert result.returncode==0
  (R/f'evidence/stage40-{row["name"]}-{label}.readonly.log').write_bytes(result.stdout)
 meta.append(dict(name=row['name'],sha256=row['sha256'],base=hex(p.base),entry=hex(entry),exports=exports,strings=strings,executed=False))
m=PE(Path(manifest[0]['copy_path']));e=PE(Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'))
strings={}
for pe,addrs,tag in [(m,[0x2113e254,0x21142770,0x2114048c],'MSS'),(e,[0x88a380,0x88a384,0x88a388,0x88a38c,0x88a390,0x88a39c,0x88a3a0,0x88a3a4,0x88a3a8,0x88a3b4,0x88a3b8],'Emperor')]:
 for a in addrs:strings[tag+':'+hex(a)]=pe.read(a,256).split(b'\0')[0].decode(errors='replace')
windows=[('evidence/stage25-miles-text-readonly.stdout.log',[(0x21101000,0x21101280,'startup-registry'),(0x21129d40,0x2112b620,'RIB-registry'),(0x2110d630,0x2110d850,'3D-wrappers'),(0x2112b8d0,0x2112c380,'3D-core')]),('evidence/stage16-original-text.disassembly.log',[(0x76307d,0x76349b,'Emperor-3D-choice'),(0x5c3470,0x5c35d0,'Emperor-audio-callers')])]
for source,wins in windows:
 out={name:[]for _,_,name in wins}
 for line in (R/source).read_text().splitlines():
  match=re.match(r'^\s*([0-9a-f]+):',line)
  if match:
   addr=int(match[1],16)
   for lo,hi,name in wins:
    if lo<=addr<hi:out[name].append(line)
 for name,lines in out.items():(R/f'evidence/stage40-{name}.readonly.log').write_text('\n'.join(lines)+'\n')
result=dict(providers=meta,strings=strings,original_DLL_exports_executed=False)
(R/'evidence/stage40-static.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));print(json.dumps(dict(providers=[{k:p[k]for k in ['name','base','entry','exports']}for p in meta],strings=strings),ensure_ascii=False))
