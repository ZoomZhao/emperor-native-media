"""Read-only evidence consolidation; no emulator or native runtime imports."""
import datetime
import hashlib
import json
from pathlib import Path

R=Path(__file__).resolve().parents[1]
E=R/'evidence'
cache={}


def load(name):return json.loads((E/name).read_text())


def sha(path):
    p=Path(path)
    if str(p) not in cache:
        h=hashlib.sha256()
        with p.open('rb') as f:
            for b in iter(lambda:f.read(1048576),b''):h.update(b)
        cache[str(p)]=h.hexdigest()
    return cache[str(p)]


def main():
    ledger=load('stage55-batch-ledger.json')
    before=sha(E/'stage55-batch-ledger.json')
    assert before=='5ed6414885425fa973e6d8de6165784f090639113ed1bc878d5fccb23457b931'
    assert len(ledger['executions'])==3 and ledger['remaining_processes']==97 and ledger['goal_closed']
    assert ledger['latest_checkpoint']=='frame55a'
    assert ledger['watch_state']['entries']==0 and not ledger['watch_state']['advances']
    assert ledger['watch_state']['tail_returns']==0
    tests={mode:load(f'stage56-music-{mode}.json') for mode in ['create','restore','adapter']}
    assert tests['create']['behavioral_contract_tests_passed'] and not tests['create']['passed']
    assert tests['restore']['passed'] and tests['adapter']['passed']
    cases=[q for t in tests.values() for q in t['cases']]
    failures=[q for q in cases if not q['passed']]
    assert len(failures)==1 and failures[0]['name']=='independent afconvert PCM byte identity'
    assert failures[0]['frame_count_equal']
    result=tests['create']['result']
    assert result['city_end_control']['consumed_frames']==result['city_open']['decoded_frames']==3214656
    assert tests['restore']['result']['future_behavior_matches']
    assert all(t['original_instructions_executed']==0 and t['closed_original_ledger_unchanged'] for t in tests.values())
    labels=['stage56-own-music-create-host-v2','stage56-own-music-independent-restore',
            'stage56-own-music-guest32-adapter']
    timings=[dict(label=n,**load(n+'.result.json')) for n in labels]
    assert all(not t['timed_out'] and t['duration_seconds']<60 for t in timings)
    assert timings[0]['returncode']==1 and all(t['returncode']==0 for t in timings[1:])
    checks=[]
    for q in load('stage55-authorized-postflight.json')['checks']:
        actual=sha(q['path'])
        checks.append(dict(path=q['path'],sha256=actual,expected_sha256=q['sha256'],
                           unchanged=actual==q['sha256']))
    prior=load('stage55-authorized-postflight.json')
    missing=[]
    for q in prior['absences']:
        p=Path(q['path'])
        missing.append(dict(path=str(p),still_missing=not p.exists() and(
            not p.parent.is_dir() or not any(c.name.casefold()==p.name.casefold() for c in p.parent.iterdir()))))
    links=[]
    for q in prior['DOS_namespace_symlinks']:
        p=Path(q['path']);target=str(p.readlink()) if p.is_symlink() else None
        links.append(dict(path=str(p),target=target,unchanged=target==q['target']))
    audit=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
               hash_checks=len(checks),unique_paths=len(cache),checks=checks,
               all_protected_files_unchanged=all(q['unchanged'] for q in checks),
               absences=missing,all_missing_inputs_unchanged=all(q['still_missing'] for q in missing),
               DOS_namespace_symlinks=links,all_symlinks_unchanged=all(q['unchanged'] for q in links))
    assert audit['all_protected_files_unchanged'] and audit['all_missing_inputs_unchanged'] and audit['all_symlinks_unchanged']
    assert hashlib.sha256((E/'stage55-batch-ledger.json').read_bytes()).hexdigest()==before
    (E/'stage56-postflight.json').write_text(json.dumps(audit,indent=2)+'\n')
    source_names=['stage56_music.py','stage56_stream_adapter.py','stage56_music_probe.py',
                  'checkpoint_startup56.py','stage56_finalize.py']
    additions={str(R/'samples'/name):sha(R/'samples'/name) for name in source_names}
    new_pins=dict(ledger['pinned_files']);new_pins.update(additions)
    manifest=dict(status='prepared; does not grant approval or reopen ledger',
                  old_closed_ledger_sha256=before,old_used=3,remaining=97,
                  old_manifest_sha256=ledger['prepared_manifest_sha256'],
                  pinned_files=new_pins,pinned_file_count=len(new_pins),new_sources=additions,
                  native_libraries_rebuilt=False,native_snapshot_ABI_changed=False,
                  shared_caps=ledger['caps'],original_instructions_executed=0)
    manifest_path=E/'stage56-prepared-continuation-manifest.json'
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n')
    pending_command=('/usr/bin/python3 samples/run_bounded.py stage56-run04-city-frame . '
                     '/usr/bin/python3 samples/checkpoint_startup56.py --mode frame '
                     '--resume frame55a --tag frame56a')
    pending=dict(status='awaiting narrowly scoped original-execution continuation approval',
                 existing_ledger='evidence/stage55-batch-ledger.json',used_must_remain=3,
                 remaining_must_remain=97,maximum_processes_must_remain=100,
                 same_checkpoint='frame55a',no_original_execution_performed_in_stage56=True,
                 hard_process_seconds=60,original_segment_seconds=14,shared_caps=ledger['caps'],
                 same_watch_state=ledger['watch_state'],
                 prepared_manifest=str(manifest_path.relative_to(R)),
                 prepared_manifest_sha256=sha(manifest_path),
                 exact_first_command_after_approval=pending_command,
                 current_closed_gate_will_refuse=True,
                 required_changes_after_approval_only=[
                     'Record direct user approval for generalized frozen-MP3 contract and loop-count semantics, with explicit PCM byte-identity limitation.',
                     'Update approval/ledger source-manifest binding to the prepared manifest without removing old pins or altering used3/remaining97/caps/watch state.',
                     'Reopen only the unknown-music boundary closure in the same ledger; preserve original execution history and use index4 next.',
                     'Run the exact prepared wrapper from frame55a; stop at new unknown behavior, first validated city frame and finite scheduler goal or any shared cap.'
                 ],
                 recommendation='Conditionally reasonable for a silent bounded city-logic trial after review; do not claim exact PCM or MSS codec/mixer equivalence. If byte-exact audio is required, resolve comparator failure before original continuation.')
    (E/'stage56-pending-original-continuation.json').write_text(json.dumps(pending,indent=2)+'\n')
    semantics=dict(source='supplied fixed original DLL bytes, read-only Apple llvm-objdump; no DLL execution',
                   DLL_sha256='f09e6024ad35eb6b7b46982efb998df26d162391b26fcf56519a4c23fcbc7015',
                   exported_setter='0x2110ae40 calls0x211229a0',
                   storage='0x211229c3 stores count in stream+136 for streaming path; preloaded mode delegates sample setter',
                   EOF='0x211223d3 reads stream+136; count1 at211223de takes EOF state160=1/return, without rewinding',
                   repeats='0x21122457 tests count; nonzero decrements at21122461, zero skips decrement; then seeks to source start',
                   interpretation='1 total play once; 0 indefinite; positiveN total plays includes first, not N extra repeats',
                   disassembly_logs=['stage56-MSS-loop-setter-complete-static.stdout.log','stage56-MSS-full-static.stdout.log'],
                   public_legacy_Miles_manual_obtained=False,
                   official_current_entry_checked='https://www.radgametools.com/miles.htm',
                   documentation_status='Current official page did not supply legacy function documentation; no evaluation SDK requested/downloaded and no unofficial replacement implementation used.',
                   qualification='Static meaning of this fixed ordinary stream EOF path; original timing, decoder and callbacks not dynamically compared.')
    (E/'stage56-loop-semantics.json').write_text(json.dumps(semantics,indent=2)+'\n')
    outcome=dict(created_utc=audit['created_utc'],loop_semantics=semantics,
                 implementation_sources=additions,native_backend_reused_without_modification=True,
                 frozen_MP3_inventory_count=140,general_song_filename_support=True,
                 contract=dict(namespace='frozen GameData/Audio only; baseline SHA; regular file; no symlink/traversal',
                               format='single-track stereo44100Hz MP3',duration='estimated0..240sec exclusive; actual native decoded frame cap retained',
                               buffer_bytes=256000,loop_count='nonnegative signed32:0 indefinite; positiveN total plays',
                               volume='0..127',original_callsite_driver_and_guest_handle_checks_retained=True,
                               existing_quiescent_close_delegated=True,snapshot_bytes=257120,snapshot_version=1),
                 total_cases=len(cases),passed_cases=len(cases)-len(failures),failed_cases=failures,
                 offline_behavior_and_restore_tests_passed=True,all_tests_passed=False,
                 actual_city_decoded_frames=3214656,actual_city_duration_seconds=3214656/22050,
                 decoded_pcm_fnv64=result['city_open']['decoded_pcm_fnv64'],
                 afconvert_pcm_fnv64=failures[0]['decoded_fnv64'],
                 PCM_byte_identity_proven=False,PCM_difference_cause_proven=False,
                 same_frame_count=True,snapshot_sha256=result['snapshot_sha256'],
                 cross_process_future_PCM_cursor_EOS_equal=True,own_guest32_adapter_passed=True,
                 full_original_checkpoint_restore_with_new_wrapper_tested=False,
                 original_music_path_executed_with_new_adapter=False,original_instructions_executed=0,
                 original_DLL_executed=False,emulator_imported=False,device_opened_or_started=False,
                 source_PCM_sent_to_device=False,source_written=False,files_uploaded=False,
                 system_settings_changed=False,dependencies_installed=False,committed_or_published=False,
                 used_original_processes_unchanged=3,remaining_closed_unused=97,
                 original_ledger_unchanged=True,original_ledger_sha256=before,
                 whole_process_results=timings,protected_hash_checks=audit['hash_checks'],
                 protected_inputs_unchanged=True,
                 failures_retained=[
                     'stage56-city-reference-afconvert.stderr.log: sandboxfmt?',
                     'stage56-own-music-create.stderr.log: sandboxfmt? nativeOSStatus1718449215',
                     'stage56-own-music-create-host.stderr.log: byte-identity assertion failed after successful decode',
                     'stage56-music-create.json: explicit comparator failure; remaining functional tests completed',
                     'stage56-city-reference-afconvert-no-dither.result.json: no-dither reference still differed; cause unproven'
                 ],pending_continuation='evidence/stage56-pending-original-continuation.json')
    (E/'stage56-result.json').write_text(json.dumps(outcome,indent=2)+'\n')
    report=f'''固定版本的循环参数1表示总共播放一次，0表示无限重复，正整数N包含首次播放。这不是根据名称猜测：原合法MSS32 DLL SHA f09e6024ad35eb6b7b46982efb998df26d162391b26fcf56519a4c23fcbc7015，只读反汇编显示export2110ae40调用211229a0，streaming setter211229c3存count到stream+136；EOF211223d3读取该值，count1走结束state160=1及返回而不rewind；重复路径21122457对非零值递减、零跳过递减后回源头。该版本普通stream路径的静态规则明确，preloaded分支转交sample setter；没有执行原DLL。当前官方Miles入口未找到旧版API细则，未下载SDK或使用其他游戏/第三方替代实现；固定二进制静态证据与自写测试分别标明，不宣称原解码器动态对照。

已实现可复核的通用冻结音频合同：samples/stage56_music.py从已有冻结SHA清单索引140个MP3，允许冻结GameData/Audio内符合single-track/stereo44100Hz/MP3/估计时长0<d<240秒的常规文件，大小写按DOS式逐组件解析，拒绝absolute/drive/..、NUL、symlink、缺失/歧义、未冻结文件和hash改变。名字不含菜单或BG01专门条件，不因换曲另建白名单；140是库存数，不代表逐一解码通过。256000缓冲来自原wrapper静态常量，native实际decoded-frame240秒上限保留。loop接受非负signed32，volume0..127为现有自写后端支持范围；未实现其他MSS格式/调用/回调。对PCM和format超限仍真实失败，不伪造成功。

复用stage44既有native arm64 ExtAudioFile/PCM环形队列与stage50 close/quiescence/generation代码，旧源码/库未改变、未重编native、版本1/257120字节快照未变。stage56_stream_adapter.py保留原caller PC、driver、buffer、guest32句柄及stdcall合同，实际open/loop/volume/start/status进入此通用实现，其他API与原close继续转交既有实现。checkpoint_startup56.py是待批准包装器，旧stage54的claim和全局Watch上限仍生效；目前原账本closed，在实际构造runtime前拒绝，未运行该原入口。

离线测试结果：共{len(cases)}项，{len(cases)-1}通过、1项明确失败。真实BG01 MP3解码{result['city_open']['decoded_frames']}帧、22050Hz立体声signed16，loop1恰好消耗3214656帧后EOS2、不再额外消耗，loop2恰好6429312帧后EOS，loop0越过一遍仍playing4。音量20、实际非零PCM混合、队列/游标/句柄释放和stale-handle拒绝通过；越界路径、非法循环和快照外部路径拒绝且活状态不变。单进程恢复loop2/0队列字节相等，跨进程从已消费12345帧的城市快照恢复后，剩余混合PCM FNV/游标/EOS与未中断control完全相等。新快照在旧NativeStreams50类还原字节相等。自造guest32寄存器/调用栈fixture实际open→loop1→volume20→start→status→旧close通过，未知caller拒绝；未导入Unicorn、未运行EXE/DLL。

必须保留的失败：独立Apple afconvert得到同样3214656帧，但完整PCM FNV {failures[0]['decoded_fnv64']}与既有backend {result['city_open']['decoded_pcm_fnv64']}不同。显式--dither0仍与默认afconvert同值，所以不能声称差异由dither造成，原因尚未定位。首次宿主自测在严格字节比较断言停止，失败原日志保留；调整测试把该比较记录为显式失败后，继续完成既定的其他行为测试，create进程仍return1且all_tests_passed=false，不把失败隐藏成通过。没有声称与原Miles MP3/重采样/混音逐字节等价。

沙箱中的afconvert与native SetClientFormat曾均报fmt?（1718449215）；同命令经工具批准在宿主成功，这支持执行环境影响的判断，未改变任何系统设置或下载组件。测试进程始终只NativeAudio.new，不open/start AudioUnit，phase0/callbacks0/render_frames0/source_frames0，因此这些是确定性离线服务测试，不是实时设备回调、原MSS时钟或完整原城市行为测试。完成create/independent-restore/guest32-adapter整进程分别{timings[0]['duration_seconds']}/{timings[1]['duration_seconds']}/{timings[2]['duration_seconds']}秒，均无超时且小于60。其他初期失败日志也保留。

stage55原批次仍已用3/100、剩97且closed、latest frame55a，SHA {before}不变；旧stage51剩1未混用。原instructions新增0，sourcePCM设备0，无原DLL/上传/安装/宿主设置/提交/发布。{audit['hash_checks']}项保护哈希全一致，旧代码、旧native库、固定EXE/MSS、冻结数据/存档/档案、新旧checkpoint、缺失项及DOS链接未变化。

能否沿97恢复：可以准备一个明确接受当前PCM字节差异、只验证静音有限城市逻辑的续行方案；若目标要求音频逐字节一致，则应先定位该失败。当前未重开或消费97。待批准方案绑定stage56-prepared-continuation-manifest.json（旧pin全保留并追加5个新源码），在同stage55账本保留历史used3/remaining97/max100和原watch状态，批准后仅重开未知音乐闭合点及更新approval/manifest binding，下一项必须index4，不新建100次预算。原硬限60秒、内层14秒、共享51有效推进/1尾/128入口全保持，当前全0。首条具体命令：

```sh
{pending_command}
```

该命令本轮未执行。新版完整frame55a内存/CPU/四线程/window等冷恢复及原音乐入口尚未测试；独立stream恢复和自造guest32 fixture不替代原路径证据。恢复后仍须真实refresh返回和本地城市内容核查，任何新未知行为或任一共享cap立即停，phase5→0不算完整周期。

交付evidence/stage56-result.json、stage56-loop-semantics.json、stage56-music-create/restore/adapter.json、stage56-postflight.json、stage56-prepared-continuation-manifest.json、stage56-pending-original-continuation.json；源码stage56_music.py/stage56_stream_adapter.py/stage56_music_probe.py/checkpoint_startup56.py。本轮按父线程约20分钟收敛要求结束，未追加其他微样例或原执行。
'''
    (R/'reports/stage56-generic-offline-city-music-and-pending-continuation.md').write_text(report)
    print(json.dumps(dict(loop1='one total play',loop0='indefinite',cases=len(cases),
                          passed=len(cases)-1,failed=1,behavior_and_restore_passed=True,
                          PCM_byte_equal=False,original_instructions=0,used=3,remaining_closed=97,
                          ledger_unchanged=True,protected_hash_checks=audit['hash_checks'])))


if __name__=='__main__':main()
