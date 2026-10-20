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

## Saved-image binding

At tick0, `0x4711A0→0x46B9B0` loads **saved `+8`** at `0x46BACF..D4`, then `0x46BD43→0x413960→0x5CD8A0` draws it. Drawing neither reselects from type nor overwrites the saved frame. Secondary image is read at `0x46BA62`. Replaying those words avoids guessing which update selected them.

| Slot | Startup assignment; string VA | Archive |
|---:|---|---|
|14|`0x475CA3`; `0x82B0A0`|`DATA/SprAmbient.sg3`|
|38|`0x475D44`; `0x82AFFC`|`DATA/SprMain.sg3`|
|39|`0x475D4F`; `0x82AFF0`|`DATA/SprMain2.sg3`|
|40|`0x475D5A`; `0x82AFE4`|`DATA/SprMain3.sg3`|

These are from the41-slot startup list, independently of General. `slot=word>>14`. Loader `0x5CD1F0` trims200 records only for first bitmap `Zeus_system.bmp` (`0x5CD354..0x5CD39E`); default trim is0. All four version214 figure archives lack that prefix and start lookup1. Thus **physical ID=`(word&0x3fff)+1`**, vector index=`word&0x3fff`. Surface version213's `+201` rule is inapplicable. Binder verifies version214, first lookup1 and absence of that prefix. Every installed component is independently inverse-mapped to its exact saved primary/secondary word.

## Forward image reconstruction and oracle score

The baseline had **0/158** implemented forward matches and **0/20** implemented cited selectors. `original_figure_image.hpp` now reconstructs **115/158 visible live nonzero primary images**, with **115/115 emitted words equal to saved +8**, and cited selectors for **12/20 live types**. The remaining **43/158** partition into **26 owner-state blockers +17 external-state blockers**; selected-image disagreements, invalid inputs, missing SG groups and unsupported types are all **0/158**. Hidden nonzero figures are a separate denominator: **3/6** match, **3/6** have owner-state blockers. There are164 live nonzero saved words including those hidden6; the requested158 denominator is the visible nonzero draw population. Across all live nonzero records the score is118/164. Zero-image initialization remains **0/28 resolved**. No literal, group or branch was chosen by fitting saved image values.

`figureImage` takes `OriginalFigureImageInputs`, which deliberately cannot contain primary or secondary image words. Its inputs are the serialized runtime class,saved semantic type, action, direction, previous/attack direction, animation counter, subtype, goods/cart fields, counter/variant and the saved terrain/view; type26 additionally takes its saved owner's semantic building type from `slotTypes`. The library reads no environment or executable. Corpus code changes both saved image words to decoys and verifies every158 prediction is unchanged. The saved +8 is used only after selection, for scoring.

Generic Figure vtable `0x7AFE60+0x28→0x4C7580` dispatches through **VA `0x84E784+40*type` /file `0x44E784+40*type`**. Pheasant overrides `0x7BBDB0+0x28→0x59F310`; Emissary overrides `0x7ACD68+0x28→0x488E70`. Thus the type table's animal entry `0x59CF10` is not the loaded Pheasant object's updater. The parser retains this class identity;Emissary's override uses walking`4C17`/bounded linear death`4C18`,stores`0x489267/28C`. Addresses/file offsets below use the hash at the top of this document; subtract0x400000 for every code/data VA.

The image-store audit covers the live-type updater paths listed below,not an exhaustive claim for every possible game class. Other field writers include constructor/reset`0x4C72B0` zeroing+8 at`0x4C72C2`,Figure7 read`0x4C8465`,and Figure copy`0x4C8B10` copying the common record including+8. Those writes do not establish a postload nonzero image. Owner-delegated paths and effect writes remain explicitly bounded blockers;the native code never substitutes a constructor frame for a saved zero.

