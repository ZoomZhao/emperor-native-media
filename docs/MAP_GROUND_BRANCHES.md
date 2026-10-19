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
| 4 | `FUN_0053FC30 @0x0053FC30` | `terrain&0x4` | `0x61B`/`0x61A`/`0x605`/`0x61C` (item 2) | partial |
| 5 | `FUN_0053F520 @0x0053F520` | `terrain&0x1` → `FUN_004BC6A0` | `T(0x624|0x625)+9k+random%9` (k∈{0,1}) or `T(0x627)+random&7` (item 3) | yes |
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

**2. `FUN_0053FC30 @0x0053FC30` (`terrain&0x4`).** Requires `terrain&0x4` (0x0053FC38
`test byte ptr [4*ebp+0x00F6A9E0],4`). It stores `DAT_0101C560=cell`, reads the
singleton byte `[0x010AF4E8]` (`FUN_00425250`, call at 0x0053FC53) and
`FUN_004B9CC0(0x104)` (0x0053FC67), which sets `DAT_0101C594` to the number of the
8 neighbours sharing bit `0x104` (neighbour views `0x00F6A650/4`,
`0x00F6A9DC/E4`, `0x00F6A64C`, `0x00F6AD6C/70/74`). If `[0x010AF4E8]==0` or
`DAT_0101C594!=8` the "`0x104` water" mode is forced off (0x0053FC76 `cmp eax,8` /
0x0053FC7B `mov byte ptr [esp+0x24],bl`). The initial neighbour count is then taken
over mask `4` (or `0x104` in water mode; 0x0053FC83/0x0053FC91 and the `sete` at
0x0053FC9F).

*Water mode off* (0x0053FCC6 `cmp al,bl` / `jne 0x53FF8A`):
* `(terrain&0x10000)!=0` (0x0053FCCE `test ...,0x10000`): `0x0053FCDB cmp dword ptr
  [0x0101C540],ebx` / `0x0053FCE1 jl 0x53FF8A`, otherwise the `D_1` case at
  0x0053FF62 writes `T(0x61A)+R+C`. `C` is 0 or a `0x20xx` descriptor (item 1) and
  never negative, so this is always the `0x61A` arm.
* Otherwise `FUN_004B8F70(0x10000)` + `FUN_004BC140()` (0x0053FCEC..0x0053FCF9).
  `0` such neighbours → `LAB_0053FF8A` (0x0053FD00 `je`). Otherwise the 8
  neighbour object pointers (`0x0088FE80/84`, `0x00890214`, `0x008905A4/A0/9C`,
  `0x0089020C`, `0x0088FE7C`; test `obj==0 || obj+0x18==0`) feed a switch on `C`
  selecting orientation `iVar8∈{0,2,4,6}`:
  * `0x61B` value `= T(0x61B) + ((iVar8-4)&7 + 6)*3 + iVar7` (+1 if the two
    orientation flags `0x0101C5B8[...]` are set). Cases `C=0→4`, `4→6`, `8→0`,
    `0xC→2`, `0x10/0x18/0x1C` combine.
  * default/`D_1`: `0x61A` value `= T(0x61A) + R + C`.
