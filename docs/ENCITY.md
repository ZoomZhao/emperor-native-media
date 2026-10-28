# ENCITY: original-data native sessions v1–v6

This independently authored playable subset was explicitly selected by the user. It shares original SAV decoding, SG3/555 assets and the existing native renderer, but owns separate rules and state. It neither extends EMP1/CitySimulation nor supplies values to the strict original executor's anonymous event gates. Original missions and SAV continuation are not supported. Unknown original behavior is not claimed as implemented.

## Ordinary fresh-city entry and versioned delivery (2026-10-28)

The normal home screen can create the existing explicit city-contract development
mode from either admitted empty SAV source. Both development options start off;
unselected creation remains v6. Old native saves keep their saved strategy and
are never migrated by opening a newer App. The development option is a bounded
native implementation, not original mission or populated-SAV continuation.

The house panel explains the opening sequence: locate the source entrance, lay
roads into its existing road network, build neighboring houses, resume for real
immigrant arrivals, then use the resulting labor for a Well and preventive tower.
`入城口` is a view-only action and the minimap marks the source endpoint. It does
not construct a road, grant population, move an actor or change the saved policy.
Only the two admitted empty sources are validated; other sources retain their
existing named refusals.

Entrance feedback is computed from the same `nativeToStrict` projection as the
runtime. These are deliberately separate facts: `入城可达` reads the B7C runtime
distance, which can traverse ordinary ground; `接原路网` compares the selected
neighboring road's B0C component with the actual entrance road component. A
locally adjacent road can satisfy the first and fail the second. Neither fact
asserts that an immigrant is already scheduled or guaranteed to arrive. Missing
source/template/endpoints remain named unknowns, not fabricated zero distances.
No scheduler, route generation, staffing or save-wire behavior was changed.

`scripts/package_native_release.py` copies the signed, self-authored App into a
new, unique directory outside the repository. It refuses existing targets,
symlinks, unexpected bundle files and invalid signatures. It includes relative
`Start.command` (CPU), `Start-Metal.command`, `PLAY.txt` and a final `release.json`
receipt; launchers reject incomplete deliveries. Keep the directory together.
No game data, private absolute resource path, login item or permanent environment
setting is packaged. With no resource argument/environment the App offers its
normal directory chooser. Explicit invalid roots fail instead of silently
falling back. A root can be supplied with `--data-root` or the process-local
`EMPEROR_GAMEDATA_ROOT`. The original data remains external and read-only.

The final same-production CLI tests pass **760/760 per source**, for both sources
in Release and UBSan: ordinary zero-population construction, physical road vs
runtime reachability, query purity, connection removal, natural arrivals,
staffing/water/patrol, and exact file recovery. Each uses 59 normal construction
commands, first arrival at update391, patrol at573 and cached water at641, with
population24. These are deterministic native updates, not recovered original
calendar timings. Existing feedback regression remains **2232/2232 per mode**
over two sources and24 checkpoints; startup contract18 checks and packaging10
authored test methods pass.

The signed **72e9b34b…** App binds **152/152** production files and **31/31**
compiled units. Ordinary-home GUI acceptance passes **4/4** source/backend flows:
CPU **204/204**, Metal **204/204**, with two normal Cmd+Q exits0 and no forced
cleanup. Every flow creates a genuinely empty city, performs all59 paid map
clicks, observes actual arrivals and population24, cached water and preventive
patrol service, then byte-exactly saves/reloads its own played city. No native
fixture is loaded before construction. CPU/Metal fresh and prepared-paused saves
are byte-identical for both sources. GUI checkpoint sampling observes arrival
at391 (CPU) /396 (Metal), and combined service at641 (CPU) /650,648 (Metal);
these are observed pause points, not claims of different simulation rules.

CPU used the first unique persistent directory; Metal directly used the final
unique directory. Both directories have byte-identical bundles and launchers;
the final receipt also binds the hardened packager. Actual final launchers pass
help and explicit missing-root failures through a path containing spaces. Metal
uses a spaced, local symlink referring only to the already authorized external
root; no resource payload is copied or published. With no root argument or
environment, the same final version visibly presents its resource chooser.
Initial AX/Escape probes failed to retain a normal cancellation result; their
records remain. A current-picture/foreground/owned-window-verified Cancel click
normally closes the chooser and process. Its parent had already timed out, so
the cancellation exit code is unavailable and is not counted among the two
successful gameplay Cmd+Q exit codes.

## MarketHouse conservation contract R2 (2026-10-28)

This is a checked transfer contract for the existing project consumer, not a
claim about original Market capacity or legal original saves. Its input guard
allowed a six-bay amount10000, while the smallest-positive-bay selector only
selects values strictly below10000. The authored counterexample returned success
after adding10 food to a house but left market food10000 unchanged. The old
binary/result is retained locally; no original input was needed.

`originalFigureMarketHouse` now calculates into separate market/house candidates.
For food28 and materials19/13/25/23/22/24, actual source debit must equal the
signed household-word increase in words0/1/6/2/4/5/3. Unmapped resources must not
change. A missing later model, unrepresentable credit or mismatch rejects the
whole callback, including earlier quality, demand, cache and count writes. The
existing selector and sentinel are retained: a large untouched bay does not
alone cause rejection when a smaller eligible bay supplies the entire credit.
No capacity400/800 rule is invented. Model intermediates use checked wide
arithmetic; demand and service-count signed-word wrapping is preserved, while
a stock-credit wrap cannot pass as a conserved positive transfer.

The entire type23 service segment also uses a candidate. Failure on a later
house or the final grid check cannot leave earlier recipients served. Success
publishes links and the service-grid lane together without replacing map nodes;
owner references remain valid. Other service types retain their existing path,
and movement keeps its wider transaction boundary.

Run `make market-transfer-test` and `make market-transfer-test-undefined`, or
`python3 scripts/market_transfer.py --mode release --output-dir <new absolute
local directory>`. Each mode compiles47 project dependencies and passes80 new
authored transfer checks,138 HouseGoods consumer checks and3369 full figure/
combined checks, including784 existing storage-transfer cases. The runner clears
the original-data root for its children; the real branch reports SKIP. These
are synthetic API/format/caller contracts, not new original-game observations.

Coverage includes all seven resource lanes at9999/10000/10001, mixed compartments,
partial selector exhaustion, late missing models, multiple-house and grid-error
rollback, saturated demand, final-food quality, numeric boundaries and ordinary
distribution. Existing signed Apps and saves are untouched. No new App/GUI
acceptance is claimed for this source change. Real food production, retail23
construction, buyer24, quality recipes, stall/calendar scheduling and the full
player market loop remain separate evidence and integration gaps.

### Market retailer retirement boundary

The existing type23 return path already marks an arrived retailer `alive=2`.
Its removal helper previously cleared the owner's primary/secondary link before
checking late home/route/cell conditions. An authored direct call with route1000
returned false after changing primary7 to0. The ordered pool caller already
restored its wider pre-update candidate; the public cleanup boundary did not.

Type23 removal now runs that existing algorithm on a candidate. Failure
publishes no owner, route, cell-list, target-reference, reset or FIFO changes;
success publishes the complete cleanup while preserving map-node addresses.
Primary-first/secondary-link ordering and reciprocal-only route release retain
their existing semantics. Unrelated stock, demand, quality, directions, service
grid and RNG are preserved. Other types use the previous removal path.

The shared market suite has137 authored checks (80 transfers plus57 lifecycle)
in each Release/UBSan mode. It covers late refusals, malformed pool/cell links,
head/middle/tail unlinking, primary/secondary cases, another actor's route,
target clearing, repeated removal, highest-slot bookkeeping and alias safety.
Copying an already represented pool snapshot and replaying removal is tested;
this is not a new SAV writer or a native market save format. The native Well
codec still rejects type23. Existing v1-v6 goldens and native defaults remain.

This closes a public cleanup transaction contract, not a complete market
lifecycle. Type23 construction/redispatch, type24 actions/removal and food
quantity/quality production remain unproved or absent. No new market UI,
original observation, new App or end-to-end replenishment claim is made.

## Read-only collector fields and qualification boundary (2026-10-28)

`make original-evidence-test` and `make original-evidence-test-undefined` build
an exporter that accepts only the finite input manifest beneath the per-process
`EMPEROR_GAMEDATA_ROOT`. With no root, real inputs explicitly SKIP. Use
`--source` to select an existing manifest entry; input hashes and identities
must remain unchanged. No game is launched and no save is written.

The `clock-producer-figure7-v1` field contract reads phase/cycle/month-part/month
from existing SAV19 anchors even when there are no buildings. It separately
exports saved year words without inferring their calendar meaning. Existing
Common5/industry readers provide stocks, resource identities, employment,
operating flags and Hunter counters; independent reader aliases are checked.
Selected Hunter, prey and projectile records expose known Figure7 base fields,
including signed coordinates and raw links. Food/quality words remain raw
observations rather than recipes or business capacities. Unknown variable FSA
stops parsing and withholds later pool/Goods certification; a known base record
never establishes the complete type23/type24 lifecycle.

Release and UBSan each pass2353 authored format checks and28 Python path/contract
groups. Four existing read-only inputs pass203 structural checks per mode,
including an empty city. These counts establish framing and cross-reader
agreement, not live production or original algorithm equivalence. Market
replenishment, calendar/month services and health/consumption remain incomplete.
The native-v6 default, previous save formats and fixed App packages are unchanged.

Three separate bounded original-control qualification attempts failed before
reaching a verified paused city. The last run's first complete observation of
both game and launcher absence, with input restored, was541.7 seconds after
startup, outside its300-second window. The original supervisor terminal record
is missing. Pause/double-save/road and controlled owner-loss checks were not
completed; automatic progression remains disabled. Later local offline driver
repairs do not retroactively qualify that run. Controllers, captures, raw saves,
trial logs and source/hash manifests are not part of this repository delivery.

## Supported source and rules

The v1 loader accepts a complete, view0 SAV19 with a complete4,000-record building table, no live buildings and saved population0. It requires validated ground layers and a nonnegative saved treasury. The tested source is an independently created initial tutorial save. MAP and populated SAV session creation are explicitly unavailable; their existing read-only browsing remains separate.

The session references the relative original source, its existing snapshot revision and a revision of `Model/EmperorBuildingModels.txt`. The original model notes identify building column0 as construction cost and housing column17 as population capacity. V1 explicitly chooses Normal building prices, types22 (road) and2 (vacant house), and the existing difficulty-independent house capacity reader's row0. The observed local model yields road2, house15 and capacity7. These values are loaded, not embedded in the rules or test corpus.

Native v1 rules:

- Construction permits only unowned flat plain/fertile cells with no mapper object or other terrain flags; unknown terrain is unavailable. Roads occupy one cell; houses occupy2×2. Overlaps and insufficient funds refuse the whole command.
- Every road/house has a monotonic native identity. Its source/model identity remains distinct from original archive runtime slots. A drawing adapter explicitly projects native housing into existing static type2/type3 artwork; it does not serialize a fake original building record.
- An orthogonally road-adjacent house gains one resident on each even native tick until the loaded capacity. Connectivity to a city gate, traveling immigrants, service needs and original immigration timing are not implemented. Native ticks are requested approximately once per second; paused or closed native windows do not advance. There is no elapsed-time catch-up.
- Treasury starts from the source's saved balance and decreases only by committed construction costs. Taxes, recurring expenses, original events/goals, services, disasters, trade, house evolution and walking figures are unavailable. Population counts only residents of native houses.
- Commands and ticks share one checked transaction function with an expected revision. Candidate state validates before publication; invalid/stale commands leave all fields unchanged. The App additionally prepares/decodes the candidate drawing before publishing construction or residency changes. A drawing failure cannot debit funds.

## Format and persistence

`.encity` is a separate bounded UTF-8 text format with ASCII numeric records. It contains no original asset payloads or original SAV byte modifications. It is never offered as an original-game save.

```
ENCITY 1 original-data-native-v1
"relative/source.sav" sourceRevision modelRevision
activeSide initialFunds roadCost houseCost houseCapacity
revision ticks nextId paused funds objectCount
id kind x y residents
...
```

`kind`22 is a native road and2 a native residence using the corresponding original model identities; it does not mean a persisted original object pointer. Every object identity must be sequential and unique, coordinates/footprints bounded and nonoverlapping, population nonnegative and within capacity, and funds exactly initial balance minus all construction prices. V1 permits at most3,999 objects, one billion ticks and one trillion revisions. Unknown versions/rules, trailing records, oversize input, changed source/model and invalid states refuse loading before replacing the current city. A failed load leaves the current state/timer intact. The format is validation for local session data, not an authentication or tamper-proof protocol.

The App's default `NativeSessions` directory is alongside its bundle; the full path is editable. `保存并暂停` first performs the ordinary Pause command, then saves a paused checkpoint. A temporary file in the destination directory is fully written and fsynced, then linked to the new destination without replacement. Existing files and symlinks are never overwritten. Reads and writes under the external game-data root are refused. New names are required for later checkpoints; saving is explicit, not automatic on quit. Relaunch, select the same original source, open the v1 subset and load the `.encity` path. Resume continues the saved tick sequence.

## Regressions and acceptance

Use `python3 scripts/native_session.py --mode release --output-dir /tmp/emperor-native-v1-release` and `--mode undefined` with a separate directory. Real checks only read `EMPEROR_GAMEDATA_ROOT`, set for that process, plus explicit `--save <relative-source.sav>`. Without the environment they exit successfully with SKIP. Test binaries, compiler dependencies, inputs and object hashes are recorded outside the repository.

Initial Release and UBSan each pass129 authored assertions: atomic refusal, overlap, negative coordinates, insufficient funds, stale commands, disconnected houses, capacity limits, pause, deterministic continuation, source/model/version mismatch, invalid counters/ledger/identities and click-projection inversion. The real source passes1/1 per mode:2000→1983 after one road and one house, seven residents at capacity, original housing art binds1/1, independent file readback is exact, existing-file replacement and game-root writes are refused. These are native-rule results, not recovered original behavior. Actual App acceptance is recorded separately after the GUI loop completes.


### Real native App acceptance (2026-10-24)

The baseline had no separate original-data playable loop **0/1**. The new normal native entry passes the defined single-source loop **1/1** through actual App controls, with no direct state injection or experimental strict-executor premises. Select `Save/2026102301/202610230101.sav` in the authorized isolated `original-blackbox-20261023/Game-ddraw` root. The final App lives at `<local-task-output>/emperor-native-playable-v1/EmperorApp.app`. The local `stage35/Launch Native v1.command` supplies this root; the older README launch example points at the user's original installation for general browsing and may not contain this newly created test source.

Using the coordinate fields and normal Build control, road(77,57) costs2 and house(78,57) costs15. The visible balance changes2000→1998→1983. Repeating the house placement refuses without adding an object or cost. Resume reaches tick4/population2; Pause holds both state and the captured canvas pixel hash across the observation interval. Resume again reaches tick7/population3. Save writes a new `.encity`; a second same-path Save refuses and preserves its bytes. These controls use the same transaction path as ticks.

The first App exits0. A new process loads the same original source and the saved native checkpoint, restoring tick7/population3/funds1983, one road and one house. At the same camera, the restored canvas pixel hash exactly matches the saved session's view **1/1**. Resume reaches tick11/population5 and a second uniquely named checkpoint saves successfully; the first remains unchanged. The UI also rejects populated original SAV creation **1/1** and an authored unknown-version native session **1/1** without replacing the restored city. Closing the native window leaves the strict executor available; its normal Play still refuses update1 with N0 and `event_ui_5eff20_not_closed` **1/1**. A second normal menu quit exits0; **2/2** test instances are gone, with no Wine game launched.

Direct canvas mouse automation did not change coordinates or state in this run. Foreground checks exposed activation loss between separate tool calls; even a combined focus/targeted-event attempt and one session-event attempt remained ineffective. This is **not** a canvas mouse pass and the exact cause remains unresolved. The source contains a mouse handler and its projection unit checks pass, but neither fact replaces successful GUI input. The coordinate fields plus Build button are the verified playable construction route. No Dock access, permission changes or repeated permission prompts were used. One unsupported-source alert exposed duplicate accessibility titles; the existing unique button identifier closed it successfully.

