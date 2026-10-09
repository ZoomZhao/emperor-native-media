#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
d=Data();section=d.alloc(b'Emperor of China\0');key=d.alloc(b'Net Name\0');filename=d.alloc(b'EOC.ini\0');default=d.alloc(b'ServerPlayer\0');trailing=d.alloc(b'abc   \0');operations=[]
for size,df in [(512,default),(5,default),(512,0),(512,trailing)]:
 out=d.alloc(bytes([0xa5])*512);result=d.alloc(bytes(8));operations.append(dict(name='GetPrivateProfileStringA',args=[section,key,df,out,size,filename],result=result,out=out,size=size))
build('profile19',d,operations);s=run('profile19');body=(ROOT/'evidence/stage10-profile19-output.bin').read_bytes();results=[]
for op in operations:
 at=d.offset(op['result']);n,err=struct.unpack_from('<II',body,at);at=d.offset(op['out']);results.append(dict(size=op['size'],default_null=op['args'][2]==0,return_value=n,last_error=err,output=body[at:at+min(32,op['size'])].hex(),tail_untouched=body[at+n+1:at+512]==bytes([0xa5])*(511-n)))
s['results']=results;(ROOT/'evidence/stage19-profile-Wine.json').write_text(json.dumps(s,indent=2)+'\n');print(json.dumps(s))
