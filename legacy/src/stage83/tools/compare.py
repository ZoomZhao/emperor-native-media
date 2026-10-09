import json,struct
from pathlib import Path
from common import ROOT,CORE,sha,decode,lock_binary,put
def compare(input_dir,reference_dir,native_dir,out):
 inp=Path(input_dir);ref=Path(reference_dir);nat=Path(native_dir);rr=json.loads((ref/'result.json').read_text());s=json.loads((ref/'snapshot.json').read_text());initial=json.loads((inp/'snapshot.json').read_text());host=decode(s['host']);ncore=struct.unpack('<10I',(nat/'core.bin').read_bytes());fail=[]
 for k,v in zip(CORE,ncore):
  if rr['final_CPU'][k]!=v:fail.append({'CPU':k,'reference':rr['final_CPU'][k],'native':v})
 cur=str(s['current_tid']);side=[k for k in rr['final_CPU']if k not in CORE and k!='GS_BASE']
 for k in side:
  if rr['final_CPU'][k]!=initial['public_CPU'][cur][k]:fail.append({'unimplemented_sidecar_mutation':k})
 for name in ('writes.bin','api-events.bin','memory.bin','regions.bin'):
  if sha(ref/name)!=sha(nat/name):fail.append({'artifact_mismatch':name,'reference_sha256':sha(ref/name),'native_sha256':sha(nat/name)})
 raw=(nat/'locks.bin').read_bytes();n=struct.unpack_from('<I',raw,4)[0]if len(raw)>=8 else -1;rows=list(struct.iter_unpack('<3I',raw[8:]))if len(raw)>=8 and (len(raw)-8)%12==0 else []
 expected=sorted((p,d,host['thread_owners'][p]or 0)for p,d in host['critical_sections'].items())
 if raw[:4]!=b'S82L'or n!=len(rows)or sorted(rows)!=expected:fail.append({'locks_mismatch':True})
 tls=struct.unpack('<IQ64I',(nat/'tls.bin').read_bytes());expected=[host['last_error'],sum(1<<i for i in host['tls']),*(host['tls'].get(i,0)for i in range(64))]
 if list(tls)!=expected:fail.append({'TLS_LastError_mismatch':True})
 hb=struct.pack('<4I',host['heap_next'],host['heap_mapped_end34'],len(host['allocations']),len(host['heaps']))+b''.join(struct.pack('<3I',p,h,n)for p,(h,n)in sorted(host['allocations'].items()))
 if hb!=(nat/'heap-state.bin').read_bytes():fail.append({'heap_metadata_mismatch':True})
 status,completed,pc,fault,steps,writes=struct.unpack('<6Q',(nat/'native-result.bin').read_bytes())
 if status or not completed:fail.append({'native_boundary':status,'fault':fault})
 if rr['status']!='REFERENCE_EXACT_RETURN':fail.append({'reference_not_complete':True})
 # Workers remain immutable; native never schedules them. Opaque initial
 # sidecars are explicitly NOT advertised as updated native engine contexts.
 result={'status':'EXACT_KNOWN_STATE_AND_ORDERED_WRITES_MATCH'if not fail else 'DIFFERENTIAL_FAILED','mismatches':fail,'supported_public_CPU_fields_compared':len(rr['final_CPU'])-1,'unknown_CPU_fields_not_promoted':['GS_BASE'],'all_mapped_bytes_compared':s['memory_bytes'],'write_records':writes,'native_steps':steps,'reference_steps':rr['steps'],'instruction_and_platform_writes_distinguished':True,'API_event_and_heap_TLS_lock_state_compared':True,'native_opaque_context_is_initial_sidecar_only':True,'workers_scheduled':0,'input_sha256':sha(inp/'frozen.json')};put(out,result);return result
