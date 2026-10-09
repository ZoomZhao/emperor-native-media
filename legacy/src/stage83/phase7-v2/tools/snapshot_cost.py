"""Original memory/context data roundtrip timing; original engine never starts."""
from core import *
import shutil
def main():
 out=V/'runs/readonly-snapshot-cost';out.mkdir(exist_ok=False);t=time.monotonic();s=json.loads((OLD/'configs/source.json').read_text());verify({str(SOURCE/n):h for n,h in PINS.items()});u=fixed();from stage73_cpu import restore
 b=(SOURCE/'context.bin').read_bytes();uc,ctx=restore(u,b,s['context']);load_start=time.monotonic()
 with(SOURCE/'memory.bin').open('rb')as f:
  for r in s['regions']:
   n=r['high']-r['low']+1;f.seek(r['offset']);body=f.read(n);assert hashlib.sha256(body).hexdigest()==r['sha256'];uc.mem_map(r['low'],n,r['permissions']);uc.mem_write(r['low'],body)
 uc.context_restore(ctx);assert uc.context_save().__getstate__()[0][40:]==b[40:];load_seconds=time.monotonic()-load_start;save_start=time.monotonic();total=0
 with(out/'memory.bin').open('xb')as f:
  for r in s['regions']:
   n=r['high']-r['low']+1;hash=hashlib.sha256()
   for off in range(0,n,1<<20):q=bytes(uc.mem_read(r['low']+off,min(n-off,1<<20)));f.write(q);hash.update(q);total+=len(q)
   assert hash.hexdigest()==r['sha256']
 assert sha(out/'memory.bin')==PINS['memory.bin'];memory_save=time.monotonic()-save_start;h=host();freeze_start=time.monotonic();manifest=h.freeze(out);freeze_seconds=time.monotonic()-freeze_start
 for tid,th in h['guest_threads20'].items():(out/f'context-{tid}.bin').write_bytes(th['context'][0])
 # Own performance trace copied as data; remaining bytes are explicitly synthetic IO.
 trace_start=time.monotonic();own=V/'runs/own-performance-full-v1/writes.bin';target=out/'own-trace-size-7M.bin';shutil.copyfile(own,target);remaining=7000000*24-target.stat().st_size;chunk=struct.pack('<6I',99,0,0,4,0,99)*65536
 with target.open('ab')as f:
  while remaining:part=chunk[:min(len(chunk),remaining)];f.write(part);remaining-=len(part)
 trace_seconds=time.monotonic()-trace_start;assert target.stat().st_size==168000000
 put(V/'evidence/snapshot-cost.json',{'status':'READONLY_ORIGINAL_MEMORY_AND_FULL_HOST_REFERENCES_PLUS_7M_OWN_TRACE_IO_PASS','source_pins':PINS,'memory_bytes':total,'source_restore_map_hash_seconds':load_seconds,'memory_read_hash_save_and_verify_seconds':memory_save,'complete_host_freeze_seconds':freeze_seconds,'host_manifest':str(manifest.relative_to(ROOT)),'host_fields_retained':len(json.loads(manifest.read_text())['fields']),'trace_IO_168MB_seconds':trace_seconds,'whole_process_data_seconds':time.monotonic()-t,'original_engine_emu_start_calls':0,'original_instructions':0,'trace_is_own_plus_synthetic_size_benchmark_not_original_log':True});print(json.dumps({'restore':load_seconds,'save':memory_save,'host':freeze_seconds,'trace168MB':trace_seconds,'total':time.monotonic()-t,'original_emu_start':0}))
if __name__=='__main__':main()
