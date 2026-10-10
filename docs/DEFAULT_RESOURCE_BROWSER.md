# Default resource browser source boundary

The published App is a resource browser. Its map view uses the shared
`map_geometry.hpp` coordinate contract to display a clicked tile's coordinates.
Clicks do not build, demolish, advance a month, or run a city model. Tile colours
remain an arbitrary preview of IDs; they do not establish original terrain rules.

The default App, integration runner and embedded source manifest require only
published platform sources. They do not require `city_simulation.hpp`. The
manifest identifies the application mode as `resource-browser` and records
`city_model_included: false`.

The movie preview page now consumes the existing MovieReader through the shared
AppMedia control, displaying only frame zero after an explicit request. Its
limited input contract and validation are described in
[APP_MOVIE_POSTER.md](APP_MOVIE_POSTER.md).

Validate from a clean source export without Git or any prototype header:

```sh
python3 -I -B /absolute/export/scripts/independent_runner.py integration --mode release --output /absolute/results
python3 -I -B /absolute/export/scripts/independent_runner.py integration --mode undefined --output /absolute/results
python3 -I -B /absolute/export/scripts/independent_runner.py app --mode release --output /absolute/results
python3 -I -B /absolute/export/scripts/independent_runner.py verify --report /absolute/results/RUN_ID
```

The shared geometry checks cover finite extreme width, NaN/infinity, all edges,
invalid dimensions, and unchanged outputs on rejection. App validation compiles,
packages, verifies the local signature and checks system-library links; it does
not launch the App. Synthetic integration uses the production resource loaders
and a fake audio backend. Each process has a 60-second limit.

The separate local city prototype and its demonstration branches remain
unpublished workspace changes. Their economy, buildings and save format have no
demonstrated original-rule source. They are outside this published entry point;
this change does not restore original city rules, save compatibility, or prove
GUI/audio-device behaviour.


影片页现已复用既有 Controller 接通静默连续预览，保留首帧入口；具体生产接口、交付失效、验收及未运行窗口/声音的边界见 [APP_MOVIE_PLAYBACK.md](APP_MOVIE_PLAYBACK.md)。默认来源仍不包含城市原型。
