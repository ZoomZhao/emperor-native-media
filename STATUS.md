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

## Stage 2: 1000 updates — 0/6

`make primary-metric UPDATES=1000`. Calendar = month / month part (one part = 51 updates).

| Source | Updates | Calendar | First refusal |
|---|---|---|---|
| zoom/历史战役自动存档_2 | 209 | 7 / 4 | animal spawn `4EA8E0` (formation phase, reported as `city_accounting_4f1560_4ac2fa`) |
| zoom/载入旧存档自动保存 | 235 | 1 / 4 | `poolRoadSelected` missing for building that gained a road after load |
| 项羽/历史战役自动保存_1 | 236 | 1 / 4 | same |
| zoom/历史战役自动保存_1 | 305 | 1 / 5 | monthly tail part 6 (`591200` city ratings); parts 7/8/12 also unported |
| 项羽/.sav | 235 | 8 / 5 | same |
| 项羽/历史战役自动存档_2 | 305 | 7 / 5 | same |

## Fidelity — 0/0

`make fidelity-metric`. Needs same-city SAV pairs where the original game only advanced time. The corpus has 4 same-map forward pairs, all rejected for player terrain edits (40–113 cells). **Blocked on new clean original saves** (load→save, +1 month, +1 year with no player actions).

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

1. Record `poolRoadSelected` for buildings that gain road access after load.
2. Port monthly tail parts 6/7/8/12 (`591200`, `4A8ED0`, `4A90A0`).
3. Port animal spawn placement `4EA8E0` (`551690/5516E0/551700/551650`, terrain test `59CBB0`).
4. Collect clean original save pairs and run `fidelity-metric`.

## Retained, frozen

Native v6/v8 city App, EMP1, CitySimulation, `.emworld` checkpoints, CPU/Metal display paths and SG3 rendering keep building and passing their tests; not extended. Details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md) · previous full status at `3c85560:STATUS.md`. Build/use: [README.md](README.md).
