# Emperor native data on macOS

Load legally owned original Emperor data with `--data-root`, render it natively on macOS, then port logic. Current phase: **DATA FIRST**. Original-world checkpoints now have a separate versioned disk format, source/resource SHA-256 bindings and atomic paused restoration of the represented kernel state. In a newly built App, use **开发 → 严格原版城市时序 → 保存原版世界 checkpoint… / 打开原版世界 checkpoint…** and choose a new `.emworld` file outside the original data directory. Keep the same original relative source/model/resource contents available through `--data-root`; an explicit relocated root with identical contents is accepted. Existing files are never overwritten. Loading rejects nonregular files, including a FIFO with no writer, before reading. A post-publication durability error reports that the file may already exist and must be checked before retrying.

The checkpoint preserves current process/input knownness, complete slots and raw24/25 bases, routes/cell/FIFO, population/Goods/calendar roles, offers, message dispositions and write-time scene state. It does not recover unknown FSA or authorize unimplemented events, calendar services or market transport. `.encity`, native v6 defaults, CitySimulation/EMP1 and existing delivered Apps remain unchanged. New source/build validation is separate from actual GUI qualification and deployment. [Disk format and validation](docs/SAV.md#original-world-disk-checkpoint-v1-2026-10-30) · [measured scope](STATUS.md).


## Launch the local App

Versioned native deliveries contain `EmperorApp.app`, `Start.command`,
`Start-Metal.command`, `PLAY.txt` and a hash manifest. Keep that version folder
together; both launchers use its relative App path and can be moved with it.
No original data is included. Pass `--data-root /absolute/owned/data`, set
`EMPEROR_GAMEDATA_ROOT` for that process, or use the normal directory chooser.
An explicit missing directory is refused; restart with the correct directory
instead of silently switching resources. Existing-source/model mismatches also
refuse to replace a running city. `scripts/package_native_release.py` publishes
only to a new external version directory, validates the bundle/signature and
never overwrites a previous release.

From the home screen choose **新建原生城市…**, then an admitted empty source.
Both development checkboxes are off by default. **城市经营与预防巡检（开发版）**
explicitly selects the current v8 strategy; it does not migrate old saves.
The new city is paused and empty. Use **入城口** or its minimap marker to locate
the source road network, connect roads, and place houses. **邻路** means only
local adjacency; **入城可达** uses the runtime's entrance-distance calculation,
which may include ordinary walking ground. **接原路网** separately compares
actual road components. Neither field promises a dispatch or future arrival.
Restore play to let actual incoming people arrive, then staff wells
and preventive towers. Workers and dispatched walkers do not themselves prove
that a house has received water or a patrol. Save to a new `.encity`; Continue
checks the same original source/model and loads its saved policy paused.

This is a bounded native game mode. Continuous original market supply,
calendar/health/material consumption, higher house evolution, active
firefighting and inhabited original-city import remain incomplete. Preserve
older Apps and matching older saves for rollback; an older App may reject a
newer saved strategy. Temporary verification checkpoints are not player starts.

**Native playable subset v6 (labor and construction):** the city uses the measured1024×768 original-interface reference:798×720 map,40-pixel top border,8-pixel bottom border and226-pixel split sidebar. Eleven category positions form one column; the four supported native categories work and the other seven remain disabled. Existing pause/speed/service/save controls stay explicitly marked **原生操作** in the lower sidebar; original advisors remain unavailable. A live terrain overview occupies the measured111×111 radar content area at(853,542);its native colours are not an original-palette claim. Click to locate the main camera;the white viewport follows zoom/pan/resize. Terrain is cached per source and native construction marks update separately,so ticks do not rebuild the terrain bitmap. Detailed state and data-browser entries live in the **开发** menu; **游戏** and the in-game **文件** menu retain New/Continue/Save/Home/Quit. Original interface art is read from the external data root; all text uses the macOS system font. Clicking a category only opens it and cancels the prior tool; choose a building in the measured3×2 selector grid before placing it on the map. The grid uses54×53 slots at(856+54×column,318+54×row) in the1024×768 reference. Housing andwater use the first slot;the food icon uses the last slot,following locally recorded tutorial selections. Road/browse/demolition buttons remain explicitly native tools. Selection shows actual model price and service conditions,never charges,and Esc/right-click cancels construction while retaining the selected category. Category choices operate the same existing transactions. Browse-click inspects a native object; dragging pans. Speed1×/2×/4× changes only wall-clock pacing,not persisted rules. Religious/trade controls remain explicitly disabled. **原生规则 / 来源 / 兼容范围…** retains the full rule/source explanation.

**Current acceptance:** native v6 uses the existing saved labor-pool and allocation model; this is code-path agreement, not proof of original-game scheduling. The selected house shows both hypothetical strict water branches and separate native water coverage. Original house-water state and automatic upgrades remain unconnected. Frozen candidate-6 passes both supported empty sources on CPU and Metal: **4/4** GUI flows, **200/200** mode checks plus **10/10** comparison/protection checks, and **4/4** real Cmd+Q exits with code 0. Querying conditions preserves the city and serialized bytes. See [candidate-6 acceptance](docs/ENCITY.md#candidate-6-water-condition-gui-acceptance-2026-10-25).

**Start/continue:** the App opens its home screen. Choose **新建原生城市 · v6**, select one of the validated original empty SAV19 sources and confirm. No sample number or diagnostic tab is needed. **继续游戏 · 选择会话…** opens an `.encity` chooser, automatically checks its original source and exact rule version, then loads paused after confirmation. The original data directory still comes from `--data-root` or the normal directory chooser; it is read-only. Build roads,2×2 houses,wells and Hunter's Tents directly on the original scene. Model Normal construction prices and housing capacity are loaded (tested road2,house15,well25,hunter32,capacity7). The existing road-drag transaction, water coverage and failure-before-debit protections remain.

**V3 explicitly authored rule:** on every even native tick, each road-adjacent Hunter's Tent produces up to4 meat into a shared city stock of100. Excess production is stopped at capacity. Production happens first, then each existing resident consumes1; if any existing demand is unmet,new immigration stops. Otherwise each eligible house may gain1 resident in native-ID order,spending1 further meat. Eligible houses require an adjacent road and all4 tiles watered. Missing food/water stops growth without eviction. Native wells retain the v2 road-adjacency and4-cell expanded coverage rule. **狩猎生产** pauses/resumes all native meat production; it does not pause city consumption. The UI shows stock,lifetime production/consumption and the last meal demand/shortfall. There is no original hunting,agricultural timing,cart/market delivery or labor equivalence claim.

**V5 explicitly authored tax:** every4 native updates,each resident of a road-adjacent native house contributes1 gold,using the population at the beginning of that update. Tax assessment precedes the meal/new arrivals; both are one atomic candidate. No eligible residents means zero income,with empty/disconnected reasons shown. City pause has no tax or offline catch-up. A completed-cycle counter,receipt and lifetime tax income persist,so reload never itself pays. Source treasury plus cumulative tax minus all construction costs (including demolished objects) equals current funds. The balance cap is2,147,483,647; an overflowing assessment refuses the entire update and pauses. V1–v4 retain their original formats and never silently gain income. The model has Tax Office type125,without a supported image binding in this renderer; no guessed tax office or original tax/labor equivalence is introduced.

**V4 demolition:** pause the city,choose **拆除（预览）**,then click one of your native roads,houses,wells or Hunter's Tents. Red footprint highlighting and service/population consequences are a preview only; **确认拆除** is a separate action. Cancel/Esc/right-click or focus loss cancels the selection. Source roads/objects/terrain have no native ID and cannot be removed. There is **no refund**: historical construction cost remains spent; rebuilding pays again and receives a new identity. Removing a house moves its residents out. Removing roads/wells/producers recalculates service immediately; disconnected surviving residents remain,with new arrivals requiring restored service. Shared meat stock and lifetime food accounting are retained. All changes render before commit. IDs are never recycled; the current renderer permits3999 total created objects per session,including retired ones. V1/v2/v3 files retain their exact format and behavior,with demolition disabled; no automatic migration occurs.

**保存会话…** opens the standard save dialog for a new **.encity** file, defaulting to `NativeSessions` alongside the App. Existing files are never overwritten. Loading rejects nonregular files, including a FIFO with no writer, before reading. Use the home screen or in-city **继续游戏…** to choose a saved file; it locates and validates the original source automatically. **首页并暂停** retains the current city in memory; **返回当前城市** reopens it. Quitting or replacing a changed city asks to save, cancel or discard; cancelling a save/quit keeps the city paused. Pause/resume alone does not count as unsaved game progress, while updates and construction do. Failed saves never authorize exit. Source/version mismatches and failed drawing retain the current city. **V1/v2 checkpoints retain their exact rules:** v1 needs only roads; v2 additionally needs water; neither runs food accounting or permits Hunter construction. No automatic migration occurs. V3 persists its stock,production switch and conservation ledger. Source/model/version mismatches refuse before replacing the city. Original mission/SAV continuation, food varieties, markets, transport, live-original labor equivalence, original taxation, disasters, trade and automatic house evolution remain unavailable. MAP/nonempty SAV creation stays unsupported. See [native session format and rules](docs/ENCITY.md).

The latest build adds **目标诊断** for a selected SAV. It shows the saved goal containers with activation explicitly unknown. Enter a manually verified positive comparison threshold to inspect the bounded housing-population interval; unknown housing tiers stay unknown. For the observed70-resident tutorial snapshots, threshold150 yields[0,70] and definitely unmet, not exact original progress0. Invalid input and source changes clear the prior comparison. This is a read-only saved-snapshot diagnostic, not a completion handler or a playable-city claim.

The App also supports a limited native **年贡消息** transaction for positive cash notices produced by successfully committed updates in the current city session. Accept credits funds and the observed income category once; Reject completes the local notice without payment. Repeated or stale identities and overflowing ledger additions are rejected. Saved type29 history is always view-only and is never reconstructed as a payable queue. The panel keeps a completed selection until the user chooses the next pending notice. This native lifecycle does not reproduce unknown original event bytes or callback registrations. Normal event guards remain intact, so successful live transactions are currently verified only in the explicitly premised headless harness; normal GUI progression remains blocked. See [STATUS.md](STATUS.md) for separate GUI and mechanism results.

```sh
EMPEROR_BUILD_DIR="/absolute/path/to/a/new-build" ./game_lab/app/build.sh
"/absolute/path/to/a/new-build/EmperorApp.app/Contents/MacOS/EmperorApp" --data-root "/absolute/path/to/legally-owned-data"
```

The local frozen candidate-6 and earlier deliveries remain preserved. The command above builds a separate package; it does not replace an accepted local bundle. The New chooser shows actual SAV filenames, supported/unsupported status, saved map dimensions/funds and a local terrain preview. Unsupported entries explain their refusal. Catalog decoding and selection previews now run on one background serial queue, with per-file progress and cancellation. Cancelled dialog generations and superseded selections cannot publish late results; the currently running read may finish in the background. See [responsive source loading](docs/ENCITY.md#responsive-source-loading-2026-10-24). Confirm re-reads SAV/model fingerprints; a changed source retains the existing city. Data admission is distinct from per-source GUI acceptance; see [opening source list](docs/ENCITY.md#opening-source-list-2026-10-24). CPU remains the default; a process-local `EMPEROR_NATIVE_METAL=1` opts into the GPU path. A bounded frame-local companion-path memo removes repeated filesystem resolution for sprites sharing a dependency; each new frame still resolves and checks that dependency. Native rules, saves and resource-failure transactions are unchanged. The existing right sidebar now explains road/water/food/full/waiting housing states, well/hunter service failures, and current food/tax estimates through shared read-only planners. Estimates recalculate as the city changes; inspecting never advances it. See [operating feedback](docs/ENCITY.md#read-only-operating-feedback-2026-10-24). Medium/large native-session performance evidence and its input-to-state (not scanout/FPS) limits are recorded in [SG3.md](docs/SG3.md#larger-native-city-performance-and-frame-local-resolution-2026-10-24). This integrated data-browser delivery stays outside Git and GameData;the earlier strict,diagnostics andcandidate App bundles are preserved. No game assets are bundled. Another legally owned directory requires changing the explicit argument. macOS15+ is required. The App starts muted; Bink playback uses the existing external FFmpeg and reports missing dependencies without installing them.

Open **原始地图** and select a standalone city MAP or a **包内城市** entry from a PAK. The **存储地表图素（MAP / PAK 城市）** view reads its saved surface-image grid and renders SG3/555 pixels. Use **筛选来源** tofind a filename or包内城市 entry without scrolling the full catalog;an empty query restores all sources. A no-match query clears the previous image andcoordinates. Camera X/Y moves the viewport. The default **候选补全（原版放置未验证）** toggle adds complete residual storage groups using an explicit candidate anchor and foreground composition. Turn it off to inspect the unchanged strict path. The panel reports strict counts,candidate decoded/visible counts and their combined renderable total separately. Missing resources,invalid groups anddecode failures remain unknown;candidate pixels do not establish original positioning orocclusion. The scrollable **地表未覆盖格或SAV查格原值** list shows each located failure,its class and active X/Y;enter those coordinates in the camera fields to inspect the area. Unknown locations stay explicitly counted,and source/plane/invalid-camera changes clear old coordinates. The list retains strict-path unknowns even when candidate art is visible. Numerical terrain/bit views remain available. PAK city entries use their own complete source-part payload;package summaries remain summaries. SAV selections retain numerical views because their image-reference field is unidentified. Entering a renderable city from a summary orSAV selects its surface automatically;an explicit numerical choice is retained when switching between renderable cities.

The App adapter reads167/167 loose-city and46/46 embedded-city sources,creating213/213 native CGImages for these source entries (not213 distinct cities). Earlier surface App checks cover15/15 transitions:complete/partial surfaces,candidate off/on,camera-targeted candidate paint,numerical fallback,PAK clearing,SAV restrictions,embedded-PAK surface/numerical switching,source filtering/no-match clearing,invalid camera andreturn;344/344 tested input files remain unchanged. The source manifest covers119/119 files and29 compiled units, including the two previously omitted post-goal/religion parser headers; strict signing and source correspondence are checked. These are delivery checks, not original-screen accuracy.

SAV selections now support a coordinate inspector: enter zero-based active-square integer X/Y and query all six saved raw values together. It shows unsigned decimal/hex, source identity, backing coordinates and off-map status. Invalid coordinates clear old values; changing to MAP/PAK or no source disables SAV queries. This adds no image-field meaning. Native checks cover **6/6** supplied SAVs, **30/30** queries and **180/180** values; App controls pass **19/19** checks with **344/344** tested files unchanged.

SAV building tables now parse all **24,000/24,000** declared slots across the six supplied snapshots. **定位未绑定建筑占地** focuses the first eligible record in the existing numerical scene;each listed record also supplies coordinates for the existing camera fields. Amber outlines use the selected SAV's own terrain. Eligibility follows the separately stored semantic type andpublished ordinary square or explicit market-rectangle rule,not an inferred serialization class orSG3 artwork. Unknown lengths/subversions stop;malformed records clear the result. Special shapes remain separate:two entertainment-square occurrences are parsed butnot drawn;market rectangles are validated from their type-specific rotation.

The App lists **1,063/1,065** live,size-positive record occurrences across four nonempty snapshots:777 crop-field records and286 other structure records,covering26 observed semantic types. These are snapshot occurrences,not1,063 independent buildings/cities. All**1,751/1,751** footprint-cell checks and1,063/1,063 focused native images pass. Native/Python record-coordinate/size/slot/offset tuples agree1,063/1,063. GUI acceptance covers6/6 SAV selections,all1,063 listed records and53 source/type focus checks,raw-field/invalid-camera clearing,PAK-summary/MAP/no-match clearing andtab return;344/344 inputs remain unchanged. The source profile has146 fixed-tail types plus19 monument types with explicit subversions9/10;synthetic support for those165 types is separate from the26 observed types. Full bounded tables also pass13/13 selected MAP/SAV/PAK entries (52,000 slots),not13 distinct cities. **Image bindings remain0**. Numerical colours andflat outlines do not verify original artwork,draw anchors,elevation orocclusion. Prior App packages/captures are preserved;this increment does not operate the original game.

For an explicit image comparison,enter a listed **建筑槽** andpress **定位未绑定建筑占地**,then use the existing SG3 tab toselect a valid static physical image ID. Return andpress **使用 SG3 当前图素（人工候选）**. The image is copied intoone source-bound manual preview;the checkbox toggles it,and**清除** removes it. Browsing another image does not silently replace an existing candidate. The image canvas bottom-edge centre is placed at the saved footprint centre as an explicit preview convention. Every view says **人工候选 · 锚点未验 · confirmed0**;this does not identify the building's original artwork oranchor. There isno built-in type→image default. Invalid/missing targets andsource/plane changes clear prior candidates.

**保存本地候选 / 载入本地候选** use normal local JSON dialogs. Save outside GameData andGit checkouts (including aliases). The16 metadata keys contain the source/snapshot revision,record geometry,archive/physicalID,pixel revision andmanual anchor convention;no pixels orconfirmed flag are stored. Load requires the currently selected SAV andsame parsed record,andredecodes the original image before accepting it. A changed snapshot/image,missing resource,invalid configuration orattempted confirmed status clears the old preview. Revision tokens are non-security FNV-1a fingerprints,not a proof ofsemantic image binding. Explicit real-data technical cases pass3/3 rendered candidates/CGImages/configuration roundtrips;they intentionally do not claim that the test images depict their target buildings. Confirmed image bindings remain0.

Selecting a SAV19 city also enables **▶ 播放 / ⏸ 暂停 / ⏭ 单步** (⌘P / ⇧⌘P / ⌘→) in the free band above the image. The saved snapshot stays the default picture: Play samples the host clock and admits updates at the documented37 ms cadence (speed80 read from the user's `Emperor.inf`),单步 admits exactly one, and 暂停 keeps N and the drawn frame. Play continues until Pause, Step or a named refusal; it has no fixed 100-update target. Normal playback keeps only the current timeline state; replay journaling is opt-in for bounded tests. The status line shows state,N / continuous mode,drawn frames,cadence and premise tier,and a refusal shows the exact library name and the update that refused. Only documented inputs are supplied and there is no in-App experiment switch,so **0/6** sources pass update1: all6 stop with `event_ui_5eff20_not_closed` (event UI gates still unresolved) and keep the saved snapshot. The App never asks the user for a value:the CRT seed is the App-start Unix second and farmer time is monotonic milliseconds since launch. A headless test drives the same controller with the CLI's declared zero values to N=100 on6/6 and checks chained playback against the batch replay (pixels and whole-scene digest,24/24);that is a mechanics check,not an App end-to-end result.

The 2026-10-23 rebuild starts from `e195147` plus the reviewable fixes in the working tree. The controller now stays paused after a manual step even when a previous Play target remains unfinished; the old implementation failed this regression. The regression passes on **6/6** supplied saves using the existing explicitly premised headless harness; it does not raise normal-App simulation coverage. The SAV picture-phase and data-plane selectors now occupy separate columns. The next runtime increment binds the loaded scenario-message table, derives the scheduler network predicate from the actual single-player session, carries process clocks/RNG/caches across loads, and publishes consumed CRT state before the next frame. Event UI gates and event slot lifecycle still lack an admissible source; they remain unknown. See [runtime input closure](docs/SAV.md#runtime-input-closure-and-session-continuation-2026-10-23) for version-specific evidence and remaining fields.

The earlier runtime-package GUI acceptance loaded **6/6** SAV snapshots and confirmed the first-update refusal on **6/6** with normal App inputs. Continuous city progression, pause after progression, resume and successful single-step remain **blocked**, not GUI passes. Cmd+Q during the supplied original Bink playback exits normally, and both observed FFmpeg children disappear. Tests use a separate read-only data copy; no original assets are bundled. [Current acceptance and blocker details](docs/SAV.md#normal-app-acceptance-and-control-regression-2026-10-23) distinguish these results from prior packages.

Open **图素与贴图**, select an SG3 package and paste an explicit comma-separated physical-ID list into **明确帧ID列表**, then choose **应用帧列表**. Physical ID1 is vector index0; failed/empty frames reject the whole list. Use **上一帧 / 下一帧 / 播放** and set a presentation rate of1..30 FPS. The loop and rate are preview controls, not original timing or reverse-endpoint rules. **减去原始X/Y偏移对齐** uses a fixed common canvas and the displayed signed metadata as an explicit preview convention; disable it for raw top-left canvas alignment. It does not establish a city drawing anchor. Changing packages/static images clears the sequence; leaving the tab pauses it. Local evidence supports **6/6** manually inspected sequences (3 walker,3 ambient;no verified separate building base),**122/122** decoded frames/CGImages and**36/36** GUI checks. Sample ID lists/contact sheets remain outside Git at `<local-task-output>/sg-animation-20261019/verified-sequences.json`; no sample-specific IDs are embedded in the App. Original red shadow-marker pixels are retained; final shadow composition remains unverified.

## Integration boundary and next city work

This is a native original-data browser and asset preview, not a complete Emperor game. The single launch command above is the recommended current package. Earlier package integration acceptance covered all **167 loose MAP +46 embedded PAK city sources**, both strict and candidate modes, **6 supplied SAV snapshots** with numerical fields/cell lookup, SG3 asset sequences, and the existing muted audio/video entries. The earlier runtime package was checked for the six SAV loads and original-movie Cmd+Q path described above; the full 213-source GUI sweep is not repeated. Strict-complete counts remain **139/167 MAP and33/46 PAK**; candidate-complete counts remain **167/167 and46/46**. Cross-tab switching, cleared error results, stopped SG3 timers, movie pause/Stop/reopen and normal process/decoder-child release were checked on the earlier package. Earlier package evidence is retained separately; frame lists/pixels remain external.

A complete original city still needs verified camera/elevation and drawing anchors, underlay/foreground/shadow composition, source-bound buildings/units, a SAV image-field contract, original animation timing and game rules (movement/pathfinding, economy, services, ownership and saved-state restoration). Candidate asset coverage does not establish these. CitySimulation/EMP1 remain frozen and cannot fill the missing original rules.

Next core target: one original city slice with a **source-bound building base drawn over its actual MAP/PAK surface**. Start from one nonempty building record in the published city-table layout and seek an explicit type/name-to-SG3 archive/record binding; then validate its stored coordinate, footprint and draw anchor against a provenance-matched original reference, with independent nonadjacent/held-out checks. Required evidence is the actual record schema/bounds, an image binding stronger than ID range or visual resemblance, and a same-source original placement reference. Until image binding is available, the existing numerical scene shows only the explicitly unbound saved footprint described above. The subsequent separately authorized black-box sampling used an isolated original game and six new saves; its observations and limitations are recorded in SAV.md.

## Separate measurements

| Measurement | Current result | Boundary |
|---|---|---|
| Stored surface rendering coverage | **139/167** city maps complete;**2,106,879/2,107,808** nonzero cells decoded (99.9559%);929 cells remain | Reads persisted image IDs directly. This is static surface coverage, not a complete city or pixel-match rate. |
| Residual structural groups | **426/426** complete groups covering929/929 residual cells | Exact stored membership,offsets anddecoded source canvases;this does not set an original screen anchor. |
| Candidate preview | **167/167** combined scenes;**2,107,808/2,107,808** nonzero cells renderable,including929 candidate cells;167/167 native CGImages | Explicit unverified anchor andforeground layer. Reference coverage does not mean projected pixel area is filled;gaps remain. Strict139/167 coverage remains separate. |
| Embedded PAK city scenes | **46/46** source parts;629,786 nonzero image references =629,148 strict＋638 candidate;strict33/46 complete,with candidate46/46 complete | Same full MAP payload schema andsource-part identity. Other62 PAK parts remain uninterpreted;SAV image references remain unidentified. |
| Reconstructed terrain→image accuracy | **1,299,617/1,514,539** emitted predictions match stored codes (85.809%) | The reconstructed selector is scored separately;the App surface view does not use it as a substitute for stored IDs. |
| Original-screen comparison | **0/167** complete city pictures verified | Camera/elevation, composition, buildings, units and runtime state still need independent evidence. Coverage does not imply playability. |

[MAP evidence and App integration](docs/MAP.md#native-app-stored-surface-view) and the existing [stored-grid pipeline](docs/MAP_SG3_PIPELINE.md) retain the scope and unresolved cases. Earlier explicit-scene experiments and original observations remain in the format documents, not the launch instructions.

## Other native entries

- [SG3/555](docs/SG3.md):58/58 indexes;71,751/72,370 images.427 empty records and192 images missing `ChinaMapProj.555` remain. Static decoding does not establish animation playback.
- [MAP/PAK](docs/MAP.md):168/168 MAP containers and31/31 packages;46 embedded city scenes and62 uninterpreted parts. Numerical views remain available.
- [SAV](docs/SAV.md):6/6 supplied-root plus9/9 separate app-root snapshots;90/90 numerical fields. These15 paths are not15 distinct cities. Bounded building-table spans andordinary square footprint profiles are parsed; other buildings, walkers and current memory remain unresolved.
- [Bink](docs/BINK.md):native video/audio preview and synchronization;physical display/speaker latency remains unmeasured.

## Build and check

Use Apple command-line tools and a fresh absolute build directory:

```sh
EMPEROR_BUILD_DIR=/absolute/new-build-directory /bin/sh game_lab/app/build.sh
python3 -B scripts/app_map_surface.py test
python3 -B scripts/app_map_surface.py test --mode undefined
```

Real-data adapter checks read only the process-local root and successfully SKIP when unset or empty:

```sh
EMPEROR_GAMEDATA_ROOT=/absolute/owned/GameData python3 -B scripts/app_map_surface.py metrics
EMPEROR_GAMEDATA_ROOT=/absolute/owned/GameData python3 -B scripts/app_map_surface.py playback
```

`python3 -B scripts/map_reference.py test` checks the underlying renderer. `python3 -I -B scripts/independent_runner.py app --output /absolute/private/check-results` checks packaging without launching either game. Existing format tests and corpus commands are documented in their format files. Synthetic checks are regression evidence, not original-data coverage.

Follow [AGENTS.md](AGENTS.md):keep user data read-only and original assets, screenshots, dumps and binaries outside Git. Every change must improve a measured [STATUS.md](STATUS.md) result or remove a named blocker. Source is [MIT](LICENSE);parser and dependency credits remain in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

The current continuous-control package retains strict event guards. Its source audit found no implemented production owner for the two event predicates or eight-slot callbacks; there is no existing initializer to connect. Current acceptance is recorded in [continuous control and source audit](docs/SAV.md#continuous-control-and-event-source-audit). Earlier packages and their evidence remain available separately.

Native v1 map controls: hover previews road/2×2-house placement; green means the same native transaction validation allows it, red explains refusal. Click to build, including while paused. Right-click/Esc or Browse cancels. Option-drag (or Browse-drag) pans; wheel/± zooms around the pointer/centre; arrow keys pan while the map has focus. Window resizing preserves aspect ratio; dark margins ignore clicks. Coordinate fields remain an auxiliary route. Original rules and `.encity` version are unchanged.

Native v1 roads: press and drag from a start tile to an end tile; the preview follows X first, then Y. Release commits the entire path atomically. Existing roads are blue/free, new legal tiles green; red rejects the whole path. Preview shows new/existing counts and total cost. Esc/right-click, leaving the image, or losing window focus cancels the pending path. Returning after cancellation cannot commit until a new press. Houses still use a single click. ENCITY v1 and residency rules remain unchanged.

The explicitly selected **new v8** city now uses native incoming type11 groups
for first occupancy and capacity-limited later arrivals. The saved RUNTIME8 5
policy connects reservation, ordered arrival, cleanup, population/meal receipts
and exact resumption; the old native meal cycle does not also add residents.
Existing v8 checkpoints retain their saved policy, and v6 remains the default.
This is a bounded native adapter: type12 emigration, type13 empty-house arrival,
original population statistics/calendar, and inhabited original-SAV import are
still unsupported. See [population contract and tests](docs/ENCITY.md#explicit-native-immigration-b-2026-10-27).

Housing feedback follows each saved policy. In the incoming-group modes it
shows committed reservations, departure/road/entry actions, actual capacity
and the saved phase28 water cache. A missing meal-cycle arrival is not reported
as a food refusal, and no meal-based population ETA is shown. Food receipts
and saved job assignments remain separate facts. Queries do not run the
scheduler or change RNG, routes, inventory or save bytes; older modes keep their
meal-cycle explanations. Wells distinguish road access, workers, dispatch,
service and return instead of treating road access as proof of supply.


**Optional city-context C1:** the separate development App offers
**城市状态与阶段派工（开发版）**. This explicitly selects RUNTIME8 6 and preserves
city working state across reconstruction/save/load. Wells/Hunters use saved
phase25 assignments, and phase17 reads the normalized house food field; default
v6 and earlier checkpoint policies retain their behavior. New native cities
start with empty owned warehouse/trade pools, while continuing a C1 save
restores its saved inventory and policies. This does not complete the original
calendar or material-consumption schedule. A house whose proposed update would
consume materials waits with a specific reason; supported water-only transitions
and other houses continue. See [C1 state and scheduling contract](docs/ENCITY.md#explicit-city-context-c1-2026-10-27).

The development source also fixes the storage-transfer return contract: partial
physical acceptance subtracts only the amount actually stored from a carrier.
This is an inventory-conservation repair, not market activation or calendar
completion. The fixed C1 App remains unchanged; see the
[warehouse transfer contract](docs/ENCITY.md#warehouse-transfer-conservation-2026-10-27).


### Native preventive Inspector development mode (2026-10-28)

The explicit development city mode also has a separate native preventive
Inspector strategy (`native_inspection_prevention_v1`, `RUNTIME8 7`). A tower
must receive scheduled workers and actually dispatch/move its Inspector before
any service is recorded. Both accumulated risk counters, owner/route cleanup,
return/redispatch, service-grid decay and in-flight saves use the shared runtime.
Old defaults and saved modes stay unchanged. Road edits cancel current patrols
without clearing risk; tower downgrades retire only the auxiliary patrol.
These are named native adaptation policies. Fire targeting, active firefighting
and continued city play after a disaster replacement remain unsupported and
refuse the entire candidate. The new mode is not full original simulation.

### Bounded original-evidence collector (2026-10-27)

`scripts/original_evidence.py` builds a read-only SAV19 numeric exporter, tests
its framing/path/qualification contracts and preflights three explicit candidate
saves under the process-local `EMPEROR_GAMEDATA_ROOT`. It does not run the game.
City year words, clock, health, treasury, house goods12/quality, Mill8/Market6,
Goods1 policies and complete known Figure7 base fields are exported separately.
Risk words have an explicit `common5-risk-v1` contract: signed accumulator68
feeds the existing collapse branch and70 the ignition branch. Deprecated
`fire_risk`/`damage_risk` aliases retain their old values; they are not hazard
probabilities. Qualification crosschecks raw/canonical/alias values and both
readers, and never certifies preventive coverage from tower staffing alone.
Unknown classes or variable FSA payloads stop the actor reader; partial data can
never certify absence or an operating-city start. The three current candidates
export successfully, but **0/3** qualify as a stocked operating-city baseline.

```sh
python3 scripts/original_evidence.py --output-dir /tmp/evidence-no-assets
EMPEROR_GAMEDATA_ROOT=/absolute/isolated/Game-ddraw python3 scripts/original_evidence.py --mode undefined --output-dir /tmp/evidence-real
```

Both output directories must be new and outside the data root. With no root,
real-data checks report SKIP. Reports and raw numeric observations stay outside
Git. [Field scope, candidate limitations and bounded protocol](docs/SAV.md#bounded-evidence-export-and-candidate-preflight-2026-10-27).


The evidence collector additionally reserves exactly16 new numeric capture names
under `Save/2026102801/` (`280001.sav` through `280016.sav`). This application list
grants no system access or experimental authorization. `--check-capture-targets`
only checks absence; it creates nothing and refuses every existing target.
`--source` selects an exact permitted read path; default remains the three old
candidates. Protected candidates can never pass capture-target validation.
Route diagnostics preserve no-route roam counters and exhausted-route cleanup
counters instead of labeling them corrupt. Unknown FSA still stops before later
actors or Goods can be certified. [Updated field contract](docs/SAV.md#evidence-route-and-capture-path-contract-2026-10-28).
