# SAV19 figures on the city overlay layer

Owned executable SHA-256: `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`. Addresses below are virtual addresses; file offset is VA−`0x400000`. This is an authored read-only snapshot consumer, reusing `file_io/include/city19_figure7_layout.hpp`. No original factories, movement, FSA or gameplay executes.

The six supplied SAV sources contain **12,000/12,000 parsed figure slots**, **192 live figures / 20 live semantic types**. Tick-zero rendering places and decodes **158/192 figures**, binds **18/20 live types**, and decodes **174/174 component images**, with **0 decode failures**. Six live figures are explicitly hidden; 28 have saved image zero. Counts are occurrences across snapshots, not distinct inhabitants/cities. Original-screen agreement remains unmeasured.

## Storage and saved phase

`0x52FDA0` selects SAV19 at read branch `0x5311DC`. Building list call `0x53125A→0x42D790` is immediately followed by figure list `0x531265→0x4E1E40`. The latter reads list version1, LE i32 count, then `0x4E1190→0x77FD90` reads each archive-tagged object with expected descriptor `0x84DEA8` (`Figure`). All six sources declare2,000 slots. Slot count is not activity.

The native reader starts at the existing complete building table's end, never a searched Figure name or fixed save offset. Archive PID begins at bounded goal-cache size plus4,000 building objects plus building class declarations. New declarations and class/object references have exact cache-kind checks; references into opaque preceding goal/building entries refuse. Supported classes are `Figure`, `cPheasantFig`, `cEmissaryDude`. Pheasant descriptor `0x85D1A0→0x5A0BC0→0x5A0C50`, vtable `0x7BBDB0+8→0x5A0C70`, reads own2 before Figure. Emissary descriptor `0x82BB40→0x488AB0→0x488B40`, vtable `0x7ACD68+8→0x488C20`, reads own2, Figure, then u32 length and that many string bytes. The old research Pheasant walker supplies field widths; this separate parser additionally supports the cited Emissary tail.

Figure serializer `0x4C75C0` reads version7 at `0x4C8430..0x4C8ABC`:161 field requests/395 bytes; version plus extension presence makes398 bytes. Optional presence at `0x4C8AC7`, if nonzero, calls `0x4C8ADC→0x4D72F0`/variable FSA. This reader refuses without guessing its length; all12,000 supplied records have zero presence. Unknown classes/versions and malformed/truncated data publish no partial figure list. Modern budgets:8MiB payload,8,192 list items,32,768 cache entries,8,192 Emissary string bytes. These are not original limits.

| Runtime field | Wire request / consumer evidence | Meaning retained |
|---|---|---|
| `+5` u8 | `0x4C8444` | saved animation counter, not universal physical-record delta |
| `+8` / `+0x194` u32 | `0x4C8465` / `0x4C8ABC` | primary/secondary packed image |
| `+0x10` i16 | `0x4C847B` | next figure slot in cell list |
| `+0x12` u8 | `0x4C8486`; `0x46B9FB`, `0x59A480` | semantic type |
| `+0x14` u8 | `0x4C849C`; `0x46BAE6` | free-movement position branch |
| `+0x16` u8 | `0x4C84B2`; `0x46B9C8..CD` | nonzero live gate; state1 participates in sort |
| `+0x19/+0x1A` | `0x4C84D2/DC` | signed direction/fallback; two wire bytes, one receiver byte each |
| `+0x1C/+0x1E` i16 | `0x4C84F1/FC` | active-crop x/y |
| `+0x24/+0x25` | `0x4C851D/27` | vertical raise / normalized draw class |
| `+0x28` u32 | `0x4C8532` | backing cell, checked against x/y+centered crop |
| `+0x40` u8 | `0x4C85A0` | action/state; action4 has depth score−100 |
| `+0x41` enum | `0x4C85AA` | movement progress; two wire bytes, one receiver byte |
| `+0x52/+0x54` i16 | `0x4C862C/37` | free-movement subcell x/y, modulo20 |
| `+0x6F` u8 | `0x4C86E7`; `0x471256..5B` | hidden gate before draw |
| `+0x76/+0x78` i16 | `0x4C8729/34` | secondary image extra x/y |
| `+0x85/+0x86` u8 | `0x4C878D/9B` | special draw-class override / vertical adjustment |

