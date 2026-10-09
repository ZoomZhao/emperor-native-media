"""Reopen only the approved input-domain boundary in the same 100-process batch."""
import copy,fcntl,json
from pathlib import Path
from stage54_batch import APPROVAL,LEDGER,OLD,atomic,load_authorization,sha,utc,validate
from stage61_prepare import PROVENANCE,OLD_SHA
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    assert sha(LEDGER)==OLD_SHA
    manifest_path=E/'stage61-prepared-continuation-manifest.json'
    manifest=json.loads(manifest_path.read_text());prepared_sha=sha(manifest_path)
    check=json.loads((E/'stage61-integration-selfcheck.json').read_text());assert check['passed'] and check['original_instructions']==0
    with LEDGER.with_name(LEDGER.name+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        ledger=json.loads(LEDGER.read_text());approval=json.loads(APPROVAL.read_text())
        validate(ledger);load_authorization(APPROVAL,ledger)
        assert ledger['goal_closed'] and len(ledger['executions'])==4 and ledger['remaining_processes']==96
        assert ledger['latest_checkpoint']=='frame56a'
        assert ledger['executions'][-1]['result']['boundary']['kind']=='keyboard_consumer_not_reviewed'
        assert all(q['status']=='finished' for q in ledger['executions']) and sha(OLD)==ledger['old_ledger_sha256']
        assert all(manifest['pinned_files'].get(p)==h for p,h in ledger['pinned_files'].items())
        for p,h in manifest['pinned_files'].items():assert sha(p)==h,p
        for q in ledger['executions'][-1]['checkpoint_files']:assert sha(ROOT/q['path'])==q['sha256']
        before=copy.deepcopy(ledger)
        received=dict(PROVENANCE,status='approved_input_domain_continuation_received_from_parent',
            received_utc=utc(),existing_ledger=str(LEDGER.relative_to(ROOT)),previous_ledger_sha256=OLD_SHA,
            previous_approval_sha256=sha(APPROVAL),prepared_manifest_sha256=prepared_sha,
            input_contract_sha256=manifest['input_contract_sha256'],maximum_processes=100,
            already_used=4,remaining=96,reset_allowed=False,next_execution_index=5,resume='frame56a',
            fixed_emulator='Unicorn 2.1.4',source_EXE_sha256=ledger['source_EXE_sha256'],
            hard_process_seconds=60,original_segment_seconds=14,caps=copy.deepcopy(ledger['caps']),
            controlled_test_keyboard_only=True,physical_keyboard_compatibility_paused=True,
            no_host_keyboard_listener=True,no_original_DLL_execution=True,source_PCM_to_device=False,
            source_read_only=True,task_only_writes=True,no_upload=True,no_host_settings_changes=True,
            precise_unknown_behavior_requires_stop=True,whole_restore_execution_counts=True)
        for source,target in [(LEDGER,E/'stage61-ledger-before-continuation.json'),(APPROVAL,E/'stage61-approval-before-continuation.json')]:
            with target.open('xb') as f:f.write(source.read_bytes())
        with (E/'stage61-continuation-approval.json').open('x') as f:json.dump(received,f,ensure_ascii=False,indent=2);f.write('\n')
        approval['prepared_manifest_sha256']=prepared_sha
        approval['continuation_approval_records']=approval.get('continuation_approval_records',[])+[received]
        ledger.update(prepared_manifest_sha256=prepared_sha,pinned_files=manifest['pinned_files'],goal_closed=False,stop_reason=None)
        ledger['closure_history']=ledger.get('closure_history',[])+[dict(old_ledger_sha256=OLD_SHA,
            reason=before['stop_reason'],boundary=before['executions'][-1]['result']['boundary'],
            reopened_utc=received['received_utc'],approval_path='evidence/stage61-continuation-approval.json')]
        ledger['continuation_approval_records']=ledger.get('continuation_approval_records',[])+[received]
        for k in ['executions','watch_state','remaining_processes','mode_limits','caps','latest_checkpoint','cold_restore_passed','caller_passed']:
            assert ledger[k]==before[k],k
        validate(ledger);atomic(APPROVAL,approval);atomic(LEDGER,ledger);load_authorization(APPROVAL,ledger)
        out=dict(status='same_batch_controlled_input_boundary_reopened',used=4,remaining=96,maximum=100,
            next_index=5,pinned_files=len(ledger['pinned_files']),original_history_and_watch_unchanged=True,
            original_instructions_executed=0,approval_sha256=sha(APPROVAL),ledger_sha256=sha(LEDGER),
            prepared_manifest_sha256=prepared_sha)
        (E/'stage61-continuation-preflight.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
