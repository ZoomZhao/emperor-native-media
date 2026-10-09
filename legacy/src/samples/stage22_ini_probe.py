import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
d=Data();filename=d.alloc(b'C:\\EmperorRotMK[ZeaS]\\Emperor.ini\0');ops=[];queries=[]
def ptr(s):return d.alloc(s+b'\0')if s is not None else 0
spec=[(b'Config',b'CDDrive',b'fallback',n)for n in [0,1,2,10]]+[(b'cOnFiG',b'rAm',b'x',n)for n in [1,2,3,4,5,10]]+[(b'Config',k,b'32',10)for k in [b'SoundFrags',b'Fatten',b'Missing']]+[(b'NoSection',b'NoKey',v,10)for v in [None,b'',b'abc   ',b'abc\t']]+[(b'Config',b'PlayIntroMovie',None,10)]+[(a,b,None,n)for a,b in [(None,b'unused'),(b'Config',None),(b'Missing',None)]for n in [1,2,5,128]]
for section,key,fallback,size in spec:
 out=d.alloc(bytes([0xa5])*128);result=d.alloc(bytes(8));args=[ptr(section),ptr(key),ptr(fallback),out,size,filename];ops.append(dict(name='GetPrivateProfileStringA',args=args,result=result));queries.append(dict(type='string',section=section.decode()if section is not None else None,key=key.decode()if key is not None else None,fallback=fallback.decode()if fallback is not None else None,size=size,out=out,result=result))
for section,key,fallback in [(b'config',b'ram',77),(b'CONFIG',b'CPU',77),(b'Config',b'CDDrive',77),(b'Config',b'PlayIntroMovie',77),(b'Config',b'Missing',0xfffffffd),(b'Missing',b'RAM',0x12345678)]:
 result=d.alloc(bytes(8));ops.append(dict(name='GetPrivateProfileIntA',args=[ptr(section),ptr(key),fallback,filename],result=result));queries.append(dict(type='int',section=section.decode(),key=key.decode(),fallback=fallback,result=result))
build('ini22',d,ops);s=run('ini22');body=(ROOT/'evidence/stage10-ini22-output.bin').read_bytes()
for q in queries:
 q['return_value'],q['last_error']=struct.unpack_from('<II',body,d.offset(q['result']))
 if q['type']=='string':q['entire_128_byte_output_hex']=body[d.offset(q['out']):d.offset(q['out'])+128].hex()
s['queries']=queries;(ROOT/'evidence/stage22-ini-Wine.json').write_text(json.dumps(s,indent=2)+'\n');assert s['returncode']==0;print(json.dumps(dict(query_count=len(queries),returncode=s['returncode'],elapsed=s['elapsed_seconds'])))
