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
| 38 | `SprMain` | 39 | `SprMain2` |
| 40 | `SprMain3` | | |

**Consequence: ground tile codes `0x600`–`0x6FF` resolve inside `China_Terrain.sg3`.** This is the
missing `MAP cell → SG3` link. The codes the engine actually uses for ground are
`0x601, 0x603, 0x604, 0x605, 0x60B, 0x60C, 0x60E, 0x60F, 0x612, 0x613, 0x614, 0x616, 0x61A,
0x61B, 0x61C, 0x61D, 0x61E, 0x61F, 0x620, 0x621, 0x623, 0x624, 0x625, 0x627` (24 codes), plus the
non-ground neighbours `0x401`, `0x451`, `0x452`, `0x4BB`.

### SG3 archive header and record layout

`China_Terrain.sg3`: version `213`, capacity 10,000, image `count` 1,443, 8 bitmaps; header/record
table starts at file offset 40,680, record stride 64 bytes, records addressed directly by
`image_id` (`offset = 40,680 + image_id * 64`, id 0 unused). Bitmap names start at offset 680
(200 bytes each): `Zeus_system.bmp`, `China_land1.bmp`, `China_Overlay.bmp`, `China_Land3.bmp`,
`China_Trees.bmp`, `China_QuarryTileSet.bmp`, `China_land2.bmp`, `Pinnacle.bmp`.

Verified record fields (byte offset → meaning):

| Offset | Width | Meaning |
|---:|---:|---|
| 0 | u32 | absolute pixel-data offset in the file |
| 4 | u32 | stored length |
| 8 | u32 | uncompressed base length (3,200 for terrain records) |
| 20 | u16 | canvas width (`78` for single-cell terrain) |
| 22 | u16 | canvas height (40–83 for single-cell terrain) |
| 24 | u16 | sheet x |
| 26 | u16 | sheet y |
| **28** | **u16** | **ordinal — zero for plain records, `1..55` for the group bases** |
| 50 | u8 | type (`30` = terrain/ground) |
| 51 | u8 | compression flag |

### The link: header slot == record ordinal

Header index at file offset 80, 300 × u16, **55 non-zero slots**. Each slot names a record id, and
that record's ordinal field at byte 28 **equals the slot number** — verified **55/55**:

| Slot | Id | Ord | W×H | | Slot | Id | Ord | W×H |
|---:|---:|---:|---|---|---:|---:|---:|---|
| 1 | 201 | 1 | 78×40 | | 29 | 646 | 29 | 78×40 |
| 2 | 202 | 2 | 78×41 | | 30 | 782 | 30 | 78×40 |
| 3 | 247 | 3 | 78×41 | | 31 | 800 | 31 | 78×40 |
| 4 | 336 | 4 | 78×47 | | 32 | 869 | 32 | 78×40 |
| 5 | 386 | 5 | 78×40 | | 33 | 878 | 33 | 78×40 |
| 6 | 458 | 6 | 78×52 | | 34 | 1066 | 34 | 78×40 |
| 7 | 472 | 7 | 78×51 | | 35 | 849 | 35 | 78×40 |
| 8 | 486 | 8 | 78×50 | | 36 | 927 | 36 | 78×72 |
| 9 | 1442 | 9 | 398×468 | | 37 | 945 | 37 | 78×83 |
| 10 | 1441 | 10 | 398×544 | | 38 | 1006 | 38 | 78×40 |
| 11 | 500 | 11 | 78×40 | | 39 | 963 | 39 | 78×50 |
| 12 | 501 | 12 | 78×40 | | 40 | 972 | 40 | 78×40 |
| 13 | 1440 | 13 | 398×520 | | 41 | 542 | 41 | 78×40 |
| 14 | 1138 | 14 | 78×45 | | 42 | 306 | 42 | 78×40 |
| 15 | 1126 | 15 | 78×59 | | 43 | 321 | 43 | 78×40 |
| 16 | 505 | 16 | 78×42 | | 44 | 1152 | 44 | 78×40 |
| 17 | 515 | 17 | 78×56 | | 45 | 1176 | 45 | 78×40 |
| 18 | 525 | 18 | 78×58 | | 46 | 1200 | 46 | 78×40 |
| 19 | 533 | 19 | 78×52 | | 47 | 1224 | 47 | 78×40 |
| 20 | 541 | 20 | 78×40 | | 48 | 1248 | 48 | 78×40 |
| 21 | 554 | 21 | 78×42 | | 49 | 1272 | 49 | 78×40 |
| 22 | 556 | 22 | 78×42 | | 50 | 1296 | 50 | 78×40 |
| 23 | 564 | 23 | 78×42 | | 51 | 1320 | 51 | 78×40 |
| 24 | 566 | 24 | 78×42 | | 52 | 1344 | 52 | 78×40 |
| 25 | 1443 | 25 | 398×475 | | 53 | 1368 | 53 | 78×40 |
| 26 | 574 | 26 | 78×40 | | 54 | 1392 | 54 | 78×40 |
| 27 | 736 | 27 | 78×40 | | 55 | 1416 | 55 | 78×40 |
| 28 | 664 | 28 | 78×40 | | | | | |

