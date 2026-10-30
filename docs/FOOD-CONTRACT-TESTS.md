# Known food-contract composition tests

This independent suite exercises the existing, bounded self-authored APIs. It
requires no game assets or original saves and does not modify shared update
headers, state, codecs, App code or the build configuration. It is not a new food
runtime and does not certify an original-food gameplay loop.

Run from any directory, with an output directory outside the repository:

```sh
python3 scripts/test_original_food_contract.py --output /tmp/food-contract-check
```

The runner builds Release and UBSan with `-Wall -Wextra -Werror`, captures command
results, hashes the preflight dependency graph before compilation and checks it
again after execution. Each mode writes its result and source/binary binding in
`verification.json`. Build outputs remain outside Git. It uses the installed
`clang++` by default; no model service, network, ASan or original game is used.

## Explicit premises and actual composition boundaries

|Group|What is exercised|What is supplied, not inferred|
|---|---|---|
|Production and dispatch|Supported producer progress, labor/input gates; disconnected/FIFO-exhausted producer; ordinary type19 allocation and source debit|Models/workforce and a separate stocked-producer premise. Production progress is not converted into food stock by this test.|
|Carrier to storage|Existing type19 arrival wait, acceptance, reservation release and partial-capacity remainder; missing destination/image refusal|Already-created ordinary carrier at the known arrival boundary, bounded Mill/53 stock and authored image metadata. No fabricated path traversal from dispatch to arrival.|
|Limited storage protocols|Physical and policy limits, missing stock, accepted-count versus unfulfilled-remainder return contracts; invalid capacity/resource/policy/amount purity|Only the existing 400-per-bay storage API domain. This does not establish general original capacity, food recipes or market capacity.|
|Retail and retirement|Actual market service visits to two houses, per-resource conservation, quality/service marking, followed by supported primary/auxiliary retirement; repeated/late-failed cleanup|Market resource28 and quality are an independent premise. The retiring alive2/action2 boundary is explicit; no unimplemented movement/return is simulated.|
|Distribution refusal|Late second-house/model/grid/sentinel refusal, empty and saturated recipients; entire public service transaction remains uncommitted on failure|Existing public candidate APIs, not a test-only transaction wrapper.|
|Retail then cycle tail|Previously committed retail/retirement stays unchanged across supported tail success, monthly/calendar/forecast refusal, exception and retry; external annual notices roll back with context|Separate owned contexts share only supplied Goods. No production food conversion, live month/year producer or world restore codec is asserted.|

The critical missing edge is **Mill/source → type24/25 pickup/return → market**.
The suite never constructs that edge, equates raw200/raw228 to cargo, invents a
recipe, admits24/25 to a handler or enables food consumption. Existing market
stock and carrier arrival premises are visibly separate. In-memory copied input
replay is not save-file/checkpoint certification.

## Failure and integration contract

The suite verifies const-input candidate APIs, public multi-house/retirement
transactions and public ordinary/annual tail transactions. It does not claim a
single outer transaction spanning all these APIs: that is the concurrent kernel
work. A failed later tail must preserve prior committed market work; it must not
roll back an earlier commit or replay it. A refused public leaf must retain its
own actor/stock/quality/demand/service or date/health/messages write set.

Kernel integration still needs one committed world authority; producer/actor
pool and Goods mirrors at the actual stage boundary; slot/serial/route/cell/FIFO
validation; whole-world/scene/session publication; complete checkpoint restore;
and certified event/calendar inputs. Once those interfaces stabilize, use these
premises as focused regressions inside that outer transaction. A passing suite
is conditional authored evidence, not evidence that an ordinary player can yet
produce, buy, deliver and consume food for repeated full original cycles.

Existing v6/default/old-save code is untouched. Run the established compatibility
suite on the final integrated kernel version; this test alone is not a new
old-save compatibility claim.
