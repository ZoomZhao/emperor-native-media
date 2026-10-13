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
| Videos | **1 BIKi/Bink1**; independent FFmpeg decoded **5,101/5,101** (640×480,30fps); project AVFoundation open/frame **0/1** (-11828). Actual playback **unmeasured**, plays Y not established |

SG3 denominator: header **in-use logical records**, excludes dummy ID0, includes mirrored IDs. **655,000 capacity slots are not the denominator.** Decoded mirrored IDs **18,345**; decoded records with alpha field **9,059**; external records **290**. Real corpus check took **4.65s** here; this is an observation, not a timing guarantee. Synthetic SG3 assertions passed Release/UBSan; actual native windows displayed raw, RLE+alpha, mirrored+alpha and Emperor isometric images, cleared failures and quit normally. Original renderer visual agreement remains unmeasured.

Next: resolve the missing companion for 192 pixel-bearing records; then map-format parsing and a Bink native playback consumer. The prior 418 unexplained rejects are all exact zero dimensions/zero payload. Nine more empty records were previously hidden behind a missing-file classification. There are **71,943 nonempty logical records**, with **71,751 decoded**; primary header denominator remains **72,370**, so decoded-image increment is **0**, not an inflated success count. Freeze CitySimulation/EMP1. Independent video baseline is for the byte-identical local intro file; decoder success is not playback or game progress.

Repository cleanup: only 53 clearly named round reports proposed for scope review; **no deletion executed**. The earlier 1,042-path request is withdrawn; source/tests/general docs remain.
