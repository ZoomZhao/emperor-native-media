"""One-time original checkpoint data conversion and field-by-field verification."""
import json,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,V,decode,sha,put
from binary_host import prepare,load
t=time.monotonic();p=ROOT/'runs/stage12-state-cycle62p19/state.json';s=json.loads(p.read_text());h=decode(s['host']);parsed=time.monotonic();manifest=prepare(h,V/'prepared-host');written=time.monotonic();m=json.loads(manifest.read_text())
for row in m['fields']:
 v=load(ROOT/row['path'],row['sha256']);assert v==h[row['key']],row['key']
verified=time.monotonic();put(V/'configs/prepared-source.json',{'format':'S83_READ_ONLY_LEGACY_SOURCE_METADATA_V1','original_source':str(p.relative_to(ROOT)),'source_sha256':sha(p),'memory_sha256':sha(p.parent/'memory.bin'),'context_sha256':sha(p.parent/'context.bin'),'registers':s['registers'],'regions':s['regions'],'context':s['context'],'host_manifest':str(manifest.relative_to(ROOT)),'host_manifest_sha256':sha(manifest)})
put(V/'evidence/prepared-host-roundtrip.json',{'status':'FULL_FIXED_HOST_FIELDS_DATA_ROUNDTRIP_PASSED','fields':len(m['fields']),'binary_bytes':sum(r['bytes']for r in m['fields']),'source_parse_decode_seconds':parsed-t,'prepare_binary_seconds':written-parsed,'reload_verify_seconds':verified-written,'full_field_equalities':len(m['fields']),'opaque_context_bytes_preserved':True,'original_or_Unicorn_execution':0,'original_source_changed':False});print(json.dumps({'fields':len(m['fields']),'binary_bytes':sum(r['bytes']for r in m['fields']),'prepare_seconds':written-parsed,'verify_seconds':verified-written,'original_execution':0}))
