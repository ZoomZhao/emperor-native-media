# Emperor native data on macOS

Load legally owned original Emperor data with `--data-root`, render it natively on macOS, then port logic. Current phase: **DATA FIRST**. CitySimulation, EMP1 and authored gameplay remain frozen and separate from original behavior.

## Launch the local App

```sh
"/Users/zoomzhao/Documents/Codex/2026-10-14/task/candidate-surface-20261019/release/EmperorApp.app/Contents/MacOS/EmperorApp" --data-root "/Users/zoomzhao/Workspace/lzjq/GameData"
```

This candidate-preview delivery stays outside Git and GameData;the earlier strict and diagnostics App bundles are preserved. No game assets are bundled. Another legally owned directory requires changing the explicit argument. macOS15+ is required. The App starts muted; Bink playback uses the existing external FFmpeg and reports missing dependencies without installing them.

Open **原始地图** and select a standalone city MAP. The default **原始地表图素（MAP 存储 surface）** view reads its saved surface-image grid and renders SG3/555 pixels. Camera X/Y moves the viewport. The default **候选补全（原版放置未验证）** toggle adds complete residual storage groups using an explicit candidate anchor and foreground composition. Turn it off to inspect the unchanged strict path. The panel reports strict counts,candidate decoded/visible counts and their combined renderable total separately. Missing resources,invalid groups anddecode failures remain unknown;candidate pixels do not establish original positioning orocclusion. The scrollable **未覆盖格分类与0基活动坐标** list shows each located failure,its class and active X/Y;enter those coordinates in the camera fields to inspect the area. Unknown locations stay explicitly counted,and source/plane/invalid-camera changes clear old coordinates. The list retains strict-path unknowns even when candidate art is visible. Numerical terrain/bit views remain available. PAK and SAV selections retain their numerical views; they do not acquire an unproved stored-surface binding.

The App adapter reads167/167 city sources and creates167/167 native CGImages. Actual App checks cover11/11 transitions:complete/partial surfaces,candidate off/on,camera-targeted candidate paint,numerical fallback,PAK clearing,SAV restrictions,invalid camera andreturn;344/344 tested input files remain unchanged. The source manifest covers82/82 files and28 compiled units;strict signing and source correspondence are checked. These are delivery checks, not original-screen accuracy.

## Separate measurements

| Measurement | Current result | Boundary |
|---|---|---|
| Stored surface rendering coverage | **139/167** city maps complete;**2,106,879/2,107,808** nonzero cells decoded (99.9559%);929 cells remain | Reads persisted image IDs directly. This is static surface coverage, not a complete city or pixel-match rate. |
| Residual structural groups | **426/426** complete groups covering929/929 residual cells | Exact stored membership,offsets anddecoded source canvases;this does not set an original screen anchor. |
| Candidate preview | **167/167** combined scenes;**2,107,808/2,107,808** nonzero cells renderable,including929 candidate cells;167/167 native CGImages | Explicit unverified anchor andforeground layer. Reference coverage does not mean projected pixel area is filled;gaps remain. Strict139/167 coverage remains separate. |
| Reconstructed terrain→image accuracy | **1,299,617/1,514,539** emitted predictions match stored codes (85.809%) | The reconstructed selector is scored separately;the App surface view does not use it as a substitute for stored IDs. |
| Original-screen comparison | **0/167** complete city pictures verified | Camera/elevation, composition, buildings, units and runtime state still need independent evidence. Coverage does not imply playability. |

[MAP evidence and App integration](docs/MAP.md#native-app-stored-surface-view) and the existing [stored-grid pipeline](docs/MAP_SG3_PIPELINE.md) retain the scope and unresolved cases. Earlier explicit-scene experiments and original observations remain in the format documents, not the launch instructions.

## Other native entries

- [SG3/555](docs/SG3.md):58/58 indexes;71,751/72,370 images.427 empty records and192 images missing `ChinaMapProj.555` remain. Static decoding does not establish animation playback.
- [MAP/PAK](docs/MAP.md):168/168 MAP containers and31/31 packages;46 embedded city grids. Numerical views remain diagnostic.
- [SAV](docs/SAV.md):6/6 supplied-root plus9/9 separate app-root snapshots;90/90 numerical fields. These15 paths are not15 distinct cities. Buildings, walkers and current memory are unparsed.
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
