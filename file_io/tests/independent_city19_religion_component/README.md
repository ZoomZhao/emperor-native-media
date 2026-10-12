# Independent Religion / Commerce component checks

Only self-authored buffers; no original EXE, save or game runtime. Ten cases per Release/UBSan build cover294/68/22 widths, raw20 opacity, two-byte Commerce, unread mainpool tail, checkpoint isolation, selected-version gates, truncation,65536B budget and preceding-section precondition.

From the repository root on macOS with Command Line Tools and Python3:

```sh
RUN_ROOT="$(mktemp -d /tmp/city19-religion-properties.XXXXXX)"
python3 file_io/tests/independent_city19_religion_component/run_properties.py --source-root "$PWD" --output "$RUN_ROOT/direct"
```

Always pass `--source-root`. The repository driver requires an explicit source root and rejects optimized Python execution before creating output. Fresh output required; total budget180s, build60s and process10s. `test-expectations.json` distinguishes static selected paths from modern subset/budget contracts; source paths/hashes refer to the fixed9d343cb review snapshot. Qualified preceding metadata must come from the same immutable input. This does not establish original runtime pointer/cache/business semantics or full save compatibility.
