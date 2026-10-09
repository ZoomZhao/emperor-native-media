#!/usr/bin/env python3
"""Independent engine executes only the three already-approved raw fragments.

Does not import the authored ISA interpreter or decoder. Existing fixtures are
inputs/reference outputs; new expected-output fixtures come directly from Unicorn.
"""
import sys,json,struct,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PKG=ROOT/'vendor-tools/unicorn-2.1.4'
WHEEL=ROOT/'evidence/unicorn-2.1.4-cp37-abi3-macosx_11_0_arm64.whl'
assert hashlib.sha256(WHEEL.read_bytes()).hexdigest()=='d6c93e0f60328d8f4a1792af3f834137a28050fcc2305f2ec01efe8558a9844e'
sys.path.insert(0,str(PKG))
import unicorn as u
from unicorn import x86_const as x
from unicorn.unicorn_py3.unicorn import uclib
assert u.__version__=='2.1.4'
assert Path(uclib._name).resolve()==(PKG/'unicorn/lib/libunicorn.2.dylib').resolve()
REG=[x.UC_X86_REG_EAX,x.UC_X86_REG_ECX,x.UC_X86_REG_EDX,x.UC_X86_REG_EBX,x.UC_X86_REG_ESP,x.UC_X86_REG_EBP,x.UC_X86_REG_ESI,x.UC_X86_REG_EDI]
SP=0x7000fff0;SENTINEL=0xfffffffe;ALL=0x8d5;LOGIC=0x8c5;U32=0xffffffff;CONTROL=U32^ALL
def pe_sections(path):
    b=Path(path).read_bytes();p=struct.unpack_from('<I',b,0x3c)[0];assert b[p:p+4]==b'PE\0\0'
    machine,n,_,_,_,optsize,_=struct.unpack_from('<HHIIIHH',b,p+4);assert machine==0x14c
    base=struct.unpack_from('<I',b,p+24+28)[0];sections=[]
    for i in range(n):
        name,vs,rva,rawsize,ptr,_,_,_,_,flags=struct.unpack_from('<8sIIIIIIHHI',b,p+24+optsize+i*40)
        sections.append(dict(name=name.rstrip(b'\0').decode(),va=base+rva,size=vs,bytes=b[ptr:ptr+min(vs,rawsize)]+bytes(max(0,vs-rawsize)),flags=flags))
    return b,sections
SOURCE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'
original,original_sections=pe_sections(SOURCE)
assert hashlib.sha256(original).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
def raw(va,size):
    s=next(s for s in original_sections if s['va']<=va and va+size<=s['va']+s['size'])
    return s['bytes'][va-s['va']:va-s['va']+size]
def initial(seed):return [((seed*0x9e3779b9)^(i*0x13579bdf)^0xa1b2c3d4)&U32 for i in range(8)]
def read_cases(path,words):
    b=path.read_bytes();header=list(struct.unpack_from('<4I',b));assert header[2]==words and len(b)==16+header[1]*words*4
    return header,[list(struct.unpack_from('<'+str(words)+'I',b,16+i*words*4)) for i in range(header[1])]
def store_cases(path,header,rows):
    with path.open('wb') as f:
        f.write(struct.pack('<4I',*header))
        for row in rows:f.write(struct.pack('<'+str(len(row))+'I',*row))
def rates(variant):return [0,4,4,8,12,16,20] if variant==1 else [0x80000000,U32,0,1,4,0x7ffffffe,0x7fffffff] if variant==2 else [0,3,6,9,12,15,18]
rows=json.loads((ROOT/'evidence/stage2-tax-input.json').read_text())['rows']
MODEL=[v&U32 for row in rows for v in row[1:]]
def matrix(variant):return [0x80000000 if i%3==0 else 0x7fffffff if i%3==1 else U32 for i in range(35)] if variant==2 else MODEL

