# Status — ORIGINAL CITY RUNS

2026-10-31 · macOS arm64

## Primary metric

**Original SAVs running ≥100 consecutive updates in normal App Play: 0/6.**
All six stop at update 1 with `event_ui_5eff20_not_closed`.

## Current blocker

`0x5EFF20` event/UI step: normal mode does not supply event gates +17/+16 (and the other unresolved process inputs in `game_lab/app/city_playback.cpp` `applyUnresolvedZeroPremises`), so the first update refuses. With those inputs zeroed (experiment mode) sources reach updates 21–41 and then refuse at unported code: phase 40 `4AC2B0`, building virtual20 `416D20`, house evolution `519120/518EF0`, road refresh `4ACFC0`, monthly tail `4AC650`.

## Open hypotheses

None yet. Each new `HYPOTHESIS(<id>)` in code gets one line here: id, evidence, how to verify.

## Next step

1. Add a `primary-metric` runner (normal Play, 6 sources, up to 100 updates, reports X/6 and first refusal per source).
2. Supply the event gates and other premise inputs in normal mode as labelled hypotheses; keep strict refusal selectable for tests.
3. Port the first unported-code blocker the runner reports, then repeat.

## Retained, frozen

Native v6/v8 city App, EMP1, CitySimulation, `.emworld` checkpoints, CPU/Metal display paths and SG3 rendering (71751/72370 images) keep building and passing their tests; not extended this phase. Details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md) · previous full status table in git history at `3c85560:STATUS.md`. Build/use: [README.md](README.md).
