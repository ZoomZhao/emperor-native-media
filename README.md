# Emperor native data on macOS

Load and render **original Emperor: Rise of the Middle Kingdom data** natively on macOS, then port game logic. Users supply their legally owned files through `--data-root`; new native data code contains no original assets or original game code.

Original Bink silent video is connected through an existing external FFmpeg installation; [Bink scope and tests](docs/BINK.md) distinguish full playback evidence from smoke and exclude audio/synchronization claims.

The **原始地图** tab opens original city MAP terrain flags as a read-only 228×228 grid with an independent numerical palette and 32 bit-plane views. It does not restore city objects or claim the original palette/projection. [MAP format and scope](docs/MAP.md) distinguishes city maps from the unsupported empire map.

Current phase: **DATA FIRST**. Every change must raise a metric in [STATUS.md](STATUS.md) or remove a listed blocker. CitySimulation/EMP1 and the authored gameplay are frozen and retained; their tests do not measure original-game progress.

Requires macOS 15+ and Apple command-line tools already installed.

```sh
make game-app
./game_lab/build/EmperorApp.app/Contents/MacOS/EmperorApp --data-root /absolute/path/to/GameData
make sg3-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make data-metrics
make map-test
make map-test-undefined
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make map-metrics
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData python3 game_lab/run_bink_tests.py
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData python3 game_lab/run_bink_tests.py --window --smoke
```

Real-data tests read only `EMPEROR_GAMEDATA_ROOT` and skip cleanly when unset. No implicit repository asset lookup or global environment changes. Native App selection retains `--data-root` precedence. [SG3 format and scope](docs/SG3.md) defines the image denominator and remaining gaps. For out-of-tree MAP checks, use `python3 scripts/map_data.py test --build-dir /absolute/temporary/path` or `metrics --mode undefined --build-dir /absolute/temporary/path`.

Work from original file bytes, published format documents and licensed modern parsers. Never use decompiled/disassembled game code or the retired static execution route. Never commit original files, extracted images, atlases, dumps or captures. Data checks emit aggregate counts, not assets. Follow [AGENTS.md](AGENTS.md).

Our code is [MIT](LICENSE); adapted public SG3 algorithm credits and license are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). 53 clearly named stale round reports were removed with a normal recoverable Git commit. Source, tests, current format documents and repository metadata remain. Archived code is excluded from the new data pipeline; local user data and task evidence remain untouched.