Every other wire width's call is in the reused source-cited layout. Existing stable reads, full framing and terrain validation precede figure parsing. Library code reads no environment.

## Saved-image binding and forward-selector boundary

At tick0, `0x4711A0→0x46B9B0` loads **saved `+8`** at `0x46BACF..D4`, then `0x46BD43→0x413960→0x5CD8A0` draws it. Drawing neither reselects from type nor overwrites the saved frame. Secondary image is read at `0x46BA62`. Replaying those words avoids guessing which update selected them.

| Slot | Startup assignment; string VA | Archive |
|---:|---|---|
|14|`0x475CA3`; `0x82B0A0`|`DATA/SprAmbient.sg3`|
|38|`0x475D44`; `0x82AFFC`|`DATA/SprMain.sg3`|
|39|`0x475D4F`; `0x82AFF0`|`DATA/SprMain2.sg3`|
|40|`0x475D5A`; `0x82AFE4`|`DATA/SprMain3.sg3`|

These are from the41-slot startup list, independently of General. `slot=word>>14`. Loader `0x5CD1F0` trims200 records only for first bitmap `Zeus_system.bmp` (`0x5CD354..0x5CD39E`); default trim is0. All four version214 figure archives lack that prefix and start lookup1. Thus **physical ID=`(word&0x3fff)+1`**, vector index=`word&0x3fff`. Surface version213's `+201` rule is inapplicable. Binder verifies version214, first lookup1 and absence of that prefix. Every installed component is independently inverse-mapped to its exact saved primary/secondary word.

Forward group helper `0x5CFDF0` uses **`T(code)+frame*directions+direction`**, frame count at SG`+0x1E`, direction count at`+0x20`. `0x5CFE60` advances counter using runtime eligibility flags. Directions are interleaved within frames. Native arithmetic has explicit synthetic truth tables, but this change does **not** claim a complete per-type/action forward selector.

One reproducible type example: `0x59A480` looks up type through pointer table`0x85CF14`, then ten-code rows at`0x85CF38`. Type76/Pheasant is row1 (`0x85CD58`), codes at`0x85CF60`; entries0/1/2/3/default9 are `0x4C2C/0x4C2D/0x4C2B/0x4C2C/0x4C2B`. Dispatch `0x59F6DB..0x59F7B1` chooses entry, direction from`+0x19/+0x1A` relative to view, and frame counter`+5`; terrain suppression can substitute entry7 at`0x59A4C5`. Emissary `0x488E70..0x48921C` uses `T(0x4C17)+8*frame+direction`, or action4's bounded `T(0x4C18)+counter`. Other type/action dispatches, update eligibility and postload initialization for zero images remain blockers for forward animation. No selector is fitted from save values or bitmap names.

## Pixel position and city scan

