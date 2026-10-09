"""Only own API glue/state with bytearray memory; no Uc or original instruction."""
import json,os,copy
from pathlib import Path
from stage51_environment import GuestEnvironment,EnvironmentMixin,PolicyError
from stage16_paths import NativePaths
from stage12_state import encode,decode
from checkpoint_startup51 import MenuRuntime,x,Boundary,u
from stage51_batch import claim,finish
R=Path(__file__).resolve().parents[1];events=[];host_env=dict(os.environ);host_cwd=os.getcwd()
class Memory:
    def __init__(self):self.body=bytearray(0x20000);self.regs={};self.call=bytes.fromhex('ff159c917a00')
    def reg_read(self,r):return self.regs.get(r,0)
    def mem_read(self,p,n):
        if p==0x77a027 and n==6:return self.call
        assert 0x1000<=p and p+n<=len(self.body)
        return bytes(self.body[p:p+n])
    def mem_write(self,p,b):assert 0x1000<=p and p+len(b)<=len(self.body);self.body[p:p+len(b)]=b
class Pattern:
    def read(self,p,n):assert(p,n)==(0x77a027,6);return bytes.fromhex('ff159c917a00')
class Tail:
    def api(self,dll,name):raise AssertionError('unexpected API delegation '+name)
class Probe(EnvironmentMixin,Tail):
    environment_write_permission51=u.UC_PROT_WRITE
    validate_hidden_environment_caller51=MenuRuntime.validate_hidden_environment_caller51
    def __init__(self):self.paths=NativePaths();self.uc=Memory();self.pe=Pattern();self.api_trace=[];self.last_error=0x13579;self.ensure_environment51()
    def readword(self,p):return int.from_bytes(self.uc.mem_read(p,4),'little')
    def args(self,n):return [self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)+4+4*i)for i in range(n)]
    def cstring(self,p):
        b=self.uc.mem_read(p,4096);assert b'\0'in b
        return b.split(b'\0',1)[0]
    def require_region(self,p,n,permissions,why):
        if not(0x1000<=p and p+n<=len(self.uc.body)):raise Boundary('own_probe_invalid_buffer',why=why)
    def environment_return_pc51(self):return self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
    def environment_boundary51(self,item,reason):raise Boundary('Guest_environment_policy51',**item,reason=reason)
    def ret(self,value,argc):
        sp=self.uc.reg_read(x.UC_X86_REG_ESP);self.uc.regs[x.UC_X86_REG_EIP]=self.readword(sp);self.uc.regs[x.UC_X86_REG_ESP]=sp+4+4*argc;self.uc.regs[x.UC_X86_REG_EAX]=value
    def invoke(self,api,name,value=None,capacity=64,pc=0x77a02d,invalid_output=False,wrong_hidden_pointer=False):
        bp=0x1800;sp=0x1200;self.uc.regs[x.UC_X86_REG_EBP]=bp;self.uc.regs[x.UC_X86_REG_ESP]=sp
        np=bp+8 if name.startswith(b'=')else 0x3000;vp=bp-264 if name.startswith(b'=')else 0x4000
        if wrong_hidden_pointer:np=0x3000
        self.uc.mem_write(np,name+b'\0')
        if value is not None:self.uc.mem_write(vp,value+b'\0')
        output=0x7000 if not invalid_output else 0
        args=[np,vp if value is not None else 0]if api=='SetEnvironmentVariableA'else [np,output,capacity]
        self.uc.mem_write(sp,b''.join(q.to_bytes(4,'little')for q in[pc]+args))
        self.api('KERNEL32.dll',api)
        assert self.uc.reg_read(x.UC_X86_REG_ESP)==sp+4+4*len(args)and self.uc.reg_read(x.UC_X86_REG_EIP)==pc
        return self.uc.reg_read(x.UC_X86_REG_EAX),self.last_error
def passed(name,**q):events.append(dict(test=name,**q))
def rejected(f,kind=Exception):
    try:f()
    except kind:return
    raise AssertionError('expected refusal')