* At `LAB_0053FF8A` with the water byte still 0 (0x0053FF99 `cmp byte ptr
  [esp+0x24],bl` / `je 0x53FFAB`) the three MAP-determined arms are:
  * `terrain&0x4000000 != 0` (0x0053FFB2) and `0x100` clear (0x0053FFB9; the
    disjunct is dead because setter 1 owns every `0x100` cell and returned above):
    recount the `0x4000000` neighbours (0x0053FFBE..0x0053FFD0). `==8` →
    `T(0x61C)+0x30+random%0x18` (0x0053FFDF, `0x0053FFEE mov esi,0x18`,
    0x0053FFF6 `lea ecx,[ecx+edx+0x30]`); `<8` → `T(0x61C)+random%0x30`
    (0x0053FFDD `jne 0x53FFFC`, `0x54000B mov esi,0x30`, 0x540011/0x540013).
  * `terrain&0x4000000 == 0` (0x0053FFB7 `je 0x540017`): the count from 0x0053FC97
    is over mask `4` (water byte 0). `==8` → `T(0x61C)+random%0x18+(random&1)`
    (0x540017 `cmp byte ptr [esp+0x13],bl` / 0x54001D..0x540051, the `and
    esi,0x80000001` being the compiler's sign-fix for `(int)(r&1)`); `<8` →
    `T(0x605)+R+C` (0x540055 `push 1541`).

*Water mode on* (0x0053FCC6 `jne 0x53FF8A`) skips the whole `(char)param_2==0`
block and always writes `T(0x61C)+0x30+random%0x18` (0x0053FF9F `push 1564` →
0x0053FFDF). A bare load has the byte 0: the singleton constructor
`FUN_004ED290 @0x004ED290` zeroes it (`mov byte ptr [edx+8],al`) and its only
writer `FUN_004ED2B0 @0x004ED2B0` is reached from gameplay code (single caller
0x0049CEE6), not the `.map` read path. So the three `0x61C` arms above are the
load-time dispatch.

Measured over all 167 `Cities/*.map`: water-ring 83,229, water-fringe 57,024 and
shore-ring 5,014 cells are MAP-determined, matching the stored grid on 39.73 %,
39.32 % and 40.33 % respectively. The remaining 94,897 `0x4-shore` cells take
`T(0x605)+R+C`; `R=0` (`0x0101C538` cleared at 0x00473326) and `C` is the runtime
elevation-pattern descriptor of item 1, so they stay unknown. Their stored ids
spread across the whole 48-piece slot-5 art range (0..47), which is
descriptor/authored art, not a MAP byte: the `d=0` mode is only 1.8 %.

**Those three percentages are not the id shift, and must not be explained by it.**
Splitting the corpus by whether the city's road band is shifted (the road band and
the `0x61C` band are both "upper", so this is the right cohort split), the arms are:

| Arm | Unshifted cities (93) | Shifted cities (74) |
|---|---|---|
| `0x4-shore-water-ring` | 47,649 emitted, 33,070 matched (69.40 %) | 35,580 / **0** (0.00 %) |
| `0x4-shore-water-fringe` | 33,264 / 22,292 (67.02 %) | 23,760 / 128 (0.54 %) |
| `0x4-shore-shore-ring` | 3,041 / 1,999 (65.73 %) | 1,973 / 23 (1.17 %) |

So the shifted cohort is fully explained by the shift, but **the unshifted cohort
still only reaches 66-69 %**, i.e. ~11,000 fringe cells are wrong with no shift to
blame. Their `stored - predicted` deltas are *scattered* (`-8..+8` all populated
plus 9-16/17-32/33-64) rather than constant - e.g. MP61 road 99.87 % matched but
fringe 0.00 % with deltas spread over `-8..+6` and 95/166/46 in the three large
buckets - while STGW-Anyang and Badaling are 100 %. Only 20 of the 84 unshifted
cities with >=50 fringe cells reach 100 %. A scattered residual in a city whose
other bands are exact is a **condition** error, not a variant or a shift error: the
candidate is the neighbour count, because `0x540001 FUN_004B8F70(0x4000000)` runs
immediately before `0x540006 FUN_004BC140` and may write descriptors the count then
reads, which the per-cell MAP model does not reproduce. The three formulas
themselves are confirmed against the decompiled `FUN_0053FC30 @0x0053FC30`:
`0x54000B`/`0x540011` give `base(0x61C) + r%0x30`,
`0x54001F`/`0x54003B`/`0x540040`/`0x540051` give `base(0x61C) + r%0x18 + (r&1)`, and
`0x53FFEE`/`0x53FFF4`/`0x53FFF6` give `base(0x61C) + 0x30 + r%0x18`.

**3. `FUN_0053F520 @0x0053F520` → `FUN_004BC6A0 @0x004BC6A0` (`terrain&0x1`).** Wrapper does
`sub&=0xF0`, `draw|=0x40`, then `FUN_004BC6A0`. Gate `(terrain&0xC064)==0`. If `terrain&0x100` and
the 8-neighbour count `DAT_0101C594==8` (`FUN_004B9CC0(0x100)`) it returns with **no** `img` write. Else, when `obj==0 && terrain&1 && !(terrain&0x80000)`:
call `FUN_004BD570` (value = `min(100, 0x00E9F8E0[cell])`, discarded, so inert here). If `0x00E92DD0[cell]==0xFF` the base is
`T(0x624)`, or `T(0x625)` when **`feat[cell]&2`** (i.e. `0x00F37DA0` bit 1); the multiplier `k` is the
return of `FUN_004BB160(x,y,1)` and `img = k*9 + base + random%9`. Otherwise
(`0x00E92DD0[cell]!=0xFF`) `img = T(0x627) + (random&7)`.

