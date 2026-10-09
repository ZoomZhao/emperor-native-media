#!/usr/bin/env python3
"""Bounded, authored i386 byte interpreter, independent of SR and old Swift.

This is an ISA-semantic oracle, not execution of the original game on Windows.
Only the three audited original fragments and explicit mapped state are allowed.
"""
import json,struct,hashlib,itertools,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
U32=0xffffffff
CF,PF,AF,ZF,SF,OF=1,4,16,64,128,2048
ALL=CF|PF|AF|ZF|SF|OF
LOGIC=ALL&~AF
STACK=0x7000ffb0
SP=0x7000fff0
SENTINEL=0xfffffffe
def signed(v,bits=32):
    v&=(1<<bits)-1
    return v-(1<<bits) if v>>(bits-1) else v
def alu(op,a,b,flags,bits=32):
    mask=(1<<bits)-1; sign=1<<(bits-1);a&=mask;b&=mask
    if op in ('sub','cmp'): result=(a-b)&mask
    elif op in ('add','inc'): result=(a+b)&mask
    elif op=='xor': result=a^b
    elif op=='or': result=a|b
    elif op=='test': result=a&b
    else: raise ValueError(op)
    defined=LOGIC if op in ('xor','or','test') else ALL
    new=PF if bin(result&255).count('1')%2==0 else 0
    new|=(ZF if result==0 else 0)|(SF if result&sign else 0)
    if op in ('add','inc','sub','cmp'):
        carry=(a+b)>mask if op in ('add','inc') else a<b
        overflow=((~(a^b)&(a^result)) if op in ('add','inc') else ((a^b)&(a^result)))&sign
        new|=(CF if carry else 0)|(OF if overflow else 0)|(AF if (a^b^result)&16 else 0)
    if op=='inc': new=(new&~CF)|(flags&CF)
    return result,(flags&~defined)|new,defined

