# Original executable: MAP cell → SG3 sprite pipeline (extracted facts)

This document records format and algorithm facts extracted from the user's **legally owned original
executable**, authorised by the repository owner on 2026-10-17 (`AGENTS.md`, clean-room bullet). It is
the first evidence that names the actual `MAP cell → SG3 image` contract, which the MAP, SG3 and PAK
data files do not contain.

Only aggregated facts, addresses and small data tables are recorded here. No decompilation listing,
disassembly text, original binary, original asset or derived binary is committed. The executable and
all derived material stay on the local machine, read-only.

## Provenance

| Item | Value |
|---|---|
| File | `Emperor[CH].exe` (inside the supplied application bundle) |
| Size | 4,820,992 bytes |
| SHA-256 | `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658` |
| Format | PE32 i386, image base `0x400000`, uncompressed |
| Sections | `.text` `0x401000`, `.rdata` `0x7a9000`, `.data` `0x814000` (raw 503,808) + BSS, `.rsrc` `0x1e5a000` |

Analysis tool: Ghidra 12.1.2 headless (OpenJDK 21), full-program auto-analysis followed by a
whole-program decompile pass (26,083 functions). Ghidra project directory must be **absolute**; a
path element starting with `.` is rejected. Example:

```
JAVA_HOME=/opt/homebrew/opt/openjdk@21 \
/opt/homebrew/Cellar/ghidra/12.1.2/libexec/support/analyzeHeadless \
  <ABS project dir> EmperorProj -import <ABS exe> \
  -scriptPath <ABS script dir> -postScript <script>.java
```

Re-running a script against the saved project uses `-process emperor.exe -noanalysis` instead of
`-import`.

## Per-cell model

The city is a **228 × 228 = 51,984-cell** backing grid, row-major, row stride `0xE4` (= 228).
The grid origin pointer is `DAT_0101D0C8`; live width/height are `DAT_0101D0C0` / `DAT_0101D0C4`.
A flat cell index is `y * 228 + x`. Every layer below is the same flat N-element array.

| VA | Type | Verified role |
|---|---|---|
| `0x00FE9880` | u32 | **surface / ground image id actually drawn for the cell** |
| `0x00F6A9E0` | u32 | runtime terrain flags; bit values match the MAP terrain field |
| `0x00F1E780` | u8 | per-cell random byte; supplies sprite variants |
| `0x00F9D620` | u8 | per-cell sub-tile / object index |
| `0x00FDCD70` | u8 | per-cell draw-state flags (`0x40`, `0x80`, …) |
| `0x00FC3750` | i16 | building / object id |

`FUN_004048E0 @0x004048E0` is the map reset: it zero-fills each parallel grid with element sizes
`0x32C40` (= 51,984 × 4) and `0xCB10` (= 51,984).

Elevation is a separate **byte** grid of 51,984 entries; `FUN_00474A70 @0x00474A70` validates it by
reading `param_1 + 2 .. param_1 + 2 + 0xCB10` and rejecting negative values ("Found A Bad Elevation").

## Ground sprite formula (verified at call sites)

```
surface_image[cell] = tileImage(code) + variant
```

| Site | Code | Variant |
|---|---|---|
| `FUN_0042C480 @0x0042C480` (building cleared → bare ground) | `0x603` | `random[cell] & 7` |
| `FUN_004B5E50 @0x004B5E50` | `0x612` | `random[cell] & 7` |
| `FUN_00421ED0 @0x00421ED0` | `0x61E` | none |
| `FUN_004EDC30 @0x004EDC30` | `0x61C` | `0x30 + random[cell] % 0x18` (24 variants) |
| `FUN_0046F9B0 @0x0046F9B0` (off-map cell) | `0x603` | `(screen_x + screen_y) % 0x3A` (58 variants) |

`FUN_00413960 @0x00413960` is the primitive "draw image id at (x, y)". The whole terrain pass is
`FUN_0046FF60 @0x0046FF60` (walks the visible cell window, reads `0x00FE9880`, dispatches), with
per-cell draw in `FUN_0046F9B0 @0x0046F9B0`.

## Code → image resolver

```
FUN_00408170 @0x00408170
  code = param
  if ((code & 0x1FF) == 0) return 0
  entry = table[(code & 0x1FF) - 1]          // FUN_004081D0 @0x004081D0, 8-byte entries
  return entry.field2 + (code >> 9) * 0x4000
```

* The table pointer is at `tilesetManager(0x01C42130) + 4`; the entry count is at `+0xCC38`.
* A tile code is therefore `(tileset << 9) | localIndex`, and the resolver tags the result with
  `tileset << 14`.
* The table is built at runtime while tilesets load, so the mapping is **not** a static table in the
  file: it is derived from the loaded `.sg3`/`.555` archives.

## Tileset code space

`FUN_005CCF70 @0x005CCF70` calls the archive loader `FUN_005CD1F0 @0x005CD1F0` with
`base = tilesetIndex << 9`, then verifies the loaded image count against a fixed 41-entry cumulative
count table in `FUN_005CCBF0 @0x005CCBF0`; a mismatch trips the "graphic file out of sync" check.

```
counts[41] = 0x0E, 0x2EB, 0x4DB, 0x638, 0x832, 0xA15, 0xC0B, 0xE03, 0x1002, 0x1202,
             0x1402, 0x1602, 0x1802, 0x1A02, 0x1C8C, 0x1E1E, 0x20AD, 0x2240, 0x241B,
             0x260D, 0x280D, 0x2A10, 0x2C16, 0x2E0D, 0x300D, 0x3208, 0x3406, 0x3605,
             0x3806, 0x3A05, 0x3C08, 0x3E05, 0x4005, 0x4205, 0x4407, 0x4608, 0x4807,
             0x4A09, 0x4CC3, 0x4ECD, 0x5007
```

