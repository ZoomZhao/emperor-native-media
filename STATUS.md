# Status — ORIGINAL CITY RUNS

2026-10-31 · macOS arm64 · measure with `EMPEROR_GAMEDATA_ROOT=<data> make primary-metric`

## Primary metric

**Original SAVs running ≥100 consecutive updates in normal App Play: 2/6** (baseline 0/6 this morning).

| Source | Updates | First refusal |
|---|---|---|
| zoom/历史战役自动保存_1 | 100 | — |
| 项羽/历史战役自动保存_1 | 100 | — |
| zoom/历史战役自动存档_2 | 39 | `house_evolution_519120_518ef0` |
| zoom/载入旧存档自动保存 | 39 | same |
| 项羽/历史战役自动存档_2 | 39 | same |
| 项羽/.sav | 20 | same |

The two passing sources are probably the building-free tutorial cities ([SAV.md](docs/SAV.md) §new SAVs); not yet confirmed. Headless controller run, Release only.

## Current blocker

`originalBuildingHouseEvolutionStep` (`game_lab/original_building_update.hpp`): only house types 2/3/5/7 are ported; larger house geometry (allocation/merge path of `519120/518EF0`) refuses. Stops 4/6 sources.

## Open hypotheses (all in `game_lab/app/city_playback.cpp` `applyLoadHypotheses`)

Process state the SAV does not store, supplied in normal play, withheld by `strictPremises`, not written into `.emworld` policy. Verify each by reading it from a live original process at load.

| Id | Value | Evidence |
|---|---|---|
| event-ui-gates-closed-at-load | 1E53538 +17/+16 = 0 | 5EFF20 proven early return, no stores |
| scenario-slots-empty-at-load | 8 slots null | fresh process zero-fill; 466E90 loops call nothing |
| manager-gate-closed-at-load | 10AF4E0 +8 = 0 | 492452..460 skips mode5 |
| migrant-spacing-zero-at-load | D62418 = 0 | phase 23 clamps at 0 |
| notice-history-at-file-initial | 8501FC/FD = 1 | `.data` initial image |
| service-coverage-zero-at-load | C82D58[11] = 0 | 4AF2D0 rewrites 0..3, 7..10 |
| highest-building-slot-zero-at-load | 0 | phase 10 scan records live max |
| religion-bonus-off-at-load | 10BFB50 = 0 | phase 18 festival branch off |
| religion-scratch-at-file-initial | 1312F78 = 0, 85D720 = −1 | zero-fill / `.data` initial |
| producer-tails-zero-at-load | entries 30..35 = 0 | phase 33 preserves them |
| effect-scratch-zero-at-load | 101D12C = 0, 8900E8 = 0 | phase 45 decays after 7 |

## Known gaps

`.emworld` restore of a hypothesis-run world refuses again at the next update (policy omits hypotheses; frozen side track). `test_original_world_checkpoint.py` fails `world_binding_goal_prefix` on 项羽/历史战役自动保存_1 — pre-existing at `73e0e03`.

## Next step

Port the larger house-evolution geometry path, re-run `primary-metric`, continue with the next first blocker.

## Retained, frozen

Native v6/v8 city App, EMP1, CitySimulation, `.emworld` checkpoints, CPU/Metal display paths and SG3 rendering keep building and passing their tests; not extended. Details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md) · previous full status at `3c85560:STATUS.md`. Build/use: [README.md](README.md).
