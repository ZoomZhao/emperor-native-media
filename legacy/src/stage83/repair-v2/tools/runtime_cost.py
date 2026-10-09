"""Measure lazy original host freeze, data only, with all referenced hashes."""
import json,sys,time
sys.dont_write_bytecode=True
from common import ROOT,V,put,verify
from binary_host import HostStore
t=time.monotonic();h=HostStore(V/'prepared-host/host.manifest.json')
required=('current_tid','guest_threads20','imports','heaps','allocations','heap_next','heap_mapped_end34','tls','last_error','critical_sections','thread_owners')
for k in required:h[k]
loaded=time.monotonic();out=V/'runs/lazy-original-host-freeze';out.mkdir();manifest=h.freeze(out);frozen=time.monotonic();m=json.loads(manifest.read_text());verify({r['path']:r['sha256']for r in m['fields']});done=time.monotonic()
put(V/'evidence/lazy-runtime-cost.json',{'status':'FULL_HOST_CONTENT_PINNED_LAZY_FREEZE_MEASURED','fields':len(m['fields']),'fields_materialized':len(h.cache),'lazy_load_seconds':loaded-t,'freeze_seconds':frozen-loaded,'all_field_content_hash_verify_seconds':done-frozen,'new_output_bytes':sum(p.stat().st_size for p in out.iterdir()if p.is_file()),'total_referenced_host_bytes':sum(r['bytes']for r in m['fields']),'host_mutated_before_freeze':False,'original_Unicorn_execution':0,'guest_or_original_CPU_changed':False,'legacy_full_binary_load_was_slower_and_is_not_runtime_path':True});print(json.dumps({'loaded_seconds':loaded-t,'freeze_seconds':frozen-loaded,'hash_seconds':done-frozen,'new_bytes':sum(p.stat().st_size for p in out.iterdir()if p.is_file()),'original_execution':0}))
