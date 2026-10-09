"""Prepare, never execute, an authored PE with fixed original MSS DLL inputs."""
import hashlib,json,struct,sys
from pathlib import Path
from stage10_probe_support import Data,build,ROOT,BASE
def main():
 d=Data();ops=[]
 def op(name,args):
  q=dict(name=name,args=args,result=d.alloc(bytes(8)));ops.append(q);return q['result']
 path=d.alloc(b'C:\\MSS36StartupProbe\\mss32.dll\0');directory=d.alloc(b'C:\\MSS36StartupProbe\\\0');module=op('LoadLibraryA',[path]);fn={}
 for name in ['_AIL_set_redist_directory@4','_AIL_get_preference@4','_AIL_set_preference@8','_AIL_startup@0','_AIL_shutdown@0','_AIL_waveOutOpen@16','_AIL_waveOutClose@4']:
  string=d.alloc(name.encode()+b'\0');fn[name]=op('GetProcAddress',[dict(load=module),string])
 def call(name,args):
  q=dict(name='MSS_dynamic',args=args,result=d.alloc(bytes(8)),callee_pointer_slot=fn[name],original_export=name);ops.append(q);return q['result']
 call('_AIL_set_redist_directory@4',[directory]);call('_AIL_get_preference@4',[42]);call('_AIL_set_preference@8',[42,32]);call('_AIL_get_preference@4',[42]);call('_AIL_startup@0',[]);call('_AIL_get_preference@4',[42]);call('_AIL_startup@0',[]);call('_AIL_set_preference@8',[40,262144]);call('_AIL_set_preference@8',[15,0]);driver=d.alloc(bytes(4));wave=d.alloc(struct.pack('<HHIIHH',1,2,22050,88200,4,16));device_result=call('_AIL_waveOutOpen@16',[driver,0,0xffffffff,wave]);op('Sleep',[200]);call('_AIL_waveOutClose@4',[dict(load=driver)]);call('_AIL_shutdown@0',[]);call('_AIL_startup@0',[]);call('_AIL_get_preference@4',[42]);call('_AIL_shutdown@0',[]);op('FreeLibrary',[dict(load=module)])
 # Emit own dynamic calls into the proven authored PE writer. The DLLs are not
 # loaded during generation. All zero-return preconditions must branch to exit
 # before any indirect call. Conditional device close skips NULL driver.
 imported=list(dict.fromkeys([q['name']for q in ops if q['name']!='MSS_dynamic']));idata=bytearray(4096);pos=128;ilt=pos;pos+=4*(len(imported)+7);iat=pos;pos+=4*(len(imported)+7);allnames=imported+['SetLastError','GetLastError','CreateFileA','WriteFile','CloseHandle','ExitProcess'];allnames=list(dict.fromkeys(allnames));namepos=pos;dll=b'KERNEL32.dll\0';idata[pos:pos+len(dll)]=dll;pos=(pos+len(dll)+1)&~1
 struct.pack_into('<5I',idata,0,0x200000+ilt,0,0,0x200000+namepos,0x200000+iat);slots={}
 for i,name in enumerate(allnames):
  rec=struct.pack('<H',0)+name.encode()+b'\0';idata[pos:pos+len(rec)]=rec
  for tab in [ilt,iat]:struct.pack_into('<I',idata,tab+4*i,0x200000+pos)
  slots[name]=BASE+0x200000+iat+4*i;pos=(pos+len(rec)+1)&~1
 code=bytearray();abort_jumps=[]
 def push(v):code.extend(b'\xff\x35'+struct.pack('<I',v['load']))if isinstance(v,dict)else code.extend(b'\x68'+struct.pack('<I',v&0xffffffff))
 def invoke(slot):code.extend(b'\xff\x15'+struct.pack('<I',slot))
 def zero_abort():code.extend(b'\x85\xc0\x0f\x84');abort_jumps.append(len(code));code.extend(bytes(4))
 for q in ops:
  skip_close=[]
  if q.get('original_export')=='_AIL_waveOutClose@4':
   for source,condition in [(device_result,0x85),(driver,0x84)]:
    code.extend(b'\xa1'+struct.pack('<I',source)+b'\x85\xc0\x0f'+bytes([condition]));skip_close.append(len(code));code.extend(bytes(4))
  for a in reversed(q['args']):push(a)
  invoke(q.get('callee_pointer_slot',slots.get(q['name'])));code.extend(b'\xa3'+struct.pack('<I',q['result']))
  if q['name']in ['LoadLibraryA','GetProcAddress']:zero_abort()
  invoke(slots['GetLastError']);code.extend(b'\xa3'+struct.pack('<I',q['result']+4))
  for at in skip_close:struct.pack_into('<i',code,at,len(code)-(at+4))
 output_size=len(d.data);written=d.alloc(bytes(4));failure=len(code)
 for at in abort_jumps:struct.pack_into('<i',code,at,failure-(at+4))
 for v in [0,0x80,2,0,0,0x40000000,BASE+0x10000]:push(v)
 invoke(slots['CreateFileA']);code.extend(b'\x89\xc3')
 for v in [0,written,output_size,BASE+0x20000]:push(v)
 code.extend(b'\x53');invoke(slots['WriteFile']);code.extend(b'\x53');invoke(slots['CloseHandle']);push(0);invoke(slots['ExitProcess'])
 assert len(code)<0xf000 and pos<len(idata)
 chunks=[('.text',0x1000,code,0x60000020),('.rdata',0x10000,b'C:\\MSS36StartupProbe\\result.bin\0',0x40000040),('.data',0x20000,d.data,0xc0000040),('.idata',0x200000,idata,0xc0000040)];header=bytearray(1024);header[:2]=b'MZ';struct.pack_into('<I',header,0x3c,0x80);header[0x80:0x84]=b'PE\0\0';struct.pack_into('<HHIIIHH',header,0x84,0x14c,4,0,0,0,224,0x10f);opt=0x98
 struct.pack_into('<HBB9I6H4I2H6I',header,opt,0x10b,1,0,(len(code)+511)&~511,(len(d.data)+511)&~511,0,0x1000,0x1000,0x20000,BASE,0x1000,0x200,4,0,0,0,4,0,0,0x201000,1024,0,3,0,0x100000,0x1000,0x100000,0x1000,0,16);struct.pack_into('<II',header,opt+104,0x200000,40);struct.pack_into('<II',header,opt+192,0x200000+iat,4*(len(allnames)+1));body=bytearray(header)
 for i,(name,rva,part,flags)in enumerate(chunks):
  raw=len(body);n=(len(part)+511)&~511;struct.pack_into('<8sIIIIIIHHI',body,opt+224+40*i,name.encode().ljust(8,b'\0'),len(part),rva,n,raw,0,0,0,0,flags);body.extend(part+bytes(n-len(part)))
 dest=ROOT/'runs/stage36-mss-pending';dest.mkdir(exist_ok=True);exe=dest/'mss36-probe.exe';exe.write_bytes(body);manifest=dict(executable=str(exe),executable_sha256=hashlib.sha256(body).hexdigest(),operations=ops,output_size=output_size,data_base=BASE+0x20000,driver_slot=driver,original_DLL_execution_pending_approval=True,scope='cold/repeated startup, preference reset, original PCM device open/close, shutdown and restart; no samples/game/audio assets/3D device open',fixed_input_rows=json.loads((ROOT/'evidence/stage36-provider-inputs.readonly.json').read_text()));(ROOT/'evidence/stage36-MSS-pending-probe.json').write_text(json.dumps(manifest,indent=2));print(json.dumps(dict(executable=str(exe),sha256=manifest['executable_sha256'],operations=len(ops),executed=False)))
if __name__=='__main__':main()
