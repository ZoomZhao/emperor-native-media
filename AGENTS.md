# Mission and working rules

Make users’ legally owned ORIGINAL Emperor data load and render natively on macOS, then port logic. Current phase: **ORIGINAL CITY RUNS**.

## North-star metric

- **Primary: original SAVs that run ≥100 consecutive updates in the normal App Play path, X/6** (current corpus). Baseline 2026-10-31: **0/6**, all stop at update 1 with `event_ui_5eff20_not_closed`. Stage 2 raises the bar to ≥1000 updates, then a full in-game year.
- Every change must either advance the primary metric or remove the blocker currently stopping it. Raising any other count (checks, fields, records, pixels) is not progress on its own.
- Always work on the **first** blocker the primary path hits. When it is removed, run the path again, record the next blocker in STATUS.md and continue there.

## Unknowns and hypotheses

- When the original behavior is not fully proven, implement the best-supported reading anyway, mark it in code as `HYPOTHESIS(<id>)`, and list it in STATUS.md with its evidence and how it could be verified. Do not stop at an unknown.
- Hypotheses must never be reported as verified original behavior. Replace them with proven logic as evidence arrives; a hypothesis later contradicted by evidence is fixed, not defended.
- Labelled fallbacks that keep the primary path running (for example "no pending event") are allowed if they are hypotheses as above and the refusal mode remains selectable for tests.

## Time-boxing and escalation

- Spend at most one working day on a single blocker without moving the primary metric. Then stop and report to the user: what is known, what was tried, the concrete question, and the proposed hypothesis to continue with.
- Do not switch to side tracks while a blocker is open; escalate instead.

## Frozen side tracks

Until the primary metric reaches 6/6 at ≥100 updates, do not extend: CitySimulation, EMP1, native v6/v8 gameplay, `.emworld` checkpoints, CPU/Metal pixel parity, path/filesystem hardening, Market/House atomicity beyond what the primary path needs, and the `*_lab` components. Keep them building and their existing tests passing; fix them only when they break.

## Verification tiers

- **Inner loop (every change):** one build mode, the tests touching the changed code, plus the primary-path run on the original corpus. Keep this fast.
- **Milestone / before push:** Release and UBSan full regression, frozen golden files, user-data protection check.
- Do not add tests for code paths outside the primary path in the current phase.

## Data and repository rules

- Users supply assets with --data-root. Real-data tests read only EMPEROR_GAMEDATA_ROOT and exit successfully with SKIP when unset. A verified local directory may be injected for one test process; never alter global environment.
- Never commit original assets or derivatives: sprites, atlases, pixel/hash dumps, screenshots, binaries, captures or game-code archives. Aggregate metrics and authored test source are allowed. Keep all user data read-only.
- STATUS.md is one screen: primary metric, current blocker, open hypotheses, next step. Details go in one document per format; do not add per-round reports or evidence JSON to docs/. Keep README.md. Preserve licenses and repository contribution metadata. Delete stale round reports via ordinary recoverable Git commits; never rewrite history or remove untracked user files/local task evidence.
- Use explicit path manifests for commits, normal main pushes and direct remote SHA verification. No force pushes. Do not turn a permission failure for one action into a ban on all data work.
