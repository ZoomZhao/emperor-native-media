import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
d=Data();section=d.alloc(b'Config\0');key=d.alloc(b'SoundFrags\0');filename=d.alloc(b'C:\\EmperorRotMK[ZeaS]\\Emperor.ini\0');fallback=d.alloc(b'32\0');out=d.alloc(bytes([0xa5])*10);result=d.alloc(bytes(8));op=dict(name='GetPrivateProfileStringA',args=[section,key,fallback,out,10,filename],result=result);build('soundini21',d,[op]);s=run('soundini21');body=(ROOT/'evidence/stage10-soundini21-output.bin').read_bytes();n,e=struct.unpack_from('<II',body,d.offset(result));b=body[d.offset(out):d.offset(out)+10];s.update(return_value=n,last_error=e,output_hex=b.hex(),all_passed=n==2 and b==b'32\0'+bytes([0xa5])*7);assert s['all_passed'];(ROOT/'evidence/stage21-sound-ini-Wine.json').write_text(json.dumps(s,indent=2)+'\n');print(json.dumps(s))
