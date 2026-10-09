"""One main thread. Used only behind pipeline approval or authored-fixture gate."""
import copy,json,struct,time
from pathlib import Path
from common import ROOT,S,CORE,API,Boundary,sha,put,decode,encode,heap_binary,lock_binary,transfers,region_binary,api_binary
from platform_model import Platform
def fixed(c):
 import sys
 sys.path.insert(0,str(ROOT/'samples'))
 from stage73_cpu import load_fixed
 return load_fixed(ROOT,c)
def public(uc):
 import sys
 sys.path.insert(0,str(ROOT/'samples'))
 from stage73_cpu import public_state
 import unicorn
 return public_state(uc,unicorn)
def restore(u,b,meta):
 from unicorn.unicorn_py3.unicorn import UcContext
 if len(b)!=5608 or meta['size']!=5608 or (meta['arch'],meta['mode'])!=(4,4):raise Boundary('OPAQUE_ABI')
 ctx=UcContext.__new__(UcContext);ctx.__setstate__((b,5608,4,4));uc=u.Uc(4,4);uc.context_restore(ctx);return uc,ctx
class Memory:
 def __init__(self,uc,regions,trace,cap):self.uc=uc;self.regions=copy.deepcopy(regions);self.trace=trace;self.cap=cap;self.records=0;self.pc=0
 def check(self,a,n,p):
  if a<0 or n<0 or a+n>0x100000000:raise Boundary('ADDRESS_WRAP')
  end=a+n;pos=a
  for r in self.regions:
   if r['low']<=pos<=r['high']:
    if r['permissions']&p!=p:raise Boundary('MEMORY_PERMISSION')
    pos=min(end,r['high']+1)
    if pos==end:return
  if not n:return
  raise Boundary('UNMAPPED_RANGE')
 def read(self,a,n):self.check(a,n,1);return bytes(self.uc.mem_read(a,n))if n else b''
 def note(self,kind,a,n,v,api=0):
  if n not in (1,2,4)or self.records>=self.cap:raise Boundary('TRACE_WIDTH_OR_CAP')
  self.trace.write(struct.pack('<6I',kind,self.pc,a,n,v&((1<<(8*n))-1),api));self.records+=1
 def write(self,a,b,api):
  self.check(a,len(b),2)
  if (len(b)+3)//4>self.cap-self.records:raise Boundary('PLATFORM_WRITE_BUDGET_BEFORE_STORE')
  for i in range(0,len(b),4):
   q=b[i:i+4];self.uc.mem_write(a+i,q);self.note(1,a+i,len(q),int.from_bytes(q,'little'),api)
 def grow(self,a,end):
  if a&0xffffff or end&0xffffff or not 0x10000000<=a<end<=0x20000000 or any(a<=r['high']and r['low']<end for r in self.regions):raise Boundary('HEAP_MAPPING')
  self.uc.mem_map(a,end-a,3);self.regions.append({'low':a,'high':end-1,'permissions':3,'offset':0});self.regions.sort(key=lambda r:r['low'])
def snapshot(uc,host,m,out,source,public_workers,contracts=()):
 import unicorn
 cur=host['current_tid'];opaque={};pub=dict(public_workers);pub[cur]=public(uc);opaque[cur]=uc.context_save().__getstate__()[0]
 for tid,t in host['guest_threads20'].items():
  if tid!=cur:opaque[tid]=t['context'][0]
 regions=[];offset=0
 with(out/'memory.bin').open('xb')as f:
  for r in m.regions:
   n=r['high']-r['low']+1;h=__import__('hashlib').sha256()
   for i in range(0,n,1<<20):b=bytes(uc.mem_read(r['low']+i,min(n-i,1<<20)));f.write(b);h.update(b)
   regions.append(dict(r,offset=offset,sha256=h.hexdigest()));offset+=n
 for tid,b in opaque.items():(out/('context-%d.bin'%tid)).write_bytes(b)
 (out/'threads.bin').write_bytes(transfers(host,pub,opaque));(out/'regions.bin').write_bytes(region_binary(regions));(out/'heap.bin').write_bytes(heap_binary(host,regions));(out/'locks.bin').write_bytes(lock_binary(host));(out/'api-sites.bin').write_bytes(api_binary(host,contracts))
 record=dict(format='S82_FROZEN_NATURAL_SNAPSHOT_V1',regions=regions,memory_bytes=offset,host=encode(host),public_CPU={str(k):v for k,v in pub.items()},current_tid=cur,context={'size':5608,'arch':4,'mode':4},source=source,GS_BASE_unknown=True,opaque_files_are_current_engine_contexts=True)
 put(out/'snapshot.json',record)
 names={'memory.bin','snapshot.json','threads.bin','regions.bin','heap.bin','locks.bin','api-sites.bin','writes.bin','api-events.bin'}
 pins={str(p.relative_to(ROOT)):sha(p)for p in out.iterdir()if p.is_file()and(p.name in names or p.name.startswith('context-'))};put(out/'frozen.json',{'files':pins,'source':source,'no_PC_SP_or_activity_mutation':True});return record
