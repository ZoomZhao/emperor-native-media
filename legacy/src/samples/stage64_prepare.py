"""Lift only the verified whole-process timeout pause; preserve every execution."""
import ast,copy,fcntl,json
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,OLD,sha,atomic,validate,load_authorization,utc
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
EXPECTED='b807f91bf787dd0b8e06af321185affd194ac3aaa4ae0c56497d48468126b212'
INSTRUCTION='核查支持同范围正常超时恢复，按用户已批准100次及当前受控输入继续一次新的第20次，计入原账本不免费重跑。仅解除由运行器超时导致的技术暂停，保留历史19超时事实；不扩大来源/权限/60秒/调度停止条件。使用已自写验证的checkpoint_startup63.py新wrapper先完整冷恢复校验cycle62p19，按实际剩余时间决定是否运行guest，最多8秒预留25秒；不够保存退出则不执行guest并如实计本次。保持17/51、17/128、0/1phase22，设备不清零；无宿主环境/全局输入/声音/上传/原DLL。若第20次仍无法在60秒安全正常退出，就停止进一步原运行交回，不批量消耗余81。若通过，报告实际各段耗时再按同界限逐次继续到原目标，约20分钟检查点。若自动审批要求新授权，停止报告，不换路径。'
def main():
    assert sha(LEDGER)==EXPECTED
    prepared=E/'stage63-prepared-continuation-manifest.json';m=json.loads(prepared.read_text())
    assert m['status']=='PREPARED_ONLY_NOT_EXECUTION_APPROVAL' and m['existing_ledger_sha256']==EXPECTED
    with LEDGER.with_name(LEDGER.name+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        l=json.loads(LEDGER.read_text());a=json.loads(APPROVAL.read_text());before=copy.deepcopy(l)
        validate(l);load_authorization(APPROVAL,l)
        assert sha(LEDGER)==EXPECTED and sha(OLD)==l['old_ledger_sha256']
        assert l['goal_closed'] and len(l['executions'])==19 and l['remaining_processes']==81 and l['latest_checkpoint']=='cycle62p19'
        w=l['watch_state'];assert w['phase']==22 and len(w['advances'])==w['entries']==17 and w['tail_entries']==w['tail_returns']==0
        assert all(q['status']=='finished' for q in l['executions'])
        assert l['executions'][-1]['result']['boundary']['kind']=='Whole_process_hard_timeout62'
        assert l['executions'][-1]['whole_process_outcome']['status']=='hard_timeout'
        for p,h in m['pinned_files'].items():assert sha(p)==h,p
        assert all(m['pinned_files'][p]==h for p,h in l['pinned_files'].items())
        for f in l['executions'][-1]['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
        for name in ['checkpoint_startup63.py','stage63_checkpoint.py','stage64_prepare.py']:ast.parse((ROOT/'samples'/name).read_text())
        record=dict(source='forwarded parent instruction within existing approval; not a new user approval',
            source_thread_id='01a10af1-425e-75ae-bded-bf47f843a029',received_utc=utc(),instruction=INSTRUCTION,
            previous_ledger_sha256=EXPECTED,previous_approval_sha256=sha(APPROVAL),
            existing_input_approval_path='evidence/stage61-continuation-approval.json',existing_input_approval_sha256=sha(E/'stage61-continuation-approval.json'),
            reviewed_recovery_result_path='evidence/stage63-result.json',reviewed_recovery_result_sha256=sha(E/'stage63-result.json'),
            reviewed_prepared_manifest_sha256=sha(prepared),next_index=20,used=19,remaining=81,
            maximum_processes=100,hard_process_seconds=60,guest_max_seconds=8,reserve_seconds=25,
            only_timeout_technical_pause_lifted=True,all_previous_executions_preserved=True,
            new_user_approval_fabricated=False,reset_allowed=False,original_instructions_executed=0)
        pins=copy.deepcopy(m['pinned_files']);own=str(ROOT/'samples/stage64_prepare.py');pins[own]=sha(own)
        manifest=dict(version=1,status='SAME_SCOPE_TECHNICAL_TIMEOUT_RECOVERY_BOUND',pinned_files=pins,
            previous_prepared_manifest_sha256=l['prepared_manifest_sha256'],provenance=record,
            caps=l['caps'],source_EXE_sha256=l['source_EXE_sha256'],input_domain_changed=False,
            original_DLL_execution=False,hard_process_seconds=60,guest_max_seconds=8,reserve_seconds=25)
        path=E/'stage64-bound-continuation-manifest.json'
        with path.open('x') as f:json.dump(manifest,f,ensure_ascii=False,indent=2);f.write('\n')
        for source,target in [(LEDGER,E/'stage64-ledger-before-timeout-recovery.json'),(APPROVAL,E/'stage64-approval-before-timeout-recovery.json')]:
            with target.open('xb') as f:f.write(source.read_bytes())
        bound=sha(path);a['prepared_manifest_sha256']=bound
        a['implementation_continuation_records']=a.get('implementation_continuation_records',[])+[record]
        l['prepared_manifest_sha256']=bound;l['pinned_files']=pins
        l['implementation_continuation_records']=l.get('implementation_continuation_records',[])+[record]
        l['closure_history']=l.get('closure_history',[])+[dict(old_ledger_sha256=EXPECTED,reason=l['stop_reason'],
            boundary=copy.deepcopy(l['executions'][-1]['result']['boundary']),reopened_utc=record['received_utc'],
            provenance_path='evidence/stage64-bound-continuation-manifest.json',technical_timeout_recovery_only=True)]
        l['goal_closed']=False;l.pop('stop_reason')
        for k in ['executions','watch_state','latest_checkpoint','remaining_processes','caps','mode_limits','city_frame_review','external_timeout_reconciliation']:
            assert l[k]==before[k],k
        atomic(APPROVAL,a);atomic(LEDGER,l);load_authorization(APPROVAL,l)
        result=dict(status='same_scope_timeout_pause_lifted',used=19,remaining=81,next_index=20,
            latest='cycle62p19',phase=22,advances=17,entries=17,tail=0,ledger_sha256=sha(LEDGER),
            pinned_files=len(pins),original_instructions_executed=0,old_timeout_preserved=True)
        (E/'stage64-preflight.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