**Therefore the tile code is `(tileset << 9) | ordinal`, and the sprite for a code is the record
whose ordinal field equals the code's low 9 bits.** Ground code `0x603` (bare/cleared ground) is
`China_Terrain.sg3` record ordinal 3 = id 247, 78×41, type 30.

A group base is followed by its variant records (a row of ~8 consecutive ids laid out on the same
sheet row), which is exactly what `tileImage(code) + variant` selects. Slots 9, 10, 13 and 25 are
large composite records (398 px wide) rather than single-cell ground tiles.

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

## Terrain flags → ground code (the complete dispatch chain)

The ground code for a cell is chosen by a **priority chain** in `FUN_0053EC90 @0x0053EC90`, which
runs the setters below in order and stops at the first that returns non-zero. Every setter writes
`0x00FE9880[cell] = tileImage(code) + variant` and marks the cell drawn (`0x00FDCD70 |= 0x40`).

| Order | Setter | Terrain test | Ground code |
|---:|---|---|---|
| 1 | `FUN_004EDC30 @0x004EDC30` | `& 0x100` (else `& 0x10000` special) | `0x61C + 0x30 + random % 0x18` |
| — | *(gate)* | only when `(terrain & 0x8C008) == 0` | — |
| 2 | `FUN_0053F240` | *not yet read* | — |
| 3 | `FUN_00540330` | *not yet read* | — |
| 4 | `FUN_0053FC30 @0x0053FC30` | `& 0x4` **and** `0x00FE9880[cell] == 0` | `0x61B` / `0x61A` / `0x605` / `0x61C` by shore context |
| 5 | `FUN_0053F520 @0x0053F520` | `& 0x1` | delegated to `0x004BC6A0` (`0x60x ± 9/7`) |
| 6 | `FUN_0053F660 @0x0053F660` | `& 0x2` | `0x606` / `0x607` (`& 0x300002 == 0x100002`) / `0x608` (`== 0x200002`) |
| 7 | `FUN_0053F790 @0x0053F790` | `& 0x101000 == 0x101000` | `0x60F` |
| 8 | `FUN_0053F560 @0x0053F560` | `& 0x80000000` | `0x60E` |
| 9 | `FUN_0053F840 @0x0053F840` | `& 0x20000` | `0x628` / `0x626` / `0x622` (or `0x603` when `5 < level+? < 0x22`) |
| 10 | `FUN_0053FA20 @0x0053FA20` | `& 0x2000000` **and** `0x00FE9880[cell] == 0` | `0x60D` / `0x60A` / `0x609` / `0x619` by `0x00F37DA0` bits 8/10/20/40 |
| 11 | `FUN_00413A00 @0x00413A00` | *not yet read* | — |
| 12 | `FUN_00540360 @0x00540360` | `& 0x1000` | `0x612 + (random & 7)` |
| 13 | `FUN_00540300 @0x00540300` | `& 0x40` | delegated to `0x004B6F90` |
| 14 | `FUN_0053FB30 @0x0053FB30` | `& 0x10000` **and** `(terrain & 0x48) == 0` | `0x61B` (+ optional `FUN_004BCE20()`) |
| 15 | `FUN_005400F0` | *not yet read* | — |
| 16 | `FUN_00540140 @0x00540140` | `& 0x20` | `0x401 + 2..7` (tileset 2, `China_General`) |
| 17 | `FUN_0053EE00 @0x0053EE00` | `(terrain & 0x10001) == 0` **and** `& 0x80` | `0x602` / `0x604` |
| 18 | **`FUN_0053F090 @0x0053F090` (fallback, plain land)** | `(terrain & 0xAEFEDE6F) == 0` | **`0x603`** `+ (random & 7)` |

