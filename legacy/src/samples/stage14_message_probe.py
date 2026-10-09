#!/usr/bin/env python3
"""Two task-authored fixed-address i386 DLLs call the actual Wine USER32 API."""
import argparse,hashlib,json,shutil,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run,GAME
NAME='MSG14'
def dll(label,base,symbol):
 # No DllMain, TLS, relocations or imported third-party task code. Distinct
 # bases avoid relocations; USER32 IAT is populated by the existing Wine loader.
 idata=bytearray(512);struct.pack_into('<5I',idata,0,0x3040,0,0,0x30a0,0x3080)
 for at in [0x40,0x80]:struct.pack_into('<I',idata,at,0x30c0)
 idata[0xa0:0xad]=b'USER32.dll\0';idata[0xc0:0xda]=b'\0\0RegisterWindowMessageA\0'
 code=b'\xff\x74\x24\x04\xff\x15'+struct.pack('<I',base+0x3080)+b'\xc2\x04\x00'
 edata=bytearray(512);struct.pack_into('<IIHHIIIIIII',edata,0,0,0,0,0,0x4080,1,1,1,0x4040,0x4044,0x4048)
 struct.pack_into('<IIH',edata,0x40,0x1000,0x40a0,0);name=(label+'.dll\0').encode();edata[0x80:0x80+len(name)]=name
 sym=(symbol+'\0').encode();edata[0xa0:0xa0+len(sym)]=sym
 chunks=[('.text',0x1000,code,0x60000020),('.rdata',0x2000,bytes(512),0x40000040),('.idata',0x3000,idata,0xc0000040),('.edata',0x4000,edata,0x40000040)]
 h=bytearray(1024);h[:2]=b'MZ';struct.pack_into('<I',h,0x3c,0x80);h[0x80:0x84]=b'PE\0\0';struct.pack_into('<HHIIIHH',h,0x84,0x14c,4,0,0,0,224,0x210f)
 o=0x98;struct.pack_into('<HBB9I6H4I2H6I',h,o,0x10b,1,0,512,1536,0,0,0x1000,0x2000,base,0x1000,0x200,4,0,0,0,4,0,0,0x5000,1024,0,2,0,0x100000,0x1000,0x100000,0x1000,0,16)
 struct.pack_into('<II',h,o+96,0x4000,0x100);struct.pack_into('<II',h,o+104,0x3000,40);struct.pack_into('<II',h,o+96+12*8,0x3080,8)
 out=bytearray(h)
 for i,(section,rva,body,flags) in enumerate(chunks):
  raw=len(out);size=(len(body)+511)&~511;struct.pack_into('<8sIIIIIIHHI',out,o+224+i*40,section.encode().ljust(8,b'\0'),len(body),rva,size,raw,0,0,0,0,flags);out.extend(body+bytes(size-len(body)))
 p=ROOT/'samples'/(label+'.dll');p.write_bytes(out);return dict(path=str(p),sha256=hashlib.sha256(out).hexdigest(),base=hex(base),export=symbol,all_code_authored_here=True)
def make():
 modules=[dll('ST14A',0x10000000,'RegisterFromA'),dll('ST14B',0x11000000,'RegisterFromB')]
 d=Data();d.alloc(b'MSG14v01');ops=[];cases=[]
 rows=[('RegisterFromA','ST14_Shared'),('RegisterFromB','ST14_Shared'),('RegisterWindowMessageA','st14_shared'),('RegisterFromB','ST14_Other'),('RegisterFromA','ST14_OTHER'),('RegisterFromA',''),('RegisterFromB',''),('RegisterWindowMessageA','commctrl_DragListMsg'),('RegisterFromB','COMMCTRL_DRAGLISTMSG')]
 for function,text in rows:
  p=d.alloc(text.encode()+b'\0');out=d.alloc(bytes(8));ops.append(dict(name=function,args=[p],result=out));cases.append(dict(module=function,text=text,offset=d.offset(out)))
 build(NAME,d,ops,dll_for={'RegisterFromA':'ST14A.dll','RegisterFromB':'ST14B.dll','RegisterWindowMessageA':'USER32.dll'})
 (ROOT/'evidence/stage14-message-cases.json').write_text(json.dumps(dict(modules=modules,cases=cases),indent=2)+'\n')
def capture():
 record=json.loads((ROOT/'evidence/stage14-message-cases.json').read_text())
 for m in record['modules']:
  p=Path(m['path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];target=GAME/p.name;assert not target.exists();shutil.copyfile(p,target)
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in record['cases']:
  value,error=struct.unpack_from('<II',raw,c['offset']);rows.append(dict(c,return_value=value,last_error=error))
 result['rows']=rows;result['modules']=record['modules'];result['checks']=dict(shared_across_two_DLLs_and_main=len({r['return_value'] for r in rows[:3]})==1,case_fold_across_modules=rows[3]['return_value']==rows[4]['return_value'],empty_error=all(r['return_value']==0 and r['last_error']==87 for r in rows[5:7]),registered_range=all(0xc000<=r['return_value']<=0xffff for r in rows if r['text']),distinct_names=rows[0]['return_value']!=rows[3]['return_value'],valid_preserves_last_error=all(r['last_error']==0x13579 for r in rows if r['text']))
 assert all(result['checks'].values());(ROOT/'evidence/stage14-message-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result['checks']))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