The package matches115/115 recorded source hashes and29/29 compiled units. Each Release/UBSan test binary binds37/37 compiler-derived project dependencies. App/GUI logs, source/test hashes, session files and captures stay in local stage35 evidence. No assets or generated session data are committed, uploaded or bundled. All prior source changes remain; the old housing diagnostic runner and strict executor sources are unchanged in this stage. One tool approval rejected an ambiguous runner-creation command before execution; an explicitly separate new runner was subsequently approved, preserving the old runner hash. An initial compile error in the new source adapter was corrected before both final regression runs.

Persistence boundary: the file is fsynced before no-replace publication, but the parent directory is not fsynced, so this is not a power-loss durability guarantee. Data-root path rejection is a best-effort application check with a check-then-use interval, not a filesystem security sandbox. The format validates local state consistency, not authorship or tamper resistance. These limits do not change the tested no-overwrite and failed-load replacement protections.


### Controlled GUI failure checks

A third instance uses a newly copied, explicitly enumerated nine-file asset root; original inputs remain untouched. With the native checkpoint restored at tick7/population3/funds1983, a300-character destination filename causes actual temporary-file creation failure (no disk filling or permission changes). The App reports failure and preserves its paused city fields **1/1**.

Only the copy's `China_General.555` is then recoverably moved aside. A valid new house command reaches drawing preparation but fails decoding; funds, object counts and tick remain unchanged **1/1**. Resume attempts the next residency tick, which likewise fails drawing before installation: tick7/population3/funds1983 remains, and a separate explicit Pause transaction stops the timer **1/1**. Resume/Pause legitimately advances the command revision; this is not a claim that no control command occurred. A further observation interval confirms no repeated advancement. A `finally` cleanup restores the exact asset hash; Resume then reaches tick9/population4 and saves a new checkpoint **1/1**. The third App exits0, making normal test exits **3/3**. No production fault-injection flag or altered original data was introduced.


## Direct map interaction acceptance (2026-10-24)

The native canvas now owns first-click/focus handling, logical-point tracking and a single aspect-fit image transform. The shared inverse40/20 projection checks finite camera/pointer values before integer conversion. Rendering and preview use the same camera and image rectangle; physical backing pixels do not enter picking. A resized window retains the image aspect ratio and rejects its dark margins. Preview copies the current native state and runs the existing atomic transaction; it never changes the ledger, objects, ticks or revisions. Green reports command eligibility, not a guarantee that a future resource decode cannot fail. Click revalidates against current state and retains render-before-commit. Houses preview four tiles. Paused construction is explicitly allowed.

Choose Road or House2×2, hover and click. Right-click/Esc (map focus), or Browse cancels. Option-drag or Browse-drag pans; wheel/± zooms; map-focus arrow keys pan. Before the road-drag increment below, dragging an active tool without Option did not build. Camera changes are view-only and are not added to ENCITY. Coordinate entry remains optional auxiliary input. Original assets, the source/model binding, native rule version, strict executor and EMP1 are unchanged.

The earlier mouse acceptance failure was investigated rather than counted as a pass: a minimal instance of the old image-view subclass passes Cocoa hit testing1/1. On the new real App, a PID-directed hover still produced no hover state0/1; explicitly raising the exact window, verifying that it owns the pointer's topmost window, then posting a normal HID mouse event produced the expected hover1/1. This demonstrates a focus/event-delivery problem in the previous validation route; it does not isolate a single OS cause or prove the old projection was defective. No Dock target, permission prompt, security change, or App state injection was used. The accepted route uses normal visible GUI input with preflight checks, exact PID/executable/window identity and immediate readback.

Real-data GUI results: **3/3** view/layout cases pass (900×768 at1×zoom; same window at2×zoom; moved/resized1100×806 at2×zoom after80 logical-point Option-pan). Captures are2×Retina backing resolution. **5/5** successful map clicks create roads(77,57),(77,58),(77,59) and houses(78,57),(78,59), debiting36 from2000 to1964. No coordinate field was written for these constructions. Green and red footprints are visible in captured images. An occupied click refuses without debit/object changes; right-click and Esc cancel2/2; toolbar and letterbox clicks do not build2/2. Wheel and map-focus arrow keys visibly change only the camera. Current GUI does not separately exhaust funds; that rejection and preview are covered by authored tests.

A first attempt to click after the Option-pan had no state effect. The driver was corrected to explicitly set ordinary event modifier flags to0 and clear Option on release; the logged zero-flag click then created exactly the expected cell. Later inspection reported no held modifiers. This was a validation-driver correction, not an inference that every previous failed event had the same cause. One requested window size was constrained by macOS; the test used the observed806-point height, not the requested840. A JSON serialization error happened after successful resize; the window was inspected before continuing and was not resized again. An unmatched quit title was rejected by the helper before any action; the observed unique terminate identifier then exited normally.

The mouse-built city reaches tick3/population2; Pause holds state across a2.2-second observation and saves a new `.encity`. A new process restores tick3/population2/funds1964,3 roads and2 houses, resumes to tick6/population6, and saves a second new file. Old checkpoint bytes remain unchanged. A repeated save reports the existing-file refusal. The previous package's v1 checkpoint also loads1/1. Both test processes quit normally, exit0 **2/2**. No original game is started.

Release and UBSan each pass **4,671/4,671** authored assertions (including scaled/letterboxed/resized/Retina logical-point transforms, diamond boundaries, pan/zoom anchors, invalid cameras and preview nonmutation), plus **1/1** real source with original sprite binding and file roundtrip. Each binary binds38 compiler-derived project dependencies. The package binds117 source files and29 compiled units. These tests do not claim original mission compatibility or remove the retained strict0/6 first-update blocker. Local stage36 contains the input receipts, captures, checkpoints and source/test/binary hash bindings; these remain outside Git.


## Atomic drag-road construction (2026-10-24)

A RoadPath command extends only the separate native-v1 command API. A path includes both endpoints, proceeds along X until the final X and then along Y; it is not pathfinding and does not route around obstacles. The preview is recalculated from the current endpoints, so moving back does not retain previously traversed cells. Zero length is one cell. Existing native or source roads are blue, retained and free; new eligible cells are green. Any unavailable terrain/house, out-of-bounds endpoint, insufficient total funds or object limit rejects the entire path. Red is a rejected proposal, not a partially buildable path. An entirely existing path has zero cost and returns a named no-op without a new revision.

The plan computes every cell and total cost without mutation. The command checks the revision, builds one candidate state with all new road IDs and the full debit, validates it, then the App prepares/renders that candidate before publishing. One successful path increments the revision once; no intermediary construction occurs during mouse movement. Preview uses current state and release revalidates it. The native single-thread update/command path owns publication; ordinary tick updates are not represented as a road commit. Pause permits construction as before.

Press and drag the Road tool; release submits the current complete preview. Esc/right-click cancels and selects Browse. Leaving the image (including toolbar/margins), leaving its window, invalid grid picking, or losing key-window status cancels a held gesture. Returning with the same press does not resurrect it. Tool switches, loads/saves, explicit camera changes and zoom also discard pending construction. Option-drag/Browse-drag remains camera-only; houses remain single-click. No demolition, map editing or original SAV writes are added. Pending gestures and camera state are not serialized.

ENCITY remains version1 with the same rules identifier and normal-model road price. RoadPath saves ordinary native road objects under their existing type22/monotonic identity representation. There is no new serialized enum or required field, no new migration rule and no change to treasury validation. The prior v1 checkpoint is accepted by the new App1/1.

Release and UBSan each pass **106,113/106,113** assertions. **4,096/4,096** ordered endpoint pairs on an8×8 authored domain check inclusive cardinal paths, fixed bend, unique cells, pure cost preview, full debit and one revision. Additional cases cover source/native roads free, blocked middle cell/house, exact and one-short budgets, negative/extreme/outside endpoints, stale revision, zero length, backtracking, repeated cancellation and no resurrection after leaving. The retained projection tests cover zoom/resize/Retina logical coordinates. Real source **1/1** per mode constructs3 roads in one command and a neighboring house, debits2000→1979 using original Normal prices, fills original capacity7, binds the house's original artwork and roundtrips the same independent format. Unset data root still reports successful SKIP. Each final test binary binds38 project dependencies.

Real GUI input uses the established exact-window/HID route. Default view builds a3-cell vertical road, then an X-first L path with1 existing/3 new cells. Each captured pre-release preview retains the previous money/objects, and release applies the whole debit. Repeating the original road charges0. A single house click succeeds. A path crossing that house displays the entire rejected proposal and changes nothing. Esc,right-click,leaving through UI then returning,and leaving the panel pass **4/4** cancellation cases. Backtracking from an extended preview commits only the final1 new/1 existing path. A zero-length existing proposal costs0; a zero-length new proposal adds one road. No coordinate-field writes are used for construction.

A second layout moves/resizes to1100×806, uses2×zoom and Option-pan to camera77.54,58.46. The pointer selects exactly(80,59)→(80,61); preview shows2 new/1 existing,4 gold; only the two new cells appear. Both default/transformed layouts pass **2/2**, with2×Retina captures. The five successful road gestures plus one house produce10 roads and1 house, funds1965. A native construction checkpoint records revision6 and all11 exact identities/positions, proving cancelled/rejected/no-op inputs added no revisions or objects. Source roads remain read-only; the real scenario tested here starts empty of buildings.

Resume reaches tick3/population1, then Pause retains state over2.1seconds and saves. Both first-instance checkpoints remain intact. A fresh process loads the previous stage's v1 save, then the drag-road save, restoring all11 identities/positions and funds1965. Resume reaches tick6/population3; a new save succeeds and repeating that destination preserves its exact bytes **1/1**. Two test processes exit0 normally **2/2**. No original game is launched, and no permissions/settings are changed. The current package binds117 source files and29 compiled units; local stage37 keeps full source/test/binary/input and GUI-evidence hashes outside Git. Original strict0/6 simulation is retained, not bypassed or remeasured. Funds exhaustion, source-road traversal and extreme endpoints were covered with authored tests rather than claimed as real GUI cases.


## Native v2 water service

The new normal entry creates `original-data-native-v2`; decoded v1 sessions keep v1 behavior. V2 uses original model type72/Well column0 at Normal difficulty (observed25) and the existing named2×2 Well SG3 binding. The native drawing adapter deliberately selects the basic Well visual; it does not infer an original saved service or upgrade byte. No original service capacity is inferred from the remaining model columns.

A well activates when any cell on its orthogonal2×2 perimeter touches a source/native road. Its coverage is a clipped10×10 square: the2×2 footprint extended4 cells in each direction. Coverage from multiple active wells is unioned, with no quota, depletion, walkers, city-gate connectivity or labor requirement. A house is watered only when all4 footprint cells are covered. Immigration still occurs at even native ticks, but requires both full water coverage and the house's own adjacent road. Dry houses stop increasing; existing residents are retained. These are visible authored v2 rules, not a claim of original gameplay compatibility.

Well placement uses the same revision-checked atomic transaction and render-before-commit as other construction. It occupies2×2 cells, cannot overlap roads/buildings and is also an obstacle to whole-road commands. Its price participates in the exact construction ledger. New wells may be built while paused; road additions activate service immediately without changing ticks. The App shows active/total wells, wet/dry housing and toggleable coverage, with hover-specific status.

V2 uses the same object/command identities and limits, with `kind72` representing a native-owned well and requiring residents0. Its header and seed line differ explicitly:

```
ENCITY 2 original-data-native-v2
"relative/source.sav" sourceRevision modelRevision
activeSide initialFunds roadCost houseCost houseCapacity wellCost waterRadius
revision ticks nextId paused funds objectCount
id kind x y residents
...
```

`waterRadius` must equal4 and `wellCost` must match the loaded model. Version/rules disagreement, missing well price, unknown versions and inconsistent ledger/objects refuse loading atomically. V1 serialization remains byte-for-byte unchanged and never requires the well model; v1 immigration ignores water and well commands refuse. The App labels the loaded version and disables its well tool for v1. No migration is implicit.

Stage38 baseline: native water gameplay **0/1**, real Well projection in the native session **0/1**; implementation and acceptance results are recorded below only after their corresponding tests complete. Prior stage35–37 deliveries and evidence remain preserved.


### Water v2 acceptance (2026-10-24)

Final package: `<local-task-output>/emperor-native-water-v2/EmperorApp.app`; local `stage38/Launch Native Water v2.command` supplies the authorized test root. Binary SHA-256 is `cd52f62527af7e6acd759f266a602e5cb71f2e23a2c48a37fe5a797916126c45`. The package matches117/117 production-source hashes and29 compiled units. Each Release/UBSan binary binds38 compiler-derived project dependencies. Both modes now pass170,912 authored assertions, including the84 initial water assertions, additional bottom-right boundary checks across7 map sizes, and the retained4,096 road endpoint pairs. Unset-data SKIP remains successful. The1/1 real source per mode exercises v1 unchanged and v2 dry→watered behavior, model25 well price,2/2 original house/Well bindings and exact roundtrip. These are native-rule results.

The water GUI loop improves **0/1→1/1** on the specified real empty tutorial source. Actual map clicks place road(77,57),house(78,57) and well(78,59). The road-adjacent dry house remains population0 after3 ticks. The disconnected well is shown inactive and costs25; an overlapping well click refuses without debit or another object. Dragging road(77,57)→(77,59) previews1 existing/free cell plus2 new cells,total4, then commits once. The well becomes active1/1 and the house watered1/1. Funds are1954 from2000; after3 further ticks the house has2 residents. Pause holds tick6/pop2. Coverage toggles without changing state;1× and2×zoom show the same well identity/status. Captures use2×Retina backing resolution.

`water-city-final.encity` records revision14,tick6,nextId6,paused1,funds1954 and5 objects (3 roads,1 house,1 well); its SHA-256 is `02ec1198cd73383097e069af967129e32566337dd654026a366d850709eeaac3`. A repeated Save refuses without replacing it. A new process restores these fields and identities **1/1**, recomputes active/wet service1/1, resumes to tick8/pop3 and writes a uniquely named continued checkpoint. The same process also loads the previous stage37 v1 checkpoint **1/1**: version1 remains visible, Well is disabled and population grows1→2 without a well. Neither old checkpoint changes.

An initial GUI check found Cocoa menu auto-validation re-enabled the visually disabled Well option in v1. The core transaction already refused v1 Well commands; no invalid city was committed. The tool menu now disables automatic item enabling, allowing the current rules version to govern availability. The preliminary bundle and failed evidence are preserved separately. The final rebuilt package passes the actual disabled-menu check and both version loops. No core/model changes occurred after the passing Release/UBSan runs; final source bindings verify those exact compiled dependencies.

The final package additionally confirms actual window-focus cancellation **1/1**: while a valid road preview is held, the exact same App's browser window is raised and `AXFocusedWindow` reports that browser. The pending gesture disappears before release; release adds no road or charge. A separate9-file data copy then supports controlled drawing failure. Only its general555 pixel file is recoverably held aside; a valid3-cell,6-gold road gesture fails decoding. All serialized city bytes,including revision,ticks,objects and funds, remain exactly equal to the loaded checkpoint **1/1**. The paused state remains stable across a further observation. A `finally` path restores the original copy hash, and the same road then succeeds **1/1**, funds1954→1948. The original data root is never changed. An unknown-version checkpoint also refuses without replacing the city **1/1**.

Final-package test instances2770,3118,3531 and5079 all exit0 **4/4** by the normal menu; no App or original-game process remains. Two earlier preview-package instances also exited0 but are not included in final-package counts. Current movie/Cmd+Q was not retested; previous results remain version-specific. All9 consumed source/model/art files and the restored9-file fault copy match their recorded input hashes. Stage35–37 App binaries remain unchanged. Local stage38 retains complete source/test/object/binary hashes, GUI receipts,captures and checkpoints outside Git. No asset upload,commit,push,original-game launch or security-setting change occurs. Strict original execution remains at the retained0/6 first-update event guard; native v2 does not alter or bypass it.