Tileset order is fixed by the name pointer array assigned in `FUN_00475B60 @0x00475B60`
(names live at `0x82B17C China_Terrain`, `0x82B18C China_General`, `0x82B19C China_Interface`,
`0x82B1AC China_Unloaded`, …):

| Index | Archive | Index | Archive |
|---|---|---|---|
| 0 | `China_Unloaded` | 13 | `China_MapOfChina01` |
| 1 | `China_Interface` | 14 | `SprAmbient` |
| 2 | `China_General` | 15 | `China_Fonts` |
| **3** | **`China_Terrain`** | 16 | `China_Elevation` |
| 4 | `China_Mon_Temple` | 17 | `Destruction` |
| 5 | `China_Mon_Tumulus` | 18 | `China_Chinese` |
| 6 | `China_Mon_Grand_Canal` | 19 | `China_Korean` |
| 7 | `China_Mon_Splendid_Temple` | 20 | `China_Vietnamese` |
| 8 | `China_Mon_Earthen_Greatwall_1` | 21 | `China_Khitan` |
| 9 | `China_Mon_Earthen_Greatwall_2` | 22 | `China_Mongol` |
| 10 | `China_Mon_Earthen_Greatwall_3` | 23 | `China_Tibetan` |
| — | (11, 12 not yet enumerated) | 24 | `China_Xiongnu` |

**Consequence: ground tile codes `0x600`–`0x6FF` resolve inside `China_Terrain.sg3`.** This is the
missing `MAP cell → SG3` link. The codes the engine actually uses for ground are
`0x601, 0x603, 0x604, 0x605, 0x60B, 0x60C, 0x60E, 0x60F, 0x612, 0x613, 0x614, 0x616, 0x61A,
0x61B, 0x61C, 0x61D, 0x61E, 0x61F, 0x620, 0x621, 0x623, 0x624, 0x625, 0x627` (24 codes), plus the
non-ground neighbours `0x401`, `0x451`, `0x452`, `0x4BB`.

### `China_Terrain.sg3` slot → image id

Header index at file offset 80, 300 × u16, 55 non-zero slots:

```
 1:201   2:202   3:247   4:336   5:386   6:458   7:472   8:486
 9:1442 10:1441 11:500  12:501  13:1440 14:1138 15:1126 16:505
17:515  18:525  19:533  20:541  21:554  22:556  23:564  24:566
25:1443 26:574  27:736  28:664  29:646  30:782  31:800  32:869
33:878  34:1066 35:849  36:927  37:945  38:1006 39:963  40:972
41:542  42:306  43:321  44:1152 45:1176 46:1200 47:1224 48:1248
49:1272 50:1296 51:1320 52:1344 53:1368 54:1392 55:1416
```

## Terrain art is polymorphic

Terrain rendering is selected by a **style index** `DAT_0115F738` (observed values `0`, `0x11`,
`0x12`; i.e. at least 19 styles). Nineteen singleton objects live at `0x130F6CC … 0x130F6E0`
(stride 4); their vtables start at `0x7BA228` with stride `0x44` and are installed by constructor
thunks near `0x588100 … 0x5882B0`. The style table is at `0x85C2C8`. Wrapper
`FUN_0046FEE0 @0x0046FEE0` dereferences the object and calls vtable slot `+0x20`; the shared
implementation bodies are around `0x5889D0 … 0x588D90`.

This matters because the ground code for a cell is not a single global table lookup: several style
strategies (climate / map mode) can pick different China_Terrain codes for the same terrain flags.

## Other extracted tables

* **Orientation neighbour table** `@0x8280E8`, 4 entries × 12 bytes:
  `(0, −1, 1)`, `(0, −228, 228)`, `(0, 1, −1)`, `(0, 228, −228)` — the four isometric
  orientation pairs for the 228² grid (`±1` = horizontal, `±228` = vertical). Used by
  `FUN_00473800 @0x00473800` and `FUN_00473A70 @0x00473A70` together with orientation bits
  (`random[cell] & 3`).
* **Code-pair table** `@0x8282DC`: `0x2072→0x2063`, `0x2074→0x2065`, `0x2076→0x2067`,
  `0x2078→0x2069`.
* **Runtime river/edge variant table** `@0x828260` (initialised in `FUN_00473800`), keyed on
  `tileImage(0x2072/0x2092/0x2094/0x2074/0x2076/0x2078/0x2096/0x2098/0x209A/0x209C/0x209E/0x20A0)`.

## MAP-side correspondence

* The MAP `mapperRandom` byte grid (logical payload offset 729,311, 228²) has **256 distinct values,
  near-uniform** — consistent with being the source of the engine's per-cell random grid
  `0x00F1E780` that supplies sprite variants. Not yet proven.
* Off-map cells (terrain bit `0x80000`, index ≥ `0xFFF0000`) draw code `0x603` with a
  coordinate-derived variant.

## Open items

1. **Enumerate order vs SG3 index.** The loader assigns `code = (tileset << 9) | i` in the order the
   archive's image records are enumerated. Acceptance for M1: reproduce the exact drawn image id for
   one known Anyang cell.
2. **Precomputed or derived at draw time.** Establish whether the base terrain image is written into
   `0x00FE9880` at load, or computed during the draw pass — this decides whether neighbour context
   (shorelines) affects the base ground.
3. **Random source.** Confirm whether `0x00F1E780` is filled from the MAP `mapperRandom` grid.
4. **Port** the formula into `game_lab` and emit image ids for all 9,940 active Anyang cells, then
   compare against the original Anyang frame.

Until items 1–2 are closed, the `MAP → SG3` join count in `STATUS.md` is unchanged.
