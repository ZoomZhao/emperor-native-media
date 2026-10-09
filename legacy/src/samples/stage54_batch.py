"""Own draft budget and authorization gate. No original-tool imports or execution."""
import copy, datetime, fcntl, hashlib, json, os
from pathlib import Path
from stage54_watch import initial_state

R = Path(__file__).resolve().parents[1]
LEDGER = R/'evidence/stage55-batch-ledger.json'
APPROVAL = R/'evidence/stage55-batch-approval.json'
OLD = R/'evidence/stage51-batch-ledger.json'
LIMITS = dict(restore=1, caller=99, frame=99, cycle=99)
SOURCE_SHA = '8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'

class GateError(Exception): pass

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def utc(): return datetime.datetime.now(datetime.timezone.utc).isoformat()
def atomic(path, obj):
    path = Path(path)
    temp = path.with_name(path.name+'.pending')
    with temp.open('x') as f:
        json.dump(obj, f, ensure_ascii=False, indent=2); f.write('\n'); f.flush(); os.fsync(f.fileno())
    os.replace(temp, path)

def draft(old_sha, pinned_files):
    return dict(version=1, authorization_status='pending', maximum_processes=100,
                hard_process_seconds=60, original_segment_seconds=14, mode_limits=LIMITS,
                caps=dict(productive_advances=51, tail_entries=1, scheduler_entries=128),
                source_EXE_sha256=SOURCE_SHA, old_ledger_sha256=old_sha, pinned_files=pinned_files,
                goal_closed=False, executions=[], remaining_processes=100,
                latest_checkpoint='returnwatch51', watch_state=initial_state(),
                cold_restore_passed=False, caller_passed=False, city_frame_review=None)

def load_authorization(path, ledger):
    if not Path(path).is_file(): raise GateError('explicit new hundred-process approval has not arrived')
    a = json.loads(Path(path).read_text())
    required = dict(status='approved', source_thread_id='01a10af1-425e-75ae-bded-bf47f843a029',
                    scope_id='stage55-approved100-cold-caller-frame-finite-cycle', maximum_processes=100,
                    hard_process_seconds=60, source_EXE_sha256=SOURCE_SHA,
                    old_remaining_must_not_be_repurposed=True,
                    no_original_DLL_execution=True, source_read_only=True, task_only_writes=True,
                    no_upload=True, no_host_settings_changes=True, source_PCM_to_device=False,
                    caps=ledger['caps'])
    if any(a.get(k) != v for k,v in required.items()): raise GateError('approval scope does not match this fixed plan')
    if a.get('user_reply') != '允许，一次性批准 100 次' or not a.get('user_approval_utc') or not a.get('approval_request_text'):
        raise GateError('actual explicit user reply, time and requested scope text are required')
    try:
        when=datetime.datetime.fromisoformat(a['user_approval_utc'].replace(' UTC','+00:00').replace('Z','+00:00'))
        if when.tzinfo is None or when<=datetime.datetime(2026,10,7,5,21,4,tzinfo=datetime.timezone.utc):
            raise ValueError('not a new dated approval')
    except (TypeError,ValueError):raise GateError('new actual timezone-qualified approval time required')
    if Path(path)==APPROVAL and a.get('synthetic_fixture'):
        raise GateError('synthetic fixture can never authorize the production ledger')
    manifest=ledger.get('prepared_manifest_sha256')
    if not isinstance(manifest,str) or len(manifest)!=64 or a.get('prepared_manifest_sha256') != manifest:
        raise GateError('approval record does not identify the prepared source manifest')
    return a

def validate(ledger):
    if (ledger['version'] != 1 or ledger['maximum_processes'] != 100 or ledger['hard_process_seconds'] != 60
            or ledger['original_segment_seconds'] != 14 or ledger['mode_limits'] != LIMITS
            or ledger['caps'] != dict(productive_advances=51, tail_entries=1, scheduler_entries=128)
            or ledger['source_EXE_sha256'] != SOURCE_SHA
            or ledger['remaining_processes'] != 100-len(ledger['executions'])):
        raise GateError('budget or contract changed')

