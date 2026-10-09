import hashlib,json,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage79';sys.path.insert(0,str(ROOT/'stage77/tools'))
from heap_extract import tagged,sha
def main():
 path=ROOT/'runs/stage12-state-cycle62p19/state.json';raw=json.loads(path.read_text());host=dict(raw['host']['dict']);memory=path.parent/'memory.bin';regions=sorted(raw['regions'],key=lambda r:r['low']);verified=[]
 with memory.open('rb')as f:
  for r in regions:
   f.seek(r['offset']);left=r['high']-r['low']+1;h=hashlib.sha256()
   while left:
    b=f.read(min(1<<20,left));assert b;left-=len(b);h.update(b)
   assert h.hexdigest()==r['sha256'];verified.append(r)
 data=b'S79M'+struct.pack('<I',len(regions))+b''.join(struct.pack('<IIQQ',r['low'],r['permissions'],r['high']+1,r['offset'])for r in regions);(S/'evidence/all-region-plan.bin').write_bytes(data)
 threads=tagged(host['guest_threads20']);selected={}
 for tid,row in threads.items():
  selected[tid]={k:v for k,v in row.items()if k!='context'};context=row['context'];b=context[0];cp=S/'evidence'/('thread-%d-context.bin'%tid);cp.write_bytes(b);selected[tid]['opaque_context']=dict(path=str(cp.relative_to(ROOT)),bytes=len(b),sha256=sha(cp),arch=context[2],mode=context[3],size=context[1]);selected[tid]['public_CPU']=None
 current=tagged(host['current_tid']);core={k.upper():v for k,v in raw['registers'].items()};rootctx=path.parent/'context.bin';assert sha(rootctx)==raw['context']['sha256'];assert rootctx.stat().st_size==raw['context']['size'];selected[current]['public_CPU']=core
 root_host={k:tagged(host[k])for k in ('current_tid','tls','last_error','critical_sections','thread_owners','thread_quantum','thread_switches','thread_waits','thread_prefer','next_tid','thread_active')if k in host}
 r=dict(status='CHECKPOINT_THREAD_AND_ALL_MEMORY_METADATA_EXTRACTED_READ_ONLY',original_sha256=raw['original_EXE_sha256'],source_state=str(path),source_state_sha256=sha(path),source_memory=str(memory),source_memory_sha256=sha(memory),regions=verified,region_count=len(regions),mapped_bytes=raw['memory_bytes'],root_current_CPU=core,root_opaque_context=dict(path=str(rootctx),**raw['context']),thread_records=selected,root_host=root_host,public_worker_CPU_missing=True,root_current_context_overrides_stale_saved_current_thread_on_restore=True,opaque_context_offsets_inspected=False,game_or_Unicorn_or_SR_library_executed=False)
 (S/'evidence/checkpoint-state.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:r[k]for k in ['status','region_count','mapped_bytes','public_worker_CPU_missing','opaque_context_offsets_inspected']}))
if __name__=='__main__':main()
