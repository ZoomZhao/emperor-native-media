"""Read fixed DLL data/disassembly; no original sample execution."""
import hashlib,json,re
from pathlib import Path
from auto_i386_analysis import PE
R=Path(__file__).resolve().parents[1]
p=PE(R/'runs/stage36-mss-prefix/drive_c/MSS36StartupProbe/mss32.dll')
assert hashlib.sha256(p.data).hexdigest()=='f09e6024ad35eb6b7b46982efb998df26d162391b26fcf56519a4c23fcbc7015'
strings={}
for a in [0x2113e2e0,0x2113e2a8,0x211413c8,0x21141548,0x21141664,0x21141654,0x21141644,0x21141630,0x2114161c]:
 strings[hex(a)]=p.read(a,256).split(b'\0')[0].decode(errors='replace')
lines=(R/'evidence/stage25-miles-text-readonly.stdout.log').read_text().splitlines()
windows=[(0x2111299b,0x21112a00,'driver-format'),(0x21112d07,0x21112daf,'driver-pool'),(0x21113225,0x211132a1,'driver-processor-defaults'),(0x21114ef0,0x21115520,'sample-processors'),(0x21115520,0x21115720,'driver-processors'),(0x21113b60,0x21113fc0,'sample-properties'),(0x211138f0,0x21113a90,'sample-lifecycle'),(0x2112b8d0,0x2112b960,'3D-enumeration-core')]
for lo,hi,name in windows:
 chosen=[line for line in lines if(m:=re.match(r'^\s*([0-9a-f]+):',line))and lo<=int(m[1],16)<hi]
 (R/f'evidence/stage39-{name}.readonly.log').write_text('\n'.join(chosen)+'\n')
out=dict(DLL_sha256=hashlib.sha256(p.data).hexdigest(),original_sample_code_executed=False,strings=strings,pan_table=list(p.read(555008880,128)),volume_preference_index=(555027444-0x21150bec)//4,pool_capacity_preference_index=1,default_output_format=3,default_filter_provider=0,windows=[dict(low=hex(a),high=hex(b),name=n)for a,b,n in windows])
(R/'evidence/stage39-static.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
