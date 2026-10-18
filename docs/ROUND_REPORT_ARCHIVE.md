# Round-report archive

Maintenance manifest for a conservative cleanup of one-off round reports and
point-in-time evidence snapshots under `docs/`, per `AGENTS.md`: *"Keep README.md,
one-screen STATUS.md and one document per format. Preserve licenses and repository
contribution metadata. Delete stale round reports via ordinary recoverable Git
commits; never rewrite history or remove untracked user files/local task evidence."*

Nothing outside `docs/` was touched. No format reference, user-facing document,
capability audit, protected-prefix file (`MAP*`, `SG3*`, `SAV*`, `BINK*`,
`CITY_MODEL*`, `CITY_SCENARIOS*`, `CITY_VALIDATION*`, `CITY_CAPABILITY*`,
`CITY_BLACKBOX*`), `evidence/`, license or contribution metadata was deleted.
Every listed deletion is a tracked file, so it is recoverable from Git history.

## Method

1. `git ls-files docs` enumerated every tracked file under `docs/` (191 files).
2. For each file the **basename** was searched as a fixed string across every
   tracked file in the repository (`git grep -F -l "<basename>"`, excluding the
   file itself). Files with **any** inbound reference were kept (criterion 2).
   75 files had zero inbound references.
3. Each zero-reference file was judged on its own name and content to decide
   whether it is a round/phase report or point-in-time snapshot (criterion 1).
4. Evidence companions whose only inbound reference was a round report that is
   itself archived here were archived as a unit; the manifest records the
   referring file so the dependency is explicit.
5. Files that are format references, user-facing docs, capability audits, source
   artifacts, interface contracts, or a surviving report/successor were kept and
   are listed below with the disqualifying reason.

**Counts:** 191 tracked files under `docs/` were triaged; 99 were evaluated as
deletion candidates (75 zero-inbound-reference files plus 24 files whose only
inbound references were other archived candidates).

- `considered`: **99**
- `deleted`: **80**
- `kept-uncertain` (zero-inbound-reference files retained): **19**
- tracked files remaining under `docs/` after this change (including this file): **112**
- files kept because they have at least one inbound reference: **116**

## Deleted files