`k` is exactly 0 or 1. Disassembly `0x004BC77C..0x004BC7A4`: the first call is
`FUN_004BB160(x,y,1)`; if it returns non-zero the loop counter stays 1, else a second call with
`param_3=0` is made and the counter drops to 0. `FUN_004BB160 @0x004BB160` returns 0 as soon as an
**in-map** ring cell has `(terrain&3)==0`, else 1. Its radius tables are dumped: tier-1 count at
`0x0081F158` is **8** and the ring offsets start at `0x00A63704` via `0x0081F1AC` (the tier-0 count
`0x0081F156` is 0), so the ring is the eight neighbours and `FUN_004BB390 @0x004BB390` supplies the
in-map test. Therefore **`k==1` iff every in-map 8-neighbour carries `(terrain&3)!=0`**; an out-of-map
neighbour is skipped, not read as a zero flag.

All three inputs are `.map` bytes: `0x00E92DD0` is payload offset **885,271**, `0x00F37DA0` offset
**469,391**, `0x00F1E780` offset **729,311** (`original_terrain.hpp`, read order `FUN_0052E7C0`). The
only non-MAP premise is `obj[cell]==0` (`0x00FC3750`); a bare load satisfies it because reset
`FUN_004B08A0` copies the zero-filled template `0x00E46B70` (item 8 / load path). Measured on the
corpus: all 246,802 road cells have `0x00E92DD0==0xFF`, so the `0x627` arm never fires.

Measured over all 167 `Cities/*.map`: 246,802 road cells, 245,520 resolve to tileset 3 (the other
1,282 store a zero/non-tileset-3 word). The road codes are all upper-band art (`0x624`/`0x625`), and
of the resolved cells the difference `storedIndex - predictedIndex` takes four modal values: **0** for
150,824 cells (61.4 %), **-30** for 48,619 (19.8 %), **-32** for 45,065 (18.4 %), **-20** for 985
(0.4 %); 27 cells sit in an isolated -600..-740 tail (other-setter/block art). Within a city the delta
is constant for that band - e.g. Anyang has 1,734 of its 1,736 road cells at exactly -30 - so the
formula and `k` test are exact and the road residual is the per-(city, band) id-namespace shift
described below, not a formula error.

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
  object types. (`FUN_004BB160`'s radius tables are now dumped: tier-1 count 8 at `0x0081F158`, offsets
  from `0x00A63704` via `0x0081F1AC`; see item 3.)
* `FUN_004B6F90` helpers `FUN_005B22B0`, `FUN_0040E630`, `FUN_00474920`, `FUN_004BDEF0` are not resolved to
  numeric codes.
* The MAP `mapperRandom` (729,311) is not consumed as the engine's per-cell random grid (item 12), but
  `0x00E92DD0` **is** MAP-carried at offset 885,271 and `0x00E9F8E0` **is** MAP-carried at offset 937,255
  (`original_terrain.hpp`). Both are restored by `FUN_0052E7C0 @0x0052E7C0`, which is why setters 5 and 17
  are modelled from the payload.
* The `0x00FE9880[cell]==0` precondition that lets setters #4/#10 run is assumed from the chain gate; its
  interaction with the rest of `FUN_0053EC90` was not re-read here.

## Load path: the per-cell arrays are not terrain-derived

The remaining unmodelled branches of `FUN_0053EC90` read per-cell arrays that a
bare city `.map` cannot supply. Established on the owned exe (SHA-256
`0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`):

- The entire per-cell working set is **zeroed** by the map reset
  `FUN_004B08A0 @0x004B08A0`. The loops are dword loops, so the counts read as
  `i = n; *p++ = 0` give: `0x00FE9880` 0xcb10 = **4 B/cell**, `0x00F6A9E0` 0xcb10
  = 4 B/cell, `0x00FC3750` 0x6588 = **2 B/cell**, and `0x00F9D620`, `0x00FDCD70`,
  `0x00F1E780`, `0x00F2B290`, `0x00F11C70`, `0x00F05160` 0x32c4 = 1 B/cell, plus
  `0x00EAC3F0`, `0x00FAA130`, `0x00EEBB40`, `0x00EDF030`, `0x00ED2520`,
  `0x00EC5A10` and more.
