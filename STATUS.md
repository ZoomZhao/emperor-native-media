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

`make primary-metric UPDATES=1000`. Calendar = month / month part (one part = 51 updates). Release and UBSan agree.

| Source | Updates | Calendar | First refusal |
|---|---|---|---|
| zoom/历史战役自动保存_1 | 611 | 1 / 11 | monthly tail part 12 `4A90A0` (trade requests) |
| 项羽/历史战役自动存档_2 | 611 | 7 / 11 | same |
| 项羽/.sav | 541 | 8 / 11 | same |
| 项羽/历史战役自动保存_1 | 356 | 1 / 6 | `monthly_city_4ac650` (part 6, 16th-cycle rating `591200`) |
| zoom/载入旧存档自动保存 | 284 | 1 / 5 | `city_accounting_4f1560_4ac2fa` |
| zoom/历史战役自动存档_2 | 209 | 7 / 4 | animal spawn `4EA8E0` (formation phase) |

Ported 2026-11-01: type-71 stall road gate/spawn (`4295C0`, `48B950`, `48B780`, `51CB50`), post-load road selection (`426DF0`), trade delays from constructor `5DC4A0` (land 0x22, sea 4), tail parts 6/8 cycle gate (`4AC882`) and part 7 no-partner return (`4A8ED0`). Executable read: Chinese build `EmperorCH.exe` (SHA `dbdeca1e…`); the documented `0ca8fc07…` is not on disk. Same section layout; 352 differing `.text` bytes near `525F2A`/`527B2E`, none at read addresses.

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

1. Port tail part 12 `4A90A0` (route table `C5DB78` via `483730`, partner ratings `592370`, war/tribute `441F70`).
2. Port the 16th-cycle city rating `591200` and the remaining accounting refusal at 284.
3. Port animal spawn placement `4EA8E0` (cell search over `F6A9E0`/`FC3750`).
4. Clean original pairs: being collected via Codex CLI per `docs/ORIGINAL_GAME_OPERATIONS.md`; isolated copy already has one clean pair (`2026102301/…0102 → …0103`, 1104 updates).

## Retained, frozen

Native v6/v8 city App, EMP1, CitySimulation, `.emworld` checkpoints, CPU/Metal display paths and SG3 rendering keep building and passing their tests; not extended. Details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md) · previous full status at `3c85560:STATUS.md`. Build/use: [README.md](README.md).
