"""Existing approved fixed official engine; own two-byte IDIV only, once."""
import hashlib,json,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81'
lib=ROOT/'vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib';assert hashlib.sha256(lib.read_bytes()).hexdigest()=='7207c8e3d7a63118fb0bca73e01816797fd51b1d8a39a4cbc7abfd562ee59c85'
sys.dont_write_bytecode=True;sys.path.insert(0,str(ROOT/'vendor-tools/unicorn-2.1.4'))
import unicorn as u
from unicorn import x86_const as x
assert u.__version__=='2.1.4'
cases=[]
for dividend,divisor in [(0,1),(7,3),(-7,3),(7,-3),(-7,-3),(0x7fffffff,1),(-0x80000000,1),(-0x80000000,-1),(0x80000000,1),(0xffffffff,2),(-0xffffffff,2),(0x7fffffffffffffff,0x7fffffff),(-0x8000000000000000,-1),(-0x8000000000000000,1),(1,0),(-1,0),(0,0),(0x100000001,3),(-0x100000001,3),(0x7fffffffffffffff,-0x80000000)]:
 raw=dividend&0xffffffffffffffff;cases.append((raw&0xffffffff,raw>>32,divisor&0xffffffff,0x602))
claim=S/'evidence/authored-idiv.claim.json';with_claim=dict(original_game_instructions=0,authored_instructions_maximum=len(cases),engine_SHA256=hashlib.sha256(lib.read_bytes()).hexdigest(),approval_evidence='evidence/stage4-approval-evidence.txt',existing_scope_continuous_authored_diagnostics=True,retries=0)
with claim.open('x')as f:json.dump(with_claim,f,indent=2)
(S/'evidence/idiv-input.bin').write_bytes(b'S81D'+struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',*c)for c in cases));results=[];start=time.monotonic()
for eax,edx,divisor,flags in cases:
 uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);uc.mem_map(0x6f000000,4096);uc.mem_write(0x6f000000,b'\xf7\xfb');uc.reg_write(x.UC_X86_REG_EAX,eax);uc.reg_write(x.UC_X86_REG_EDX,edx);uc.reg_write(x.UC_X86_REG_EBX,divisor);uc.reg_write(x.UC_X86_REG_EFLAGS,flags);fault=0
 try:uc.emu_start(0x6f000000,0x6f000002,timeout=100000,count=1)
 except u.UcError as e:assert e.errno==u.UC_ERR_EXCEPTION;fault=1
 results.append((uc.reg_read(x.UC_X86_REG_EAX),uc.reg_read(x.UC_X86_REG_EDX),uc.reg_read(x.UC_X86_REG_EFLAGS),fault))
(S/'evidence/idiv-oracle.bin').write_bytes(b'S81D'+struct.pack('<I',len(cases))+b''.join(struct.pack('<4I',*c)for c in results));r=dict(status='20_AUTHORED_IDIV_PROBES_COLLECTED',cases=len(cases),original_game_instructions=0,original_contexts_restored=0,duration_seconds=time.monotonic()-start,engine_SHA256=with_claim['engine_SHA256']);(S/'evidence/idiv-oracle.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