Supplemental review closure: bottom-right coverage passes **7/7** authored map sizes (3,4,5,6,8,112,228). Each checks the clipped square cell-by-cell, expected clipped count, fully covered nearby housing, a partial house across the coverage edge where applicable, and evaluation purity. Right/bottom clipping does not wrap to the opposite edge. These synthetic sizes add no new real-source coverage; the real source remains1/1. Production code and the signed App binary are unchanged, so no rebuild was needed; the extended Release/UBSan test binaries are separately hash-bound.

A fourth final-package GUI instance constructs one road,one dry house and one disconnected well, paused at tick3/pop0/funds1958. While only the independent copy's general555 file is recoverably held aside, a road extension previews2 new cells+1 existing cell,total4. This is exactly the connection that would activate the well. Rendering fails, leaving funds1958,3 object identities,revision,ticks and all serialized bytes unchanged. The well remains inactive0/1 and the house dry0/1. With the same Browse tool,pointer,camera and overlay setting, before/after canvas captures match **1/1** exactly (1,506,848/1,506,848 pixels); no candidate water coverage leaks into the displayed state. After byte-exact asset restoration,the same connection succeeds for4 gold, well1/1 and watered house1/1, and resumed immigration reaches1 resident. A new checkpoint saves; the instance exits0. Both independent drawing-failure scenarios now pass2/2, with input preservation and4/4 final-package exits rechecked. V1 byte compatibility and v2's stop-growth-without-eviction rule remain unchanged.


## Native v3 single-food loop

Stage39 baseline is native food GUI **0/1**. V3 reuses the existing named type33 Hunter base binding (`China_Husbandry.bmp` within the original General archive),2×2 footprint and user's Normal model construction cost (observed32). The original model calls it Hunter's Tent. Only this food-production building and one food,meat,are added. Its rate,shared stock,distribution and consumption below are authored native rules; no original labor,hunting figures,prey,agricultural calendar,market or transport behavior is inferred.

Every even tick processes one meal as an atomic candidate transaction. Each road-adjacent Hunter offers4 units while the global production switch is on. Up to the free room in the city-wide100-unit stock is actually produced; excess is stopped rather than silently counted as produced/discarded. Next,the existing resident count demands1 unit per resident, consuming the lesser of stock and demand. A shortfall prevents all new arrivals that cycle but removes no residents. If existing demand is fully fed,eligible houses receive at most1 newcomer each in ascending native identity order,spending1 extra meat per newcomer. Eligibility retains the adjacent-road and fully watered2×2 requirement and original model house capacity. This is direct shared-city distribution,not road-network transport. Production adjacency does not require an animal/field resource or city-gate route. Pausing hunting leaves consumption running; pausing the city stops all tick activity. No price,recurring charge or food sale is invented.

The pure meal planner returns proposed production,consumption,newcomer identities and remaining stock without modifying its input. The ordinary transaction commits the entire plan plus tick/population together. Lifetime actual production minus lifetime consumption must equal stock; consumption includes existing residents and newcomer rations. Bounds,nonnegative stock,capacity,receipt consistency and a rate-derived lifetime limit are validated before publication or load. The App still renders any candidate whose population/buildings change before committing its complete state; a drawing failure cannot partially apply its food ledger or construction debit.

New sessions use `original-data-native-v3`. V1/v2 serialization remains unchanged,without new food fields; their decoded state has no food activity and their producer/control commands refuse. V3 uses:

```
ENCITY 3 original-data-native-v3
"relative/source.sav" sourceRevision modelRevision
activeSide initialFunds roadCost houseCost houseCapacity wellCost waterRadius hunterCost
revision ticks nextId paused funds objectCount
FOOD meat 4 100 huntingEnabled stock lifetimeProduced lifetimeConsumed lastProduced lastDemand lastFed lastArrivals
id kind x y residents
...
```

Kind33 is the native Hunter's Tent. Like a well,it occupies2×2 and cannot contain residents. The food identity,yield4,capacity100,well radius4 and model costs must match this rule version. The last receipt remains visible through odd ticks and construction/control commands. Saving/reloading preserves production enablement,stock,ledger,receipt and next-cycle behavior; there is no offline catch-up. Unknown versions,food identities,rule parameters or inconsistent ledgers refuse before replacing the current session. Old versions reject nondefault food state rather than silently discarding it. Native v3 is a separate playable subset,not original SAV continuation; strict event guards stay unchanged.


### Food v3 acceptance (2026-10-24)

The native food loop improves **0/1→1/1** on the same single real empty SAV19 source. Final App: `<local-task-output>/emperor-native-food-v3/EmperorApp.app`; `stage39/Launch Native Food v3.command` supplies the approved external root. Binary SHA-256: `cb3a5b4b32260461ab288156b0b9f8e1838c9693c47431e7e7b3217264af0e20`. Production source correspondence is117/117 with29 compiled units and a verified strict signature. Release and UBSan each pass171,878 authored assertions,including966 new food checks; each binary binds38 compiler-derived project dependencies. Tests cover pure planning,production adjacency,consumption-before-arrival,stable-ID ration order,stock saturation/one-unit capacity,conservation,overflow/corrupt records,paused ticks,shortage without eviction,recovery and exact old-version serialization. Unset-data SKIP remains successful. The real source passes1/1 per mode with original construction prices and3/3 house/Well/Hunter bindings; this does not establish original hunting/gameplay equivalence.

Actual GUI uses the nearest verified5×5 flat area around house(82,56),well(82,58),Hunter(84,58). Camera fields select that view; construction itself uses map clicks and two visible road drags. Eight roads cost16,the house15,well25,Hunter32:2000→1912. Before the producer is built,a fully watered adjacent-road house stays population0 after2 updates. A second click on the Hunter refuses overlap without debit or another object. Production enabled then reaches tick6,pop2,lifetime produced8/consumed3,stock5. Pausing production allows consumption to continue: tick10,pop3,stock0,one resident meal unmet and no newcomer; a further meal still keeps3 residents. Resuming production reaches tick14,pop4,produced12/consumed12,stock0. The city-wide Pause holds tick,population and food counters through the observation interval. A new save succeeds and repeated same-path Save refuses.

`food-city.encity` records revision31,tick14,nextId12,paused1,funds1912,11 native objects and the complete food receipt. SHA-256: `3cd12858e4c6eed0651502a51f56f5a4976cab6d0141f1e855bb09264a801712`. A separate production-disabled checkpoint is also saved while starved. A new App process reloads the disabled checkpoint,keeps hunting off,pop3 and produced8/consumed8 after another meal **1/1**. Loading the enabled checkpoint restores all11 identities/positions and food state; continued operation reaches tick16,produced16/consumed16,pop4 **1/1**. A uniquely named continued save is written. The one producer exactly feeds these4 residents that cycle; lack of surplus is not misreported as a stalled timer.

The same new process checks old v1 and v2 checkpoints **2/2**: immediate new-file saves match their existing bytes exactly; the Hunter menu is disabled in both and Well remains available only in v2. V1 continues population1→2 without water/food; v2 continues2→3 with water but no food. Old versions explicitly display that they do not run the food subsystem. An authored stock101/ledger mismatch checkpoint refuses before replacing the current tick,population or food counters **1/1**. All prior checkpoint bytes remain unchanged.

A third final-package instance uses a new9-file asset copy,with the original root unchanged. It loads the production-disabled checkpoint,advances one successful non-meal tick to13,then enables hunting and saves a baseline. Only the copy's general555 pixels are held aside inside a guaranteed restore scope. A legal new Hunter placement fails drawing without a32-gold debit or object **1/1**. Resume then attempts the next meal: the candidate would produce4,feed3 and admit1,but drawing fails,so none of its stock,ledger,population or tick14 commits **1/1**. The controller explicitly pauses; the only revision changes are the successful Resume and automatic Pause (+2),not a claimed unchanged control history. All other serialized fields and objects match the baseline exactly. Paused state stays stable; before/after canvas and overlays match1,506,848/1,506,848 pixels. Byte-exact asset restoration allows the same next meal to succeed (tick14,pop4,12/12 ledger),then save normally **1/1**. No production fault flag or altered original asset is introduced.

All3 final-package test instances (8721,9299,9969) exit0 through the normal menu **3/3**. No original game was launched. All9 consumed input files and the restored9-file fault copy remain hash-identical; all4 previously delivered native packages remain unchanged. Current movie/Cmd+Q is not retested and retained strict original progression stays0/6 with no modified/zero-filled gate. Local stage39 binds the exact source,compiler dependencies,objects,binaries,GUI receipts and checkpoints; no original assets/derivatives are committed,uploaded or bundled. Existing work remains reviewable and unpushed.


## Native v4 demolition and rebuilding

Stage40 baseline: native-owned demolition/rebuild GUI **0/1**. V1/v2/v3 are preserved byte-for-byte and do not gain demolition commands. V4 keeps the v3 food/water rules and adds paused,identity-targeted demolition with no refund. The preview reports the exact footprint,residents removed,active wells,watered and road-adjacent houses,and producing camps before/after. Selection never changes state. Confirmation carries both the native object ID and expected revision; stale,missing,original-source or running-state targets refuse. Cancel and focus loss discard the pending selection.

Active object IDs stay ordered and never change. Retired ID/kind pairs preserve spent construction prices and prevent identity reuse; no original SAV record is deleted. Current original projection uses slots1–3999,so active plus retired creations remain capped at3999; rebuilding does not reset this lifetime limit. Funds equal source treasury minus the model cost of active and retired objects; demolition itself has zero debit and zero refund. Removing a house removes its inhabitants,while removing access/water stops surviving-house growth without eviction. Removed camp capacity stops production,but stored city-wide meat is retained. The prior meal receipt is historical: a separate `departedSinceMeal` count reconciles it with current population until the next committed meal clears that count. Food production-minus-consumption still equals stock.

V4 format is `ENCITY 4 original-data-native-v4`. Seed and ordinary state records match v3. Its FOOD line appends `departedSinceMeal` after `lastArrivals`. Live-object records follow as before; then `RETIRED count` and ordered `id kind` pairs account for demolished objects. Active/retired identities must form a unique complete1..nextId-1 partition. Unknown types,duplicates,holes,bad counts,balance or food-history inconsistencies refuse loading atomically. There is no implicit old-file upgrade. The UI recomputes projection and service overlay from the candidate and publishes only after successful drawing; failed demolition retains the complete old state,including revision,stock,population and money.


### Demolition v4 acceptance (2026-10-24)

Native-owned demolition/rebuild GUI improves **0/1→1/1** on the same verified real empty SAV19. Final App is `<local-task-output>/emperor-native-demolition-v4/EmperorApp.app`; `stage40/Launch Native Rebuild v4.command` supplies the external read-only root. Its binary SHA-256 is `c81d8d94ca9b31d84b1707021c63037d1e7af352d57d560583fedf683e4fb969`;117/117 production-source hashes and29 compiled units match. Release/UBSan each pass172,161 assertions,including283 demolition checks and unchanged966 food checks. Each binds38 compiler-derived dependencies. Both modes pass1/1 real source,plus successful unset-env SKIP. A first authored road-disconnection fixture accidentally left a second adjacent road; its coordinates were corrected to isolate the intended dependency before the final passing suites. No production behavior was changed to satisfy that assertion.

The real-data test retires all11 native objects in a food city without refund or food loss. The resulting terrain flags and ground-owner cells exactly equal the original source **1/1**. A rebuilt house receives ID12,pays the original15-gold price,and renders using the original archive **1/1**. Retirement history and last-meal departures roundtrip exactly. These are native-rule checks; they do not establish original demolition/refund semantics.

Actual GUI starts by clicking a visible original rocky area with demolition selected: no native object is selected and confirmation remains disabled **1/1**. Three native roads at(81,56),(81,58),(85,57),house(82,56),well(82,58),camp(84,58) cost78,total funds1922. At tick4,pop2,stock5,production8/consumption3,a click on a different cell of the same2×2 house selects the same ID4. The red full-footprint preview explicitly says no refund and2 residents removed. Clicking Cancel leaves checkpoint bytes exactly equal; Escape also clears selection without state change **2/2**.

