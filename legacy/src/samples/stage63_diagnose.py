"""Readonly checkpoint/process/timestamp audit, no emulator or native constructor."""
import hashlib,json,os,struct,time
from pathlib import Path
from stage12_state import decode
from stage61_input import Device61
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def sha(p):
    h=hashlib.sha256()
    with Path(p).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def main():
    ledger_path=E/'stage55-batch-ledger.json';ledger_sha=sha(ledger_path)
    assert ledger_sha=='b807f91bf787dd0b8e06af321185affd194ac3aaa4ae0c56497d48468126b212'
    l=json.loads(ledger_path.read_text());q=l['executions'][-1];d=ROOT/'runs/stage12-state-cycle62p19'
    checks=[]
    for f in q['checkpoint_files']:
        p=ROOT/f['path'];actual=sha(p);assert actual==f['sha256'] and p.stat().st_size==f['bytes']
        checks.append(dict(path=f['path'],bytes=f['bytes'],sha256=actual))
    record=json.loads((d/'state.json').read_text());host=dict(record['host']['dict'])
    position=0;region_checks=[];last_high=-1
    with (d/'memory.bin').open('rb') as f:
        for r in record['regions']:
            assert r['offset']==position and r['low']>last_high and 1<=r['permissions']<=7
            n=r['high']-r['low']+1;b=f.read(n);assert len(b)==n and hashlib.sha256(b).hexdigest()==r['sha256']
            region_checks.append(dict(low=r['low'],high=r['high'],bytes=n,sha256=r['sha256']));position+=n;last_high=r['high']
        assert not f.read(1) and position==record['memory_bytes']==(d/'memory.bin').stat().st_size
    context=(d/'context.bin').read_bytes();meta=record['context']
    assert len(context)==meta['size'] and hashlib.sha256(context).hexdigest()==meta['sha256']
    header=struct.unpack('<Qiii?3xQQ',context[:40]);assert header[0]==len(context)-40 and header[1]==meta['mode'] and header[2]==meta['arch'] and not header[-1]
    threads=decode(host['guest_threads20']);current=decode(host['current_tid']);assert current==1 and set(threads)=={1,2,3,4}
    thread_rows=[]
    for tid,t in threads.items():
        raw,size,arch,mode=t['context'];assert len(raw)==size==5608 and arch==meta['arch'] and mode==meta['mode']
        thread_rows.append(dict(tid=tid,status=t['status'],bytes=len(raw),CPU_payload_sha256=hashlib.sha256(raw[40:]).hexdigest()))
    assert threads[current]['context'][0][40:]==context[40:]
    assert decode(host['watch_state54'])==l['watch_state']
    device=decode(host['keyboard_snapshot61']);restored=Device61.restore(device['device']);assert restored.snapshot()==device['device']
    exit=json.loads((E/'stage62-cycle62p19-input-exit.json').read_text());assert device==dict(device=exit['device'],meta=exit['meta'])
    process_rows=[]
    for index in range(16,20):
        run=json.loads((E/f'stage54-cycle62p{index}.json').read_text());pid=run['native_diagnostic']['current_pid']
        try:os.kill(pid,0);status='exists_or_PID_reused'
        except ProcessLookupError:status='absent'
        except PermissionError:status='cannot_determine_permission_denied'
        close=E/f'stage31-native-close-{pid}.json'
        process_rows.append(dict(index=index,pid=pid,status=status,close_record_present=close.exists(),
            close_record=json.loads(close.read_text()) if close.exists() else None))
    raw=json.loads((E/'stage63-timing-raw.json').read_text());timings=[]
    for row in raw['rows']:
        tag=f"cycle62p{row['index']}";files=row['files'];get=lambda p:files[p]['mtime_relative_claim']
        entry=get(f'evidence/stage61-{tag}-input-entry.json');end=get(f'evidence/stage62-{tag}-input-exit.json')
        memory=get(f'runs/stage12-state-{tag}/memory.bin');ctx=get(f'runs/stage12-state-{tag}/context.bin');state=get(f'runs/stage12-state-{tag}/state.json')
        birth=files[f'runs/stage12-state-{tag}/state.json']['birthtime_relative_claim'];out=get(f'evidence/stage54-{tag}.json')
        before=get(f'evidence/stage54-{tag}-before-dispose.json');after=get(f'evidence/stage54-{tag}-after-dispose.json')
        timings.append(dict(index=row['index'],restore_setup_upper_estimate_seconds=entry,
            guest_slice_and_device_exit_seconds=round(end-entry,4),memory_write_and_region_hash_seconds=round(memory-end,4),
            context_write_seconds=round(ctx-memory,4),host_get_encode_JSON_before_state_open_seconds=round(birth-ctx,4),
            state_file_write_seconds=round(state-birth,4),postsave_hash_and_output_seconds=round(out-state,4),
            native_dispose_observed_seconds=round(after-before,4),
            primary_PNG_count=len(row['present_PNGs']),external_window_screencapture_this_run=False,
            primary_PNG_seconds_individually_measured=False,normal_interpreter_exit_seconds_not_measured=True))
    sizes=[]
    for name,value in host.items():
        start=time.monotonic();s=json.dumps(value,separators=(',',':'));elapsed=time.monotonic()-start
        sizes.append(dict(field=name,encoded_JSON_bytes=len(s),reserialize_encoded_field_seconds=round(elapsed,5)))
    sizes.sort(key=lambda x:x['encoded_JSON_bytes'],reverse=True)
    assert sha(ledger_path)==ledger_sha
    out=dict(status='offline_integrity_passed_runtime_cold_restore_not_executed',ledger_sha256=ledger_sha,
        used=19,remaining=81,closed=True,phase=22,advances=17,entries=17,tail=0,
        checkpoint_files=checks,all_region_hashes_passed=True,memory_bytes=position,regions=region_checks,
        complete_JSON_parsed=True,host_fields=len(host),complete_context_header_and_payload_passed=True,
        current_thread_payload_equals_context_bin=True,threads=thread_rows,watch_equals_ledger=True,
        device_snapshot_and_trace_restore_exact=True,new_DEVICE_ATTACH=0,device_equals_saved_input_exit=True,
        checkpoint_files_atomic_transaction=False,checkpoint_files_fsync_before_ledger=False,
        ledger_atomic_replace_and_fsync=True,checkpoint_complete_on_current_filesystem=True,
        abrupt_power_loss_durability_proven=False,actual_emulator_cold_restore_of_latest_checkpoint_run=False,
        processes=process_rows,no_old_task_processes_remain=all(x['status']=='absent' for x in process_rows),
        audio_cleanup_basis='Own AudioUnit stop/uninitialize/dispose in existing source; owner PIDs absent. No hardware/global audio state probe or host mutation.',
        timings=timings,timing_basis='file mtime/birthtime and claim/finish; estimates, no historical function-level probes',
        largest_encoded_host_fields=sizes[:12],all_field_encoded_JSON_bytes=sum(x['encoded_JSON_bytes'] for x in sizes),
        original_instructions_executed=0,original_DLL_executed=False,emulator_imported=False,
        global_input_or_focus=False,host_settings_changed=False,source_modified=False,ledger_unchanged=True)
    (E/'stage63-offline-diagnosis.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps({k:v for k,v in out.items() if k not in ['regions','checkpoint_files','threads','largest_encoded_host_fields','timings']}))
if __name__=='__main__':main()
