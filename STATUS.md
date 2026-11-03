# Status — ORIGINAL CITY RUNS

2026-11-03 · macOS arm64 · Stage 3: `EMPEROR_GAMEDATA_ROOT=<data> make primary-metric UPDATES=9792`

**Stage 1: 6/6 at 100 updates. Stage 2: 6/6 at 1000 updates**, Release and UBSan verified 2026-11-03. Original behavior agreement is measured separately below. One year is 12 × 16 × 51 = 9792 updates. Chinese EXE SHA `dbdeca1e…`.

**Stage 3: 6/6 at 9792**, Release and UBSan 2026-11-03: all six original saves complete one in-game year in normal App Play without refusal. This is a run-length result, not a claim of full fidelity.

**Fidelity** (original save pairs; — means not scored):

| Pair | Updates | Buildings | Figures | Images | Formation | Labor | RNG |
|---|---:|---:|---:|---:|---:|---:|---:|
| 280014 → 280015 | 2678/2678 | 2302/2450 | 970/1210 | 12/22 | 45/60 | 18/20 | −699 draws |
| 280014 → 280016 | 3450/3450 | 2300/2450 | 793/990 | 9/18 | 47/60 | 18/20 | −562 draws |
| 280015 → 280016 | 772/772 | 2627/2650 | 2068/2314 | 25/42 | 53/60 | 20/20 | −162 draws |
| 281030 → 281031 | 40/40 | 28407/28409 | 5164/5170 | 92/94 | 60/60 | 20/20 | 2/2 |

The annual pair loads 53/53 live records (12 ordinary, 41 ruins) and scores previous-year finance 12/12, production 36/36, quotas 616/616 and year 1/1; wind is unsaved and cannot be directly compared. On the longer pair one original building is absent; the port records five destructions and 41 live ruins, matching the original's ruin count. Terrain agrees on 51980/51984 cells, so four of the 45 original changes differ from the port; their event attribution remains unclassified. The port's 58 total records include five tombstones, leaving 53 live. Longer-pair labor mismatches are health and year arrivals; formation mismatches are count, spawn timer and member slots.

**Fidelity:** `281030 → 281031` 40/40: building common 28407/28409 (farmer link differs on two adjacent, equal-scored fields); crop industry 884/884; figure state 5164/5170 (four cell-list links across two cells, two process-clock variants), images 92/94, type-26 50/50; RNG 2/2; calendar, treasury, accounting, labor, employment, sentiment, population, house goods/services, crop storage and formations agree. The original process's field-registration order is unverified. `281031 → 280008` 1008/1008: treasury 0/1, accounting 11/12, RNG 0/2 (−517 draws), building common 28265/28409, crop industry 846/884, house services 375/416, figure identity 1997/2047 and state 619/660; the user-observed +75 trade, −60 charge and +786 accepted envoy gift reconcile the original money. Empty-event hypotheses may be contradicted if the saves share a trajectory. `280008 → 280009` 860/860: treasury 1/1 and accounting 12/12; RNG 0/2 (−1330 draws), building common 28238/28409, crop industry 747/884, figure identity 1976/2037 and state 1123/1210.

Animal pairs `0102 → 280011` and `280011 → 280012` match figure state 2310/2310, images 42/42 and RNG 2/2; `0102 → 0103` and `280012 → 0103` have 2278/2310, 41/42 and RNG −36 draws. The midpoint capture is pending; four terrain flag differences remain unexplained.

**Open hypotheses** (normal play unless specified; strict premises withhold them). Verify the following process-only load defaults against independent initialization or observed state: `event-ui-gates-closed-at-load`, `scenario-slots-empty-at-load`, `manager-gate-closed-at-load`, `migrant-spacing-zero-at-load`, `notice-history-at-file-initial`, `service-coverage-zero-at-load`, `highest-building-slot-zero-at-load`, `religion-bonus-off-at-load`, `religion-scratch-at-file-initial`, `producer-tails-zero-at-load`, `effect-scratch-zero-at-load`, `force-evolution-off-at-load`, `previous-health-zero-at-load` (evidence: fresh-process initialization or cited gate/phase in `city_playback.cpp`).

- `animal-spawn-missed-candidates`: no spawn candidate; verify 4EA8E0 placement. `single-mill-food-qualifies`: sole stocked Mill qualifies; verify 544480 grade selection.
- `fire-wind-zero-at-load`: process-only C82F2C starts at zero; 4ACE50 changes it during annual rollover. Verify fresh-process initialization and same-process loads; strict premises retain refusal.
- `saved-flame-animation-from-cell`: 42AA5B initializes the unsaved flame animation from a terrain random byte modulo five; assume its saved stationary cell is the creation cell. Strict loads retain refusal. Verify a same-process flame capture or the creation-cell argument.
- `saved-ruin-road-identity`: treat neighboring saved ruins like the unchanged owner probes already used for newly created ruins. Strict loads refuse. Verify ruin virtual road-neighbor callbacks against the executable.
- `stall-food-pickup-empty`: 5464E0 produces no transfer/servants; a Mill has food. Verify Market grade/capacity and 4E3C70/5464E0. `station-cargo-permission-denied`: 443BA0/443C00 rejects eligible cargo; verify saved permissions and allocations.
- `trader-station-offers-goods`: 5DB640 finds a trade; verify eligibility. `trader-god-price-neutral`: process modifiers A5B028/A5B024 are inactive; verify process state or supervised transfer.
- `sentiment-tax-table`, `sentiment-monuments`, `sentiment-notice-state`, `sentiment-reason-multiplier`: fallback model/registry/notice effects; verify the named 591200 subcalls.
- `monthly-partner-4936d0-empty`, `monthly-partner-495a50-empty`, `monthly-part12-candidates-empty`, `monthly-part12-relation-default`: assume no unbound event/relation effects; verify selected event constructors and the supervised envoy trajectory. `census-skip-phase` is diagnostic only and assumes omitted phases have no effects.
- Rollover: `rollover-manager-empty`, `rollover-primary-building-inactive`, `rollover-health-target-stable`, `rollover-tax-recount-stable`, `rollover-normal-finance-mode`, `rollover-food-prelude-neutral`, `rollover-monument-timer-inactive`, `rollover-religion-monthly-neutral`, `rollover-other-notice-counters-zero`, `rollover-road-refresh-stable`, `rollover-notice-sort-neutral`, `rollover-incoming-cargo-stable`, `rollover-monthly-events-empty`, `rollover-scenario-events-empty`; verify the named services and target-calendar saves.

**Next:** locate the first RNG/figure divergence in 280015 → 280016 and explain the collapse/fire terrain changes and longer-pair labor/formation differences. The supervised transfer pair and animal midpoint are pending. Checkpoint coverage explicitly identifies fields absent from the frozen wire format; real App restore was skipped and previously failed `world_binding_goal_prefix` at `73e0e03`.
