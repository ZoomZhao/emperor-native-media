# DATA FIRST status

2026-10-13 · macOS 26.5.2 arm64 · measured local GameData corpus.

| Metric | Current result |
|---|---|
| Original under existing bundled Wine | **Named-window runs 3/3**, normal exit0 and PIDs gone. Menu pixels verified in **2/3 observation runs**; city and same-city match **0/3**. Latest 20min-capped run exited early after ESC; single-player mouse transition unverified; city playability/projection unmeasured. Unsigned Wine+native ddraw override |
| SG3 indexes / native browser | **58/58** enumerated and opened in real GUI; first images **57/58**, one missing dependency. Legacy selection cap of 6 removed |
| SG3 images decoded / CGImage created | **71,751/72,370 (99.14%)**, baseline 0/72,370 |
| SG3 by uint16 type | 0:1,800/2,124; 1:1,434/1,435; 10:526/535; 12:408/420; 13:32/33; 30:4,114/4,114; 256:58,047/58,159; 257:2/2; 276:5,388/5,548 |
| SG3 classified remainder | **427 empty logical records**, **192** nonempty records missing `ChinaMapProj.555`; invalid record/payload **0/0**, unsupported type **0** |
| MAP containers / roles | **168/168** completely consumed and verified; city grids **167/167**, empire excluded **1/1**, role errors **0** |
| City MAP terrain / native display | **167/167** grids and CGImages; **167/167** native window selections, five active-side cases and **32/32** bit planes. Complete world parsing **unmeasured/168** |
| MAP activity crop / unknown flags | **167/167** valid crops, crop CGImages and native crop selections, baseline 0/167; preserves **2,107,836** non-off-map cells, 0 outside. **6** unannotated bits in **77,341** cells; verified SG3 tile joins **0/167** |
| Videos | **BIKi/Bink1 silent video playable 1/1**; 640×480,30fps,5,101 frames. Independent full window 5,101/5,101; integrated decoder 5,101/5,101 and window smoke 59 decoded/58 visible draws. Audio/A-V sync unimplemented |

SG3 denominator is header **in-use logical records**, excluding dummy ID0 and including mirrored IDs; 655,000 capacity slots are excluded. **71,943 nonempty** records, 18,345 decoded mirrored IDs and 9,059 decoded records with alpha. Classification did not increase the decoded-image metric. Release/UBSan and actual native examples passed; original-renderer agreement remains unmeasured. [SG3 scope](docs/SG3.md).

MAP baseline **168/168** was first-block acceptance as a hardcoded 128×128 preview. New Release/UBSan agree on complete containers and terrain; 131 authored-fixture assertions and eight role/completeness/crop gate cases pass per mode; unset and empty roots never count as real passes. Backing grid is **228×228**; active-side distribution is 84:1,112:16,140:67,170:72,226:11. Native checks wait for filename/plane/crop image identity; five crop/raw toggles and 32 UTF-8 labels pass, empire rejection clears the old image and final normal quit passes. Centered rectangular crops preserve 56 boundary cells in 17 maps that disagree with the ideal diamond. Numerical colours and partial public annotations do not establish original appearance; projection and SG3 tile mapping remain unknown. Same-corpus case-insensitive recursive search found **0 `ChinaMapProj.555` candidates**; no substitute was used. [MAP scope](docs/MAP.md).

Full silent playback is independent-branch evidence (170.007s,5,101 draws); integration smoke is 1.916s, not another full run. External FFmpeg8.0.1 is not bundled; the 8MiB frame limit excludes process memory/copies. Final c51dfb4 lifecycle review passed Release/UBSan9/0 per mode and did not replay the full movie or GUI. [Bink scope](docs/BINK.md).

Next: remaining terrain bits and SG3 tile mapping, original projection, empire-map layout, the missing SG3 companion and original audio/A-V synchronization. Freeze CitySimulation/EMP1. Latest approved run: 09:12:50–09:15:14 UTC (20min cap not reached), after prior 180s runs; screencapture of the named window yielded the main menu (Sierra2003, version1.0.1.0), while its untitled peer was black. Previous city entry was refused after the 180s deadline; latest single-player input did not establish a transition before ESC ended the program. Two existing saves retained size/mtime, none created or removed. Installed/data-root same-name city MAPs are byte-identical **167/167**, but no city was selected: tile-mapping coverage and native comparable city region remain **unmeasured**, not diagnostic-grid passes.

Own-window input diagnosis only: **0/3** PID-click variants triggered the AppKit button despite6/6 events received with windowNumber0; matching guarded session input **1/1** and AX control **1/1**. Retina point/pixel mapping and focus checked; fixture normal exit0. Original startup count is unchanged; Wine mouse repair remains **unverified**. [Input evidence and next observation](docs/MAP.md).
