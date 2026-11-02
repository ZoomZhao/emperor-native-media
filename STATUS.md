# Status — ORIGINAL CITY RUNS

2026-11-03 · macOS arm64 · measure with `EMPEROR_GAMEDATA_ROOT=<data> make primary-metric`

## Primary metric

**Original SAVs running ≥100 consecutive updates in normal App Play: 6/6** (0/6 → 2/6 → 6/6 on 2026-10-31). Release and UBSan both 6/6; headless controller run.

"Runs" means no refusal; agreement with the original game is **unmeasured**. 100 updates ≈ two monthly tails.

## Stage 2: 1000 updates — 0/6

`make primary-metric UPDATES=1000`. Calendar = month / month part (one part = 51 updates). Release measured 2026-11-03; UBSan has not yet been rerun after the sentiment change.

| Source | Updates | Calendar | First refusal |
|---|---|---|---|
| zoom/历史战役自动保存_1 | 815 | 1 / 15 | `monthly_city_4ac650` (month rollover) |
| 项羽/历史战役自动存档_2 | 815 | 7 / 15 | `monthly_city_4ac650` (month rollover) |
| 项羽/.sav | 745 | 8 / 15 | `monthly_city_4ac650` (month rollover) |
| 项羽/历史战役自动保存_1 | 356 | 1 / 6 | `monthly_city_4ac650` |
| zoom/载入旧存档自动保存 | 286 | 1 / 5 | `ordered_type_update_slot_1966_type_24_action_6` |
| zoom/历史战役自动存档_2 | 356 | 7 / 6 | `monthly_city_4ac650` |

Priority: part 8 calls `591200` every cycle. Disassembly identifies population reconciliation and city sentiment, including per-house `590F30`, building condition `591670`, tax/wages/unemployment, monuments `55AE30`, and notices `548920`. Common5 +5C/+8C/+A0 (wire 83/125/144) and Population7 latch/previous sentiment (wire 11266/11173) are bound. Legacy checkpoint decoding leaves the new fields absent. Implement the remaining subcalls; rollover services remain refused. Executable: Chinese `EmperorCH.exe` (SHA `dbdeca1e…`).

Inner loop: 1,000-update Release run 171 s with rebuild before, 46 s warm after; six stop points unchanged. Build conditions differ, and the 30 s target remains unmet.

## Fidelity — 0/0 (2026-11-03)

Explicit isolated pairs: `2026102301/202610230102 → 202610230103` stops at 161/1104 at phase 50, part 15 (month rollover); `2026102801/280008 → 280009` cannot prepare (`combined saved building inputs`). No target-state agreement is scored. The latter includes an unmet request and rating notice; the envoy and gift preceded 280008. Its recorded −60 treasury change has unresolved attribution.

## Open hypotheses

The load hypotheses below are in `game_lab/app/city_playback.cpp` `applyLoadHypotheses`: process state the SAV does not store, supplied in normal play, withheld by `strictPremises`, not written into `.emworld` policy. Verify each by reading it from a live original process at load.

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

`census-skip-phase` (diagnostic mode only): a refused accounting or monthly phase has no side effects apart from calendar advancement. Evidence: phase rollback preserves the last complete candidate; there is no evidence that the omitted original effects are empty. Compare a supervised time-only SAV pair and the executable to verify. Other refusals and year rollover remain hard stops.

`animal-spawn-missed-candidates` (normal-play fallback): assume the 16 placement candidates all fail, so no animal is born. `4EA986..4EA98A` reset the timer and consume one engine random value before the search; actual placement and successful allocation remain unported. Verify by implementing `4EA8E0` placement/allocation or comparing a supervised save across a known spawn attempt. Strict premises and the direct phase API retain refusal.

`single-mill-food-qualifies` (normal-play fallback): assume a sole eligible Mill with ≥100 available food qualifies for the Market's grade-1 request. `541220`/`544480` establish the candidate gates; the blocked save has one matching Mill, 3,000 available food, and saved Market grade range 1–3. The general saved grade bounds and multi-Mill ranking are not yet bound. Verify by retaining the saved bounds and porting `544480`; strict premises retain refusal.

`sentiment-tax-table`: assume the unbound difficulty-dependent `591180` model table contributes zero; verify by parsing its `5D1BF0` model-text source. `sentiment-monuments`: assume `55AE30` has no qualifying completed monument and changes no registry flags; verify by binding the process monument registry. `sentiment-notice-state`: assume notices are enabled and process-only achievement flags are clear; verify from the original initialization and notice registry. `sentiment-reason-multiplier`: assume the `5913E0` floating-point term is not the worst notice reason; verify its saved input and runtime rounding. These hypotheses apply only in normal play; strict premises retain refusal.

## Next step

Pre-animal-fallback 10,000-update diagnostic census, sorted by earliest attempted update (speculative after any no-op; all six saves stopped hard):

1. `city_accounting_4f1560_4ac2fa`: 210, 285 and 1383 (hard; at 210 this is phase 5 animal spawning `4EA8E0`, not accounting). Two live animal formations have spawn timer 26 and are below their limit.
2. `monthly_city_4ac650` phase 50: first 338, 69 no-op encounters across four saves; year rollover hard at 8976 in two saves.
3. `type_action_route_target_combat_update` (figure type 40/action 7): 812 (hard).

Next: bind the `591180` tax table, then investigate the part-7 and month-rollover refusals. Type-24 supplier action 6 remains blocked after 286 admitted updates; generic dispatch selects `4D1810`. The animal spawn fallback also needs a real port.

Frozen checkpoint note: hypothesis-run `.emworld` restore refuses again; `test_original_world_checkpoint.py` has a pre-existing `world_binding_goal_prefix` failure at `73e0e03`. Format details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md).
