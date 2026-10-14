# Emperor native data on macOS

Load and render **original Emperor: Rise of the Middle Kingdom data** natively on macOS, then port game logic. Users supply their legally owned files through `--data-root`; new native data code contains no original assets or original game code.

Original Bink silent video is connected through an existing external FFmpeg installation; [Bink scope and tests](docs/BINK.md) distinguish full playback evidence from smoke and exclude audio/synchronization claims.

The **原始地图** tab opens original city MAP terrain flags as a read-only grid with a verified centered active-box crop, an independent numerical palette and 32 annotated bit-plane views. Its **等距空间对照** view adds a configurable active-cell camera and optional cell outlines; fractional camera pans preserve the half-pixel raster phase across zero, and all 167 city maps create native spatial diagnostic images. The full 228×228 backing grid remains selectable; unknown bits are explicit. All 167 city maps now also retain raw object/edge grids and the public mapper variables `random` and `fertile`; those names do not establish game meanings or SG3 selection. Three frozen repeat-byte predictions on visibly clear grass all failed full-content equality; the new cells’ underlying content IDs remain unknown. One Anyang native window was checked against the frozen farther-road footprint: 4/4 cell centres agree at 0px error under the measured flat-plane model. Ground-image comparisons distinguish the 78×40 base from a taller record canvas; 10 reused full-ground positives pass the comparison gate. A declared-base pool now contains 2,983 decoded records, but all 16 fixed footprint cells remain unmatched. A newly frozen grass control without cardinal roads now has one full unique content match; the road-adjacent contrast remains unresolved. The sprite browser accepts an exact manual image ID and a nearest-pixel base-centred preview; MAP selection stays unknown. Its first window sample is displayed, with a strict screen-colour mismatch still under investigation. This numerical display does not restore city objects, original artwork, elevation or SG3 selection. [MAP format and scope](docs/MAP.md) distinguishes city maps from the unsupported empire map.