def load_snapshot(c,source,u):
 p=Path(source);s=json.loads(p.read_text());root=p.parent
 if s.get('format')=='S82_FROZEN_NATURAL_SNAPSHOT_V1':
  from common import verify
  verify(json.loads((root/'frozen.json').read_text())['files']);h=decode(s['host']);cur=h['current_tid'];b=(root/('context-%d.bin'%cur)).read_bytes();meta=s['context'];workers={int(k):v for k,v in s['public_CPU'].items()}
 else:
  h=decode(s['host']);cur=h['current_tid'];b=(root/'context.bin').read_bytes();meta=s['context'];workers={int(k):v['public_CPU']for k,v in json.loads((ROOT/'stage80/evidence/measured-state.json').read_text())['thread_records'].items()}
 if cur!=1 or set(h['guest_threads20'])!={1,2,3,4}or any(t['status']!='waiting'for tid,t in h['guest_threads20'].items()if tid!=cur):raise Boundary('ONLY_MAIN_AND_THREE_WAITING_WORKERS')
 uc,ctx=restore(u,b,meta)
 with(root/'memory.bin').open('rb')as f:
  for r in s['regions']:
   n=r['high']-r['low']+1;uc.mem_map(r['low'],n,r['permissions']);f.seek(r['offset']);body=f.read(n)
   if len(body)!=n or __import__('hashlib').sha256(body).hexdigest()!=r['sha256']:raise Boundary('SOURCE_REGION_HASH')
   uc.mem_write(r['low'],body)
 uc.context_restore(ctx)
 if uc.context_save().__getstate__()[0][40:]!=b[40:]:raise Boundary('ENTIRE_OPAQUE_CPU_PAYLOAD_MISMATCH')
 actual=public(uc)
 if s.get('format')=='S82_FROZEN_NATURAL_SNAPSHOT_V1':
  if actual!=workers[cur]:raise Boundary('RESTORED_PUBLIC_CPU_MISMATCH')
 else:
  if any(actual[k.upper()]!=v for k,v in s['registers'].items()):raise Boundary('ROOT_CORE_MISMATCH')
 return uc,h,s,workers
def original_bytes(path):
 b=Path(path).read_bytes();pe=struct.unpack_from('<I',b,60)[0];n=struct.unpack_from('<H',b,pe+6)[0];opt=struct.unpack_from('<H',b,pe+20)[0];base=struct.unpack_from('<I',b,pe+24+28)[0];rows=[]
 for i in range(n):
  q=pe+24+opt+40*i;vs,va,size,off=struct.unpack_from('<4I',b,q+8);flags=struct.unpack_from('<I',b,q+36)[0]
  if flags&0x20000000:rows.append((base+va,base+va+size,off))
 def read(pc,n):
  for lo,hi,off in rows:
   if lo<=pc and pc+n<=hi:return b[off+pc-lo:off+pc-lo+n]
  raise Boundary('PC_OUTSIDE_ORIGINAL_EXECUTABLE_SECTIONS')
 return read
