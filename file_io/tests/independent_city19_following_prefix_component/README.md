# Independent following fixed prefix / zero vectors properties

16 cases per Release/UBSan build using actual predecessor walkers and synthetic bytes. Tests cover arbitrary nonzero partial35 prefix, twelfth-prefix incomplete status, opaque raw patterns not inferring getter type/null, all35 cuts,12x35 versus56 destination stride, header ownership, six positive count groups before typed tag, partial27th vector,702B zero-vector model, counts/budgets and predecessor refusal.

From repository root on macOS with Command Line Tools/Python3:

```sh
RUN_ROOT="$(mktemp -d /tmp/city19-following-prefix-properties.XXXXXX)"
python3 file_io/tests/independent_city19_following_prefix_component/run_properties.py --source-root "$PWD" --output "$RUN_ROOT/direct"
```

Explicit source-root, fresh output and unoptimized Python; driver180s total/build60s/process10s. Source paths/hashes refer to fixed9277994 review bundle and supplemental getter checkpoint. Same immutable input metadata is a caller precondition, not a crypto-origin token.35 is a prefix when presence is nonzero; getter returns existing+30 pointer, initialization/currenttype/vptr/payload unknown. Postload noarchive-call proof covers two closed functions under validstorage/noalias assumptions, no original execution. Real vectors not entered; empty models do not prove original destructor/reset/alias/lifetime/fullcity pass. No EXE/save/REA/device actions.
