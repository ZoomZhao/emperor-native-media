# Independent mainpool Building v5 component properties

20 cases per Release/UBSan build, synthetic input only. Tests cover shared cache, inactive and duplicate sequence items, class/object references,4001 complete records, modern checkpoints, signed extremes, exact budgets, exhaustive182 prefix truncation cuts and unread next-consumer tail. No original EXE/save/runtime/hook or new REA query.

From repository root on macOS with Command Line Tools and Python3:

```sh
RUN_ROOT="$(mktemp -d /tmp/city19-mainpool-properties.XXXXXX)"
python3 file_io/tests/independent_city19_mainpool_component/run_properties.py --source-root "$PWD" --output "$RUN_ROOT/direct"
```

Explicit source-root, fresh output and unoptimized Python required. Driver total180s/build60s/process10s. test-expectations.json maps selected static paths and modern subset/resource policies, with source paths/hashes referring to the fixedd596187 review bundle.4000 is a saved sample count and source padding threshold, not an inferred global upper bound. Source metadata must come from the same immutable input. Full original runtime instance/cache/vptr/lifecycle/business compatibility remains unproved.
