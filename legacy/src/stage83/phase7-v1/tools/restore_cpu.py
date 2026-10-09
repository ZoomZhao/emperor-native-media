"""Four official CPU restores; only 12 authored instructions in empty clones."""
from core import *
import struct
def main():
 start=time.monotonic();s=json.loads((V/'configs/source.json').read_text());verify({str(SOURCE/n):h for n,h in PINS.items()});h=HostStore(ROOT/s['host_manifest']);u=fixed()
 from stage73_cpu import restore,public_state
 from unicorn import x86_const as x
 threads=h['guest_threads20'];current=h['current_tid'];raw=(SOURCE/'context.bin').read_bytes();uc,ctx=restore(u,raw,s['context'])
 with(SOURCE/'memory.bin').open('rb')as f:
  for r in s['regions']:
   n=r['high']-r['low']+1;f.seek(r['offset']);b=f.read(n)
   if len(b)!=n or hashlib.sha256(b).hexdigest()!=r['sha256']:raise Boundary('REGION_HASH')
   uc.mem_map(r['low'],n,r['permissions']);uc.mem_write(r['low'],b)
 uc.context_restore(ctx);before=uc.context_save().__getstate__()[0];rows={}
 from unicorn.unicorn_py3.unicorn import UcContext
 for tid,t in sorted(threads.items()):
  b=raw if tid==current else t['context'][0];c=UcContext.__new__(UcContext);c.__setstate__((b,5608,4,4));uc.context_restore(c);p=public_state(uc,u)
  if uc.context_save().__getstate__()[0][40:]!=b[40:]:raise Boundary('OFFICIAL_CPU_PAYLOAD '+str(tid))
  if p['FS']!=24+8*(tid-1)or p['FS_BASE']!=t['teb']:raise Boundary('PUBLIC_FS_TEB '+str(tid))
  if tid==current and any(p[k.upper()]!=v for k,v in s['registers'].items()):raise Boundary('CURRENT_CPU_REGISTERS')
  clone,cc=restore(u,b,s['context']);code=0x6f000000;stack=code+4096;marker=0x71400000+tid
  clone.mem_map(code,8192,7);clone.mem_map(t['teb'],4096,3);clone.mem_write(t['teb']+0x18,struct.pack('<I',marker));body=bytes.fromhex('64a118000000505b');clone.mem_write(code,body);clone.mem_protect(code,4096,5);clone.reg_write(x.UC_X86_REG_EIP,code);clone.reg_write(x.UC_X86_REG_ESP,stack+2048)
  seen=[]
  def hook(e,pc,n,data):
   if not code<=pc<code+len(body):raise Boundary('AUTHORED_CLONE_PC_ONLY')
   seen.append(pc)
  clone.hook_add(u.UC_HOOK_CODE,hook);clone.emu_start(code,code+len(body),timeout=100000,count=3)
  if len(seen)!=3 or clone.reg_read(x.UC_X86_REG_EAX)!=marker or clone.reg_read(x.UC_X86_REG_EBX)!=marker or clone.reg_read(x.UC_X86_REG_ESP)!=stack+2048:raise Boundary('AUTHORED_FS_SS_PROBE')
  rows[str(tid)]={'public_CPU':p,'CPU_payload_sha256':hashlib.sha256(b[40:]).hexdigest(),'entire_payload_equal':True,'status':t['status'],'tls':t['tls'],'last_error':t['last_error'],'authored_FS_SS_probe':{'instructions':3,'bytes':body.hex(),'marker':marker,'passed':True}}
  del clone,cc
 uc.context_restore(ctx)
 if uc.context_save().__getstate__()[0][40:]!=before[40:]:raise Boundary('READ_ONLY_CURRENT_CONTEXT_CHANGED')
 memory_equal=True
 for r in s['regions']:
  if hashlib.sha256(bytes(uc.mem_read(r['low'],r['high']-r['low']+1))).hexdigest()!=r['sha256']:memory_equal=False
 if not memory_equal:raise Boundary('READ_ONLY_MEMORY_CHANGED')
 put(V/'evidence/cpu-restore.json',{'status':'FOUR_OFFICIAL_CPU_RESTORES_AND_AUTHORED_FS_SS_PROBES_PASS','thread_records':rows,'current_tid_preserved':current,'mapped_memory_bytes':s['memory_bytes'],'all_memory_hashes_unchanged':True,'source_pins':PINS,'fixed_unicorn_sha256':LIB_SHA,'original_engine_emu_start_calls':0,'authored_empty_clone_calls':4,'authored_instructions':12,'original_instructions':0,'GS_BASE_read_in_32bit_is_not_verified':True,'seconds':time.monotonic()-start})
 print(json.dumps({'status':'FOUR_CPUS_AND_FS_SS_PASS','original_instructions':0,'authored_instructions':12,'memory_hashes_equal':True,'public_PC_SP':{tid:[hex(r['public_CPU']['EIP']),hex(r['public_CPU']['ESP'])]for tid,r in rows.items()},'seconds':time.monotonic()-start}))
if __name__=='__main__':main()
