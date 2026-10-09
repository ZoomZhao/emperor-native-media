"""Summarize the one PCM diagnosis and audit frozen inputs; no original run."""
import array
import hashlib
import json
import math
import os
import struct
import subprocess
import wave
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EV = ROOT/'evidence'


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024*1024), b''):
            h.update(block)
    return h.hexdigest()


def pcm(path):
    with wave.open(str(path), 'rb') as f:
        data = f.readframes(f.getnframes())
    samples = array.array('h')
    samples.frombytes(data)
    return data, samples


def wave_fmt(path):
    raw = Path(path).read_bytes()
    assert raw[:4] == b'RIFF' and raw[8:12] == b'WAVE'
    cursor = 12
    while cursor+8 <= len(raw):
        kind, size = struct.unpack_from('<4sI', raw, cursor)
        if kind == b'fmt ':
            fields = struct.unpack_from('<HHIIHH', raw, cursor+8)
            return dict(zip(['format_code', 'channels', 'rate', 'bytes_per_second',
                             'block_align', 'bits_per_sample'], fields))
        cursor += 8+size+(size%2)
    raise ValueError('missing WAV format')


def main():
    capture = json.loads((EV/'stage57-capture.json').read_text())
    old = json.loads((EV/'stage57-compare-default.json').read_text())
    fixed = json.loads((EV/'stage57-compare-matched-SRC64.json').read_text())
    command = json.loads((EV/'stage57-reference-SRC64.result.json').read_text())
    assert fixed['byte_identity'] and fixed['different_samples'] == fixed['error_RMS_LSB'] == 0
    assert command['returncode'] == 0 and not command['timed_out']
    assert capture['converter_properties']['srcq']['uint32'] == 64
    assert capture['converter_properties']['srca']['fourcc'] == 'norm'
    native_raw, native = pcm(ROOT/'runs/stage57-backend-city.wav')
    old_raw, reference = pcm(ROOT/'runs/stage56-city-reference.wav')
    fixed_raw, _ = pcm(ROOT/'runs/stage57-city-reference-SRC64.wav')
    no_dither_raw, _ = pcm(ROOT/'runs/stage56-city-reference-no-dither.wav')
    assert native_raw == fixed_raw and old_raw == no_dither_raw
    neighborhoods = {}
    for label, sample in [('first', old['first_difference_sample']),
                          ('last', old['last_difference_sample'])]:
        frame = sample//2
        neighborhoods[label] = dict(sample=sample, frame=frame, channel=sample%2,
                                   seconds=frame/22050,
                                   frames=[dict(frame=i, native=list(native[2*i:2*i+2]),
                                                reference=list(reference[2*i:2*i+2]),
                                                delta=[native[2*i+c]-reference[2*i+c] for c in (0,1)])
                                           for i in range(max(0,frame-4), min(old['format']['frames'],frame+5))])
    base = json.loads((EV/'stage56-postflight.json').read_text())
    checks = []
    cache = {}
    for item in base['checks']:
        p = item['path']
        if p not in cache:
            cache[p] = digest(p)
        expected = item['expected_sha256']
        checks.append(dict(path=p, expected_sha256=expected, sha256=cache[p], unchanged=cache[p]==expected))
    absences = [dict(path=q['path'], still_missing=not Path(q['path']).exists()) for q in base['absences']]
    links = [dict(path=q['path'], target=os.readlink(q['path']), unchanged=os.readlink(q['path'])==q['target'])
             for q in base['DOS_namespace_symlinks']]
    source = json.loads((EV/'stage56-result.json').read_text())['implementation_sources']
    source_checks = [dict(path=p, expected_sha256=h, sha256=digest(p), unchanged=digest(p)==h) for p,h in source.items()]
    ledger_path = EV/'stage55-batch-ledger.json'
    ledger_sha = digest(ledger_path)
    assert ledger_sha == '5ed6414885425fa973e6d8de6165784f090639113ed1bc878d5fccb23457b931'
    ledger = json.loads(ledger_path.read_text())
    audit = dict(hash_checks=len(checks), unique_paths=len(cache), checks=checks,
                 absences=absences, DOS_namespace_symlinks=links, stage56_sources=source_checks,
                 all_unchanged=all(q['unchanged'] for q in checks+links+source_checks) and all(q['still_missing'] for q in absences))
    assert audit['all_unchanged']
    (EV/'stage57-postflight.json').write_text(json.dumps(audit, indent=2))
    help_result = subprocess.run(['/usr/bin/afconvert', '--help'], capture_output=True, timeout=5)
    (EV/'stage57-afconvert-help.txt').write_bytes(help_result.stdout+help_result.stderr)
    regions = old['one_second_distribution']
    result = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                  cause='Reference sample-rate-converter quality127 versus measured backend quality64; only -r64 changed and full PCM then matched.',
                  cause_class='confirmed controlled single-parameter offline comparison',
                  representation_difference=False, resampled_content_difference_with_old_contract=True,
                  corrected_reference_byte_identity=True,
                  native_library_or_decoder_changed=False,
                  format=old['format'], wave_headers={p:wave_fmt(ROOT/'runs'/p) for p in
                       ['stage57-backend-city.wav','stage56-city-reference.wav','stage57-city-reference-SRC64.wav']},
                  measured_converter=capture['converter_properties'],
                  backend_PCM_sha256=fixed['native_PCM_sha256'], matched_PCM_sha256=fixed['reference_PCM_sha256'],
                  old_reference_PCM_sha256=old['reference_PCM_sha256'],
                  exact_PCM_bytes_compared=len(native_raw),
                  old_comparison=dict(different_samples=old['different_samples'],total_samples=old['total_samples'],
                     fraction=old['different_sample_fraction'],max_abs_error_LSB=old['max_abs_error_LSB'],
                     error_RMS_LSB=old['error_RMS_LSB'], error_dbFS=20*math.log10(old['error_RMS_full_scale']),
                     abs_error_bins_upper_inclusive=old['abs_error_bins_upper_inclusive'],
                     channels=old['channels'],first_last_differences=neighborhoods,
                     one_second_regions=len(regions),regions_with_difference=sum(bool(r['different_samples']) for r in regions),
                     best_distributed_lag=old['best_tested_lag'],
                     delay_search_qualification='1024 distributed points and +/-2048 frames; not exhaustive proof for old contract',
                     leading_zero_frames=[old['native_edges']['leading_zero_frames'],old['reference_edges']['leading_zero_frames']],
                     trailing_zero_frames=[old['native_edges']['trailing_zero_frames'],old['reference_edges']['trailing_zero_frames']]),
                  corrected_comparison=dict(different_samples=0,max_abs_error_LSB=0,error_RMS_LSB=0,
                     byte_identity=fixed['byte_identity'],same_frame_count=True),
                  dither_zero_alone_did_not_resolve=True,
                  quality_only_change=command['command'],
                  historical_stage56_suite_unchanged='29/30; old mismatch retained, only failed comparator rerun with matched SRC contract passed',
                  full_stage56_suite_rerun=False, no_added_general_behavior_tests=True,
                  original_Miles_decoder_mixer_equivalence='unknown; neither original EXE nor DLL executed in this diagnosis',
                  audibility_assessed=False, original_timing_callback_state_equivalence='unknown',
                  next_action='Use explicit -r64 matched reference for this backend; retain old failure logs. Do not change backend to quality127 merely to satisfy the old unmatched reference. Original-fidelity audio still requires independent original evidence.',
                  original_instructions_executed=0, original_DLL_executed=False, device_opened_or_started=False,
                  source_PCM_sent_to_device=False, files_uploaded=False, dependencies_installed=False,
                  system_settings_changed=False, committed_or_published=False,
                  original_ledger_sha256=ledger_sha, original_ledger_unchanged=True,
                  protected_hash_checks=len(checks), all_protected_inputs_unchanged=True,
                  original_budget='used3/100 remaining97 closed; not reopened or consumed',
                  diagnostics_source_sha256=digest(ROOT/'samples/stage57_pcm_diagnose.py'))
    (EV/'stage57-result.json').write_text(json.dumps(result,indent=2))
    report = f'''唯一PCM失败已定位：旧参考afconvert默认重采样质量127，现有后端AudioConverter实际为64（norm）。保持输入、采样率、声道、位深及其余命令不变，只加`-r 64`，得到3,214,656帧、12,858,624字节与后端完整逐字节相同的PCM。双方SHA-256 `{fixed['native_PCM_sha256']}`。这是匹配参考转换合同后的复测通过，不是要求接受未知差异；未修改native后端、旧快照、Stage56代码或已有准备清单。

实际格式均为22,050Hz双声道交错signed16小端PCM，WAV fmt为PCM1、block_align4、88200bytes/s。后端client/converter ASBD flags0x0c（signed/packed，非float、非big-endian、非non-interleaved），channel map[0,1]、dither0、prime_method1、prime_info[0,0]。旧参考和显式dither0的PCM相同，不能归因dither。旧输出字节换序或声道互换均不能匹配；只对齐SRC质量即全文件相等，故不存在需要重排表示、另调归一化或平移/裁切才能消除的残余差异。首尾各16帧均为0，前251帧/后264帧静音两边相同；匹配质量后的全部首尾及填充也逐字节一致。

旧不匹配条件下，6,429,312个样本中4,862,064（75.6234%）不同，max1576 LSB（左779/右1576），合并RMS40.252627 LSB、{result['old_comparison']['error_dbFS']:.4f}dBFS；左右RMS34.101748/45.580904，误差均值0.000263/0.000638。0差1,567,248；绝对差1共886,548，2共541,780，3..4共715,925，5..8共800,662，9..16共691,549，17..32共495,996，33..64共381,137，65..128共226,000，129..256共91,623，257..1024共30,761，1025..1576共83。差异覆盖{result['old_comparison']['regions_with_difference']}/{len(regions)}个逐秒区间，属于重采样内容差异，超过单个量化单位舍入误差。首差frame299左、末差frame3214373左；具体首末邻域、逐秒分布、左右幅度和拟合gain见JSON。分布点±2048帧延迟搜索最优0，非全量延迟证明；匹配合同后全量零差直接覆盖延迟问题。

有界实际过程：后端捕获0.385秒；Apple参考-r64转换0.259秒；旧比较14.677秒；修正比较13.704秒，均return0且未超时。旧参考WAV、旧失败日志及Stage56历史29/30结果保留；没有重跑整套30项，也未新增泛化行为测试。本次仅失败比较独立复测通过，零差max/RMS/不同样本数均0。实际纠正参考命令：

```sh
/usr/bin/afconvert -f WAVE -d LEI16@22050 -c 2 -r 64 'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]/Audio/Music/BG01-Lonely_Er_Hu.mp3' runs/stage57-city-reference-SRC64.wav
```

可操作判断：该afconvert比较失败由测试参考条件不一致造成，已解除这项阻塞。未来此后端参考必须显式-r64；无需为了旧参考改backend为127，否则会改变绑定PCM及快照一致性。本阶段没有证明原Miles解码/重采样/混音的等价，也未听音或测试原时钟/回调，完整复刻的这些问题仍unknown；不要将Apple两路径相同扩大为原游戏音频通过。新版原音乐调用/完整原checkpoint恢复仍未执行，97次原批次保持closed；不在这次诊断中重开。

限制核查：原EXE指令0、原DLL执行0、设备open/start0、播放/上传/安装/系统设置/提交/发布0。原账本SHA仍`{ledger_sha}`，used3/100、remaining97、closed。2945项保护哈希、18项缺失、2项DOS链接及5个Stage56新源码哈希全不变。供父线程审查的主要记录：evidence/stage57-result.json、stage57-capture.json、stage57-compare-default.json、stage57-compare-matched-SRC64.json、stage57-postflight.json；每条有界命令的result/stdout/stderr及旧失败记录均保留。
'''
    (ROOT/'reports/stage57-PCM-reference-quality-mismatch-resolved.md').write_text(report)
    print(json.dumps(dict(cause=result['cause'], corrected_byte_identity=True,
                          hash_checks=len(checks), all_unchanged=True, original_instructions=0)))


if __name__ == '__main__':
    main()
