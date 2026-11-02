# Status — ORIGINAL CITY RUNS

2026-11-03 · macOS arm64 · measure with `EMPEROR_GAMEDATA_ROOT=<data> make primary-metric`

## Primary metric

**Original SAVs running ≥100 consecutive updates in normal App Play: 6/6** (0/6 → 2/6 → 6/6 on 2026-10-31). Release and UBSan both 6/6; headless controller run.

"Runs" means no refusal; agreement with the original game is **unmeasured**. 100 updates ≈ two monthly tails.

## Stage 2: 1000 updates — 4/6

`make primary-metric UPDATES=1000`. Calendar = month / month part (one part = 51 updates). Release and UBSan measured 2026-11-03; both produce the counts and refusals below.

| Source | Updates | Calendar | First refusal |
|---|---|---|---|
| zoom/历史战役自动保存_1 | 1000 | 2 / 3 | none |
| 项羽/历史战役自动存档_2 | 1000 | 8 / 3 | none |
| 项羽/.sav | 1000 | 9 / 4 | none |
| 项羽/历史战役自动保存_1 | 1000 | 2 / 3 | none |
| zoom/载入旧存档自动保存 | 337 | 1 / 6 | `building_virtual20_416d20` |
| zoom/历史战役自动存档_2 | 627 | 7 / 12 | `type_action_route_target_combat_update_slot_1984_type_45_action_8` (buy/sell search) |

Ordinary month rollover now runs traced food, wages, debt, production and population-history effects plus explicitly labelled remaining assumptions. Annual rollover remains refused. Executable: Chinese `EmperorCH.exe` (SHA `dbdeca1e…`).

Inner loop: 1,000-update Release run 171 s with rebuild before, 46 s warm after; six stop points unchanged. Build conditions differ, and the 30 s target remains unmet.

## Fidelity — 1 pair reached (2026-11-03)

`2026102801/280011 → 280012` reaches 123/123 with figure state 2310/2310, images 42/42 and RNG 2/2 after correcting fractional-speed route pauses. `2026102301/202610230102 → 280011` likewise matches those groups at 128/128; the original pair's terrain differs in four cells, with bit400000 cleared on all four, plus200 on one and20000000 on another. `202610230102 → 202610230103` reaches 1104/1104: figure state 2278/2310, images 41/42, RNG 0/2 (original 36 draws behind playback). `280012 → 202610230103` now reaches 853/853 with the same long-pair score and RNG delta, localizing the remaining divergence after 280012. Calendar 5/5, treasury/year/population 1/1 each, sentiment 3/3, accounting 12/12, labor 20/20, employment 55/55, formation identity 100/100 and state 60/60, figure identity 2041/2041. These saves have no live buildings, so building/house groups are 0/0. `280008 → 280009` prepares but stops at 3/860 (`building_virtual20_416d20`); its −60 treasury change remains unattributed.

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

`sentiment-tax-table`: if `Model/EmperorTaxSentimentModel.txt` is absent or invalid, assume the `591180` tax contribution is zero; the seven-row table is otherwise parsed from the data root. Verify fallback against the original model-loading failure path. `sentiment-monuments`: assume `55AE30` has no qualifying completed monument and changes no registry flags; verify by binding the process monument registry. `sentiment-notice-state`: assume notices are enabled and process-only achievement flags are clear; verify from the original initialization and notice registry. `sentiment-reason-multiplier`: assume the `5913E0` floating-point term is not the worst notice reason; verify its saved input and runtime rounding. These hypotheses apply only in normal play; strict premises retain refusal.

`monthly-partner-4936d0-empty` and `monthly-partner-495a50-empty`: assume the respective part-7 selected interactions make no relation/event changes or additional random draws. `4A8ED0`'s saved eligibility and scenario flag and its one to three random draws are retained; actual event effects are unported. Verify by porting both selected event constructors and comparing saved event/relationship state. Strict premises retain refusal.

`monthly-part12-candidates-empty`: assume the unbound request flag, strength, ratings and existing-request predicates in `4A90A0` yield no candidate, so the selection draw and `494260` are not reached. The executable proves status 0/4 also requires negative treasury and relation value ≥50; the blocked save has nonnegative treasury but other statuses remain eligible. `monthly-part12-relation-default`: assume `441F70`'s `440C10` default-state normalization changes no relation. Verify by porting the missing gates and event construction; strict premises retain refusal.

`trader-station-offers-goods`: assume `5DB640` finds a trade at the merchant's alive, staffed, road-connected station. The blocked save has such a station with storage records; its partner permission and stock searches remain unbound. Verify by porting those predicates and comparing trade and station state. Strict premises retain refusal.

Rollover hypotheses (normal play only; strict premises refuse): `rollover-manager-empty` assumes the `4BFF20/4BFE10` manager has no monthly action; `rollover-primary-building-inactive` assumes the type-110 hook in `592060` is inactive; `rollover-health-target-stable` and `rollover-tax-recount-stable` preserve the latest targets across the unbound house recounts `590DB0` and `4AE900`; `rollover-normal-finance-mode` assumes `591AC0`'s editor gate is clear; `rollover-food-prelude-neutral` assumes `4AEDF0`'s additional effects and the `5482E0` notice are absent; `rollover-monument-timer-inactive` assumes `55AF70` has no qualifying timer; `rollover-religion-monthly-neutral` assumes `5A8620` has no additional effects; `rollover-other-notice-counters-zero` assumes the 19 unbound `547670` counters are zero; `rollover-road-refresh-stable` assumes `4B6D00/5AD8F0` leave terrain and roads unchanged; `rollover-notice-sort-neutral` assumes `547440` only changes notice presentation; `rollover-incoming-cargo-stable` preserves the `4EB850` counts; `rollover-monthly-events-empty` and `rollover-scenario-events-empty` assume the event calls named in code produce no additional events. Evidence is the cited executable call order and bound saved state, not proof of these outcomes. Verify by porting each named service and comparing target-calendar saves. The annual branch remains refused.

## Next step

Pre-animal-fallback 10,000-update diagnostic census, sorted by earliest attempted update (speculative after any no-op; all six saves stopped hard):

1. `city_accounting_4f1560_4ac2fa`: 210, 285 and 1383 (hard; at 210 this is phase 5 animal spawning `4EA8E0`, not accounting). Two live animal formations have spawn timer 26 and are below their limit.
2. `monthly_city_4ac650` phase 50: first 338, 69 no-op encounters across four saves; year rollover hard at 8976 in two saves.
3. `type_action_route_target_combat_update` (figure type 40/action 7): 812 (hard).

Next: identify the update-338 building virtual refusal in the old zoom save, then continue the type-24 supplier whose arrival transfer remains refused; close the land trader's action-8 buy/sell search. The −36 RNG gap is parked pending the supervised midpoint. The trader's inactive-network wait clears transaction mode/timer via `4C9AD0`; above 10, station virtuals may change stock and trade state. Goods1 cursors, Grid15 quota usage/base prices, child5 relationship prices, the trader sold count and SAV19's 100 trader ledgers are now bound for those searches. The former type-40 refusal does not recur through 1000 updates on the corrected animal trajectory.

Frozen checkpoint note: hypothesis-run `.emworld` restore refuses again; `test_original_world_checkpoint.py` has a pre-existing `world_binding_goal_prefix` failure at `73e0e03`. Format details: [SAV.md](docs/SAV.md) · [ENCITY.md](docs/ENCITY.md) · [SG3.md](docs/SG3.md).
