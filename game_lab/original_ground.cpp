#include "original_ground.hpp"
#include <cstring>
namespace game_lab {
namespace {
// FUN_004BC140 @0x004BC140 returns how many of the 8 neighbour flags (written by
// FUN_004B9CC0 @0x004B9CC0 / FUN_004B8F70 @0x004B8F70) have the tested mask set.
// FUN_004B8F70 @0x004B8F70 (0x4B8F70..0x4B9020) is a straight-line writer of that
// flag array *only*: it loads the current cell `[0x0101C560]`, ANDs each of the
// eight neighbour terrain words with the mask and normalises the result to 0/1
// into 16893368..16893396 = 0x0101C5B8..0x0101C5D4, then returns. The eight
// neighbours are the flat offsets -228,-227,+1,+229,+228,+227,-1,-229 read via
// 0x00F6A650/54, 0x00F6A9E4/DC, 0x00F6AD74/70/6C, 0x00F6A64C. It writes no
// descriptor or cache. So at 0x53FFBE (`push 0x4000000`) -> 0x53FFC3 (`call
// FUN_004B8F70`) -> 0x53FFCB (`call FUN_004BC140`) -> 0x53FFD0 (`cmp eax,8`) the
// counted value is identical to neighbourMaskCount(request,0x4000000u). The
// earlier hypothesis that the count reads descriptors FUN_004B8F70 just wrote is
// therefore fully refuted; the shore residual is not a stale-distance error.
size_t neighbourMaskCount(const GroundCodeRequest& request,uint32_t mask) {
    size_t count=0;
    for(uint32_t flag:request.neighbourFlags)if(flag&mask)++count;
    return count;
}
// FUN_004BB160 @0x004BB160(x,y,1), the setter-5 ring test. Its tier-1 table has
// 8 entries (count at 0x0081F158 == 8) with cell-index offsets read from
// 0x00A63704 (start pointer at 0x0081F1AC), i.e. the eight neighbours. For each
// in-map ring cell (FUN_004BB390 @0x004BB390 returns nonzero out of map) it
// returns 0 as soon as (terrain&3)==0; otherwise 1. FUN_004BC6A0 @0x004BC6A0
// uses that 0/1 as k, so k==1 exactly when every in-map 8-neighbour carries
// (terrain&3)!=0. An absent neighbour is skipped, never read as a zero flag.
bool roadRingComplete(const GroundCodeRequest& request) {
    for(size_t i=0;i<8;++i) {
        if(!request.neighbourPresent[i])continue;
        if((request.neighbourFlags[i]&3u)==0u)return false;
    }
    return true;
}
}
// Build the exactly-8-neighbour request from the loader's per-cell arrays
// (original_terrain.hpp, FUN_0052E7C0 @0x0052E7C0 read order). The executable
// reads 0x00F6A9E0 at the flat offsets N -228, NE -227, E +1, SE +229, S +228,
// SW +227, W -1, NW -229. Only the count of neighbours per mask is consumed, so
// the order is recorded for reproducibility rather than arithmetically significant.
GroundCodeRequest groundCodeRequest(const OriginalTerrainState& state,size_t cell) {
    GroundCodeRequest request;
    if(!state.valid()||cell>=OriginalTerrainState::cells)return request;
    request.terrainFlags=state.terrain[cell];request.randomByte=state.randomByte[cell];
    request.hasLayers=true;request.featureFlags=state.feature[cell];
    request.fertility=state.fertility[cell];request.roadDistance=state.roadDistance[cell];
    static constexpr int64_t offsets[8]={-228,-227,1,229,228,227,-1,-229};
    request.hasNeighbours=true;
    for(size_t i=0;i<8;++i) {
        const int64_t neighbour=int64_t(cell)+offsets[i];
        if(neighbour<0||size_t(neighbour)>=OriginalTerrainState::cells)continue;
        request.neighbourFlags[i]=state.terrain[size_t(neighbour)];
        request.neighbourPresent[i]=true;
    }
    return request;
}
namespace {
// FUN_004BD940 @0x004BD940, called by FUN_004BD8C0 @0x004BD8C0 for the setter-17
// farm value. It forms q = (int)((10.0/100.0)*fertility) from the constants at
// 0x007AE0E0 (10.0) and 0x007AE138 (100.0), returns 4 when q==10 (0x004BD95F
// `cmp ecx,0xa` / 0x004BD964 `dec eax`), else v=q>>1 (0x004BD95B `sar eax,1`),
// decrements v when q is even and the cell's random byte is odd (0x004BD969
// `test cl,1` / 0x004BD96E `test byte ptr [esi+0xF1E780],1`), then clamps at 0
// (0x004BD97D `setle` / `and eax,ecx`). `fertility` is the MAP byte only because
// FUN_004BD920 @0x004BD920 gates it on terrain&0x80, which setter 17 guarantees.
uint8_t farmFertilityRank(uint8_t fertility,uint8_t randomByte) {
    const uint32_t quotient=fertility/10u; // exact for the byte domain (0.1*fert)
    if(quotient==10u)return 4u;
    int32_t value=int32_t(quotient>>1);
    if((quotient&1u)==0u&&(randomByte&1u))--value;
    return uint8_t(value<0?0:value);
}
GroundSelection farmSingle(const GroundCodeRequest& request,const char* branch) {
    if(!request.hasLayers)return {0,false,false,"0x80-farm-variant"};
    return {0x602,true,false,branch,false,
        uint8_t(9u*farmFertilityRank(request.fertility,request.randomByte)+request.randomByte%9u)};
}
}
// Extract of the per-cell ground priority chain FUN_0053EC90 @0x0053EC90 in the
// user's legally owned Emperor[CH].exe (SHA-256
// 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658, 4,820,992
// bytes, PE32 i386, imagebase 0x400000). Each branch below cites its setter
// address and literal terrain test.
//
// The loader FUN_0052E7C0 @0x0052E7C0 restores the whole per-cell working set,
// including feature (0x00F37DA0) and fertility (0x00E9F8E0), from the same `.map`
// logical payload (original_terrain.hpp), so branches that read those two arrays
// are MAP-determinable and are modelled. Branches whose only missing input is a
// runtime array the read path does NOT restore (the object id 0x00FC3750, the
// river/edge descriptor 0x0101C538/0x0101C540, the caches 0x00BB92F4/0x00B866B4/
// 0x00B53A74) stay known=false and name the exact missing input rather than a
// guessed code. Multi-cell block art is not expanded to the whole block here.
GroundSelection selectGroundTileCode(const GroundCodeRequest& request) {
    const uint32_t terrain=request.terrainFlags;
    const uint8_t variant=uint8_t(request.randomByte&7u);
    // 1. FUN_004EDC30 @0x004EDC30. The setter's special mode needs this+0x14==2;
    //    in the default mode FUN_004EDA50 @0x004EDA50 refuses unless terrain&0x100
    //    is set, which is the only MAP-visible precondition. The written value
    //    (0x61C + 0x30 + random%0x18, or 0x62A/0x62B) needs runtime objects.
    if(terrain&0x100u)return {0,false,false,"0x100-special"};
    // (gate) FUN_0053EC90 @0x0053EC90 runs setters 2..18 only when setter 1
    //    (FUN_004EDC30) returned 0 *and* the terrain flags are clear of
    //    0x8C008 = 0x8 | 0x4000 | 0x8000 | 0x80000: `0x0053ECB2 test al,al; jne
    //    0x53EDF5` then `0x0053ECBA test edi,0x8c008; jne 0x53EDF5`. Reaching this
    //    line already means 0x100 is clear (the 0x100-special branch returned
    //    above), and for a 0x100-clear cell FUN_004EDC30 @0x004EDC30 returns 0 in
    //    every mode: mode 2 jumps straight to the return-0 exit `0x004EDC71`
    //    (`test ah,1; je 0x4EDC71`), while modes 0/1 can only store inside
    //    `test bl,bl; jne 0x4EDC79` where bl is the FUN_004EDA50 accept test, and
    //    that test refuses when 0x100 is clear (mode 0 additionally needs all 8
    //    neighbours to carry 0x100, so it too returns 0). Setter 1 therefore never
    //    writes 0x00FE9880 for a gate cell, the gate skips setters 2..18, and the
    //    cell keeps its loaded stored art: a determinate chain no-write. Note
    //    0x800 is *not* part of the gate; setter 3 handles it further down.
    if(terrain&0x8C008u)return {0,false,false,"chain-declined",true};
    // 2. FUN_0053F240 @0x0053F240. The guard reads per-cell river/edge descriptors
    //    0x00BB92F4[cell] and 0x00B866B4[cell] that an earlier runtime pass
    //    (FUN_0053EAE0 @0x0053EAE0) fills from the river/edge tables of
    //    FUN_00474B90/B0/C50/C90; no MAP byte carries them, so a MAP-only request
    //    cannot select this branch. It is skipped, never guessed; a request that
    //    later carries the descriptor must report known=false as "river-edge".
    // 3. FUN_00540330 @0x00540330: terrain&0x800 -> FUN_004B9E90 + FUN_00524E90(0).
    if(terrain&0x800u)return {0,false,false,"0x800-water"};
    // 4. FUN_0053FC30 @0x0053FC30: terrain&0x4 while 0x00FE9880[cell]==0 (true
    //    whenever the chain reaches here). Shore context selects 0x61B/0x61A/0x605
    //    /0x61C and adds the runtime DAT_0101C538 + DAT_0101C540.
    if(terrain&0x4u) {
        // Path to LAB_0053FF8A with the water-mode byte clear. `0x53FCC6 cmp
        // al,bl` (the byte FUN_00425250 @0x00425250 returned at 0x53FC53) `jne
        // 0x53FF8A` takes channel mode straight to the label; otherwise
        // `0x53FCCE test dword ptr [4*ebp+0x00F6A9E0],0x10000` and, for a
        // 0x10000 cell, `0x53FCDB cmp dword ptr [0x0101C540],ebx` / `0x53FCE1 jl
        // 0x53FF8A`. DAT_0101C540 (C) is 0 or a 0x20xx descriptor (never
        // negative), so a 0x10000 cell always goes to the 0x61A art instead;
        // the label is reached for a 0x10000-clear cell only when the 0x10000
        // neighbour count FUN_004BC140 @0x004BC140 is zero (`0x53FCF9 call;
        // 0x53FCFE cmp eax,ebx` / `0x53FD00 je 0x53FF8A`).
        // The water-mode byte is 0 for a bare load: the singleton constructor
        // FUN_004ED290 @0x004ED290 zeroes it (`mov byte ptr [edx+8],al`) and its
        // only writer FUN_004ED2B0 @0x004ED2B0 is reached from gameplay code
        // (single caller 0x0049CEE6), not the `.map` read path. The same premise
        // is already used by the setter-17 farm arm below.
        if(request.hasNeighbours&&!(terrain&0x10000u)&&
           neighbourMaskCount(request,0x10000u)==0) {
            // `0x53FF99 cmp byte ptr [esp+0x24],bl` / `je 0x53FFAB` selects the
            // per-cell arms for a clear water-mode byte.
            if(terrain&0x4000000u) {
                // `0x53FFB2 test eax,0x4000000` / `0x53FFB7 je 0x540017`;
                // `0x53FFB9 test ah,1` / `0x53FFBC jne 0x540017` (the 0x100
                // disjunct is dead: setter 1 owns every 0x100 cell and returned
                // above). `0x53FFBE push 0x4000000`, `0x53FFC3 call
                // FUN_004B8F70`, `0x53FFCB call FUN_004BC140`, `0x53FFD0 cmp
                // eax,8`, `0x53FFDD jne 0x53FFFC`. With all 8 neighbours
                // carrying 0x4000000 the eq8 arm writes img = T(0x61C) + 0x30 +
                // random%0x18 (`0x53FFD3 push 0x61C`, `0x53FFEE mov esi,0x18`,
                // `0x53FFF4 idiv esi`, `0x53FFF6 lea ecx,[ecx+edx+0x30]`).
                if(neighbourMaskCount(request,0x4000000u)==8)
                    return {0x61C,true,false,"0x4-shore-water-ring",false,uint8_t(0x30u+request.randomByte%0x18u)};
                // Fewer than 8: `0x53FFFC` reloads T(0x61C), `0x54000B mov
                // esi,0x30`, `0x540011 idiv esi`, `0x540013 add ecx,edx` gives
                // img = T(0x61C) + random%0x30. Measured over all 167
                // Cities/*.map this arm is 57,024 MAP-determined cells. The
                // residual (stored archive id minus the predicted id) is even
                // for 57,024/57,024 cells (100 %); because variant=random%0x30
                // that is exactly storedVariant%2 == randomByte%2, i.e.
                // storedVariant = (randomByte%48 + 2h) mod 48 for an integer h
                // that no MAP payload byte carries. Evenness is a data fact, not
                // a formula artifact: water-ring's residual is odd for 15.25 % of
                // its cells. h is a within-band authored vintage offset (a
                // per-city even base, 0/16/18/...), the same stale-data class as
                // the per-(city, band) archive id shift. No MAP-derivable rule
                // using any single payload scalar beats random%0x30: the measured
                // bound on the unshifted cohort is 22,292/33,264 (67.02 %), so
                // the arm keeps the executable's own formula instead of tuning
                // to the oracle.
                return {0x61C,true,false,"0x4-shore-water-fringe",false,uint8_t(request.randomByte%0x30u)};
            }
            // No 0x4000000 on the cell: `0x540017 cmp byte ptr [esp+0x13],bl` /
            // `0x540055 je` tests the count of neighbours carrying 0x4 that
            // `0x53FC97 call FUN_004BC140` produced (`0x53FC9F sete byte ptr
            // [esp+0x17]`) for the mask pushed at 0x53FC83/0x53FC91 (0x4 when
            // the water-mode byte is clear -- 0x104 only in channel mode).
            // Eight shore neighbours give img = T(0x61C) + random%0x18 +
            // (random&1) (`0x54001F push 0x61C`, `0x54003B mov edi,0x18`,
            // `0x540040 idiv edi`, `0x540042 add ecx,edx`, `0x540044 and
            // esi,0x80000001`, `0x540051 add ecx,esi`; the and/jns/dec/or/inc
            // sequence is the compiler's sign-fix for `(int)(r&1)` on a byte).
            if(neighbourMaskCount(request,0x4u)==8)
                return {0x61C,true,false,"0x4-shore-shore-ring",false,
                    uint8_t(request.randomByte%0x18u+(request.randomByte&1u))};
            // Otherwise `0x540055 push 0x605` writes T(0x605) + DAT_0101C538 +
            // DAT_0101C540. Both are runtime descriptors: 0x00473326 of
            // FUN_00473210 @0x00473210 clears 0x0101C538 and 0x0047330C stores
            // the table descriptor in 0x0101C540 from the 147-entry (0x93) table
            // at 0x00825C28 fed by the elevation grid; no MAP byte carries them.
            // Measured on the corpus, the stored id for these cells spreads over
            // the whole 48-piece slot-5 art range (0..47, e.g. MP41 modes 42-45,
            // 0-2, 9-19), which is authored/descriptor-driven, so the arm stays
            // unknown rather than guessing a variant.
        }
        return {0,false,false,"0x4-shore"};
    }
    // 5. FUN_0053F520 @0x0053F520: terrain&0x1 -> FUN_004BC6A0 (0x624/0x625 + block art).
    if(terrain&0x1u) {
        // FUN_004BC6A0 @0x004BC6A0 refuses (0x004BC6AC test 0xC064) before any
        // runtime read and FUN_0053F520 still returns 1, so no later setter runs
        // and 0x00FE9880 keeps its loaded value: a determinate chain no-write. At
        // branch 5 only 0x20 and 0x40 of 0xC064 can still be present (0x4 is
        // consumed by branch 4, 0x4000/0x8000 by the 0x8C008 gate, 0x80000
        // likewise).
        if(terrain&0xC064u)return {0,false,false,"chain-declined",true};
        // FUN_004BC6A0 @0x004BC6A0 road arm. Three preconditions from the MAP
        // already hold here: the 0x100 sub-arm (0x004BC6B7 `test ah,1`, then
        // FUN_004B9CC0(0x100) and 0x004BC6CC/0x004BC6D4 `cmp [0x0101C594],8`) is
        // unreachable because setter 1 owns every 0x100 cell (branch 1 returns
        // above); the 0x80000 bit is in the 0x8C008 gate; and FUN_004BD570
        // @0x004BD570 (called at 0x004BC707) is inert, its clamped fertility byte
        // overwritten by the road-distance read at 0x004BC70C. That leaves the
        // runtime object premise 0x00FC3750[cell]==0 (0x004BC6DD `cmp word ptr
        // [2*edi+0x00FC3750],0`, `jne` to the return at 0x004BC7EF), which a bare
        // load satisfies: reset (FUN_004B08A0 @0x004B08A0) copies the zero-filled
        // template 0x00E46B70, so every cell starts with object id 0. It is not a
        // MAP byte, so a request without the loader layers stays unknown rather
        // than reading the object gate as data.
        if(!request.hasLayers||!request.hasNeighbours)return {0,false,false,"0x1-road"};
        // 0x004BC70C `mov al,[edi+0x00E92DD0]`, 0x004BC715 `cmp al,-1`,
        // 0x004BC717 `jb 0x004BC7CE`. The 0xFF arm draws the 3x3 road block:
        // img = FUN_00408170(feat&2 ? 0x625 : 0x624) (0x004BC75E reads
        // 0x00F37DA0, 0x004BC765 `test al,2`, 0x004BC769 `push 0x625`; else
        // 0x004BC739 `push 0x624`) + 9*k (0x004BC7BB `lea ecx,[esi+8*esi]`) +
        // random%9 (0x004BC7AA reads 0x00F1E780, 0x004BC7B0 `mov ecx,9`,
        // 0x004BC7B5 `div`), with k from FUN_004BB160(x,y,1) (0x004BC77C `mov
        // esi,1`, call at 0x004BC784, the k=0 retry at 0x004BC79A). The non-0xFF
        // arm is the settled road tile 0x627 (0x004BC7CE `push 0x627`) +
        // random&7 (0x004BC7E3 `and edx,7`).
        if(request.roadDistance!=0xFFu)
            return {0x627,true,false,"0x1-road",false,uint8_t(request.randomByte&7u)};
        const uint16_t base=(request.featureFlags&2u)?uint16_t(0x625):uint16_t(0x624);
        return {base,true,false,"0x1-road",false,
            uint8_t(9u*roadRingComplete(request)+uint32_t(request.randomByte%9u))};
    }
    // 6. FUN_0053F660 @0x0053F660: terrain&0x2 (0x0053F660 `test byte ptr
    //    [esp+4],2`); base code follows terrain&0x300002 (0x0053F679 `and
    //    eax,0x300002`): ==0x100002 -> 0x607 (0x0053F685), ==0x200002 -> 0x608
    //    (0x0053F697), else 0x606. The store is T(base)+offset with three tiers:
    //    FUN_004B8000 @0x004B8000 accepting 3x3 -> +0xC+(random&1) as 3x3 art
    //    (0x0053F6E6), 2x2 -> +8+(random&3) as 2x2 art (0x0053F72C), else the 1x1
    //    T(base)+(random&7) (0x0053F76C). Only the 1x1 tier is emitted here: the
    //    block acceptance (FUN_004B8000, same per-cell predicate family as
    //    FUN_004B7E80) and the block art are not modelled, so `variant` is exact
    //    for the 1x1 tier and approximate (too small) where a block tier is
    //    accepted. Documented rather than silently wrong; the measured Anyang
    //    corpus shows 212/303 of these cells on the 1x1 tier.
    if(terrain&0x2u) {
        const uint32_t select=terrain&0x300002u;
        const uint16_t base=select==0x100002u?uint16_t(0x607):select==0x200002u?uint16_t(0x608):uint16_t(0x606);
        return {base,true,false,"0x2-land",false,variant};
    }
    // 7. FUN_0053F790 @0x0053F790: (terrain&0x101000)==0x101000 -> 0x60F + random&7.
    if((terrain&0x101000u)==0x101000u)return {0x60F,true,false,"0x101000-twin",false,variant};
    // 8. FUN_0053F560 @0x0053F560: terrain&0x80000000 -> 0x60E + random&7.
    if(terrain&0x80000000u)return {0x60E,true,false,"0x80000000-flood",false,variant};
    // 9. FUN_0053F840 @0x0053F840: terrain&0x20000 -> 0x628/0x626/0x622 from runtime state.
    if(terrain&0x20000u)return {0,false,false,"0x20000-forest"};
    // 10. FUN_0053FA20 @0x0053FA20: terrain&0x2000000 (0x0053FA27 `test dword
    //     ptr [ecx*4+0xF6A9E0],0x2000000`) and 0x00FE9880[cell]==0 (0x0053FA41).
    //     It then reads feat[cell] = 0x00F37DA0 (0x0053FA4B) and selects the base
    //     code by the low byte: feat&8 -> 0x60D (0x0053FA56 test / 0x0053FA79
    //     `jne 0x53FA9A`), else feat&0x10 -> 0x60A (0x0053FA7F), else feat&0x20 ->
    //     0x609 (0x0053FA8A), else feat&0x40 -> 0x619 (0x0053FA95), else 0x60D. The
    //     four literals live at esp0+0xC+4*i (0x0053FA59..0x0053FA71) and are
    //     indexed by that i at 0x0053FB00; the value is drawn as a 5x5 block
    //     through FUN_004B72B0 @0x004B72B0 (0x0053FB13) with no per-cell variant,
    //     so code is pure and block art is reported but not expanded here.
    //     `feature` is one of the per-cell arrays FUN_0052E7C0 restores from the
    //     same `.map` payload (offset 469,391 = original_terrain.hpp), so with
    //     hasLayers the branch is fully MAP-determined: measured over all 167
    //     Cities/*.map the four feat classes produce four distinct, consecutive
    //     image ids in exactly this order. The relative (constant-base) check is
    //     the only valid one here because the stored `.map` image ids use a header
    //     namespace that does not match China_Terrain.sg3.
    //     Premise: the chain reaches setter 10 only when img==0. A bare city load
    //     runs the ground pass on the reset grid (FUN_004B08A0 @0x004B08A0), and
    //     the stored image ids reproduce this chain for every modelled branch, so
    //     the precondition holds; the one feat-clear anomaly (6 cells in MP48.map,
    //     a partial 5x5 region) is a stored-snapshot artefact, not a code change.
    if(terrain&0x2000000u) {
        if(!request.hasLayers)return {0,false,false,"0x2000000-delegated"};
        const uint32_t feature=request.featureFlags&0xFFu;
        const uint16_t code=uint16_t((feature&8u)?0x60Du:(feature&0x10u)?0x60Au:
            (feature&0x20u)?0x609u:(feature&0x40u)?0x619u:0x60Du);
        return {code,true,true,"0x2000000-feature"};
    }
    // 11. FUN_00413A00 @0x00413A00 returns 0 for every input: it never handles a cell.
    // 12. FUN_00540360 @0x00540360: terrain&0x1000 -> 0x612 + random&7.
    if(terrain&0x1000u)return {0x612,true,false,"0x1000-irrigation",false,variant};
    // 13. FUN_00540300 @0x00540300: terrain&0x40 -> FUN_004B8F70/FUN_004B9B20/FUN_004B6F90.
    if(terrain&0x40u)return {0,false,false,"0x40-road"};
    // 14. FUN_0053FB30 @0x0053FB30: terrain&0x10000 and (terrain&0x48)==0; the value
    //     adds DAT_0101C538 + DAT_0101C540 (+ FUN_004BCE20()).
    if((terrain&0x10000u)&&(terrain&0x48u)==0u)return {0,false,false,"0x10000-monument"};
    // 15. FUN_005400F0 @0x005400F0: terrain&0x40000 -> FUN_004BC800.
    if(terrain&0x40000u) {
        // FUN_004BC800 @0x004BC800 writes img = T(0x620) + random%9 (0x004BC833
        // div by 9) whenever (terrain&0xC064)==0 and !(terrain&0x80000), and
        // FUN_005400F0 returns 1 either way. At branch 15 only 0x20 of 0xC064 can
        // still be set (0x4 is consumed by branch 4, 0x40 by branch 13, 0x4000/
        // 0x8000/0x80000 by the 0x8C008 gate), so the refusal is determinate and
        // the cell keeps its loaded art.
        if(terrain&0xC064u)return {0,false,false,"chain-declined",true};
        return {0x620,true,false,"0x40000-canal",false,uint8_t(request.randomByte%9u)};
    }
    // 16. FUN_00540140 @0x00540140: terrain&0x20 -> tileset 2 (China_General). The
    //     executable picks 0x401 + 2..7 by runtime orientation/reach; the native
    //     per-cell model uses the first ordinal and records the tileset in the code.
    if(terrain&0x20u)return {0x403,true,false,"0x20-general"};
    // 17. FUN_0053EE00 @0x0053EE00: (terrain&0x10001)==0 (0x0053EE07) and
    //     terrain&0x80 (0x0053EE12). Farm ground. Three sub-cases, in order:
    //  (a) 0x0053EE1E `push 0x10000; call FUN_004B8F70` then 0x0053EE2B
    //      `call FUN_004BC140` (the 8-neighbour count) selects the river arm
    //      (0x0053EE34..0x0053EE75) for any neighbour carrying 0x10000: it writes
    //      img = T(0x61B) + DAT_0101C540 + 6 (0x0053EE44 reads DAT_0101C540,
    //      0x0053EE56 `lea edi,[eax+6]`, 0x0053EE5F `push 0x61b`, store at
    //      0x0053F071). DAT_0101C540 is a runtime river/edge descriptor the `.map`
    //      does not carry, so the value is unknown and named. Without neighbour
    //      data the river arm cannot be excluded either.
    //  (b) 0x0053EE84 `test al,0xF8; je 0x53EF3E` on the cell's random byte
    //      selects the whole-plot block arm at 0x0053EF3E with b=random&6: b>=6
    //      tries FUN_004B7E80 @0x004B7E80(...,4,4) then writes T(0x604)+0x22
    //      (0x0053EF75) through the 4x4 art of FUN_004B7520; on refusal it falls
    //      through to b>=4 (3x3 -> T(0x604)+0x10+9*(random&1), 0x0053EFC9), then
    //      b>=2 (2x2 -> T(0x604)+4*(random&3)), then the 0x602 1x1 fallback at
    //      0x0053F039. FUN_004B7E80 validates every cell of the region
    //      (terrain&0x80, fertility>=100, clear of 0x8/0x40/0x4000/0x10000/
    //      0x10000000/0x87375227, no 0x10000 neighbour, plus FUN_004B0800) and the
    //      block art needs selectGroundMap below; this per-cell API leaves only
    //      context-dependent attempts unknown and resolves the b==0 fallback.
    //  (c) otherwise 0x0053EE8C `push esi; call FUN_00425260` then 0x0053EE94
    //      `call FUN_004EDBA0 @0x004EDBA0`, which returns 0 while the singleton
    //      byte [0x010AF4E8] is 0 (0x004EDBA6 `call FUN_00425250` reads this+8;
    //      0x004EDBAD `jne` guards 0x004EDBB0 `xor eax,eax`). That byte is zeroed
    //      by the singleton constructor FUN_004ED290 @0x004ED290 (`mov byte ptr
    //      [edx+8],al`) and its only writer FUN_004ED2B0 @0x004ED2B0 is reached
    //      from gameplay code (single caller 0x0049CEE6), not the `.map` read path,
    //      so a bare load leaves it 0. Premise: [0x010AF4E8]==0 at load. Then
    //      0x0053EEF6 `call FUN_004BD8C0` and 0x0053EEFF `push 0x602` write
    //      img = T(0x602) + FUN_004BD8C0(cell), where FUN_004BD8C0 @0x004BD8C0 =
    //      9*FUN_004BD940(fertility) + random%9 (0x004BD8FC `lea ecx,[eax+eax*8]`,
    //      0x004BD903 `mov esi,9` / `div`). fertility is the MAP byte 0x00E9F8E0
    //      (offset 937,255 in original_terrain.hpp) because FUN_004BD920 gates it
    //      on terrain&0x80, which this branch guarantees. Measured on the corpus,
    //      this reproduces the stored image id exactly for all 5,139 uncovered
    //      Anyang farm cells (0 counterexamples); the remaining Anyang farm cells
    //      sit inside a whole-plot block whose art overwrites them.
    //      The non-zero FUN_004EDBA0 arm (T(0x62A|0x62B)+clamp(rank,0,4)+
    //      5*(random%3)) needs the runtime byte and stays unknown.
    if((terrain&0x10001u)==0u&&(terrain&0x80u)) {
        if(request.hasNeighbours&&neighbourMaskCount(request,0x10000u)!=0)
            return {0,false,false,"0x80-farm-river"};
        if(!request.hasNeighbours)return {0,false,false,"0x80-farm-neighbours"};
        if((request.randomByte&0xF8u)==0u) {
            // b<2 attempts no rectangle (0x53EF4B/93, 0x53EFE8) and reaches
            // 0x53F03B directly, so this outcome needs no whole-map context.
            if((request.randomByte&6u)==0u)return farmSingle(request,"0x80-farm-block-fallback");
            return {0,false,false,"0x80-farm-block"};
        }
        return farmSingle(request,"0x80-farm");
    }
    // 18. FUN_0053F090 @0x0053F090: (terrain&0xAEFEDE6F)==0 -> plain land, the last
    //     link. `random&0xF8!=0` writes the 1x1 T(0x603)+(random&7); when
    //     `random&0xF8==0` the doc item-10 block tiers apply
    //     (b=random&6: 6 -> +0x2A, 4/5 -> +0x18+9*(random&1), 2/3 -> +8+4*(random&3),
    //     else +(random&7)) through FUN_004B7520 block art. The block flag is
    //     reported here; selectGroundMap supplies the accepted rectangle and
    //     consecutive per-cell variants when the whole-map context is available.
    if((terrain&0xAEFEDE6Fu)==0u)
        return {0x603,true,(request.randomByte&0xF8u)==0u,"plain-land",false,variant};
    // No setter wrote 0x00FE9880: every branch above either matched and named its
    // dependency or the setter returned 0 (FUN_0053F090 @0x0053F090 when
    // terrain&0xAEFEDE6F != 0). The cell keeps its loaded stored art; the chain
    // determinately declines to write.
    return {0,false,false,"chain-declined",true};
}
namespace {
// FUN_004B05F0 @0x004B05F0, 0x4B061A..0x4B0657: storage-space diamond
// bounds and midpoints. 0x00856C64 (file 0x456C64) supplies side and grid origin.
struct GroundMapBounds {
    int side,first,last,middle;
    explicit GroundMapBounds(uint32_t size):side(int(size)),first((228-side)/2),
        last(first+side-1),middle((last+first)/2){}
    size_t cell(int x,int y) const {return size_t(first+y)*228u+size_t(first+x);}
    // FUN_004B0770 @0x004B0770, called at 0x4B080C. Deliberately preserves
    // the executable's coordinate convention: the farm caller passes local x/y
    // (subtract origin at 0x4B7F8F), while these bounds are in storage space.
    bool inside(int x,int y) const {
        if(x>middle)return y<=middle?x-y<=last-middle:x+y<=last+1+middle;
        return y<=middle?first+middle<=x+y:middle-last<=x-y;
    }
    // FUN_004B0800 @0x004B0800: first refuses outside the diamond, then
    // tests its four edge strips (0x4B0832/3D, 0x4B0853/56,
    // 0x4B0874/77, 0x4B0884/91), with margin 3 at 0x4B7F98.
    bool edge(int x,int y,int margin) const {
        if(!inside(x,y))return false;
        if(x<=middle)return y<=middle?x-margin+y<first+middle:x-y-margin<middle-last;
        return y<=middle?last-middle<x-y+margin:last+1+middle<x+y+margin;
    }
};
// DAT_0081FF18, file 0x41FF18: six rows of six (flat offset, draw offset)
// pairs. Column stride 8 bytes at 0x4B7FCA; row stride 48 at 0x4B7FE2.
// The flat member of row r, column c is r*228+c, not a cumulative delta.
size_t blockCell(size_t origin,int row,int column) {return origin+size_t(row)*228u+size_t(column);}
uint32_t cardinalMaskCount(const OriginalTerrainState& state,size_t cell,uint32_t mask) {
    // FUN_004BA230 @0x004BA230, 0x4BA23A/248/252/25C: N,E,S,W.
    uint32_t count=0;
    for(int offset:{-228,1,228,-1}) {
        const int64_t neighbour=int64_t(cell)+offset;
        if(neighbour>=0&&neighbour<int64_t(OriginalTerrainState::cells)&&
           (state.terrain[size_t(neighbour)]&mask))++count;
    }
    return count;
}
bool diagonalMaskAny(const OriginalTerrainState& state,size_t cell,uint32_t mask) {
    // FUN_004BA1E0 @0x004BA1E0, 0x4BA1E8/1F7/206/215: NE,SE,SW,NW.
    for(int offset:{-227,229,227,-229}) {
        const int64_t neighbour=int64_t(cell)+offset;
        if(neighbour>=0&&neighbour<int64_t(OriginalTerrainState::cells)&&
           (state.terrain[size_t(neighbour)]&mask))return true;
    }
    return false;
}
}
bool acceptsFarmGroundBlock(const OriginalTerrainState& state,uint32_t activeSide,
        int x,int y,int width,int height) {
    if(!state.valid()||!activeSide||activeSide>228u||width<1||width>6||height<1||height>6)return false;
    const GroundMapBounds bounds(activeSide);
    // 0x4B7E87/98, 0x4B7EAE..B7, 0x4B7ECA..D3 (strict upper margin).
    if(x<3||y<3||x>=bounds.side||y>=bounds.side||
       x+width-1>=bounds.side-3||y+height-1>=bounds.side-3)return false;
    const size_t origin=bounds.cell(x,y);
    for(int row=0;row<height;++row)for(int column=0;column<width;++column) {
        const size_t cell=blockCell(origin,row,column);
        const uint32_t flag=state.terrain[cell];
        // 0x4B7F23, 0x4B7F27 (unsigned byte >=100), 0x4B7F30/37/3B/3F,
        // 0x4B7F44/4B/52: every literal is an independent executable refusal.
        if(!(flag&0x80u)||state.fertility[cell]<100u||
           (flag&(0x10000000u|8u|0x40u|0x4000u|0x87375227u|0x10000u|0x800u)))return false;
        if(cardinalMaskCount(state,cell,0x10000u)||diagonalMaskAny(state,cell,0x10000u))return false;
        if(bounds.edge(x+column,y+row,3))return false; // call 0x4B7FB4
    }
    return true;
}
namespace {
bool acceptsPlainGroundBlock(const OriginalTerrainState& state,const GroundMapBounds& bounds,
        int x,int y,int side) {
    // FUN_004B7CA0 @0x004B7CA0, called with (-1,1,0,1,1) at 0x53F10F,
    // 0x53F15E, 0x53F1BA. Margins 0x4B7CA7/BE/CD7/CF6; terrain mask
    // 0xBFFFDEEF at 0x4B7DE9 (param_9==1), img==param_6-1==0 at 0x4B7E30..3A.
    if(x<3||y<3||x+side-1>=bounds.side-3||y+side-1>=bounds.side-3)return false;
    const size_t origin=bounds.cell(x,y);
    bool monument=false,nonMonument=false;
    for(int row=0;row<side;++row)for(int column=0;column<side;++column) {
        const size_t cell=blockCell(origin,row,column);
        // Unlike the farm predicate, 0x4B7D81 passes storage coordinates.
        if(bounds.edge(int(cell%228u),int(cell/228u),3))return false;
        const uint32_t flag=state.terrain[cell];
        monument|=(flag&0x10000u)!=0;nonMonument|=(flag&0x10000u)==0;
        if(monument&&nonMonument)return false; // 0x4B7DB5..DC
        if(flag&0xBFFFDEEFu)return false;
        // FUN_00416BB0 @0x00416BB0 tests a live object pointer, zero on a bare
        // load. FUN_00416BD0's result is bypassed by param_8==1 (0x4B7E1C/20).
        // No ported setter populates that object-pointer array.
        if(state.image[cell]!=0u)return false;
    }
    return true;
}
}
std::vector<GroundMapCell> selectGroundMap(const OriginalTerrainState& state,uint32_t activeSide) {
    if(!state.valid()||!activeSide||activeSide>228u)return {};
    std::vector<GroundMapCell> result(OriginalTerrainState::cells);
    // Keep the per-cell selector the source for every arm outside this block port,
    // including diagnostic cells outside the engine's diamond traversal.
    for(size_t cell=0;cell<result.size();++cell)
        result[cell]={selectGroundTileCode(groundCodeRequest(state,cell)),cell,1};
    auto working=state;
    working.image.assign(OriginalTerrainState::cells,0u);
    const GroundMapBounds bounds(activeSide);
    std::vector<uint8_t> stamped(result.size(),0);
    // FUN_005403C0 @0x005403C0, 0x54044F..0x540482: increasing row, then
    // increasing column from the row tables constructed at 0x4B0694..0x4B06DC.
    for(int y=0;y<bounds.side;++y) {
        const int storageY=bounds.first+y;
        const int start=storageY<=bounds.middle?bounds.middle-y:
            bounds.first-bounds.middle-1+storageY;
        const int end=2*bounds.middle-start+2-bounds.first;
        for(int x=start-bounds.first;x<end;++x) {
            if(x<0||x>=bounds.side)continue;
            const size_t cell=bounds.cell(x,y);
            if(state.terrain[cell]&0x80000u)continue;
            // FUN_0053EC90 0x53ECF4..0x53ED02 skips setters 4..18 if img!=0.
            if(stamped[cell])continue;
            const auto request=groundCodeRequest(working,cell);
            auto selection=selectGroundTileCode(request);
            result[cell]={selection,cell,1};
            const bool farm=std::strcmp(selection.branch,"0x80-farm-block")==0;
            const bool plain=std::strcmp(selection.branch,"plain-land")==0;
            if(!farm&&!plain) {
                if(selection.known)working.image[cell]=1u;
                continue;
            }
            const unsigned tier=request.randomByte&6u; // 0x53EF46
            int side=1;
            auto accepts=[&](int size){return farm?acceptsFarmGroundBlock(working,activeSide,x,y,size,size):
                acceptsPlainGroundBlock(working,bounds,x,y,size);};
            if((request.randomByte&0xF8u)==0u) {
                if(tier>=6u&&accepts(4))side=4;
                else if(tier>=4u&&accepts(3))side=3;
                else if(tier>=2u&&accepts(2))side=2;
            }
            if(side==1) {
                // 0x53F03B calls FUN_004BD8C0 directly, without FUN_004EDBA0.
                if(farm)result[cell].selection=farmSingle(request,"0x80-farm-block-fallback");
                working.image[cell]=1u;
                if(plain)working.terrain[cell]&=0x93872790u;
                continue;
            }
            const uint8_t variant=uint8_t(farm?(side==4?0x22u:side==3?
                0x10u+(request.randomByte&1u)*9u:(request.randomByte&3u)*4u):
                (side==4?0x2Au:side==3?0x18u+(request.randomByte&1u)*9u:8u+(request.randomByte&3u)*4u));
            const char* branch=farm?(side==4?"0x80-farm-block-4x4":side==3?
                "0x80-farm-block-3x3":"0x80-farm-block-2x2"):(side==4?
                "plain-land-block-4x4":side==3?"plain-land-block-3x3":"plain-land-block-2x2");
            for(int row=0;row<side;++row)for(int column=0;column<side;++column) {
                const size_t target=blockCell(cell,row,column);
                // FUN_004B7520 0x4B7629/2B/2D: add n, increment n, store.
                // The farm predicate and stamp have no img test: later farm
                // origins overwrite. The plain predicate requires img==0.
                selection={uint16_t(farm?0x604:0x603),true,true,branch,false,uint8_t(variant+row*side+column)};
                result[target]={selection,cell,uint8_t(side)};stamped[target]=1;
                working.image[target]=1u;
                working.terrain[target]&=0x93872790u; // 0x4B75DB (param_7==0)
                working.subTile[target]&=0xF0u; // 0x4B760D
                working.draw[target]|=0x40u; // 0x4B761C
            }
        }
    }
    return result;
}
bool resolveGroundTileCode(const SG3Archive& archive,uint16_t code,size_t& archiveIndex) {
    archiveIndex=0;
    const size_t slot=size_t(code&0x1FFu);
    // Tileset 3 is China_Terrain; every other tileset stays unsupported here.
    if(!slot||slot>299u||(code>>9)!=3)return false;
    const size_t id=archive.imageIndex[slot];
    if(!id||id>archive.sprites.size())return false;
    archiveIndex=id-1;
    return true;
}
// The setter stores T(code)+variant into 0x00FE9880; T is (recordId(slot)-1) plus
// tileset*0x4000 (FUN_00408170 @0x00408170 builds on the loader table filled by
// FUN_005CD1F0 @0x005CD1F0, which writes recordId-1). The consumer masks the low
// 14 bits (FUN_005CD8A0 @0x005CD8A0) and strides an 8-bit-per-image table by that
// index (FUN_00408150 @0x00408150), so the variant is an offset in the record/id
// space, not in the header-slot space. Out-of-range sums are refused, not clamped.
bool resolveGroundTileCode(const SG3Archive& archive,uint16_t code,uint8_t variant,size_t& archiveIndex) {
    archiveIndex=0;
    size_t base=0;
    if(!resolveGroundTileCode(archive,code,base))return false;
    if(size_t(variant)>archive.sprites.size()-1-base)return false;
    archiveIndex=base+variant;
    return true;
}
}