def indirect(uc,body,m):
 """Strict 32bit FF /2,/4 operand decoding; unsupported address prefix stops."""
 from unicorn import x86_const as x
 if body[:1]!=b'\xff':return None
 modrm=body[1];kind=(modrm>>3)&7
 if kind not in (2,4):return None
 regs=[uc.reg_read(getattr(x,'UC_X86_REG_'+n))for n in CORE[:8]];mode=modrm>>6;rm=modrm&7;at=2
 if mode==3:return regs[rm],None
 disp=0
 if rm==4:
  sib=body[at];at+=1;scale=sib>>6;index=(sib>>3)&7;base=sib&7;disp=0 if index==4 else regs[index]<<scale
  if mode==0 and base==5:disp+=struct.unpack_from('<I',body,at)[0];at+=4
  else:disp+=regs[base]
 elif mode==0 and rm==5:disp=struct.unpack_from('<I',body,at)[0];at+=4
 else:disp=regs[rm]
 if mode==1:disp+=struct.unpack_from('<b',body,at)[0]
 if mode==2:disp+=struct.unpack_from('<i',body,at)[0]
 slot=disp&0xffffffff;return int.from_bytes(m.read(slot,4),'little'),slot
def run(c,role,source,out):
 u=fixed(c);from unicorn import x86_const as x
 uc,h,s,workers=load_snapshot(c,source,u);initial=public(uc);limits=c['limits'][role];cap=limits['steps'];deadline=time.monotonic()+limits['guest_seconds'];count=0;last=None;caller_sp=None;stopped=False;failure=[];apirows=[];virtual_visits=0
 original=c['kind']=='original';allowed={r['pc']:r for r in c['instructions']};domains=c['conditional_domains'];imports=h['imports'];nameid={name:(id,n)for id,(name,n)in API.items()};readoriginal=original_bytes(c['original_EXE'])if original else None
 with(out/'writes.bin').open('xb')as f:
  m=Memory(uc,s['regions'],f,limits['writes']);platform=Platform(h,m)
  def stop(message):failure.append(message);uc.emu_stop()
  def code(uc,pc,size,data):
   nonlocal count,last,caller_sp,stopped,virtual_visits
   try:
    if time.monotonic()>deadline:raise Boundary('GUEST_WALL_CAP')
    if role=='capture'and pc==c['entry']:
     sp=uc.reg_read(x.UC_X86_REG_ESP)
     if last!=c['caller']or caller_sp!=sp+4 or int.from_bytes(m.read(sp,4),'little')!=c['return']:raise Boundary('NOT_GENUINE_NATURAL_CALL_STACK')
     stopped=True;uc.emu_stop();return
    if role=='reference'and pc==c['return']:
     if uc.reg_read(x.UC_X86_REG_ESP)!=initial['ESP']+4:raise Boundary('WRONG_COMPLETION_SP')
     stopped=True;uc.emu_stop();return
    if pc in imports:
     dll,name=imports[pc]
     if name not in nameid or dll.lower()not in ('kernel32.dll','msvcrt.dll','authored.fixture'):raise Boundary('UNIMPLEMENTED_API '+dll+'!'+name)
     id,n=nameid[name];sp=uc.reg_read(x.UC_X86_REG_ESP);raw=m.read(sp,4+4*n);ret,*args=struct.unpack('<'+str(n+1)+'I',raw);result=platform.invoke(id,args,uc.reg_read(x.UC_X86_REG_EAX));uc.reg_write(x.UC_X86_REG_EAX,result);uc.reg_write(x.UC_X86_REG_ESP,sp+4+(0 if id>=13 else 4*n));uc.reg_write(x.UC_X86_REG_EIP,ret)
     apirows.append([id,m.pc,n,*args,*([0]*(4-n)),result,h['last_error'],h['heap_next'],len(h['allocations'])]);return
    body=bytes(uc.mem_read(pc,size))
    if original:
     if readoriginal(pc,size)!=body:raise Boundary('ORIGINAL_INSTRUCTION_BYTES_CHANGED')
    if role=='reference'and (pc not in allowed or bytes.fromhex(allowed[pc]['bytes'])!=body):raise Boundary('OUTSIDE_COMPLETE_PHASE_CLOSURE')
    if body[:1]in (b'\x67',b'\xea',b'\x9a'):raise Boundary('UNSUPPORTED_ADDRESS_OR_FAR_TRANSFER')
    pref=0
    while pref<len(body)and body[pref]in (0x26,0x2e,0x36,0x3e,0x64,0x65,0x66,0x67,0xf0,0xf2,0xf3):pref+=1
    if pref<len(body)and body[pref]==0xff and ((body[pref+1]>>3)&7)in (2,3,4,5):
     if pref or ((body[pref+1]>>3)&7)in (3,5):raise Boundary('UNSUPPORTED_PREFIXED_OR_FAR_INDIRECT_TRANSFER')
    m.pc=pc;ind=indirect(uc,body,m)
    if ind and ind[0]not in imports:
     target,slot=ind;good=False
     for row in domains:
      if row['site']==pc and row['target']==target and uc.reg_read(getattr(x,'UC_X86_REG_'+CORE[row['reg']]))==row['value']:
       m.check(row['slot'],4,1)
       if any(r['low']<=row['slot']and row['slot']+4<=r['high']+1 and r['permissions']&2 for r in m.regions):raise Boundary('MUTABLE_TARGET_SLOT')
       good=int.from_bytes(m.read(row['slot'],4),'little')==target
       if good:virtual_visits+=1;break
     if not good:raise Boundary('UNKNOWN_DYNAMIC_DOMAIN '+hex(pc))
    if count>=cap:raise Boundary('INSTRUCTION_CAP')
    count+=1;last=pc
    if pc==c['caller']:
     if body!=bytes.fromhex(c['caller_bytes']):raise Boundary('CALLER_BYTES')
     caller_sp=uc.reg_read(x.UC_X86_REG_ESP)
   except(Exception)as e:stop(str(e))
  def write(uc,access,a,n,v,data):
   try:m.check(a,n,2);m.note(0,a,n,v)
   except Exception as e:stop(str(e))
  def invalid(uc,access,a,n,v,data):stop('INVALID_MEMORY '+str((access,a,n)));return False
  uc.hook_add(u.UC_HOOK_CODE,code);uc.hook_add(u.UC_HOOK_MEM_WRITE,write);uc.hook_add(u.UC_HOOK_MEM_INVALID,invalid)
  try:uc.emu_start(initial['EIP'],0,timeout=int(limits['guest_seconds']*1000000),count=cap+1024)
  except u.UcError as e:failure.append('ENGINE '+str(e))
 final=public(uc)
 with(out/'api-events.bin').open('xb')as f:
  for row in apirows:f.write(struct.pack('<11I',*row))
 if not stopped or failure:
  put(out/'result.json',{'status':'BOUNDARY_NOT_SUCCESS','role':role,'steps':count,'failure':failure or ['NO_EXACT_STOP'],'PC':final['EIP'],'initial_CPU':initial,'final_CPU':final,'last_completed_PC':last,'write_records':m.records,'API_events':len(apirows),'new_original_instructions':count if original else 0});raise Boundary(str(failure or 'NO_EXACT_STOP'))
 snap=snapshot(uc,h,m,out,{'configuration_kind':c['kind'],'role':role,'input_sha256':sha(source)},workers,c.get('named_sites',()))
 result={'status':'NATURAL_ENTRY_CAPTURED'if role=='capture'else 'REFERENCE_EXACT_RETURN','role':role,'initial_CPU':initial,'final_CPU':final,'steps':count,'write_records':m.records,'API_events':len(apirows),'conditional_target_visits':virtual_visits,'natural_call':{'caller':c['caller'],'entry':c['entry'],'return':c['return'],'caller_SP':caller_sp,'entry_SP':final['ESP']}if role=='capture'else None,'new_original_instructions':count if original else 0,'snapshot_sha256':sha(out/'snapshot.json'),'workers_scheduled':0,'input_fields_modified_before_execution':0}
 put(out/'result.json',result);return result
