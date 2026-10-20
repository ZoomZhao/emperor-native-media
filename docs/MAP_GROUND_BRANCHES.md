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
| `R` | `0x0101C538` u32 | shared scratch sub-index; shore setter overwrites it from its own table |
| `C` | `0x0101C540` u32 | shared scratch code; elevation and shore have different writers/tables |
| `D` | `0x0101D0D0` u32 | view rotation 0/2/4/6; bare-map reconstruction uses 0 |
| `B`,`S` | `0x00BEBF34`, `0x0101C53C` | edge block index / flag |
| `T(c)` | `FUN_00408170 @0x00408170` | tile-image base for code `c` (item 11) |

`cell = y*228 + x`; grid origin `0x0101D0C8`, stride `0xE4`, size 51,984. `T(c)=0` when `c&0x1FF==0`.
The chain `FUN_0053EC90 @0x0053EC90` calls `FUN_00425260`+`FUN_004EDC30` first; only if `FUN_004EDC30`
returns low-byte 0 **and** `(terrain & 0x8C008)==0` does it run the list below, stopping at the first
non-zero setter. Single-cell ground writers generally do `draw|=0x40` and `sub&=0xF0`;
elevation assigns draw `0x40`/`0x41`, and constant-image blocks assign draw offsets
and group size instead (see the elevation and SAV19 sections).

## Setter chain (one row per setter)

| # | Setter `@0x` | Exact condition | Exact value written | Modelled |
|---:|---|---|---|---|
| 1 | `FUN_004EDC30 @0x004EDC30` | `FUN_004EDA50` true and mode `[0x010AF4F4]` allows (item 9) | `img=T(0x61C)+0x30+random%0x18` | no |
| 2 | `FUN_0053F240 @0x0053F240` | `B>=0 && C!=0` from height-pattern cache arrays (item 1) | 2×2 elevation block | yes: snapshot image reconstruction |
| 3 | `FUN_00540330 @0x00540330` | `terrain&0x800` | **no `img` write** (item 5) | no |
| 4 | `FUN_0053FC30 @0x0053FC30` | `terrain&0x4` | `0x61A`/`0x605`/`0x61C`; `0x61B` requires an active river object (item 2) | yes: bare-load snapshot |
| 5 | `FUN_0053F520 @0x0053F520` | `terrain&0x1` → `FUN_004BC6A0` | `T(0x624|0x625)+9k+random%9` (k∈{0,1}) or `T(0x627)+random&7` (item 3) | yes |
| 6 | `FUN_0053F660 @0x0053F660` | `terrain&0x2` | `T(0x607/0x608/0x606)` by `terrain&0x300002`; 3×3 `+12+(random&1)`, 2×2 `+8+(random&3)`, else `+(random&7)` | yes: whole-map composite stamps |
| 7 | `FUN_0053F790 @0x0053F790` | `(terrain&0x101000)==0x101000` | `T(0x60F)`; 2×2 `+8+(random&3)`, else `+(random&7)` | yes: whole-map composite stamps |
| 8 | `FUN_0053F560 @0x0053F560` | `terrain&0x80000000` | `T(0x60E)`; 3×3 `+12+(random&1)`, 2×2 `+8+(random&3)`, else `+(random&7)` | yes: whole-map composite stamps |
| 9 | `FUN_0053F840 @0x0053F840` | `terrain&0x20000` | `T(0x628/0x626/0x622/0x603)+R+C` (item 7) | yes: bare-load snapshot |
| 10 | `FUN_0053FA20 @0x0053FA20` | `terrain&0x2000000` and `img==0` | `T(0x60D/0x60A/0x609/0x619)` by `feat` bits (item 8) | no |
| 11 | `FUN_00413A00 @0x00413A00` | none — 3-byte body | returns 0 (never handles a cell) | yes |
| 12 | `FUN_00540360 @0x00540360` | `terrain&0x1000` | `T(0x612)+random&7` | yes |
| 13 | `FUN_00540300 @0x00540300` | `terrain&0x40` → `FUN_004B6F90` | `0x61B`/`0x61D`/`0x435`/`0x491`/`0x621`/`0x61E`/`0x61F` (item 4) | yes: bare-load `0x61E/61D/623`, elevation default no-write |
| 14 | `FUN_0053FB30 @0x0053FB30` | `terrain&0x10000` and `(terrain&0x48)==0` | `T(0x61B)+R` after C is cleared (+`FUN_004BCE20` with an object) | yes: per-cell null-object phase; whole-map post-object `T(0x61B)+random%6` |
| 15 | `FUN_005400F0 @0x005400F0` | `terrain&0x40000` | **no direct `img` write**; delegates (item 6) | no |
| 16 | `FUN_00540140 @0x00540140` | `terrain&0x20` | `T(0x401)+2..7` (tileset 2, block art) | partial |
| 17 | `FUN_0053EE00 @0x0053EE00` | `(terrain&0x10001)==0` and `terrain&0x80` | `T(0x602)`+fertility variant, consecutive `T(0x604)` block cells, or `T(0x61B)+C+6` | yes: bare-load snapshot |
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
(view column chosen by `DAT_0101D0D0/2`). The caches have no payload offsets, but their
height input **is MAP-derived**: `0x00BEBF3A`, payload 989,275, already parsed as
`gBEBF3A` (the elevation section below corrects the earlier missing-input claim). `FUN_0053F240` reads
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
  0x0053FF62 writes `T(0x61A)+R+C`. The shore-specific matcher described below
  writes a nonnegative byte descriptor, so a matched row always takes this arm.
* Otherwise `FUN_004B8F70(0x10000)` + `FUN_004BC140()` (0x0053FCEC..0x0053FCF9).
  `0` such neighbours → `LAB_0053FF8A` (0x0053FD00 `je`). Otherwise the 8
  neighbour object pointers (`0x0088FE80/84`, `0x00890214`, `0x008905A4/A0/9C`,
  `0x0089020C`, `0x0088FE7C`; local is true iff `obj!=0 && obj->byte18==0`) feed a switch on `C`
  selecting orientation `iVar8∈{0,2,4,6}`:
  * `0x61B` value `= T(0x61B) + (((iVar8-4)&7) + 6)*3 + iVar7` (+1 if either of the two
    orientation flags `0x0101C5B8[...]` are set). Cases `C=0→4`, `4→6`, `8→0`,
    `0xC→2`, `0x10/0x18/0x1C` combine.
  * default/`D_1`: `0x61A` value `= T(0x61A) + R + C`. All eight pointers are
    null in the bare-load phase, so all switch cases fall back here.
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
0x0049CEE6), not the `.map` read path. The bare-load dispatch includes the three
`0x61C` arms and the descriptor-driven `0x605`/`0x61A` arms.

**View-rotation premise.** `DAT_0101D0D0` is copied from `DAT_0115F720`: the
bare-load path reads the source at `0x534477` and stores the view at `0x534495`,
after clamping to 0..6 by resetting out-of-range values to 0
(`0x53447D..0x534489`). `DAT_0115F720` is UI camera-rotation state, not a MAP
field: rotate handlers `FUN_0046A010` / `FUN_0046A0D0` step it by +2 / -2,
wrapping at 0/6 (writers `0x46A055`, `0x46A05E`, `0x46A112`, `0x46A119`).
The reconstruction **assumes view 0**. The unshifted corpus supports that
premise: elevation-direct matches 60,679/61,651 (98.42 %) and shore-edge
51,199/52,597 (97.342 %); it does not establish a MAP-carried view value.

**The incoming elevation descriptor is overwritten inside setter 4.** After
`FUN_004B8F70(4)` and the count, `0x53FCA4` calls `FUN_004BC100`, inverting the
eight 0/1 flags. `0x53FCAC..0x53FCBA` passes the random byte, forward mode 0,
count **46** and table **`0x00848D00`** (file `0x448D00`) to `FUN_004BBDC0`.
Each ordered 20-byte row contains eight byte predicates (0=shore, 1=non-shore,
2=wildcard), four view offsets at +8..11, S at +12, variant count at +13,
mutable counter at +14, and compiled predicate bits at +16. `FUN_004BC090`
builds those bits (call `0x4B0527`); the port matches the equivalent predicates.
The forward matcher scans at `0x4BBE91..0x4BBEA1`, writes `C=row[8+D/2]`
at `0x4BBEBF`, S at `0x4BBECC`, and **`R=random%row[13]`** at
`0x4BBEDB/0x4BBEE3`. Counts are 1, 2 or 4. The negative-random counter arm
`0x4BBEEB..0x4BBF06` is unreachable from this setter's unsigned random byte.
Thus the existing elevation table/matcher remains the prepass source; no
elevation matcher or table was duplicated for shore.

The shore table matches **255/256** non-shore masks. The unmatched all-shore
mask normally takes the `0x61C` ring formula, which never reads C/R. If a
`0x10000` river gate accompanies that mask, the executable retains incoming
scratch C/R: this unported cross-call state stays unknown (0 such cells in the
current active corpus). It is not evidence that those values are intrinsically
non-derivable.

The pointer views are one grid at **`0x00890210`**, shifted by
`-228,-227,+1,+229,+228,+227,-1,-229`. Reset `0x4B0999` calls
`FUN_0041F3B0`, whose `0x41F3B5..0x41F3BE` clears all **51,984/51,984** pointers.
The MAP read `0x52EAAD..0x52EAB7` restores only 36 bytes at grid+`0x32C40`
through `FUN_00408C70`; it does not restore pointers. The null tests at
`0x53FD0C..0x53FDD2` make every local false before the C switch, excluding
`0x61B` at `0x53FF15` and selecting `0x61A` at `0x53FF62`. Later river-object
construction is separate: `FUN_0041F400` clears/rebuilds the grid using terrain,
`FUN_0041F820` writes its per-cell pointer (`0x41F855`), and
`FUN_0041F230` sets the object's +0x18 byte (`0x41F25F`). The bare city path
`FUN_00534410` calls `FUN_0053D630` (ground reconstruction) at `0x53449F`
**before** river-object construction at `0x5344BD`. Construction itself calls a
second ground reconstruction at **`0x41F63A`**. The per-cell API retains the first
phase; the whole-map API now includes the proven monument output of the second
phase (final section). Other live-object effects remain outside this partial
snapshot reconstruction.

All **94,493/94,493** post-elevation unknown shore cells now resolve: **89,887**
take `0x605`, **4,606** take `0x61A`. Unshifted matches are **51,199/52,597**
(97.342%) and **2,144/2,319** (92.454%); shifted matches are **0/37,290** and
**0/2,287**. Their residuals remain measured without fitted constants. Full
totals and the complete scratch-writer inventory appear in the final section.

**The three existing `0x61C` shore-arm shortfalls are not solely the id shift.**
Splitting the corpus by whether the city's road band is shifted (the road band and
the `0x61C` band are both "upper", so this is the right cohort split), the arms are:

| Arm | Unshifted cities (93) | Shifted cities (74) |
|---|---|---|
| `0x4-shore-water-ring` | 47,649 emitted, 33,070 matched (69.40 %) | 35,580 / **0** (0.00 %) |
| `0x4-shore-water-fringe` | 33,264 / 22,292 (67.02 %) | 23,760 / 128 (0.54 %) |
| `0x4-shore-shore-ring` | 3,040 / 1,999 (65.76 %) | 1,973 / 23 (1.17 %) |

So the shifted cohort is fully explained by the shift, but **the unshifted cohort
still only reaches 66-69 %**, i.e. ~11,000 fringe cells are wrong with no shift to
blame. Their `stored - predicted` deltas are *scattered* (`-8..+8` all populated
plus 9-16/17-32/33-64) rather than constant - e.g. MP61 road 99.87 % matched but
fringe 0.00 % with deltas spread over `-8..+6` and 95/166/46 in the three large
buckets - while STGW-Anyang and Badaling are 100 %. Only 20 of the 84 unshifted
cities with >=50 fringe cells reach 100 %.

A scattered residual in a city whose other bands are exact originally suggested a
**condition** error in the neighbour count - that `FUN_004B8F70 @0x004B8F70` might
write descriptors `FUN_004BC140 @0x004BC140` then reads. **That hypothesis is
refuted.** `FUN_004B8F70` (0x4B8F70..0x4B9020) is straight-line: it ANDs each of the
eight neighbour terrain words (flat offsets -228,-227,+1,+229,+228,+227,-1,-229 via
`0x00F6A650/54`, `0x00F6A9E4/DC`, `0x00F6AD74/70/6C`, `0x00F6A64C`) with the pushed
mask, normalises each to 0/1 into `0x0101C5B8..0x0101C5D4`, and returns; it writes
no descriptor or cache. `FUN_004BC140` counts the nonzero dwords in that range. So
at `0x53FFBE push 0x4000000` -> `0x53FFC3 call FUN_004B8F70` -> `0x53FFCB call
FUN_004BC140` -> `0x53FFD0 cmp eax,8` the count is *identical* to the model's
`neighbourMaskCount(request,0x4000000u)`. The `img[cell]==0` gate at `0x53ECF4`
(`mov eax,[0x0101C560]` / `mov ecx,[4*eax+0x00FE9880]` / `test ecx,ecx` / `jne`)
also holds, so the arm is genuinely reached.