The separate [experimental MAP reference viewer](docs/MAP.md#experimental-raw-map-reference-viewport) now reads an explicit raw byte offset/width and caller-supplied SG3 archive/ID base to draw a complete native viewport. Anyang displayed 390/390 visible candidate cells using 62 decoded records. It has no per-cell image overrides or screenshot textures. The observed base 48921 is an explicit experimental parameter, with no default or original-engine claim; the frozen viewport comparison agrees at 491,284/2,176,064 eligible pixels and fails complete agreement. Full-city joins remain 0/167.

The [frozen spatial error audit](docs/MAP.md#spatial-error-classes-and-fixed-neighbour-rle-check) preserves that score and finds most disagreement in base interiors with no RLE support. The known positive plus three fixed neighbours retain full content equality; no anchor/order repair is supported. The candidate denominator contains no unpainted background or nominal unsupported geometry. Current-city state versus the static MAP and effective base selection remain unresolved.

The [read-only SAV source audit](docs/SAV.md) now verifies existing save containers and independently locates terrain through SAV-specific public markers. Two automatic saves updated during the current run uniquely match Anyang among 167 city MAPs, with all51,984 terrain values unchanged. Their one following u32 candidate span is zero at all nine fixed comparison points and has no proved graphics meaning. They precede the frozen screenshot; current effective base reconstruction remains unverified. This tool does not restore saves in the native app.

The [finite failed-base content check](docs/SG3.md#冻结失败格的有限-base-内容识别) preserves one complete RLE-free failed footprint and its same-view positive. Reusing the 2,983-record pool and extending only declared multibase subpieces plus exact-alpha natural78×40 RLE bases finds0/32,585 exact variants at the target; the positive remains ID361. This does not establish absence from all resources or exclude missing transparent layers. The cache verifier has23 authored regressions and clean unset-root SKIP. BGRA cache reads reject negative or overflowing offsets, truncated records and incomplete pixels; both frozen real-cache protocols retain identical complete results after this P2 fix.

The [fixed raw-alpha layer check](docs/SG3.md#固定底图的单层原始-alpha-对照) also preserves0/172 exact target matches with the fixed MAP-predicted base and source alpha. Its geometric dust-bitmap candidates have no proved original ground binding. The four relevant SG3/555 files match the current Applications installation byte for byte; the remaining gap is actual image/layer consumption, not a demonstrated mismatch in these inputs.

The [frozen grass temporal qualification](docs/MAP.md#frozen-grass-temporal-qualification) establishes10/10 unchanged complete footprints in two paused original frames. A normal viewer close and one original activation still leave the foreground keyboard guard failing, so running ground stability remains unmeasured and the original stays paused.

Current phase: **DATA FIRST**. Every change must raise a metric in [STATUS.md](STATUS.md) or remove a listed blocker. CitySimulation/EMP1 and the authored gameplay are frozen and retained; their tests do not measure original-game progress.

Requires macOS 15+ and Apple command-line tools already installed.

```sh
make game-app
./game_lab/build/EmperorApp.app/Contents/MacOS/EmperorApp --data-root /absolute/path/to/GameData
make sg3-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make data-metrics
make map-test
make map-test-undefined
make map-reference-test
make map-reference-test-undefined
make save-state-audit-test
make base-content-audit-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make save-state-audit
make map-sg3-audit-test
make map-raw-spans-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make map-raw-spans
# Optional offline comparison primitives; uses existing numpy/OpenCV:
make reference-compare-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make map-sg3-audit
make input-test
make input-test-undefined
# Explicit own-window UI test; requires existing AX/input/capture permissions:
make input-window-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make map-metrics
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData python3 game_lab/run_bink_tests.py
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData python3 game_lab/run_bink_tests.py --window --smoke
```

Real-data tests read only `EMPEROR_GAMEDATA_ROOT` and skip cleanly when unset. No implicit repository asset lookup or global environment changes. Native App selection retains `--data-root` precedence. [SG3 format and scope](docs/SG3.md) defines the image denominator and remaining gaps. For out-of-tree MAP checks, use `python3 scripts/map_data.py test --build-dir /absolute/temporary/path` or `metrics --mode undefined --build-dir /absolute/temporary/path`.

Run the experimental viewer with every diagnostic parameter explicit (this measured Anyang example is not an accepted selector):

```sh
python3 scripts/map_reference.py view --data-root /absolute/path/to/GameData \
  --map Cities/Anyang.map --raw-offset 1535 --raw-width 4 \
  --sg3 DATA/China_Terrain.sg3 --id-base 48921 \
  --camera-x 69.5 --camera-y 41.5 --width 1588 --height 1432 --scale 2
```

For read-only real-data metrics, replace `view --data-root /absolute/path/to/GameData` with `metrics` and set `EMPEROR_GAMEDATA_ROOT` for that process. `--build-dir` retains an out-of-tree app; optional `--output` requires a fresh absolute PNG path outside GameData. Local images must stay outside Git.

Offline Anyang comparison now has a measured minimap fit and 78 verified water-sprite placements from 40 visual IDs. Relative main-view steps are measured; the current conditional landmark replay and remaining anchor/shoreline limits are recorded in the linked MAP scope. [Registration and visual-match scope](docs/MAP.md#offline-anyang-registration-and-sg3-visual-samples) distinguishes logical color entries from repeated 2x pixels, raw/quantized errors and unproved MAP rules. A separately approved original pause/resume observation now shows changing content at78/78 fixed water positions with unchanged tree controls; a cancelled four-cell housing preview now verifies the measured ground-lattice phase, while absolute MAP cell anchors remain unproved. `scripts/reference_compare.py` has no file lookup or UI; its 46 authored tests (five new spatial-classification checks beyond the prior 41) preserve those boundaries, ambiguous candidates, fixed hypotheses and static-key counterexamples without returning an engine selector.

Three separately captured and cancelled Anyang housing footprints now support one canonical road-grid coordinate registration over12 tested cells (4 training,8 held out;this partition is not8 independently blind cells). The claim assumes the fixed measured basis, a flat translation model and tint-as-road/non-road. The fixed road orientation trial narrows2→1; the broader1,144→10→2→1 enumeration is retrospective. [Coordinate scope](docs/MAP.md#canonical-anyang-road-grid-correspondence) keeps the tested regional correspondence separate from internal camera naming, full-scene projection and the still-missing SG3 selector.

A separately predicted52-step farther road turn now agrees at4/4 cells with0px footprint-box residual, without refitting. The two trials total **four footprints and16 cells**, not16 independent observations. [Farther-turn evidence](docs/MAP.md#frozen-farther-road-turn-verification) preserves failed tooltip registration/recapture attempts and keeps SG3 image selection unknown.

The native SG3 parser also retains the five publicly named animation fields, verified against all72,370 logical records; [animation scope](docs/SG3.md#水面时间关系与动画元数据边界) keeps frame order, offset consumption, reversible endpoints and speed timing unmeasured.

The read-only MAP→SG3 metadata audit verifies the published object-grid layout and SG3 lookup bounds, and rejects an unmodified direct-ID candidate with corpus counts. It reads no .555 pixels and produces no image join. [Mapping evidence and stop condition](docs/MAP.md#mapsg3-association-audit) distinguish field facts, candidate tests and unknown semantics.

The finite [unexplained MAP-span audit](docs/MAP.md#unexplained-map-spans-finite-layout-audit) checks eight u8 candidate reshapes and three bounded u16/u32 alternatives, with no original-layer meanings assigned. All 167 city files cover the spans and remain unchanged across two reads; 22 direct ID/index-slot hypotheses fail the old ten matched points, leaving the clear point reserved in that check (later consumed by the namespace hypothesis). The prefix-u32 standard-part check rejects all 24 fixed relations. A later uniquely observed offset of 48921 predicts the reserved clear point’s content 1/1, but four additional fixed full-image predictions fail; its base origin and current-renderer use remain unverified. Fixed-ID layer checks preserve0/4:grass bases disagree throughout, the road base agrees1366/1600 and its RLE remainder75/77; all four within-block2× sample phases give identical counts. A fixed same-raw two-cell check also finds2524/6400 actual pixels differ, excluding raw-value-alone complete-pattern selection while other inputs/layers stay unknown. Optional `scripts/map_raw_spans.py --sample-manifest /absolute/local/manifest.json` accepts caller-supplied discovery/heldout content observations; it emits aggregates and never reads image payloads.

The reusable [scoped input driver](input_lab/scoped_click.hpp) and own-AppKit regression cover the earlier mouse-routing regression without GameData or original-game startup. Tests build in a temporary directory by default; `--build-dir` keeps local evidence, `--window` opts into one authored window and its capture, and missing existing permissions produce SKIP without requesting access. [MAP input scope](docs/MAP.md) documents the single-click request, the limits of the earlier authored-window tests and the later 6/6 actual game transitions. Use `python3 input_lab/run_tests.py --request /absolute/local/request.json --inspect` for a read-only snapshot of AX/CG owners, bounds, layer/alpha and the public display-shield ID; it sends no input. Missing values remain null. Application windows at the numeric cursor level remain eligible; foreign front covers still block. A full-display system Dock row is excluded at the selected point only when an existing-permission single-window capture proves a transparent3×3 pixel patch, current AX hits the exact target, and fresh process/frame/layer evidence agrees. The250ms budget starts on a monotonic clock before the capture request and is rechecked immediately before sending; slow processing or an unknown timestamp retains refusal. All other Dock/foreign covers remain subject to the original checks; missing capture evidence retains the refusal. Inspection may acquire this in-memory Dock witness; it saves no pixels. The earlier Release/UBSan own-window tests establish platform behavior; the later normal Applications run below separately verifies6/6 actual game page changes. After independent review of the timing fix, one separately approved normal LaunchServices run of `/Applications/皇帝龙之崛起.app` verified6/6 menu/page transitions and entered Anyang1/1. The selected `Cities/Anyang.map` matches the installed copy1/1; the same file is displayed natively as140×140 terrain flags and a water-related bit2 plane. [Anyang reference and comparison boundary](docs/MAP.md#anyang-original-reference-and-native-diagnostic-comparison) distinguishes this same-MAP diagnostic from an SG3 city scene. The game ended normally within652.3s, with no further run pending.

Work from original file bytes, published format documents and licensed modern parsers. Never use decompiled/disassembled game code or the retired static execution route. Never commit original files, extracted images, atlases, dumps or captures. Data checks emit aggregate counts, not assets. Follow [AGENTS.md](AGENTS.md).

Our code is [MIT](LICENSE); adapted public SG3 algorithm credits and license are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). 53 clearly named stale round reports were removed with a normal recoverable Git commit. Source, tests, current format documents and repository metadata remain. Archived code is excluded from the new data pipeline; local user data and task evidence remain untouched.
