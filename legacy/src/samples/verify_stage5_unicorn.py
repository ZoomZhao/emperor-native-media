#!/usr/bin/env python3
"""Bounded original-byte feedback calls; no authored game/ISA interpreter."""
import sys,json,struct,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PKG=ROOT/'vendor-tools/unicorn-2.1.4'
assert hashlib.sha256((ROOT/'evidence/unicorn-2.1.4-cp37-abi3-macosx_11_0_arm64.whl').read_bytes()).hexdigest()=='d6c93e0f60328d8f4a1792af3f834137a28050fcc2305f2ec01efe8558a9844e'
assert hashlib.sha256((PKG/'unicorn/lib/libunicorn.2.dylib').read_bytes()).hexdigest()=='7207c8e3d7a63118fb0bca73e01816797fd51b1d8a39a4cbc7abfd562ee59c85'
sys.path.insert(0,str(PKG))
import unicorn as u
from unicorn import x86_const as x
assert u.__version__=='2.1.4'
REG=[x.UC_X86_REG_EAX,x.UC_X86_REG_ECX,x.UC_X86_REG_EDX,x.UC_X86_REG_EBX,x.UC_X86_REG_ESP,x.UC_X86_REG_EBP,x.UC_X86_REG_ESI,x.UC_X86_REG_EDI]
SP=0x7000fff0; SENTINEL=0xfffffffe; ALL=0x8d5; LOGIC=0x8c5; U32=0xffffffff
layout=json.loads((ROOT/'evidence/stage5-slice-layout.json').read_text())
analysis=json.loads((ROOT/'runs/stage5-analysis/analysis.json').read_text())
source=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe').read_bytes()
assert hashlib.sha256(source).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
# This file's code ranges all lie in the original .text (VA 0x401000, raw 0x1000).
def raw(va,n):return source[va-0x400000:va-0x400000+n]
def seedregs(seed):return [((seed*0x9e3779b9)^(i*0x13579bdf)^0xa1b2c3d4)&U32 for i in range(8)]
entries=[f['start'] for f in layout['fragments']]
state=layout['state'];data=layout['data_sections'];allowed={i['pc']:len(bytes.fromhex(i['bytes'])) for i in analysis['instructions']}
class Engine:
    def __init__(self):
        self.uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);self.pages=set();self.total_steps=0;self.active=False;self.writes=0;self.changed=0;self.paths=set();self.addresses=set();self.mask=ALL
        def mapping(va,n,perms):
            for page in range(va&~4095,(va+n+4095)&~4095,4096):
                if page not in self.pages:self.uc.mem_map(page,4096,perms);self.pages.add(page)
        for f in layout['fragments']:
            mapping(f['start'],f['end']-f['start'],u.UC_PROT_ALL);self.uc.mem_write(f['start'],raw(f['start'],f['end']-f['start']))
        for f in layout['fragments']:
            self.uc.mem_protect(f['start']&~4095,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
        for s in data:mapping(s['va'],4096,u.UC_PROT_READ|u.UC_PROT_WRITE)
        mapping(0x70000000,65536,u.UC_PROT_READ|u.UC_PROT_WRITE)
        mapping(SENTINEL,1,u.UC_PROT_ALL);self.uc.mem_write(SENTINEL,b'\xf4');self.uc.mem_protect(SENTINEL&~4095,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
        self.uc.hook_add(u.UC_HOOK_CODE,self.code_hook);self.uc.hook_add(u.UC_HOOK_MEM_WRITE,self.write_hook)
        self.reset(1,0x3202)
    def word(self,va,v):self.uc.mem_write(va,struct.pack('<I',v&U32))
    def readword(self,va):return int.from_bytes(self.uc.mem_read(va,4),'little')
    def reset(self,seed,flags):
        for s in data:self.uc.mem_write(s['va'],bytes(4096))
        self.uc.mem_write(0x70000000,b'\xa5'*65536);self.return_slots={};self.pending=None
        for r,v in zip(REG,seedregs(seed)):self.uc.reg_write(r,v)
        self.uc.reg_write(x.UC_X86_REG_ESP,SP);self.uc.reg_write(x.UC_X86_REG_EFLAGS,flags);self.mask=ALL
    def snapshot(self):return b''.join(bytes(self.uc.mem_read(s['va'],4096)) for s in data)
    def code_hook(self,uc,pc,size,_):
        if pc==SENTINEL:uc.emu_stop();return
        if allowed.get(pc)!=size:raise RuntimeError(f'Escaped guarded original CFG at {pc:#x}')
        self.total_steps+=1;self.trace.append(pc);self.addresses.add(pc)
        if pc in entries:self.counts[entries.index(pc)]+=1
        b=bytes(uc.mem_read(pc,size));op=b[0]
        if op==0xe8:self.pending=(uc.reg_read(x.UC_X86_REG_ESP)-4,pc+size)
        if op in (0x03,0x2b,0x3b,0x3d):self.mask=ALL
        elif op in (0x33,0x32,0x0b,0x0a,0x85,0x84):self.mask=LOGIC
        elif op in (0x81,0x83):self.mask=ALL if (b[1]>>3)&7 in (0,5,7) else LOGIC
        elif 0x40<=op<=0x4f:self.mask=(self.mask&1)|(ALL&~1)
        elif op==0xf7:
            group=(b[1]>>3)&7;self.mask=ALL if group==3 else 0x801 if group==5 else 0
        elif op==0x0f and b[1]==0xaf:self.mask=0x801
        elif op in (0xc1,0xd1):
            count=(b[-1] if op==0xc1 else 1)&31
            if count:self.mask=LOGIC if count==1 else LOGIC&~0x800
    def write_hook(self,uc,access,addr,size,value,_):
        if not self.active:return
        if 0x70000000<=addr and addr+size<=0x70010000:
            for p in list(self.return_slots):
                if addr<p+4 and p<addr+size:del self.return_slots[p]
            if self.pending is not None:
                assert (addr,size,value&U32)==(self.pending[0],4,self.pending[1]);self.return_slots[addr]=self.pending[1];self.pending=None
            return
        allowed_writes={state[k] for k in ('feedback','positive','negative','negative_cooldown','positive_cooldown')}
        if addr not in allowed_writes or size!=4:raise RuntimeError(f'Unexpected data write {addr:#x}/{size}')
        self.writes+=1;self.changed+=self.readword(addr)!=(value&U32)
    def call(self):
        if self.readword(state['gate'])!=1:raise ValueError('Manual gate=1 contract refused before guest execution')
        assert self.uc.reg_read(x.UC_X86_REG_ESP)==SP
        self.word(SP-4,SENTINEL);self.return_slots[SP-4]=SENTINEL;self.uc.reg_write(x.UC_X86_REG_ESP,SP-4)
        self.trace=[];self.counts=[0]*4;self.active=True
        try:self.uc.emu_start(entries[0],SENTINEL,timeout=100000,count=500)
        finally:self.active=False
        assert self.uc.reg_read(x.UC_X86_REG_EIP)==SENTINEL and self.uc.reg_read(x.UC_X86_REG_ESP)==SP
        self.paths.add(tuple(self.trace))
        stack=list(struct.unpack('<32I',bytes(self.uc.mem_read(SP-128,128))))
        markers=[self.return_slots.get(SP-128+4*i,0) for i in range(32)]
        assert all(m==0 or m==stack[i] for i,m in enumerate(markers))
        return [self.uc.reg_read(r) for r in REG]+[self.uc.reg_read(x.UC_X86_REG_EFLAGS),self.mask]+stack+markers+self.counts
def main():
    e=Engine()
    if '--negative' in sys.argv:
        e.word(state['gate'],0)
        try:e.call();raise AssertionError('gate=0 accepted')
        except ValueError:pass
        result=dict(negative_gate=0,rejected=True,guest_instructions=e.total_steps,passed=e.total_steps==0)
        (ROOT/'evidence/stage5-unicorn-negative.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));return 0
    scores=[0,15,16,25,26,35,36,49,50,60,61,70,71,100,0xffffffff,0x7fffffff]
    path=ROOT/'samples/stage5-unicorn-cases.bin';calls=0;rows=[]
    with path.open('wb') as f:
        f.write(struct.pack('<4I',0x53544735,256,90,16384))
        for seq in range(8):
            for turn in range(32):
                score=scores[(turn//2+seq)%16];metric=[0,199999,200000,200001][turn%4] if seq==5 else 0
                if seq==6:score=[0x80000000,0x7fffffff][turn%2]
                restriction=4 if seq==4 and turn%4<2 else 0
                reset=int(turn==0);seed=seq+1;feedback=[0,100,0xfffffff8,0x80000000][seq%4];cool=[0,1,2,5][seq%4]
                inputs=[reset,seed,score,metric,restriction,feedback,17,23,cool,cool,0x3202,1]
                if reset:
                    e.reset(seed,inputs[10])
                    for k,v in zip(('feedback','positive','negative','negative_cooldown','positive_cooldown'),inputs[5:10]):e.word(state[k],v)
                for k,v in zip(('score','metric','restriction'),inputs[2:5]):e.word(state[k],v)
                e.word(state['gate'],inputs[11]);output=e.call();calls+=1
                record=inputs+output;assert len(record)==90;f.write(struct.pack('<90I',*record));f.write(e.snapshot())
                rows.append(dict(sequence=seq,turn=turn,score=score,metric=metric,restriction=restriction,feedback=e.readword(state['feedback']),positive=e.readword(state['positive']),negative=e.readword(state['negative']),negative_cooldown=e.readword(state['negative_cooldown']),positive_cooldown=e.readword(state['positive_cooldown']),original_entry_counts=e.counts))
    (ROOT/'evidence/stage5-state-transitions.json').write_text(json.dumps(rows,indent=2))
    (ROOT/'evidence/stage5-unicorn-paths.json').write_text(json.dumps([[hex(v) for v in p] for p in sorted(e.paths)],indent=2))
    result=dict(passed=True,calls=calls,sequences=8,rounds_per_sequence=32,unique_paths=len(e.paths),instructions_reached=len(e.addresses),guarded_instructions=128,guest_instructions=e.total_steps,guest_data_writes=e.writes,state_changes=e.changed,fixture=str(path),fixture_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),data_bytes_per_round=16384,stack_bytes_per_round=128,guest_budget_per_call=500,timeout_microseconds_per_call=100000,city_save_loaded=False,original_scheduler_executed=False,manual_gate_contract=1,real_city_reachability_proved=False)
    (ROOT/'evidence/stage5-unicorn-results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));return 0
if __name__=='__main__':raise SystemExit(main())
