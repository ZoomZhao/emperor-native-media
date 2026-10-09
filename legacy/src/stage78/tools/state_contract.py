"""Read checkpoint/public results only; no engine import or context restoration."""
import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'stage77/tools'))
from heap_extract import tagged
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=ROOT/'runs/stage12-state-cycle62p19/state.json';raw=json.loads(p.read_text());assert raw['original_EXE_sha256']=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753';host=dict(raw['host']['dict']);tid=tagged(host['current_tid']);threads=tagged(host['guest_threads20']);rows=[]
 for t,row in sorted(threads.items()):
  context=row.get('context');payload=context[0] if context else None
  rows.append(dict(tid=t,metadata_keys=sorted(row),status=row.get('status'),teb=row.get('teb'),context_bytes=len(payload) if payload else None,context_sha256=hashlib.sha256(payload).hexdigest() if payload else None))
 before=json.loads((ROOT/'runs/stage73-reference-once/result.json').read_text())['initial_CPU'];segments={k:before[k] for k in ('CS','DS','SS','ES','FS','GS','CR0','GDTR','LDTR','FS_BASE','GS_BASE')}
 closure=json.loads((ROOT/'stage76/evidence/full-dependency-closure-v1.json').read_text());fs=[q for q in closure['instructions'] if q['bytes'].startswith('64')];rep=[q for q in closure['instructions'] if q['mnemonic'] in ('rep','repne')]
 assert len(fs)==31 and all(q['mnemonic']=='mov' and 'fs:[0]' in q['operand'] for q in fs),(len(fs),fs[:1])
 fp=[q for q in closure['instructions'] if q['mnemonic'].startswith('f') or any(z in q['operand'] for z in ('xmm','mm0','st('))]
 r=dict(status='CPU_CONTRACT_METADATA_READ_ONLY_OPEN',checkpoint_state=str(p),checkpoint_state_sha256=sha(p),current_tid=tid,public_registers=raw['registers'],opaque_context_manifest=raw['context'],thread_contexts=rows,current_context_is_phase8_entry=False,complete_fresh_public_CPU_serialization_present=False,prior_stage73_public_CPU_fields=sorted(before),prior_stage73_segments=segments,prior_stage73_fields_prove_fresh_cached_segments=False,architectural_FP80_conversion_to_double_allowed=False,opaque_engine_context_sidecar_required=True,selected_original_FP_or_SIMD_instructions=len(fp),selected_FS_operations=len(fs),selected_FS_offsets=[0],selected_REP_operations=len(rep),FS_SEH_chain_adapter_implemented=True,SEH_dispatch_and_unwind_implemented=False,fresh_FS_cache_base_limit_rights_verified=False,DS_ES_flat_cache_verified_at_fresh_entry=False,all_platform_callbacks_preserve_full_CPU_sidecar_verified=False,whole_memory_thread_TLS_LastError_restore_implemented=False,game_or_Unicorn_executions=0)
 (ROOT/'stage78/evidence/state-contract.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:r[k] for k in ('status','current_tid','selected_FS_operations','selected_REP_operations','selected_original_FP_or_SIMD_instructions','fresh_FS_cache_base_limit_rights_verified','complete_fresh_public_CPU_serialization_present')}))
if __name__=='__main__':main()
