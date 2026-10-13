# Emperor native data on macOS

Load and render **original Emperor: Rise of the Middle Kingdom data** natively on macOS, then port game logic. Users supply their legally owned files through `--data-root`; new native data code contains no original assets or original game code.

Current phase: **DATA FIRST**. Every change must raise a metric in [STATUS.md](STATUS.md) or remove a listed blocker. CitySimulation/EMP1 and the authored gameplay are frozen and retained; their tests do not measure original-game progress.

Requires macOS 15+ and Apple command-line tools already installed.

```sh
make game-app
./game_lab/build/EmperorApp.app/Contents/MacOS/EmperorApp --data-root /absolute/path/to/GameData
make sg3-test
EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make data-metrics
```

Real-data tests read only `EMPEROR_GAMEDATA_ROOT` and skip cleanly when unset. No implicit repository asset lookup or global environment changes. Native App selection retains `--data-root` precedence. The first data milestone is all discovered SG3 archives, image IDs and supported pixels in the native browser; [SG3 format and scope](docs/SG3.md) defines the denominator and remaining gaps.

Work from original file bytes, published format documents and licensed modern parsers. Never use decompiled/disassembled game code or the retired static execution route. Never commit original files, extracted images, atlases, dumps or captures. Data checks emit aggregate counts, not assets. Follow [AGENTS.md](AGENTS.md).

Our code is [MIT](LICENSE); adapted public SG3 algorithm credits and license are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The concrete cleanup list for historical reports/game-code archives is pending approval after automatic review rejected its bulk scope. They are excluded from the new data pipeline; local user data and task evidence remain untouched.