View0 only. `0x4711A0` reads cell-head signed-word grid`0xFAA130`, SAV19 **terrain+207,936** at`0x531338`, follows figure`+0x10`, checks cell/live/hidden and calls`0x46B9B0`. Each placed figure must be reachable in a complete acyclic saved cell list. The native reader withholds lists longer than50 (the original sort's bounded local array at`0x469580`), missing/dead links and duplicate slot identities. `0x469580→0x469690` sorts by vertical subcell score, then slot (`0x469560`). Score is tableY for state1/action≠4, otherwise−100, excluding visual height adjustments.

Ordinary cell scan: surface`0x4708B2`, mode1 objects`0x470931`, building animation`0x470A78/90`, **figures`0x470E1D→0x4711A0`**, mode2 objects`0x470EBA`. Generic depth phases are0/1/2/3/4 respectively, before the next cell. Elevated-neighbour calls`0x470BA7`/`0x470D09` exist; full elevation occlusion stays outside the existing flat scene. Figures are not appended as a final whole-city pass.

`0x46BA96..0x46BAC4` picks d=`+0x19` if<8, else`+0x1A` if<8, else0; view0 needs no subtraction. For ordinary progress p=`+0x41` in0..20, let s=p+1 for0..9, s=p−19 for10..19, s=0 for20. Signed tables`0x81EFC0/0x81F068` give:

`subX = [2,4,2,0,−2,−4,−2,0][d] * s`

`subY = [−1,0,1,2,1,0,−1,−2][d] * s − byte(+0x86)`

Free movement uses tables`0x81E8D0/0x81EA8C`, index=`(word(+0x54)%20)*21+word(+0x52)%20`: for nonnegative u/v, **subX=2*(u−v),subY=u+v−byte(+0x24)**. The implementation expresses table facts algebraically, not as original listings. Type1/9 special draw paths and type54's free-movement adjustment are withheld.

Viewport`0x469820` supplies half-width39 and half-height20 for78×40 flat cells. `0x46BC1C..0x46BC93` adds those halves, x2/y10, then subtracts the **selected sprite's** signed SG34/36 offsets. With the existing cell-centre projection `(cx,cy)`:

`left = cx + (2 + subX − image.offsetX) * scale`

`top = cy + (10 + subY − image.offsetY) * scale`

Selected logical metadata supplies offsets even when the existing decoder mirrors an earlier payload. Terrain bit0x20000 raises y baseline10 to18 when saved roadDistance0, or14 when100 (`0x46BA6F..96`); only supplied scene input is used. Secondary starts at `(cx+subX,cy+subY)`, then adds `(3,11)` for19/21/22 or `(2,10)` for20, subtracts secondary offsets and adds saved`+0x76/+0x78` (`0x46C641..0x46C6C9`). Primary draws first. Other composite/special/draw-class branches are withheld.

## Metrics, limits and verification

| Catalog SAV ordinal | Slots | Live | Placed+decoded | Hidden | Image zero | Component images decoded |
|---:|---:|---:|---:|---:|---:|---:|
|1|2,000|21|0|0|21|0|
|2|2,000|46|45|1|0|49|
|3|2,000|65|60|5|0|72|
|4|2,000|32|32|0|0|32|
|5|2,000|7|0|0|7|0|
|6|2,000|21|21|0|0|21|
|Total|**12,000**|**192**|**158**|**6**|**28**|**174**|

Bound types:11,13,19,20,21,22,23,26,28,29,30,35,36,39,40,45,47,76. Type34's sole occurrence is hidden. Type1's seven and21 type76 occurrences have image0. Hidden6 are type34×1,type35×1,type36×1,type39×1,type40×2. Snapshot-bound types18/20; eligible visible nonzero figures158/158; neither replaces live denominator192. No eligible corpus figure reaches an unsupported composite/position. Actions remain raw values, without invented gameplay names. Full forward type/action selection, zero-image initialization and nonzero time are explicitly unimplemented.

**Caller tick0 preserves the saved image/counter/position.** Nonzero figure tick is withheld: original movement/state updates change position, group and phase together. Building animation retains its separately documented regular-draw preview premise. No wall-clock phase is claimed.

`make figure-scene-test` / `make figure-scene-test-undefined` cover wire/enums, archive classes/objects, own-version tails, FSA/truncation refusal, frame-major arithmetic, direction/progress/free movement, exact pixels/scales, component/depth order, hidden/cell-list/time gates and all bound types' saved-image invariance. Each installed original occurrence also paints a focused generic-compositor view. Corpus access uses only process-local`EMPEROR_GAMEDATA_ROOT`; unset/empty SKIP succeeds. `scripts/figure_scene_bite.py` privately mutates physical index, signed anchor, sort order, frame stride and action byte;5/5 must fail. Data, pixels and binaries stay outside Git.

Current Release and UBSan each pass238 authored/614 total checks, including158/158 focused original figure blits. Unset/empty corpus roots pass4/4 native SKIP runs across those modes. Existing Figure7 research regressions pass912/912 cases (456 per mode); figure mutations fail5/5 and existing object/building-animation mutations fail6/6. The App builds/signs, with92/92 production source manifest entries matching and strict signature verification passing. These engineering checks do not count as original-screen fidelity.
