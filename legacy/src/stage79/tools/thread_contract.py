"""Pure checkpoint/public CPU transfer and finite reference-thread contracts.

No engine import, opaque-context interpretation, original/SR execution or SEH
success emulation. Public worker CPU must be supplied as independent evidence
before switching; no register defaults are invented.
"""
import copy,hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
CORE=('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI','EIP','EFLAGS')
FULL=CORE+('CS','DS','SS','ES','FS','GS','CR0','GDTR','LDTR','FS_BASE','GS_BASE','FPCW','FPSW','FPTAG','FP80')
class Boundary(RuntimeError):
 def __init__(self,kind,**evidence):super().__init__(kind);self.kind=kind;self.evidence=evidence
def checked_CPU(value,full=False):
 required=FULL if full else CORE
 if value is None or any(k not in value for k in required):raise Boundary('PUBLIC_CPU_MISSING',required=list(required))
 for k in CORE:
  if type(value[k])is not int or not 0<=value[k]<=0xffffffff:raise Boundary('INVALID_PUBLIC_CORE',field=k)
 if full:
  for k in ('CS','DS','SS','ES','FS','GS','FPCW','FPSW','FPTAG'):
   if type(value[k])is not int or not 0<=value[k]<=0xffff:raise Boundary('INVALID_PUBLIC_FIELD',field=k)
  for k in ('GDTR','LDTR'):
   if not isinstance(value[k],(list,tuple))or len(value[k])!=4 or any(type(v)is not int for v in value[k]):raise Boundary('INVALID_PUBLIC_DESCRIPTOR_TABLE',field=k)
  if not isinstance(value['FP80'],list)or len(value['FP80'])!=8:raise Boundary('INVALID_FP80')
  for pair in value['FP80']:
   if not isinstance(pair,(list,tuple))or len(pair)!=2 or type(pair[0])is not int or type(pair[1])is not int or not 0<=pair[0]<1<<64 or not 0<=pair[1]<1<<16:raise Boundary('INVALID_FP80_VALUE')
 return copy.deepcopy(value)
class ThreadRuntime:
 def __init__(self,records,current,root_CPU,root_opaque,root_tls,root_last_error):
  self.records=copy.deepcopy(records);self.current=current;self.boundary=None
  if current not in self.records:raise Boundary('UNKNOWN_CURRENT_THREAD')
  row=self.records[current];row['public_CPU']=checked_CPU(root_CPU);row['opaque_bytes']=bytes(root_opaque);row['tls']=copy.deepcopy(root_tls);row['last_error']=root_last_error
  if len(root_opaque)!=5608:raise Boundary('OPAQUE_ABI_SIZE')
  self.current_CPU=copy.deepcopy(row['public_CPU']);self.opaque=bytes(root_opaque);self.tls=copy.deepcopy(root_tls);self.last_error=root_last_error
 def save(self):
  row=self.records[self.current];row['public_CPU']=copy.deepcopy(self.current_CPU);row['opaque_bytes']=bytes(self.opaque);row['tls']=copy.deepcopy(self.tls);row['last_error']=self.last_error
 def switch(self,tid):
  if tid not in self.records:raise Boundary('UNKNOWN_THREAD')
  candidate=self.records[tid]
  if candidate.get('status')!='runnable':raise Boundary('THREAD_NOT_RUNNABLE')
  cpu=checked_CPU(candidate.get('public_CPU'),True)
  if not candidate.get('cached_segments_verified'):raise Boundary('CACHED_SEGMENTS_UNVERIFIED')
  opaque=candidate.get('opaque_bytes')
  if not isinstance(opaque,bytes)or len(opaque)!=5608:raise Boundary('OPAQUE_CONTEXT_UNAVAILABLE')
  # No state changes occur before every incoming-state gate passes.
  self.save();self.current=tid;self.current_CPU=cpu;self.opaque=bytes(opaque);self.tls=copy.deepcopy(candidate['tls']);self.last_error=candidate['last_error']
 def kernel(self,name,*args):
  if name=='GetCurrentThreadId' and not args:return self.current
  if name=='GetLastError' and not args:return self.last_error
  if name=='SetLastError' and len(args)==1:self.last_error=args[0]&0xffffffff;return None
  if name in ('TlsGetValue','TlsSetValue') and len(args)==(1 if name=='TlsGetValue' else 2):
   index=args[0]
   if type(index)is not int or index not in self.tls:raise Boundary('UNALLOCATED_REFERENCE_TLS_INDEX',index=index)
   if name=='TlsGetValue':self.last_error=0;return self.tls[index]
   self.tls[index]=args[1]&0xffffffff;return 1
  raise Boundary('UNIMPLEMENTED_THREAD_OR_KERNEL_API',API=name)
 def raise_exception(self,code,flags,count,pointer,read):
  # Capture raw parameters, not a fabricated successful RaiseException return.
  # NULL lpArguments means count is ignored by Windows. Non-NULL >15 is an
  # explicit unsupported contract; no count clipping or handler dispatch.
  before=copy.deepcopy(self.current_CPU);arguments=[];diagnostic=None
  if pointer:
   if count>15:diagnostic='TOO_MANY_EXCEPTION_PARAMETERS'
   else:
    try:
     body=read(pointer,4*count)
     if len(body)!=4*count:raise Boundary('SHORT_EXCEPTION_ARGUMENT_READ')
     arguments=[int.from_bytes(body[i:i+4],'little') for i in range(0,len(body),4)]
    except Boundary as e:diagnostic=e.kind
  event=dict(kind='UNIMPLEMENTED_SEH_DISPATCH_STOP',thread=self.current,code=code,flags=flags,requested_count=count,argument_pointer=pointer,arguments=arguments,argument_capture_boundary=diagnostic,public_CPU=before,opaque_sha256=hashlib.sha256(self.opaque).hexdigest(),TLS=copy.deepcopy(self.tls),LastError=self.last_error,return_value_fabricated=False,guest_stack_cleanup_performed=False,handler_or_unwind_executed=False)
  self.boundary=event;raise Boundary(event['kind'],event=event)
 def export(self):self.save();return copy.deepcopy(self.records)
def restore_metadata(meta,root):
 records={int(k):copy.deepcopy(v)for k,v in meta['thread_records'].items()}
 for row in records.values():
  row['tls']={int(k):v for k,v in row['tls'].items()};p=root/row['opaque_context']['path'];b=p.read_bytes()
  if len(b)!=row['opaque_context']['bytes']or hashlib.sha256(b).hexdigest()!=row['opaque_context']['sha256']:raise Boundary('THREAD_CONTEXT_DIGEST_MISMATCH')
  row['opaque_bytes']=b
 current=meta['root_host']['current_tid'];context=meta['root_opaque_context'];p=Path(context['path']);b=p.read_bytes()
 if hashlib.sha256(b).hexdigest()!=context['sha256']:raise Boundary('ROOT_CONTEXT_DIGEST_MISMATCH')
 return ThreadRuntime(records,current,meta['root_current_CPU'],b,{int(k):v for k,v in meta['root_host']['tls'].items()},meta['root_host']['last_error'])
