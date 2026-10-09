"""Authored API calls verify rejection and stdcall, no original DLL execution."""
import copy,hashlib,json,struct
from checkpoint_startup35 import ContractRuntime,R,x,Boundary
def main():
 r=ContractRuntime('ST9BASE');core=0x10c1f60;sp=0x700ff000;pathp=0x20003000;rows=[];return_pc=0x401234
 def invoke(name,args):
  r.uc.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),return_pc,*args));r.uc.reg_write(x.UC_X86_REG_ESP,sp);r.api('ijl10.dll',name);assert r.uc.reg_read(x.UC_X86_REG_ESP)==sp+4+len(args)*4 and r.uc.reg_read(x.UC_X86_REG_EIP)==return_pc and r.uc.reg_read(x.UC_X86_REG_EAX)==0
 def refuses(label,name,args):
  before=bytes(r.uc.mem_read(core,20032));count=len(r.jpeg_contract_checks35);sessions=copy.deepcopy(r.jpeg_sessions)
  try:invoke(name,args)
  except Boundary as error:
   assert bytes(r.uc.mem_read(core,20032))==before and len(r.jpeg_contract_checks35)==count and r.jpeg_sessions==sessions
   rows.append(dict(test=label,boundary=error.details,core_and_lifecycle_unchanged=True));return
  raise AssertionError('unsupported input was accepted: '+label)
 try:
  r.uc.mem_write(core,bytes(20032));invoke('ordinal:2',[core]);refuses('duplicate Init','ordinal:2',[core]);refuses('Read(2) before header','ordinal:4',[core,2]);refuses('unimplemented Read mode','ordinal:4',[core,5]);refuses('Free before decode','ordinal:3',[core])
  r.uc.mem_write(pathp,b'data\\CHINA_MapOfChina01.jpg\0');r.word(core+32,pathp);r.word(core+36,0x20004000);refuses('memory input buffer','ordinal:4',[core,0]);r.word(core+36,0);invoke('ordinal:4',[core,0]);session=r.jpeg_sessions[core];w=session['metadata']['width'];h=session['metadata']['height'];owner=r.allocate(next(iter(r.heaps)),8,8)
  public=r.defaults();public[8]=pathp;public[1]=owner;public[2:7]=[w,h,0,3,2];public[11:15]=[w,h,3,3];r.uc.mem_write(core,struct.pack('<22I',*public));refuses('mapped but undersized live allocation','ordinal:4',[core,2]);r.word(core+12,(-h)&0xffffffff);refuses('negative DIB height','ordinal:4',[core,2]);r.word(core+12,h);r.word(core+16,1);refuses('unsupported row padding','ordinal:4',[core,2]);r.word(core+16,0);r.word(core+24,4);refuses('unsupported grayscale DIB contract','ordinal:4',[core,2]);r.word(core+24,2)
  owner=r.allocate(next(iter(r.heaps)),8,w*h*3);r.word(core+4,owner);invoke('ordinal:4',[core,2]);invoke('ordinal:3',[core]);assert not r.jpeg_sessions
  out=dict(refusals=rows,all_unknown_inputs_refused_without_guest_core_or_session_mutation=True,successful_full_sequence_with_non_original_return_PC=hex(return_pc),stdcall_EAX_ESP_EIP_verified=True,original_DLL_executed=False,guest_image_instructions_executed=False,source='self-authored API invocation, legal hashed JPEG data only',success_call_count=len(r.jpeg_contract_checks35));(R/'evidence/stage35-contract-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(refusals=len(rows),full_sequence_and_stdcall_passed=True)))
 finally:r.dispose()
if __name__=='__main__':main()