p=Probe()
try:
    p.uc.mem_write(0x7000,b'\xa5'*64)
    assert p.invoke('GetEnvironmentVariableA',b'=C:')==(0,203)and p.uc.mem_read(0x7000,64)==b'\xa5'*64;passed('legacy empty guest environment preserves missing/error203 and untouched output')
    p.last_error=0x13579
    assert p.invoke('SetEnvironmentVariableA',b'=C:',b'C:\\Emperor')==(1,0x13579)
    assert p.invoke('GetEnvironmentVariableA',b'=c:')==(10,0x13579)and p.uc.mem_read(0x7000,11)==b'C:\\Emperor\0';passed('exact observed hidden drive setter, case-insensitive getter, stdcall32 stack and preserved LastError')
    assert p.invoke('GetEnvironmentVariableA',b'=C:',capacity=0,invalid_output=True)==(11,0x13579);passed('size query with null output returns required size including NUL')
    p.uc.mem_write(0x7000,b'\xa5'*64);assert p.invoke('GetEnvironmentVariableA',b'=C:',capacity=3)==(11,0x13579)and p.uc.mem_read(0x7000,64)==b'\xa5'*64;passed('undersized buffer reports required length without a write; Windows contents are undefined')
    rejected(lambda:p.invoke('GetEnvironmentVariableA',b'=C:',invalid_output=True),Boundary);passed('successful getter validates mapped writable buffer before write')
    assert p.invoke('SetEnvironmentVariableA',b'own_probe',b'VaLuE')==(1,0x13579)
    assert p.invoke('SetEnvironmentVariableA',b'OWN_PROBE',b'Changed')==(1,0x13579)
    assert p.invoke('GetEnvironmentVariableA',b'Own_Probe')==(7,0x13579)and p.uc.mem_read(0x7000,8)==b'Changed\0';passed('ordinary variables update in guest state and preserve value case')
    assert p.invoke('SetEnvironmentVariableA',b'OWN_PROBE',None)==(1,0x13579)
    assert p.invoke('GetEnvironmentVariableA',b'OWN_PROBE')==(0,203);passed('NULL deletion of an existing guest variable')
    for api_name,value in [(b'A=B',b'x'),(b'=D:',b'D:\\Other'),(b'bad name',b'x'),(b'EMPTY',b''),(b'MISSING',None)]:
        frozen=p.environment_state51;rejected(lambda:p.invoke('SetEnvironmentVariableA',api_name,value),Boundary);assert p.environment_state51==frozen
    passed('unsupported hidden drive/invalid names/unmeasured empty or absent deletion are atomic refusals')
    frozen=p.environment_state51
    for q in [dict(value=b'C:\\Emperor\\Save'),dict(value=b'C:\\Outside'),dict(value=b'C:\\Emperor',pc=0x401234),dict(value=b'C:\\Emperor',wrong_hidden_pointer=True)]:
        rejected(lambda:p.invoke('SetEnvironmentVariableA',b'=C:',**q),Boundary);assert p.environment_state51==frozen
    p.uc.call=b'\x90'*6;rejected(lambda:p.invoke('SetEnvironmentVariableA',b'=C:',b'C:\\Emperor'),Boundary);assert p.environment_state51==frozen;p.uc.call=bytes.fromhex('ff159c917a00');passed('hidden API requires exact reviewed caller, original instruction signature, original frame and actual native cwd')
    p.last_error=0x13579;text='自造值'.encode('gbk');assert p.invoke('SetEnvironmentVariableA',b'ACP_VALUE',text)==(1,0x13579)
    assert p.invoke('GetEnvironmentVariableA',b'acp_value')==(len(text),0x13579)and p.uc.mem_read(0x7000,len(text)+1)==text+b'\0';passed('ACP936 value retains its exact bytes')
    saved=p.environment_state51;restored=decode(encode(saved));p.invoke('SetEnvironmentVariableA',b'ACP_VALUE',b'other');p.environment_state51=restored;assert p.environment_state51==saved;passed('typed semantic snapshot restores ordinary and hidden values with native cwd validation')
    for mutation in ['version','drive','cwd','duplicate','order']:
        bad=copy.deepcopy(saved)
        if mutation=='version':bad['version']=2
        elif mutation=='drive':bad['drive_cwds']['D']=b'D:\\Other'
        elif mutation=='cwd':bad['drive_cwds']['C']=b'C:\\Outside'
        elif mutation=='duplicate':bad['variables'].append(bad['variables'][0])
        else:bad['variables']=list(reversed(bad['variables']))
        rejected(lambda:setattr(p,'environment_state51',bad),PolicyError);assert p.environment_state51==saved
    passed('malformed semantic versions/drive identities/duplicates/order rejected before replacing live state')
    expected=b'C:\\Emperor\\Save\\zoom\\ST9BASE.sav'
    for raw in [b'C:Save\\zoom\\ST9BASE.sav',b'c:Save/zoom/ST9BASE.sav',b'C:.\\Save\\zoom\\ST9BASE.sav',b'Save\\zoom\\ST9BASE.sav',b'C:\\Emperor\\Save\\zoom\\ST9BASE.sav',b'\\Emperor\\Save\\zoom\\ST9BASE.sav']:
        assert p.path49(raw)[1]==expected
    assert p.path49(b'C:')[1]==p.path49(b'C:.')[1]==b'C:\\Emperor';passed('drive-relative, plain-relative and rooted full paths agree with the actual native root')
    assert p.path49(b'C:new-own-missing.dat')[1]==b'C:\\Emperor\\new-own-missing.dat';passed('full path calculation does not require file existence')
    for raw in [b'D:Save',b'C:\\Save',b'C:..\\outside',b'C:\\Emperor\\..\\outside',b'C:NUL',b'C:save:ADS',b'C:bad.']:
        rejected(lambda:p.path49(raw),PolicyError)
    passed('drive-root distinction, unmounted drives, escape, devices and ambiguity still refused')
    native_before=p.paths.export();assert p.paths.set(b'.',0x13579)==(1,1,0x13579);assert p.paths.export()==native_before and p.guest_cwd51()==b'C:\\Emperor'and p.environment_state51==saved;passed('existing FD-backed native current directory is unchanged and agrees with guest drive cache')
    big=GuestEnvironment()
    for i in range(7):assert big.set(('OWN'+str(i)).encode(),b'x'*4095,0,b'C:\\Emperor')==(1,0)
    frozen=big.snapshot(b'C:\\Emperor');rejected(lambda:big.set(b'OWN7',b'x'*4095,0,b'C:\\Emperor'),PolicyError);assert big.snapshot(b'C:\\Emperor')==frozen;passed('XP environment block resource bound refuses atomically')
    fixture=R/'runs/stage51-own-ledger-fixture.json';fixture.write_text(json.dumps(dict(maximum_original_executions=10,user_approval_utc='2026-10-07 05:21:04 UTC',executions=[],remaining_original_executions=10)))
    for i in range(10):index=claim('own test; no original instructions','own','own'+str(i),['own'],fixture);finish(index,{'status':'own_test_only'},{},fixture)
    frozen=fixture.read_bytes();rejected(lambda:claim('eleventh own test','own','own10',['own'],fixture),AssertionError);assert fixture.read_bytes()==frozen;assert json.loads((R/'evidence/stage51-batch-ledger.json').read_text())['executions']==[];passed('own ledger fixture enforces ten claims; production original execution budget remains unused')
    assert dict(os.environ)==host_env and os.getcwd()==host_cwd;passed('host process environment and cwd unchanged')
    out=dict(passed=True,cases=len(events),events=events,original_EXE_instructions_executed=False,original_DLL_exports_executed=False,emulator_instance_created=False,host_environment_unchanged=True,host_cwd_unchanged=True,actual_native_directory_fd_used=True,source_files_written=False,guest_semantic_state_restored=True)
    (R/'evidence/stage51-environment-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['passed','cases','original_EXE_instructions_executed','host_environment_unchanged']}))
finally:p.paths.dispose()