The residual is instead an **even within-band authored offset**. For the
water-fringe arm the residual is even for **57,024/57,024** corpus cells (100 %).
Because this arm's variant is `randomByte%48` (which has the same parity as
`randomByte`), that is exactly `storedVariant ≡ randomByte (mod 2)`, i.e.
`storedVariant = (randomByte%48 + 2h) mod 48` for an integer `h`. Evenness is a
data fact, not a formula artifact: the water-ring arm's residual is odd for
15.25 % of its cells. (`shore-ring`'s residual is also even, but its variant
`randomByte%24 + (randomByte&1)` is always even, so its stored word carries no
parity signal either way.) The residual reduced mod 48 is a per-city constant
(shifted cities: mode 16 or 18, i.e. -32/-30, the same shift as their road band at
75-100 % purity; unshifted
cities: mostly 0, but ~20 scatter off it), i.e. the same stale-vintage class as the
per-(city, band) archive id shift. No MAP-derivable rule reproduces it: over
`storedVar == (a*r + b*X + c) % 48` with `a,b` in 0..7 and `X` one of {x, y, x+y,
x-y, xy, cell, sub, draw, fert, bebf3a, road, f05160, eac3f0, bf8a4a, feature},
the best is always exactly the current `r%48` = **22,292/33,264 (67.02 %)** on the
unshifted cohort; local neighbour signatures, BFS distance to non-water, and
`r%48 -> storedVar` injectivity all fail as well.

The three formulas themselves are confirmed against the decompiled
`FUN_0053FC30 @0x0053FC30`: `0x54000B`/`0x540011` give `base(0x61C) + r%0x30`,
`0x54001F`/`0x54003B`/`0x540040`/`0x540051` give `base(0x61C) + r%0x18 + (r&1)`, and
`0x53FFEE`/`0x53FFF4`/`0x53FFF6` give `base(0x61C) + 0x30 + r%0x18`. **Conclusion:
the unshifted-cohort shortfall is not MAP-derivable, so the model keeps the
executable's formula and does not tune a constant to the oracle.** The corpus lock
`shoreVariantStructureTests` in `game_lab/map_reference_tests.cpp` asserts the
even-residual structure over the whole corpus.

The additional elevation-descriptor hypothesis was measured using the existing
`elevationDescriptor` matcher directly, with no duplicate implementation:
**47,649/47,649** unshifted water-ring, **33,264/33,264** water-fringe and
**3,040/3,040** shore-ring cells return exactly `{C=0,S=0,B=-1,unknown=false}`.
That single descriptor class has the same exact-match counts **33,070**, **22,292**,
**1,999**; even choosing its best constant residual gives no extra matches.
The descriptor therefore explains **0/26,592** existing unshifted residuals.
It is absent from their arithmetic. The matcher overwrites it on a matched
shore row; the unmatched all-shore ring ignores it.

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
* Elevation's static table `0x00825C28` and neighbour offsets are now ported; in-place
  height repair remains outside the snapshot reconstruction (elevation section).
* The `0x61B` reads are shifted views of the river-object pointer grid, null in the
  first bare-load reconstruction (item 2). `FUN_0041F400` ends with a second
  reconstruction at `0x41F63A`; its monument output is now ported. Full terrain
  mutation, shore/road live-object effects and height repair remain outside it.
* Setter 13's live-object and nonzero road-material paths remain outside the
  bare-load model. Its null-object, zero-material path and elevation default are
  resolved in the final section; the 208 elevation ramp remaps are unchanged.
* The MAP `mapperRandom` (729,311) is not consumed as the engine's per-cell random grid (item 12), but
  `0x00E92DD0` **is** MAP-carried at offset 885,271 and `0x00E9F8E0` **is** MAP-carried at offset 937,255
  (`original_terrain.hpp`). Both are restored by `FUN_0052E7C0 @0x0052E7C0`, which is why setters 5 and 17
  are modelled from the payload.
* The `0x00FE9880[cell]==0` precondition that lets setters #4/#10 run is assumed from the chain gate; its
  interaction with the rest of `FUN_0053EC90` was not re-read here.

## Load path: restored arrays, derived caches and reset object state

