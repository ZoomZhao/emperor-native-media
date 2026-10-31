# Status — ORIGINAL CITY RUNS

2026-10-31 · macOS arm64 · measure with `EMPEROR_GAMEDATA_ROOT=<data> make primary-metric`

## Primary metric

**Original SAVs running ≥100 consecutive updates in normal App Play: 6/6** (0/6 → 2/6 → 6/6 on 2026-10-31). Release and UBSan both 6/6; headless controller run.

| Source | Live buildings / houses | Updates |
|---|---|---|
| zoom/历史战役自动保存_1 | 0 / 0 | 100 |
| 项羽/历史战役自动保存_1 | 0 / 0 | 100 |
| zoom/历史战役自动存档_2 | 568 / 26 (type 5) | 100 |
| zoom/载入旧存档自动保存 | 473 / 26 (type 7) | 100 |
| 项羽/历史战役自动存档_2 | 7 / 4 (type 3) | 100 |
| 项羽/.sav | 17 / 14 (types 2, 3) | 100 |

"Runs" means no refusal; agreement with the original game is **unmeasured**. 100 updates ≈ two monthly tails.

## Current blocker

None within 100 updates. Stage 2 target: ≥1000 updates, then one in-game year. Known unported but unreached: larger-house geometry in `519120/518EF0` (no such house in this corpus).

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
| force-evolution-off-at-load | 10BFF78 = 0 | phase 39 keeps desirability comparison |
| previous-health-zero-at-load | D6241C = 0 | monthly tail part 3 reads only at rating 151/152 |

## Known gaps

`.emworld` restore of a hypothesis-run world refuses again at the next update (policy omits hypotheses; frozen side track). `test_original_world_checkpoint.py` fails `world_binding_goal_prefix` on 项羽/历史战役自动保存_1 — pre-existing at `73e0e03`.

## Next step

1. Raise the runner horizon to 1000 updates and a full year; port each first blocker.
2. Add a fidelity check: compare a run against a later original save of the same city where one exists.

## Retained, frozen

Native v6/v8 city App, EMP1, CitySimulation, `.emworld` checkpoints, CPU/Metal display paths and SG3 rendering keep building and passing their tests; not extended. Details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md) · previous full status at `3c85560:STATUS.md`. Build/use: [README.md](README.md).
