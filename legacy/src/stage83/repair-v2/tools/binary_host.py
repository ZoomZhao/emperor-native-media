"""Data-only typed binary codec and lazy host field manifests; never pickle."""
import array,collections,hashlib,json,struct,sys
from pathlib import Path
from collections.abc import Mapping,MutableMapping
from common import ROOT,sha,put
MAGIC=b'S83H\x02\0\0\0'
MUTABLE={'allocations','heap_next','heap_mapped_end34','heaps','tls','last_error','critical_sections','thread_owners','watch_state54','dd_native_blob','dd_calls'}
def dump(value,path):
 with Path(path).open('xb')as f:
  f.write(MAGIC)
  def emit(v):
   if v is None:f.write(b'n');return
   if isinstance(v,bool):f.write(b'y'if v else b'z');return
   if isinstance(v,int):
    if 0<=v<=0xffffffff:f.write(b'i'+struct.pack('<I',v))
    elif -0x8000000000000000<=v<=0x7fffffffffffffff:f.write(b'I'+struct.pack('<q',v))
    else:b=str(v).encode();f.write(b'G'+struct.pack('<Q',len(b))+b)
    return
   if isinstance(v,float):f.write(b'f'+struct.pack('<d',v));return
   if isinstance(v,(bytes,str)):
    b=v if isinstance(v,bytes)else v.encode('utf8');f.write((b'b'if isinstance(v,bytes)else b's')+struct.pack('<Q',len(b)));f.write(b);return
   if isinstance(v,Mapping):
    f.write(b'd'+struct.pack('<Q',len(v)))
    for k,x in v.items():emit(k);emit(x)
    return
   if isinstance(v,list)and v and all(type(x)is int and 0<=x<=0xffffffff for x in v):
    a=array.array('I',v)
    if sys.byteorder!='little':a.byteswap()
    f.write(b'a'+struct.pack('<Q',len(v)));a.tofile(f);return
   if isinstance(v,(list,tuple,set,collections.deque)):
    tag=b'l'if isinstance(v,list)else b't'if isinstance(v,tuple)else b'e'if isinstance(v,set)else b'q';f.write(tag+struct.pack('<Q',len(v)))
    if tag==b'q':f.write(struct.pack('<q',-1 if v.maxlen is None else v.maxlen))
    for x in v:emit(x)
    return
   raise TypeError(type(v))
  emit(value)
 return {'sha256':sha(path),'bytes':Path(path).stat().st_size}
def load(path,expected=None):
 path=Path(path)
 if expected and sha(path)!=expected:raise ValueError('BINARY_CONTENT_HASH_MISMATCH')
 with path.open('rb')as f:
  size=path.stat().st_size
  def read(n):
   if n<0 or n>size or n>size-f.tell():raise ValueError('BINARY_LENGTH_OR_TRUNCATION')
   b=f.read(n)
   if len(b)!=n:raise ValueError('BINARY_TRUNCATION')
   return b
  if read(8)!=MAGIC:raise ValueError('BINARY_VERSION')
  def item(depth=0):
   if depth>256:raise ValueError('BINARY_DEPTH')
   tag=read(1)
   if tag==b'n':return None
   if tag in(b'y',b'z'):return tag==b'y'
   if tag==b'i':return struct.unpack('<I',read(4))[0]
   if tag in(b'I',b'f'):return struct.unpack('<q'if tag==b'I'else'<d',read(8))[0]
   n=struct.unpack('<Q',read(8))[0]
   if n>size:raise ValueError('BINARY_COUNT')
   if tag in(b'b',b's',b'G'):
    b=read(n);return b if tag==b'b'else b.decode('utf8')if tag==b's'else int(b)
   if tag==b'a':
    b=read(n*4);a=array.array('I');a.frombytes(b)
    if sys.byteorder!='little':a.byteswap()
    return list(a)
   if tag==b'd':
    out={}
    for _ in range(n):
     k=item(depth+1)
     if k in out:raise ValueError('BINARY_DUPLICATE_KEY')
     out[k]=item(depth+1)
    return out
   if tag not in(b'l',b't',b'e',b'q'):raise ValueError('BINARY_TAG')
   cap=struct.unpack('<q',read(8))[0]if tag==b'q'else None
   if cap is not None and cap< -1:raise ValueError('BINARY_DEQUE_MAXLEN')
   a=[item(depth+1)for _ in range(n)]
   return a if tag==b'l'else tuple(a)if tag==b't'else set(a)if tag==b'e'else collections.deque(a,maxlen=None if cap==-1 else cap)
  value=item()
  if f.read(1):raise ValueError('BINARY_TRAILING_DATA')
  return value
