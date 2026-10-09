#!/usr/bin/env python3
"""Holdout reuses independent ISA interpreter, without SR output or old SCI."""
import json,struct,hashlib
from tax_x86_oracle import Machine,ROOT,SP,SENTINEL,rates_for,signed,U32
class Writer(Machine):
    def __init__(self,*args):
        self.writes=0;super().__init__(*args)
        self.code={int(k):bytes.fromhex(v) for k,v in json.loads((ROOT/'samples/write-fragments.json').read_text()).items()};self.pc=0x592d10
    def write(self,address,size,value):
        if address==0x130f96c and size==4:
            region=next(data for va,data in self.regions if va==address);region[:]=(value&U32).to_bytes(4,'little');self.writes+=1
        else:super().write(address,size,value)
def next_value(tax,variant):
    rates=rates_for(variant);dist=[]
    for rate in rates:
        d=signed(tax-rate);dist.append(signed(-d) if d<0 else d)
    idx=min(range(7),key=lambda i:dist[i])+1
    return (rates[idx],True) if idx<7 else (tax&U32,False)
records=[];paths=set();written=0;changed=0;rulediff=0;instructions=set()
taxes=list(range(-3,25))+[-0x80000000,-0x7fffffff,0x7ffffffe,0x7fffffff]
for variant in (0,1,2):
    for tax in taxes:
        seed=len(records)+1;flags=0x202|((seed*73)&0x8d5);m=Writer(0,seed,tax,0,11,350,1,7,variant,flags)
        for step in range(8):
            old=m.read(0x130f96c,4);expected,expectedwrite=next_value(old,variant);before=m.writes
            if step:m.pc=0x592d10;m.push(SENTINEL);m.trace=[];m.steps=0
            m.run();after=m.read(0x130f96c,4);didwrite=m.writes-before
            assert after==expected and bool(didwrite)==expectedwrite
            written+=didwrite;changed+=after!=old;paths.add(tuple(m.trace));instructions.update(m.trace)
            stack=list(struct.unpack('<16I',m.regions[-1][1][:64]))
            records.append([int(step==0),seed,variant,old,flags,*m.regs,m.flags,m.defined,after,didwrite,*stack])
f=ROOT/'samples/write-cases.bin'
with f.open('wb') as out:
    out.write(struct.pack('<4I',0x57525433,len(records),33,8))
    for record in records:assert len(record)==33;out.write(struct.pack('<33I',*record))
result=dict(cases=len(taxes)*3,calls=len(records),consecutive_calls_per_state=8,writes=written,changed_state_transitions=changed,unique_paths=len(paths),instruction_addresses=len(instructions),rule_oracle_failures=rulediff,fixture_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),baseline='authored original-byte ISA interpreter; not direct original i386 execution')
(ROOT/'evidence/stage3-write-oracle.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
