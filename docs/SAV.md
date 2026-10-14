# SAV source identity and bounded diagnostic spans

This is a read-only source audit, separate from the city MAP reader and frozen CitySimulation/EMP1. It does not restore buildings, walkers, game rules or a graphical city. Inputs come only from `EMPEROR_GAMEDATA_ROOT`; unset/empty roots succeed with SKIP. No automatic save is requested or generated.

The pinned GPL2-or-later modern [mapper's role branch](https://github.com/bvschaik/citybuilding-mappers/blob/bb97d7c7edf7c6608d5e9d3d6d5b63ba3686cda6/emperorfile.cpp#L56-L69) recognizes logical prefix13,00 as SAV. Its [saved branch](https://github.com/bvschaik/citybuilding-mappers/blob/bb97d7c7edf7c6608d5e9d3d6d5b63ba3686cda6/emperorfile.cpp#L113-L151), [size marker](https://github.com/bvschaik/citybuilding-mappers/blob/bb97d7c7edf7c6608d5e9d3d6d5b63ba3686cda6/emperorfile.cpp#L813-L835) and [after-pattern pointer contract](https://github.com/bvschaik/citybuilding-mappers/blob/bb97d7c7edf7c6608d5e9d3d6d5b63ba3686cda6/gamefile.cpp#L262-L265) provide format facts. No mapper renderer, variable building/walker parser or original game code is copied or executed.

| Independently checked fact | Audit interpretation |
|---|---|
| Outer u32 marker0xFEDCBAAA; framed marker0xABCDEFFF | Complete zlib framing/checksum, lengths and EOF checked before publishing grids |
| Logical prefix13,00 | SAV role only; remaining header/version meanings stay opaque |
| `Brief descriptio`16-byte marker | A valid u32 side1..228 at marker-start−16; one valid side anchor required |
| Published16-byte terrain marker00000000050008000500080005000800 | Terrain starts marker-start＋4, after the size field; duplicate terrain anchors refused |
| Terrain | 51,984 LE u32 values,228×228; declared centered active crop must agree |
| Immediately following207,936-byte span skipped as an int grid | One explicit unknown u32 row-major diagnostic reshape, not an identified graphics field |

Input≤4 MiB, decoded≤8 MiB,≤256 frames, chunks≤32 KiB,≤1,024 marker hits and selected canonical cells≤128 are bounded. The audit has a conservative marker subset; alternate saved versions/size-marker fallbacks are unsupported, not guessed. City MAP offsets are used only to read the separately verified city catalog. SAV locations are derived from their own markers. Candidate terrain offsets across this local save corpus vary2,203,307..2,283,506; no fixed MAP offset is reused. Symlink/root escapes refuse, hidden `.sav` names are included, and each source is read twice with unchanged size/mtime checks. The catalog keeps only copied terrain arrays rather than retaining every entire MAP payload.

`make save-state-audit-test` runs authored fixtures using existing NumPy. The real audit imports NumPy only after a configured root is needed; unset/empty SKIP also works without site packages. `EMPEROR_GAMEDATA_ROOT=/absolute/path/to/GameData make save-state-audit` emits metadata and aggregates only. Optional `python3 scripts/save_state_audit.py --sample-manifest /absolute/local/cells.json` takes `{"cells":[[x,y],...]}` and reports zero/distinct counts, not pointwise dumps. Neither filename nor a city-name token determines identity; full51984-value terrain equality is tested against every accepted city MAP.

## Existing original-instance provenance

Read-only process metadata confirms the already approved PID12997's current working directory is its normal Applications installation. Its save root and the supplied GameData save root are distinct. Complete inventory is **8/8 instance saves and6/6 user-root saves**, including two hidden `.sav` files; **14/14** containers, SAV roles, size anchors and terrain/following spans verify, baseline0/14. They contain136..138 frames and4,430,547..4,510,746 decoded bytes. All **14/14** remain byte-equal across two reads and unchanged in size/mtime during the audit. Detailed filenames/times/headers remain local; no save or derivative enters Git.

Two files were updated during the current instance's run:

| Actual existing instance-relative path | Last modification UTC2026-10-14 | File bytes |
|---|---|---:|
| Save/zoom/开放游戏自动存档_2.sav | 06:22:33.365221 | 172,889 |
| Save/zoom/开放游戏自动存档_1.sav | 06:25:38.539034 | 172,899 |

Both files were originally created during the earlier02:47/02:50 run and subsequently updated; they are not newly created by this audit. The current-instance post-launch/pre-city inventory is preserved, so the update is measured rather than inferred from names. A historical226-side save also contains an `Anyang` token, demonstrating why token presence alone is insufficient.

These two saves independently declare side140, a centered44..183 backing crop and9,940 on-map cells. Their whole terrain arrays agree with static Anyang at **51,984/51,984 values each**, including all on/off-map flags. Across **167/167 city MAPs**, only Anyang has this exact array:source identity improves **0/2→2/2**, with1/167 possible matches per save. Installed and user-root Anyang MAP bytes also agree1/1. This proves the saved terrain source, not a contemporaneous screenshot or an effective SG3 reference.

For these two records the derived terrain offset is2,203,307 and following-span offset2,411,243. At the same nine frozen canonical cells from the spatial audit (four full-content matches including the prior control, five failures), the diagnostic following u32 value is **0 at18/18 selected values**. Both groups share it; the terrain value is also unchanged at every selected cell. This is falsifiable literal file data with a documented span boundary and explicit reshape. Its semantic relationship to MAP's byte1535 candidate is unproved, so different numbers cannot be called a change in the same graphics reference. Reusing the old48921 namespace difference gives **0/18 in-domain candidates**, not an accepted SAV rule, and no alternate constant, packed component, displacement or image ID is searched.

The latest save06:25:38 precedes the frozen original frame08:50:32; screenshot-instant state attribution remains **0/2**. No new save is allowed in this check, and a missing later snapshot is not replaced with an assumption. The saved terrain has no flag differences from static MAP; the initialization mechanism is unobserved. Whether loading rebuilds the effective ground image, where that reference is serialized (if at all), and whether a missing layer explains the screenshot remain unverified; confirmed rebuilt graphical grids **0** and complete MAP→SG3 joins **0/167**. The next format evidence must identify a persisted graphics-reference field or another documented current-state source before rendering a save as an original scene.

Authored tests pass **12/12** for complete/dynamic framing, unaligned offsets, unsigned/canonical boundaries, late corruption, trailing stream bytes, role and marker ambiguity, full-span/crop refusals, budgets, symlink escapes, hidden saves and content-based identity despite a misleading filename. Unset/empty roots **2/2 SKIP**. These fixtures are parser regressions, not original-data success. Original PID12997 remains open; no original input, capture, save, restart or close occurred.