class FrozenDict(dict):
 def deny(self,*a,**k):raise TypeError('IMMUTABLE_HOST_FIELD')
 __setitem__=__delitem__=clear=pop=popitem=setdefault=update=__ior__=deny
class FrozenList(list):
 def deny(self,*a,**k):raise TypeError('IMMUTABLE_HOST_FIELD')
 __setitem__=__delitem__=append=extend=insert=pop=remove=clear=sort=reverse=__iadd__=__imul__=deny
def frozen(v):
 if isinstance(v,dict):return FrozenDict((frozen(k),frozen(x))for k,x in v.items())
 if isinstance(v,list):return FrozenList(map(frozen,v))
 if isinstance(v,tuple):return tuple(map(frozen,v))
 # Unused immutable set/deque fields stay opaque; loading one requires audit.
 if isinstance(v,(set,collections.deque)):raise TypeError('IMMUTABLE_CONTAINER_ACCESS_NOT_AUDITED')
 return v
class HostStore(MutableMapping):
 def __init__(self,manifest,mutable=MUTABLE):
  self.path=Path(manifest);self.manifest=json.loads(self.path.read_text());self.fields={r['key']:r for r in self.manifest['fields']};self.cache={};self.mutable=set(mutable)
  if len(self.fields)!=len(self.manifest['fields']):raise ValueError('HOST_DUPLICATE_FIELD')
 def __getitem__(self,k):
  if k not in self.cache:
   r=self.fields[k];p=ROOT/r['path'];v=load(p,r['sha256']);self.cache[k]=v if k in self.mutable else frozen(v)
  return self.cache[k]
 def __setitem__(self,k,v):
  if k not in self.mutable or k not in self.fields:raise TypeError('HOST_WRITE_NOT_APPROVED '+str(k))
  self.cache[k]=v
 def __delitem__(self,k):raise TypeError('HOST_FIELD_DELETE_REFUSED')
 def __iter__(self):return iter(self.fields)
 def __len__(self):return len(self.fields)
 def freeze(self,out):
  out=Path(out);rows=[]
  for k,r in self.fields.items():
   if k in self.mutable and k in self.cache:
    p=out/('host-'+str(len(rows))+'.bin');info=dump(self.cache[k],p);rows.append(dict(key=k,path=str(p.relative_to(ROOT)),**info))
   else:rows.append(r)
  p=out/'host.manifest.json';put(p,{'format':'S83_HOST_FIELDS_V2','base_manifest_sha256':sha(self.path),'fields':rows,'complete_fields':len(rows),'unchanged_fields_use_pinned_content':True});return p
def prepare(host,out):
 out=Path(out);out.mkdir(exist_ok=False);rows=[]
 for k,v in host.items():
  if not isinstance(k,str):raise ValueError('HOST_KEY_NOT_STRING')
  p=out/('%03d.bin'%len(rows));info=dump(v,p);rows.append(dict(key=k,path=str(p.relative_to(ROOT)),**info))
 p=out/'host.manifest.json';put(p,{'format':'S83_HOST_FIELDS_V2','fields':rows,'complete_fields':len(rows)});return p