- A second baseline path copies `.data` templates into the working arrays:
  `FUN_004191f0 @0x004191f0` is a memcpy whose **first argument is the source**,
  e.g. `FUN_004191f0(&DAT_00e46b70, &DAT_00fc3750, 0x32c40)`,
  `FUN_004191f0(&DAT_00dbb1c0, &DAT_00f9d620, 0xcb10)`,
  `FUN_004191f0(&DAT_00dae6b0, &DAT_00fdcd70, 0xcb10)`,
  `FUN_004191f0(&DAT_00de12f0, &DAT_00f37da0, 0x32c40)`. Direction is proved by the
  reversed call at file line 326260, which saves working state back into the
  template. All four templates lie in `.data` beyond `SizeOfRawData`, i.e. they
  are zero-filled at load.
- The state is then restored from a serialised blob by
  `FUN_0052FDA0 @0x0052FDA0` (e.g. `0xcb10` bytes into `0x00F1E780`). The byte grid
  at that address is the same field the save files carry, and the repo reproduces
  it byte-exactly for 6/6 supplied saves (`make original-random-test`).
- `0x00FC3750[cell]` is a **loaded-blob per-cell object/descriptor id** resolved
  through `FUN_0047F1B0 @0x0047f1b0`, not a function of terrain.

Consequence: branches keyed on the runtime caches or the object id are not
MAP-determinable. Two per-cell inputs are *not* in this group because they are
payload fields restored by the same read pass: `0x00E92DD0` (offset 885,271) and
`0x00E9F8E0` (offset 937,255), which is why setters 5 and 17 are modelled.
In particular setter 3 (`terrain & 0x800` -> `FUN_00540330` -> `FUN_00524E90(0)`)
**does** write `tileImage(0x435)` when `0x00FC3750[cell] == 0`, so it is not a
chain no-write case either; it stays unmodelled and measured 0 cells across all 167
maps, so no metric moves.

### The `0x8C008` gate is a determinate chain no-write

