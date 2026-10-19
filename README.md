# Emperor native data on macOS

Load legally owned original Emperor data with `--data-root`, render it natively on macOS, then port logic. Current phase: **DATA FIRST**. CitySimulation, EMP1 and authored gameplay remain frozen and separate from original behavior.

## Launch the local App

```sh
"/Users/zoomzhao/Documents/Codex/2026-10-14/task/building-image-candidate-20261019/delivery-final/EmperorApp.app/Contents/MacOS/EmperorApp" --data-root "/Users/zoomzhao/Workspace/lzjq/GameData"
```

This integrated data-browser delivery stays outside Git and GameData;the earlier strict,diagnostics andcandidate App bundles are preserved. No game assets are bundled. Another legally owned directory requires changing the explicit argument. macOS15+ is required. The App starts muted; Bink playback uses the existing external FFmpeg and reports missing dependencies without installing them.

Open **原始地图** and select a standalone city MAP or a **包内城市** entry from a PAK. The **存储地表图素（MAP / PAK 城市）** view reads its saved surface-image grid and renders SG3/555 pixels. Use **筛选来源** tofind a filename or包内城市 entry without scrolling the full catalog;an empty query restores all sources. A no-match query clears the previous image andcoordinates. Camera X/Y moves the viewport. The default **候选补全（原版放置未验证）** toggle adds complete residual storage groups using an explicit candidate anchor and foreground composition. Turn it off to inspect the unchanged strict path. The panel reports strict counts,candidate decoded/visible counts and their combined renderable total separately. Missing resources,invalid groups anddecode failures remain unknown;candidate pixels do not establish original positioning orocclusion. The scrollable **地表未覆盖格或SAV查格原值** list shows each located failure,its class and active X/Y;enter those coordinates in the camera fields to inspect the area. Unknown locations stay explicitly counted,and source/plane/invalid-camera changes clear old coordinates. The list retains strict-path unknowns even when candidate art is visible. Numerical terrain/bit views remain available. PAK city entries use their own complete source-part payload;package summaries remain summaries. SAV selections retain numerical views because their image-reference field is unidentified. Entering a renderable city from a summary orSAV selects its surface automatically;an explicit numerical choice is retained when switching between renderable cities.

The App adapter reads167/167 loose-city and46/46 embedded-city sources,creating213/213 native CGImages for these source entries (not213 distinct cities). Earlier surface App checks cover15/15 transitions:complete/partial surfaces,candidate off/on,camera-targeted candidate paint,numerical fallback,PAK clearing,SAV restrictions,embedded-PAK surface/numerical switching,source filtering/no-match clearing,invalid camera andreturn;344/344 tested input files remain unchanged. The source manifest covers87/87 files and28 compiled units;strict signing and source correspondence are checked. These are delivery checks, not original-screen accuracy.

SAV selections now support a coordinate inspector: enter zero-based active-square integer X/Y and query all six saved raw values together. It shows unsigned decimal/hex, source identity, backing coordinates and off-map status. Invalid coordinates clear old values; changing to MAP/PAK or no source disables SAV queries. This adds no image-field meaning. Native checks cover **6/6** supplied SAVs, **30/30** queries and **180/180** values; App controls pass **19/19** checks with **344/344** tested files unchanged.

SAV building tables now parse all **24,000/24,000** declared slots across the six supplied snapshots. **定位未绑定建筑占地** focuses the first eligible record in the existing numerical scene;each listed record also supplies coordinates for the existing camera fields. Amber outlines use the selected SAV's own terrain. Eligibility follows the separately stored semantic type andpublished ordinary square or explicit market-rectangle rule,not an inferred serialization class orSG3 artwork. Unknown lengths/subversions stop;malformed records clear the result. Special shapes remain separate:two entertainment-square occurrences are parsed butnot drawn;market rectangles are validated from their type-specific rotation.

The App lists **1,063/1,065** live,size-positive record occurrences across four nonempty snapshots:777 crop-field records and286 other structure records,covering26 observed semantic types. These are snapshot occurrences,not1,063 independent buildings/cities. All**1,751/1,751** footprint-cell checks and1,063/1,063 focused native images pass. Native/Python record-coordinate/size/slot/offset tuples agree1,063/1,063. GUI acceptance covers6/6 SAV selections,all1,063 listed records and53 source/type focus checks,raw-field/invalid-camera clearing,PAK-summary/MAP/no-match clearing andtab return;344/344 inputs remain unchanged. The source profile has146 fixed-tail types plus19 monument types with explicit subversions9/10;synthetic support for those165 types is separate from the26 observed types. Full bounded tables also pass13/13 selected MAP/SAV/PAK entries (52,000 slots),not13 distinct cities. **Image bindings remain0**. Numerical colours andflat outlines do not verify original artwork,draw anchors,elevation orocclusion. Prior App packages/captures are preserved;this increment does not operate the original game.

