# Emperor native data on macOS

Load and render **original Emperor: Rise of the Middle Kingdom data** natively on macOS, then port game logic. Users supply their legally owned files through `--data-root`; new native data code contains no original assets or original game code.

Original Bink silent video is connected through an existing external FFmpeg installation; [Bink scope and tests](docs/BINK.md) distinguish full playback evidence from smoke and exclude audio/synchronization claims.

The **原始地图** tab opens original city MAP terrain flags as a read-only grid with a verified centered active-box crop, an independent numerical palette and 32 annotated bit-plane views. The full 228×228 backing grid remains selectable; unknown bits are explicit. It does not restore city objects or claim the original palette/projection. [MAP format and scope](docs/MAP.md) distinguishes city maps from the unsupported empire map.

Current phase: **DATA FIRST**. Every change must raise a metric in [STATUS.md](STATUS.md) or remove a listed blocker. CitySimulation/EMP1 and the authored gameplay are frozen and retained; their tests do not measure original-game progress.

Requires macOS 15+ and Apple command-line tools already installed.

```sh
make game-app
./game_lab/build/EmperorApp.app/Contents/MacOS/EmperorApp --data-root /absolute/path/to/GameData
make sg3-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make data-metrics
make map-test
make map-test-undefined
make map-sg3-audit-test
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

The read-only MAP→SG3 metadata audit verifies the published object-grid layout and SG3 lookup bounds, and rejects an unmodified direct-ID candidate with corpus counts. It reads no .555 pixels and produces no image join. [Mapping evidence and stop condition](docs/MAP.md#mapsg3-association-audit) distinguish field facts, candidate tests and unknown semantics.

The reusable [scoped input driver](input_lab/scoped_click.hpp) and own-AppKit regression diagnose the current mouse-routing blocker without GameData or original-game startup. Tests build in a temporary directory by default; `--build-dir` keeps local evidence, `--window` opts into one authored window and its capture, and missing existing permissions produce SKIP without requesting access. [MAP input scope](docs/MAP.md) documents the single-click request and the still-unverified Wine boundary. Use `python3 input_lab/run_tests.py --request /absolute/local/request.json --inspect` for a read-only snapshot of AX/CG owners, bounds, layer/alpha and the public display-shield ID; it sends no input. Missing values remain null. Application windows at the numeric cursor level remain eligible; foreign front covers still block. A full-display system Dock row is excluded at the selected point only when an existing-permission single-window capture proves a transparent3×3 pixel patch, current AX hits the exact target, and fresh process/frame/layer evidence agrees. All other Dock/foreign covers remain subject to the original checks; missing capture evidence retains the refusal. Inspection may acquire this in-memory Dock witness; it saves no pixels. Release/UBSan ordinary own-window tests pass; Wine clicking remains unverified. The user's `/Applications/皇帝龙之崛起.app` normal window is the historical observation baseline; the2026-10-14 readonly check found0 current processes/windows for that bundle, so no game was restarted or clicked.

Work from original file bytes, published format documents and licensed modern parsers. Never use decompiled/disassembled game code or the retired static execution route. Never commit original files, extracted images, atlases, dumps or captures. Data checks emit aggregate counts, not assets. Follow [AGENTS.md](AGENTS.md).

Our code is [MIT](LICENSE); adapted public SG3 algorithm credits and license are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). 53 clearly named stale round reports were removed with a normal recoverable Git commit. Source, tests, current format documents and repository metadata remain. Archived code is excluded from the new data pipeline; local user data and task evidence remain untouched.
