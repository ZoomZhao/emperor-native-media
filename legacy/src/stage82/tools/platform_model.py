"""Exact bounded captured-platform contracts; no OS calls or fake API success."""
import struct
from common import API,Boundary
class Platform:
 def __init__(self,host,memory):self.h=host;self.m=memory
 def invoke(self,id,args,eax):
  h=self.h;m=self.m;tid=h['current_tid'];a=args
  def allocation(p,heap):
   if heap not in h['heaps']or p not in h['allocations']or h['allocations'][p][0]!=heap:raise Boundary('HEAP_OWNERSHIP')
   return h['allocations'][p][1]
  def alloc(heap,flags,size):
   if heap not in h['heaps']or flags&~9:raise Boundary('HEAP_FLAGS_OR_HANDLE')
   actual=max(8,(size+7)&~7);p=h['heap_next'];end=p+actual
   if end>0x20000000:return 0
   if end>h['heap_mapped_end34']:
    new=(end+0xffffff)&~0xffffff;m.grow(h['heap_mapped_end34'],new);h['heap_mapped_end34']=new
   m.write(p,bytes([0 if flags&8 else 0xcd])*actual,id)
   h['heap_next']=end;h['allocations'][p]=(heap,size);return p
  if id==1:return alloc(*a)
  if id==2:
   heap,flags,p,n=a
   if flags&~9:raise Boundary('HEAP_REALLOC_FLAGS')
   old=allocation(p,heap);keep=min(old,n);body=m.read(p,keep);fresh=alloc(heap,flags,n)
   if fresh:
    if keep:m.write(fresh,body,id)
    del h['allocations'][p]
   return fresh
  if id in (3,4):
   heap,flags,p=a
   if flags&~1:raise Boundary('HEAP_FREE_SIZE_FLAGS')
   n=allocation(p,heap)
   if id==3:del h['allocations'][p];return 1
   return n
  if id==5:return tid
  if id==6:return h['last_error']
  if id==7:h['last_error']=a[0];return 0
  if id in (8,9):
   if a[0]not in h['tls']:raise Boundary('UNALLOCATED_TLS')
   if id==8:h['last_error']=0;return h['tls'][a[0]]
   h['tls'][a[0]]=a[1];return 1
  if id in (10,11,12):
   p=a[0]
   if id==10:
    if p in h['critical_sections']:raise Boundary('LOCK_REINITIALIZATION')
    m.check(p,24,3);h['critical_sections'][p]=0;h['thread_owners'][p]=None;return eax
   if p not in h['critical_sections']:raise Boundary('UNINITIALIZED_LOCK')
   owner=h['thread_owners'][p];depth=h['critical_sections'][p]
   if id==11:
    if owner not in (None,tid):raise Boundary('LOCK_WOULD_BLOCK_NO_WORKER_SCHEDULE')
    if depth==0xffffffff:raise Boundary('LOCK_RECURSION_OVERFLOW')
    h['thread_owners'][p]=tid;h['critical_sections'][p]=depth+1
   else:
    if owner!=tid or depth<1:raise Boundary('WRONG_THREAD_LOCK_LEAVE')
    h['critical_sections'][p]=depth-1
    if depth==1:h['thread_owners'][p]=None
   return eax
  if id in (13,14,15):
   dst,src,n=a
   if n>64*1024*1024:raise Boundary('PLATFORM_WRITE_SIZE_CAP')
   if id==13 and n and dst<src+n and src<dst+n:raise Boundary('MEMCPY_OVERLAP_UNDEFINED')
   body=bytes([src&255])*n if id==15 else m.read(src,n)
   if n:m.write(dst,body,id)
   return dst
  raise Boundary('UNIMPLEMENTED_PLATFORM_API')
