"""Authored x86 bytes only; creates source state, executes no instruction."""
import argparse,json,sys
sys.dont_write_bytecode=True
from common import ROOT,S,exclusive_dir
from engine import fixed,Memory,public,snapshot
def main():
 p=argparse.ArgumentParser();p.add_argument('--config',default=str(S/'configs/authored.json'));p.add_argument('--out',default=str(S/'runs/fixture-input'));a=p.parse_args();c=json.loads(open(a.config).read());u=fixed(c);from unicorn import x86_const as x
 uc=u.Uc(4,4);regions=[{'low':0x10000000,'high':0x10ffffff,'permissions':3,'offset':0},{'low':0x70000000,'high':0x70000fff,'permissions':3,'offset':0},{'low':0xa0000000,'high':0xa0000fff,'permissions':5,'offset':0},{'low':0xa1000000,'high':0xa1000fff,'permissions':3,'offset':0}]
 for r in regions:uc.mem_map(r['low'],r['high']-r['low']+1,r['permissions'])
 uc.mem_write(c['caller'],bytes.fromhex(c['caller_bytes']));uc.mem_write(c['entry'],b''.join(bytes.fromhex(r['bytes'])for r in c['instructions']));uc.mem_write(0xa1000000,(0xfffffff9).to_bytes(4,'little'));uc.reg_write(x.UC_X86_REG_EIP,c['caller']);uc.reg_write(x.UC_X86_REG_ESP,0x70000ff0);uc.reg_write(x.UC_X86_REG_EFLAGS,0x202)
 ctx=uc.context_save().__getstate__();pub=public(uc);threads={i:{'status':'runnable'if i==1 else 'waiting','context':ctx,'tls':{0:0x10000000},'last_error':2,'teb':0}for i in range(1,5)}
 host={'current_tid':1,'guest_threads20':threads,'tls':{0:0x10000000},'last_error':2,'heaps':{0x3001:{'options':0}},'allocations':{},'heap_next':0x10000000,'heap_mapped_end34':0x11000000,'critical_sections':{},'thread_owners':{},'imports':{int(k):(v[0],v[1])for k,v in c.get('fixture_imports',{}).items()},'count':123,'thread_switches':17,'watch_state54':{'phase':22,'entries':17}}
 out=exclusive_dir(a.out);m=Memory(uc,regions,None,0);snapshot(uc,host,m,out,{'kind':'AUTHORED_NO_ORIGINAL_BYTES','instructions_executed':0},{i:pub for i in range(1,5)});print('Authored input frozen; game mapping/context/instructions=0')
if __name__=='__main__':main()
