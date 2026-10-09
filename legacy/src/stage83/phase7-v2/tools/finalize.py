"""Seal bounded development evidence; no original execution or claims."""
from core import *
import subprocess

def main():
 baseline=json.loads((V/'evidence/protection-baseline.json').read_text())
 verify(baseline['hashes'])
 assert baseline['count']==6903
 ledger=ROOT/'evidence/stage55-batch-ledger.json'
 assert sha(ledger)=='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
 ls=json.loads(ledger.read_text())
 assert len(ls['executions'])==20 and ls['remaining_processes']==80 and ls['goal_closed']
 assert len(list((ROOT/'stage83/runs').glob('batch-v1-round*/round.claim.json')))==1
 tests=json.loads((V/'evidence/closure-tests.json').read_text())
 actual=tests['actual_own_capture']
 assert tests['source_keyboard_restore_exact'] and tests['source_device_no_new_attach']
 assert actual['steps']==68 and actual['writes']==124
 assert len(actual['watch']['advances'])==3 and actual['watch']['tail_entries']==0
 assert tests['clock_QPC_frequency']==10000000
 callback=json.loads((V/'evidence/callback-proof.json').read_text())
 assert callback['current_object']==0 and not callback['registration_proven']
 performance=json.loads((V/'runs/own-performance-full-v1/result.json').read_text())
 proc=json.loads((V/'runs/own-performance-full-v1/process.json').read_text())
 assert performance['status']=='OWN_FULL_MIXED_LOOP_PASS'
 assert performance['own_instructions']==14942208 and performance['own_write_records']==3145728
 assert not proc['timed_out'] and proc['returncode']==0 and proc['total_seconds']<60
 snapshot=json.loads((V/'evidence/snapshot-cost.json').read_text())
 assert snapshot['original_engine_emu_start_calls']==0 and snapshot['host_fields_retained']==178
 # Separate, explicit OWN timeline tests the exact reused Device61 algorithm.
 # This instance is never attached to the original source or an original runner.
 sys.path.insert(0,str(ROOT/'samples'))
 from stage61_input import Device61
 own_manifest={'explicit_new_input_domain':True,'acknowledge_legacy_physical_state_unknown':True,
  'purpose':'OWN_ONLY_NONZERO_LATCH_TEST_NOT_SOURCE_ORIGINAL_DOMAIN',
  'timeline':[{'tick':1,'vk':113,'down':True},{'tick':2,'vk':113,'down':False}]}
 device=Device61(own_manifest,{'pending_saved_keyboard_input':False})
 device.advance(1);saved=device.snapshot();restored=Device61.restore(saved)
 assert restored.snapshot()==saved
 values=[restored.query(113,1,'own'),restored.query(113,2,'own')]
 restored.advance(2);values.append(restored.query(113,1,'own'))
 pending=Device61.restore(saved);pending.advance(2);values.append(pending.query(113,1,'own'))
 assert values==[32769,32768,0,1]
 put(V/'evidence/own-key-latch.json',{'status':'REUSED_DEVICE61_NONZERO_SHARED_LATCH_PASS',
  'values':values,'own_timeline':own_manifest['timeline'],'original_source_device_changed':False,
  'original_source_device_attached_or_reset':False,'same_reused_class':True,'original_instructions':0})
 q=subprocess.run(['/usr/bin/python3','-B',str(V/'tools/preflight.py')],cwd=ROOT,
  stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
 assert q.returncode==0
 pre=json.loads(q.stdout);assert not pre['original_engine_imported']
 put(V/'evidence/preflight.json',pre)
 command=['/usr/bin/python3','-B','-c',
  "import sys;from pathlib import Path;sys.path.insert(0,str(Path('stage83/phase7-v2/tools').resolve()));import preflight;sys.argv=['preflight','--original-run'];\ntry:preflight.main()\nexcept Exception as e:print(str(e));print('unicorn_imported='+str('unicorn' in sys.modules));raise SystemExit(2)"]
 q=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
 assert q.returncode==2 and b'unicorn_imported=False' in q.stdout
 (V/'evidence/original-gate.stdout').write_bytes(q.stdout)
 (V/'evidence/original-gate.stderr').write_bytes(q.stderr)
 put(V/'evidence/postflight.json',{'status':'PROTECTED_INPUTS_AND_LEDGER_UNCHANGED',
  'protected_pins':baseline['count'],'all_verified':True,'ledger_sha256':sha(ledger),
  'old_ledger':'20 used / 80 remaining / closed','original_rounds_used':1,'remaining_rounds':9,
  'original_instructions':0,'SR_game_functions':0,'original_reference':0})
 put(V/'evidence/result.json',{'status':'TWO_PREFIX_MODEL_CLOSURES_PASS_CALLBACK_TARGET_ABI_UNPROVEN',
  'closed':['native clocks and cumulative saved Watch','exact existing controlled Device61'],
  'unclosed':['future object/vtable registration and callee ABI at 0x5fc139'],
  'own_integration_steps':68,'own_integration_writes':124,
  'own_performance_instructions':performance['own_instructions'],
  'own_performance_writes':performance['own_write_records'],
  'own_guest_seconds':performance['guest_seconds'],'own_whole_role_seconds':proc['total_seconds'],
  'readonly_restore_snapshot_and_trace_data_seconds':snapshot['whole_process_data_seconds'],
  'proposed_A_caps':pre['proposed_caps'],'new_caps_approved':False,
  'original_diagnostic_executor_implemented':False,'original_execution_entry_closed':True,
  'original_natural_phase8_input_captured':False,'original_instructions':0,
  'old_pins_verified':baseline['count'],'old_ledger':'20 used / 80 remaining / closed',
  'original_rounds_used':1,'remaining_rounds':9,'all_paths_static_guarantee':False})
 verify(baseline['hashes'])
 pins={str(p.relative_to(ROOT)):sha(p) for p in V.rglob('*') if p.is_file() and p.name!='manifest.json'}
 put(V/'manifest.json',{'status':'PREFIX_CLOSURE_DEVELOPMENT_SEALED_ORIGINAL_EXECUTION_CLOSED',
  'files':len(pins),'hashes':pins})
 verify(pins)
 print(json.dumps({'status':'PHASE7_V2_SEALED','files':len(pins),'protected_pins':baseline['count'],
  'manifest_sha256':sha(V/'manifest.json'),'original_instructions':0,'remaining_rounds':9}))

if __name__=='__main__':main()