`FUN_0053EC90` disassembles as `0x0053ECBA test edi,0x8c008` / `0x0053ECC0 jne
0x53EDF5`, i.e. a gate hit skips setters 2..18. That line is only reached when
`0x100` is clear (the `0x100-special` path returns earlier), and for a `0x100`-clear
cell `FUN_004EDC30 @0x004EDC30` returns 0 with no store in every mode: mode 2 exits
at `0x004EDC68`, and modes 0/1 store only under `0x004EDC79`, whose accept test
`FUN_004EDA50 @0x004EDA50` refuses when `0x100` is clear. So setter 1 writes
nothing, setters 2..18 are skipped, and the cell keeps its prior `0x00FE9880`
value. That prior value is **loaded from the `.map` payload** (logical offset 1,535,
`original_terrain.hpp`), not zero: the cell is not overwritten, it is not "not
drawn". Measured over all 167 maps, every one of the 114,937 chain-no-write cells
stores nonzero art and `0` store zero. See
[`MAP_SG3_PIPELINE.md`](MAP_SG3_PIPELINE.md#the-stored-surface-grid-is-an-oracle-not-a-derivation).
This was previously misclassified as "determinate no-ground" — and, before that, as
an unknown.

### MAP-only ceiling, measured over all 167 `Cities/*.map`

`active 2,107,836`; `known 1,823,379` (86.50 %), every emitted code resolving in
`China_Terrain.sg3` with 0 unmatched; `unknown 169,520` (8.04 %, across the
runtime-array branches below); determinate `chain-declined` 114,937; `decode_failures`
0 for every city. The unknown residue is `0x4-shore` 94,897, `0x80-farm-block`
36,950, `0x40-road` 23,826, `0x10000-monument` 6,216, `0x20000-forest` 4,347 and
`0x80-farm-river` 3,284. Completing those requires the runtime descriptors
(`0x0101C538/540`, the road/edge caches, the object id) or block-art expansion, not
more terrain analysis. (The earlier ceiling `known 1,514,539` / `unknown 478,360`
predates modelling setters 5 and 4 and is superseded: the pre-road `known` was
1,514,539, `1,514,539 + 246,802 + 62,038 = 1,823,379`, i.e. the road cells and the
two `LAB_0053FF8A` shore arms are the entire change.
The `0x4-shore` residue fell from 156,935 to 94,897 when those arms were ported:
`156,935 - 57,024 - 5,014 = 94,897`.)

## Mismatch classes: every residual cell now has a named cause

`matched` is the count of emitted codes that equal the stored grid (`1,474,883` of
`1,823,379`, 80.887 %). The remaining `348,496` cells were a bare count. They are
now classified per branch, and the classes partition `mismatched` exactly - the
sweep treats a city whose classes do not sum to its `mismatched` as a **failure**,
so a silent drop cannot pass.

Reproduce with the sanctioned harness (`--image-grid`), which carries
`chain_mismatch_classes` and `chain_branches` in its aggregate JSON:

```sh
EMPEROR_GAMEDATA_ROOT=<data root> make map-ground-sweep
# or: scripts/map_ground_sweep.py sweep --data-root <root> --image-grid \
#       --sg3 DATA/China_Terrain.sg3 --sg3-elevation DATA/China_Elevation.sg3
```

Measured over all 167 `Cities/*.map` (167/167 processed, 0 failures):

| Class | Cells |
|---|---:|
| stored word is 0 while the chain emitted | 15 |
| stored word names a tileset other than 3 | 9,828 |
| same tileset 3, stored index within +-8 of the prediction | 27,100 |
| same tileset 3, stored index further off | 311,553 |
| **total** | **348,496** |

The 9,828 non-tileset-3 cells all name **tileset 16 (`China_Elevation.sg3`)**; no
other tileset appears (direct corpus probe of `stored>>14`). So the chain does not
merely pick the wrong record on those cells, it claims a cell whose art is not
ground art at all: the dispatch chain models setters that write into the ground
namespace, and the engine's real choice for those cells produced elevation art.
That is a missing branch, not a variant error, and it is the single largest
*non-index* class.

The 311,553 "further off" cells are bucketed by magnitude of
`storedIndex - predictedIndex`: 9-16, 19,323; 17-32, 203,004; 33-64, 22,267;
65-128, 51,246; 129-256, 14,669; 257-512, 1,014; 513+, 30. The 17-32 bucket
grew by 23,864 because the two new shore arms emit cells whose per-city namespace
sits 20/30/32 below the prediction, and because setter 5 already emits the road
cells with the same shift (next section). Note that the *shift* only accounts for
the shifted cohort: in the 93 cities whose upper bands are exact the shore arms
still reach only 66-69 % (item 2), so part of the shore residual is a condition
error with scattered deltas. A wrong *variant* alone can
only produce a small delta (the variant space is 0..32 for road), so the clustered
large deltas are wrong *base/band* selections. Per branch, the residual is
dominated by `0x1-road` 95,978 (61.11 %), `0x80-farm` 65,742 (94.01 %),
`0x4-shore-water-ring` 50,159 (39.73 %), `0x2-land` 49,808 (33.14 %),
`0x4-shore-water-fringe` 34,604 (39.32 %), `plain-land` 26,936 (88.16 %) and
`0x4-shore-shore-ring` 2,992 (40.33 %).

### The per-(city, record-id band) id shift is the largest single cause

The 17-32 magnitude bucket is not one branch's problem. Cells whose stored index
sits 17-32 records from the prediction, by branch: `0x1-road` 94,669,
`0x4-shore-water-ring` 31,876, `0x2-land` 26,331, `0x4-shore-water-fringe` 21,923,
`0x80000000-flood` 9,949, `plain-land` 9,833, `0x40000-canal` 4,776,
`0x4-shore-shore-ring` 1,941 and `0x101000-twin` 1,700 - **203,004
cells, 65 % of the 311,553 large-delta residual**, from one cause.

Inside a city the offset is constant *for a given band*: Anyang has 1,734 of its
1,736 road cells at exactly -30, its `0x2-land` mode is -30, and all 384 of its
`0x4-shore-water-ring` cells are -30, while Bo, Badaling and Chizhou are uniformly
0. Over the corpus the road offset splits the cities cleanly - 93 cities at 0 with
150,824 matched road cells, and 74 cities where road, land and shore all match
**exactly 0.000** of their cells, while `0x80-farm` (94.16 % vs 93.81 %) and
`plain-land` (88.70 % vs 87.79 %) are unaffected in both groups. An all-or-nothing
0.000 across thousands of cells cannot be a formula error, and no `.map` header
field, active side or file size separates the two groups.

The shift is not one constant per city either. Grouping the modelled emitted codes
by the `imageIndex` region their art sits in - farm `202`, plain land `247`, land
`458/472/486`, feature `1440..1443`, upper ground `664..1138` (canal, road, shore,
twin, flood) - the corpus splits into four vintage classes, and **30 cities take
-30 on the land band while the road band takes -32** (34 when canal, flood and shore
are aggregated into the upper set), which proves the shift is per-(city, band):

| Band | Emitted codes (`imageIndex`) | Modal residual over 167 cities |
|---|---|---|
| farm | `0x602` (202) | 0 in all 167 |
| plain land | `0x603` (247) | 0 in all 89 cities with >= 16 cells |
| land | `0x606/607/608` (458/472/486) | 0 (93), -30 (73), -20 (1) |
| upper ground | `0x60E/60F/61C/620/624/625` (664..1138) | 0 (93), -30 (39), -32 (34), -20 (1) |
| feature block | `0x609/60A/60D/619` (1440..1443) | 0 (13), -318 (8), -308 (1) |

No band's modal residual is ever positive. Farm and plain land sit below every
inserted record run, so the installed archive reproduces them exactly; the feature
residual is the upper residual minus exactly the 288-record run inserted below
`1440`. This is the signature of an archive whose records were **inserted below a
band after the `.map` was authored**: the stored word is the authoring-time archive
id, and today's `China_Terrain.sg3` has extra records between the low bands and the
shifted ones.

The shift is not corrected inside `selectGroundTileCode`: that would tune to the
oracle. The selector predicts the executable's own code; the score compares archive
indices. The farm residual is a separate, non-band cause - see "Every farm residual
is a wrong branch, not a wrong variant" below.

### The shift is a stale archive vintage, and it is not derivable

The residual shape is **measured**: a constant per (city, band), never positive, with
four (land, upper) vintage classes. The explanation that fits every measurement is
that the stored word is an archive id captured when each `.map` was authored, and the
installed `China_Terrain.sg3` has since had records inserted before some art bands.
That explanation is an **inference**, not a measurement: the authoring-time table is
**not shipped**, so the constant cannot be recovered from the payload, the archive or
the executable, and fitting it would be tuning to the oracle. Tested and refuted (all
on the owned data / exe SHA-256
`0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`, 4,820,992 B):

* **No shifted land table exists in the install.** The data root holds 58 `.sg3`
  files and exactly one `China_Terrain.sg3`; no archive carries the
  `[201,202,247,336,386,458,472,486]` land/canal header pattern, shifted or not.
* **The exe contains no such table.** A byte search of the whole executable finds
  neither the current `imageIndex[1..8]` pattern nor its -20/-30/-32 shifts nor the
  full 600-byte header table.
* **No `.pak` / alternate archive carries a second copy** of the table.
* **No `.map` header field separates the classes.** A 1,535-byte header scan at
  widths 1/2/4 finds 0 fields constant within a vintage class and distinct across
  classes.
* **No payload scalar separates the classes.** `side`, `g101D0AC`, `g101D0B0`,
  `g10C6F18` and the `0x168`-byte blob each fail the same separation test; only the
  file size "separates", and that is vacuous.
* **The loader does not pack per city.** `FUN_005CD1F0 @0x005CD1F0` builds each
  slot -> record entry verbatim from the archive header (`entry[1] =
  imageIndex[slot] - 201`), with no per-city adjustment.

Two consequences follow, and they must not be confused. For the **score**, the stored
grid is a stale oracle above the farm and plain-land bands, so `80.887 %` is a floor
on the reconstruction's agreement with it, not the reconstruction's accuracy. For the
**render**, `recordId = low14 + 201` reads those stored ids against the installed
archive, so in a shifted city it decodes a record 20/30/32 entries from the one the
authoring table named. Whether that record is the intended art is **open**: "exact
render" only proves that every cell decodes, not that it is the right image, and the
original-screen comparison that would settle it is still 0/167.

The residual is also not a namespace fixed once at load, because 34 of the 167 cities
shift the land band and the upper bands by different constants, so even a single
per-city table would be wrong. The corpus assertion in
`game_lab/map_reference_tests.cpp` (`storedGridVintageTests`) locks in the structure -
farm exact everywhere, land/upper modal residual in the measured negative set and
never positive, feature `<= -288`, and at least one city shifting the land band
differently from the upper bands - without encoding any per-city value.


## Composite blocks store one image word, not a per-cell variant

Measured over the same corpus with the block decomposition already used by the
stored-grid renderer (anchor = `cell - (draw & 7) - ((draw >> 3) & 7) * 228`):

- 6,207 composite blocks (4,557 of 4 cells, 549 of 9, 1,020 of 16, 58 of 25, 23 of 1).
- The stored image word is **identical in every cell of a block: 6,207/6,207
  (100.00 %)**, 0 exceptions.
- The per-cell random byte (`0x00F1E780`) **varies inside 6,184/6,207 blocks
  (99.63 %)** and is constant in only 23.

So block art is written once and stamped across the footprint. Any rule of the form
`code = base + (random & 7)` evaluated *per cell* can match at most one cell per
block, which is exactly the low-accuracy signature of the block branches.

This predicts a specific fix - read the variant from the block anchor cell - and
that prediction was **tested and refuted**. Applying it corpus-wide
(`groundCodeRequest` taking `randomByte[anchor]`) moved `matched` from 1,299,617 to
1,296,643 (measured before setter 5 was modelled; **worse by 2,974**; `0x80000000-flood` 14.57 % -> 11.88 %,
`0x2-land` 33.14 % -> 30.05 %) and the change was reverted. The MAP's draw grid
therefore does not identify the cell whose random byte produced the block's variant,
and the variant is not derivable from the MAP's per-cell payload alone. That is a
measured boundary, not a guess: for those branches the stored grid stays the
faithful source for rendering, and the reimplementation's residual is bounded by
state the MAP does not carry.

### Every farm residual is a wrong branch, not a wrong variant

Setter 17's farm formula is `T(0x602) + 9*rank + random%9` with `rank` in 0..4
(`FUN_004BD8C0 @0x004BD8C0` calls `FUN_004BD940 @0x004BD940`, cited above), so the
reachable archive indexes are `imageIndex[2] - 1 + 0..44`. For `China_Terrain.sg3`,
`imageIndex[2] = 202`, so the whole farm band is 45 records wide (201..245). **No
variant error can move the stored index by more than 44.** That bounds the residual
without any new measurement: of `0x80-farm`'s 65,742 mismatches, 65,525 sit in the
65-128 (50,857) and 129-256 (14,668) magnitude buckets alone, which the formula
cannot reach.

An independent per-cell probe confirms the remaining 33-64 group is out of band too.
The probe replicates the setter-17 gates from the payload alone
(`(terrain & 0x10001) == 0 && (terrain & 0x80)`, no 8-neighbour carrying `0x10000`,
`random & 0xF8 != 0`) and computes the prediction directly, over all 167 maps:

- it reproduces the branch's **matched** set exactly - 1,031,695 cells, identical to
  the score's `1,097,437 - 65,742` - so the formula is confirmed;
- it attributes 79,768 extra cells to the branch (all of them mismatches, since the
  matched sets are identical), so its mismatch set is a strict superset of the
  chain's: 145,510;
- of those 145,510, **0 have a stored index anywhere in the band (offset 0..44)**.
  The stored art is either another tileset - 8 (China_Mon_Earthen_Greatwall_1) 6,018,
  6 (China_Mon_Grand_Canal) 4,061, 2 (China_General) 1,943, 5 (China_Mon_Tumulus)
  115, 16 (China_Elevation) 6 - or a tileset-3 record far outside the band.

So the farm residual is not "the wrong band was selected from the farm table"; it is
that the chain **claims cells the engine gave other art**. Those art sources are the
setters this reimplementation does not model (setters 2, 3, 4, 9, 13, 14, 15).
The 169,520 `chain_unknown` cells and the 311,553 large-delta mismatches are the same
blocker seen from two sides: **porting those setters is the lever**, and until it is
done the stored grid stays the faithful render source.
