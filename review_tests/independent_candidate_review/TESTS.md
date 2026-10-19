# Reproducible independent tests

Reviewed baseline: `87d5babf7817c977b2dacdd23cada5d53ea4fa1f`.
macOS / Apple clang / Foundation / CoreGraphics / zlib are required.
No AppKit, GUI, original executable, original data or network is used by this suite.

```sh
python3 -B run_review.py --source-root /absolute/read-only/source
```

The default output is a new private temporary directory. `--out-dir` must be outside
the source checkout, every Git checkout, and GameData. Each compile has a 60 second
timeout and each execution 30 seconds. Seven compilations and eight executions
give an upper bound of eleven minutes, excluding process cleanup. All subprocesses
are serial. Original-data environment settings are removed from authored tests.

At this baseline the exit status is **1**: the negative-slot and metadata read-budget
regressions fail. Positive properties and persistence callbacks pass. A zero exit
requires all three groups to pass; read `review-results.json` for separate results.
The first run predates this aggregation change; its saved results were recomputed
with the final `finish` function, yielding exit 1. Native tests were not changed.

| Test | Expected behavior and source | What is actually exercised |
|---|---|---|
| `negative_slot_mre.mm` | README manual candidate contract and `docs/SAV.md` source/record invalidation: invalid record input clears old selection/candidate | Current `onOriginalBuildingLocate:` and `onOriginalManualAdopt:` are extracted verbatim at each run. After a valid slot 1 adoption, `-18446744073709551615` must not retain slot 1. Foundation controls; plane callback stubbed. |
| `candidate_properties.mm` | `docs/SAV.md` explicit preview convention, premultiplied RGBA and source binding; config header's public schema bounds | 64 whole-image comparisons use independent footprint-centre projection and floating point source-over oracle; scales 1–4, fractional camera centres, partial clipping, alpha 0/1/127/128/254/255. All eight target identity components, source/revision, invalid huge dimensions, metadata types and promotion attempts. No original-image association is inferred. |
| `headless_persistence.py` | README/local config contract and `docs/SAV.md`: 16 reference keys, unchanged source/record/image required; failure clears old candidate; data/Git paths excluded | Five actual App callbacks extracted unchanged: locate/adopt/clear/save/load. Real authored JSON file writes and reads; mocked accepted/cancelled panel responses and fake image decoder. Tests stale/current source, revision, all record fields, missing/changed pixels, missing/malformed/oversize JSON, recovery, forbidden save paths. It does not validate native file panels, image view rendering, or source/plane-switch callbacks. |
| `metadata_memory_probe.mm` | Metrics source explicitly limits manifests to 16 KiB; `docs/SAV.md` limits real metrics to six explicit cases | Calls unchanged metrics entrypoint, then measures its peak RSS. Two authored regular sparse files (16,385 bytes and 32 MiB) are rejected. Peak RSS growth should stay below a deliberately loose 8 MiB envelope, not scale with the rejected file. Asset root is an empty authored directory. This assertion is macOS-specific and not a claim about ordinary GUI loading. |

The patch adds authored review code only under
`review_tests/independent_candidate_review/`; it does not change production code.
`negative_slot_mre.mm` retains the reviewed baseline methods for readability;
`run_review.py` replaces their implementations with the selected source each time.
New production callback dependencies may require updating mock declarations.
Original built-in 40-check tests and the three private real-data cases are separate
evidence in `REVIEW.md`, not assets or fixtures included in this patch.