The SG resolver `0x408170` splits code into archive slot `code>>9` and group ordinal `code&511`. Loader **`0x5CD51B..0x5CD524` stores lookup physical ID−1**; the returned packed base is **`(slot<<14)+lookup[group−1]−1`**. No system trim applies to these four supplied v214 archives. Native resolution refuses a zero preceding lookup slot because the engine compacts positive lookup entries. SG header lookup is file80+2*(group−1); group metadata is file40680+72*physicalID for v214. `0x5CFDB0/0x5CFDD0` read frame/direction counts at group record+30/+32. `0x5CFDF0` masks the frame argument to u8, clamps it to frames−1, clamps direction to directions−1, then returns **base+frame*directions+direction**. Direct updaters instead explicitly use stride8, sometimes with direction8..10; those are not silently normalized. View subtraction adds8 once when negative.

| Type | Visible nonzero match | Action: matched/total | Loaded selector and image-store evidence / blocker |
|---:|---:|---|---|
|76|48/48|31:22/22;32:26/26|Virtual `0x59F310`;stores `0x59F713/76B/7B1`;action31 entry3=`4C2C`,32 entry6=`4C2B`|
|20|0/20|6:0/7;7:0/3;8:0/7;9:0/3|Table pointer `0x84EAA4→0x4D4C70`;stores `0x4D4F76/506E/513C/5185`;leader identity `0x4D4C50`,saved link+72,resource predicates and leader/current state select different cart families;withheld|
|19|18/18|6:8/8;7:5/5;8:2/2;13:3/3|`0x84EA7C→0x4CBEC0→0x4CB910`;stores `0x4CBA1E/3F`;goods/cart branch below|
|11|17/17|7:15/15;8:2/2|`0x84E93C→0x4C9FD0`;stores `0x4CA2FE/32A`;subtype+13 nonzero=`4C13`,zero=`4C15`;death next code|
|40|7/12|7:0/5;8:1/1;10:5/5;13:1/1|`0x84EDC4→0x4D2F10`;stores `0x4D3586/35A5/35D3/35F2/3613/3639/365C/367A`;action7's local attack-eligibility boolean depends on another figure and is withheld|
|26|8/8|7:4/4;8:4/4|`0x84EB94→0x4CC670`;stores `0x4CD0B9/10A/15B/1AD`;saved owner type +variant select four-code families below|
|47|0/8|6:0/8|`0x84EEDC→0x4CF380`;stores `0x4CF55F/58C/5AE`;leader's faction/model selector `0x440120`,leader action8 and saved live-state2 affect image;withheld|
|23|4/4|1:3/3;2:1/1|`0x84EB1C→0x4D0270`;stores `0x4D035D/383`;walk `4C01`,death `4C02`|
|39|4/4|8:4/4|`0x84ED9C→0x4CD230`;stores `0x4CD825/843/888/8AE/8C8`;walk `4C09`,action10 `4C0A`,11 `4C0B`,death `4C0C`|
|45|0/4|9:0/4|`0x84EE8C→0x4CE2D0`;stores `0x4CEA26/4D`;faction at+7A→`0x480740→0x481F10→0x440120` chooses `4E81/4EB4/4EB2`;withheld|
|13|3/3|9:3/3|`0x84E98C→0x4CA960`;stores `0x4CB2C9/316`;walk `4C0F`,death `4C10`|
|36|0/3|8:0/1;9:0/2|`0x84ED24→0x4D5D00`,owner+62 virtual+24 delegates;shared `0x51D0C0` reads owner's virtual+244 and special-action override;withheld|
|21|2/2|11:2/2|`0x84EACC→0x4CD8F0→0x4CB910`;same cited goods selector|
|29|2/2|8:2/2|`0x84EC0C→0x4D3B50`;stores `0x4D3F90/FB7/FDB`;walk `4C1B`,attack `4C1C`,death `4C1D`|
|30|0/2|8:0/2|`0x84EC34→0x4D5D00→owner virtual+24`;owner's virtual+244 branch unresolved|
|22|1/1|9:1/1|`0x84EAF4→0x4D13C0→0x4CB910`;same cited goods selector|
|28|1/1|2:1/1|`0x84EBE4→0x4D3A70`;stores `0x4D3B21/45`;walk `4C55`,death `4C56`|
|35|0/1|8:0/1|`0x84ECFC→0x4D5D00→owner virtual+24`;owner's virtual+244 branch unresolved|
|34|0/0|hidden:0/1|`0x84ECD4→0x4D5D00`;same owner blocker;sole occurrence hidden|
|1|0/0|image0:0/7|`0x84E7AC→0x4CD210→0x417550`;effect animation object rather than ordinary walker;zero-image load draw unresolved|
|Total|**115/158**|**43 blocked;0 selected disagreements**|**12/20 implemented cited selectors;0/28 zero initialized**|

