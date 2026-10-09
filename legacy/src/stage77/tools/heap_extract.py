"""Strict tagged checkpoint metadata extraction and heap-address validation.

Loads JSON only; does not import/restore an emulator or execute guest code.
No guest bytes are emitted or rewritten. Heap-region byte hashes are verified.
"""
import argparse,collections,hashlib,json,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage77';LOW=0x10000000;LIMIT=0x20000000
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def tagged(v):
 if isinstance(v,list):return[tagged(x)for x in v]
 if not isinstance(v,dict):return v
 if set(v)=={'dict'}:
  pairs=[(tagged(k),tagged(x))for k,x in v['dict']];keys=[k for k,x in pairs]
  if len(set(keys))!=len(keys):raise ValueError('duplicate tagged dictionary key')
  return dict(pairs)
 if set(v)=={'tuple'}:return tuple(tagged(x)for x in v['tuple'])
 if set(v)=={'bytes'}:return bytes.fromhex(v['bytes'])
 raise ValueError('unrecognized tagged metadata structure')
def validate(heaps,allocations,next_pointer,mapped_end,regions):
 assert type(next_pointer)is int and type(mapped_end)is int and LOW<=next_pointer<=mapped_end<=LIMIT and not next_pointer&7 and not mapped_end&0xffffff
 assert heaps and all(type(h)is int and 0<h<=0xffffffff and set(info)=={'options'}and type(info['options'])is int and not info['options']&~1 for h,info in heaps.items())
 covered=sorted(regions,key=lambda r:r['low']);rows=[];previous=LOW
 for p,(h,n)in sorted(allocations.items()):
  assert type(p)is int and type(h)is int and type(n)is int and h in heaps and 0<=n<=0xffffffff and not p&7
  rounded=max(8,(n+7)&~7);end=p+rounded;assert LOW<=p and previous<=p and end<=next_pointer
  cursor=p
  for r in covered:
   if r['low']<=cursor<r['high']+1 and r['permissions']&3==3:cursor=min(end,r['high']+1)
   if cursor==end:break
  assert cursor==end,('unmapped/readonly allocation',hex(p),n)
  previous=end;rows.append(dict(pointer=p,heap=h,requested_size=n,rounded_size=rounded))
 return rows
def main():
 p=argparse.ArgumentParser();p.add_argument('--state',required=True);p.add_argument('--output',required=True);a=p.parse_args();state=(ROOT/a.state).resolve();out=(ROOT/a.output).resolve();assert out.is_relative_to(S/'evidence')and not out.exists()
 # Parse the root schema and host pair collection exactly, rather than matching
 # a key somewhere inside nested API logs or guessing a tuple representation.
 raw=json.loads(state.read_text());assert raw['format']==1 and raw['original_EXE_sha256']=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 assert set(raw['host'])=={'dict'};pairs=raw['host']['dict'];keys=[k for k,v in pairs];assert len(set(keys))==len(keys);host=dict(pairs)
 heaps=tagged(host['heaps']);allocations=tagged(host['allocations']);next_pointer=tagged(host['heap_next']);mapped_end=tagged(host['heap_mapped_end34'])
 regions=[r for r in raw['regions']if LOW<=r['low']and r['high']<LIMIT];rows=validate(heaps,allocations,next_pointer,mapped_end,regions)
 verified=[]
 with(state.parent/'memory.bin').open('rb')as f:
  for r in regions:
   f.seek(r['offset']);left=r['high']-r['low']+1;h=hashlib.sha256()
   while left:
    b=f.read(min(left,1<<20));assert b;left-=len(b);h.update(b)
   assert h.hexdigest()==r['sha256'];verified.append(dict(**r,heap_memory_not_copied_or_modified=True))
 selected={k:tagged(host[k])for k in ['current_tid','last_error','tls']if k in host}
 requested=sum(r['requested_size']for r in rows);rounded=sum(r['rounded_size']for r in rows);consumed=next_pointer-LOW
 result=dict(status='REAL_CHECKPOINT_HEAP_METADATA_VALIDATED_READ_ONLY',source_state=str(state),source_state_sha256=sha(state),source_memory=str(state.parent/'memory.bin'),source_memory_sha256=sha(state.parent/'memory.bin'),heaps=[dict(handle=h,**v)for h,v in sorted(heaps.items())],allocations=rows,heap_next=next_pointer,heap_mapped_end=mapped_end,heap_regions=verified,live_allocation_count=len(rows),requested_live_bytes=requested,rounded_live_bytes=rounded,consumed_bump_bytes=consumed,unowned_consumed_bytes=consumed-rounded,allocator_policy='original fixed reference bump allocator; frees remove ownership without reusing addresses; no claim about real Windows Heap internals',heap_bytes_imported_to_native_runtime=False,selected_current_thread_metadata=selected,guest_TLS_and_LastError_full_thread_restore_implemented=False,original_instructions_or_Unicorn_executed=False)
 binary=out.with_suffix('.bin');assert not binary.exists();body=bytearray(b'S77H'+struct.pack('<6I',1,len(heaps),len(regions),len(rows),next_pointer,mapped_end))
 for h,v in sorted(heaps.items()):body+=struct.pack('<2I',h,v['options'])
 for r in sorted(regions,key=lambda r:r['low']):body+=struct.pack('<3I',r['low'],r['high']+1,r['permissions'])
 for r in rows:body+=struct.pack('<3I',r['pointer'],r['heap'],r['requested_size'])
 binary.write_bytes(body);result.update(metadata_transfer_binary=str(binary),metadata_transfer_sha256=sha(binary),metadata_transfer_bytes=len(body));out.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:result[k]for k in ['status','live_allocation_count','requested_live_bytes','rounded_live_bytes','consumed_bump_bytes','unowned_consumed_bytes','heap_next','heap_mapped_end','metadata_transfer_bytes','selected_current_thread_metadata']}))
if __name__=='__main__':main()