def claim(mode, resume, tag, argv, path=LEDGER, approval=APPROVAL, old_path=OLD, check_pins=True):
    path = Path(path)
    if mode not in LIMITS or not tag.replace('-','').isalnum(): raise GateError('invalid mode/tag')
    with path.with_name(path.name+'.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        ledger = json.loads(path.read_text()); validate(ledger)
        a = load_authorization(approval, ledger)
        if sha(old_path) != ledger['old_ledger_sha256']: raise GateError('old closed ledger changed')
        if ledger['goal_closed'] or len(ledger['executions']) >= 100: raise GateError('batch closed or exhausted')
        if any(q['status'] == 'claimed' for q in ledger['executions']): raise GateError('prior process outcome unresolved; no automatic retry')
        if any(q['tag'] == tag for q in ledger['executions']): raise GateError('tag already claimed')
        if sum(q['mode'] == mode for q in ledger['executions']) >= LIMITS[mode]: raise GateError('mode allocation exhausted')
        if resume != ledger['latest_checkpoint']: raise GateError('resume must use the latest recorded checkpoint')
        if mode == 'restore' and ledger['executions']: raise GateError('cold restore must be first')
        if mode != 'restore' and not ledger['cold_restore_passed']: raise GateError('cold restore has not passed')
        if mode=='caller' and ledger['caller_passed']:raise GateError('caller goal already achieved; do not repeat it')
        if mode=='frame' and ledger['watch_state']['frame_candidate']:raise GateError('fresh frame candidate already saved; review it before cycle')
        if mode in ('frame','cycle') and not ledger['caller_passed']: raise GateError('caller return has not passed')
        if mode == 'cycle' and not ledger['city_frame_review']: raise GateError('fresh city candidate has not been reviewed')
        if (R/('runs/stage12-state-'+tag)).exists() and path == LEDGER: raise GateError('checkpoint destination already exists')
        if check_pins:
            for p, expected in ledger['pinned_files'].items():
                if sha(p) != expected: raise GateError('pinned source/input changed: '+p)
            for q in reversed(ledger['executions']):
                if q.get('checkpoint') == resume:
                    for f in q['checkpoint_files']:
                        if sha(R/f['path']) != f['sha256']:raise GateError('latest checkpoint changed')
                    break
        index = len(ledger['executions'])+1
        ledger['authorization_status'] = 'approved'
        ledger['executions'].append(dict(index=index, mode=mode, resume=resume, tag=tag,
                                         argv=list(argv), claimed_utc=utc(), status='claimed',
                                         before_runtime_import_and_construction=True,
                                         consumes_one_process_even_if_restore_only_or_failure=True,
                                         approval_sha256=sha(approval)))
        ledger['remaining_processes'] = 100-index
        atomic(path, ledger)
        return index, copy.deepcopy(ledger)

def finish(index, result, watch_state, checkpoint=None, checkpoint_files=None,
           path=LEDGER, old_path=OLD):
    from stage54_watch import Watch
    Watch('restore', watch_state).validate()
    path = Path(path)
    with path.with_name(path.name+'.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        ledger = json.loads(path.read_text()); validate(ledger)
        if sha(old_path) != ledger['old_ledger_sha256']: raise GateError('old closed ledger changed')
        q = ledger['executions'][index-1]
        if q['index'] != index or q['status'] != 'claimed': raise GateError('execution is not the active claim')
        previous = ledger['watch_state']
        if (watch_state['entries'] < previous['entries'] or watch_state['tail_entries'] < previous['tail_entries']
                or watch_state['advances'][:len(previous['advances'])] != previous['advances']):
            raise GateError('shared observation count or sequence reset')
        q.update(status='finished', finished_utc=utc(), result=result, checkpoint=checkpoint,
                 checkpoint_files=checkpoint_files or [])
        ledger['watch_state'] = copy.deepcopy(watch_state)
        if checkpoint:
            if checkpoint != q['tag'] or len(checkpoint_files or []) != 3:
                raise GateError('saved checkpoint identity is incomplete')
            ledger['latest_checkpoint'] = checkpoint
        if result.get('status') == 'cold_restore_passed': ledger['cold_restore_passed'] = True
        if result.get('boundary',{}).get('kind') == 'Caller_return_goal_stop54': ledger['caller_passed'] = True
        kind = result.get('boundary',{}).get('kind','')
        intentional = {'Caller_return_goal_stop54','Fresh_city_frame_candidate_stop54','Finite_scheduler_cap_stop54'}
        if result.get('status') in ('failed','refused') and kind not in intentional:
            ledger.update(goal_closed=True, stop_reason='precise failed/refused boundary; hand back before another process')
        if kind == 'Finite_scheduler_cap_stop54' or ledger['remaining_processes'] == 0:
            ledger.update(goal_closed=True, stop_reason='finite scheduler cap or process budget reached')
        atomic(path, ledger)

def record_frame_review(record, path=LEDGER):
    """Own local image review only; this is evidence, never execution approval."""
    path = Path(path)
    with path.with_name(path.name+'.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        ledger = json.loads(path.read_text()); validate(ledger)
        c = ledger['watch_state']['frame_candidate']
        if not c or record.get('passed') is not True or record.get('checkpoint') != ledger['latest_checkpoint']:
            raise GateError('review is not tied to the saved fresh frame candidate')
        if (not c.get('owned_window_capture')
                or record.get('PNG_path') != c['PNG_path'] or record.get('PNG_sha256') != sha(c['PNG_path'])
                or not record.get('city_content_observation') or record.get('old_menu_or_restore_presentation') is not False):
            raise GateError('actual local city-content review and matching owned capture hash required')
        ledger['city_frame_review'] = copy.deepcopy(record)
        atomic(path, ledger)
