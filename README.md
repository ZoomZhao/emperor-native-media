# Emperor native data on macOS

Load legally owned **original Emperor: Rise of the Middle Kingdom data** through `--data-root`, render it natively on macOS, then port logic. Current phase: **DATA FIRST**. CitySimulation, EMP1 and authored gameplay are retained and frozen; they are not evidence of original-game behavior. **Complete same-city MAP→SG3 scenes remain 0/167.**

## Launch the verified local App

On this machine, use this single launch command:

```sh
"/Users/zoomzhao/Documents/Codex/2026-10-14/task/sav-app-delivery-20261015/release-final/EmperorApp.app/Contents/MacOS/EmperorApp" --data-root "/Users/zoomzhao/Workspace/lzjq/GameData"
```

The App path above is the final local delivery built from `c807114`, outside Git and GameData. Assets are not bundled. The data root is explicit; another legally owned directory requires changing that argument. The App starts muted; the bottom mute/volume controls also govern Bink audio. macOS15+ and the existing external FFmpeg are required for Bink playback; missing software is reported, never installed automatically.

For a source rebuild, use Apple command-line tools and a **fresh absolute output directory**:

```sh
EMPEROR_BUILD_DIR=/absolute/new-build-directory /bin/sh game_lab/app/build.sh
```

This produces `EmperorApp.app` in that directory. It does not start the original game or copy its assets.

## What the native entries show

| Format / App entry | Measured original-data scope | Remaining boundary |
|---|---|---|
| [SG3/555](docs/SG3.md) · 图素与贴图 | 58/58 indexes;71,751/72,370 logical images decode and create CGImages | 427 empty records;192 nonempty records lack `ChinaMapProj.555`. Static images do not prove animation or scene selection. |
| [MAP](docs/MAP.md) · 原始地图 | 168/168 containers;167/167 city terrain/raw-layer sets and native selections | Numerical diagnostic colors; empire/world parsing and original scene composition remain unproved. |
| [PAK](docs/MAP.md#native-pak-source-selection) · 原始地图 source menu | 31/31 packages,108 parts,46/46 embedded city grids/images | Package summaries clear old images;62 non-city parts and mission semantics remain uninterpreted. |
| [SAV](docs/SAV.md#native-sav13-snapshot-loading) · 原始地图 source menu | 6/6 supplied-root＋9/9 app-root files;90/90 numerical field images/views | These15 filesystem paths are not15 distinct cities. Buildings, walkers, rules and current memory are unparsed. |
| [Bink](docs/BINK.md) · 影片音画预览 | 1/1 intro;5,101/5,101 video frames and7,499,520/7,499,520 stereo PCM frames | Native audio-clock synchronization, pause/resume/drain verified. Hearing quality and physical speaker/display latency remain unmeasured. |

The experimental raw-reference viewer uses caller-supplied hypotheses and is not the original scene renderer. Its frozen viewport agrees at491,284/2,176,064 eligible pixels. Regional flat-grid registration covers16/16 cells in4 cancelled footprints; this does not establish elevation, full draw anchors or effective sprite selection. Detailed evidence and historical attempts stay in the format documents, not this entry page.

An [original campaign load control](docs/MAP.md#original-campaign-load-source-positive-control) now records **3/3 A→B→A loads** and **2/2 frozen whole-minimap comparisons**. The extra SAV check initially accepted 2/3 saves; [unique structural-candidate validation](docs/SAV.md#unique-validated-terrain-anchor) now accepts **3/3** without MAP content selecting the anchor. The App at the single launch path above includes this repair: new B passes 6/6 actual 170×170 field views, and the two tested roots pass 19/19 SAV sources and 114/114 field views (including trial derivatives). Its source manifest and strict signature are verified; the old delivery remains preserved separately. This supports loading the selected terrain in that trial; the persisted image/layer consumer remains unproved.

## Existing checks

The repository has individual Make targets and the existing App aggregate verifier; there is no single all-formats test target. Focused authored regression checks include:

```sh
make sg3-test sg3-test-undefined map-test map-test-undefined pak-native-test pak-native-test-undefined save-native-test save-native-test-undefined
python3 -I -B scripts/independent_runner.py app --output /absolute/local/check-results
```

Read-only corpus aggregates use only process-local `EMPEROR_GAMEDATA_ROOT`; unset/empty is a successful SKIP. Use the existing `data-metrics`, `map-metrics`, `pak-native-metrics` and `save-native-metrics` targets. Bink PCM, legacy-video and native-window checks are documented in [BINK.md](docs/BINK.md). Synthetic passes are regression evidence, not original-data progress.

## What blocks the same-city original picture

- **Effective ground-image/layer consumer:** neither the MAP byte1535/base48921 hypothesis nor the SAV following-u32 span has an accepted runtime binding. The retained one-byte PAK change produced no target change in its tested load path. The smallest useful new evidence is a published format contract or licensed modern reader that identifies a persisted graphics-reference/layer field in the **same city snapshot**, sufficient to freeze predictions for two distinct cells and an untouched region. Until such evidence exists, stop the same-cell ID/base/alpha permutations and repeated negative trials.
- **Composition, anchors and saved scene state:** an identified source must also have an evidenced layer placement/draw-order contract; SAV needs documented building/walker records before it can restore those objects. Use an existing, provenance-matched save/reference pair plus the newly identified contract for an offline held-out comparison. The current flat registration does not supply missing layer, elevation or full projection semantics.
- **Resource and animation gaps:** the192 missing images need the actual `ChinaMapProj.555` from a legally owned installation, checked against its index; no substitute pixels. The five named SG3 animation fields need frame-order/count/direction/reversal/speed timing semantics before native animation can be claimed. Mere static decoding or changing water content does not close that contract.

The [MAP–SAV field comparison](docs/SAV.md#map-sav-field-correspondence-check) checks5/5 existing snapshot pairs: all20 common named arrays remain unchanged, and no SAV counterpart for the candidate MAP image span is identified. This narrows the persisted-field explanation without raising scene coverage. A later independently accepted [paused-save control](docs/SAV.md#paused-save-reload-and-single-cell-control) verifies4/4 static save brackets, one fixed-screen-cell road action matching its saved terrain coordinate1/1, and24/24 native raw arrays. These controls do not identify a graphics field or raise full-scene coverage. No new original-game experiment is scheduled. Recovery evidence remains local; the original was restored and exited normally, with all7 recorded task processes and descendants gone. Close owned applications normally after each completed step; preserve unsaved content. Future observations require a distinct, frozen question that can separate the remaining gaps.

Follow [AGENTS.md](AGENTS.md): clean room; no original decompiled/disassembled code, original fragments or retired static execution route. Keep all original data read-only and all assets, derivatives, screenshots, dumps and binaries outside Git. Every change must raise a measured [STATUS.md](STATUS.md) result or remove its named blocker. Our source is [MIT](LICENSE); parser credits and external dependency licenses remain in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
