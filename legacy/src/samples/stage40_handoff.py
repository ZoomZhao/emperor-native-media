# -*- coding: utf-8 -*-
import hashlib,json,datetime,struct,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];tag=sys.argv[1]if len(sys.argv)>1 else 'record40'
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
rows=[]
for q in read('stage39-postflight.json')['checks']:
 h=sha(q['path']);rows.append(dict(path=q['path'],expected_sha256=q['sha256'],sha256=h,unchanged=h==q['sha256']))
for q in read('stage39-result.json')['current_checkpoint_files']:
 p=R/q['path'];h=sha(p);rows.append(dict(path=str(p),expected_sha256=q['sha256'],sha256=h,unchanged=h==q['sha256']))
for name in ['stage40-audio-text-catalog.readonly.json','stage40-record-catalog.readonly.json']:
 for q in read(name)['rows']:
  for which in ['original','copy']:
   p=q[which+'_path'];h=sha(p);rows.append(dict(path=p,expected_sha256=q[which+'_sha256'],sha256=h,unchanged=h==q[which+'_sha256']))
run=read('stage40-'+tag+'.json');absences=[]
for call in run.get('missing_model_calls',[]):
 for q in call['actual_POSIX_readonly_failures']:
  p=Path(q['path']);absences.append(dict(path=str(p),still_missing=not p.exists()and not any(c.name.casefold()==p.name.casefold()for c in p.parent.iterdir())))
audit=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_frozen_inputs_unchanged=all(r['unchanged']for r in rows),hash_checks=len(rows),unique_paths=len({r['path']for r in rows}),checks=rows,all_observed_missing_inputs_still_missing=all(r['still_missing']for r in absences),absences=absences)
(R/'evidence/stage40-postflight.json').write_text(json.dumps(audit,ensure_ascii=False,indent=2));assert audit['all_frozen_inputs_unchanged']and audit['all_observed_missing_inputs_still_missing']
dir=R/f'runs/stage12-state-{tag}';state=json.loads((dir/'state.json').read_text());files=[dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p))for p in sorted(dir.iterdir())if p.is_file()]
def guest(addr,n):
 region=next(r for r in state['regions']if r['low']<=addr and addr+n<=r['high']+1)
 with(dir/'memory.bin').open('rb')as f:f.seek(region['offset']+addr-region['low']);return f.read(n)
obj=0x11550e8;vectors=[]
for offset,label in [(12,'general'),(144,'combat')]:
 active,start,end,cap=struct.unpack('<4I',guest(obj+offset,16));count=(end-start)//101 if start and end>=start and (end-start)%101==0 else None;names=[]
 if count is not None and count<32:
  for i in range(count):names.append(guest(start+i*101,101).split(b'\0')[0].decode(errors='replace'))
 vectors.append(dict(label=label,active=active,start=hex(start),end=hex(end),capacity_end=hex(cap),count=count,paths=names))
from stage12_state import decode
host_fields=dict(state['host']['dict']);volume_calls=decode(host_fields['volume_calls']);file_calls=decode(host_fields['startup_file_calls']);file_IO=decode(host_fields['startup_file_IO'])
record_calls=[q for q in file_calls+file_IO if 'imperialrecord.imr' in str(q).lower()]
restore=read('stage40-restore.json');assert restore['source']==tag and restore['all_memory_CPU_four_threads_and_native_blobs_restored']
result=dict(created_utc=audit['created_utc'],outcome='Capability-backed native registry causes original lawful 3D-unavailable branch; original audio startup group returns; later startup preserved at real next boundary',current_checkpoint=str(dir.relative_to(R)),current_checkpoint_files=files,mapped_memory_bytes=state['memory_bytes'],main_PC=hex(state['registers']['eip']),current_guest_registers=state['registers'],bounded_execution_segments=run['result']['segments'],next_status=run['result']['status'],next_boundary=run['result'].get('boundary'),provider_calls=run['provider_calls'],provider_events=run['provider_events'],native_capabilities=run['native_capabilities'],native_registry_library=read('stage40-build.json'),native_registry_lifecycle=read('stage40-lifecycle.json'),native_audio=run['native_audio'],full_checkpoint_restore=restore,original_music_vectors=vectors,record_path_volume_calls=[q for q in volume_calls if 'imperialrecord.imr' in str(q).lower()],record_readonly_file_calls=record_calls,original_music_vector_counts_match_input_12_4=[v['count']for v in vectors]==[12,4],missing_model_calls=run.get('missing_model_calls',[]),native_diagnostic=run['native_diagnostic'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_input_hashes_unchanged=True,original_MSS_or_M3D_exports_executed_this_stage=False,MSS36_approval_not_expanded=True,old_Windows_DLL_provider_registry_equivalence=False,original_code_patched=False,SpatialMixer_discovery_proves_Miles3D_adapter=False,main_menu_passed=False,city_logic_passed=False,full_native_WinMain_proven=False,new_original_function_static_conversion_proven=False,original_audio_assets_played=False,recording_performed=False,automatic_approval_rejected=False,worth_continuing=True,remaining_uncertainties=['Native registry describes current authored backend; fixed Windows DirectSound/RSX registry remains a separate unmeasured environment','Apple Spatial Mixer discovered but not instantiated or adapted to Miles3D listener/voice/position callbacks','Nonempty Miles PCM mixing, codec/stream/EOS still unproven','Original startup reference remains Unicorn i386 plus own arm64 platform adapters'])
(R/'evidence/stage40-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));print(json.dumps({k:result[k]for k in ['current_checkpoint','next_status','next_boundary','mapped_memory_bytes','original_music_vector_counts_match_input_12_4','protected_hash_checks','unique_protected_paths']},ensure_ascii=False))