For an explicit image comparison,enter a listed **建筑槽** andpress **定位未绑定建筑占地**,then use the existing SG3 tab toselect a valid static physical image ID. Return andpress **使用 SG3 当前图素（人工候选）**. The image is copied intoone source-bound manual preview;the checkbox toggles it,and**清除** removes it. Browsing another image does not silently replace an existing candidate. The image canvas bottom-edge centre is placed at the saved footprint centre as an explicit preview convention. Every view says **人工候选 · 锚点未验 · confirmed0**;this does not identify the building's original artwork oranchor. There isno built-in type→image default. Invalid/missing targets andsource/plane changes clear prior candidates.

**保存本地候选 / 载入本地候选** use normal local JSON dialogs. Save outside GameData andGit checkouts (including aliases). The16 metadata keys contain the source/snapshot revision,record geometry,archive/physicalID,pixel revision andmanual anchor convention;no pixels orconfirmed flag are stored. Load requires the currently selected SAV andsame parsed record,andredecodes the original image before accepting it. A changed snapshot/image,missing resource,invalid configuration orattempted confirmed status clears the old preview. Revision tokens are non-security FNV-1a fingerprints,not a proof ofsemantic image binding. Explicit real-data technical cases pass3/3 rendered candidates/CGImages/configuration roundtrips;they intentionally do not claim that the test images depict their target buildings. Confirmed image bindings remain0.

Open **图素与贴图**, select an SG3 package and paste an explicit comma-separated physical-ID list into **明确帧ID列表**, then choose **应用帧列表**. Physical ID1 is vector index0; failed/empty frames reject the whole list. Use **上一帧 / 下一帧 / 播放** and set a presentation rate of1..30 FPS. The loop and rate are preview controls, not original timing or reverse-endpoint rules. **减去原始X/Y偏移对齐** uses a fixed common canvas and the displayed signed metadata as an explicit preview convention; disable it for raw top-left canvas alignment. It does not establish a city drawing anchor. Changing packages/static images clears the sequence; leaving the tab pauses it. Local evidence supports **6/6** manually inspected sequences (3 walker,3 ambient;no verified separate building base),**122/122** decoded frames/CGImages and**36/36** GUI checks. Sample ID lists/contact sheets remain outside Git at `/Users/zoomzhao/Documents/Codex/2026-10-14/task/sg-animation-20261019/verified-sequences.json`; no sample-specific IDs are embedded in the App. Original red shadow-marker pixels are retained; final shadow composition remains unverified.

## Integration boundary and next city work

This is a native original-data browser and asset preview, not a complete Emperor game. The single launch command above is the recommended current package. Its integration acceptance covers all **167 loose MAP +46 embedded PAK city sources**, both strict and candidate modes, **6 supplied SAV snapshots** with numerical fields/cell lookup, SG3 asset sequences, and the existing muted audio/video entries. Strict-complete counts remain **139/167 MAP and33/46 PAK**; candidate-complete counts remain **167/167 and46/46**. Cross-tab switching, cleared error results, stopped SG3 timers, movie pause/Stop/reopen and normal process/decoder-child release are checked on this package. Earlier package evidence is retained separately; frame lists/pixels remain external.

A complete original city still needs verified camera/elevation and drawing anchors, underlay/foreground/shadow composition, source-bound buildings/units, a SAV image-field contract, original animation timing and game rules (movement/pathfinding, economy, services, ownership and saved-state restoration). Candidate asset coverage does not establish these. CitySimulation/EMP1 remain frozen and cannot fill the missing original rules.

Next core target: one original city slice with a **source-bound building base drawn over its actual MAP/PAK surface**. Start from one nonempty building record in the published city-table layout and seek an explicit type/name-to-SG3 archive/record binding; then validate its stored coordinate, footprint and draw anchor against a provenance-matched original reference, with independent nonadjacent/held-out checks. Required evidence is the actual record schema/bounds, an image binding stronger than ID range or visual resemblance, and a same-source original placement reference. Until image binding is available, the existing numerical scene shows only the explicitly unbound saved footprint described above. This increment does not operate the original game.

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
```

`python3 -B scripts/map_reference.py test` checks the underlying renderer. `python3 -I -B scripts/independent_runner.py app --output /absolute/private/check-results` checks packaging without launching either game. Existing format tests and corpus commands are documented in their format files. Synthetic checks are regression evidence, not original-data coverage.

Follow [AGENTS.md](AGENTS.md):keep user data read-only and original assets, screenshots, dumps and binaries outside Git. Every change must improve a measured [STATUS.md](STATUS.md) result or remove a named blocker. Source is [MIT](LICENSE);parser and dependency credits remain in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
