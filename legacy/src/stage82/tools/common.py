"""Stage82 data protocol. No engine import, execution, or approval on import."""
import collections, hashlib, json, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
S=ROOT/'stage82'
CORE=('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI','EIP','EFLAGS')
API={1:('HeapAlloc',3),2:('HeapReAlloc',4),3:('HeapFree',3),4:('HeapSize',3),5:('GetCurrentThreadId',0),6:('GetLastError',0),7:('SetLastError',1),8:('TlsGetValue',1),9:('TlsSetValue',2),10:('InitializeCriticalSection',1),11:('EnterCriticalSection',1),12:('LeaveCriticalSection',1),13:('memcpy',3),14:('memmove',3),15:('memset',3)}
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def put(p,x):
 with Path(p).open('x')as f:json.dump(x,f,indent=2);f.write('\n')
def decode(v):
 if isinstance(v,list):return [decode(x)for x in v]
 if not isinstance(v,dict):return v
 if set(v)=={'dict'}:return {decode(k):decode(x)for k,x in v['dict']}
 if set(v)=={'tuple'}:return tuple(decode(x)for x in v['tuple'])
 if set(v)=={'bytes'}:return bytes.fromhex(v['bytes'])
 if set(v)=={'set'}:return set(decode(x)for x in v['set'])
 if set(v)=={'deque','maxlen'}:return collections.deque(map(decode,v['deque']),maxlen=v['maxlen'])
 raise ValueError('Unknown typed checkpoint value')
def encode(v):
 if isinstance(v,dict):return {'dict':[[encode(k),encode(x)]for k,x in v.items()]}
 if isinstance(v,bytes):return {'bytes':v.hex()}
 if isinstance(v,tuple):return {'tuple':list(map(encode,v))}
 if isinstance(v,set):return {'set':list(map(encode,v))}
 if isinstance(v,collections.deque):return {'deque':list(map(encode,v)),'maxlen':v.maxlen}
 if isinstance(v,list):return list(map(encode,v))
 return v
def verify(pins):
 for p,h in pins.items():
  path=Path(p)if Path(p).is_absolute()else ROOT/p
  if sha(path)!=h:raise RuntimeError('PIN_MISMATCH '+str(path))
def exclusive_dir(p):
 p=Path(p).resolve()
 if not p.is_relative_to(S/'runs') or p==S/'runs':raise RuntimeError('Output outside Stage82 runs')
 p.mkdir(parents=True,exist_ok=False);return p
def heap_binary(host,regions):
 heaps=sorted(host['heaps'].items());rows=sorted(host['allocations'].items())
 hr=[r for r in regions if 0x10000000<=r['low']<0x20000000]
 b=b'S77H'+struct.pack('<6I',1,len(heaps),len(hr),len(rows),host['heap_next'],host['heap_mapped_end34'])
 b+=b''.join(struct.pack('<II',h,v['options'])for h,v in heaps)
 b+=b''.join(struct.pack('<III',r['low'],r['high']+1,r['permissions'])for r in hr)
 b+=b''.join(struct.pack('<III',p,h,n)for p,(h,n)in rows)
 return b
def lock_binary(host):
 rows=sorted(host['critical_sections'].items());b=b'S82L'+struct.pack('<I',len(rows))
 return b+b''.join(struct.pack('<III',p,depth,host['thread_owners'][p]or 0)for p,depth in rows)
def transfers(host,public,opaque):
 b=b'S80T'+struct.pack('<III',1,4,host['current_tid'])
 for tid,t in sorted(host['guest_threads20'].items()):
  p=public[tid];cur=tid==host['current_tid'];tls=host['tls']if cur else t['tls'];error=host['last_error']if cur else t['last_error']
  if len(opaque[tid])!=5608 or any(not 0<=i<64 for i in tls):raise RuntimeError('CPU/TLS domain')
  b+=struct.pack('<4IQ',tid,{'runnable':1,'waiting':2,'exited':3}[t['status']],error,t['teb'],sum(1<<i for i in tls))+struct.pack('<64I',*(tls.get(i,0)for i in range(64)))+struct.pack('<10I',*(p[k]for k in CORE))+struct.pack('<6I',*(p[k]for k in ('CS','DS','SS','ES','FS','GS')))+struct.pack('<I',p['CR0'])
  for name in ('GDTR','LDTR'):b+=struct.pack('<4Q',*p[name])
  b+=struct.pack('<2Q3I',p['FS_BASE'],p.get('GS_BASE',0),p['FPCW'],p['FPSW'],p['FPTAG'])
  for m,e in p['FP80']:b+=struct.pack('<QH',m,e)
  b+=opaque[tid]
 return b
def region_binary(regions):return b'S79M'+struct.pack('<I',len(regions))+b''.join(struct.pack('<IIQQ',r['low'],r['permissions'],r['high']+1,r['offset'])for r in regions)
def api_binary(host,contracts):
 rows=[]
 for r in contracts:
  matches=[p for p,(dll,name)in host['imports'].items()if dll.lower()=='kernel32.dll'and name==r['name']]
  if len(matches)!=1:raise RuntimeError('AMBIGUOUS_API_IMPORT '+r['name'])
  rows.append((r['site'],r['slot'],matches[0],r['reg']))
 return b'S82I'+struct.pack('<I',len(rows))+b''.join(struct.pack('<4I',*r)for r in rows)
class Boundary(RuntimeError):pass
