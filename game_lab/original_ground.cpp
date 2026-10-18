#include "original_ground.hpp"
namespace game_lab {
namespace {
// FUN_004BC140 @0x004BC140 returns how many of the 8 neighbour flags (written by
// FUN_004B9CC0 @0x004B9CC0 / FUN_004B8F70 @0x004B8F70) have the tested mask set.
size_t neighbourMaskCount(const GroundCodeRequest& request,uint32_t mask) {
    size_t count=0;
    for(uint32_t flag:request.neighbourFlags)if(flag&mask)++count;
    return count;
}
}
// Build the exactly-8-neighbour request from the MAP grids. The executable reads
// 0x00F6A9E0 at the flat offsets N -228, NE -227, E +1, SE +229, S +228, SW +227,
// W -1, NW -229. Only the count of neighbours per mask is consumed, so the order
// is recorded for reproducibility rather than arithmetically significant.
GroundCodeRequest groundCodeRequest(const std::vector<uint32_t>& terrainFlags,
        const std::vector<uint8_t>& randomGrid,size_t cell) {
    GroundCodeRequest request;
    if(cell>=terrainFlags.size()||cell>=randomGrid.size())return request;
    request.terrainFlags=terrainFlags[cell];request.randomByte=randomGrid[cell];
    static constexpr int64_t offsets[8]={-228,-227,1,229,228,227,-1,-229};
    request.hasNeighbours=true;
    for(size_t i=0;i<8;++i) {
        const int64_t neighbour=int64_t(cell)+offsets[i];
        if(neighbour<0||size_t(neighbour)>=terrainFlags.size())continue;
        request.neighbourFlags[i]=terrainFlags[size_t(neighbour)];
    }
    return request;
}
// Extract of the per-cell ground priority chain FUN_0053EC90 @0x0053EC90 in the
// user's legally owned Emperor[CH].exe (SHA-256
// 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658, 4,820,992
// bytes, PE32 i386, imagebase 0x400000). Each branch below cites its setter
// address and literal terrain test. Values that depend on runtime arrays absent
// from the MAP file return known=false rather than a guessed code; multi-cell
// block art is not expanded to the whole block here.
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
    //    cell is left without a ground code: a determinate no-ground cell. Note
    //    0x800 is *not* part of the gate; setter 3 handles it further down.
    if(terrain&0x8C008u)return {0,false,false,"no-ground",true};
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
        // Exactly one sub-case is fixed by MAP bytes alone. With
        // !(terrain&0x10000) (so the 0x53FCEC arm cannot run), zero neighbours
        // carrying 0x10000 (0x53FCF9 FUN_004BC140), terrain&0x4000000 and
        // !(terrain&0x100) (0x53FFB2/0x53FFB9), and all 8 neighbours carrying
        // 0x4000000 (0x53FFCB FUN_004BC140 == 8), the 0x53FFDF arm writes
        // img = T(0x61C) + 0x30 + random%0x18 (0x53FFF6) and returns.
        // The single-byte water mode 0x010AF4E8 (FUN_00425250 @0x00425250) only
        // gates which arm runs; its other state converges to the same T(0x61C)
        // +0x30+random%0x18 value (0x53FF9F -> 0x53FFDF), so the result is the
        // same for both states and is fully determined by the MAP.
        if(request.hasNeighbours&&!(terrain&0x10000u)&&(terrain&0x4000000u)&&
           !(terrain&0x100u)&&neighbourMaskCount(request,0x10000u)==0&&
           neighbourMaskCount(request,0x4000000u)==8)
            return {0x61C,true,false,"0x4-shore-water-ring",false,uint8_t(0x30u+request.randomByte%0x18u)};
        return {0,false,false,"0x4-shore"};
    }
    // 5. FUN_0053F520 @0x0053F520: terrain&0x1 -> FUN_004BC6A0 (0x624/0x625 + block art).
    if(terrain&0x1u) {
        // FUN_004BC6A0 @0x004BC6A0 refuses (0x004BC6AC test 0xC064) before any
        // runtime read and FUN_0053F520 still returns 1, so no later setter runs
        // and 0x00FE9880 stays 0: a determinate no-ground cell. At branch 5 only
        // 0x20 and 0x40 of 0xC064 can still be present (0x4 is consumed by branch
        // 4, 0x4000/0x8000 by the 0x8C008 gate, 0x80000 likewise).
        if(terrain&0xC064u)return {0,false,false,"no-ground",true};
        // Otherwise the value depends on the runtime object 0x00FC3750[cell] and
        // the road-distance byte 0x00E92DD0[cell] (0x004BC6DD..0x004BC7E8), which
        // the MAP does not carry.
        return {0,false,false,"0x1-road"};
    }
    // 6. FUN_0053F660 @0x0053F660: terrain&0x2; base code follows terrain&0x300002.
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
    // 10. FUN_0053FA20 @0x0053FA20: terrain&0x2000000 while 0x00FE9880[cell]==0;
    //     the choice uses bits 8/10/20/40 of the runtime 0x00F37DA0[cell].
    if(terrain&0x2000000u)return {0,false,false,"0x2000000-delegated"};
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
        // 0x8000/0x80000 by the 0x8C008 gate), so the refusal is determinate.
        if(terrain&0xC064u)return {0,false,false,"no-ground",true};
        return {0x620,true,false,"0x40000-canal",false,uint8_t(request.randomByte%9u)};
    }
    // 16. FUN_00540140 @0x00540140: terrain&0x20 -> tileset 2 (China_General). The
    //     executable picks 0x401 + 2..7 by runtime orientation/reach; the native
    //     per-cell model uses the first ordinal and records the tileset in the code.
    if(terrain&0x20u)return {0x403,true,false,"0x20-general"};
    // 17. FUN_0053EE00 @0x0053EE00: (terrain&0x10001)==0 and terrain&0x80.
    //     Farm ground, base ordinal 0x602 (1x1) or 0x604 (4x4/3x3/2x2 block).
    //     The exact store is img=T(base)+FUN_004BD8C0(), where
    //     FUN_004BD8C0 @0x004BD8C0 = FUN_004BD940(fertility)*9 + random%9 and
    //     fertility comes from 0x00E9F8E0 via FUN_004BD920 @0x004BD920 (0 when
    //     the low 0x80 bit is clear).  A block base 0x604 is written only when
    //     FUN_004B7E80 @0x004B7E80 accepts the whole 4x4/3x3/2x2 region, which
    //     demands fertility>=100 on every cell plus no 0x8/0x40/0x800/0x10000/
    //     0x4000/... neighbour, and an alternate T(0x62A|0x62B) value needs the
    //     singleton byte 0x010AF4E8/FUN_004EDBA0 @0x004EDBA0.  The MAP file
    //     carries none of those runtime arrays: a MAP-only load has fertility 0
    //     (FUN_004BD940(0)==0) and no farm/building neighbour, so no block is
    //     accepted and the selected value is the 1x1 T(0x602)+random%9.  The
    //     neighbour-0x10000 variant (T(0x61B)+DAT_0101C540+6 via FUN_004BC140
    //     @0x004BC140) also needs the runtime river/edge cache 0x00BB92F4 and is
    //     likewise unreachable for a MAP-only request.
    if((terrain&0x10001u)==0u&&(terrain&0x80u))
        return {0x602,true,false,"0x80-farm",false,uint8_t(request.randomByte%9u)};
    // 18. FUN_0053F090 @0x0053F090: (terrain&0xAEFEDE6F)==0 -> plain land, the last link.
    if((terrain&0xAEFEDE6Fu)==0u)
        return {0x603,true,(request.randomByte&0xF8u)==0u,"plain-land",false,variant};
    // No setter wrote 0x00FE9880: every branch above either matched and named its
    // dependency or the setter returned 0 (FUN_0053F090 @0x0053F090 when
    // terrain&0xAEFEDE6F != 0). The cell is provably left without a ground code.
    return {0,false,false,"no-ground",true};
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
