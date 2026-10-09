# -*- coding: utf-8 -*-
import hashlib,json,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def digest(p):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  while True:
   b=f.read(1048576)
   if not b:break
   h.update(b)
 return h.hexdigest()
old=read('stage38-postflight.json'); s38=read('stage38-result.json'); rows=[]
for q in old['checks']:
 p=Path(q['path']);actual=digest(p)
 rows.append(dict(q,sha256=actual,expected_sha256=q['sha256'],unchanged=actual==q['sha256']))
for q in s38['current_checkpoint_files']:
 p=R/q['path'];actual=digest(p)
 rows.append(dict(group='device38_checkpoint',path=str(p),sha256=actual,expected_sha256=q['sha256'],unchanged=actual==q['sha256']))
audit=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_frozen_inputs_unchanged=all(q['unchanged'] for q in rows),check_count=len(rows),checks=rows)
(R/'evidence/stage39-postflight.json').write_text(json.dumps(audit,ensure_ascii=False,indent=2))
assert audit['all_frozen_inputs_unchanged']
run=read('stage39-closed39.json');restore=read('stage39-restore.json');life=read('stage39-lifecycle.json');spec=read('stage39-lifecycle-spec.json')
assert restore['all_memory_CPU_four_threads_and_native_blobs_restored']
assert restore['original_engine_sample_count']==8 and len(restore['original_engine_stored_native_handles'])==8
assert restore['original_2D_initializer_observed_returns'][0]['eax']=='0x1'
files=[dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=digest(p)) for p in sorted((R/'runs/stage12-state-closed39').iterdir()) if p.is_file()]
out=dict(created_utc=audit['created_utc'],outcome='Original 2D initializer returns EAX=1 after eight actual native empty sample allocations; next unimplemented contract is 3D provider enumeration',current_checkpoint='runs/stage12-state-closed39',current_checkpoint_files=files,next_boundary=run['result']['boundary'],main_PC=restore['main_PC'],native_sample_library=read('stage39-native-build.json'),original_engine_stored_native_handles=restore['original_engine_stored_native_handles'],original_2D_initializer_observed_returns=restore['original_2D_initializer_observed_returns'],sample_default_volume_preference_index=2,full_checkpoint_restore=restore,own_lifecycle=life,own_lifecycle_spec=spec,original_MSS_sample_stream_3D_export_executed=False,MSS36_approval_scope_not_expanded=True,original_code_patched=False,nonempty_MIXER_merge_equivalence_proven=False,provider_registry_equivalence_proven=False,full_native_WinMain_proven=False,new_original_function_static_conversion_proven=False,main_menu_passed=False,city_logic_passed=False,all_frozen_input_hashes_unchanged=True,protected_path_checks=len(rows),automatic_approval_rejected=False,worth_continuing=True,remaining_uncertainties=['3D enumeration requires dynamic RIB provider registry and provider Name attribute callback; DLL presence/load alone insufficient','Empty sample mixer binding metadata does not implement original MIXER_merge ABI, PCM mixing, ASI/filter streaming or voice callbacks','Original sample defaults statically derived; no original sample export dynamic comparison approved','Original startup is unmodified i386 Unicorn reference calling own arm64 platform adapters, not full native WinMain'])
(R/'evidence/stage39-result.json').write_text(json.dumps(out,ensure_ascii=False,indent=2))
# Correct an earlier static-reading index; both old indices default to 127.
for rel in ['reports/stage38-MSS-approved-result.md','samples/stage38_handoff.py']:
 p=R/rel;t=p.read_text();p.write_text(t.replace('pref5音量','pref2音量（Stage39按0x21150bf4核正索引）'))
cor=dict(source='stage39-static.json',address='0x21150bf4',correct_preference_index=2,earlier_report_index=5,default_value=127,native_stage38_runtime_unchanged=True)
(R/'evidence/stage39-pref-index-correction.json').write_text(json.dumps(cor,indent=2))
print(json.dumps(dict(outcome=out['outcome'],checkpoint=out['current_checkpoint'],protected_path_checks=len(rows),all_unchanged=True),ensure_ascii=False))