Removing the sole adjacent well road (#2) previews active wells1→0,watered houses1→0. Confirmation leaves funds1922; tick6 retains2 residents,with no new arrivals. Rebuilding that road costs2 and creates ID7. Removing the camp's only road (#3) previews producing camps1→0; the next meal consumes existing stock without increasing lifetime production. Rebuilding it creates ID8 and costs2. Removing/rebuilding the well (#5→#9) and camp (#6→#10) works through separate previews and confirmations. Lastly the populated house preview shows3 residents departing; confirmation changes population3→0,leaves stock4 and funds1861,and preserves the historical receipt via departedSinceMeal3. Its replacement is ID11 and costs15. At tick10,population1,stock7,production16/consumption9,funds1846. Four object kinds pass GUI removal/rebuild **4/4**; service-road loss passes **2/2**.

`rebuilt-city.encity` SHA-256 `ceebe323fb629b8cbc2a30578a5f8456078545a9a3fb1d8278d709d370990ce1` records revision34,tick10,nextId12,6 active objects,5 retired identities,funds1846. A new process reads both the removed-house checkpoint and rebuilt checkpoint and writes exact same bytes to new paths **2/2**. Continuing the rebuilt city reaches tick12,pop2,stock9,lifetime20/11 and writes `rebuilt-city-continued.encity`. The same process loads v1,v2,v3 historical checkpoints and saves identical bytes **3/3**; the actual demolition menu item remains disabled in all3. A duplicate retired/live ID checkpoint refuses without changing the current food/population/counters **1/1**. A pending selected house is then cancelled by raising the exact same App's browser; AXFocusedWindow verifies the browser,confirmation becomes disabled and state remains unchanged **1/1**.

A third process uses only a new9-file test copy. While the copy's General555 pixels are recoverably held aside,confirming removal of the populated house and confirming removal of the well's sole road each fail drawing **2/2**. The saved states match `before-demolition.encity` byte-for-byte,including revision,money,food,objects and retirement history. With the same pointer,camera and Browse mode,the before/after canvas and water overlay match1,506,848/1,506,848 pixels. The timer remains paused. The held file is restored byte-exact in a guaranteed cleanup block; the same house removal then succeeds **1/1** and saves a new checkpoint. The original asset root is unchanged.

All3 final-package instances(13545,14483,15088) exit0 through the normal menu; no original game is launched. Current movie/Cmd+Q is not retested. All9 consumed original inputs and fault-copy inputs are preserved; all5 earlier delivered native App binaries remain unchanged. Stage40 changes9 source/doc/test files and preserves the other22 already-dirty files. Local stage40 holds source/test/object/binary hashes,GUI receipts and screenshots outside Git. No assets or derivatives are committed/uploaded; no commit or push occurs. Strict original event admission remains at the retained0/6 result and is not bypassed. Only the supported empty-source native v4 session gains demolition; original/nonempty cities remain unsupported.


## Native v5 single-source income

Stage41 baseline: income-to-construction real GUI **0/1**. Model row125 is named Tax Office; its base model construction value40 is visible in the user's data. The existing original static-base selector has no qualified type125 image/footprint binding,so no building is guessed. V5 instead uses a city-wide authored1-gold tax per adjacent-road resident every4 native ticks. Water/food are still needed for new residents according to v3 rules,but existing road-connected residents pay regardless of current water/food status. This is a deliberately limited native economic rule,no original taxation,labor or walker equivalence.

At each due update the pure tax planner assesses the **pre-update** resident population. Disconnected residents and empty houses are counted explicitly as nonpaying; empty cities still complete a zero-income period. Candidate income and receipt are applied before the same update's meal. Income,food,population,tick and cycle count commit together only after all validation and any required drawing succeed. Treasury is bounded to signed32-bit positive range; insufficient headroom refuses the complete update. Normal GUI failure handling then pauses. No income is generated by display refresh,load,wall-clock delay or paused ticks.

V5 retains v4 costs/retired IDs and defines `funds = source initialFunds + taxReceived - all active/retired construction costs`. All cumulative amounts are bounded before signed arithmetic. `taxCycles == ticks / 4` identifies the last completed period; receipts are historical,so later construction/demolition does not rewrite past collections. Rebuilding costs can now exceed the initial treasury over time if actual income funded them. The existing3999 total-created ID limit still applies and is shown explicitly; it is not reset by demolition.

V5 uses `ENCITY 5 original-data-native-v5`. The seed,ordinary state,FOOD (including departures),live objects and RETIRED records retain v4 structure. After FOOD,before live objects,one mandatory line is added: `TAX 4 1 completedCycles lifetimeReceived lastEligibleResidents lastDisconnectedResidents lastEmptyHouses lastIncome`. Unknown cadence/rate,missing/duplicate/unbounded receipts,inconsistent funds or cycle count refuse loading atomically. V1–v4 contain no TAX line and reject hidden nonzero tax state; loading never upgrades them. Food and tax have no deferred offline work.


### Income v5 acceptance (2026-10-24)

The income/construction GUI loop improves **0/1→1/1** on the single verified complete empty SAV19. Final App: `<local-task-output>/emperor-native-tax-v5/EmperorApp.app`; `stage41/Launch Native Economy v5.command` supplies the external read-only test root. Binary SHA-256: `ca2a63a43893447ed4b846e9c84dacd8a174e81b24f829d70027315a908b58f3`. Production source correspondence is117/117 with29 compiled units and a verified strict signature. Release/UBSan each pass172,876 assertions,including715 tax/departure checks,283 demolition checks and966 food checks;38 compiler-derived dependencies bind each test binary. Each mode passes1/1 real source across all5 versions and the unset-root SKIP. Original house/Well/Hunter rendering remains3/3; no original economic-equivalence metric increases.

The authored tax checks prove pure assessment,pre-update population selection,zero-income receipts,road removal/restoration,house removal,paused inactivity,cross-period roundtrip and200-update ledger conservation. A low-budget fixture has1 gold after construction and cannot buy a2-gold road; its first1-gold tax makes the road affordable. Treasury exactly at the signed32-bit cap refuses an additional assessment atomically; exact headroom succeeds. Invalid rules,unbounded receipts,cycle mismatches and hidden old-version income refuse. The first near-cap fixture omitted an extra road's prior cost; that authored fixture was corrected using the actual spent total before both final suites passed. Production code was not weakened. Two populated houses are removed consecutively: departedSinceMeal2→4 reconciles each historical receipt,stays4 across the odd tick,and clears on the next meal without food loss or duplicate consumers.

Actual GUI starts with three roads,house,well and Hunter built from the original model costs:2000→1922. At tick3,pop1,stock3,produced4/consumed1,income0,the paused checkpoint is saved. Tick4 assesses the existing1 resident before its new arrival:funds1923,pop2,stock5,produced8/consumed3,income1/cycle1. A new road through the normal coordinate/build controls costs2,funds1921. Pausing holds every counter and treasury; a tick7 checkpoint records pop3,stock6,produced12/consumed6 with the same cumulative income1.

A new process loads the paid checkpoint and saves identical bytes,then loads tick7 and continues across tick8. Exactly3 gold is added:funds1924,pop4,stock6,produced16/consumed10,cumulative income4/cycle2. Reloading this paid checkpoint and waiting while paused does not pay again; its new-file roundtrip is exact **2/2** paid checkpoints. Removing the house's sole adjacent road through map selection and separate confirmation immediately reports4 disconnected nonpayers. Tick12 closes the next period with0 income and unchanged funds1924;population4 remains,food24/18,stock6. Rebuilding that road costs2;tick16 assesses5 existing residents and reaches funds1927,pop6,food32/29,stock3,cumulative income9/cycle4. Thus loss/restoration of the tax dependency passes **2/2**. The resulting city is saved independently. An inconsistent TAX cycle record refuses `invalid_tax_ledger` without replacing any current city state **1/1**.

Historical v1/v2/v3/v4 checkpoints load in the same final App and save byte-exact **4/4**. Each explicitly reports that taxes are unavailable and no upgrade occurs. Selecting a v5 house for demolition,then loading the v4 checkpoint,clears the pending selection and disables confirmation; the loaded v4 state saves byte-exact **1/1**. Reloading v5 also retains no stale selection. This is the requested load-before-confirm case; source switching was not separately repeated. The lifetime3999 creation limit (including retired IDs) retains core refusal tests and now has explicit GUI error/preview text; creating3999 objects in GUI was not attempted.

The fault instance uses a separate9-file asset copy. After loading the tick3 checkpoint,only the copy's General555 pixels are recoverably held aside. Resume attempts tick4,which would collect1 gold,serve food and add a resident. Drawing fails and the controller pauses:tick3,pop1,funds1922,stock3,produced4/consumed1,income0/cycles0 remain intact. Every serialized field except control revision matches the baseline; revision+2 records the successful Resume and automatic Pause. The same pointer/camera/Browse view compares1,506,848/1,506,848 equal canvas/overlay pixels. Asset restoration is byte-exact in a guaranteed cleanup block. Resuming then commits the same tax/meal once:tick4,pop2,funds1923,income1/cycle1 **1/1**. The original root remains unchanged.

Two early first-click no-ops occurred while other desktop activity was present; receipts showed neither construction nor debit. One later click was stopped before posting because another window covered its target. Those interrupted attempts were retained as diagnostics,not counted as completed loops; their individual causes were not all proved. After the user reserved the desktop,the existing paused city continued with verified mouse construction and the full tax sequence passed. No production behavior or guard was changed to bypass a failed GUI observation. All4 final-package test instances(18042,19086,20314,20848),including the preliminary diagnostic instance,exit0 via the normal menu. No original game was started; current movie/Cmd+Q was not retested.

All9 consumed original inputs and restored9-file fault inputs remain hash-identical;6 prior delivered native packages are preserved. Stage41 changes8 source/doc/test files while preserving23 other dirty files. Local `stage41/source-test-bindings.json` binds source,compiler dependencies,objects,test/App binaries,GUI receipts and checkpoints; these local derived artifacts stay outside Git. No commit,push,asset upload or original SAV modification occurs. Strict original update admission remains at retained0/6 and is unchanged. ENCITY validation is consistency checking,not tamper-proofing; existing local path checks are not a security sandbox,and saves do not promise power-loss durability of the parent directory.


## Native v5 entry and checkpoint lifecycle

Stage42 changes the application flow,not rules or format. Home New lists only original sources accepted by the existing empty-SAV19 loader and v5 seed validation. The player chooses the visible source and version before creating a city. Continue reads a bounded header only to locate the source,then validates the complete checkpoint against fresh source/model identities before offering replacement. A valid header alone is never accepted. Loaded games open paused,without offline simulation or a version upgrade.

Save uses the standard file chooser and still publishes only a new file outside the original data root. Returning home keeps a paused in-memory city. Quit and replacement ask Save/Cancel/Discard if persistent game content changed. Dirty comparison ignores only pause and control-revision differences; it does not rewrite actual serialized bytes. Invalid state remains dirty. Save cancellation/failure cannot close or replace the city. Drawing must succeed before switching source/state; failures restore the previous session. Original source and existing saves remain read-only.

Stage42 non-GUI milestone: Release/UBSan each pass172,948 assertions,including72 new reference/dirty-content checks. Each runs the same real source through12,000 native ticks,120 new-file save/readback checkpoints and24 demolition/rebuild operations while toggling food production and road connectivity. The uninterrupted history and repeatedly loaded history stay byte-exact. Independent expected income is20,424,final funds22,142,population7; food conservation and paused no-update checks pass. Save overwrite attempts refuse120/120 per mode. These simulate3h20m of nominal1-second ticks; they are accelerated core checks,not real-time GUI operation.

Release takes1,483ms for the complete12,000-tick scenario including disk roundtrips; UBSan4,774ms. Twelve1,000-tick memory samples show RSS23,625,728→24,051,712 bytes /26,329,088→26,640,384 bytes as retained identities grow2→24. This bounded trend does not prove absence of all leaks or establish App rendering performance. The exact App catalog path yields21 SAV candidates and17 qualified empty seeds in432ms,without remembering a sample identifier.

### Delivery-flow GUI acceptance (2026-10-24)

Home New→construction→save→new-process Continue improves **0/1→1/1**. Verified package `emperor-native-ready-v5/EmperorApp.app` keeps binary SHA-256 `f5b8373f6a220329c69ea74cf69351cdeb07663c33730ac96e9997c5958ad813`,117/117 production hashes,29/29 compiled units and a valid strict signature. Local `stage42/Launch Emperor v5.command` supplies the authorized external data root. No gameplay rule or production code changed during final GUI acceptance; all7 preceding native packages remain intact.

The actual homepage New chooser lists qualified sources and explicitly confirms v5. Selecting a visible empty source opens a paused city at a derived buildable view,without typing a sample number or entering diagnostics. Map clicks place3 roads,house,well and Hunter:2000→1922. At tick3,population1,stock3,production4/consumption1,the standard Save dialog opens while running and pauses the city. Cancel retains its content and dirty status. Cancelling the subsequent quit prompt retains the same process and city. A unique checkpoint then saves successfully.

The same App runs300 additional updates across300 seconds,with31 ten-second observations. Every sample advances; production minus consumption equals stock,and funds equal1922 plus tax. At tick303,population7,funds2435,food604/604,stock0,tax513/cycle75. One camp cannot fully feed7 residents; the last meal visibly reports3 unmet demand,without eviction under the declared v3 rule. Pausing3 seconds preserves all counters. RSS starts206,274,560 bytes and ends210,321,408,max210,878,464; process CPU time grows2.59→5.38 seconds. This bounded real-App sample does not prove leak freedom or long-term performance.

Saving to the earlier task-owned filename reaches macOS Replace; confirming it reaches the App writer's existing-file refusal. The checkpoint hash remains unchanged and the city stays paused/dirty. QuitCancel preserves its state. A new name saves tick303,then the process exits0 through the menu. A new process uses homepage Continue,selects the visible file and confirms source/version. Re-saving to another path is byte-exact **1/1**. Paused waiting causes no offline tax. Resume to tick304 adds exactly7 gold:funds2442,tax520/cycle76,food608/608. It saves independently and exits0.

A third process uses a fresh10-file source/asset copy,loads tick3 and turns hunting off once,leaving the city dirty. It selects a different qualified source and reaches the unsaved prompt. Only the copy's Terrain555 file is recoverably held aside; Discard then reaches a real render/decode failure. The prior source,camera,path,objects,money,population,food/tax and paused state survive;dirty remains true. Canvas/overlay matches **1,506,848/1,506,848** pixels. Quit still asks about unsaved progress and Cancel retains it. Saving the retained city differs from the initial checkpoint only by the deliberate hunting toggle and revision+1; failed replacement adds no serialized change. A guaranteed cleanup restores the copied asset byte-exact. The instance exits0 normally.

Pending GUI cases pass **3/3 executed**: running SaveCancel,existing-target writer refusal followed by QuitCancel,and Discard followed by new-source render failure. A separate unwritable-directory/disk-full variant was not induced. Movie-close failure followed by a renewed dirty-city quit prompt remains **GUI-untested**: no safe,bounded real failure method was established. Source review and authored MovieSession tests do not count as that combined App pass. Normal playing-movie Cmd+Q is independently verified **1/1** in [BINK](BINK.md#ready-v5-delivery-recheck).

All4 stage42 instances exit0 (58121,60004,60775,61434); both decoder children disappear **2/2**. No test App remains and desktop control is released. Receipts retain transient modal/focus/selection helper failures and readbacks; a modal AX timeout/invalid return alone is never a pass. The first rollback comparison also stopped on a local Python None-handling error before further UI actions; the corrected comparison passed. No production guard was changed or bypassed.

Release/UBSan results above stay bound to38 compiler-derived dependencies,8 object hashes and exact binaries per mode. Stage42 tracks10/10 unchanged original inputs and10/10 restored copy inputs. Local source/test/binary binding,GUI receipts,captures and checkpoints remain outside Git in `stage42/source-test-bindings.json` and `delivery.json`. No original game was started,no system permissions changed,no assets were committed/uploaded,and no commit/push occurred. Strict original normal update retains its0/6 refusal; this delivery improves the separately authorized native subset and ordinary user flow.

## Native operating interface and rendering baseline

Stage43 changes presentation and input wiring only. Native v1–v5 rules, state transactions and checkpoint encodings are unchanged. Existing lawful original-game city screenshots establish the broad layout reference: top resources/controls, a large map on the left, and building categories/details on the right. This is the first functional layout pass, not pixel-identical original UI reproduction. Original minimap/advisors and unsupported religious/trade systems remain unimplemented; the latter controls are explicitly disabled. Native rule/source/compatibility explanations stay accessible in a separate dialog.

The externally supplied China_Interface archive has2,107 entries:2,098 decode and9 are zero-size EmptyRecords. Six read-only images decorate the interface: physical IDs1221/1224 for top/side panels and1323/1327/1339/1319 for housing/food/water/roads. All6 decode. These are authored control bindings to identified visual assets,not a recovered original callback table. Original assets,contact sheets and captures stay outside Git. Missing decoration falls back to plain colored controls. Category text,buttons,resource counters and details use NSFont system APIs with normal Chinese fallback; no bitmap Chinese font or custom glyph atlas is used. Cocoa draws text at the display backing scale.

Existing atomic commands are reached through visible categories and map clicks. Browse-click selects an owned object by its stable ID; dragging pans without constructing. Details show residents,capacity,road/water service and production status. Demolition still requires preview then explicit confirmation. The top bar provides save,continue,pause/resume and1×/2×/4× pacing; separate zoom buttons and the existing wheel/drag path share camera transforms. Pacing is a view preference,not a persisted rule: a fresh process defaults to1×,and ordinary Continue always loads paused. The old direct onNativeLoad method has no control,menu or call-site binding in the repository; the normal Continue path never reaches its legacy1-second timer. Old-version category gates and demolition gates are tested independently.

The default map image changes868×434→816×620 (376,712→505,920 logical pixels,+34.3%). Narrow windows fit this image with letterboxing; no new tiles or higher-resolution original art are fabricated. The simulation still executes synchronously on the main thread. Scene changes still create a candidate AppMapSurface,perform CPU sprite blending,construct a CGImage/NSImage and present through AppKit. Not every tick redraws the city: construction,demolition,population and camera changes trigger the expensive scene path. The six interface images are cached for the App lifetime; scene sprite decoding remains scoped to each render. Hover now avoids recomputing the same tile's preview on every sub-tile mouse movement.

A20-iteration original empty-source benchmark per viewport separates scene preparation from CPU rasterization. At868×434,means are3.299ms preparation and24.053ms rasterization,with1,506,848 BGRA bytes per full frame. At820×650,means are3.071ms and24.941ms,with2,132,000 bytes. The second viewport is a larger-layout probe,not the final816×620 dimensions. These measurements exclude AppKit presentation/texture upload and do not cover dense populations,p95 latency or GPU timings. They establish neither60fps nor a measured Metal speedup.

The smallest next renderer migration is to emit the existing ordered sprite draws into Metal quads,retaining nearest sampling,premultiplied alpha and the established painter order,with CPU reference/fallback. Cross-frame immutable decoded-resource caching and versioned snapshot preparation can precede that work; stale revisions must never replace a newer city. AppKit system-text controls can remain above the map. A wholesale ImGui/framework replacement is not required by this measured boundary. No custom sprite Metal renderer is claimed in stage43.

### Stage43 final-package GUI acceptance

The operating-interface workflow improves0/1→1/1 in `emperor-native-interface-v5-final/EmperorApp.app`,binary SHA-256 `265ec1ff619fdb0422ef10a06ad6ca0a9740a597690bdff0386bd1ec9e9cb6be`. All119 production hashes and29 compiled units match; strict signing passes. Release and UBSan each pass173,052 authored checks,including104 presentation checks,with39 compiler-derived dependency bindings per mode. The same real source passes across5 rules versions and retains3/3 original building-art bindings. These core results are distinct from GUI evidence.

Final-package map clicks place3 roads,house,well and Hunter through the visible categories:funds2000→1922. Three actual category-button clicks preserve all city counters3/3,showing no click-through construction. Browse selects house#4 without building; the same identity is picked at1×,2× zoom and a1024-point-wide letterboxed window3/3. Captures are2× backing resolution,with system Chinese text and uncut controls. Browse panning preserves state. An occupied-house placement is refused without debit1/1.

Pacing1×/2×/4× produces2/4/9 ticks in2.65/2.65/2.66 seconds per measurement3/3 (including pause/readback overhead); paused reads preserve counters3/3. Population reaches7 at tick15,funds1931 and accumulated tax9. SaveCancel and QuitCancel retain the paused dirty city2/2; a unique file saves. Legacy v1–v4 files load with correct category/demolition gates4/4 and re-save byte-exact4/4. The unused direct reload selector has no callers; ordinary Continue is the exercised path.

A fresh process opens home Continue,selects the saved city and confirms source/version. Tick15,population7,funds1931 are restored; speed defaults to1× and the city stays paused. Another save is byte-exact1/1. Road#3 demolition preview,cancel,confirm and same-position reconstruction are exercised:only confirmation removes the road,no refund occurs,and rebuilding costs2. The next update reaches tick16,credits7 once and preserves food conservation:funds1936,tax16/4cycles,food32 produced/32 consumed. The same-window drag path cancels with Esc without mutation,then commits a two-cell path with one existing free cell and one new2-gold cell;funds1934,roads4. Results are saved under new names.

After saving,home navigation and playing-movie Cmd+Q exit normally. Both final-package processes91325/92783 exit0; decoder children93484/93485 disappear2/2. Preliminary-package process88871 also exits0 but is not counted as final-package GUI evidence. No project App remains; the desktop is released. The one preliminary helper lookup for a wrong Save identifier failed before sending an action; the actual identifier was then read and used. No permission denial or unknown action was retried.

Stage43 preserves8/8 previously delivered App binaries and10/10 earlier hashed original inputs,with2 additional read-only UI asset hashes bound locally. Gameplay and serialization sources retain their stage42 hashes. Eleven source/doc/test files change in this stage;22 other previously dirty files are preserved. Local `stage43/source-test-bindings.json` and `delivery.json` bind the exact tests,App,screenshots,receipts and checkpoints. The prior stage42 12,000-tick/120-roundtrip core soak and300-second App trend were not repeated because gameplay did not change. New-layout render-failure injection,unwritable/disk-full targets and movie-close-failure/dirty-quit retry are not claimed as newly tested; earlier transaction evidence remains separately recorded. Strict original-city update remains retained0/6,not retested or bypassed. No original game launch,system permission change,asset upload,commit or push occurs.


## Retained sprite cache and publication gate

Stage44 changes rendering resources only; v1–v5 gameplay and serialized bytes retain their stage43 hashes. The App holds a bounded immutable decoded-sprite cache across candidate `AppMapSurface` instances. Source/index version checks and both-layer failure rejection precede publishing a candidate. [SG3 cache measurements](SG3.md#bounded-cross-frame-sprite-cache-2026-10-24) give exact budgets,scope and reproducible CPU timings. The previous section's per-render-only decoding baseline is superseded; scene preparation is still per candidate and synchronous.

The final `emperor-native-cache-v5/EmperorApp.app` binary is SHA-256 `28aa26ae486d1cf3ee5aa88a294fbca7685a40073ab6c7b69ae427d8425dac75`;120/120 source hashes,29/29 compiled units and strict signature match. Release/UBSan each retain173,052 authored native checks and104 presentation checks,with40 compiler-derived dependency bindings. Cache suites bind49 dependencies each. No gameplay/version migration occurs. The legacy direct `onNativeLoad` selector still has no call/control/menu binding; ordinary Continue loads paused and the user-visible speed path is unchanged.

Final-package GUI passes New→three roads/house/well/hunter(2000→1922),warm-cache zoom/pan,one further road(1920),running to tick4/population2/tax1/funds1921,and stable pause. While paused and already warm,the test temporarily removes only an owned fault-copy General.555. The attempted legal road refuses:funds,objects,tick,population,food,water,tax and clean checkpoint state remain unchanged. The entire1632×1240 backing-pixel map including service/placement overlay is identical(2,023,680/2,023,680 pixels); before/after-refusal saves are byte-exact1/1. The copy is restored to its initial hash before the same road successfully commits. This GUI case exercises base-layer decode failure; independent object-layer failures are the authored renderer tests described in SG3,not a fabricated current-native GUI scenario.

A fresh process uses home Continue,selects the new file and confirms source/version. Tick4/population2/funds1921 reload paused and re-save byte-exact1/1. Resume advances to tick6/population3,stock6,produced12/consumed6;funds1921 and lifetime tax1 remain unchanged,so reload did not repay the tax. Both instances6091/7186 exit0 via actual Cmd+Q after unique-name saves; no instance remains. The desktop is released. This package does not repeat movie playback or long-run soak; stage43 movie Cmd+Q/decoder cleanup and stage42 long-run results remain explicitly historical.

Twelve hashed source inputs and all twelve fault-copy files retain their original bytes; nine prior delivered App binaries remain unchanged. All local captures,checkpoints,source/test/object/binary hashes and receipts are under `stage44/`,outside Git. No original game was started; no permission was denied or reset; no original asset was uploaded,committed or changed. Strict normal original-city updates retain0/6 rather than being bypassed.


## Read-only operating feedback (2026-10-24)

Stage55 fills the existing sidebar, without new rules, windows or save fields. The measured native housing explanation baseline was2/5 named conditions (road/water); it is now5/5: disconnected, dry, food-limited, full and waiting for the next arrival cycle. This is native-subset usability progress on original terrain/art, not original simulation compatibility. Original event gates remain blocked0/6.

`queryNativeCityFeedback` uses the existing food/tax planners and the same road/water predicates as committed updates. It reports all applicable structural reasons in a stable order, distinguishes existing-resident shortage from ID-ordered arrival allocation, and gives full occupancy priority without promising eviction. Zero stock does not mean shortage when the next production cycle supplies the arrival. Wells show road-dependent activation; Hunters show road loss, the production switch and a full-stock stop. The summary exposes current stock, capped next production, existing demand, planned arrivals and tax basis (currently connected residents times1), with update-count countdowns. The tooltip qualifies these as current-state estimates: intervening construction/population changes recalculate them; tax precedes arrivals on a shared cycle. Paused cities do not advance. Tax overflow never promises a successful meal.

Known original house/well/hunter icons and macOS system12-point text occupy the existing detail and service rectangles. No layout, category, advisor or gameplay expansion occurs. A read-only query cache is keyed by source identity, session epoch and revision. The weak source identity prevents old-city feedback surviving an asynchronous same-revision replacement and does not retain old source assets.

Release and UBSan each pass374/374 authored feedback assertions and30/30 forecast-versus-committed-house comparisons. A real supplied empty SAV/model passes136/136 checks,including18/18 forecast comparisons and actual capacity7; these execute native rules, not original rules. The unset-root run succeeds with SKIP. Existing native regression remains173,052/173,052 checks per mode,including104 presentation checks. Actual-App method tests pass60/60 per mode,including pending replacement preserving old committed feedback and publication invalidating it when a new source has the same revision. These tests substitute the GPU and create no window. Text bounds pass320/320 detail plus320/320 summary cases per mode; maximum measured heights150/75 fit the existing162/76-point interior bounds.

Real candidate2 CPU interaction built a fresh city, separately exercised road/water/food shortages, restored service, reached full7/7, demolished/rebuilt the house, disconnected/reconnected the well and saved through normal dialogs. Population stayed0 through each initial blocked meal,grew after service restoration,and stayed7 when full. Later hunting pause demonstrated shortage for existing residents; resuming restored growth. An initial test expectation wrongly assumed removing one road disconnected the well,while a second Hunter-access road still served it. The App correctly stayed supplied. Relocating that access and removing both well-adjacent roads produced the expected dry state; restoration led to population1 at tick28. A test receipt-name collision stopped one helper before input; the next verified continuation used a new receipt name. Neither helper failure is an App success or permission failure.

Final candidate3 contains the additional cache-identity fix. Fresh CPU and Metal processes each load the six GUI-produced condition checkpoints through Continue and match every detail/summary text6/6. The Metal-mode first load uses the documented CPU fallback while its self-test is pending; later condition loads use Metal. The6/6 figure measures visible feedback in each mode,not six GPU-only renders. Both continue the full checkpoint from tick22 to24 without adding residents; tax adds7 once (funds1898→1905). Read-only inspection and normal re-save preserve bytes3/3 across the creation process and both final processes. Final1024/1120-point window checks retain the layout,controls and map798/894×720; both backends use the same system text. Actual final-window detail/summary regions are pixel-exact12/12 between the two modes,independently checked locally rather than inferred from accessibility strings. All three App processes exit0 via normal Cmd+Q; no test instance remains.

Final package: local `stage55/candidate-3/EmperorApp.app`,binary SHA-256 **cbd2da2cf288028ffea555fb457cc398c73e20877e9a172635a807b09aaa89b8**,manifest **3223de7ed0f8ca74552dedc45090c5b76063305f7629a7bece231de934961257**,134/134 production hashes and31 compiled units,strict signing verified. The CPU launcher is the default; Metal remains explicit opt-in. Native rules/serialization,strict executor,renderer and original assets are unchanged from stage54. Stage54's performance/movie/longer-run results are retained historical evidence,not repeated here. No original game launch,permission change,asset upload,commit orpush occurs. Original inputs12/12 and copies12/12 remain byte-identical. Stage54 package is preserved. Full source/test/binary/checkpoint/capture bindings remain local in `stage55/source-GUI-test-bindings.json`; images and assets are not stored in Git.


## Opening source list (2026-10-24)

Stage56 keeps the existing empty-SAV admission and native v1–v5 formats/rules. The baseline new-city popup displayed only2 supported paths from6 supplied SAV files;4 refusals were hidden and preview coverage was0/2. The new chooser includes6/6 filenames with parent folders,explicit data support and refusal reasons. The2 supported snapshots expose their parsed112×112/226×226 active-square sizes and saved funds2000/18000. Unsupported or unreadable fields remain unknown. Names are actual UTF-8 filenames,not inferred mission names. Explicit menu row identities keep the selected file,preview and create action aligned. System fonts and the main-game layout are unchanged.

Selection produces a local terrain overview from the same admitted snapshot,using the existing native overview colours; it is not a reconstructed original scenario screenshot. A row marked data-supported is not a claim that every arbitrary external source has passed GUI acceptance. Missing art can still refuse publication through the existing render transaction. MAP/PAK starts,original mission goals and inhabited original-city continuation remain unavailable. The catalog checks at most128 distinct sorted sources and visibly reports any unchecked remainder;it does not silently label them supported.

Preview,New confirmation and Continue confirmation re-read the source/model revisions. A changed source cannot silently become a different seed. Re-reading occurs after replacement/save prompts;failure preserves the existing city and existing pause/save protections. Continue still validates the selected .encity version/source and loads paused. The existing source revision is a content identity check,not a hostile-tampering security claim. No original SAV,mission state,funds or model price is fabricated.

Release/UBSan each pass16/16 authored catalog checks and36/36 real-data checks. The6-entry supplied catalog retains2 supported and4 unsupported entries;both supported sources pass preview,actual native road/house commands,unique independent save/readback and exact roundtrip2/2. The unset-root run succeeds with SKIP. Existing native suites pass173,052/173,052 authored checks per mode and both real sources complete the existing housing/water/food/demolition/tax checks. Actual-App method regression passes60/60 per mode,with an injected GPU and no window;these figures do not stand in for GUI acceptance.

The final package's CPU and Metal processes each validate all6 displayed filenames and the4 disabled unsupported entries. Both actually create each of the2 supported sources,click a road and house,save through the standard chooser and Continue them with exact bytes2/2 per mode. Model debits are2000→1983 and18000→17983. Chinese source names remain correct in both New and Continue. Local captures of the2 selected terrain previews are bound to the final package and CPU/Metal preview interiors are pixel-identical2/2 (230,400 pixels each);initial captures of the underlying browser were excluded and replaced with the actual modal window captures. Cancelling source selection preserves the current city in both modes.

Two final-CPU fault cases exchange only the paths of2 owned test copies,without modifying their bytes or the original installation. New selection detects the changed fingerprint and disables creation before a confirmation action; the disabled button is not forced. Continue rejects a change made after its source-information dialog is shown. After restoration,the live city remains unchanged and both refusal checkpoints equal the earlier saved city2/2. All17 source and17 copied inputs match their initial SHA-256 values. Test input helpers wait for the selected filename and enabled state to settle;an AX action returning is not treated as a completed preview. A preliminary candidate exposed locale-dependent %s handling of Chinese filenames;the final path uses explicit UTF-8 NSString conversion. Earlier helper recording/early-read failures are retained separately,not counted as App passes.

Final local package: `stage56/candidate-3/EmperorApp.app`,binary **fb2e4f125546efd0869c6ce62469303f24ce3948bf0692d91d0a8e91d3d7afc5**,manifest **2abf7e3fb09b7420ce97f50e9b1bcc6293115ff896bdcdd3684b2439f237e372**;135/135 production hashes,31 compiled units and strict signature match. CPU remains default,Metal explicit opt-in. The4 stage56 processes (2 preliminary,2 final) exit normally;all earlier packages,including stage55,are preserved. The complete source/test/binary/checkpoint/local-capture binding is `stage56/source-GUI-test-bindings.json`,outside Git. No new gameplay/save version,original-event compatibility,large-city performance ormovie-exit claim is made. Prior results remain scoped historical evidence. Original strict simulation retains0/6 at its event guard. No original game launch,permission change,asset upload,commit orpush occurs.


## Responsive source loading (2026-10-24)

Stage57 removes the named stage56 responsiveness blocker: opening up to128 sources and changing previews decoded them synchronously on the main thread,with no progress or cancellation. Source enumeration,one-at-a-time SAV/model admission and terrain overview preparation now use one persistent serial dispatch queue. The modal chooser opens before work starts,shows checked/total progress and leaves Cancel available. The picker and Create remain disabled until their respective catalog/preview is ready. AppKit controls and final image installation remain on the main thread. No resource cache,rule or format is added.

Each chooser owns a fresh shared generation token;each selection owns a separate token. Closing/reopening,cancelling,changing selection and application teardown invalidate those tokens. Every queued result checks its live owner and exact tokens before touching controls. No partial catalog is published. Cancellation is checked before/after source enumeration and between each SAV read/decode;an already running OS read or enumeration may finish in the background. Cancellation does not synchronously wait for that work. Reopening uses the same serial queue,so it cannot spawn parallel readers. This is cooperative cancellation,not a claim that blocked filesystem I/O can be forcibly interrupted. Creating or continuing a city still rechecks the selected source/model identity after confirmation,using the existing transactional publication path.

Release/UBSan each pass28/28 authored catalog checks and36/36 real-data checks. The unchanged6 supplied SAVs still admit2 and explain4 refusals;2/2 previews and native build/save roundtrips pass. Actual-App method tests now pass76/76 per mode using2 original snapshots. They deterministically hold an old decoded preview callback,select the other source,then deliver the old callback:the old result cannot paint,enable Create,or overwrite the newer ready result. Actual worker progress/cancel/weak-owner teardown are tested too. These method tests create no window and substitute completion delivery;they are not GUI passes. Both snapshot hashes are bound,including the second preview source.

The final candidate2 CPU/Metal packages each pass the actual6-source selection checks and2/2 road/house/save/Continue loops. All4 unsupported original entries keep Create disabled. CPU New and Continue detect changed test-copy source identity after confirmation and preserve the old city byte-exact2/2. Final local overview interiors compare pixel-exact2/2 (230,400 pixels each);no original-screen compatibility is inferred.

For the GUI load test,122 independently owned copies of one real SAV were added beside the6 original-named copies,forming128 rows. These are workload copies,not122 additional supported scenarios. The final Metal window displayed后台核验 29 / 128 份 · 可随时取消 during active background work;Cancel closed the chooser in0.308 seconds (one AX action-to-return sample,including automation overhead,not a latency percentile). Reopening completed and the original2 sources could still be selected,previewed,built and saved. A preliminary candidate1 (same production implementation;candidate2 only updates README) also cancelled at33/128,completed128/128 after reopening,and checked consecutive source changes. Its first rapid-input helper failed to wait for native menu close animation;no second target action was sent after the missing-target check. Deterministic late-preview ordering is established by the App-method test rather than falsely treating that helper failure as a GUI pass. Normal cancellation followed by Cmd+Q exited safely;direct Cmd+Q while a modal alert is open was not claimed.

Final local package `stage57/candidate-2/EmperorApp.app`:binary **da985e6cf75c8d2b03931a658b2dd3abddaaa3e12cd3ec7684b7ccfd5defdd3b**,manifest **fe776e418dca9ea4ba536aaf802adb3be5917f3f238fc9a85a27b5f2b0ceade6**,135/135 source hashes and31 units,strict signature. CPU default and explicit Metal launchers are in `stage57/`. Source/test/input/GUI bindings are `stage57/source-GUI-test-bindings.json`,outside Git. The3 stage57 instances (one preliminary,two final) exit0;earlier stage56 packages stay preserved. All17 original inputs and17 baseline copies remain unchanged;all139 workload-copy inputs remain unchanged. Existing173,052 native checks per mode are retained from stage56 with matching dependency hashes,not rerun. Original strict simulation remains0/6 at the event guard. No gameplay/save version,asset upload,original game launch,commit orpush occurs.

## Native v6 original labor (2026-10-24)

New cities start as `original-data-native-v6`; v1–v5 sessions keep their own rules and bytes. v6 reuses original functions instead of authoring labor:

* **Labor pool** `floor(residents × P / 100)` with `P = difficulty + wage band + health band` from `4AD850` (`originalBuildingLaborPercent`, now also called by the strict update). P comes from the source SAV's saved difficulty, wages and health; sources with nonzero saved formation labor leave v6 unavailable. Every supplied empty source gives P=42.
* **Jobs** use original employment model column 5 (Hunter 33 → 15, Well 72 → 4), categories 0/3 and the source's saved category priorities. City allocation is `originalBuildingCityEmploymentStep` (`4F1590`); per-building assignment mirrors `51E310` in ascending native id. Only road-adjacent Wells and road-adjacent, enabled Hunters are eligible.
* **Authored consequences, not original behavior:** Hunter meat per meal = `floor(4 × workers / required)`. Arrivals are not charged meat. An unfed city keeps growing while any eligible Hunter is understaffed; with all Hunters staffed (or none built), a shortage stops growth. Well workers are displayed only and do not change water coverage. Original house level 0 has no food requirement, which motivates removing the v3–v5 bootstrap deadlock, but native growth/water/food are still authored.
* Format: the v5 body plus `LABOR P hunter well priority0..9`, which must equal the reloaded source/model or the load is refused (`labor_model_mismatch`). Food-ledger validation drops the v3–v5 "consumed ≥ population" and paid-arrival constraints for v6 only.

### v6 measurements

* Saved labor pool versus the original formula: **21/21** supplied SAVs, of which **2/2** inhabited (70 residents → 29); the other 19 are trivially 0. One P value (42) is exercised.
* Native/strict equivalence: **400/400** randomized cases, **4,210/4,210** building assignments equal to `originalBuildingEmploymentStep` (113 shortage cases, 329 mixed-priority cases).
* Release/UBSan each: labor/contract **14,595** checks, v6 rule scenarios **71** (drafted by Qoder DeepSeek-Flash from a written spec, reviewed), real-data **1,663** checks over **17/17** v6 sources with exact roundtrip; existing native **173,052**, feedback **374**, catalog **246** real, strict update **67,299** all retained. Actual App method checks **76 → 175** including a v6 city through the real transaction/render path and labelled waits.
* GUI acceptance of v6 labels is **0/1 attempted, blocked**: the verifying host lacks macOS Accessibility trust; no permission was requested. The App package matches **135/135** production sources.
* **Evidence boundary.** Labor source: original formula over current native residents, inputs frozen from the source SAV (difficulty/wage/health never change in native). Recompute cycle: native recomputes on every query and at each 2-update meal; the original runs employment inside its city phase cycle (phase25), whose timing relative to native updates is **unobserved**. The 4,210/4,210 result proves the native code path equals the ported strict code path only, **not** live original agreement; normal original update stays **0/6**.
* **App wait diagnosis (stage58 closeout).** The earlier UBSan failure was the opening-source scan wait (`catalog-scan`), not a product hang: 21-SAV root 0.52s Release / 3.32s UBSan vs a fixed 3s test limit; 2-SAV root 0.04s/0.23s. Longest main-runloop turn during the scan ≤8.4ms. Waits are now labelled; background scans may take ≤120s but any main-runloop turn ≥250ms fails. Release/UBSan × two data roots **4/4** pass.

## Candidate-6 water-condition GUI acceptance (2026-10-25)

The frozen local `stage58/candidate-6/EmperorApp.app` passes both admitted empty SAV19 sources on CPU and Metal: **4/4** separate flows, **50/50** mode checks each (**200/200** total), **6/6** cross-backend comparisons and **4/4** evidence-protection checks. All **4/4** actual Cmd+Q operations exit with code 0 and no cleanup fallback. The final driver and each CPU reference are bound by SHA-256; earlier failed runs remain separate local evidence. The package has **136/136** production source bindings and **31/31** compiled units.

Covered interactions include source selection, direct categories/tools, legal roads and construction, rejection without debit, empty/occupied house conditions, real scrolling and closing, native water coverage before/after a well, confirmed/cancelled demolition, pause/speed, resizing, saving/reloading, and old-v6 byte compatibility. Conditions queries preserve both city state and serialized bytes. Removing a well records a newer successfully published revision without a refund, then verifies coverage changing from wet to dry; a persistent disabled confirmation control is not mistaken for failure or sufficient proof of commitment.

The two admitted sources have independent initial-funds expectations (2000 and 18000). Test placement uses bounded product-hover coordinates instead of a source-specific camera offset; the negative placement is a house overlapping a road committed by that run. All **7/7** checked real inputs and **118/118** common protected evidence entries remain unchanged. Raw inputs, saves, images, action logs and complete binding reports stay outside Git.

This verifies native UI and model projections only. The original house-water walker state, decay/update timing and automatic house upgrades/downgrades remain unconnected and lack independent live-original acceptance. Normal strict-original progression remains **0/6** at the existing event gate. ASan initialization failed in an earlier attempt and remains stopped; these GUI passes do not establish an ASan result.


## Experimental v7 house-state foundation

V7 is an explicitly selected native foundation format named
original-data-native-v7-foundation. The default/latest playable rules remain v6.
It does not migrate an old checkpoint on load, recover an original running city,
or enable automatic house upgrading. Candidate-6 and earlier accepted App
packages remain separate fixed deliverables.


### V7 wire layout and limits

The encoder retains the legacy source/seed, state, FOOD, TAX, and LABOR records.
Immediately after LABOR it writes one model-table line and one clock line:

    HOUSEMODELS <count> [<model> <capacity>]...
    HOUSECLOCK unknown

A supplied clock replaces the second line with:

    HOUSECLOCK <phase> <monthPart>

HOUSEMODELS contains the complete seed capacity table in ascending model order,
including unused rows, so readback must bind that table to the same seed.
Only the admitted models 0/2/4 contribute to supported house capacities or the
historical departure ceiling; an unrelated bound row is not a new supported
grade. HOUSECLOCK unknown preserves missing timing evidence instead of choosing
a phase or calendar mapping.

The ordinary object rows and RETIRED count/id/kind rows follow unchanged.
After those records, v7 appends:

    HOUSES <count>
    <id> <type> <model> <servicesKnown> <water> <tailhex> <imageWord20> <imageHouseByte3c>

There is exactly one HOUSES row for each active house, in ascending object-id
order; no nonhouse or retired object owns one. servicesKnown is 0 or 1. water is
a decimal 0, 1, or 2, or a single "-" for unknown, independently of servicesKnown
and tailhex. The encoder emits tailhex as exactly 122 lowercase hexadecimal
characters for the 61 remaining-state bytes. imageWord20 is a signed 16-bit
decimal and imageHouseByte3c an unsigned byte decimal. Types/models still pass
the supported-grade validation; serializing a value does not enable evolution.

The named byte ceilings are nativeSessionLegacyByteLimit = 512000 for versions
1–6 and nativeSessionFoundationByteLimit = 2 * 1024 * 1024 (2097152) for v7.
nativeSessionByteLimit selects the budget only after the caller identifies a
recognized version/rules pair. nativeSessionWireVersion inspects that header;
nativeSessionReference additionally locates the source. Neither replaces full
body/source/model/state validation by decodeNativeSession. Unknown versions do
not acquire the larger v7 allowance.

The extension stores a native house's explicit type/model, 61 bounded remaining
state bytes, a flag saying whether those service bytes are known, independent
optional houseWater (unknown or 0/1/2), and the image fields consumed by the
existing projection. Model capacities are source/model-bound. Unknown service
state is not substituted with the native well-radius overlay. In particular,
houseWater is a separately latched value: storing later decayed service bytes
must not recompute or overwrite an earlier water value. This preserves the
distinction between reading supply and later service decay without claiming a
measured original cadence.

An optional house clock stores an explicit phase/month part, or remains unknown.
Ordinary native ticks do not advance it, decay these service bytes, recompute
latched water, or invoke strict automatic house evolution. A known clock and
water value alone do not enable evolution. Type 4 and unsupported image/type
combinations must refuse explicitly; they are never silently projected as type 3.
Supported explicit grades use the same model for validation, population capacity,
persistence, and scene projection. Existing native first arrival may retain its
preexisting empty type-2 to occupied type-3 presentation transition; this is not
a claim that the missing original type-3 to type-4 upgrade is implemented.

V1–v6 have no hidden v7 runtime state and retain their prior encoding and rules.
A v7 implementation must reject missing per-house state, house metadata attached
to nonhouses, incompatible type/model pairs, out-of-domain water or phase values,
missing/unsupported active model capacities, mismatched capacity tables, and
malformed/trailing records before publication. The admitted 3999-object maximum
also has to fit its bounded v7 parser budget; the old format's byte ceiling cannot
silently make a valid new state impossible to reload.

The regression source fixes 12 authored v1–v6 byte strings (empty and constructed
states for each version) captured before the v7 production change. Their source
header SHA-256 was a4bbfc18179f72dc50c77fcb69d85b73c6bda0f0a7fc8bbc6f5a23aa96246048.
The new synthetic tests cover those legacy bytes, explicit v7 roundtrip and
rejection, independent water/tail persistence, grade capacity and projection,
unknown-state preservation, and disabled automatic evolution. Test results are
reported only after the matching production implementation is compiled and run;
these are contract/compatibility tests, not original-data or GUI acceptance.

The original strict routines currently remain incomplete for the observed first
upgrade: type-4 selection and the water carrier's return branch are not admitted.
An original-to-native clock mapping and complete walker/service persistence are
also unresolved. Address-annotated migration code is a project-code contract, not
independent clean-room proof of original timing or behavior. This foundation
therefore does not improve the retained strict-original acceptance denominator,
does not justify a new upgrade algorithm, and does not authorize decompilation,
original binary execution, or asset upload.


Retired houses retain identity and kind, not their former runtime grade. Historical departure validation therefore uses the maximum capacity of the admitted house models 0/2/4 only; unrelated table rows cannot widen this bound. Active houses still use their own exact model capacity. The regression includes legitimate 14/21-resident demolitions and fabricated departures above that supported ceiling.

The byte budget is enforced at all four boundaries: header/reference inspection,
new-file publication, file readback, and full decode. A large valid v7 checkpoint
must pass the entire write-new → read-file → decode chain; extending only the
decoder is insufficient. Oversized v1–v6 and unrecognized headers do not inherit
v7's larger allowance. All versions retain a finite maximum.

The IO regression creates a unique /tmp directory with an empty simulated data
root and a separate checkpoint directory. It uses only generated native bytes.
It verifies exact large-file readback, refusal to replace an existing v6/v7 file,
data-root read/write rejection, version-specific size limits, and a controlled
overlong-filename failure that leaves the prior file and caller state unchanged.
These checks do not alter permissions, fill a disk, or access user saves.


### Non-GUI verification (2026-10-26)

Release and UBSan each pass 173391 authored assertions on the same frozen
production/test source snapshot. This includes the 12 pre-v7 golden cases
(126 checks) and 213 v7 foundation checks, of which 35 are IO checks; these
subcounts overlap and must not be added to the total. Supported-grade
capacity/projection, 14/21-resident demolition, and refusal of fabricated
departures widened by an unrelated model99 are included.

The 3999-house synthetic checkpoint is 619393 bytes, exceeding the legacy
512000 ceiling but within v7's 2097152-byte budget. Its actual new-file write,
independent read, and decode/re-encode are exact. The IO checks also preserve
the exact legacy ceiling, reject oversized old/unknown headers and oversized
v7 input, refuse existing-target replacement and simulated data-root access,
and retain caller/file state after controlled temporary-file creation failure.

Each mode binds 43 compiler-derived project dependencies and records unchanged
production/test hashes before and after compilation/runs. Each test process has
a 60-second deadline. The separate unset-data-root invocation exits with SKIP
in both modes; this is not a real-data pass. No original assets, user saves,
App windows, or ASan are used. These results establish native format/contracts
and do not enable automatic upgrading or improve strict-original acceptance.


### Explicit v8 house runtime development mode (2026-10-26)

V8 is an explicit default-off new-city option; default v6 and all v1–v7 wire
identities remain unchanged. It is a native adapter, not original-city continuation.
The immutable source factory binds the same SAV/model revisions and requires the
supplied figure/building models, speech metadata and existing SG archives. It
constructs an explicitly labelled fresh native Well-only pool; it does not resume
source wildlife or invent a recovered original process clock.

After HOUSES, `RUNTIME8 1` carries a fixed named field schema: phase/cycle/month
state and origin, complete Well figures, cell-head links, free FIFO/highest slot,
route owners/directions, shared service grid, full dual32-bit RNG and process
history, formation/speech state, hook countdown, per-House automation, Well
primary/counters, complete strict building records/context, persistent holds and
native policy tags. Payloads are explicit versioned fields, not memory/pointers.
Integer narrowing/counts/duplicates/strings are bounded. V8 reference/write/read/
decode allow64MiB; v7 remains2MiB and v1–v6 retain512000 bytes. Writes use unique
outside-data names and never replace files.

The whole transaction copies native ledger/resident input, executes an admitted
strict phase and ordered Well pass, synchronizes grade/capacity/render mirrors,
validates persistence, and only then offers a render candidate. Any failure keeps
the old state. Direct raw v8 Tick refuses. Original phase28 water is independent
of the later-decaying service tail. Phases21/22/28/31/35–40 and existing empty
phases are admitted; unrelated phases/post-pass are explicit native skips.
One native tick advances one phase; phase50 wraps while retaining source
month-part/cycle. This is NOT proven original calendar timing. Native v6 labor,
Hunter production, arrivals/type3 first arrival, food and tax remain labelled
native policies, not original engine equivalence.

Local original2614/2612 evidence admits occupied type4 variant1/group472/physical1512.
Actual type3→4, capacity, image and save updates are connected. Variant0/empty4
and subsequent type4 evolution remain unproved. A grade change retains the native
constructed2×2 collision footprint, explicitly tagged; the original type4 virtual
pass-through predicate is not claimed. A Well hitting its unproved return target
is held with the same owner/route and does not respawn; other objects continue.
Normal Well demolition uses reciprocal worker/cell/route cleanup before deletion.
These limits prevent a claim of complete continuous delivery or full升降级.

Stable-source Release and UBSan each pass2/2 explicit empty-source adapter
scenarios ×1807 checks at700 updates (including native adjacency/dispatch regressions): real dispatch/service/type3→4, grade capacity,
resumed-phase equality, named return hold/no respawn, Well removal/FIFO/route
cleanup, atomic refusal and actual write/read/no-overwrite. Original SAV hashes
are unchanged. The core/legacy suite passes173481 checks per mode; outer-wire
passes32/32 including12 legacy identities plus1 v7 literal. Query regression
passes27 cases and360 candidate comparisons. These are adapter metrics; strict
original normal update remains0/6.

GUI acceptance is tracked against independent App hashes, never inherited
from candidate-6. The corrected package (binary3ceb3399…) passed actual CPU and
Metal on both selected inputs: default-off v8 creation, update110/population28
with four dry houses, live delivery/type3→4 by update348/population29, loaded
update700/population42 and resumed to703, pause, exact save/reload, actual house
condition popovers with byte-identical saves before/after queries,1024/1280-wide
window changes, select/cancel without city mutation, and normal Cmd+Q/exit0 for
both processes. Logs confirm the CPU/Metal route and all expected walkers drawn.
The map crops match geometry but are not byte-identical: each has2298240 pixels,
maximum RGB channel difference2 (local screenshot/display comparison, not an
original-game equivalence test). The v8 water overlay now uses persisted cached
water instead of static v6 coverage. Failed driver navigation/AX transport/save-
marker assertions are preserved, diagnosed and independently resolved by actual
state; they are not counted as product passes. Source0/source1 input hashes stayed
unchanged. Test evidence is outside Git in the house-v8-gui-acceptance-20261026
output. The subsequent native-only road/initial-roam footprint correction is also
verified: it preserves the constructed footprint view for road-owner probes and
phase31 dispatch, then restores canonical type4 in all pools before publication.
Strict type tables remain unchanged and the new phase input defaults off.
Type4 lifecycle needs no extra skip: constructed houseInUse=1 survives conversion.
Final package binaryfa1b8026c14a8c3bba23a9758071f9d7a9df0c74e4699ffd34bcfbc69a71455a
binds143/143 production files and independently passes both sources in CPU and
Metal, including the live110→348 delivery/upgrade,700→703 resume, actual
condition/query byte invariance, save/reload, resize/cancel and normalCmd+Q/exit0.
Final GUI evidence is in house-v8-gui-fa1b8026-20261026 outside Git; prior driver
failures are preserved. No prior package's result is silently reused. Earlier approval denials and subsequent accepted user
authorization remain in local evidence.
No ASan, push, original executable inspection, asset upload or user-save overwrite.


A later bounded local prefix investigation validates all2000 Figure slots,
class/object references, complete draw-cell chains and immediate known following
tables for six isolated test SAVs. Two known type28 records (2614/2612, same Well)
are action2 with destination=owner road=home=roam origin(89,88),roamSteps0 and
reciprocal routes. This is numeric prefix observation only: the animal layout
contract is still candidate, unknownFixed4 is not traversed, and whole-SAV
admission/source support is unchanged. Opus reviewed the necessary source and
numbers in the existing approved session at12:52–12:54UTC and proposed a default-
off, guarded native observed-return policy. That NEW policy is not implemented
in this package: return target ambiguity/nonzero-route handling and subsequent
type4 evolution remain explicit limits. It is not evidence of a general original
return algorithm, calendar timing or full升降级.


### Limited native observed Well-return policy v1 (2026-10-26)

The explicitly selected new-v8 App path now opts into a **native observation
policy**, not a proven general original return algorithm. Default new-city rules
remain v6; the programmatic runtime constructor defaults to Hold. Old untagged
v8 saves keep Hold. The frozen fa1b8026 package and candidate-6 are not replaced.

Two known type28 records from locally validated Figure prefixes (2614/2612,
same Well) have action2, zero roamSteps and a destination equal to owner-road,
home and roam origin. The full2000-slot/class/object/draw-cell graph and immediate
known following tables validate that bounded numeric observation. The unknown
animal contract remains candidate and unknownFixed4 is not traversed; this does
not admit the whole SAV or enlarge strict-original supported-source counts.

The caller-owned observed flag defaults absent in the strict context and is
restored before a phase candidate is published. It is enabled only by the saved
v8 versioned policy tag. A transition requires a live reciprocal Well/worker,
route0, matching owner-road/home/origin, legal current/home cells and roads,
and a path accepted by the existing mode18 route finder. The transition frame
retains position/route/destinationCell, sets action2/destination/roamSteps0 and
writes the normal next frame. FollowRoute starts on the following update.
Negative/inconsistent route values or missing/changed roads are not guessed.
Existing route allocation, walking services, arrival removal, primary/cell/route/
FIFO cleanup and the subsequent phase31 hook do the rest.

Policy version/conflict errors and a saved transition missing its enabling policy
are atomic named refusals. Evidence-outside states retain their own worker and
route in a named hold; other native objects continue. Restoring the exact legal
road rechecks qualification before continuation. Holds, full route/phase state,
policy tags and transition provenance survive save/reload. These are development
v8 policies for a policy-aware engine, not a promise that earlier v8 draft
binaries implement them; legacy v1-v7/default-v6 byte behavior is unchanged.

The phase39 caller also supplies its admitted native type/model pairs. A valid
strict candidate outside that projection contract is isolated as
house_target_native_grade_unsupported, preserving the record and conversion
count instead of failing the whole city during projection. The direct guard
regression keeps a positive3-to4 candidate blocked when4/model1 is omitted;
geometry/identity failures still refuse before that guard. This does not invent
any type4 evolution target, delay, goods consumption or merge/split behavior.

Frozen run2 non-GUI verification passes six groups: Release and UBSan local
on both selected sources (106797/106686 assertions per mode), plus genuine
Release natural continuation for both. The latter changes neither model, speed
nor roamSteps/limit:862/1117 updates complete two returns, reciprocal cleanup and
phase31 redispatches, followed by20/17 new service writes and60/82 decays.
All61 actual file checkpoints resume byte-identically, including before/after
transition, route flight, arrival and redispatch. Source/dependency hashes match;
negative policy/route/ownership/coordinate/road cases retain their assertions.
UBSan natural soak is not run and no ASan result is claimed.

A separate actual-command regression removes either the home road or current
road while the worker is returning, saves/reloads the hold, rebuilds that same
road, and verifies route/phase-consistent recovery. Both cases pass. Fresh
Observed constructors also pass the full existing two-source build/service/
first3-to4/capacity/save integration at700 updates (1809 checks each), retaining
independent legacy-Hold cleanup probes. These are native adapter checks.

The final independent package is in house-v8-return-policy-v1-r2-app-20261026,
binary631d3d71975189b4ae93d7d8d17de05309157c445cab10d61ee5fbfcdc34de48,
with143/143 production inputs. Actual CPU and Metal two-source return/redispatch
GUI, in-flight and post-redispatch exact save/reload, visible policy, query
invariance, resize/cancel and normalCmd+Q/exit0 pass (122/119 recorded checks).
Both modes enter action2 with route1, return/clear the old1999 worker, and publish
new primary1998. The native maps/sprites admit without zero/unsupported/failed
figure images. Metal mode uses CPU for its initial empty-city request; all730
later published frames use Metal, including the return scenarios. GUI boundary
inputs are explicitly staged native checkpoints; they do not substitute for the
separate natural two-round runs. The early CPU inspector emitted an8-bit counter
as a character; the failed driver record is kept, the inspector fixed, and the
same paused instance subsequently completes the full acceptance. No old pass
count is substituted for this binary.

Still missing: type4's actual evolution target function (up/down target,
applicable delay and whether slot/2x2 geometry survives). Existing5-to4 code does
not establish4-to3. Generic model1 water/threshold queries and known zero goods
columns do not establish that dispatch. The implementation keeps that path
isolated and does not silently substitute a type3/5 approximation. Source
month-part/cycle tail remains the labelled native preserve policy. Strict
original normal progression is still0/6. No original executable inspection,
asset/image upload, commit/push or user-save overwrite occurs.


### Occupied-house rehousing R3 (2026-10-27)

This supersedes the earlier development package's type4-downgrade blocker,
without changing default v6 or strict-original admission (still0/6). R2's
explicit observed-dry native policy admits the measured occupied type4/model1
to type3/model0 transition, retains its constructed2x2 footprint, then applies
capacity refresh and outgoing population accounting. Positive-delay rewatering,
empty type4 conversion and higher type4 evolution remain isolated. The original
observation and the native implementation are separate evidence categories.

R3 adds the explicit native_migrant_nonempty_rehousing_policy_v1 tag and
RUNTIME8 3. Layout is unchanged from2; this wire identity requires both the
new policy and observed-dry mode. Versions1/2 cannot silently carry the new
policy. Default v6 and legacy v1-v7 golden bytes stay unchanged; old v8
checkpoints do not silently gain rehousing. Fresh explicitly selected v8 App
sessions use the R3 policy.

A type13 migrant uses the existing reservation/action7/action8 arrival path
for a nonempty target. Reciprocal house.worker ownership, slot0 cleanup,
capacity, route/FIFO membership and exact arrival provenance are checked in
one copied phase transaction. Only proved positive resident deltas enter the
arrival/meal ledger; repeated ticks or restore do not add people twice.
Demolishing its reserved target releases the owned route and reservation but
keeps the migrant payload and people alive at the same cell in native waiting
action6. This target-removal fallback is an explicit native policy, not proof
of general original behavior. An empty target remains in a named hold rather
than inventing its conversion; a nearer empty house is not silently skipped.

On one frozen production revision, source0 Release passes2616 checks; source1
Release and source0 UBSan each pass2788, including repeated/capacity-clipped
arrivals, full houses, cancellation/target demolition, action7/8 restore,
empty-boundary holds and population conservation. The latter two runs each
execute360 ticks and53 replay boundaries. R3 codec Release/UBSan each pass117;
the independent outer-wire/legacy validator passes32/32. These use explicitly
staged native fixtures over the two admitted sources, not new original samples.

Independent R3 binary4adb278c84b32b2ed7f289ff74aedc369d4840dcc9797542f87fc82e7a7e7bb7
binds143/143 production sources. Actual CPU144/144 and Metal139/139 GUI checks
cover both sources' visible bounded policy, resident2-to6 arrival, reservation
cleanup, byte-exact save/reload and no repeated arrival. Each backend also tests
source0 actual cancel and target demolition with all four migrants retained,
then normal Cmd+Q/exit0. Early
Go-sheet keyboard and window-raise driver failures remain recorded; recovery
uses the same paused CPU instance, not substituted prior-package results.
The package, source snapshot and evidence are outside Git in the local
house-v8-nonempty-rehousing-r3 and house-r3-integration outputs. R2 and earlier
accepted packages,31 protected original inputs and user saves remain unchanged.
No ASan, original-game experiment, asset upload, commit or push is performed.


### Explicit native immigration B (2026-10-27)

The new explicitly selected v8 App path uses
`native_population_immigrant11_policy_v1` and `RUNTIME8 5`. Default-v6 cities,
old v1-v7 wire bytes and RUNTIME8 1-4 policies retain their old behavior.
Loading an old v8 checkpoint never enables this strategy implicitly. Empty
original-source admission is unchanged; this does not import an inhabited SAV.

**Authority and scheduling.** `NativePopulationRuntime` owns every mutable
`OriginalBuildingUpdateContext::Labor` field (27), employment rows/notices,
immigrant spacing, delays and remainders, and cumulative receipts. Native
conversion restores these fields instead of resetting them from the template.
The admitted phase23 calls the whole existing
`originalBuildingCityLaborStep`; its resulting figures, reservation links,
services and labor state are published in one transaction. Native construction
starts with its own empty formation pool, not stale source wildlife formations.
Health/sentiment inputs and model values come from the admitted source; the
preserved-month-part native clock is not a recovered original calendar.
Population-statistics pending work, military formation work and type12
emigration remain named unsupported domains, with no partial committed phase.

Only proven strict arrivals write new-mode residents. The previous native
meal-cycle arrival loop is disabled for this policy alone; food/tax and native
v6 staffing policies remain explicit native adapters. Arrival publication
rebases that derived staffing on the final resident count, keeping prebuilt
Wells/Hunters, figure links and persisted records consistent. It does not claim
original phase25 execution. Old modes keep their previous meal growth.

**Wire and receipts.** After the frozen observedDry/type13 extension and before
object automation records, wire5 adds `populationRuntime` and
`immigrantPayloads`. The latter uses its own `NATIVE-IMMIGRANT 1 118` framing;
it is never inserted into the frozen v1 Well visitor. Full Labor/Employment
state and these five uint64 receipts persist: `admitted`, `arrived`, `unplaced`,
`populationArrived`, `populationDeparted`.

- `admitted = live type11 offered people + arrived + unplaced`.
- Native resident total equals `populationArrived - populationDeparted`.
- The existing since-meal receipts additionally reconcile actual arrivals,
  observed excess departures and demolition with the last food receipt.

Admissions use successful allocation slot/serial/home/amount proofs. The
original labor `arrivals` counter includes attempted allocations and is not
substituted for that receipt. Offers allocated and normally removed in the same
pass are still admitted/unplaced once. Inconsistent strict-context population
or labor mirrors refuse load-ready checks. Negative decrementing delays,
spacing outside0..10000000 and cumulative counters above399900000000000 are
rejected before updater execution.

**Arrival and cleanup.** Type11/action8 has a per-house proof, deduplicated and
summed for the complete ordered pass. Ordinary empty type2/model0 uses the
existing worker conversion to type3/model0. Strict worker code only clips an
empty house; the new native reservation policy separately clips a nonempty
reservation before execution and records excess as unplaced. A full target or
known unreachable entry cancels through normal owned cleanup. Actual home,
slot0, cell membership, FIFO, route ownership and full actor reset are checked.
Several houses may arrive in one pass; global `lastArrival` need not equal one
house's delta. A lower-slot arrival may release a target subsequently reserved
by a higher-slot type13; that later reservation is proved in slot order using
the existing nearest-house function rather than requiring the final worker to
be zero. This never enables type13 arrival into an empty house.

Demolishing an inbound target cancels its offer; demolishing a type13 target
releases its reservation while keeping the departed people. Mixed11/13 contexts
use the same saved capabilities for both pre- and post-cancellation checks.
Occupied demolition explicitly debits the strict/native population mirrors.
Query/preview operate on discarded candidates; failed operations retain all
bytes and do not leak a phase prefix.

**Validation on one frozen code version.** Source-bound native integration:
Release9137/9137 for each of2 admitted SAV sources,4937 updates and62 exact
next-tick replay boundaries each, including actual natural first occupancy,
nonempty clipping/full cancellation, multiple same-pass arrivals, mixed-family
target cancellation, same-pass unreachable offer cancellation and a prebuilt
unstaffed Well acquiring workers after arrival. UBSan9120/9120 uses the same
code without the17 file-stage assertions. These are native/authored integration
cases over real source data, not new original-game observations. Independent
codec69/69, arrival proof183/183 and population wire578/578 pass in both modes;
old wire1-4 manual goldens remain exact. Shared A contract770/770 per source,
old dry/rehousing and native core173481/173481 per mode pass. Historical failed
driver/fixture/implementation runs are retained outside Git. The final separate R2 App binary is `e4a7a380276c7105f4d8dc1360cd63a439a1262b5408810025403f30fade7d4d`.
Its explicit source manifest covers146/146 production files and31/31 compiled
units, including the three new contract/immigrant headers. Only that packaging
manifest generator changed after the frozen final regression; all runtime code
remained identical. Strict signature and all embedded source hashes verify.

Actual GUI on the same signed binary: CPU238/238 and Metal238/238, each over2
sources, with real Well construction, live action7 advancement/in-flight reload,
source-bound action8 arrival, population receipt checks, query byte invariance,
staffing restore, pause/speed, two window sizes and new-file save/reload. The GUI
uses checkpoints for the long route and close approach; the complete4937-update
paths are separately CLI evidence, not claimed as an uninterrupted GUI walk.
Both real Cmd+Q exits return0 without force/discard. CPU logs84 CPU frames;
Metal logs1 initial CPU frame and84 Metal frames. Each has56 visible-figure
samples with placed1/decoded1 and no zero/unsupported figure; these counters
are not scanout/FPS or pixel-equivalence claims. All79 protected inputs and7
older fixed App binaries remain unchanged.

The first B App failed its actual CPU GUI because scene projection still
omitted type11 while the committed-state guard counted it. Its failed report
and normal exit0 are retained. R2 independently decodes the full118-field11
payload into slots/live/liveTypes; new action7/8 projection assertions prevent
that omission. No completeness guard or failed assertion was removed.

Reproduce the B contract with `make native-population-test` or
`make native-population-test-undefined`. Real-source integration reads only the
per-process `EMPEROR_GAMEDATA_ROOT`; use repeatable `--source` arguments with
`scripts/native_population.py` to select admitted sources. Unset roots are
reported as SKIP, never counted as original-data success. Original assets,
SAVs, screenshots and binaries are not committed. No push or ASan was run.


## Explicit city-context C1 (2026-10-27)

C1 is an optional native development policy, selected by the new-city checkbox
**城市状态与阶段派工（开发版）**. It implies the v8 runtime and immigration policy;
it does not change the default v6 mode or silently upgrade existing files.
RUNTIME8 1–5 retain their framing and policy-specific behavior. Unsupported
policy versions, a hidden city payload without its tag, absent population
state, conflicting mirrors and mismatched worker ownership refuse before load
or update can replace a city.

### State ownership and wire6

`native_city_context.hpp` captures69 top-level mutable city-context members plus
native job assignments. These include accounting/derived caches, census,
religion, cycle-tail/trade state, totals, Goods/GoodsRecords, trends and counters.
Immutable templates/models/terrain remain source-bound. Labor/Employment,
population, actors, RNG and phase clock keep their existing single owners rather
than acquiring second C copies. UI notifications/redraw requests are not replayed
as a persisted event queue; annual message/offers remain in their existing outer
combined-state domain, which C1 does not schedule.

The additive RUNTIME8 6 extension contains a named `NATIVE-CITY-CONTEXT 1`
text packet and `native_city_context_contract_v1`. Typed ranges, vector/map
counts, duplicates, trailing data and active references are checked. Only this
packet has an8MiB string limit; older wire strings keep their existing262144-byte
limit. A446529-byte legal packet is round-tripped without weakening those limits.
Rebuilding from a poisoned template overlays the saved mutable state, rather
than resetting it. A rejected whole-step candidate publishes neither a context
prefix nor population/shape/stock/assignment changes.

### Food, Goods and employment contracts

The existing house format consumers agree on12 signed little-endian Goods
words at normalized payload offset13 (wire offset17 minus its4-byte framing).
`original_house_goods_contract.hpp` is their shared bounded accessor. The C1
food census uses word0 at that offset. Legacy callers retain their old offset
policy unless explicitly selecting normalized C1 processing.

The source factory reads the existing parsed Goods component into an optional
immutable template source. Fresh C1 construction requires either that source
or an already supplied Goods context; missing data returns
`city_goods_source_required`. Fresh construction clears only native-owned
warehouse/trade pools and retains source policies/priority/unavailability;
`native_city_goods_empty_owned_pool_v1` names this native initialization.
Continuing an existing C1 save never runs that initializer: nonzero saved
inventory, policy and derived fields survive load/reconstruction and next-step
replay. Figure Goods are authoritative after the ordered actor pass; they sync
to city Goods before persistence, and the saved phase mirrors the shared phase.

C1 admits complete phases12 availability,17 food census and25 employment, in
addition to the already supported B phases. Its saved jobs are the staffing
authority for Wells/Hunters, presentation and production; v6 staffing is not
recalculated after every C1 arrival or template rebuild. Construction adds an
unemployed job, demolition removes it transactionally, and phase25 assigns it.
Type4's no-employment-category override and initial missing employment-notice
cursor are explicitly tagged native adapters; provided false notice values
remain false. Old B/default paths do not acquire those initializers.

### Frozen calendar and material-consuming houses

Phase50 calendar/month/year/month-service and general health scheduling remain
outside C1. The existing51-phase loop retains its explicit native frozen-month
policy. Advancing that loop cannot be represented as advancing the original
calendar. General production/market/storage logistics and original nonempty-SAV
import are not supplied by this patch.

A retained high-grade house with positive material demands demonstrates why
that boundary matters: a strict direction0 candidate can debit materials again
at frozen monthPart0/7. Before committing that candidate,
`planNativePhaseHouseEvolution` compares all12 Goods words. If C1 would change
any word, it discards both candidate building and candidate context, returning
`native_city_material_schedule_unmapped`. Phase39 isolates only that house and
restores its full record/context links; other supported houses and phases keep
running. This guard protects stock but **does not complete material consumption**.

The shared query and presenter expose the same reason:
“物资消耗时序尚未接通，保持当前住宅和库存；其他已支持的供水与升降级继续运行。”
The query is byte-preserving. This is not a blanket downgrade prohibition:
the existing admitted water-only type4 dry downgrade and positive-delay rewater
reset still execute, with their own explicit evidence/grade restrictions.
Legacy non-C candidate behavior is unchanged.

### Concentrated validation and remaining scope

All scores below are bounded native/authored execution over source data, not
new observations of the original game. Original strict simulation remains0/6.
No new original-game experiment, ASan run or push was performed.

- Codec1023/1023, normalized-Goods138/138 and old population-wire578/578 pass
  in Release and UBSan. Their actual compiler dependencies still match final
  source. Independent old wire1–5 goldens and the large C packet remain exact.
- Final C contract1515/1515 in each mode covers template poisoning, all10
  selected restore phases, exact schemas, inventory construction-versus-load,
  policy retention, missing Goods, notice initialization, mirror rejection and
  failed-operation rollback. It includes103 ticks/three phase39 material holds,
  checking type/model/61-byte tail/population/shape/Goods/worker invariance,
  pure query/presenter agreement and continuing other-house/phase25 work.
  C water-only dry and rewater candidates match actual runtime outcomes.
- Two admitted empty SAV sources each pass33974 checks in Release and UBSan:
  1760 native updates,51/51 exact next-phase save restorations,35 passes each
  of availability/food/employment, natural type11 arrival at tick1636 and
  Well staffing at1658, then102 more updates. These start from native-owned
  cities, not inhabited original-SAV continuation.
- Old B Release paths pass9137 checks/4937 updates/62 restorations per source.
  Final legacy core173481 passes per mode; old A query770 and dry/rewater268
  and53588 checks pass per source. Existing accepted packages remain separate.

The real-source runner's conservative sourcewide flag retained a failure when
App-only reason text changed during the CLI runs. The raw reports are preserved.
Their actual compiler dependency hashes match the final CLI version; the final
query/presenter suite and App were compiled afterward. The separate compiled-
input binding records this distinction instead of rewriting an old report.

Use `make native-city-context-test` or `make native-city-context-test-undefined`.
`scripts/native_city_context.py` accepts repeatable relative `--source` paths
and an optional new absolute `--build-dir`. Real inputs are read only from the
per-process `EMPEROR_GAMEDATA_ROOT`; unset roots report SKIP. Outputs default to
a unique local test directory; no original input is modified. GUI package and
source binding evidence is retained outside Git and summarized in STATUS.md.


The final independent C1 App binary is
`077fe349f7a91902eabb5a509d5b63474f9dda05c5b8f4f7614036b5e073132a`.
Its reviewed GUI-build snapshot contains148 production files and31 compilation
units, with a valid strict signature. CPU285/285 and Metal279/279 actual GUI
checks cover both sources: fresh explicit C1 creation, real Well deletion and
reconstruction, live type11 action7 motion/exact in-flight restoration,
source-bound action8 actual arrival, scheduled-worker feedback, pure conditions,
continued updates, two window sizes, save/reload and real Cmd+Q exit0.

CPU acceptance resumed the same paused App after two driver errors: a hidden
legacy-label binding, then an undefined resume-only variable. Both failures and
the earlier passed checks are retained separately; final counts include executed
resume checks. The corrected check selects the visible Well category and reads
its real worker summary instead of a retired hidden widget. The production App
was unchanged. Metal runs the full corrected driver from fresh launch. Long
natural arrival/staffing paths are the separate1760-update C1 CLI checks; GUI
uses source-bound checkpoints for route/approach. Earlier B4937-update evidence
is a regression result, not additional C1 runtime steps.

After both GUI routes exited, only README/STATUS/this document were updated.
The signed App and its embedded README/manifest retain the exact GUI-build
version, while current repository documentation records the final outcomes.
At the C1 freeze, all runtime sources matched that148-file build snapshot;
the later README wording was the only difference from its production-source list. Frozen GUI-build and
current documentation/source snapshots are retained separately outside Git.
No accepted earlier App was replaced.


## Warehouse transfer conservation (2026-10-27)

This development-source repair does not replace the frozen C1 App above.
`originalFigureWarehouseDeliver` returns **actual accepted units**, including a
physical partial fit; `originalFigureStorageWithdraw` returns **units not
removed**, so cargo acquired is request minus return. `nullopt` rejects an
unsupported or invalid transfer before any stock write. A valid zero result is
a refusal/no-op, and resource0 (empty bay identity) cannot carry positive stock.

The counterexample has eight400-unit Warehouse bays: seven full of another
resource, the last requested-resource bay at399, policy headroom at3200 and
incoming cargo2. Previously the helper credited1 then returned0, leaving cargo2.
Now it returns1, with stock delta1 and retained cargo1. The same accepted-prefix
contract applies to the existing Mill and trade-station delivery paths; known
bay counts/capacities, policy gates, first-tie selection and represented unsigned
Goods arithmetic are unchanged. No market capacity/quality rule is inferred.

Type19 commits accepted stock and residual cargo together; Mill reservations
release the original incoming cargo before any next reservation. Type21/22
home-return still requires full acceptance: a partial local candidate is
refused, not partially committed or silently discarded. Type21 withdrawal
creates only the cargo actually removed. Warehouse exporter allocation precedes
withdrawal, so an exhausted/failed allocator cannot debit stock. Raw hook and
allocator functions mutate an owned candidate and may leave a prefix on
failure; atomicity belongs to the public phase/timeline transaction, not these
lowlevel mutators. Figure updates and ordered-pass failures publish no partial
stock, cargo, route, reservation or FIFO candidate.

The combined authored figure suite passes3369/3369 in Release and UBSan,
including784 table cases for physical/policy headroom, full/partial/zero
transfers, Warehouse/Mill/station pools, clear/retain identity and unsigned
withdrawal. Actual caller regressions cover Mill partial reservation release,
Warehouse residual cargo, image-failure retry, type21/22 partial-return refusal,
allocator failure and later-slot transaction rollback. The standalone
counterexample changes from returned0/physical+1 to returned1/physical+1.
Real-data corpus tests report SKIP with an unset root; these authored checks
are not original-game observations or additional C1 GUI acceptance.

General market replenishment/type24, food quality, unsupported Mill withdrawal,
partial home-return continuation and original calendar/material consumption
remain outside this repair. Existing explicit refusals remain. No new format,
UI entry, production capacity or phase is enabled.

## Policy-aware committed housing feedback (2026-10-28)

`native_population_feedback.hpp` indexes the existing house reservations,
type11/type13 payloads, Well owners and strict-record references once per
query. It adds no wire fields or scheduler. Incoming-group policies show the
actual departure/road/entry action and reserved people before capacity or
structural information. Full and pending-excess houses remain distinct.
Cleared or demolished targets cannot retain a displayed reservation. Missing
or inconsistent records produce an explicit unavailable result.

The shared housing/category/detail/hover consumers use the saved phase28 water
cache independently of decayed service bytes. The empty meal-arrival list of
the incoming-group policy is never interpreted as a per-house food refusal.
Meal and tax planners still describe their own native accounting; existing
food receipts and C1 job assignments are separate from actual people arriving.
Earlier v1–v6 and untagged/earlier-v8 modes retain their meal explanations.
Well feedback distinguishes access, workers, dispatch, movement, held routes
and return; C1 saved staffing is identified separately from older derived
native staffing. No query advances RNG, actors, routes, inventory, revision or
rules version. Repeated query/description calls must preserve encoded bytes.

The concentrated Release and UBSan feedback suites each pass 2,232 checks:
374 authored legacy checks, 272 checks over the two admitted original sources,
and 1,586 checks over 24 existing native checkpoints, including saved outward
and returning water walkers. Synthetic cache, action,
population/overcapacity and invalid-reference fixtures are explicitly authored
native boundary cases, not new original-game observations. Actual native
target-demolition transactions are queried after cancellation. The root-unset
branch still returns SKIP. GUI acceptance and package/source bindings are
recorded separately in STATUS; this repair does not complete the original
calendar, economy, firefighting or unsupported house grades.

## Explicit preventive Inspector strategy: RUNTIME8 7 (2026-10-28)

This additive native strategy is `native_inspection_prevention_v1`. It requires
the existing C1 city contract and enables Inspector Tower124 / Figure39 only
for the explicitly selected development city mode. Default v6 and runtime1–6
are unchanged; loading an old archive does not enable this strategy. It still
constructs a new native pool over an admitted empty original source, not an
import/continuation of an inhabited original city.

Runtime7 appends a required optional-valued `inspection` block after
`cityContext` and before the existing per-object projection fields. It contains
named fields: `payloads`, `maximum` (two signed process maxima), `decayCounter`,
`dispatches`, `returns`, `services`, and `visits`. Each visit stores native tick,
figure slot and both counter values before/after an actual successful service
callback. `NATIVE-INSPECTOR 1` reuses the complete named Figure member traversal
under a separate magic/type check; it never disguises39 as a Well walker.
Unknown versions, missing fields, unsupported actions/grades, invalid ownership
or graph/payload budgets refuse; integer narrowing is checked. Existing outer
ENCITY syntax permits an optional final newline; losing a mandatory runtime
end marker or field is a different, invalid truncation.

Tower identity, footprint, cost and funds remain native object/ledger authority.
Cost, workers, reduction, walking speed and roam length come from the same
source/model revision as the city. The canonical strict record owns model,
upgrade, risk words, primary/auxiliary references, dispatch counter and direction.
Workers remain phase25 authority; query/render never allocates them. The new
mode adds124 to worker-key capture and validation without changing old object
sets. The explicit native Hunter economy is retained; its availability consumer
gets a native-constructed zero-stock production view, not fabricated original
production. A new tower has a declared native-zero-initialized hook payload.

Inspector creation uses the existing shared constructor, serial/RNG/speech,
cell chain and FIFO. Full staffing still requires the actual counter threshold;
the action6 countdown, action7 departure, action8 roam, action9 return and
owner-cell cleanup follow the existing supported service updater. Inspector
auxiliary creation/checking and cleanup consistently use its +6C slot; Warehouse
+6A behavior is unchanged. A model downgrade cancels only the auxiliary through
normal reciprocal cleanup before the basic-owner gate could orphan it.

Two native adaptations are explicit: tower return-road availability is derived
from the computed overlay's adjacent-road selection rather than blindly kept
at constructor zero, and deleting a native road first cancels current patrols
with normal route/cell/owner/FIFO cleanup. Risk and historical visits remain;
later phase31 can redispatch. These are not new claims about original live-edit
or constructor semantics. No force removal, unknown fire route, or silent held
Inspector is substituted for a complete loop.

Actual clipped, occluded service callbacks reduce both signed accumulated risk
words and write serviceGrid bits3–5. Workers, proximity to a tower and an old
grid bit do not certify a house visit. Neither the risk words nor the truncated
Figure watchCoverage byte are probabilities. `maximum` and the phase45 decay
counter are explicitly native-fresh zero state, then persist exactly across
rebuild/save/reload. Every eighth phase45 call decays existing packed lanes;
the phase sequence remains the labeled native adaptation with the calendar
tail/month state held, not a proven original wall-clock/day schedule.

The complete candidate includes risk growth, services, staffing, population,
goods, RNG and graph state. Positive disaster triggers, active fire targeting,
unknown special service paths and unported watch-risk branches refuse without
committing partial effects. Active firefighting, fire spread with process wind,
building replacement by ruins and continued disaster recovery are unsupported.
The App pauses on such failure and explains the boundary; it does not erase
risk, reset fireCount or report that disasters are implemented. GUI terminology
distinguishes employed/pending-dispatch/in-flight/returning and a historical
actual visit. Selecting a tool or inspecting an object changes no simulation.
