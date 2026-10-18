# DATA FIRST status

2026-10-18 · macOS arm64 · original data supplied explicitly by the user.

| Measured scope | Result / current boundary |
|---|---|
| [SG3/555](docs/SG3.md) | **71,751/72,370** native images;58/58 indexes.427 empty＋192 missing companion images. Animation metadata72,370/72,370;playback semantics unmeasured. |
| [MAP / PAK](docs/MAP.md) | **168/168** MAP containers,167/167 city grids/images;32/32 planes.31/31 PAKs,108 parts,46 embedded city grids;62 other parts uninterpreted. |
| [SAV](docs/SAV.md) | **6/6＋9/9** supplied files across separate roots;90/90 numerical fields,4,678,560/4,678,560 values checked. Buildings/walkers/current memory unparsed. |
| **Stored surface rendering coverage** | **139/167** complete stored grids;**2,106,879/2,107,808** nonzero cells actually decoded (99.9559%),including offscreen payload validation.929 remain:286 subTile conflicts＋643 non-block overlay cells. No decode failure or unresolved index in this corpus. |
| **Reconstructed terrain→image accuracy** | **1,299,617/1,514,539** emitted codes match storage (85.809%);214,922 mismatches. Separate from rendering stored IDs. |
| **Original-screen city comparison** | **0/167** complete city pictures verified. Coverage is not pixel agreement or playability;camera/elevation,composition,objects,units andruntime state remain unproved. |
| [Native App surface](README.md) | **0/1→1/1** App entry;167/167 sources/CGImages,6/6 GUI transitions,344/344 tested inputs unchanged. Manifest81/81,27 compiled units＋strict signature. PAK/SAV remain numerical. |
| [Composite / decode safety](docs/MAP.md#composite-safety-and-payload-validation) | Local repairs remove **2/2** reproduced P1/P2 blockers:dimension conflicts stop before indexing;decode failures and successes are exclusive.292/292 authored assertions,965/965 real-data assertions each Release/UBSan;libc++ hardening292/292. Reviewer reproducer0 findings;72/72 authored pixel cases match,not an original-screen oracle. |
| [Persisted-field controls](docs/SAV.md#map-sav-field-correspondence-check) | **5/5** MAP–SAV pairs,20 common arrays unchanged;SAV image counterpart0/1. Complete delta5/5 transitions,22,155,407/22,155,407 bytes,412 changes accounted. Six opaque windows remain unnamed. |
| [Original observation limits](docs/MAP.md#same-sav-reload-repeatability) | **2/2** qualified reloads:pre world2,274,016/2,274,016 exact;aft13,288/2,274,016 changed. Runtime cause unknown. Earlier controls and failed material/source hypotheses remain in format docs. |
| [Bink](docs/BINK.md) | **1/1** native intro;5,101/5,101 video＋7,499,520/7,499,520 PCM frames. Physical speaker/display latency unmeasured. |
| [Filesystem output safety](docs/MAP.md#filesystem-equivalent-output-reservation) | Exclusive-reservation P2 independently closed;case/Unicode aliases refused. Separate from the newly repaired composite/decode defects above. |

Next: classify the929 residual stored-surface cells using data-backed geometry;retain unsupported object/overlay placement as unknown. Score the reconstructed selector separately,and obtain independent original-screen comparisons before claiming visual accuracy. CitySimulation/EMP1 remain frozen.

Counts distinguish original-data evidence andsynthetic regressions. Real-data tests use process-local `EMPEROR_GAMEDATA_ROOT` andsuccessful unset/empty SKIP. Original assets,derivatives,captures,hash dumps andbinaries remain outside Git. This continuation only tests the native App;it does not launch or operate the original. Test App instances exit normally.
