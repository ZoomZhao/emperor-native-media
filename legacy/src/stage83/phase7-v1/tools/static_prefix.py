"""Pinned PE and historical trace reads; no Unicorn/runtime import."""
from core import *
import struct,collections
sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE,decode as instruction
s=json.loads((V/'configs/source.json').read_text());h=HostStore(ROOT/s['host_manifest']);cpu=json.loads((V/'evidence/cpu-restore.json').read_text())['thread_records'];pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');imports=h['imports']
def memory(a,n):
 for r in s['regions']:
  if r['low']<=a and a+n<=r['high']+1:
   with(SOURCE/'memory.bin').open('rb')as f:f.seek(r['offset']+a-r['low']);return f.read(n)
 raise Boundary('UNMAPPED_STATIC_READ')
def disasm(pc,n):
 rows=[]
 for _ in range(n):
  try:i=instruction(pe,pc);rows.append(i);pc=i['end']
  except Exception as e:rows.append({'pc':pc,'decode_boundary':str(e)});break
 return rows
p4=cpu['4']['public_CPU'];worker={'current_PC':p4['EIP'],'next_CALL_site':0x5fc044,'next_target_from_public_EBX':p4['EBX'],'next_target_binding':imports.get(p4['EBX']),'EnterCriticalSection_argument_after_push_ESI':p4['ESI'],'captured_critical_depth':h['critical_sections'].get(p4['ESI']),'captured_owner':h['thread_owners'].get(p4['ESI']),'captured_callback_count_address':p4['EDI']+44,'captured_callback_count':int.from_bytes(memory(p4['EDI']+44,4),'little'),'first_straight_line':disasm(0x5fc043,28),'post_empty_callback_branch':disasm(0x5fc11a,25),'current_queue_branch_is_not_whole_future_dynamic_domain_proof':True}
legacy=ROOT/'runs/stage12-state-cycle61p07/state.json';later=json.loads(legacy.read_text());lh=dict(later['host']['dict']);now=h['api_trace'];future=decode(lh['api_trace']);same_prefix=future[:len(now)]==list(now);delta=future[len(now):]if same_prefix else[]
calls=[]
for row in delta:
 name=row.get('name',row.get('method','?'));dll=row.get('dll','');pc=row.get('return_pc',row.get('guest_return_pc',row.get('original_call_return_pc')));calls.append({'name':name,'dll':dll,'return_PC':pc,'tid':row.get('tid'),'args':row.get('args')})
surface=[];blob=h['dd_native_blob']
for pointer,o in h['dd_objects'].items():
 if o['kind']==2:surface.append({'pointer':pointer,'guest_pixels':o['guest_pixels'],'id':o['id'],'header':list(struct.unpack_from('<12I',blob,48+(o['id']-1)*48))})
report={'status':'BOUNDED_STATIC_PREFIX_REVIEW_NOT_COMPLETE_CLOSURE','source_pins':PINS,'original_EXE_sha256':sha(pe.path),'worker4':worker,'source_pending_callbacks':{k:h[k]for k in ('pending_callbacks','platform_callback','style_callback','window_pending')},'original_caller_to_phase8':{'CALL':0x4ac338,'bytes':pe.read(0x4ac338,5).hex(),'entry':0x4aeeb0,'return':0x4ac33d},'main_after_known_stage73_stop':disasm(0x4ac333,8),'render_lock_prefix':disasm(0x52bf00,65),'source_surfaces':surface,'historical_next_snapshot_sha256':sha(legacy),'historical_api_same_prefix':same_prefix,'historical_new_API_count':len(delta),'historical_new_API_kinds':dict(collections.Counter((r['dll']+'!'+r['name'])for r in calls)),'historical_new_API_first_30':calls[:30],'historical_trace_proves_all_possible_future_paths':False,'original_execution':0}
put(V/'evidence/static-prefix.json',report);print(json.dumps({'worker4_first_target':worker['next_target_binding'],'worker4_callback_count':worker['captured_callback_count'],'historical_delta_kinds':report['historical_new_API_kinds'],'same_trace_prefix':same_prefix,'source_surfaces':surface}))
