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

The [screen-cell protocol](docs/MAP.md#screen-registration-and-whole-road-predictions) now separates exact screen registration, visible-obstruction observations and frozen whole-source predictions. The archived mouse point is rejected as a centre1/1; its complete hover footprint registers1/1 without a texture identity. Background source matching is not an acquisition prerequisite; full prediction failures are retained. A calculated centre alone cannot confirm a final click: exact-point preview, same frozen cell and fresh uninterrupted context are required; missing evidence refuses. The [sender boundary](docs/MAP.md#sending-boundary-and-unavailable-live-producer) now has an in-process dry-run connection; the live final-road CLI refuses because a trusted continuous producer is absent. Its final native snapshot is read again after the second validation; a prior snapshot is never relabelled current. Its250ms limit is a protocol assumption, not measured live feasibility. This is offline preparation, with0 new original operations.

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

- **Effective ground-image/layer consumer:** neither the MAP byte1535/base48921 hypothesis nor the SAV following-u32 span has an accepted runtime binding. The retained one-byte PAK change produced no target change in its tested load path. The smallest useful new evidence is a published format contract or licensed modern reader that identifies a persisted graphics-reference/layer field in the **same city snapshot**, sufficient to freeze predictions for two distinct cells and an untouched region. Until such evidence exists, stop the same-cell ID/base/alpha permutations and repeated negative trials. This interpretation gap does not require background-source matching before a separately authorized, frozen whole-appearance test.
- **Composition, anchors and saved scene state:** an identified source must also have an evidenced layer placement/draw-order contract; SAV needs documented building/walker records before it can restore those objects. Use an existing, provenance-matched save/reference pair plus the newly identified contract for an offline held-out comparison. The current flat registration does not supply missing layer, elevation or full projection semantics.
- **Resource and animation gaps:** the192 missing images need the actual `ChinaMapProj.555` from a legally owned installation, checked against its index; no substitute pixels. The five named SG3 animation fields need frame-order/count/direction/reversal/speed timing semantics before native animation can be claimed. Mere static decoding or changing water content does not close that contract.

The [MAP–SAV field comparison](docs/SAV.md#map-sav-field-correspondence-check) checks5/5 existing snapshot pairs: all20 common named arrays remain unchanged, and no SAV counterpart for the candidate MAP image span is identified. This narrows the persisted-field explanation without raising scene coverage. A later independently accepted [paused-save control](docs/SAV.md#paused-save-reload-and-single-cell-control) verifies4/4 static save brackets, one fixed-screen-cell road action matching its saved terrain coordinate1/1, and24/24 native raw arrays. These controls do not identify a graphics field or raise full-scene coverage. That historical run restored and exited its owned instance, with its7 recorded task processes and descendants gone. The current user-supplied manual sample is analyzed offline; the user’s running original application stays under user control. Future observations require a distinct, frozen question that can separate the remaining gaps.

The historical paired road captures support [one finite-domain content localization](docs/MAP.md#offline-single-road-content-localization): all1,014 changed logical base pixels match one of1,206 declared terrain records at the frozen placement. That historical complete road-layer comparison remains0/1 because586 unchanged base positions disagree with that opaque source. The explicit offline diagnostic reports both counts; it supplies no MAP/SAV selector or inferred transparency. Its replay now re-decodes and compares every complete native canvas/raw base and reads both resource roots on each run; corrupted owned copies fail before success counters are emitted. A [fixed foreground colour necessary check](docs/MAP.md#fixed-foreground-colour-necessary-condition) finds opaque candidate colours for586/586 disagreement positions, while preserving all candidates and their conflicts; it supplies no consistent source assignment or draw order.

The [manual grass-scene evidence](docs/MAP.md#colorsync-complete-fixed-source-comparison) retains the earlier raw-channel794 failure as a representation-confounded comparison,not a global exclusion. Ten old controls support RGB555<<3 at82,556/82,556 raw8 pixels. A fixed public ColorSync Display P3→sRGB hypothesis now converts the entire declared source,with no colour patch or position search. New794 agrees4,532/4,544 changed-base pixels,0/1,856 unchanged pixels,4,588/6,456 opaque pixels and9,100/12,480 whole-overlay pixels. The prior20 errors disappear,but12 other one-step errors remain. All1,856 unchanged-painted pixels contradict the simple opaque overlay,and1,512 RGB5 changes lie outside its paint;if794 is used,an independently evidenced coverage/source/order relation is needed. Actual capture behaviour and absolute SAV→screen binding remain unproved. The [source/colour contract and controls](docs/SG3.md#colorsync-rgb8-implementation-and-complete-fixed-source-score) support this comparison hypothesis,not an engine graphics rule. Prior bounded searches belong to their earlier representations;complete scenes stay0/167.

The [separated neighbour discovery](docs/MAP.md#separated-neighbour-identification-and-complete-top-coverage) identifies2/3 neighbouring sources exactly within the frozen585-record domain;third-cell246 remains an8/624-error diagnostic. Complete natural tops240/242 recover1,464/1,856 unchanged-painted pixels under a fixed hypothesis,raising road-canvas agreement9,100→10,564/12,480 while retaining280 whole-top conflicts. Complete prediction remains0/1. Even the loose union of all1,755 front-top placements cannot reach876/1,512 nonpainted changes or496/496 outside-canvas changes;it is not a realizable composition. The read-only manifest replay and8 focused regressions preserve these bounds. Actual drawing sources/removal and selection conditions remain unknown;scene coverage stays0/167.

The [four conditional replacement models](docs/MAP.md#four-conditional-old-top-replacement-predictions) retain centre241/325 and propose rear244/246/240 as **unconfirmed** sources with48 retained identification errors. Complete natural sources cover2,116/2,116 old241-top pixels;no post-state background fills a hole. The241 branch predicts1,508/1,512 nonpainted changes and496/496 outside-canvas changes. With two front tops its separated validation has396/24,496 errors;an explicit additional unconfirmed front246 branch has24/24,496. Complete ROI errors are408/32,000 and36/32,000,respectively;both325 controls and every old score remain. All four predictions still fail,source identities/order remain unproved,and scenes stay0/167. The read-only replay retains full component conflicts and passes14 focused regressions.

The [pixel-provenance audit and pure native delivery](docs/MAP.md#pixel-provenance-and-pure-sg3-native-local-delivery) distinguish these results explicitly:the earlier best32k edit prediction generated10,816 pixels from SG3 and retained21,184 pre-state screenshot pixels. A new **pure SG3** diagnostic output uses0 screenshot pixels:23,968/32,000 ROI pixels are asset-generated,with23,436 matching,532 conflicts and8,032 transparent unknown under the fixed comparison contract. All86,400 native RGBA pixels and producer labels match an independent original-file composition. Seven explicit hypothetical layers render through the existing native reference viewer/CGImage path;the private scene is not an automatic MAP/SAV selector. Both coverage and conflicts remain visible;complete scenes stay0/167.

To view an explicit local diagnostic scene through the existing native entry:

```sh
python3 -B scripts/map_reference.py scene --data-root /absolute/owned/data --scene-manifest /absolute/private/scene.json
```

The [scene schema and offscreen command](docs/MAP.md#native-diagnostic-scene-entry-and-checks) keep inputs metadata-only and output unknown regions transparent. The current measured delivery is offscreen CGImage/PNG;original SG3/555 files and private images/manifests stay outside the App bundle and Git.

The [bounded peripheral expansion](docs/MAP.md#peripheral-base-coverage-with-pre-state-source-hypotheses) reuses that native renderer with private diagnostic manifests. Public-base geometry assigns the8,032 unknown pixels to nine positions (three existing top-only cells plus six peripheral positions). Pre-state-only evidence supports some additions;all3,510 candidate rows and the left241/325 ambiguity remain. The primary unconfirmed variants raise **pure SG3 coverage23,968→31,824/32,000**,with372/556 conflicts and176 transparent unknown pixels. Separate243 exploratory variants reach31,856/32,000,with268/452 conflicts and144 unknown;243 failed the original prefilter and is not a default source. No screenshot fills a hole,no native code changes,and all seven model outputs/producers match604,800/604,800 independent samples. The two unsupported cells remain unknown;this is no automatic MAP/SAV selector or complete scene.

Follow [AGENTS.md](AGENTS.md): clean room; no original decompiled/disassembled code, original fragments or retired static execution route. Keep all original data read-only and all assets, derivatives, screenshots, dumps and binaries outside Git. Every change must raise a measured [STATUS.md](STATUS.md) result or remove its named blocker. Our source is [MIT](LICENSE); parser credits and external dependency licenses remain in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