The remaining unmodelled branches of `FUN_0053EC90` read per-cell arrays with
different provenance; the elevation and shore sections supersede the earlier
blanket claim that a bare `.map` cannot supply them. Established on the owned exe (SHA-256
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

Consequence: a missing runtime array alone does not establish non-derivability.
The elevation caches are reconstructed from MAP heights, and the bare-load
river-object pointer grid is determinately null (item 2). Two other inputs are
MAP-carried because they are
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
drawn". Measured over all 167 maps, every one of the 19,935 current chain-no-write
cells stores nonzero art and `0` store zero. The count was 19,841 before the
last four setters added 94 road no-writes, and 114,937 before the elevation
prepass; most former declined cells were elevation art written before
dispatch. See
[`MAP_SG3_PIPELINE.md`](MAP_SG3_PIPELINE.md#the-stored-surface-grid-is-an-oracle-not-a-derivation).
This was previously misclassified as "determinate no-ground" — and, before that, as
an unknown.

### MAP-only coverage, measured over all 167 `Cities/*.map`

`active 2,107,836`; `known 2,087,691` (99.044 %), every emitted code resolving in
`China_Terrain.sg3` or explicit `China_Elevation.sg3`; `unknown 210` (0.010 %);
determinate `chain-declined` 19,935. The unknown residue is
`elevation-road-material` 208 and `elevation-block-declined` 2.
The image reconstruction is partial: elevation ramp remaps and live-object redraw
paths other than the final section's monument projection remain outside the port.
The four remaining ground arms remove 36,993
unknowns: 36,899 emit and 94 are determinate no-writes. The whole-map block tiers removed
the entire `0x80-farm-block` unknown branch:
before block tiers, `169,520 - 36,950 = 132,570` unknown cells.
The current identity is `2,087,691 + 210 + 19,935 = 2,107,836` active cells.
The elevation writers remove 874 unknown cells from the prepass baseline; the
shore-specific descriptor table removes another 94,493.
(History: the earlier ceiling `known 1,514,539` / `unknown 478,360` predates
modelling setters 5 and 4; `1,514,539 + 246,802 + 62,038 + 36,950 = 1,860,329`,
i.e. road, the two `LAB_0053FF8A` shore arms and the block tiers are the changes.
The `0x4-shore` residue fell from 156,935 to 94,897 when those arms were ported:
`156,935 - 57,024 - 5,014 = 94,897`.)

## Mismatch classes: every residual cell now has a named cause

The `f3b0eab` baseline headline was **1,766,664/2,107,836 active cells matched (83.814 %)**;
the SAV19/paired-elevation section below records the current **1,767,172** matches.
`matched` counts emitted codes that equal the stored grid (`1,766,664` of
`2,087,691`, 84.623 % of emitted). The remaining
`321,027` cells are classified per branch, and the classes partition `mismatched`
exactly - the sweep treats a city whose classes do not sum to its `mismatched` as a **failure**,
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
| stored word is 0 while the chain emitted | 28 |
| stored word names a tileset different from the prediction | 64 |
| same tileset, stored index within +-8 of the prediction | 14,069 |
| same tileset, stored index further off | 306,866 |
| **total** | **321,027** |

Before the elevation port, the 9,829 non-tileset-3 cells all named **tileset 16 (`China_Elevation.sg3`)**; no
other tileset appears (direct corpus probe of `stored>>14`). So the chain does not
merely pick the wrong record on those cells, it claims a cell whose art is not
ground art at all: the dispatch chain models setters that write into the ground
namespace, and the engine's real choice for those cells produced elevation art.
The elevation prepass and setter 2 now account for that missing writer. The legacy
JSON field `stored_not_tileset3` counts a different tileset from the prediction
(3 or 16); retaining its name preserves existing serializers and the partition.

The 306,866 "further off" cells are bucketed by magnitude of
`storedIndex - predictedIndex`: 9-16, 12,203; 17-32, 281,818; 33-64, 10,165;
65-128, 1,324; 129-256, 315; 257-512, 1,011; 513+, 30. The 17-32 bucket accounts for
281,818/306,866 (91.837 %) of larger deltas. The farm block tiers
contribute 34,362 cells to the 17-32 bucket because `0x604` carries the same known
per-city vintage shift as the land and upper bands (next section). Note that the
*shift* only accounts for the shifted cohort: in the 93 cities whose upper bands
are exact the three `0x61C` shore arms still reach only 66-69 % (item 2). The water-fringe
residual is an **even within-band authored offset**; the neighbour-count condition-error hypothesis
was refuted. The selector keeps the executable's formulas without fitting either
offset to the oracle.

Before elevation, per-branch residual counts (matched/emitted percentages) were `0x1-road` 95,978
(61.11 %), `0x4-shore-water-ring` 50,159 (39.73 %), `0x2-land` 34,673 (53.45 %),
`0x4-shore-water-fringe` 34,604 (39.32 %), `0x80000000-flood` 10,076 (37.59 %),
`0x80-farm-block-4x4` 13,500 (55.51 %), `0x80-farm-block-3x3` 12,242 (55.81 %),
`0x80-farm-block-2x2` 8,665 (57.85 %), `0x40000-canal` 4,872 (52.04 %),
`0x4-shore-shore-ring` 2,992 (40.33 %), `0x101000-twin` 1,700 (49.49 %),
`0x2000000-feature` 981 (56.03 %), `plain-land` 156 (99.92 %), `0x80-farm` 69
(99.993 %), `plain-land-block-3x3` 58 (99.41 %), `plain-land-block-2x2` 7
(99.89 %) and `0x80-farm-block-fallback` 6 (99.975 %); `plain-land-block-4x4`
has 0 residual (100 %).

### The per-(city, record-id band) id shift is the largest single cause

The branch breakdown in this subsection is the `c303acb` baseline, before the
four remaining ground branches. Current aggregate totals are above and in the
final section; the established vintage analysis is unchanged.

The 17-32 magnitude bucket is not one branch's problem. Cells whose stored index
sits 17-32 records from the prediction, by branch: `0x1-road` 94,669,
`0x4-shore-edge` 36,990, `0x4-shore-water-ring` 31,876, `0x2-land` 26,340,
`0x4-shore-water-fringe` 21,923,
`0x80-farm-block-4x4` 13,488, `0x80-farm-block-3x3` 12,233,
`0x80000000-flood` 9,949, `0x80-farm-block-2x2` 8,641, `0x40000-canal` 4,776,
`0x4-shore-river` 2,136, `0x4-shore-shore-ring` 1,941, `0x101000-twin` 1,700,
`plain-land-block-3x3` 46,
`0x80-farm` 5, `plain-land-block-2x2` 3, `0x80-farm-block-fallback` 1 and
`elevation-direct` 101 - **266,818 cells, 92.033 % of the 289,915 large-delta
residual**. The dominant cause
is the per-(city, band) vintage shift; the small ordinary-farm and plain-land-block
tails also occupy this magnitude bucket.

Inside a city the offset is constant *for a given band*: Anyang has 1,734 of its
1,736 road cells at exactly -30, its `0x2-land` mode is -30, and all 384 of its
`0x4-shore-water-ring` cells are -30, while Bo, Badaling and Chizhou are uniformly
0. Over the corpus the road offset splits the cities cleanly - 93 cities at 0 with
150,824 matched road cells, and 74 cities where road, land and shore-water-ring
match **exactly 0.000** of their cells; shore-water-fringe and shore-shore-ring
reach only 0.54 % and 1.17 %. Ordinary `0x80-farm` (99.994 % vs 99.993 %) and
`plain-land` (99.901 % vs 99.937 %) remain nearly exact in both groups. An all-or-nothing
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
indices. The historical ordinary-farm residual was a separate, non-band cause,
now resolved by whole-map stamps - see "Every farm residual is a wrong branch,
not a wrong variant" below. The newly emitted `0x604` farm block tiers themselves
exhibit the known vintage shift.

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
grid is a stale oracle above the farm and plain-land bands, so the current `84.623 %` is a floor
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


## Composite placements store one image word

Measured over the same corpus with the block decomposition already used by the
stored-grid renderer (anchor = `cell - (draw & 7) - ((draw >> 3) & 7) * 228`):

- 6,207 composite blocks (4,557 of 4 cells, 549 of 9, 1,020 of 16, 58 of 25, 23 of 1).
- The stored image word is **identical in every cell of a block: 6,207/6,207
  (100.00 %)**, 0 exceptions.
- The per-cell random byte (`0x00F1E780`) **varies inside 6,184/6,207 blocks
  (99.63 %)** and is constant in only 23.

For these composite placements, art is written once and stamped across the footprint. Any rule of the form
`code = base + (random & 7)` evaluated *per cell* can match at most one cell per
block, which is exactly the low-accuracy signature of the block branches.

This suggested a specific fix for these composite placements - read the variant from the stored draw-grid anchor cell - and
that prediction was **tested and refuted**. Applying it corpus-wide
(`groundCodeRequest` taking `randomByte[anchor]`) moved `matched` from 1,299,617 to
1,296,643 (measured before setter 5 was modelled; **worse by 2,974**; `0x80000000-flood` 14.57 % -> 11.88 %,
`0x2-land` 33.14 % -> 30.05 %) and the change was reverted. This refutes that
stored-anchor substitution; it does not refute the executable's rectangle
acceptance and tier offsets. The whole-map port below derives the origin from
working terrain and image occupancy, tries the executable's arms, and uses
`+8+(origin random&3)` or `+12+(origin random&1)`. It recovers 19,783 matches.
The stored draw/sub grids never choose a predicted origin or variant.

### Every farm residual is a wrong branch, not a wrong variant

**Resolution (whole-map pass):** before block tiers, `0x80-farm` had 65,742 mismatches
and 1,031,695/1,097,437 matches (94.01 %). Whole-plot block stamps
(`FUN_004B7520 @0x004B7520`) overwrite the cells behind that dominant residual.
The pass moves **65,710 = 1,097,437 - 1,031,727**
cells out of `0x80-farm` into the block branches, leaving **1,031,658/1,031,727
matched, 69 mismatched (99.993 %)**. The 69 span 19 cities, led by MP48 (21),
MPcanal2 (8) and Shanhaiguan_S (7); they comprise 16 within ±8, 39 larger offsets,
8 stored zero and 6 stored tileset 16, with no single dominant magnitude bucket.
The following analysis records the evidence before block tiers were ported.

Setter 17's farm formula is `T(0x602) + 9*rank + random%9` with `rank` in 0..4
(`FUN_004BD8C0 @0x004BD8C0` calls `FUN_004BD940 @0x004BD940`, cited above), so the
reachable archive indexes are `imageIndex[2] - 1 + 0..44`. For `China_Terrain.sg3`,
`imageIndex[2] = 202`, so the whole farm band is 45 records wide (201..245). **No
variant error can move the stored index by more than 44.** That bounds the residual
without any new measurement. Before block tiers, of `0x80-farm`'s 65,742 mismatches,
65,525 sit in the 65-128 (50,857) and 129-256 (14,668) magnitude buckets alone,
which the formula cannot reach.

An independent per-cell probe confirms the remaining 33-64 group is out of band too.
The probe replicates the setter-17 gates from the payload alone
(`(terrain & 0x10001) == 0 && (terrain & 0x80)`, no 8-neighbour carrying `0x10000`,
`random & 0xF8 != 0`) and computes the prediction directly, over all 167 maps:

- it reproduces the branch's **matched** set exactly - 1,031,695 cells, identical to
  the historical before-block score's `1,097,437 - 65,742` - so the formula is confirmed;
- it attributes 79,768 extra cells to the branch (all of them mismatches, since the
  matched sets are identical), so its mismatch set is a strict superset of the
  chain's: 145,510;
- of those 145,510, **0 have a stored index anywhere in the band (offset 0..44)**.
  The stored art is either another tileset - 8 (China_Mon_Earthen_Greatwall_1) 6,018,
  6 (China_Mon_Grand_Canal) 4,061, 2 (China_General) 1,943, 5 (China_Mon_Tumulus)
  115, 16 (China_Elevation) 6 - or a tileset-3 record far outside the band.

The historical probe established that the chain **claimed cells the engine gave
other art**, rather than selecting the wrong farm variant; it did not identify
which writer owned them. The whole-map stamp pass now resolves the dominant cause.
Before elevation, the 132,570 `chain_unknown` cells required the runtime descriptors listed
above, while 227,592 of the 249,333 large-delta mismatches lay in the 17-32 bucket,
dominated by the known vintage offset. Porting more setters addresses the unknown
branches, but cannot recover the missing authoring-time archive table. The stored grid
remains the measured render source, with intended art in shifted cities still open.

## Whole-map farm and plain-land block tiers (2026-10-20)

This section supersedes the farm-block unknown ceiling, the plain-land block
approximation, and the attribution of every farm residual to missing setters
above. History before block tiers (`1e87f22`): 1,474,883/1,823,379 emitted cells matched (80.887%), 169,520/2,107,836 active cells unknown.
The consecutive-image farm stamp was the missing cause of most farm residuals.
The composite-placement measurement remains valid for its own population; it does
not describe this stamp.

All executable facts below were verified against the owned `Emperor.exe`, SHA-256
`0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`, imagebase
`0x400000`. `.data` VA `0x814000` maps to file offset `0x414000`. No original
listing or binary is included in the repository.

### Predicate, table and traversal evidence

`FUN_004B7E80 @0x004B7E80` changes no cell layers (it does assign the current-cell
scratch global). Its exact farm acceptance rules are:

| Condition | Instruction evidence |
|---|---|
| `x>=3`, `y>=3` | `0x4B7E87`, `0x4B7E98` |
| `x+w-1 < width-3`, `y+h-1 < height-3` | `0x4B7EAE..0x4B7EB7`, `0x4B7ECA..0x4B7ED3` |
| every cell has `terrain&0x80` | `0x4B7F23` |
| every fertility byte is unsigned `>=100` | `0x4B7F27`, refusal `0x4B7F2E` |
| clear `0x10000000`, `0x8`, `0x40`, `0x4000` | `0x4B7F30`, `0x4B7F37`, `0x4B7F3B`, `0x4B7F3F` |
| clear `0x87375227`, `0x10000`, `0x800` | `0x4B7F44`, `0x4B7F4B`, `0x4B7F52` |
| no cardinal neighbour with `0x10000` | push `0x10000` at `0x4B7F5B`, call `FUN_004BA230` at `0x4B7F61` |
| no diagonal neighbour with `0x10000` | push `0x10000` at `0x4B7F71`, call `FUN_004BA1E0` at `0x4B7F77` |
| `FUN_004B0800(cx,cy,3)==0` | subtract grid origin `0x4B7F8F`, divide by 228 `0x4B7F91..0x4B7FB0`, push 3 `0x4B7F98`, call `0x4B7FB4` |

`FUN_004BA230 @0x004BA230` returns the count of cardinal matches: N flat -228
(`0x4BA23A`), E +1 (`0x4BA248`), S +228 (`0x4BA252`), W -1 (`0x4BA25C`).
`FUN_004BA1E0 @0x004BA1E0` returns whether any diagonal matches: NE -227
(`0x4BA1E8`), SE +229 (`0x4BA1F7`), SW +227 (`0x4BA206`), NW -229 (`0x4BA215`).

The static table `DAT_0081FF18` starts at **file offset `0x41FF18`**. It has six
rows and six columns of `(flat offset, draw offset)` dword pairs. Column stride is
8 bytes (`0x4B7FCA`), row stride 48 bytes (`0x4B7FE2`). Its first members are:

| Row | Column 0 | 1 | 2 | 3 | 4 | 5 |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | 0 | 1 | 2 | 3 | 4 | 5 |
| 1 | 228 | 229 | 230 | 231 | 232 | 233 |
| 2 | 456 | 457 | 458 | 459 | 460 | 461 |
| 3 | 684 | 685 | 686 | 687 | 688 | 689 |
| 4 | 912 | 913 | 914 | 915 | 916 | 917 |
| 5 | 1140 | 1141 | 1142 | 1143 | 1144 | 1145 |

The second member at row r, column c is `8*r+c`: row 0 has 0..5, row 1 8..13,
row 2 16..21, row 3 24..29, row 4 32..37, row 5 40..45. These are format facts;
the implementation computes the equivalent flat offset `228*r+c`. The predicate
reloads the origin at `0x4B7FC4`, so offsets are absolute relative to that origin.

`FUN_004B05F0 @0x004B05F0` sets storage bounds `first=(228-side)/2`,
`last=first+side-1`, `middle=(first+last)/2` (`0x4B061A..0x4B0657`). The size table
at VA `0x856C64`, file `0x456C64`, gives `(side,side,flatOrigin,rowRemainder)`:
`(56,56,19694,172)`, `(84,84,16488,144)`, `(112,112,13282,116)`,
`(140,140,10076,88)`, `(170,170,6641,58)`, `(226,226,229,2)`.

`FUN_004B0770 @0x004B0770` defines the diamond. In its four quadrants it accepts
`x+y>=first+middle`, `x-y>=middle-last`, `x-y<=last-middle`, or
`x+y<=last+1+middle`. `FUN_004B0800 @0x004B0800` first returns false outside that
diamond (call `0x4B080C`, gate `0x4B0814..0x4B081A`), then tests its four margin
strips: `x-3+y<first+middle` (`0x4B0832..0x4B083D`),
`x-y-3<middle-last` (`0x4B084B..0x4B0856`), `x-y+3>last-middle`
(`0x4B086C..0x4B0877`), or `x+y+3>last+1+middle` (`0x4B0882..0x4B0891`).
The farm caller passes **crop-local** coordinates after subtracting flat origin;
the plain predicate passes **storage** coordinates. This asymmetry is present in
the instructions; the port preserves it.

`FUN_005403C0 @0x005403C0` first clears/prepares the grid through `FUN_0053EAA0`
(`0x54040B..0x54043E`), then visits increasing rows, and within each row increasing
columns from the diamond row tables: table loads `0x54044F/456/45D`, chain call
`0x54046B`, column/cell increments `0x540473/474`, row increment `0x54047F`, loop
`0x540482`. The rectangle driver `FUN_0053E870 @0x0053E870` also uses increasing
y then x (chain call `0x53E9C5`). `FUN_0053EC90 @0x0053EC90` skips setters 4..18
when the current image is nonzero: read `0x53ECF4/0x53ECF9`, test `0x53ED00`,
branch `0x53ED02`. The new map pass calls the existing per-cell selector, carries
new stamp ownership across cells, and keeps a private image-occupancy grid. It
never consults stored image, draw or sub-tile bytes to choose an origin or tier.
Other unported writers remain the existing selector's measured limitations.

### Tier and stamp evidence; reconciliation with composite placement

Setter 17 `FUN_0053EE00 @0x0053EE00` reaches block attempts only after its existing
farm/river gates: `terrain&0x10001==0` (`0x53EE07`), `terrain&0x80`
(`0x53EE12`), zero `0x10000` neighbour count (`0x53EE1E..0x53EE32`), and
`random&0xF8==0` (`0x53EE84`). `b=random&6` at `0x53EF46` selects attempts in
order: b>=6 (`0x53EF4B`) 4×4, b>=4 (`0x53EF93`) 3×3, b>=2 (`0x53EFE8`) 2×2.
All use `FUN_004B7E80`. Accepted image bases are `T(0x604)+0x22`
(`0x53EF66/0x53EF75`), `T(0x604)+0x10+9*(random&1)`
(`0x53EFAE..0x53EFC9`), and `T(0x604)+4*(random&3)`
(`0x53F003..0x53F01B`). The failed-attempt fallback directly calls `FUN_004BD8C0`
at `0x53F03B`, resolves `0x602` at `0x53F04E/0x53F069`, and stores at `0x53F071`.
It does not call `FUN_004EDBA0`. For b==0 there is no attempted predicate, so this
fallback is already determinable through the per-cell API.

`FUN_004B7520 @0x004B7520` uses the same offset table and writes
`img=T(code)+variant+n`, with n initially 0 (`0x4B7527`), add n `0x4B7629`,
**increment n `0x4B762B`**, store `0x4B762D`. n continues across rows. It also
sets `terrain=(terrain&0x93872790)|param_7` (`0x4B75DB/0x4B75E1`), clears the
object id (`0x4B75F3`, param_1=0), masks sub-tile with `0xF0` (`0x4B760D`), and
sets draw `|0x40` (`0x4B761C`). Both ported callers pass param_7=param_8=0
(`0x53EF62/64`, `0x53F11B/11D`). The farm predicate never tests image occupancy,
so a later origin may overwrite an earlier stamp; a covered origin itself skips
the chain. Those two cases have separate assertions.

The existing **composite** writer is `FUN_004B72B0 @0x004B72B0`: it stores one
unchanged image word (`0x4B7423`), sets sub-tile side-1 (`0x4B73EA..0x4B7415`),
and copies the table's draw-offset member (`0x4B742A/2E`). Thus the older
6,207/6,207 constant-word composite measurement is consistent with the incrementing
stamp. The scorer compares farm stamp cells individually, including the origin
(upper-left crop coordinate) at n=0, then n=`row*side+column`.

Independent stored-grid measurement at surviving predicted farm origins:

| Side | Origins | Entire footprint consecutive from origin word | Entire footprint constant | All sub-tile low nibbles zero | Footprint contains composite SG3 record |
|---:|---:|---:|---:|---:|---:|
| 2 | 5,186 | 5,035 | 0 | 5,186 | 0 |
| 3 | 3,123 | 2,955 | 0 | 3,123 | 0 |
| 4 | 1,952 | 1,706 | 0 | 1,952 | 0 |

Footprints with overwritten cells need not remain consecutive. The complete
constant-word hypothesis is independently refuted by **0/10,261** farm origins;
9,696/10,261 still have every consecutive id in the stored footprint. No draw-grid
anchor or archive-shift compensation is used.

Setter 18 `FUN_0053F090 @0x0053F090` uses its existing guard `0xAEFEDE6F`
(`0x53F090`), random gate `0xF8` (`0x53F0AB`), and b mask 6 (`0x53F0F5`). Its
4→3→2 tests are at `0x53F0FA/0x53F149/0x53F1A5`; bases are `T(0x603)+0x2A`
(`0x53F11F/0x53F12E`), `+0x18+9*(random&1)` (`0x53F16E..0x53F189`),
`+8+4*(random&3)` (`0x53F1CA..0x53F1E2`), fallback `+(random&7)`
(`0x53F202..0x53F21A`). Its `FUN_004B7CA0 @0x004B7CA0` arguments are
`(...,side,side,-1,1,0,1,1)` (`0x53F0FF..0x53F10F`, repeated at `0x53F15E`,
`0x53F1BA`). The predicate has the same rectangular margins
(`0x4B7CA7/0x4B7CBE/0x4B7CD7/0x4B7CF6`), checks storage-coordinate diamond edges
(`0x4B7D81`), refuses mixed `0x10000` membership (`0x4B7DB5..0x4B7DDC`), masks
terrain with `0xBFFFDEEF` (`0x4B7DE9`), and requires **image==0**
(`0x4B7E30..0x4B7E3A`, param_6=1). `FUN_00416BD0` is immaterial because
param_8=1 bypasses its refusal (`0x4B7E1C/20`). `FUN_00416BB0 @0x00416BB0`
tests the runtime object pointer at `0x890210` (`0x416BB4/0x416BBF`); the port
retains the bare-load zero-pointer premise. This array is beyond `.data`'s raw
end `0x88F000`, hence initially zero-filled; it is not a MAP input. A live-game
object pass is outside this reconstruction.

### Historical corpus result and assertions (before elevation)

All 167/167 cities processed, 0 failures. The initial farm-only sweep gave
1,543,093/1,860,326 matched (82.947%), 132,573 unknown; the remaining three requests
were MP48 cells outside the traversal with b==0. Resolving that context-free
fallback adds 3 emitted cells, 0 matches, and removes all three unknowns. The
plain-land phase then adds **26,715 matches** without changing coverage.

| Metric | Before block tiers (`1e87f22`) | After block/composite tiers, before elevation |
|---|---:|---:|
| Active cells | 2,107,836 | 2,107,836 |
| Emitted / active | before: 1,823,379 (86.505%) | 1,860,329 (88.258%) |
| Matched / emitted | before: 1,474,883 (80.887%) | was 1,589,591 (85.447%) |
| Unknown / active | before: 169,520 | 132,570 |
| Mismatched / emitted | before: 348,496 | was 270,738 |
| Mismatch class sum | before: 348,496 | was 270,738 |

Historical pre-elevation classes: stored zero 16, other tileset 9,829, same-tileset delta within ±8
11,560, larger delta 249,333; the historical sum was **16+9,829+11,560+249,333=270,738**. The emitted
ratio is confounded by newly covered cells and archived id vintages; absolute
matches rise **114,708**, unknown falls **36,950**, and neither closed offset class
is fitted. Farm's ordinary branch becomes 1,031,658/1,031,727 matched; 37 old
accidental matches are reassigned by exact non-local stamps, alongside the much
larger correction of formerly mismatched farm cells.

Unshifted means road branch matched/emitted >=50%, exactly 93 cities; shifted is
the other 74. In each table entry below **cells=emitted**; the numerator is matched.

| New branch | All cities matched/emitted | Unshifted (93) matched/emitted | Shifted (74) matched/emitted |
|---|---:|---:|---:|
| `0x80-farm-block-2x2` | 11,890/20,555 | 11,890/11,897 (99.941%) | 0/8,658 (0%) |
| `0x80-farm-block-3x3` | 15,462/27,704 | 15,462/15,471 (99.942%) | 0/12,233 (0%) |
| `0x80-farm-block-4x4` | 16,844/30,344 | 16,844/16,856 (99.929%) | 0/13,488 (0%) |
| `0x80-farm-block-fallback` | 24,051/24,057 | 14,072/14,077 (99.964%) | 9,979/9,980 (99.990%) |
| `plain-land-block-2x2` | 6,453/6,460 | 2,553/2,560 (99.727%) | 3,900/3,900 (100%) |
| `plain-land-block-3x3` | 9,815/9,873 | 3,748/3,789 (98.918%) | 6,067/6,084 (99.721%) |
| `plain-land-block-4x4` | 10,496/10,496 | 4,032/4,032 (100%) | 6,464/6,464 (100%) |

The pre-elevation whole-chain cohort results were 1,007,585/1,041,309 emitted matches
(unshifted, 75,473 unknown / 1,179,755 active), and 582,006/819,020
(shifted, 57,097 unknown / 928,081 active). `0x604` sits above the plain-land band;
its shifted cohort remains uncompensated. Ordinary plain land moves from
200,545/227,481 matched (88.16 %) to 200,496/200,652 (99.92 %).
Anyang moves from 5,673/8,859 to 5,831/9,079 matched,
unknown 914→694; its 381 stamped farm cells belong to the shifted cohort.

Stored-grid render results are identical: exact 139/167 cities, 2,107,808 nonzero,
0 decode failures; composite 6,169 blocks, 38,433 confirmed cells, 2,243 recovered,
286 mismatched, 643 foreign. Only chain scoring uses the new pass; the experimental
per-cell viewport remains the existing diagnostic.

Tests cover each forbidden bit, both rectangular margins, four diamond strips,
outside-diamond return convention, all eight neighbour refusals, fertility
threshold, all tier parity formulas, 4→3→2→fallback, row-order overwrite, covered
origin skip, plain occupancy refusal, unchanged source layers, and exact corpus
totals/branch cohorts/consecutive-vs-constant structure. Temporarily replacing
`variant+row*side+column` with `variant` fails the stamp-order assertion; the rule
was restored. No decreasing-match/increasing-unknown candidate was retained.
Pre-elevation required runs: no-data release **1,066 assertions** with successful SKIP;
original-data release **2,287 assertions**; original-data UBSan **2,287 assertions**,
all passing. `git diff --check` is clean.

## Land, flood and twin composite tiers

The `f88d75a` baseline emitted 1,860,329, matched 1,569,808, left 132,570 unknown,
and mismatched 290,521. Reading setters 6/8/7 and both acceptance helpers against
the same executable hash above found omitted arms, with no discrepancy in the
existing base codes, terrain dispatch order or random-byte source:

| Branch | Setter evidence | Missing whole-map behavior |
|---|---|---|
| `0x2-land` | `0x53F679/7E/8E` chooses `0x607/608`, else `0x606` and required terrain 2 at `0x53F69E` | `FUN_004B8000` calls `0x53F6CA` (3×3), `0x53F710` (2×2); variants `12+(r&1)` at `0x53F6E3/E6`, `8+(r&3)` at `0x53F729/2C`; fallback `r&7` at `0x53F76C` |
| `0x80000000-flood` | `0x53F560` tests the high bit; `0x53F570` selects `0x60E` | Same 3×3→2×2→single order; helper calls `0x53F59B/0x53F5E8`, variants `0x53F5B8/BB`, `0x53F605/608`, fallback `0x53F647` |
| `0x101000-twin` | `0x53F794/99` tests both bits; `0x53F7A6` selects `0x60F` | Only 2×2 via `FUN_004B8100` at `0x53F7D1`, `8+(r&3)` at `0x53F7EE/F1`, fallback `r&7` at `0x53F81D` |

Here `r` is the origin's `0x00F1E780` byte, read at `0x53F6DD/0x53F723`,
`0x53F5B2/0x53F5FF`, or `0x53F7E8`; no auxiliary byte feeds these variants and
there is no farm/plain `r&0xF8` gate or divisor. All callers pass image test 1 and
excluded terrain `0x100`. `FUN_004B8000 @0x004B8000` requires nonnegative local
x/y and `x+side-1<activeSide`, `y+side-1<activeSide` (`0x4B8007/17/30/49`), then
for every target `(terrain&0xAFFEDE6F)==required` (`0x4B8099/9F`), image 0
(`0x4B80AD/AE`), and no `0x100` (`0x4B80BF`). It has no diamond-edge test or
three-cell margin. Twin's `FUN_004B8100 @0x004B8100` instead requires
`(terrain&0x101000)==0x101000` (`0x4B819D/9F`), permitting extra terrain bits;
its image and `0x100` checks are `0x4B81AB/AC` and `0x4B81BD`.

The writer is **`FUN_004B72B0 @0x004B72B0`**, distinct from farm/plain's
`FUN_004B7520`: it writes the same image word to every target (`0x4B7423`),
terrain `(old&0x93872790)|required` (`0x4B73BA/C0`), and sub-tile
`(old&0xE0)|(side-1)` (`0x4B73DC..0x4B7415`). Table `0x0081FF18` (file
`0x41FF18`) has flat offsets `row*228+column` and draw offsets `row*8+column`;
`0x4B742A/2E` copies the latter, setting draw `0x40` on the view's draw cell
(`0x4B72C8..0x4B7300`, `0x4B7434..43`). Twin's single-cell fallback assigns
draw `0x40` (`0x53F82B`), while land/flood OR it. The pass already builds requests
from working state; these predicates read its zero/nonzero images and the stamp
mutates its terrain/sub/draw before later row/column visits. Stored draw, sub-tile
and image words never select an arm or origin.

Before porting, a cheap unshifted-cohort structural probe reproduced the exact
baseline. Same-tileset residuals included both parities and all residues mod 4/8;
apart from land's 22 distant cells, every mismatch had a **positive** delta 1–13
(land/flood) or 1–11 (twin). This differs from shore's even authored offset:

| Branch | Same-tileset cells | Even / odd deltas, including exact | Matched by stored group size 1 / 2 / 3 |
|---|---:|---:|---|
| land | 39,845 | 33,898 / 5,947 | 24,685/24,706; 0/11,701; 0/3,438 |
| flood | 6,068 | 4,599 / 1,469 | 2,352/2,352; 0/2,276; 0/1,440 |
| twin | 1,666 | 1,335 / 331 | 734/734; 0/932; no 3×3 |

Neighbour-pattern dependence was strong: pattern 0 (no same-kind neighbours) was
exact for land 3,000/3,000, flood 82/82 and twin 65/65; pattern 255 was land
398/2,645, flood 75/1,233 and twin 1/80. Coordinate parity did not select an
arm: the four x/y parity groups had land match rates 61.69–62.57%, flood
37.01–40.54%, and twin 41.01–46.92% among same-tileset cells. Stored variants
covered exactly 0–13 (land/flood) or 0–11 (twin), plus the land tail. The apparent
9–16 errors therefore came from missing block arms, not an authored offset or
an alternative random/auxiliary byte. The stored group measurements diagnose the
error; they are never predictor inputs.

Every baseline unshifted land cell is accounted for:
**24,685 exact + 11,537 within ±8 + 3,601 in 9–16 + 22 distant + 5,909 tileset 16
= 45,754**. After the port: **39,820 exact + 2 within ±8 + 1 in 9–16 + 22 distant
+ 5,909 tileset 16 = 45,754**. The three small residuals are -7, -4 and -11;
no rule is fitted to them. Flood leaves only 91 tileset-16 cells. Twin is exact
on all 1,666 unshifted cells.

| Branch | Unshifted matched/emitted before → after | Shifted matched/emitted before → after |
|---|---|---|
| land | 24,685/45,754 (53.95%) → 39,820/45,754 (87.03%) | 0/28,739 → 0/28,739 |
| flood | 2,352/6,159 (38.19%) → 6,068/6,159 (98.52%) | 0/9,985 → 0/9,985 |
| twin | 734/1,666 (44.06%) → 1,666/1,666 (100%) | 0/1,700 → 0/1,700 |

Sequential probes added 15,135 matches for land, 3,716 for flood and 932 for twin:
**19,783 total**. All other branches' emitted and matched counts are identical in
both cohorts. The historical pre-elevation sanctioned sweep processed 167/167 cities with 0 failures:
emitted 1,860,329; matched was 1,589,591; unknown 132,570; mismatched was 270,738;
Its historical mismatch class sum was `16+9,829+11,560+249,333=270,738`. Stored-grid exact rendering
remains 139/167. No per-city or band offset is compensated.

`compositeGroundBlockTests` covers every base, all tiers and random masks,
constant-word footprints, exact versus subset predicates, each masked terrain bit,
mixed land classes, inclusive rectangle edges, working-image overlap refusal and
source-layer independence. Corpus assertions lock the three branch denominators,
both cohorts, all other unshifted branch rates and total mismatch accounting.
A scratch-only mutation changing the 2×2 offset from 8 to 9 exits 1 at
“composite ground tiers repeat the origin image across every covered cell for all
random masks”; the repository rule remains 8. Required tests pass: no-data release
1,066 assertions with successful SKIP, original-data release 2,287 and UBSan 2,287.

## Elevation image writers and MAP height input

The `cb81d4b` baseline was 1,589,591/1,860,329 matched, 132,570 unknown,
its mismatched total was 270,738. Its 9,829 emitted cells storing tileset 16 comprised land
8,298, road 1,278, flood 127, canal 93, plain 25, farm 6, farm fallback 1 and
shore-ring 1. In the 93 unshifted cities, tileset 16 explained 5,909 of land's
5,934 mismatches. These are earlier image writers, not a later cliff overlay.

**Height is already in the payload.** `FUN_00408CF0 @0x00408CF0` reads the byte
at `this+2+cell`; `0x473225` sets `this=0x00BEBF38`, so the address is
`0x00BEBF3A`. `FUN_0052E7C0` reads 51,984 bytes into that address at
`0x52EABF..0x52EACB`, giving logical payload offset **989,275** (the existing
`gBEBF3A` field). `0x473232` sign-extends the byte. No new array or payload
offset is needed. The previous claim that elevation caches were non-derivable
confused the caches with their MAP input.

**Descriptor trigger.** `FUN_00473210 @0x00473210` reads the eight offsets at
`0x007ACA48`: N, NE, E, SE, S, SW, W, NW. It subtracts the centre signed height
from each neighbour and refuses if none is strictly higher (`0x473288/28A`).
It scans **147 ordered rows**, count returned by `FUN_00473170`, at
`0x00825C28` (file `0x425C28`), stride 64: eight signed differences, four view
codes, flags S, direction B, terrain mask, ramp gate. Pattern 3 accepts any
difference; pattern 4 accepts absolute difference <=1 (`0x4732D4..2F6`). Ramp
rows require terrain `0x400` (`0x4732B4..2C9`). View 0 uses the first code;
codes 1/2 request height repair. `FUN_00472F50 @0x00472F50` supplies random
code alternatives, not record-index variants. Terrain `0x800000` selects paired
art through `FUN_00473180` and the mappings at `0x47335C..4734B8`.

**Writers and order.** `FUN_005403C0 @0x005403C0` completes the row/column
prepass `FUN_0053EAA0`/`FUN_0053EAE0` before its dispatcher traversal.
The prepass excludes terrain `0x8C008` (`0x53EAAB`). A nonzero descriptor other
than repair codes 1/2 and the deferred block classes calls **`FUN_00473760`**
at `0x53EBDD`: it stores `T(C)` at `0x47378E`, sets draw 0x40 and clears the
sub-tile low nibble. When B>=0 and terrain `0x800000` is set it also stores
`T(C)+1` at neighbour B (`0x4737A0..4737ED`). It then ORs S and `0x200`.
The working images occupy the grid before the land/flood block predicates.

Setter 2, **`FUN_0053F240 @0x0053F240`**, runs before the `img!=0` gate at
`0x53ECF4..53ED02`. Its deferred classes use `FUN_004736A0`'s 56-code
membership table at `0x00828118` or the two exact-image predicates
`FUN_00473800` / `FUN_00473A70`. It adjusts the origin by B and stamps a
constant image across 2x2 through `FUN_004B72B0` at `0x53F429` or `0x53F3DA`.
This may overwrite an earlier image. **`FUN_00474920 @0x00474920`** is another
writer using the same cached C/B/S and paired-neighbour rule, reached from the
road arm `FUN_004B6F90`; it handles codes `0x204E..2059` and the listed paired
codes `0x2072..20A8`. No independent later elevation pass was found in the
chain driver. The port reconstructs direct images and setter-2 blocks in this
order; it leaves the road-material path unknown (`0x00F11C70`, read by
`FUN_0044F180`, is not one of the MAP loader's flat arrays).

The **before-port probe** evaluates all 2,107,836 active cells against the fixed
9,829 target cells. Counts below are true positives / false positives; false
negatives equal 9,829 minus true positives. Stored image words only define the
probe's target; they never feed selection.

| Candidate condition | TP / 9,829 | FP / 2,098,007 |
|---|---:|---:|
| Nonzero height | 3,870 | 452,599 |
| Any strictly higher neighbour | 9,756 | 93,694 |
| Terrain `0x200` | 9,800 | 97,043 |
| A matched descriptor with code >2 | 9,732 | 93,382 |
| Descriptor >2 and baseline chain emitted | **9,732** | **23** |
| Nonzero feature | 2,019 | 366,375 |
| Nonzero `gEAC3F0` | 9,496 | 2,017,772 |
| Nonzero fertility | 6,779 | 1,784,458 |
| Nonzero `gBF8A4A` | 5,370 | 459,333 |
| `roadDistance==255` | 9,829 | 2,098,007 |
| Nonzero `gF05160` | 0 | 0 |

The 93,382 descriptor positives outside the target mostly store elevation
already but were baseline no-write/unknown cells: total stored elevation is
106,165. The port therefore improves both incorrect ground predictions and
previously uncovered elevation. Height>0, terrain `0x200`, and a byte-presence
heuristic are refuted as standalone gates. The actual ordered patterns separate
the target; stamps and paired writes explain neighbour-owned cells.

**Snapshot reconstruction boundary.** `0x53EAFA` also clears derived terrain
bits and `FUN_004740D0 @0x004740D0` repairs heights in place before the writers.
Replaying flag clearing without height repair gave 1,681,684 matches but land
39,810/39,858 and plain 81,774/81,979 in the unshifted cohort. A follow-up with
the `T(0x601)` placeholder (`0x53EC4E..53EC7D`) and the completed random-code
alternatives gave 1,693,854/1,956,703 total matches, but plain's unshifted rate
fell to 81,774/81,877 from the baseline 81,774/81,855. Both candidates were
removed. The retained port applies image writers to the loaded terrain/height
snapshot and preserves its derived terrain exclusions where no elevation image
writes. It does not claim to emulate height editing/repair. The remaining
1,395 direct and 226 block mismatches are measured, without fitted shifts. The
road-material refusal names 208 unknown cells, four of which previously carried
incorrect direct predictions; total matches are unchanged by that refusal.

| Historical elevation metric | Before elevation | After elevation, before shore |
|---|---:|---:|
| Active | 2,107,836 | 2,107,836 |
| Emitted | 1,860,329 | 1,956,299 |
| Matched | was 1,589,591 | was **1,693,753** |
| Unknown | 132,570 | was **131,696** |
| Chain no-write | was 114,937 | 19,841 |
| Mismatched | was 270,738 | was **262,546** |
| Different-tileset class | 9,829 | **64** |
| Stored-grid exact cities | 139/167 | 139/167 |

At the elevation baseline, all 167/167 cities process with 0 failures, every code resolves, and every city's
mismatch classes partition exactly. The historical pre-shore class sum was
**16 + 64 + 12,566 + 249,900 = 262,546**; this historical port added **104,162** matches.
Unshifted totals are 1,069,057/1,096,914 emitted matches with 74,967 unknown of
1,179,755 active; shifted totals are 624,696/859,385 with 56,729 unknown of
928,081 active. No existing branch's matched/emitted rate decreases in either cohort.

| Changed branch | Unshifted matched/emitted before → after | Shifted before → after |
|---|---|---|
| land | 39,820/45,754 → **39,823/39,858 (99.91%)** | 0/28,739 → 0/26,351 |
| road | 150,824/151,646 → 150,824/150,845 | 0/95,156 → 0/94,682 |
| flood | 6,068/6,159 → **6,068/6,068** | 0/9,985 → 0/9,949 |
| canal | 5,286/5,364 → **5,286/5,286** | 0/4,794 → 0/4,779 |
| plain | 81,774/81,855 → 81,774/81,839 | 118,722/118,797 → 118,722/118,790 |
| farm | 594,935/594,973 → 594,935/594,970 | 436,723/436,754 → 436,723/436,751 |
| farm fallback | 14,072/14,077 → same | 9,979/9,980 → **9,979/9,979** |
| shore-ring | 1,999/3,041 → 1,999/3,040 | 23/1,973 → same |
| elevation-direct | new **60,679/61,651** | new **41,194/41,617** |
| elevation-block | new **790/840** | new **1,496/1,672** |

The elevation archive is **not affected by the terrain archive's vintage id offset**:
both elevation branches match in the shifted as well as the unshifted cohort,
without a compensating offset.

Scoring uses an explicit tileset-identity resolver: 3 against China_Terrain,
16 against China_Elevation. The old terrain-only resolver still refuses 16.
The scorer API takes tileset 16 explicitly as `const SG3Archive* tileset16`;
it never reads the environment or loads an archive. The image-grid view passes
the archive already in its loaded tileset map, selected from `--sg3-elevation`
or `China_Elevation.sg3` next to `--sg3` under `--data-root`, including explicit
tileset overrides. Real-data tests load it from their own
`EMPEROR_GAMEDATA_ROOT`. If the pointer is null, elevation predictions count as
`unresolved`, not emitted or matched; the sweep exposes each city's count in
`chain.unresolved` and the `chainUnres` table column. Stored-grid rendering and
its supplied archives are unchanged.

`elevationGroundTests` asserts the exact height trigger, odd random alternative,
equal-height refusal, stored-image/draw/sub-tile independence, explicit archive
identity, exact elevation scoring, null-archive unresolved accounting and same-tileset
delta partition. Corpus locks
both elevation branches, all changed ground denominators and every city's class
sum. A scratch mutation reversing the strictly-higher guard exits 1 at
“the elevation prepass owns a land cell with the exact height-difference
descriptor”; the repository guard remains unchanged. Required runs: no-data
release **1,076 assertions** with successful SKIP, original-data release and
UBSan **2,298 assertions** each. The full `--data-root` image-grid sweeps with
`EMPEROR_GAMEDATA_ROOT` set and with `env -u EMPEROR_GAMEDATA_ROOT` produce
identical aggregate JSON, also identical to the lead's measurement: 167/167 cities,
0 failures, matched/emitted was 1,693,753/1,956,299 and 0 unresolved predictions.
No original asset or executable listing is added.

## Shore descriptor writer and bare-load port (2026-10-20, historical baseline)

Historical baseline `d181bfc`: **1,693,753/1,956,299** emitted cells matched;
historical unknown **131,696/2,107,836** active cells, including **94,493** shore cells.
The initial hypothesis that shore consumes the newly ported elevation descriptor
was refuted by the same-call overwrite at `0x53FCBA` (item 2). The independent
shore table was the missing rule. All facts below use the executable hash in the
document header; addresses are VAs, not redistributed executable listings.

### Shared scratch writers, readers and timing

An entire `.text` immediate-reference scan finds **8 R store sites, 48 C store
sites and 8 D store sites**. R is a dword, despite the older u8 notation. The
complete writer inventory grouped by function is:

| Writer | R (`0x0101C538`) | C (`0x0101C540`) | Meaning |
|---|---|---|---|
| `FUN_004216F0` | `0x4217FE` | — | special road variant `random&3` |
| `FUN_00473210` | `0x473326` | 34 stores in `0x47330C..0x473643` | elevation match, terrain remaps and random code alternatives |
| `FUN_004743C0`, `004744B0`, `00474730`, `00474920` | — | `0x474416`, `0x474520`, `0x4747DF`, `0x474945` | reload elevation cache |
| `FUN_004B5F70` | `0x4B61C9` | `0x4B61D8` | forest fallback `R=0,C=random&7` |
| `FUN_004BBDC0` | `0x4BBE78`, `0x4BBEE3`, `0x4BBF06` | `0x4BBE50`, `0x4BBEBF` | reverse/forward pattern match; supplied random or mutable counter |
| `FUN_004BBF20` | `0x4BC06F` | `0x4BC046` | another table matcher with a mutable counter |
| `FUN_00524FF0` | — | `0x525174`, `0x525180` | water path 1/0 scratch |
| `FUN_0053EAE0` | — | `0x53EB74` | prepass cache reload after elevation matching |
| `FUN_0053F240` | — | `0x53F25A` | setter 2 reloads this cell's cached elevation C |
| `FUN_0053F840` | `0x53F9C7` | `0x53F9D1` | setter 9 forest fallback |
| `FUN_0053FB30` | — | `0x53FBC2` | setter 14 resets C **after** its own table call |

The eight **D/view** stores are `0x406B05`, `0x406BD2`, `0x406BF3`,
`0x406C0B` (view change/normalisation), `0x534495`, `0x534DF2` (load selected
view from `0x0115F720`), `0x53D0D3` and `0x5D1587` (zero/reset). D is a view,
not a height-pattern row number. Neither setter 4 nor its helpers changes it;
`FUN_004BBDC0` reads D/2 to choose a table column at `0x4BBDC1..0x4BBDD1`.
The snapshot model consistently uses view 0, as the existing elevation port does.

Within one current-cell dispatch, setter 2 first writes C from its elevation
cache (`0x53F25A`). Setter 4 then writes `DAT_0101C560=cell`, reads shore flags,
counts them (`0x53FC97`), inverts them (`0x53FCA4`) and calls the shore matcher
(`0x53FCBA`). A successful match replaces both C and R before **any** shore
consumer: C's own-river sign test `0x53FCDB`, switch load `0x53FDE3`,
`0x61A` addition `0x53FF71..0x53FF80`, or `0x605` addition
`0x540065..0x540073`. The `0x61B` arithmetic reads D and the freshly rewritten
`0x10000` neighbour flags (`0x53FCF1`), plus the pointer-derived locals
`0x53FD0C..0x53FDD2`; it requires at least one true local. R/C are shared
scratch used by other setters too, so a name such as “the elevation descriptor”
is valid only between the relevant writer and the next overwrite.

### Historical shore result and assertions

The lead-verified `--data-root` sweep, run with `EMPEROR_GAMEDATA_ROOT` unset,
processed **167/167** cities with **0 failures**. All
**94,493/94,493** formerly unknown shore cells are now emitted, adding
**53,343** matches. The historical headline was **1,747,096/2,107,836 active cells matched**;
historical matched/active was **82.886 %** (previously 80.355 %). Stored-grid exact rendering remains **139/167**; every
stored-grid aggregate equals the baseline aggregate.

| Historical shore metric | Before | After |
|---|---:|---:|
| Active denominator | 2,107,836 | 2,107,836 |
| Emitted | 1,956,299 | **2,050,792** |
| Matched | was 1,693,753 | was **1,747,096** |
| Matched / active | was 80.355 % | was **82.886 %** |
| Matched / emitted | was 86.579 % | was **85.191 %** |
| Unknown | was 131,696 | was **37,203** |
| Mismatched | was 262,546 | was **303,696** |
| Stored zero mismatch | 16 | 16 |
| Other tileset mismatch | 64 | 64 |
| Same tileset within ±8 | 12,566 | **13,701** |
| Same tileset further off | 249,900 | **289,915** |
| Unresolved | 0 | **0** |
| Chain no-write | 19,841 | 19,841 |

The historical after-class sum was **16+64+13,701+289,915=303,696**. The emitted ratio fell
to **85.191 % (was 86.579 %)** only because 41,150 formerly unknown cells then
carried mismatching predictions: 39,577 in shifted cities with vintage offsets,
plus 1,573 in unshifted cities. No former match is lost. This denominator change
confounds matched/emitted, so matched/active is the headline.
The far-delta buckets (9–16, 17–32, 33–64, 65–128, 129–256, 257–512, 513+)
are **11,817; 266,818; 9,343; 634; 262; 1,011; 30**; the 17–32 bucket contains
**266,818/289,915 (92.033 %)** of larger deltas.

Unshifted means a city's road branch matches at least half of its emitted cells:
**93/167** cities; the other **74/167** are shifted. Both cohorts retain every
existing branch's exact emitted and matched counts, so **0 existing per-cohort
rates decrease**. Only the former unknown shore population splits into new arms:

| Branch | Unshifted emitted / matched | Shifted emitted / matched |
|---|---:|---:|
| Old unknown `0x4-shore` | 0 / 0, 54,916 unknown → 0 | 0 / 0, 39,577 unknown → 0 |
| New `0x4-shore-edge` (`0x605`) | **52,597 / 51,199** (97.342%) | **37,290 / 0** |
| New `0x4-shore-river` (`0x61A`) | **2,319 / 2,144** (92.454%) | **2,287 / 0** |
| Existing water-ring | 47,649 / 33,070 | 35,580 / 0 |
| Existing water-fringe | 33,264 / 22,292 | 23,760 / 128 |
| Existing shore-ring | 3,040 / 1,999 | 1,973 / 23 |

Total unshifted matches are **1,122,400/1,151,830** emitted, unknown
**20,051/1,179,755** active; shifted matches **624,696/898,962**, unknown
**17,152/928,081** active. No per-city archive correction or tuned residual
constant enters selection. Anyang resolves another **551/551** unknown shore
cells (all shifted mismatches), yielding **6,009/9,799** matches and **141/9,940**
unknown cells.

Assertions cover the eight directions, ordered wildcard precedence, variant
counts 1/2/4, active-object exclusion at bare load, own-river routing, unmatched
all-shore river refusal, both new cohort denominators, all existing branch
cohorts, exact totals, unresolved count and all four mismatch classes. A scratch
mutation of row `12000002`'s offset **8→9** exits **1** at “seven shore neighbours
use the shore table's north non-shore edge descriptor”; the repository offset
remains 8. The existing elevation matcher was reused by the diagnostic probe:
all **83,953/83,953** unshifted cells in the three existing shore arms have a
zero descriptor, explaining **0/26,592** residuals (item 2).

Required runs all pass: `make map-reference-test` with no root gives **1,095**
assertions and successful SKIP; with `EMPEROR_GAMEDATA_ROOT` gives **2,327**;
`make map-reference-test-undefined` with the same process-local root gives
**2,327**. `git diff --check` is clean. No assets, executable listings, derived
binaries or dumps were added to the repository.

## Remaining ground branches: bare-load rules (2026-10-20)

Historical baseline `c303acb`: **1,747,096/2,107,836** active matches, **2,050,792** emitted;
unknown was **37,203**, mismatched was **303,696**, unresolved **0**. The executable is the
same SHA-256 identified in the header. All addresses below are VAs; each static
table's file offset is VA minus `0x400000`. The existing view-0 premise applies.
No stored image word feeds selection, and the library loads no environment or
archive implicitly. All new predictions use the existing explicit terrain archive.

### 1. `0x40-road`: `FUN_00540300` → `FUN_004B6F90`

The wrapper constructs the eight flags from `terrain&0x08000040`
(`0x540311..0x54031E`) and calls `FUN_004B9B20`, which adds cardinal gate
connections from object descriptors (`0x4B9B20..0x4B9C47`). On the bare-load path
the object-id grid `0xFC3750` is zero: reset clears 51,984 i16 entries
(`0x4B08C0..0x4B08C5`); it is not one of the MAP-restored flat arrays. The
current-cell object gate is `0x4B6F9A`. `FUN_00471D20` requires terrain bit 2 and
sub-tile bit 0x80 (`0x471D24..0x471D35`); setter 6 has already consumed bit 2.
The remaining sub-tile high-bit refusal is preserved from the MAP layer and
reported as a no-write. No such refusal occurs among these corpus cells.

`FUN_00416BB0` reads **the same `0x890210` river-pointer grid** as shore
(`0x416BB4..0x416BC7`), cleared by `0x4B0999` and rebuilt only after ground
(`0x53449F` before `0x5344BD`). Its live-object arm is excluded at bare load.
Own `0x10000` takes the `0x61D` matcher arm: with working image zero,
`FUN_005B22B0` preserves the negative `img-T(0x61B)` value; that helper only
remaps 6/8/10/12 to 42/43/44/45 (`0x5B22B0..0x5B22DD`).

For ordinary road, `FUN_004BDEF0` reads signed material byte `0xF11C70` through
`FUN_0044F180` (`0x4BDF00..0x4BDF29`, `0x44F180`). Reset zeroes all **51,984/51,984**
bytes at `0x4B0919`; the MAP reader does not restore this grid. Its later rebuild
`FUN_0044F1D0` is a gameplay scheduler case (`FUN_004AC2B0`, case 0x25), not the
ground load sequence. Material zero returns type 1, selecting `0x61E` at
`0x4B7120`. The 17-row table **VA `0x84AEF0`, file `0x44AEF0`** is called at
`0x4B7113` (own monument at `0x4B71FB`). All rows have one variant, so the
negative-random counter arm always yields R=0. It covers **256/256** masks.
The port transcribes its predicates, view-0 offsets and variant counts.

`FUN_004B6E20` converts corner offsets 4..7 to `0x623` only if either of the
corner's two neighbours is **exactly** terrain `0x80` or `4`
(`0x4B6EE5..0x4B6F70`); a mask test is wrong. Own-monument road returns before
this conversion. The MAP terrain and neighbour words supply every input.

The `terrain&0x600` arm calls `FUN_00474920`. This reloads the current elevation
cache at `0x474920..0x474945`, writes only codes `0x204E..0x2059` and the even
codes `0x2072..0x20A8`, and otherwise returns without a write. Those writers are
already handled by the prepass. The **94/23,154** remaining road cells
(46 unshifted, 48 shifted) take its default, becoming determinate no-writes;
**23,060/23,154** emit. The 208 elevation ramp-material unknowns are unchanged.

### 2. `0x10000-monument`: `FUN_0053FB30`

The branch name denotes the terrain bit, not the art's archive: this setter
emits terrain `0x61B`, not a `China_Mon_*` tileset. After inverting the
`0x10000` neighbour flags it tests `FUN_00416BB0` at `0x53FB64`, so the same
proven-null river-pointer grid selects its null arm. `FUN_004B9330(4,1)` adds
shore flags, then `0x53FB98..0x53FBAA` zeroes the diagonals and calls
`FUN_004BBF20` at `0x53FBB0`. Its table is **VA `0x849818`, file `0x449818`**,
eight groups of 24 rows. The cardinal terrain/shore combinations match
**256/256** possibilities; every row has variant count one, hence R=0.

Crucially, **`0x53FBC2` clears C after the table call**. The earlier scratch
inventory's “before” wording was wrong. The write at `0x53FC0F` is therefore
`T(0x61B)` on the null path. `FUN_004BCE20` would add an object's random%6, but
the pointer check at `0x53FBE6..0x53FBEF` excludes it. All **6,215/6,215** cells
resolve. Measured unshifted matches are only **662/3,533**: the other **2,871/3,533**
stored values are exactly base+1..5 (611/628/547/558/527 cells).

**[Resolved below](#city-load-post-object-monument-redraw-forest-editor-premise-2026-10-20):**
the ordinary city load rebuilds these objects and immediately
redraws ground. Their cell field and random input are MAP-derived. The whole-map
post-object monument output now matches **3,469/3,533** unshifted cells, recovering
all 2,871 positive residuals and leaving 64 base-only exceptions in ASSR-Niya.
The per-cell null-object rule stays unchanged. Forest has a separate editor-mode
cause; its setter never reads an object pointer.
The existing plain-land predicate's mixed-monument refusal
(`0x4B7DB5..0x4B7DDC`) is retained; no new monument block writer exists here.

### 3. `0x20000-forest`: `FUN_0053F840`

The setter builds and inverts `0x20000` neighbour flags, then calls
`FUN_004BBDC0` at `0x53F87E`: **32 rows, VA `0x84B548`, file `0x44B548`**,
supplied random byte, forward mode. C=0 enters distance tiers; otherwise
`5<C+R<34` chooses `0x603` and resets C=random&7, R=0 (`0x53F9C7/0x53F9D1`).
The ordinary city path has editor flag `0x88EC38=0` (set at `0x4FB60C`);
editor entry sets it to 1 separately (`FUN_0042E430`, case 2). At C=0,
roadDistance 255 retains `0x628`; otherwise `FUN_004B5EE0(cell,100)` tests each
neighbour's MAP road-distance byte <=100, inverts it, and matches **48 rows,
VA `0x84B7C8`, file `0x44B7C8`**, selecting `0x626`. If own distance <100 and
the new C=0, threshold 0 repeats with `0x622` (`0x53F960..0x53F99D`).
`0xE92DD0` is payload 885,271, so both thresholds are MAP-derived. In this
corpus all **4,340/4,340** selected forest cells carry distance 255; threshold
arms are nevertheless asserted with authored fixtures.

The first table covers **197/256** inverted masks. One selected corpus cell
misses: STGW-Xiangjun, storage cell 38,182. A miss preserves C/R. Setter 2 has
already reloaded C=0 from this cell's elevation cache (`0x53F25A`); cell 38,181
just took the forest-to-plain fallback that explicitly sets R=0. The whole-map
pass carries only that proven immediately preceding zero, producing `0x628+0`.
Other unmatched requests without proven scratch provenance remain unknown.
All **4,340/4,340** corpus forest cells now resolve; their 2,078 matches and
2,262 residuals remain measured: **1,052/2,114** unshifted (49.76%) and
**1,026/2,226** shifted (46.09%). The plain-art fallback explains the 1,026
shifted-cohort matches without compensating any vintage offset. The final section
probes the editor's topology-based tiers and separates them from city-load state.

### 4. `0x80-farm-river`: `FUN_0053EE00`

After counting `0x10000` neighbours (`0x53EE1E..0x53EE32`), this arm calls
`FUN_004BBDC0` at `0x53EE3F` with **26 rows, VA `0x84AB90`, file `0x44AB90`**,
random=-1, forward mode. It consumes only **C+6** (`0x53EE44..0x53EE56`), so
the cycling R never enters its image. Raw neighbour flags supply the pattern;
this arm does not invert them. The table covers **167/256** masks. Its eight
selected corpus misses retain the current elevation-cache C reloaded by setter
2; the whole-map pass supplies that MAP-derived value, while the per-cell API
continues to refuse a table miss without cache context. All **3,284/3,284**
resolve, including **1,908/1,908** unshifted exact matches. Shifted matches stay
**0/1,376**, as expected for this upper-band art.

### Measurements and checks

Historical bare-load sweep, root environment unset: **167/167 cities**, **0 failures**.
Stored-grid render metrics remain identical, including **139/167** exact cities.

| Metric | Historical `c303acb` baseline | After bare-load setters, before post-object phase |
|---|---:|---:|
| Active cells | 2,107,836 | 2,107,836 |
| Emitted | 2,050,792 | 2,087,691 |
| Matched | was 1,747,096 | was 1,763,857 |
| Unknown | was 37,203 | 210 |
| Mismatched | was 303,696 | was 323,834 |
| Unresolved | 0 | 0 |
| Chain no-write | 19,841 | 19,935 |
| Stored zero mismatch | 16 | 28 |
| Other tileset mismatch | 64 | 64 |
| Same tileset within ±8 | 13,701 | 16,876 |
| Same tileset further off | 289,915 | 306,866 |

Historical bare-load mismatch classes sum to **28+64+16,876+306,866=323,834**
and partition every city independently. Absolute matches rose **16,761**;
matched/active was 83.681% (previously 82.886%). Matched/emitted was 84.488%
(previously 85.191%) because 20,138 newly emitted cells mismatched. Every existing
branch retained exactly its prior per-cohort cells, emitted and matched counts;
only `chain-declined` added the 94 road no-writes. No existing per-cohort matched
count decreased.

| Changed branch | Before unshifted/shifted unknown | After unshifted emitted / matched | After shifted emitted / matched |
|---|---:|---:|---:|
| `0x40-road` | 12,350 / 10,804 | 12,304 / 12,113 | 10,756 / 0 |
| `0x10000-monument` | 3,533 / 2,682 | 3,533 / 662 | 2,682 / 0 |
| `0x20000-forest` | 2,114 / 2,226 | 2,114 / 1,052 | 2,226 / 1,026 |
| `0x80-farm-river` | 1,908 / 1,376 | 1,908 / 1,908 | 1,376 / 0 |

Unshifted totals: **1,138,135/1,171,689** emitted matched, **146/1,179,755**
active unknown. Shifted: **625,722/916,002** emitted matched,
**64/928,081** active unknown. Anyang now emits **9,940/9,940**, matches
**6,009/9,940**, and has zero unknown; its additional 141 roads are shifted.

Refuted hypotheses: descriptors being unavailable runtime data (36,993 unknowns
removed); all C/R misses being unknowable (8 farm-river and 1 forest resolved
from cache/order); elevation-flagged roads necessarily emitting art (94/94
take the default no-write); current-cell monument table C selecting orientation
(C is cleared after matching, and all 2,871 unshifted residuals are +1..5).
No decreasing-match candidate or fitted constant was retained.

Assertions cover road endpoint/corner orientation, exact terrain equality,
own-monument precedence, sub-tile refusal, all 256 monument masks, forest
random/threshold/strict-boundary arms, both table misses and scratch retention,
farm-river C-only arithmetic, stored-image independence, exact corpus totals,
both changed branch cohorts, and all existing cohort counts. A private source
mutation of monument code **0x61B→0x61C** exits **1** at “null river-object
monument arm clears C after matching and every table row cycles R modulo one”.
The repository rule stays `0x61B`. Required tests pass: no-root release
**1,371** assertions with successful SKIP; original-data release **2,612**;
original-data UBSan **2,612**. `git diff --check` is clean.

## City-load post-object monument redraw; forest editor premise (2026-10-20)

At this historical phase, **1,766,664/2,087,691 emitted cells matched** (84.623%;
**1,766,664/2,107,836 active**, 83.814%).
Baseline `a7e0c3a` was 1,763,857/2,087,691. The gain is **2,807**: all 2,871 unshifted monument misses
are recovered, with 64 former base-only matches becoming measured residuals.
Every existing branch's per-cohort matched count is preserved or increased.
Forest remains **1,052/2,114** unshifted and **1,026/2,226** shifted; its
editor-only topology rule is documented and tested as a diagnostic, not enabled.

All executable facts use the header's owned executable SHA-256. Code VAs below
map to file offset `VA-0x400000`; no listing or original binary is included.

### Object fields, construction inputs and second-pass order

`FUN_004BCE20 @0x4BCE20` has just one data dependency beyond its object pointer:
read **i32 object+8** (`0x4BCE20`), read the **unsigned** random byte at
`0xF1E780+cell` (`0x4BCE23..25`), return its remainder modulo **6**
(`0x4BCE2D..35`). It reads no object type, orientation, image or neighbour grid.
Thus precisely random remainders 1..5 select the observed base+1..5.

The only direct construction chain found is `FUN_0041F400` → `FUN_0041F820`
(`0x41F5AF`) → `FUN_0041F350` (`0x41F850`) → `FUN_0041F230`. There is no separate
monument-pool builder behind the `0x890210` grid. Each object is 28 bytes:

| Field | Constructor meaning | Evidence VA |
|---|---|---|
| +0 | owning pool pointer | `0x41F25D` |
| +4 | initially zero; later sorted-list node | `0x41F256`, `FUN_0041F350` |
| +8 | this exact cell's flat index | **`0x41F23A`** |
| +12 / +16 | crop-local x / y | `0x41F241`, `0x41F248` |
| +20 | pool number 1 on this rebuild | `0x41F24F`, `0x41F835` |
| +24 byte | boolean own terrain bit `0x10000` | `0x41F533..536`, **`0x41F25F`** |

The object pointer is stored at `grid[cell]` at **`0x41F855`**. The sorted-list
allocation orders by this cell's random byte; it does not replace +8 or select
the image variant.

`FUN_0041F400` first clears all 51,984 pointer entries (`0x41F414` calls
`FUN_0041F3B0`). It clamps the requested rectangle, expands it by one, then scans
in increasing y/x order, repeating until a pass creates no object
(`0x41F4F3..0x41F605`; a visited byte prevents duplicate allocation). For a cell:

- Count its eight neighbours carrying `0x10000` with `FUN_004B4520` at
  `0x41F542`. Count neighbours carrying `4` but clear of `0x90000` with
  `FUN_004B4480` at `0x41F55E`.
- Require own `0x10000` or a nonzero first count. Refuse only when the two counts
  sum to 8 **and** own `(terrain&0x10004)==4` (`0x41F54E..0x41F57C`). Therefore
  **every own-0x10000 cell in the crop gets a pointer**, regardless of neighbours.
- Set terrain to `(old&0x7FF9FF7F)|0x80` (`0x41F584..0x41F591`), clearing
  `0x80000000`, `0x40000` and `0x20000`, preserving `0x10000`. Call
  `FUN_004B2CD0` at `0x41F593` for nearby farm image refresh, then allocate/store
  the object. The nearby refresh (`FUN_004B3FD0`, param5=1) changes images but
  not this pointer/terrain/random decision.

These inputs are the already parsed MAP terrain and random arrays; neither the
stored image grid nor serialised object pools supply the variant. MAP metadata
at payload 989,239 is read into grid+`0x32C40`, not into the pointer array.
**Derivability is established for the monument image after object rebuild.**

The load sequence is **not** a single null-object pass:

1. `FUN_00534410` calls `FUN_0053D630` at **`0x53449F`**.
2. It calls the full-crop object rebuild at **`0x5344BD`**, ECX=`0x890210`.
3. The rebuild itself calls `FUN_0053D630` at **`0x41F63A`** before returning.
4. That driver calls `FUN_005403C0(3)` at **`0x53D689`**. This completes the
   elevation prepass before row/column setter dispatch (`0x54040B..0x540482`).
   `FUN_0053EAA0` clears ordinary working images at `0x53EAB9`, so the prior
   image does not prevent the monument setter from running again.

For own-monument objects +24=1, **`FUN_00416BB0` still returns false**: it accepts
only nonnull pointers whose +24 is zero (`0x416BB4..0x416BC7`). Therefore the
one-variant descriptor arm still supplies R=0, C is cleared at `0x53FBC2`, but
the separate **nonnull-only** check at `0x53FBE6..0x53FBEF` now calls
`FUN_004BCE20` at `0x53FBF1`. The resulting image is exactly
**`T(0x61B)+random[cell]%6`**. No orientation chooses +1..+5.

`selectGroundMap` explicitly projects this proven monument output after its
existing snapshot pass, using the same diamond traversal. `selectGroundTileCode`
continues to represent the null-object per-cell phase. This is a partial
snapshot reconstruction, not an emulation of all terrain mutation, height repair,
or live-object shore/road behavior. The phase only updates already-selected
`0x10000-monument` variants, preserving earlier setter ownership and all origins.

### Corpus probe: monument and separate forest cause

The direct before-port probe of `base+random%6` reproduces **3,469/3,533** unshifted
monument images. The old rule matches 662; **598/662** remain exact, **64/662**
become mismatches, and **2,871/2,871** old misses become exact. All 64 exceptions
are in **ASSR-Niya.map**, all stored exactly at the base (63 cells terrain
`0x10000`, one `0x10080`). They are inside the redraw traversal. No file-name,
image, or terrain-bit exception is fitted. Shifted matches remain **0/2,682**.
These exceptions refute universal agreement of the stored monument grid with
the current executable's completed city-load state; they do not invalidate the
traced second pass. The branch's per-cohort count rises by 2,807 and 0.

Forest reads **no object pointer** in `FUN_0053F840`. All **4,340/4,340** selected
forest cells have distance 255; **0/4,340** have own/adjacent `0x10000`, so the
object builder neither allocates there nor clears their forest bit. Re-running
the city setter consequently leaves **2,078/4,340** exact. Its two output classes:

| City-load output | Unshifted emitted / matched | Shifted emitted / matched |
|---|---:|---:|
| plain `0x603` | **1,052 / 1,052** | **1,026 / 1,026** |
| outer `0x628` | **1,062 / 0** | **1,200 / 0** |

The apparent ~50% in both cohorts is this split, not a shared variant failure.
Plain-art base is low14 **46** (archive record 247); it lies below the shifted
bands. The forest bases **771 / 805 / 865** for `0x628 / 0x626 / 0x622` lie
inside the upper shifted band. They do not escape it. Stored nonplain forest
low14 ranges are **811..924** unshifted and **779..894** shifted. Relative to the
city prediction, all 1,062 unshifted residuals are positive **35..151**, and all
1,200 shifted residuals positive **3..121**, with both parities. A modulo-six
variant error cannot produce these distances.

The executable supplies a separate **editor mode** at `0x53F893..0x53F90E`,
requiring `[0x88EC38]==1`. City mode instead checks distance 255 at
`0x53F91D..0x53F929` and bypasses both tiers. Editor mode chooses `0x626` using
`FUN_004B9030(0x20000)` at `0x53F8A6`; if C remains zero, it chooses `0x622`
using `FUN_004B91B0` at `0x53F8E7`. The first helper accepts a neighbour only if
that neighbour and all eight around it carry the forest bit (`FUN_004BCFC0`);
the second requires all eight neighbours to satisfy that first erosion
(`FUN_004BD0F0`). Both invert the flags and use the existing 48-row table.
These are deterministic MAP terrain rules **under the editor premise**. Mode 1
is set by editor entry `FUN_0042E430`, case 2; the city path is mode 0.

The diagnostic reuses the existing matcher and represents the two erosion masks
as its exact 100/0 threshold inputs; it never changes production selection:

| Forest premise | Unshifted matched / all forest | Shifted matched / all forest |
|---|---:|---:|
| city-load, before or after objects | **1,052 / 2,114** | **1,026 / 2,226** |
| editor topology tiers | **2,049 / 2,114** | **1,026 / 2,226** |

The editor probe resolves 2,113/2,114 unshifted requests (the existing one-cell
scratch-state miss stays unresolved), gains **997/1,062** old residuals and loses
**0/1,052** exact images. Its remaining 64 resolved unshifted deltas are
`{-7:1,-4:37,-3:3,-1:3,+1:4,+2:1,+3:1,+9:3,+12:11}`. Shifted nonplain deltas
now concentrate at **-30 (572/1,200)** and **-32 (550/1,200)**, with
`{-34:45,-33:3,-31:5,-29:5,-28:2,-21:1,-18:17}` accounting for the other 78.
This explains why the city-load forest score appears similar across cohorts:
the exact half is plain art, while the stored forest tiers reflect a different
context and still expose the established upper-band shift. It is evidence
consistent with editor-authored stored images, not proof of their save history.
**The editor rule is not ported as the default city-load behavior.**

### Final measurements and verification

| Metric | `a7e0c3a` baseline | Post-object monument phase |
|---|---:|---:|
| Active / emitted | 2,107,836 / 2,087,691 | 2,107,836 / 2,087,691 |
| Matched | was 1,763,857 | **1,766,664** |
| Mismatched | was 323,834 | **321,027** |
| Unknown / unresolved / no-write | 210 / 0 / 19,935 | 210 / 0 / 19,935 |
| Stored zero / other tileset | 28 / 64 | 28 / 64 |
| Same tileset within ±8 / further off | 16,876 / 306,866 | **14,069 / 306,866** |
| Stored-grid exact cities | 139/167 | 139/167 |

The sanctioned sweep has **167/167 cities processed, 0 failed**; mismatch
classes sum exactly **28+64+14,069+306,866=321,027**. Summing its JSON branch
magnitude buckets gives **281,818/306,866 (91.837%)** in 17–32. Its before/after
per-cohort comparison changes only monument: unshifted **662→3,469 / 3,533 emitted**,
shifted **0→0 / 2,682**. Forest stays **1,052/2,114** and **1,026/2,226**. Every
other branch retains exactly its cells, emitted and matched counts in both
cohorts; **0 per-cohort matched counts decrease**. Unshifted total matches are
**1,140,942/1,171,689 emitted**, shifted **625,722/916,002**.

Required tests pass: no-root release **1,884** assertions with successful SKIP;
original-data release **3,130**; original-data UBSan **3,130**. Assertions cover
all 256 unsigned random bytes, phase distinction, earlier-setter ownership,
stored-image independence, both cohort totals, all branch counts, the 64
monument exceptions, forest class/delta structure and the excluded editor probe.
A private-copy mutation **`%6→%5`** exits **1** at “post-object city-load monument
redraw uses its own unsigned random byte modulo six”. No mutation touches the
working source. `git diff --check` passes; no original assets, dumps, listings or
binaries are added to the repository.

## SAV19 material roads, general ground and paired-elevation correction (2026-10-20)

This section supersedes the elevation and ground-unknown counts above. Baseline
`f3b0eab`: **154/57,684** active SAV cells unknown, **55,790** emitted/decoded,
21 no-write. After the changes: **0/57,684** unknown, **55,956** emitted/decoded,
9 no-write, 0 unresolved. All **55,956/55,956** ground draw outputs and independently
written subTile low nibbles agree with the saved arrays. Draw has **3,304/3,304**
fully assigned bytes; the other writers independently predict the marker bit
while preserving the other bits. Entirely retained bytes do not count toward
these denominators. SAV19 has no persisted image-grid oracle, so these
numbers establish decoding and metadata agreement, not image-word equality.

All facts use the executable SHA-256 in the document header. Code file offsets
are VA minus `0x400000`; static-table locations are stated explicitly below.
No stored draw/subTile/image value chooses a descriptor, origin, tier or variant.

### Paired-elevation comparisons are keyed, not numerically ordered

`FUN_00473760` is already represented correctly: origin image store
`0x47378E`, neighbour `T(C)+1` store `0x4737E0`, origin/paired draw assignments
`0x473782` / `0x4737CC`, low-nibble clears `0x473772` / `0x4737C0`.
The missing final stamp was in `FUN_00473800`, called by setter 2 at `0x53F33C`.
Its runtime table starts at **VA `0x82825C`, file `0x42825C`**, eight rows of
four dwords `(key, image0, image1, outputCode)`. Most entries are initialised
by `0x473800..0x4739EE`, so their initial file zeros are not descriptor facts.
The fifth and sixth rows are deliberately keyed **`0x209C` then `0x209A`**:

| Key | Working image at offset0 | Working image at offset1 | Output |
|---|---|---|---|
| `0x209C` | `T(0x2076)` | `T(0x209C)` | `0x205F` |
| `0x209A` | `T(0x209A)` | `T(0x2076)` | `0x205E` |

Evidence: key stores `0x473908` / `0x473944`, image stores
`0x473921` / `0x473935` / `0x47395D` / `0x473971`, output stores
`0x47393A` / `0x473976`. The old numeric-index port exchanged the two complete
comparison/output rows. Fixing the keys lets the intended 2×2 write replace the
earlier paired direct art. Setter 2 calls `FUN_004B72B0` at `0x53F3DA`:
constant image, `(terrain&0x93872790)|0x200`, sub `(old&0xE0)|1`, draw offsets
0/1/0x48/9 in view 0. The offsets come from **VA `0x81FF18`, file `0x41FF18`**.
The existing direction shifts are correct: the successive subtractions at
`0x53F385..0x53F392` compare B=0,2,6, giving NW/N/W respectively; B=4 stays put.

### Positive signed material and road corner bands

SAV19 already restores signed byte grid `0xF11C70` (terrain-relative logical
offset **T+467,856**, read `0x53137C`). `FUN_004BDEF0` calls the signed reader
`FUN_0044F180`: <=0 selects type1, 1..25 type0, 26..127 type2. Setter 13's
`FUN_004B6F90` chooses type0 base **`0x621`** at `0x4B70FC`, type2 base
**`0x61F`** at `0x4B7162`, and calls the separate **48-row** table
**VA `0x84B048`, file `0x44B048`** at `0x4B70EA` / `0x4B7150`.
It has the same 20-byte row format as the 17-row type1 table at
VA `0x84AEF0`, file `0x44AEF0`, but distinguishes diagonal connectivity.
All 48 variant counts are 1, so the negative-random cycling matcher produces R=0.

`FUN_004B6E20` handles offsets 4..7 and 22..25. If either oriented neighbour is
exactly terrain `0x80` or `4`, it changes to `T(0x623)+corner+materialBand`, where
materialBand is 4 for type0, 0 for type1, 12 for type2, plus another 4 for the
22..25 family (`0x4B6E81..0x4B6F70`). Owner and subTile refusal gates retain
their existing precedence. The original corpus contains **50 type0 + 34 type2**
cells; all **84/84** now emit/decode, reproduce draw output and independently
match marker/subTile.
Gate-object neighbour augmentation in `FUN_004B9B20` is outside this port;
the complete original SAV metadata agreement does not establish that unexercised
gate-art variant logic.

### General ground is a normal constant-image composite arm

`FUN_00540140` tests terrain `0x20`, then calls `FUN_004B8000` with required
mask `0x20`, empty working-image requirement 1 and excluded mask `0x100`:
3×3 at `0x54016C`, 2×2 at `0x5401B4`. It uses **`T(0x401)`**, not header
slot `0x403`. A 3×3 gets record offset 7 (`0x54018B`); a 2×2 gets offset 6
for random&3 0/1, 5 for 2, 4 for 3. Both call `FUN_004B72B0`, using the same
draw-offset table VA `0x81FF18`, file `0x41FF18` and constant image semantics.
On rectangle refusal, single offset is **`2*(localY&1)+(localX&1)`**
(`0x540230..0x5402E2`), with draw OR 0x40 and subTile low-nibble clear.
All **64/64** previously unknown general-layout cells now emit/decode,
reproduce draw output and independently match marker/subTile. The per-cell coordinate-free diagnostic API
retains its legacy placeholder; both whole-map entry points use these rules.

### Persisted material also closes the paired road-ramp dependency

`FUN_00473210`'s existing 147-row elevation descriptor table is **VA `0x825C28`,
file `0x425C28`**, stride 64. Ramp rows 67..70 and 103..106 require terrain
`0x400`, carry S=`0x200`, cardinal B and mask `0x400`. On own `0x800000`,
the code is remapped into paired art, then terrain `0x440` adds **8** for
material type1 or **16** for type0/type2 (`0x47358C..0x4735BA`), before
`FUN_00472F50`. The seeded SAV entry now supplies the persisted material to this
arm. MAP still lacks that restored grid and keeps its explicit unknown.
Three formerly unknown ramp cells write their paired neighbour as well; the
result makes the adjacent setter-2 membership test succeed. This resolves all
six remaining unknowns and replaces twelve no-writes, adding **18 emitted**
cells. `FUN_00474920` uses the same cache and paired rule; its default no-write
logic needs no change.

### Measurements and regression evidence

| SAV branch | Baseline draw output / subTile | Final draw output / subTile |
|---|---|---|
| elevation-direct | 2,391/2,405 / 2,391/2,405 | **2,403/2,403 / 2,403/2,403** |
| elevation-block | 184/208 / 202/208 | **228/228 / 228/228** |
| road material type0 | 0 written / 50 unknown | **50/50 / 50/50** |
| road material type2 | 0 written / 34 unknown | **34/34 / 34/34** |
| general layout | 0 written / 64 unknown | **64/64 / 64/64** |

The baseline draw-marker-only values were 2,396/2,405 direct and 193/208 block;
those differ from full draw-byte equality. Every other ground branch retains
its cell denominator and exact agreement. Final ground-only touched draw output,
independently predicted marker and subTile agreement are each **55,956/55,956**;
fully assigned ground draw bytes are **3,304/3,304**. Parallel building work changes
the bound/unbound partition separately; these ground assertions lock their
combined 1,719-cell occupancy denominator and leave binding counts to its suite.

The final sanctioned MAP sweep processes **167/167**, 0 failures:
matched **1,766,664→1,767,172 / 2,107,836 active**, emitted unchanged
2,087,691, mismatched **321,027→320,519**, unknown 210, no-write 19,935,
unresolved 0. Mismatch classes are **28+64+14,043+306,384=320,519**.
Stored-grid decoding is unchanged, with 139/167 exact cities.

| MAP branch/cohort | Baseline matched/emitted | Final matched/emitted |
|---|---:|---:|
| direct, unshifted | 60,679/61,651 | **60,693/61,599** |
| block, unshifted | 790/840 | **860/892** |
| direct, shifted | 41,194/41,617 | **41,230/41,401** |
| block, shifted | 1,496/1,672 | **1,884/1,888** |

Unshifted still means road matched >=50% emitted, giving **93/74** cities.
No per-branch/per-cohort matched count decreases; every non-elevation branch's
cells/emitted/matched tuple is identical. No fitted constant enters selection.

Regression checks cover the 209A→205E footprint, all signed material boundaries,
diagonal-sensitive road rows, both corner material bands, general 1×1/2×2/3×3
tiers and coordinate parity, and paired road ramps for all material classes.
A private source copy restoring the old 209A left-image comparison exits **1**
at “209A pairs with east 2076 and stamps 205E with independent block metadata”.
The working source is never mutated for that check. Map-reference passes release
and UBSan with **1,884 authored / 3,130 original-data** assertions; saved-ground
passes both modes with **110 authored / 1,912 original-data** checks. The
building-scene suite also passes both modes; its counts track parallel building
work separately. Unset-root runs successfully SKIP original-data work.
`git diff --check` passes. All executable probes, mutation copies and logs stay
in the external private task directory.
