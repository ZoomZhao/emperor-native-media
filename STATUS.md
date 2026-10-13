# DATA FIRST status

2026-10-13 · macOS 26.5.2 arm64 · measured local GameData corpus.

| Metric | Current result |
|---|---|
| Original under Wine/GPTK | **Unmeasured**; launch/playability unknown |
| SG3 indexes | **58/58** enumerated; 213:33, 214:25 |
| Native SG3 browser | **58/58** indexes opened in real GUI; first images **57/58**, one missing dependency. Legacy selection cap of 6 removed |
| SG3 images decoded / CGImage created | **71,751/72,370 (99.14%)**, baseline 0/72,370 |
| SG3 by uint16 type | 0:1,800/2,124; 1:1,434/1,435; 10:526/535; 12:408/420; 13:32/33; 30:4,114/4,114; 256:58,047/58,159; 257:2/2; 276:5,388/5,548 |
| SG3 classified remainder | **427 empty logical records**, not decoded; **192** nonempty records missing `ChinaMapProj.555`; invalid record/payload **0/0**, unsupported type **0** |
| Maps parsed | Complete original layout **unmeasured/168**; old 128×128 preview payload accepted **168/168**, not complete world parsing |
| Videos | **BIKi/Bink1 silent video playable 1/1**; 640×480,30fps,5,101 frames. Independent full window playback 5,101/5,101; integrated decoder 5,101/5,101 and window smoke 59 decoded/58 visible draws. Audio/A-V sync unimplemented |

SG3 denominator: header **in-use logical records**, excludes dummy ID0, includes mirrored IDs. **655,000 capacity slots are not the denominator.** Decoded mirrored IDs **18,345**; decoded records with alpha field **9,059**; external records **290**. Real corpus check took **4.65s** here; this is an observation, not a timing guarantee. Synthetic SG3 assertions passed Release/UBSan; actual native windows displayed raw, RLE+alpha, mirrored+alpha and Emperor isometric images, cleared failures and quit normally. Original renderer visual agreement remains unmeasured.

Next: resolve the missing companion for 192 pixel-bearing records; then map-format parsing and original audio/A-V synchronization. The prior 418 unexplained rejects are all exact zero dimensions/zero payload. Nine more empty records were previously hidden behind a missing-file classification. There are **71,943 nonempty logical records**, with **71,751 decoded**; primary header denominator remains **72,370**, so decoded-image increment is **0**, not an inflated success count. Freeze CitySimulation/EMP1. Full silent playback is independent-branch evidence (170.007s, 5,101 draws); integration smoke is 1.916s and normal close, not another full run. External FFmpeg 8.0.1 is not bundled; single-frame cap 8 MiB excludes process memory/copies. Original game and original-renderer fidelity are unmeasured.

Repository cleanup: **53 stale round reports removed** in normal recoverable commit `3fc878e`; the overbroad 1,042-path request stays withdrawn. Source/tests/general docs, licenses and frozen CitySimulation/EMP1 remain.