Additional writers outside the chain:

* `FUN_0053EAA0`/`FUN_0053EAE0 @0x0053EAA0` — sets `0x601` when the cell is higher than a neighbour
  and the cell level is 0 (`0x00F1E780`-independent).
* `FUN_0042C480 @0x0042C480` — building cleared: `0x603 + (random & 7)`.
* `FUN_004B5E50 @0x004B5E50` — `0x612 + (random & 7)`.
* `FUN_004EDBA0 @0x004EDBA0` — `0x62B` (`& 0x100`) / `0x62A`.
* `FUN_0046F9B0 @0x0046F9B0` — off-map cells: `0x603 + (x + y) % 0x3A`.

### Plain land is code `0x603`

`FUN_0053F090` is the last link of the chain, so anything that reaches it is ordinary land. It emits
code `0x603` and, when the cell belongs to a large uniform block (`random & 0xF8`, `& 6`, `& 4`, `& 2`),
it switches to the 2×2 / 3×3 / 4×4 block art using larger variant offsets
(`+8 + (random&3)*4`, `+0x18 + (random&1)*9`, `+0x2A`). Those offsets are the only reason the drawn
record is not always exactly the ordinal record; they walk the sheet's 9-wide rows, so the variant
space is 0…0x39 rather than 0…7.

### Derived rule for a native renderer

```
code  = 0x600 | ordinal                       // ordinal from the table above
image = China_Terrain.sg3 record(ordinal) + variant_offset
```

Because the code's low 9 bits *equal* the record's own ordinal field (verified 55/55), the chain
above is a complete, testable `MAP → SG3` contract:

| Terrain | Code | Ordinal | `China_Terrain.sg3` id |
|---|---|---|---|
| plain land (fallback) | `0x603` | 3 | 247 (78×41, type 30) |
| land, high vs neighbour | `0x601` | 1 | 201 (78×40) |
| `& 0x80` fertile/irrigated | `0x602` / `0x604` | 2 / 4 | 202 (78×41) / 336 (78×47) |
| `& 0x2` | `0x606`/`0x607`/`0x608` | 6/7/8 | 458 (78×52) / 472 (78×51) / 486 (78×50) |
| `& 0x1000` | `0x612` | 18 | 525 (78×58) |
| `& 0x10000` | `0x61B` | 27 | 736 |

## The stored surface grid is an oracle, not a derivation

The engine does **not** compute `0x00FE9880` from terrain alone. The per-cell surface
image is part of the city `.map` payload and is copied into the grid by the loader at
`FUN_0052E7C0 @0x0052E7C0`, which reads a fixed sequence of flat 228² grids in this
order (logical payload offsets; `original_terrain.hpp` carries the same table):

| Payload offset | Target VA | Width | Grid |
|---:|---|---:|---|
| 1,535 | `0x00FE9880` | u32 | **surface image id** |
| 209,471 | `0x00FDCD70` | u8 | draw-state flags |
| 261,455 | `0x00F6A9E0` | u32 | terrain flags (equals the MAP terrain field) |
| 469,391 | `0x00F37DA0` | u32 | feature/orientation bits |
| 677,327 | `0x00F9D620` | u8 | sub-tile / object index |
| 729,311 | `0x00F1E780` | u8 | per-cell random byte |
| 781,295 | `0x00F05160` | u8 | unmodelled |
| 833,279/833,283 | `0x0101D0AC`/`0x0101D0B0` | u32 | unmodelled pair |
| 833,287 | `0x00EAC3F0` | u8 | unmodelled |
| 885,271 | `0x00E92DD0` | u8 | road distance |
| 937,255 | `0x00E9F8E0` | u8 | fertility |
| 989,239 | — | 0x24 B | blob |
| 989,275 | `0x00BEBF3A` | u8 | unmodelled |
| 1,041,259 | `0x00BF8A4A` | u8 | unmodelled |
| 1,093,243 | `0x010C6F18` | u32 | unmodelled |
| 1,093,247 | `0x010C6DB0` | 0x168 B | blob; **flat run ends at 1,093,607** |

