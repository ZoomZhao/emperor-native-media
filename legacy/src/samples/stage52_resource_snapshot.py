"""Read-only metadata extraction from an own checkpoint; no runtime execution."""
import json,sys,collections
from pathlib import Path
from stage12_state import decode
R=Path(__file__).resolve().parents[1];tag=sys.argv[1];p=R/('runs/stage12-state-'+tag)
record=json.loads((p/'state.json').read_text());host=dict(record['host']['dict'])
allocations=decode(host['allocations']);dd=decode(host['dd_objects']);identities=decode(host['startup_file_identity']);bindings=decode(host['resource_binding_checks'])
files=[]
for handle,raw in host['files']['dict']:
 fields=dict(raw['dict']);path=decode(fields.get('path'))
 if isinstance(path,bytes):path=path.decode('gbk',errors='backslashreplace')
 files.append(dict(handle=decode(handle),path=path,position=decode(fields.get('position')),payload_bytes=len(fields.get('payload',{}).get('bytes',''))//2))
run=json.loads((R/('evidence/stage51-'+tag+'.json')).read_text())
main=[q for q in run['result'].get('segments',[])if q['tid']==1]
out=dict(checkpoint=tag,read_only_checkpoint_inspection=True,original_EXE_instructions_executed=False,original_DLL_exports_executed=False,saved_CPU_registers=record['registers'],guest_tracked_live_heap_blocks=len(allocations),guest_tracked_live_heap_bytes=sum(q[1]for q in allocations.values()),guest_heap_count=len(decode(host['heaps'])),guest_mapped_regions=len(record['regions']),guest_mapped_memory_bytes=record['memory_bytes'],guest_heap_mapped_end=hex(decode(host['heap_mapped_end34'])),tracked_DirectDraw_COM_objects=len(dd),DirectDraw_objects_by_kind=dict(collections.Counter(q['kind']for q in dd.values())),DirectDraw_guest_objects=[dict(guest_address=hex(addr),**q)for addr,q in dd.items()],frozen_file_identity_count=len(identities),frozen_file_identities=[dict(path=k.decode('gbk'),size=v[0],device=v[1],inode=v[2],sha256=v[3])for k,v in identities.items()],resource_binding_checks=bindings,live_guest_files=files,pending_original_city_frames=run['city_frames49'],original_city_observations=run['city_observations49'],current_CPU_PC=run['current_CPU_PC'],last_main_thread_PC=main[-1]['end_pc']if main else run['current_CPU_PC'],natural_state=run['natural_state'],semantic_game_object_graph_decoded=False,note='heap blocks and owned platform objects are actual tracked metadata, not a count or interpretation of every original city entity')
(R/('evidence/stage52-'+tag+'-resources.json')).write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['checkpoint','guest_tracked_live_heap_blocks','guest_tracked_live_heap_bytes','tracked_DirectDraw_COM_objects','frozen_file_identity_count','last_main_thread_PC']}))
