# DATA FIRST status

2026-10-15 · macOS arm64 · original data supplied explicitly by the user.

| Measured scope | Result / current boundary |
|---|---|
| [SG3/555](docs/SG3.md) | **71,751/72,370** decoded/native images;58/58 indexes.427 empty records＋192 nonempty missing `ChinaMapProj.555` (absent2/2 roots). Animation metadata72,370/72,370; playback semantics unmeasured. |
| [MAP](docs/MAP.md) | **168/168** containers;**167/167** city terrain/raw-layer sets/native images.32/32 bit planes; empire/world parsing unmeasured. |
| [PAK](docs/MAP.md#native-pak-source-selection) | **31/31** packages,108/108 parts;**46/46** embedded city grids/raw layers/native images.62 non-city parts uninterpreted. |
| [SAV](docs/SAV.md#native-sav13-snapshot-loading) | **6/6＋9/9** files across separate roots;**90/90** numerical grids/images/window views,4,678,560 values independently compared. Buildings/walkers/current memory unparsed. |
| [MAP↔SAV persisted fields](docs/SAV.md#map-sav-field-correspondence-check) | **0/5→5/5** source pairs;**20/20** named common arrays unchanged,1,039,680/1,039,680 values. Fixed-neighbour repeats1,404/1,404 unchanged; SAV image counterpart **0/1 identified**. |
| [Bink](docs/BINK.md) | **1/1** native audio/video intro:5,101/5,101 video＋7,499,520/7,499,520 PCM frames.5,072/5,101 draws (29 drops under concurrent compilation); max software clock difference57.14ms. Native drain1/1; sound quality/physical latency unmeasured. |
| Regional registration / experimental viewport | **16/16** cells in4 cancelled footprints under a flat model; broader anchors/elevation unknown. Hypothetical raw-reference view491,284/2,176,064 eligible pixels equal; full agreement fails. |
| **Complete same-city MAP→SG3 scenes** | **0/167**. Effective source/namespace, layer composition and draw order remain unproved; no default selector. A retained-byte trial's predicted visual change0/1 does not identify its consumer. |
| [Original campaign load control](docs/MAP.md#original-campaign-load-source-positive-control) | **0/3→3/3** A→B→A loads;**2/2** frozen whole-minimap comparisons. [SAV structural validation](docs/SAV.md#unique-validated-terrain-anchor) **2/3→3/3**, 18/18 numerical CGImages; original 15 inputs unchanged. Scene joins remain 0/167. |
| Blocked-read cancellation regression | Review failure5/5 → local fix **5/5** per mode; partial PCM/BGRA lifecycle **18/18**, process boundaries **6/6** each. Independent confirmation pending. |
| Final App delivery | New B **1/1 source,6/6 GUI fields** (previous window views unmeasured); SAV windows **19/19**,114/114 field views including trial derivatives. Same-binary entries **5/5**; shared checks13/13; source hashes73/73＋strict signature. |

SG3 includes mirrors/427 empty IDs, excluding dummy0/capacity; empty records are not successes. MAP/PAK counts use the supplied original corpus; App-root trial files add no original coverage. Synthetic passes are regression evidence.

Next evidence: identify a documented persisted image/layer consumer for a matched city snapshot, then freeze distinct-cell/held-out predictions; obtain the missing companion and animation contract. No repeated same-cell enumeration or new original-game experiment is authorized here. CitySimulation/EMP1 stay frozen. Original data read-only; tests use process-local `EMPEROR_GAMEDATA_ROOT`, successful SKIP when unset/empty. Assets and derivatives stay outside Git. Latest cleanup: task App/game/media instances0, children0; no restart.