Pheasant row selection `0x59A480→0x59CB40` walks type pointers at **VA0x85CF14/file0x45CF14**, with row1 pointer **0x85CF18→0x85CD58** (type76). Ten-code rows start **VA0x85CF38/file0x45CF38**; row1 starts **VA0x85CF60/file0x45CF60**. Its entries0/1/2/3/6/7/9 are `4C2C/4C2D/4C2B/4C2C/4C2B/4C2B/4C2B`. Action dispatch is byte table **VA0x59F824/file0x19F824** (actions4..34) into six targets at **VA0x59F80C/file0x19F80C**:4/10 use bounded counter `min(+3E,7)` added directly to T(entry1);5 entry0 with speed eligibility100;31 entry3;32 entry6;33/34 entry0;others entry9. Direction+19 falls back to+1A when≥8. For entries other than0/1, `0x59A4B8..D3` substitutes entry7 when terrain has `0x0C000100`;type70 alone bypasses it. This trace neither initializes the21 saved-zero Pheasants nor asserts action19 is drawable on load.

Goods types19/21/22 share `0x4CB910`. Resource is byte+13 if nonzero,otherwise+88 (`0x4D6A80/C0`). Special resources10/18/20 (`0x4D6A40`) and11/14/15/16/17/21/23/25/26/27 (`0x4D6A00`) use `4CA6`/death`4CA7`;ordinary uses `4E60`/death`4E64`,or `4E62-bool(+87)` when+85 is neither0,100 nor101. Previous direction and frame0 are used if direction≥8,or special goods with+89 nonzero. Ordinary primary images remain distinct from carried-goods secondary images;their generator `0x4CB6B0→0x4CB320`,amount+82 bands,cart height and offsets are traced but not implemented in this primary-image score. Forward scene mode omits secondary images,so it cannot present saved cargo as reconstructed.

Type26 owner typesED/EE/EF (`0x4C3370`) select family bases `4E79/4E7D/4E75`;other owners use `4E65+4*k`,where saved word+130 k is0/1/2,otherwise3 (`0x4CCFBA..0x4CD04A`). Four entries are walking,action8,actions12/15,death. This is a saved-owner/type input,not a constant fitted per snapshot. Type40 uses action8's **T(4EC1)+word(+3E)**,action10 `4EC2`,13/5/16 `4EC0`,action12's T(4EBE)+direction,ordinary `4EBE`,death `4EC3`. Action7's alternative `4EBF` versus static `4EC0` remains blocked. Type29 attack5 uses+1B direction and+74 frame0 below12,otherwise(+74−12)/2;death uses+3E/2. Type39 attack5 uses+1B/frame0;action11 uses+1B;death uses+3E/2. Type23/28 deaths likewise use+3E/2. Type11/13/goods deaths use a bounded linear+3E. No death/attack branch is represented by a guessed universal frame rule.

