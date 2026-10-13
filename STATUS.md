# DATA FIRST status

2026-10-13 · macOS 26.5.2 arm64 · measured local GameData corpus.

| Metric | Current result |
|---|---|
| Original under Wine/GPTK | **Unmeasured**; launch/playability unknown |
| SG3 indexes | **58/58** enumerated; 213:33, 214:25 |
| SG3 images decoded / CGImage created | **71,751/72,370 (99.14%)**, baseline 0/72,370 |
| SG3 by uint16 type | 0:1,800/2,124; 1:1,434/1,435; 10:526/535; 12:408/420; 13:32/33; 30:4,114/4,114; 256:58,047/58,159; 257:2/2; 276:5,388/5,548 |
| SG3 unresolved | **201** missing `ChinaMapProj.555`; **418** current record/payload validation rejects; unsupported type **0** |
| Maps parsed | Complete original layout **unmeasured/168**; old 128×128 preview payload accepted **168/168**, not complete world parsing |
| Videos | **1 BIKi/Bink1**; independent FFmpeg decoded **5,101/5,101** (640×480,30fps); project AVFoundation open/frame **0/1** (-11828). Actual playback **unmeasured**, plays Y not established |

SG3 denominator: header **in-use logical records**, excludes dummy ID0, includes mirrored IDs. **655,000 capacity slots are not the denominator.** Decoded mirrored IDs **18,345**; decoded records with alpha field **9,059**; external records **290**. Real corpus check took **4.43s** here; this is an observation, not a timing guarantee. Synthetic SG3 assertions passed Release/UBSan; actual native windows displayed raw, RLE+alpha, mirrored+alpha and Emperor isometric images, cleared failures and quit normally. Original renderer visual agreement remains unmeasured.

Next: explain the 418 record/payload rejects from bytes/public specs and resolve the missing companion; then map-format parsing and a Bink native playback consumer. Freeze CitySimulation/EMP1. Independent video baseline is for the byte-identical local intro file; decoder success is not playback or game progress.

Repository cleanup: concrete tracked-path deletion list pending user approval after automatic review rejected bulk removal; new SG3 pipeline does not read or execute those archives.
