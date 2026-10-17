# Per-cell ground branch: original-executable facts

Aggregated facts extracted (authorised clean-room, `AGENTS.md`, 2026-10-17) from the user's legally owned
`Emperor[CH].exe`, SHA-256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`
(4,820,992 bytes, PE32 i386, imagebase `0x400000`). Only offsets, tables and formulas are recorded; no
listing, binary or asset is stored. Tooling: Ghidra whole-program decompile + Capstone `.text` scans.

## Notation

| Name | Address | Role |
|---|---|---|
| `terrain[cell]` | `0x00F6A9E0` u32 | runtime terrain flags (values match the MAP terrain field) |
| `random[cell]` | `0x00F1E780` u8 | per-cell random byte (RNG-generated, see item 12) |
| `img[cell]` | `0x00FE9880` u32 | **surface image id written by the ground chain** |
| `sub[cell]` | `0x00F9D620` u8 | sub-tile index |
| `draw[cell]` | `0x00FDCD70` u8 | draw flags |
| `obj[cell]` | `0x00FC3750` i16 | building/object id |
| `feat[cell]` | `0x00F37DA0` u32 | runtime per-cell feature/orientation bits |
| `R` | `0x0101C538` u8 | river/edge sub-index |
| `C` | `0x0101C540` u32 | river/edge descriptor code |
| `B`,`S` | `0x00BEBF34`, `0x0101C53C` | edge block index / flag |
| `T(c)` | `FUN_00408170 @0x00408170` | tile-image base for code `c` (item 11) |

`cell = y*228 + x`; grid origin `0x0101D0C8`, stride `0xE4`, size 51,984. `T(c)=0` when `c&0x1FF==0`.
The chain `FUN_0053EC90 @0x0053EC90` calls `FUN_00425260`+`FUN_004EDC30` first; only if `FUN_004EDC30`
returns low-byte 0 **and** `(terrain & 0x8C008)==0` does it run the list below, stopping at the first
non-zero setter. Every setter that writes `img` also does `draw|=0x40` and `sub&=0xF0`.

## Setter chain (one row per setter)

| # | Setter `@0x` | Exact condition | Exact value written | Modelled |
|---:|---|---|---|---|
| 1 | `FUN_004EDC30 @0x004EDC30` | `FUN_004EDA50` true and mode `[0x010AF4F4]` allows (item 9) | `img=T(0x61C)+0x30+random%0x18` | no |
| 2 | `FUN_0053F240 @0x0053F240` | `B>=0 && C!=0` from cache arrays (item 1) | 2×2 block `T(C)`/`T(B)`-style river art | no |
| 3 | `FUN_00540330 @0x00540330` | `terrain&0x800` | **no `img` write** (item 5) | no |
| 4 | `FUN_0053FC30 @0x0053FC30` | `terrain&0x4` | `0x61B`/`0x61A`/`0x605`/`0x61C` (item 2) | no |
| 5 | `FUN_0053F520 @0x0053F520` | `terrain&0x1` → `FUN_004BC6A0` | `T(0x624|0x625)+k*9+random%9` or `T(0x627)+random&7` (item 3) | no |
| 6 | `FUN_0053F660 @0x0053F660` | `terrain&0x2` | `T(0x607/0x608/0x606)+random&7` by `terrain&0x300002` | yes |
| 7 | `FUN_0053F790 @0x0053F790` | `(terrain&0x101000)==0x101000` | `T(0x60F)+random&7` | yes |
| 8 | `FUN_0053F560 @0x0053F560` | `terrain&0x80000000` | `T(0x60E)+random&7` (2×2/3×3 block art) | yes |
| 9 | `FUN_0053F840 @0x0053F840` | `terrain&0x20000` | `T(0x628/0x626/0x622/0x603)+R+C` (item 7) | no |
| 10 | `FUN_0053FA20 @0x0053FA20` | `terrain&0x2000000` and `img==0` | `T(0x60D/0x60A/0x609/0x619)` by `feat` bits (item 8) | no |
| 11 | `FUN_00413A00 @0x00413A00` | none — 3-byte body | returns 0 (never handles a cell) | yes |
| 12 | `FUN_00540360 @0x00540360` | `terrain&0x1000` | `T(0x612)+random&7` | yes |
| 13 | `FUN_00540300 @0x00540300` | `terrain&0x40` → `FUN_004B6F90` | `0x61B`/`0x61D`/`0x435`/`0x491`/`0x621`/`0x61E`/`0x61F` (item 4) | no |
| 14 | `FUN_0053FB30 @0x0053FB30` | `terrain&0x10000` and `(terrain&0x48)==0` | `T(0x61B)+R+C` (+`FUN_004BCE20`) | no |
| 15 | `FUN_005400F0 @0x005400F0` | `terrain&0x40000` | **no direct `img` write**; delegates (item 6) | no |
| 16 | `FUN_00540140 @0x00540140` | `terrain&0x20` | `T(0x401)+2..7` (tileset 2, block art) | partial |
| 17 | `FUN_0053EE00 @0x0053EE00` | `(terrain&0x10001)==0` and `terrain&0x80` | `T(0x602)`+farm-block offset | no |
| 18 | `FUN_0053F090 @0x0053F090` | `(terrain&0xAEFEDE6F)==0` | plain land `T(0x603)`+block variant (item 10) | yes |

Extra writers outside the chain: `FUN_0053EAA0`/`FUN_0053EAE0 @0x0053EAE0` (`0x601`), `FUN_0042C480 @0x0042C480`
(`0x603+random&7`), `FUN_004B5E50 @0x004B5E50` (`0x612+random&7`), `FUN_004EDBA0 @0x004EDBA0`
(`0x62B`/`0x62A`), `FUN_0046F9B0 @0x0046F9B0` (off-map `0x603+(x+y)%0x3A`), `FUN_004BC800 @0x004BC800`
(`T(0x620)+random%9`).

## Task items 1–12

**1. `FUN_0053F240 @0x0053F240` / arrays `0x00BB92F4`,`0x00B866B4`,`0x00B53A74`.**
These are three consecutive u32 per-cell **runtime caches** (spacing `0x32C40`), not MAP bytes. Their only
writer is `FUN_0053EAE0 @0x0053EAE0`: it zeroes all three for the cell (`terrain &= 0xDFBFFDFF`), then if
`FUN_00473210(@0x00473210, &DAT_00825C28, FUN_00473170(cell,-1,1))` returns non-zero it copies
`C`, `B`, `S` into them. `FUN_00473210` matches the 8-neighbour **elevation-difference** pattern
(`FUN_00408cf0`, neighbour offsets `&DAT_007ACA48`) against a **static descriptor table at `0x00825C28`**
(row chosen by `DAT_0101D0D0/2`). So their values derive from a static exe table plus the elevation grid —
**there is no MAP logical-payload offset/stride for them.** `FUN_0053F240` reads
`B=0x00B866B4[cell]`, `C=0x00BB92F4[cell]`, `S=0x00B53A74[cell]`; if `B<0` or `C==0` it returns 0, else it
emits 2×2 block river/edge art and sets `terrain|=0x200` (and `|S`).

**2. `FUN_0053FC30 @0x0053FC30` (`terrain&0x4`).** Requires `terrain&0x4`. It sets `DAT_0101C560=cell`,
reads the singleton byte `[0x010AF4E8]` (`FUN_00425250`) and `FUN_004B9CC0(0x104)`, which sets
`DAT_0101C594` = the number of the 8 neighbours sharing bit `0x104`
(neighbour views at `0x00F6A650/4`, `0x00F6A9DC/E4`, `0x00F6A64C`, `0x00F6AD6C/70/74`). If
`[0x010AF4E8]==0` or `DAT_0101C594 != 8` the "`0x104` water" mode is forced off.
*Water mode off →* if `(terrain&0x10000)!=0`: `C<0` → `0x605`, else `0x61A`. Otherwise it scans the 8
neighbour object pointers (`0x0088FE80/84`, `0x00890214`, `0x008905A4/A0/9C`, `0x0089020C`, `0x0088FE7C`;
test `obj==0 || obj+0x18==0`), and a switch on `C` selects an orientation index `iVar8∈{0,2,4,6}`:
* `0x61B` value `= T(0x61B) + ((iVar8-4)&7 + 6)*3 + iVar7` (+1 if the two orientation flags
  `0x0101C5B8[...]` are set). Cases `C=0→4`, `4→6`, `8→0`, `0xC→2`, `0x10/0x18/0x1C` combine.
* `0x61A` value `= T(0x61A) + R + C` (default/`D_1` case).
* `0x605` value `= T(0x605) + R + C`.
*Water mode on →* (`LAB_0053FF8A`) and only when `terrain&0x4000000==0` or `terrain&0x100!=0`:
`FUN_004BC140()==8` → `T(0x61C)+random%0x18+(random&1)`, else `0x605+R+C`. When
`terrain&0x4000000!=0 && terrain&0x100==0`: re-run with `FUN_004B8F70(0x4000000)`; if `FUN_004BC140()!=8`
→ `T(0x61C)+random%0x30`, else `T(0x61C)+0x30+random%0x18`. `DAT_0101C594` is a neighbour-container count
(see item 8's `FUN_004EDA50`), not a MAP field.

**3. `FUN_0053F520 @0x0053F520` → `FUN_004BC6A0 @0x004BC6A0` (`terrain&0x1`).** Wrapper does
`sub&=0xF0`, `draw|=0x40`, then `FUN_004BC6A0`. Gate `(terrain&0xC064)==0`. If `terrain&0x100` and
the 8-neighbour count `DAT_0101C594==8` (`FUN_004B9CC0(0x100)`) it returns with **no** `img` write. Else, when `obj==0 && terrain&1 && !(terrain&0x80000)`:
call `FUN_004BD570` (value = `min(100, 0x00E9F8E0[cell])`). If `0x00E92DD0[cell]==0xFF` the base is
`T(0x624)`, or `T(0x625)` when **`feat[cell]&2`** (i.e. `0x00F37DA0` bit 1); the multiplier `k` comes from a
descending loop of `FUN_004BB160(x,y,k)` (radius tables `0x0081F156`, `0x0081F1AA`, `0x00A63704`), and
`img = k*9 + base + random%9`. Otherwise (`0x00E92DD0[cell]!=0xFF`) `img = T(0x627) + (random&7)`.

**4. `FUN_00540300 @0x00540300` → `FUN_004B6F90 @0x004B6F90` (`terrain&0x40`).** Wrapper calls
`FUN_004B8F70(0x8000040)`, `FUN_004B9B20()`, `FUN_004B6F90()`, returns 1. `FUN_004B6F90` (current cell
`0x0101C560`) returns early if `obj!=0`, or if `FUN_00471D20` false and `sub&0x80`. Cases:
`(terrain&0x10000)||FUN_00416BB0` → `i=img[cell]-T(0x61B)`, `j=FUN_005B22B0(cell,i)`; if `i==j` and
`(i<0x2A || i>0x2D)` then `img=T(0x61D)+R+C` (and `FUN_004BBDC0(&0x0084AEF0,0x11)`), else
`img=T(0x61B)+j`. `terrain&0x800` → `img=T(0x435)+FUN_00524F50(...)`. Otherwise `cVar2` →
`img=T(0x491)+FUN_0040E630`. `terrain&0x600` → `FUN_00474920`. Else `FUN_004BDEF0` selects
`0→T(0x621)+R+C`, `1→T(0x61E)+R+C` (also `&0x0084AEF0,0x11`), other→`T(0x61F)+R+C`, then
`FUN_004B6E20`. `0x0084AEF0` is descriptor data (0x11 entries) passed to `FUN_004BBDC0`.

**5. `FUN_00540330 @0x00540330` (`terrain&0x800`).** It does **not** write `0x00FE9880`. It calls
`FUN_004B9E90()` and `FUN_00524E90(0)` and returns 1.

**6. `FUN_005400F0 @0x005400F0` (`terrain&0x40000`).** No direct `img` write: `sub&=0xF0`, `draw|=0x40`,
`obj=0`, then `FUN_004BC800(cell)`. `FUN_004BC800` itself writes
`img = T(0x620) + random%9` **iff** `(terrain&0xC064)==0 && (terrain&0x40000)!=0 && (terrain&0x80000)==0`
(earlier chain branches normally consume `0xc064`), otherwise it leaves `img` untouched.

**7. `FUN_0053F840 @0x0053F840` (`terrain&0x20000`).** `FUN_004B8F70(0x20000)`, `FUN_004BC100()`,
default code `0x628`. If `C==0`: `DAT_0088EC38==1` → `0x626` (then, if `C==0`, `0x622`); else if
`0x00E92DD0[cell]!=0xFF` → `0x626` (then, if value `<100` and `C==0`, `0x622`). Else if
`5 < C+R < 0x22` → `0x603` with `C=random&7`, `R=0`. Final `img = T(code) + R + C`.

**8. `FUN_0053FA20 @0x0053FA20` (`terrain&0x2000000`, `img==0`).** Reads `feat[cell]`: `feat&8`→index 0;
else `feat&0x10`→1; else `feat&0x20`→2; else `feat&0x40`→3; else 0, selecting
`{0x60D,0x60A,0x609,0x619}`. It draws as a 5×5 block at `FUN_004B7BD0(cell)` with that base (via
`FUN_004B72B0`), so `img` is written through the block pass, not a direct per-cell store. **`0x00F37DA0` is not derived from
MAP bytes.** It is zero at startup, restored from the template `0x00DE12F0` by
`FUN_004048E0 @0x004048E0`; that template is only ever written by `FUN_005EF270 @0x005EF270` (snapshot of
the live grid) or read in from a SAV by `FUN_00780642` (size `0x32C40`). A whole-`.text` scan for the
immediate `0x00F37DA0` finds exactly six store sites (`0x004156C7`, `0x004B48A9`, `0x004B48BD`,
`0x004B74F6`, `0x004B7AB4`, `0x004BEBC0`) — all runtime bit set/clear operations — plus the template/SAV
copies; no MAP stride/offset feeds it.

**9. `FUN_004EDC30 @0x004EDC30`.** `this` is the singleton returned by `FUN_00425260`
(`&0x010AF4E0`), so `*(this+0x14)` is global `[0x010AF4F4]` — a 3-state mode `0/1/2` (zeroed by the
constructor `FUN_004ED290 @0x004ED290` inside `FUN_004ED240`, vtable `0x007B2BB8`; advanced by
`FUN_004ED3B0 @0x004ED3B0`). Logic: state 2 and `terrain&0x100==0` → return 0; state 0 and
`FUN_004EDB00` true (neighbour count `!=8`) → return 0. `FUN_004EDA50 @0x004EDA50` (the accept test)
requires: state 2 → `FUN_004EDB30` true (any neighbour has `0x100`); else `terrain&0x100`; then
`FUN_00408CF0<=0`; `terrain&0x80000==0`; and for `obj!=0` both `FUN_00562F70` false and `FUN_004EDCD0`
true. On accept: `img = T(0x61C) + 0x30 + random%0x18`, `draw|=0x40`, `sub&=0xF0`, return 1. The sibling
`FUN_004EDBA0 @0x004EDBA0` returns code `0x62B` (state 2 with a `0x100` neighbour, or `terrain&0x100`)
or `0x62A` (any of the 8 shifted neighbours via `FUN_004EDB30`/`FUN_004EDB60`).

**10. `FUN_0053F090 @0x0053F090` tail (`random&0xF8==0`).** After the guard `(terrain&0xAEFEDE6F)==0`:
`random&0xF8!=0` → 1×1 `img=T(0x603)+(random&7)`. Else `b=random&6`:
`b==6` and `FUN_004B7CA0(...,4,4)` → `T(0x603)+0x2A`; else `b∈{4,5}` and `(...,3,3)` →
`T(0x603)+0x18+((random&1)*9)`; else `b∈{2,3}` and `(...,2,2)` → `T(0x603)+8+((random&3)*4)`; else
`T(0x603)+(random&7)`. All are drawn through `FUN_004B7520` block art.

**11. `FUN_00408170 @0x00408170` resolver + loader.** `T(c) = 0` when `c&0x1FF==0`, else
`entry[(c&0x1FF)-1].field2 + (c>>9)*0x4000`. `entry` base is `0x01C42130 + 4`, stride 8, count at
`+0xCC38`; `FUN_004081D0 @0x004081D0` bounds-checks and falls back to `entry[0]`. The archive loader
`FUN_005CD1F0 @0x005CD1F0` copies the `MAP`/SG3 name, reads the u16 header-index array at `0x01B50218`
(= mapped payload `0x01B501C8` + 0x50 = file offset 80, 300 slots) and scans slots `k=0..299` **ascending**,
registering a node only when `headerValue - local_a0 > 0` (`local_a0=0`, or `200` for the
`Zeus_system.bmp`/`param_2` path). `FUN_005CD7C0 @0x005CD7C0` stores `node[0] = headerValue - local_a0`;
`FUN_005CFF20 @0x005CFF20` links; the count is incremented. Each output entry (stride 8) gets
`entry[0]=param_2`, `entry[1]=node[0]-1`, i.e. **`field2` = registered node index = `headerValue-1`** on the
normal path. So `code 0x600|k` corresponds to header slot `k`; version byte `0x01B501CC==0xD5 (213)`
triggers an extra `0x3624A`-dword copy. This is corroborated by `China_Terrain.sg3` (version 213, 300-slot
header index at file offset 80) whose 55 populated records each carry ordinal == slot.

**12. `0x00F1E780` random byte.** Filled by `FUN_004B0AC0 @0x004B0AC0`: a `228×228` loop writing the low byte
of the RNG state `0x010C7138` produced by `FUN_004189B0 @0x004189B0` (an LFSR seeded
`0x010C714C=0x54657687` / `0x010C7148=0x72641663` in `FUN_0052B590 @0x0052B590`). It is zeroed by
`FUN_004B08A0 @0x004B08A0`. **The source is the engine RNG, not the MAP random grid.** A whole-`.text`
scan for the immediate `0x00F1E780` (213 references) finds exactly one genuine byte store
(`0x004B0AD9`, the RNG fill) plus the zero-fill; every other apparent write uses a non-zero base register
(e.g. `0x005AD679`/`0x005AD68F` actually address `0x00FDCD70`/`0x00F9D620` with base `0xBE5F0`/`0x7EEA0`).
The MAP `mapperRandom` grid (payload offset 729,311) is therefore **not** consumed as this grid.

## Unresolved

* `FUN_004EDC30` mode `[0x010AF4F4]`: the exact game event/timing that advances it via `FUN_004ED3B0`
  (state 0/1/2 transitions) is not pinned down.
* `FUN_0053F240`: internal row layout of the static descriptor table `0x00825C28` (and `0x007ACA48`
  neighbour offsets) is not enumerated; only that it is static and keyed on elevation differences.
* The 8 neighbour object-pointer arrays for tile `0x61B` (`0x0088FE80`…) are not identified to gameplay
  object types; `FUN_004BB160`'s radius tables (`0x0081F156`/`0x0081F1AA`/`0x00A63704`) are not dumped.
* `FUN_004B6F90` helpers `FUN_005B22B0`, `FUN_0040E630`, `FUN_00474920`, `FUN_004BDEF0` are not resolved to
  numeric codes.
* Whether the MAP `mapperRandom` (729,311) and `mapperFertile` (937,255) grids are consumed anywhere at
  all was not found; the engine's per-cell random is RNG-generated (item 12). Relation of `0x00E92DD0`
  (used by items 3/7) and `0x00E9F8E0` to any MAP grid is untraced.
* The `0x00FE9880[cell]==0` precondition that lets setters #4/#10 run is assumed from the chain gate; its
  interaction with the rest of `FUN_0053EC90` was not re-read here.
