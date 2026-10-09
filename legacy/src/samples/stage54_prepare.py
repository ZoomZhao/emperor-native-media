"""Prepare an unapproved draft ledger and source manifest; read-only original inputs."""
import ast, json, datetime
from pathlib import Path
from stage54_batch import draft, sha, atomic, LEDGER, APPROVAL, OLD, R, GateError

def main():
    if LEDGER.exists() or APPROVAL.exists(): raise GateError('never reset an existing ledger or replace approval')
    old=json.loads(OLD.read_text())
    if not old['goal_closed'] or len(old['executions'])!=9 or old['remaining_original_executions']!=1:
        raise GateError('old batch must remain closed at9 used/1 unused')
    baseline=json.loads((R/'evidence/stage52-postflight.json').read_text())
    known={q['path']:q['sha256'] for q in baseline['checks']}
    pins={}
    for p in (R/'samples').iterdir():
        if p.suffix not in ('.py','.c','.h','.llasm'):continue
        actual=sha(p)
        if str(p) in known and known[str(p)]!=actual:raise GateError('old protected source changed: '+str(p))
        pins[str(p)]=actual
    for path,expected in known.items():
        p=Path(path)
        if p.suffix=='.dylib' or str(p).startswith(str(R/'vendor-tools/unicorn-2.1.4')+'/'):
            if sha(p)!=expected:raise GateError('approved native/engine input changed: '+path)
            pins[path]=expected
    for p in (R/'evidence').glob('*.json'):
        if p.name.startswith('stage54-') or p.stat().st_size>5000000:continue
        if p.name.endswith(('-build.json','-contract.json','-resources-hashed.json','-source.json','-sources.json')) or p.name=='pe-metadata.json':
            actual=sha(p)
            if str(p) in known and known[str(p)]!=actual:raise GateError('old protected platform metadata changed: '+str(p))
            pins[str(p)]=actual
    fixed=json.loads((R/'evidence/stage53-static-review.json').read_text())
    pins[fixed['source_EXE']]=fixed['source_EXE_sha256']
    for q in fixed['checkpoint_files_unchanged']:
        p=R/q['path']
        if sha(p)!=q['sha256']:raise GateError('final paused checkpoint changed')
        pins[str(p)]=q['sha256']
    if sha(OLD)!=fixed['ledger_sha256']:raise GateError('old closed ledger changed')
    wrapper=R/'samples/checkpoint_startup54.py';tree=ast.parse(wrapper.read_text())
    top_imports=[n.module for n in tree.body if isinstance(n,ast.ImportFrom)]
    if any(m not in ('pathlib','stage54_batch','stage54_watch') for m in top_imports):
        raise GateError('wrapper imports a runtime before its approval gate')
    manifest=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  purpose='prepared source manifest, not user approval',authorization_received=False,
                  old_ledger_sha256=sha(OLD),old_used=9,old_remaining_unused=1,old_goal_closed=True,
                  source_EXE_sha256=fixed['source_EXE_sha256'],checkpoint='returnwatch51',
                  pinned_files=pins,pinned_file_count=len(pins),
                  wrapper_top_level_imports=top_imports,original_instructions_executed=0,
                  proposed_maximum_processes=100,proposed_hard_process_seconds=60,
                  shared_caps=dict(productive_advances=51,tail_entries=1,scheduler_entries=128))
    target=R/'evidence/stage54-prepared-manifest.json';atomic(target,manifest)
    ledger=draft(sha(OLD),pins);ledger['prepared_manifest_sha256']=sha(target)
    atomic(LEDGER,ledger)
    print(json.dumps(dict(authorization_status='pending',executions=0,remaining_processes=100,
                         pinned_file_count=len(pins),prepared_manifest_sha256=ledger['prepared_manifest_sha256'],
                         original_instructions_executed=0,old_ledger_unchanged=True)))
if __name__=='__main__':main()