New non-image wire inputs reuse these existing Figure7 requests: subtype+13 at`0x4C8491`;attack direction+1B `0x4C84E6`;counter+3E `0x4C8595`;owner+62 `0x4C868F`;attack frame+74 `0x4C8713`;amount+82 `0x4C8771`;cart state+85/+87 `0x4C878D/7A9`;resource+88 `0x4C87B7`;stationary+89 `0x4C87C5`;variant+130 `0x4C896B`. These are runtime offsets;none is a physical wire offset assumption.

Selected installed group metadata: all use8 directions except `4EC1` (1). Frames12/speed4: `4C01,4C09,4C0F,4C13,4C15,4C1B,4C2B,4C55,4CA6,4E60,4E6D,4E6E,4E71,4E72,4EC2`;`4E66` has12/speed5;`4C2C`20/speed4;`4EC0`16/speed4;`4EC1`40/speed4. Counts and speeds are supplied SG fields,not executable constants. `0x5CFE60` resets an out-of-range counter,advances one frame when its speed argument is0 or global eligibility at`0x8901CC[100*SGspeed/speedArgument]` is true,and wraps at group length. The global eligibility is produced by`0x418AA0` from wall clock;figure updates also increment/reset counters and change state/position before selection. Consequently **nonzero tick remains refused**;there is no verified frame-step-per-tick contract from saved figure timing alone.

Saved-image playback remains the default in `installOriginalFigureOverlays` and `AppMapSurface::selectSavedBuildings`. Callers may explicitly pass `OriginalFigureRenderSource::Forward`;the returned scene/view exposes matched/compared/selected counts and disjoint mismatch reasons. Forward mode uses reconstructed primary words for115/158 figures,withholds43,omits secondary cargo,and retains hidden/zero/tick gates. Changing a nonzero saved image word does not change its forward choice. The saved-zero gate remains intentional: the load path's initialization and whether an effect/Pheasant first draws before an update are still untraced.

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

Bound saved-image types:11,13,19,20,21,22,23,26,28,29,30,35,36,39,40,45,47,76. Type34's sole occurrence is hidden. Type1's seven and21 type76 occurrences have image0. Hidden6 are type34×1,type35×1,type36×1,type39×1,type40×2. Snapshot-bound types18/20; eligible visible nonzero figures158/158; neither replaces live denominator192. No eligible corpus figure reaches an unsupported composite/position. Actions remain raw values, without invented gameplay names. Forward reconstruction is partial as scored above;zero-image initialization and nonzero time remain unimplemented.

**Caller tick0 preserves the saved image/counter/position.** Nonzero figure tick is withheld: original movement/state updates change position, group and phase together. Building animation retains its separately documented regular-draw preview premise. No wall-clock phase is claimed.

`make figure-scene-test` / `make figure-scene-test-undefined` cover wire/enums, archive classes/objects, own-version tails, FSA/truncation refusal, frame-major arithmetic, direction/progress/free movement, exact pixels/scales, component/depth order, hidden/cell-list/time gates, all bound types' saved-image invariance and forward group/clamp/decoy/source controls. Each installed original occurrence also paints a focused generic-compositor view;every reconstructible figure is additionally composed/decoded through the opt-in forward App API. Corpus access uses only process-local`EMPEROR_GAMEDATA_ROOT`; unset/empty SKIP succeeds. `scripts/figure_scene_bite.py` privately mutates physical index, signed anchor, sort order, frame stride, action byte and the cited subtype selector;6/6 must fail. Data, pixels and binaries stay outside Git.

Current Release and UBSan each pass246 authored/800 total checks, including158/158 focused original figure blits and115/115 forward primary image matches/placements. Unset/empty roots pass4/4 native SKIP runs across both modes. Existing Figure7 research regressions pass912/912 cases (456 per mode);figure mutations fail6/6. Building scene234/7,653,building image1,283/3,880,saved ground110/1,915 and map reference1,884/3,130 remain green. The App builds/signs,with93/93 source manifest entries matching and strict signature verification passing. These engineering checks do not count as original-screen fidelity.