The grids abut exactly, so the first grid starts immediately after the 1,535-byte city
header and no other record separates them. Because `0x00FE9880` arrives pre-filled, the
ground chain is a setter that may or may not overwrite it: a cell the chain does not
write **keeps its loaded art**. That refutes the earlier "determinate no-ground"
reading of the `0x8C008` gate and of the other declining branches; the repo now calls
them `chain-declined` / `chainNoWrite` (`original_ground.hpp`).

Measured over all 167 `Cities/*.map` (whole crop, camera-free):

* grid `2,107,836` active cells, `28` stored zero, `2,107,808` stored nonzero;
* chain emits `1,514,539`, agrees with the stored grid on `1,299,617` = **85.809 %**;
* chain declines to write on `114,937` cells: `0` of them store zero, **all `114,937`
  store nonzero art** — direct evidence that "declined" is not "empty";
* unknown / non-terrain-determinable chain cells `478,360`.

### Record indexing in a version-213 archive

Each stored word is `v = (tileset << 14) | low14`. For a version-213 `.sg3` the loader
publishes header value `h` as entry `h - 201`, so the record id is
**`recordId = low14 + 201`** (0-based sprites index `recordId - 1`). `China_Terrain.sg3`
has `imageIndex[0..16] = [0, 201, 202, 247, 336, 386, 458, 472, 486, 1442, 1441, 500, …]`
and a contiguous type-30 run `201..1443` (1,238 records); ids `1..200` are types 256/276
(palettes, not art). A `0` or `200` base is refuted: base 200 maps tileset-8 low `0` to
id `200`, a type-276 palette. All six referenced archives declare `imageIndex[1] = 201`
(`China_Unloaded` alone is 205).

### Tileset index → archive (stored grid)

`FUN_00475B60 @0x00475B60` assigns the archive name pointers; the stored-grid renderer
loads the archive the name table names. Geometry cannot pick the archive — several
version-213 archives share the same 201-based type-30 run — so the name is the evidence.

| Tileset | Archive | Records | Stored cells (167 cities) |
|---:|---|---:|---:|
| 2 | `China_General.sg3` | 2,941 | 2,388 |
| 3 | `China_Terrain.sg3` | 1,443 | 1,981,998 |
| 5 | `China_Mon_Tumulus.sg3` | 577 | 617 |
| 6 | `China_Mon_Grand_Canal.sg3` | 322 | 4,224 |
| 8 | `China_Mon_Earthen_Greatwall_1.sg3` | 242 | 12,416 |
| 16 | `China_Elevation.sg3` | 372 | 106,165 |

Before the mapping, tilesets 2/5/6/8 were `archive_unavailable` (`19,645` cells). After
it, `archive_unavailable = 0`, decoded rises `2,063,845 → 2,066,203` (97.914 % →
**98.026 %**), and `geometry_unsupported` rises `24,318 → 41,605`: of the 19,645 cells,
`2,358` decode as single-cell type-30 art (1,741 in tileset 2, 617 in tileset 5) and
`17,287` are records the single-cell path refuses (647 in tileset 2 = 228 composite
type-30 + 419 non-type-30 object art, all 4,224 tileset-6 cells across 3 318×160
records, and all 12,416 tileset-8 cells across 37 such records). Those are real stored
art, not empty slots, and stay `geometry_unsupported`, not silently dropped. `5/167`
cities reach a fully single-cell render (`MP13`, `MP25`, `MP28`, `SA-Qufu`, `Baoji`).

## Open items

1. Read the five setters still marked *not yet read* (`FUN_0053F240`, `FUN_00540330`, `FUN_00413A00`,
   `FUN_005400F0`, and confirm `FUN_004EDC30`'s `0x10000` branch) to complete the table.
2. Confirm the enumeration order assumption: registration walks the header index in slot order, so
   entry `k` ↔ slot `k` ↔ record ordinal `k`. Evidence: the 55 records' own ordinal fields match
   their slots exactly, and the tie-break `entry[0]` ⇔ slot 1 ⇔ id 201 is consistent with
   `tileImage(0x601) == 201 - 201 + 3*0x4000`.
3. Confirm `0x00F1E780` is filled from the MAP random grid (logical payload offset 729,311).
4. Port the table into `game_lab` and emit image ids for all active Anyang cells (M2/M3).

The MAP → SG3 join count in `STATUS.md` is unchanged until the port reproduces a city end to end.
