"""Extend pins on the still-open approved batch; keep all histories/counters."""
import ast,copy,fcntl,json
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,OLD,sha,atomic,validate,load_authorization,utc
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
EXPECTED='c3fac0230f9346b632a10d95f96fed17adadb1af584c68c260385c8fd3f7bf26'
def main():
    assert sha(LEDGER)==EXPECTED
    names=['checkpoint_startup62.py','stage62_prepare.py','stage62_cycle_driver.py']
    for name in names:ast.parse((ROOT/'samples'/name).read_text())
    with LEDGER.with_name(LEDGER.name+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        ledger=json.loads(LEDGER.read_text());approval=json.loads(APPROVAL.read_text());before=copy.deepcopy(ledger)
        validate(ledger);load_authorization(APPROVAL,ledger)
        assert not ledger['goal_closed'] and len(ledger['executions'])==15 and ledger['remaining_processes']==85
        assert ledger['latest_checkpoint']=='cycle61p15'
        w=ledger['watch_state'];assert w['phase']==17 and len(w['advances'])==w['entries']==12 and w['tail_entries']==0
        assert all(q['status']=='finished' for q in ledger['executions']) and sha(OLD)==ledger['old_ledger_sha256']
        for p,h in ledger['pinned_files'].items():assert sha(p)==h,p
        for q in ledger['executions'][-1]['checkpoint_files']:assert sha(ROOT/q['path'])==q['sha256']
        additions={str(ROOT/'samples'/p):sha(ROOT/'samples'/p) for p in names}
        pins=copy.deepcopy(ledger['pinned_files']);pins.update(additions)
        record=dict(source='parent continuation instruction within existing user-approved scope; not a new direct user approval',
            source_thread_id='01a10af1-425e-75ae-bded-bf47f843a029',received_utc=utc(),
            existing_user_approval_path='evidence/stage61-continuation-approval.json',
            existing_user_approval_sha256=sha(E/'stage61-continuation-approval.json'),
            instruction='Resume cycle61p15 at index16 with same input state and cumulative12 advances/12 entries/0 tails; stop first allowed tail or previous bounds; hard60 seconds per original process; no new permission question.',
            implementation_change='Use approved original_segment_seconds14 instead of narrower10; same saved device and original Watch/restore/gate',
            previous_ledger_sha256=EXPECTED,previous_approval_sha256=sha(APPROVAL),
            maximum_processes=100,used=15,remaining=85,next_index=16,reset_allowed=False,
            original_instructions_executed=0)
        manifest=dict(version=1,prepared_utc=utc(),previous_prepared_manifest_sha256=ledger['prepared_manifest_sha256'],
            pinned_files=pins,new_sources=additions,all_previous_pins_preserved=True,
            actual_original_segment_seconds=14,hard_process_seconds=60,
            input_domain_changed=False,counters_reset=False,provenance=record)
        target=E/'stage62-prepared-continuation-manifest.json'
        with target.open('x') as f:json.dump(manifest,f,ensure_ascii=False,indent=2);f.write('\n')
        for source,target in [(LEDGER,E/'stage62-ledger-before-continuation.json'),(APPROVAL,E/'stage62-approval-before-continuation.json')]:
            with target.open('xb') as f:f.write(source.read_bytes())
        prepared_sha=sha(E/'stage62-prepared-continuation-manifest.json')
        approval['prepared_manifest_sha256']=prepared_sha
        approval['implementation_continuation_records']=approval.get('implementation_continuation_records',[])+[record]
        ledger['prepared_manifest_sha256']=prepared_sha;ledger['pinned_files']=pins
        ledger['implementation_continuation_records']=ledger.get('implementation_continuation_records',[])+[record]
        for k in ['executions','watch_state','remaining_processes','latest_checkpoint','goal_closed','stop_reason','caps','mode_limits']:
            assert ledger[k]==before[k],k
        atomic(APPROVAL,approval);atomic(LEDGER,ledger);load_authorization(APPROVAL,ledger)
        out=dict(status='same_open_batch_pins_extended',used=15,remaining=85,next_index=16,
            pinned_files=len(pins),device_domain_unchanged=True,counters_unchanged=True,
            original_instructions_executed=0,ledger_sha256=sha(LEDGER),prepared_manifest_sha256=prepared_sha)
        (E/'stage62-preflight.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