class Machine:
    def __init__(self,kind,seed,tax,difficulty,coverage,metric,guard,count,variant,flags):
        self.code={int(k):bytes.fromhex(v) for k,v in json.loads((ROOT/'samples/tax-fragments.json').read_text()).items()}
        self.regs=initial_regs(seed);self.regs[4]=SP
        self.flags=flags;self.defined=ALL;self.steps=0;self.trace=[];self.edges=set()
        self.regions=[(0x85cc8c,bytearray(struct.pack('<7I',*rates_for(variant)))),(0x10de2e0,bytearray(struct.pack('<I',difficulty&U32))),(0x130f810,bytearray(struct.pack('<4I',0x85cc8c,count&U32,0x11e,0x130f96c))),(0x130f858,bytearray(struct.pack('<35I',*[x&U32 for x in matrix_for(variant)]))),(0x130f96c,bytearray(struct.pack('<I',tax&U32))),(0x130f988,bytearray(struct.pack('<I',metric&U32))),(0x131223c,bytearray(struct.pack('<I',coverage&U32))),(0x1312575,bytearray([guard])),(STACK,bytearray([0xa5]*80))]
        self.kind=kind;self.pc=0x591180 if kind==0 else 0x592be0
        if kind==1:self.regs[1]=0x130f810
        self.push(SENTINEL)
    def read(self,address,size,execute=False):
        address&=U32
        for base,data in (self.code.items() if execute else self.regions):
            if base<=address and address+size<=base+len(data):return int.from_bytes(data[address-base:address-base+size],'little')
        raise ValueError(f'unmapped {"code" if execute else "memory"}: {address:#x}/{size}')
    def write(self,address,size,value):
        address&=U32
        assert STACK<=address and address+size<=STACK+80,'Routine unexpectedly wrote non-stack state'
        data=self.regions[-1][1];data[address-STACK:address-STACK+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
    def fetch(self,size=1):
        value=self.read(self.pc,size,True);self.pc+=size;return value
    def push(self,value):self.regs[4]=(self.regs[4]-4)&U32;self.write(self.regs[4],4,value)
    def pop(self):
        value=self.read(self.regs[4],4);self.regs[4]=(self.regs[4]+4)&U32;return value
    def operand(self,modrm,bits=32):
        mod,index,rm=modrm>>6,(modrm>>3)&7,modrm&7
        if mod==3:return ('reg',rm,bits),index
        disp=0;base=0
        if rm==4:
            sib=self.fetch();scale,idx,reg=sib>>6,(sib>>3)&7,sib&7
            base=0 if reg==5 and mod==0 else self.regs[reg]
            if idx!=4:base+=self.regs[idx]<<scale
            if reg==5 and mod==0:disp=self.fetch(4)
        elif rm==5 and mod==0:disp=self.fetch(4)
        else:base=self.regs[rm]
        if mod==1:disp+=signed(self.fetch(),8)
        elif mod==2:disp+=self.fetch(4)
        return ('mem',(base+disp)&U32,bits),index
    def get(self,o):
        kind,n,bits=o
        if kind=='mem':return self.read(n,bits//8)
        if bits==8:return (self.regs[n%4]>>(8 if n>=4 else 0))&255
        return self.regs[n]
    def set(self,o,value):
        kind,n,bits=o
        if kind=='mem':self.write(n,bits//8,value)
        elif bits==8:
            shift=8 if n>=4 else 0;reg=n%4
            self.regs[reg]=(self.regs[reg]&~(255<<shift))|((value&255)<<shift)
        else:self.regs[n]=value&U32
    def arithmetic(self,op,dest,b):
        value,self.flags,self.defined=alu(op,self.get(dest),b,self.flags,dest[2])
        if op not in ('cmp','test'):self.set(dest,value)
    def run(self):
        while self.pc!=SENTINEL:
            assert self.steps<500,'Instruction budget exceeded'
            old=self.pc;self.trace.append(old);self.steps+=1;op=self.fetch()
            if 0xb8<=op<=0xbf:self.regs[op-0xb8]=self.fetch(4)
            elif 0x50<=op<=0x57:self.push(self.regs[op-0x50])
            elif 0x58<=op<=0x5f:self.regs[op-0x58]=self.pop()
            elif 0x40<=op<=0x47:self.arithmetic('inc',('reg',op-0x40,32),1)
            elif op==0x99:self.regs[2]=U32 if self.regs[0]&0x80000000 else 0
            elif op in (0xe8,0xe9):
                delta=signed(self.fetch(4));target=(self.pc+delta)&U32
                if op==0xe8:self.push(self.pc)
                self.pc=target
            elif op==0xc3:self.pc=self.pop()
            elif op in (0x7c,0x7d,0x7e,0x7f,0x75):
                delta=signed(self.fetch(),8);sf=bool(self.flags&SF);of=bool(self.flags&OF);zf=bool(self.flags&ZF)
                take={0x7c:sf!=of,0x7d:sf==of,0x7e:zf or sf!=of,0x7f:not zf and sf==of,0x75:not zf}[op]
                self.edges.add((old,take))
                if take:self.pc=(self.pc+delta)&U32
            elif op in (0x8b,0x8a,0x89,0x8d,0x33,0x03,0x2b,0x3b,0x85,0x84):
                bits=8 if op in (0x8a,0x84) else 32
                operand,reg=self.operand(self.fetch(),bits);r=('reg',reg,bits)
                if op in (0x8b,0x8a):self.set(r,self.get(operand))
                elif op==0x89:self.set(operand,self.get(r))
                elif op==0x8d:assert operand[0]=='mem';self.set(r,operand[1])
                elif op in (0x85,0x84):self.arithmetic('test',operand,self.get(r))
                else:self.arithmetic({0x33:'xor',0x03:'add',0x2b:'sub',0x3b:'cmp'}[op],r,self.get(operand))
            elif op in (0x83,0x81):
                operand,group=self.operand(self.fetch());imm=signed(self.fetch(),8)&U32 if op==0x83 else self.fetch(4)
                self.arithmetic({0:'add',1:'or',7:'cmp'}[group],operand,imm)
            else:raise ValueError(f'Unsupported opcode {op:#x} at {old:#x}')
        assert self.regs[4]==SP
        return self

def initial_regs(seed):
    return [((seed*0x9e3779b9)^(i*0x13579bdf)^0xa1b2c3d4)&U32 for i in range(8)]
def rates_for(variant):
    if variant==1:return [0,4,4,8,12,16,20]
    if variant==2:return [0x80000000,0xffffffff,0,1,4,0x7ffffffe,0x7fffffff]
    return [0,3,6,9,12,15,18]
def matrix_for(variant):
    rows=json.loads((ROOT/'evidence/stage2-tax-input.json').read_text())['rows']
    if variant==2:return [(-0x80000000 if i%3==0 else 0x7fffffff if i%3==1 else -1) for i in range(35)]
    return [v for row in rows for v in row[1:]]
def table_oracle(tax,difficulty,coverage,metric,guard,count,variant):
    """Separate direct data/rule model; no interpreter register/flag formulas."""
    distances=[signed((tax-r)&U32) for r in rates_for(variant)[:count]]
    distances=[signed(-x) if x<0 else x for x in distances]
    row=min(range(count),key=lambda i:distances[i]) if count>0 else -1
    if coverage<=10:row=0
    result=matrix_for(variant)[row*5+difficulty]
    return 0 if result<0 and metric<350 and guard==0 else result

def generate():
    rows=[];branches=set();instructions=set();paths={};maxsteps=0;data_oracle_failures=0
    def append(kind,tax,difficulty=0,coverage=11,metric=350,guard=1,count=7,variant=0):
        nonlocal maxsteps,data_oracle_failures
        seed=len(rows)+1;flags=0x202|((seed*73)&ALL)
        m=Machine(kind,seed,tax,difficulty,coverage,metric,guard,count,variant,flags).run()
        if kind==0 and m.regs[0]!=(table_oracle(tax,difficulty,coverage,metric,guard,count,variant)&U32):data_oracle_failures+=1
        branches.update(m.edges);instructions.update(m.trace);maxsteps=max(maxsteps,m.steps)
        path=tuple(m.trace);paths.setdefault(path,len(rows))
        stack=list(struct.unpack('<16I',m.regions[-1][1][:64]))
        rows.append([kind,seed,tax&U32,difficulty&U32,coverage&U32,metric&U32,guard,count&U32,variant,flags,*m.regs,m.flags,m.defined,*stack])
    for values in itertools.product(range(-1,20),range(5),(-1,0,10,11,100),(0,349,350,351,0x7fffffff),(0,1,128,255)):
        append(0,*values)
    for values in itertools.product((-0x80000000,-0x7fffffff,-65536,0x7ffffffe,0x7fffffff),range(5),(-0x80000000,10,11,0x7fffffff),(-0x80000000,349,350,0x7fffffff),(0,255)):
        append(0,*values)
    for variant in (0,1,2):
        for tax,count in itertools.product((-0x80000000,-1,0,1,2,3,4,6,17,18,19,0x7ffffffe,0x7fffffff),(-1,0,1,2,7)):
            append(1,tax,count=count,variant=variant)
        for values in itertools.product((-0x80000000,-1,0,2,4,17,0x7fffffff),range(5),(10,11),(349,350),(0,255)):
            append(0,*values,variant=variant)
    assert data_oracle_failures==0
    fixture=ROOT/'samples/tax-cases.bin'
    with fixture.open('wb') as f:
        f.write(struct.pack('<4I',0x54415832,len(rows),36,1))
        for row in rows:assert len(row)==36;f.write(struct.pack('<36I',*row))
    info=dict(cases=len(rows),tax_cases=sum(r[0]==0 for r in rows),helper_cases=sum(r[0]==1 for r in rows),unique_paths=len(paths),unique_instruction_addresses=len(instructions),conditional_edges=[dict(address=hex(a),taken=t) for a,t in sorted(branches)],max_steps=maxsteps,table_rule_oracle_failures=data_oracle_failures,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),baseline='authored raw-i386-byte interpreter, ISA-semantic; not original Windows execution',return_pointer_normalization='top host return slot, plus original internal direct-call return slot for tax root only')
    (ROOT/'evidence/stage2-oracle.json').write_text(json.dumps(info,indent=2))
    (ROOT/'evidence/stage2-paths.json').write_text(json.dumps([dict(case=i,trace=[hex(x) for x in path]) for path,i in paths.items()],indent=2))
    print(json.dumps(info))
def primitive_vectors():
    rng=random.Random(0x54415832);out=ROOT/'samples/x86-alu-cases.bin';n=0
    with out.open('wb') as f:
        for opcode,op,bits in [(0,'add',32),(1,'sub',32),(2,'xor',32),(3,'or',32),(4,'test',32),(5,'inc',32),(6,'test',8)]:
            values=[0,1,15,16,127,128,255,256,0x7fffffff,0x80000000,0xfffffffe,U32]
            pairs=list(itertools.product(values,values))+[(rng.getrandbits(32),rng.getrandbits(32)) for _ in range(1024)]
            for a,b in pairs:
                if op=='inc':b=1
                flags=0x202|rng.getrandbits(32)&ALL
                result,expected,mask=alu(op,a,b,flags,bits)
                f.write(struct.pack('<7I',opcode,a,b,flags,result,expected,mask));n+=1
    print(json.dumps(dict(primitive_vectors=n)))
if __name__=='__main__':
    import sys
    primitive_vectors() if len(sys.argv)>1 and sys.argv[1]=='primitives' else generate()