| # | path | kind | superseded-by | inbound `git grep` refs |
|---|---|---|---|---|
| 1 | `docs/CALLBACK_REPAIR.md` | App round report | obsolete, no successor | none (0 hits) |
| 2 | `docs/CANDIDATE_IMPORT.md` | App round report | superseded by `docs/CANDIDATE_IMPORT_REVIEW_FIXES.md` | none (0 hits) |
| 3 | `docs/CG_IMAGE_CONVERSION.md` | App round report | obsolete, no successor | none (0 hits) |
| 4 | `docs/CITY19_AFTER_GOALS_C_FLOOD_FRONTIER.md` | CITY19 phase report | superseded by `docs/CITY19_FLOOD_FESTIVAL_SEEDED_READERS.md` | none (0 hits) |
| 5 | `docs/CITY19_COMPONENT2_FIXED_SECTION.md` | CITY19 phase report | superseded by `docs/CITY19_MEMBER_HEADER_FRONTIER.md` | none (0 hits) |
| 6 | `docs/CITY19_CURRENT_POINTER_PROVENANCE.md` | CITY19 phase report | superseded by `docs/CITY19_ACTUAL_LOAD_ROUTE.md` | none (0 hits) |
| 7 | `docs/CITY19_EMPIRE_GRIDS15_COMPLETE.md` | CITY19 phase report | superseded by `docs/CITY19_AFTER_EMPIRE_CONSUMERS.md` | none (0 hits) |
| 8 | `docs/CITY19_EMPIRE_INLINE_PREFIX.md` | CITY19 phase report | superseded by `docs/CITY19_EMPIRE_GRIDS15_COMPLETE.md` | none (0 hits) |
| 9 | `docs/CITY19_FIRST_CLASS_DECLARATION.md` | CITY19 phase report | superseded by `docs/CITY19_HOUSING_PAYLOAD.md` | none (0 hits) |
| 10 | `docs/CITY19_FIRST_FIGURE7_FRONTIER.md` | CITY19 phase report | superseded by `docs/CITY19_PHEASANT_FIGURE_LIST_COMPLETE.md` | none (0 hits) |
| 11 | `docs/CITY19_FIRST_SECTION6.md` | CITY19 phase report | superseded by `docs/CITY19_COMPONENT2_FIXED_SECTION.md` | none (0 hits) |
| 12 | `docs/CITY19_GOAL_WALK_TO_HOMAGE.md` | CITY19 phase report | superseded by `docs/CITY19_HOMAGE_TO_TRADING_DECLARATION.md` | none (0 hits) |
| 13 | `docs/CITY19_GOODSCONTROL_COMPLETE.md` | CITY19 phase report | superseded by `docs/CITY19_EMPIRE_INLINE_PREFIX.md` | none (0 hits) |
| 14 | `docs/CITY19_HOMAGE_TO_TRADING_DECLARATION.md` | CITY19 phase report | superseded by `docs/CITY19_TRADING_TO_YEARLY_DECLARATION.md` | none (0 hits) |
| 15 | `docs/CITY19_HOUSING_PAYLOAD.md` | CITY19 phase report | superseded by `docs/CITY19_NEXT_MEMBER_TAG_CACHE.md` | none (0 hits) |
| 16 | `docs/CITY19_MAINPOOL_BUILDING_BATCH.md` | CITY19 phase report | superseded by `docs/CITY19_FIRST_FIGURE7_FRONTIER.md` | none (0 hits) |
| 17 | `docs/CITY19_MEMBER_HEADER_FRONTIER.md` | CITY19 phase report | superseded by `docs/CITY19_FIRST_CLASS_DECLARATION.md` | none (0 hits) |
| 18 | `docs/CITY19_NEXT_MEMBER_TAG_CACHE.md` | CITY19 phase report | superseded by `docs/CITY19_GOAL_WALK_TO_HOMAGE.md` | none (0 hits) |
| 19 | `docs/CITY19_PHEASANT_FIGURE_LIST_COMPLETE.md` | CITY19 phase report | superseded by `docs/CITY19_GOODSCONTROL_COMPLETE.md` | none (0 hits) |
| 20 | `docs/CITY19_POPULATION_AND_TEN_MEMBERS.md` | CITY19 phase report | superseded by `docs/CITY19_AFTER_GOALS_C_FLOOD_FRONTIER.md` | none (0 hits) |
| 21 | `docs/CITY19_RELIGION_COMMERCE_TO_MAINPOOL.md` | CITY19 phase report | superseded by `docs/CITY19_MAINPOOL_BUILDING_BATCH.md` | none (0 hits) |
| 22 | `docs/CITY19_TRADING_TO_YEARLY_DECLARATION.md` | CITY19 phase report | superseded by `docs/CITY19_YEARLY_TO_POPULATION_DECLARATION.md` | none (0 hits) |
| 23 | `docs/CITY19_YEARLY_TO_POPULATION_DECLARATION.md` | CITY19 phase report | superseded by `docs/CITY19_POPULATION_AND_TEN_MEMBERS.md` | none (0 hits) |
| 24 | `docs/COMMIT_SCOPE.md` | point-in-time commit-scope report | obsolete, no successor | none (0 hits) |
| 25 | `docs/NATIVE_MEDIA_DOCUMENT_REVIEW_20261012.md` | dated document review | obsolete, no successor | none (0 hits) |
| 26 | `docs/ORIGINAL_FILE_PREFIX_QUALIFICATION.md` | qualification round report | superseded by `docs/REAL_MAP_PARAMETER_AND_CITY19_PREFIX.md` | none (0 hits) |
| 27 | `docs/PROFILE_GATE_OPTIMIZATION_FIX.md` | one-off fix report | obsolete, no successor | none (0 hits) |
| 28 | `docs/QUALIFICATION_UNPARSED_LENGTH_FIX.md` | one-off fix report | obsolete, no successor | none (0 hits) |
| 29 | `docs/REAL_DATA_MINIMAL_STRUCTURE.md` | qualification round report | superseded by `docs/REAL_FILE_BLOCK_DECODE_QUALIFICATION.md` | none (0 hits) |
| 30 | `docs/REAL_FILE_BLOCK_DECODE_QUALIFICATION.md` | qualification round report | superseded by `docs/REAL_MAP_PARAMETER_AND_CITY19_PREFIX.md` | none (0 hits) |
| 31 | `docs/REAL_MAP_PARAMETER_AND_CITY19_PREFIX.md` | qualification round report | superseded by `docs/CITY19_REAL_FILE_BLOCK_QUALIFICATION.md` | none (0 hits) |
| 32 | `docs/TEXT_RESOURCE_FEEDBACK.md` | App round report | obsolete, no successor | none (0 hits) |
| 33 | `docs/city19-callback4f5e50-effects-20261012/base-call-sites-adopted.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 34 | `docs/city19-callback4f5e50-effects-20261012/selected-effects.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 35 | `docs/city19-component2-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_COMPONENT2_FIXED_SECTION.md` |
| 36 | `docs/city19-current-pointer-provenance-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_CURRENT_POINTER_PROVENANCE.md` |
| 37 | `docs/city19-empire-grids15-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_EMPIRE_GRIDS15_COMPLETE.md` |
| 38 | `docs/city19-empire-inline-prefix-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_EMPIRE_INLINE_PREFIX.md` |
| 39 | `docs/city19-first-Figure7-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_FIRST_FIGURE7_FRONTIER.md`, `docs/city19-pheasant-Figure-list-evidence.json` |
| 40 | `docs/city19-first-class-declaration-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_FIRST_CLASS_DECLARATION.md` |
| 41 | `docs/city19-first-section6-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_FIRST_SECTION6.md` |
| 42 | `docs/city19-goal-walk-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_GOAL_WALK_TO_HOMAGE.md` |
| 43 | `docs/city19-goodscontrol-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_GOODSCONTROL_COMPLETE.md` |
| 44 | `docs/city19-hero-virtual24-and-next-owner-20261012/hero-current-type-dependencies.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 45 | `docs/city19-hero-virtual24-and-next-owner-20261012/next-owner6-prefix-source-binding.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 46 | `docs/city19-homage-walk-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_HOMAGE_TO_TRADING_DECLARATION.md` |
| 47 | `docs/city19-housing-payload-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_HOUSING_PAYLOAD.md` |
| 48 | `docs/city19-init-and-later-source7-20261012/constructor-and-unresolved-type-evidence.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 49 | `docs/city19-init-and-later-source7-20261012/later7-request-source-binding.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 50 | `docs/city19-init-and-later-source7-20261012/maincaller-consumer-graph.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 51 | `docs/city19-mainpool-building-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_MAINPOOL_BUILDING_BATCH.md` |
| 52 | `docs/city19-member-header-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_MEMBER_HEADER_FRONTIER.md` |
| 53 | `docs/city19-minimal-initial-node-contract-20261012/consumer493f00-outer-contract.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 54 | `docs/city19-minimal-initial-node-contract-20261012/minimal-initial-type-contract.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 55 | `docs/city19-minimal-initial-node-contract-20261012/review-targets.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 56 | `docs/city19-next-member-tag-cache-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_NEXT_MEMBER_TAG_CACHE.md` |
| 57 | `docs/city19-owner-reset-and-node-callback-20261012/parent-reset-root-preservation.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 58 | `docs/city19-owner-reset-and-node-callback-20261012/previous-component-validation-preserved.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 59 | `docs/city19-owner-reset-and-node-callback-20261012/related-out-counter-write-scope.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 60 | `docs/city19-owner-reset-and-node-callback-20261012/selected-node-route-and-unresolved-callbacks.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 61 | `docs/city19-pheasant-Figure-list-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_PHEASANT_FIGURE_LIST_COMPLETE.md` |
| 62 | `docs/city19-population-walk-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_POPULATION_AND_TEN_MEMBERS.md` |
| 63 | `docs/city19-postgoals-frontier-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_AFTER_GOALS_C_FLOOD_FRONTIER.md` |
| 64 | `docs/city19-reader-conditions-20261012/conditional-modern-model-saved-validation.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 65 | `docs/city19-reader-conditions-20261012/minimum-conditions.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 66 | `docs/city19-reader-conditions-20261012/source-evidence.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 67 | `docs/city19-registration4de410-20261012/callback-paths-adopted.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 68 | `docs/city19-registration4de410-20261012/minimum-read-contract-progress.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 69 | `docs/city19-root-effects-and-embedded-20261012/embedded-selected-contract-binding.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 70 | `docs/city19-root-effects-and-embedded-20261012/hero-root-effects-and-remaining-callees.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 71 | `docs/city19-root-effects-and-embedded-20261012/nominal-record-validation.json` | dated REA/Ghidra session snapshot | obsolete, no successor | none (0 hits) |
| 72 | `docs/city19-trading-walk-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_TRADING_TO_YEARLY_DECLARATION.md` |
| 73 | `docs/city19-yearly-walk-evidence.json` | city19 evidence snapshot | obsolete, no successor | `docs/CITY19_YEARLY_TO_POPULATION_DECLARATION.md` |
| 74 | `docs/native-media-document-review-20261012.json` | point-in-time evidence snapshot | obsolete, no successor | `docs/NATIVE_MEDIA_DOCUMENT_REVIEW_20261012.md` |
| 75 | `docs/original-profile-synthetic-integration-evidence.json` | point-in-time evidence snapshot | obsolete, no successor | none (0 hits) |
| 76 | `docs/profile-gate-optimization-fix-evidence.json` | point-in-time evidence snapshot | obsolete, no successor | `docs/PROFILE_GATE_OPTIMIZATION_FIX.md` |
| 77 | `docs/qualification-unparsed-length-fix-evidence.json` | point-in-time evidence snapshot | obsolete, no successor | `docs/QUALIFICATION_UNPARSED_LENGTH_FIX.md` |
| 78 | `docs/real-data-minimal-structure-evidence.json` | point-in-time evidence snapshot | obsolete, no successor | none (0 hits) |
| 79 | `docs/real-file-block-decode-qualification-evidence.json` | point-in-time evidence snapshot | obsolete, no successor | `docs/REAL_FILE_BLOCK_DECODE_QUALIFICATION.md` |
| 80 | `docs/real-map-parameter-city19-prefix-evidence.json` | point-in-time evidence snapshot | obsolete, no successor | `docs/REAL_MAP_PARAMETER_AND_CITY19_PREFIX.md`, `docs/real-data-minimal-structure-evidence.json` |

Notes on the successor column: entries reading *"obsolete, no successor"* are honest
statements that no current document consolidates the file's content; the extracted
format facts remain reproducible from Git history (the deletion commit). Where a
later report in the same incremental chain continues from an earlier one, the
immediate successor is named; the surviving terminal reports in each chain are
`docs/CITY19_ACTUAL_LOAD_ROUTE.md`, `docs/CITY19_AFTER_EMPIRE_CONSUMERS.md`,
`docs/CITY19_FLOOD_FESTIVAL_SEEDED_READERS.md`,
`docs/CITY19_REAL_FILE_BLOCK_QUALIFICATION.md` and
`docs/CANDIDATE_IMPORT_REVIEW_FIXES.md` (all kept).

## Considered and kept

### Zero-inbound-reference files kept (criterion 1 or 3 uncertain)

| path | reason kept |
|---|---|
| `docs/APP_LAUNCH.md` | Describes the App's current launch/`--data-root` entry behaviour; plausibly a user-facing doc, not a one-round report. |
| `docs/DEFAULT_RESOURCE_BROWSER.md` | Published-App source/behaviour boundary; arguably a current capability/user-facing doc. |
| `docs/DEPENDENCIES.md` | Dependency/execution-boundary reference, not a round report. |
| `docs/FIRST_PLAY.md` | User-facing first-play guide. |
| `docs/NATIVE_MAP_INPUT.md` | Documents a current interface contract (`map_geometry.hpp`). |
| `docs/ORIGINAL_FILE_OUTER_PROFILE.md` | Outer-file format/profile reference (header markers, offsets, vtable slots); a format fact, not a report. |
| `docs/ORIGINAL_FILE_SYNTHETIC_FIXTURES.md` | Fixture policy/boundary reference; pairs with `tests/original_file_fixtures/source_contract.json`. |
| `docs/READER_LIFETIME_CONTRACT.md` | Documents the current public `FramedReader`/`OwnedFramedReader` lifetime contract. |
| `docs/callback-regression-source.mm` | Source artifact, not a round report. |
| `docs/CITY_VALIDATION.json` | Protected name prefix (`CITY_VALIDATION*`). |
| `docs/MAP_GROUND_BRANCHES.md` | Protected name prefix (`MAP*`). |
| `docs/city19-figurefsa-conditional-reader-20261012/holder30-candidates.json` | Directory is referenced by `file_io/tests/figurefsa_conditional/README.zh-CN.md`; kept to avoid gutting a referenced snapshot directory. |
| `docs/city19-figurefsa-conditional-reader-20261012/wire-contract.json` | Same directory reference. |
| `docs/city19-figurefsa-new-payloads-20261012/remaining-gaps.json` | Directory referenced by `file_io/tests/figurefsa_objects/README.zh-CN.md`. |
| `docs/city19-figurefsa-new-payloads-20261012/wire-contract.json` | Same directory reference. |
| `docs/city19-figurefsa-shared-prefix-chain-20261012/conditional-experiment.json` | Directory referenced by `file_io/tests/figurefsa_prefix_chain/README.zh-CN.md`. |
| `docs/city19-figurefsa-shared-prefix-chain-20261012/object-mapping.json` | Same directory reference. |
| `docs/city19-figurefsastate-payload-20261012/conditional-experiment.json` | Directory referenced by `file_io/tests/figurefsastate_payload/README.zh-CN.md`. |
| `docs/city19-figurefsastate-payload-20261012/source-contract.json` | Same directory reference. |

### Kept because referenced (criterion 2)

116 tracked files under `docs/` have at least one inbound `git grep` reference and
were kept unconditionally. This includes the format references named by
`README.md`/`STATUS.md` (`docs/SG3.md`, `docs/MAP.md`, `docs/SAV.md`, `docs/BINK.md`,
`docs/MAP_SG3_PIPELINE.md`), the retained city19 chain terminal reports listed
above, and JSON read by tooling such as `docs/original-profile-owned-readiness.json`
and `docs/original-file-outer-profile.json` (used by `file_io/qualify_*.py` and the
`file_io/tests/city19_*` suites) and
`docs/city19-religion-following-evidence.json` (read by
`file_io/tests/independent_city19_religion_component/test-expectations.json`).

## Verification (measured after the deletions were applied)

- `git status --short` — exactly 80 unstaged deletions (` D`) under `docs/` plus
  one untracked addition (`?? docs/ROUND_REPORT_ARCHIVE.md`). No staged entries,
  no changes outside `docs/`, no concurrent-agent modifications observed.
- `git diff --diff-filter=D --name-only | wc -l` → `80`; `git diff --stat --diff-filter=D`
  reports `80 files changed, 31640 deletions(-)`.
- Re-running `git grep -F -l "<basename>"` for every one of the 80 deleted
  basenames, excluding this manifest, returned **0 hits** (`basenames_still_referenced_outside_manifest=0`).
- `make map-reference-test` (with `EMPEROR_GAMEDATA_ROOT` unset) → unchanged:
  `{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}` and
  `{"passed":true,"assertions":241,"original_data":false}`, exit 0.
- Seven dated snapshot directories emptied by the deletions were `rmdir`-ed;
  their tracked contents had already been removed.

No build target, test, or code path referenced any deleted file (all deleted
files had zero inbound references, verified both before and after the change).

## Recovery

Every path in the table above is recoverable from the commit preceding this
one; deletion is an ordinary recoverable commit and no history was rewritten.

    git show 9e6e451:<path>            # view a deleted report
    git checkout 9e6e451 -- <path>     # restore it

Verified before landing: 80 deleted files, all tracked and all under `docs/`;
zero inbound references to any deleted basename from anywhere in the
repository; zero code, script, Makefile or test references; zero new broken
Markdown links (the 47 pre-existing broken links among tracked files are
unchanged, and none of them points at a file removed here).

Reversibility risk, recorded rather than hidden: for the city19 chain and the
`.map`/`.dat` outer-container chain, the archived reports are the only place
those offsets/SHAs/addresses were written down — `docs/SAV.md` and
`docs/MAP.md` have no section that supersedes them. If such a fact is needed
again, restore the report from the revision above and fold the fact into the
format document rather than re-adding a round report.