class Engine:
    def __init__(self,module):
        self.module=module;self.uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);self.pages=set();self.active=False;self.mask=ALL;self.trace=[];self.writes=0;self.paths=set();self.instruction_addresses=set();self.changed=0
        _,sections=pe_sections(ROOT/'samples'/({'leaf':'emperor-leaf.exe','tax':'emperor-tax.exe','write':'emperor-write-fixture.exe'}[module]))
        self.data=[s for s in sections if not s['flags']&0x20000000 and s['name']!='.reloc']
        for s in self.data:self.map(s['va'],s['size'],u.UC_PROT_READ|u.UC_PROT_WRITE)
        ranges={'leaf':[(0x401010,0x401025)],'tax':[(0x591180,0x5911c4),(0x592be0,0x592c2a),(0x592cf0,0x592cfa)],'write':[(0x592d10,0x592d1a),(0x592c70,0x592c8c),(0x592be0,0x592c2a)]}
        self.code=ranges[module]
        for lo,hi in self.code:
            self.map(lo,hi-lo,u.UC_PROT_ALL);self.uc.mem_write(lo,raw(lo,hi-lo))
        for page in {va&~4095 for lo,hi in self.code for va in range(lo,hi)}:self.uc.mem_protect(page,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
        self.map(0x70000000,65536,u.UC_PROT_READ|u.UC_PROT_WRITE)
        self.map(SENTINEL,1,u.UC_PROT_ALL);self.uc.mem_write(SENTINEL,b'\xf4');self.uc.mem_protect(SENTINEL&~4095,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
        self.uc.hook_add(u.UC_HOOK_CODE,self.code_hook)
        self.uc.hook_add(u.UC_HOOK_MEM_WRITE,self.write_hook)
        self.reset()
    def map(self,va,size,permissions):
        for page in range(va&~4095,(va+size+4095)&~4095,4096):
            if page not in self.pages:self.uc.mem_map(page,4096,permissions);self.pages.add(page)
    def reset(self):
        for s in self.data:self.uc.mem_write(s['va'],s['bytes'])
        self.uc.mem_write(0x70000000,bytes([0xa5])*65536)
    def word(self,va,v):self.uc.mem_write(va,struct.pack('<I',v&U32))
    def words(self,va,values):self.uc.mem_write(va,struct.pack('<'+str(len(values))+'I',*[v&U32 for v in values]))
    def initialize(self,registers,flags):
        for identifier,value in zip(REG,registers):self.uc.reg_write(identifier,value)
        self.uc.reg_write(x.UC_X86_REG_ESP,SP);self.uc.reg_write(x.UC_X86_REG_EFLAGS,flags)
    def code_hook(self,uc,address,size,user):
        if address==SENTINEL:uc.emu_stop();return
        if not any(lo<=address and address+size<=hi for lo,hi in self.code):raise RuntimeError('Execution escaped reviewed original fragments: '+hex(address))
        self.trace.append(address);self.instruction_addresses.add(address);b=bytes(uc.mem_read(address,size));op=b[0]
        # Definedness only; EFLAGS values are computed by Unicorn, not our ALU.
        if op in (0x03,0x2b,0x3b):self.mask=ALL
        elif op in (0x33,0x32,0x0b,0x0a,0x85,0x84):self.mask=LOGIC
        elif op in (0x81,0x83):self.mask=ALL if (b[1]>>3)&7 in (0,5,7) else LOGIC
        elif 0x40<=op<=0x47:self.mask=(self.mask&1)|(ALL&~1)
    def write_hook(self,uc,access,address,size,value,user):
        if not self.active:return
        if 0x70000000<=address and address+size<=0x70010000:return
        expected=(0x1e58bf8,1) if self.module=='leaf' else (0x130f96c,4) if self.module=='write' else None
        if (address,size)!=expected:raise RuntimeError(f'Unexpected original data write {address:#x}/{size}')
        self.writes+=1
    def call(self,entry):
        esp=self.uc.reg_read(x.UC_X86_REG_ESP);self.word(esp-4,SENTINEL);self.uc.reg_write(x.UC_X86_REG_ESP,esp-4)
        self.trace=[];self.mask=ALL;self.active=True
        try:self.uc.emu_start(entry,SENTINEL,timeout=100000,count=500)
        finally:self.active=False
        assert self.uc.reg_read(x.UC_X86_REG_EIP)==SENTINEL,'Instruction/engine timeout budget reached before return'
        assert self.uc.reg_read(x.UC_X86_REG_ESP)==SP,'Guest stack imbalance'
        self.paths.add(tuple(self.trace))
        return [self.uc.reg_read(r) for r in REG],self.uc.reg_read(x.UC_X86_REG_EFLAGS),self.mask,list(struct.unpack('<16I',bytes(self.uc.mem_read(SP-64,64))))
    def snapshot(self):return b''.join(bytes(self.uc.mem_read(s['va'],s['size'])) for s in self.data)

def main():
    module=sys.argv[1];assert module in ('leaf','tax','write')
    start=int(sys.argv[2]) if len(sys.argv)>2 else 0;limit=int(sys.argv[3]) if len(sys.argv)>3 else 10**9
    e=Engine(module);diffs=dict(register=0,flags=0,defined_mask=0,stack=0,memory=0,repeat=0);examples=[];calls=0;output=[]
    def compare(label,got,expected,index,round=0,mask=None):
        equal=got==expected if mask is None else ((got^expected)&mask)==0
        if not equal:
            diffs[label]+=1
            if len(examples)<12:examples.append(dict(case=index,round=round,kind=label,got=got,expected=expected,mask=mask,trace=[hex(p) for p in e.trace]))
    if module=='leaf':
        header=[0x4c454146,256,28,1]
        for b in range(256):
            e.reset();e.uc.mem_write(0x1e58bf8,bytes([b]));init=[0xabcdefed,0x23456788,0x76543210,0x11223344,SP,0x55667788,0x99aabbcc,0xddeeff00];e.initialize(init,0x3202)
            regs,flags,mask,stack=e.call(0x401010);flag=bytes(e.uc.mem_read(0x1e58bf8,1))[0];calls+=1
            expected=[0xabcdef01,0x23456700|(b|1),*init[2:]];r=1 if b&1 else b|1;ef=(4 if bin(r).count('1')%2==0 else 0)|(128 if r&128 else 0)
            compare('register',regs,expected,b);compare('flags',flags,0x3202|ef,b,mask=LOGIC|CONTROL);compare('defined_mask',mask,LOGIC,b);compare('memory',flag,b|1,b);compare('stack',stack,[0xa5a5a5a5]*15+[SENTINEL],b);e.changed+=flag!=b
            output.append([b,*regs,flags,mask,flag,*stack])
    elif module=='tax':
        header,inputs=read_cases(ROOT/'samples/tax-cases.bin',36);end=min(len(inputs),start+limit);header[1]=end-start
        for i in range(start,end):
            r=inputs[i];e.reset();e.words(0x85cc8c,rates(r[8]));e.words(0x130f858,matrix(r[8]));e.words(0x130f810,[0x85cc8c,r[7],0x11e,0x130f96c])
            for va,value in [(0x130f96c,r[2]),(0x10de2e0,r[3]),(0x131223c,r[4]),(0x130f988,r[5])]:e.word(va,value)
            e.uc.mem_write(0x1312575,bytes([r[6]]));e.initialize(initial(r[1]),r[9]);before=e.snapshot();first=None
            for round in range(2):
                if r[0]:e.uc.reg_write(x.UC_X86_REG_ECX,0x130f810)
                result=e.call(0x592be0 if r[0] else 0x591180);regs,flags,mask,stack=result;calls+=1
                compare('register',regs,r[10:18],i,round);compare('flags',flags,r[18],i,round,mask=r[19]|CONTROL);compare('defined_mask',mask,r[19],i,round);compare('stack',stack,r[20:36],i,round);compare('memory',e.snapshot(),before,i,round)
                if first is None:first=result
                else:
                    compare('repeat',regs,first[0],i,round);compare('repeat',flags,first[1],i,round,mask=mask|CONTROL);compare('repeat',stack,first[3],i,round)
            new=r[:];new[10:18]=first[0];new[18]=first[1];new[19]=first[2];new[20:36]=first[3];output.append(new)
    else:
        header,inputs=read_cases(ROOT/'samples/write-cases.bin',33);assert start==0
        for i,r in enumerate(inputs):
            if r[0]:
                e.reset();e.words(0x85cc8c,rates(r[2]));e.words(0x130f858,matrix(r[2]));e.words(0x130f810,[0x85cc8c,7,0x11e,0x130f96c]);e.word(0x130f96c,r[3]);e.word(0x10de2e0,0);e.word(0x131223c,11);e.word(0x130f988,350);e.uc.mem_write(0x1312575,b'\1');e.initialize(initial(r[1]),r[4])
            old=int.from_bytes(e.uc.mem_read(0x130f96c,4),'little');before=e.snapshot();w=e.writes
            regs,flags,mask,stack=e.call(0x592d10);calls+=1;after=int.from_bytes(e.uc.mem_read(0x130f96c,4),'little');didwrite=e.writes-w;e.changed+=old!=after
            compare('register',regs,r[5:13],i);compare('flags',flags,r[13],i,mask=r[14]|CONTROL);compare('defined_mask',mask,r[14],i);compare('stack',stack,r[17:33],i);compare('memory',after,r[15],i);compare('memory',didwrite,r[16],i)
            # Check that no byte except the current tax field changed.
            expected=bytearray(before);offset=sum(s['size'] for s in e.data[:next(j for j,s in enumerate(e.data) if s['va']<=0x130f96c<s['va']+s['size'])])+0x130f96c-next(s['va'] for s in e.data if s['va']<=0x130f96c<s['va']+s['size']);expected[offset:offset+4]=after.to_bytes(4,'little');compare('memory',e.snapshot(),bytes(expected),i)
            new=r[:];new[3]=old;new[5:13]=regs;new[13]=flags;new[14]=mask;new[15]=after;new[16]=didwrite;new[17:33]=stack;output.append(new)
    label=f'{module}-{start}';path=ROOT/f'samples/stage4-{label}-unicorn-cases.bin';store_cases(path,header,output)
    path_evidence=ROOT/f'evidence/stage4-{label}-unicorn-paths.json';path_evidence.write_text(json.dumps([[hex(a) for a in p] for p in sorted(e.paths)],indent=2))
    # Raw large byte buffers need not be embedded in diagnostic JSON.
    for item in examples:
        for key in ('got','expected'):
            if isinstance(item[key],bytes):item[key]=dict(bytes=len(item[key]),sha256=hashlib.sha256(item[key]).hexdigest())
    result=dict(module=module,start=start,records=len(output),calls=calls,guest_data_write_events=e.writes,observed_state_changes=e.changed,unique_paths=len(e.paths),instruction_addresses=len(e.instruction_addresses),self_baseline_differences=diffs,examples=examples,passed=not any(diffs.values()),fixture=str(path),fixture_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),engine='Unicorn 2.1.4 UC_MODE_32 on arm64',loaded_library=uclib._name,raw_original_instruction_bytes=True,full_Windows_game_run=False,per_call_instruction_budget=500,per_call_timeout_microseconds=100000)
    (ROOT/f'evidence/stage4-{label}-unicorn.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));return 0 if result['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
