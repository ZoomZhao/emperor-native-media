#pragma once
#include "original_map.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <new>
#include <map>
namespace game_lab {
namespace original_save19 {
// Owned exe SHA256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
// Logical wire offsets relative to the independently validated terrain anchor T.
// Runtime record stride180 (0x5313F4) is NOT its serialized width: version6
// reads173 payload bytes at0x550A80..0x550E93 plus0x5512B0, and a u16
// version through0x5501C0 ->0x41FC10 ->0x41FCB0.
inline constexpr size_t routeOwners=779760; //2000 bytes,0x5313D1
inline constexpr size_t routeDirections=routeOwners+2000; //500000 bytes,0x5313E2
inline constexpr size_t records=routeDirections+500000;
inline constexpr size_t recordBytes=2+173,recordCount=100; //0x5313EC..0x531400
inline constexpr size_t recordsEnd=records+recordCount*recordBytes;
inline constexpr size_t block64=recordsEnd; //0x130F910,64 bytes,0x53140B
inline constexpr size_t savedView=block64+64; //0x115F720,u32,0x531419
inline constexpr size_t c82ef8=savedView+4; //u32,0x531427
inline constexpr size_t c82ef0=c82ef8+4; //u32,0x531435
inline constexpr size_t c82ef4=c82ef0+4; //u32,0x531443
inline constexpr size_t randomFirst=c82ef4+4; //0x10C714C,u32,0x531451
inline constexpr size_t randomSecond=randomFirst+4; //0x10C7148,u32,0x53145F
inline constexpr size_t g101D0AC=randomSecond+4; //u32,0x53146D
inline constexpr size_t g101D0B0=g101D0AC+4; //u32,0x53147B
}
// Raw runtime-field identities, not guessed service/construction semantics.
// Owned exe SHA256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658:
// FUN_00427430 serializes these at common+18 and common+(v5?100:99).
struct OriginalWellImageInputs {int16_t word16=0;uint8_t byte6f=0;};
struct OriginalStorageCompartment {uint32_t resource=0,amount=0,capacity=0;};
// Common serializer0x427430: runtime+44/+4E/+6E are common+62/+72/
// +(v5?98:97). SAV19 additionally supplies the draw-corner owner and phase.
struct OriginalBuildingAnimationInputs {
    int16_t workers=0,suppress=0;uint8_t laborDisabled=0;
    std::optional<uint8_t> phase,savedView;std::optional<uint16_t> owner;
};
struct OriginalBuildingImageInputs {
    uint32_t gridIndex=0,religiousCode=0;
    int16_t word20=0,cropWord4=0;
    uint8_t houseByte3c=0;
    std::optional<uint8_t> randomByte;
    std::optional<uint8_t> wallMask,savedView;
    std::optional<std::vector<OriginalStorageCompartment>> compartments;
    std::optional<uint8_t> marketOrientation;
    std::array<uint32_t,4> marketStalls{};
    std::vector<uint8_t> marketRandom,marketDraw;
};
struct OriginalBuildingFootprint {uint16_t type=0,x=0,y=0;uint8_t size=0,width=0,height=0;size_t slot=0,recordOffset=0;std::optional<OriginalWellImageInputs> wellImageInputs;std::optional<OriginalBuildingImageInputs> imageInputs;std::optional<OriginalBuildingAnimationInputs> animationInputs;};
// A raw common-record position whose occupied shape is not yet established.
struct OriginalBuildingMarker {uint16_t type=0,x=0,y=0;size_t slot=0;};
// Positioned object writer, deliberately separate from an occupied square.
struct OriginalBuildingObjectSource {
    uint16_t type=0,x=0,y=0;uint8_t size=0;size_t slot=0,recordOffset=0;
    uint32_t gridIndex=0,parentSlot=0;std::optional<uint8_t> savedView;
};
enum class OriginalBuildingStatus {Complete,UnsupportedTail,MissingAnchor,AmbiguousAnchor,Malformed,BudgetExceeded};
struct OriginalBuildingProfile {
    OriginalBuildingStatus status=OriginalBuildingStatus::MissingAnchor;
    std::optional<uint32_t> savedView; //SAV19 read0x531419; scene supports view0.
    size_t tableEnd=0,declaredClasses=0; // Complete list end; archive class declarations.
    size_t parsedRecords=0,namedWellRecords=0,rejectedTerrainFootprints=0,namedInspectorRecords=0,namedWatchtowerRecords=0,liveSizedRecords=0,unsupportedFootprintRecords=0,clearedClassRecords=0,classReferencesVerified=0;
    std::vector<OriginalBuildingFootprint> footprints;
    std::vector<OriginalBuildingMarker> unplacedMarkers;
    std::vector<OriginalBuildingObjectSource> objectBuildings;
    std::map<uint16_t,size_t> liveTypeCounts; // Includes live types whose placement is withheld.
    std::array<uint16_t,4000> slotTypes{}; // Semantic common-body types, never archive PIDs.
};
inline const char* originalBuildingName(uint16_t type){
    switch(type){case 0x2:case 0x3:case 0x4:case 0x5:case 0x6:case 0x7:case 0x8:case 0x9:case 0xa:case 0xb:case 0xc:case 0xd:case 0xe:case 0xf:case 0x10:case 0x11:return "House";
    case 0x3b:return "Market";case 0x3c:return "GrandMarket";case 0x21:return "Hunter";case 0x23:return "ClayPit";case 0x2b:return "Kiln";case 0x35:return "Mill";case 0x36:return "Warehouse";case 0x3a:return "TradingStation";case 0x3e:case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:return "Stall";
    case 0x7e:return "Roadblock";case 0xc0:return "SingleCropHold";case 0xc1:return "MultipleCropHold";case 0xc2:case 0xc3:case 0xc4:case 0xc5:case 0xc6:case 0xc7:return "CropField";
    case 0xcf:return "Herbalist";case 0xd0:return "Acupuncturist";case 0xd3:return "MusicSchool";case 0xd6:case 0xd7:case 0xd9:return "Shrine";case 0xe7:return "ResidentialWall";
    case 0x48:return "Well";case 0x7c:return "Inspector";case 0x7f:return "Watchtower";default:return "PublishedBuilding";}
}
inline bool namedOriginalBuilding(uint16_t type,const std::string& name){
    return (type==0x48&&name=="cWellBldg")||(type==0x7c&&name=="cInspectorBldg")||(type==0x7f&&name=="cWatchtower");
}
inline bool originalBuildingSquareType(uint16_t type);
inline uint8_t originalFootprintWidth(const OriginalBuildingFootprint& f){return f.width?f.width:f.size;}
inline uint8_t originalFootprintHeight(const OriginalBuildingFootprint& f){return f.height?f.height:f.size;}
struct OriginalBuildingImageBinding {
    const char* archive="DATA/China_General.sg3";
    uint16_t imageCode=0;uint32_t tileset=2,physicalId=0;size_t archiveIndex=0;uint16_t variant=0;
};
// Static base only. Well factory 0x51BEF0 -> ctor 0x51C090 -> vtable 0x7B5EB4;
// +0x214 is 0x51BAC0: 0x482 + bool(vcall+0x224). That predicate is 0x5B3AD0:
// signed word at runtime+0x16 > 0 OR byte at runtime+0x6F != 0.
// Type0x48/size2: table row VA0x823C58, image code at0x823C60 (file0x423C60).
inline std::optional<uint16_t> originalWellBaseImageCode(const OriginalBuildingFootprint& f){
    if(f.type!=0x48||f.size!=2||originalFootprintWidth(f)!=2||originalFootprintHeight(f)!=2||!f.wellImageInputs)return std::nullopt;
    return uint16_t(0x482+(f.wellImageInputs->word16>0||f.wellImageInputs->byte6f!=0));
}
// FUN_0051DD20 @0x51DF22..0x51DF61 calls that selector, T(code), and the
// surface writer0x4B72B0 without a variant. Startup0x475C4B puts China_General
// (string VA0x82B18C/file0x42B18C) in tileset slot2. Physical SG ids below
// refer to the on-disk table, before the original loader's system-record trim.
// Caller supplies the named archive; the library never reads the environment.
inline std::optional<OriginalBuildingImageBinding> bindOriginalWellBaseImage(const OriginalBuildingFootprint& f,
    const std::string& archivePath,const SG3Archive& archive){
    const auto code=originalWellBaseImageCode(f);if(!code||archivePath!="DATA/China_General.sg3")return std::nullopt;
    const size_t group=*code&0x1ffu;
    if((archive.version!=213&&archive.version!=214)||archive.numImages!=archive.sprites.size()||group>=archive.imageIndex.size())return std::nullopt;
    const size_t id=archive.imageIndex[group];if(!id||id>archive.numImages)return std::nullopt;
    // Public type30 2x2 base geometry is an independent format check, not a
    // semantic-name test. Other/mirrored geometries stay unsupported here.
    const auto& s=archive.sprites[id-1];
    if(s.mirrorOffset<0||s.type!=30||s.width!=158||s.height<80||s.uncompressedLength!=12800||(s.tileSize&&s.tileSize!=2))return std::nullopt;
    OriginalBuildingImageBinding out;out.imageCode=*code;out.physicalId=uint32_t(id);out.archiveIndex=id-1;return out;
}
struct OriginalBuildingBaseSelection {uint16_t code=0,variant=0;uint8_t side=0;const char* bitmap="";};
// 0x4153B0 / file0x153B0: N,E,S,W owner classes (0x415700), table
// VA0x815D40/file0x415D40, six u32 per row: mask, four views, gate variant.
// The local RNG0x41F990(1) is reseeded0x41FA80 from x/((y+8)/3).
// 0x41FAA0 is a uint32 LCG, distinct from the persisted cell-random LFSR.
inline std::optional<uint16_t> originalResidentialWallVariant(uint8_t mask,unsigned view,uint16_t x,uint16_t y){
    static constexpr uint8_t variants[16][4]={{0,2,0,2},{2,0,2,0},{0,2,0,2},{7,6,5,4},
        {2,0,2,0},{2,0,2,0},{4,7,6,5},{9,8,11,10},{0,2,0,2},{6,5,4,7},
        {0,2,0,2},{8,11,10,9},{5,4,7,6},{11,10,9,8},{10,9,8,11},{12,12,12,12}};
    if(mask>15||view>6||(view&1)||x>227||y>227)return std::nullopt;
    const uint16_t base=variants[mask][view/2];
    const uint32_t seed=uint32_t(x)/((uint32_t(y)+8)/3);
    const uint32_t next=seed*1664525u+1013904223u;
    return uint16_t(base+((base==0||base==2)?((next>>4)%2):0));
}
// Every literal below is cited by type, constructor/vtable, selector/writer and
// table VA in docs/SAV.md#exe-cited-saved-building-static-bases (same exe hash).
// Variant is added AFTER T(code), never to the packed code/group number.
inline std::optional<OriginalBuildingBaseSelection> originalBuildingBaseSelection(const OriginalBuildingFootprint& f){
    OriginalBuildingBaseSelection out;
    switch(f.type){
    case 0x2:case 0x3:case 0x5:case 0x7:{
        if(!f.imageInputs||!f.imageInputs->randomByte)return std::nullopt;
        const auto& i=*f.imageInputs;
        // 0x519F30 -> 0x519060 (empty-house branch), otherwise 0x518DE0.
        out.code=(f.type==2||(!i.houseByte3c&&!i.word20))?0x471:uint16_t(0x471+f.type-3);
        out.variant=*i.randomByte&1u;out.side=2;out.bitmap="China_Housing.bmp";break;}
    case 0x21:out={0x437,0,2,"China_Husbandry.bmp"};break; // 0x5F1EE0 -> 0x4F13F0
    case 0x23:out={0x448,0,2,"China_Industry_2.bmp"};break;
    case 0x2b:out={0x449,0,2,"China_Industry_2.bmp"};break;
    case 0x35:out={0x423,0,5,"China_StorNDist.bmp"};break; // 0x5566C0
    case 0x3e:
        if(!f.imageInputs||!f.imageInputs->randomByte)return std::nullopt;
        out={0x42a,uint16_t(*f.imageInputs->randomByte%3),2,"China_StorNDist.bmp"};break; // 0x541CAD
    case 0x42:out={0x414,0,2,"China_StorNDist.bmp"};break; // 0x5416A0 / table0x8572E8
    case 0x43:out={0x416,0,2,"China_StorNDist.bmp"};break;
    case 0x48:case 0x7c:case 0x7f:{
        if(!f.wellImageInputs)return std::nullopt;
        const uint16_t base=f.type==0x48?0x482:f.type==0x7c?0x486:0x488;
        out={uint16_t(base+(f.wellImageInputs->word16>0||f.wellImageInputs->byte6f!=0)),0,2,"China_Safety.bmp"};break;}
    case 0x7e:out={0x492,0,1,"China_Government2.bmp"};break; // 0x51DE5F..0x51DEAB; flags do not change code
    case 0xc0:out={0x4ad,0,2,"China_Husbandry.bmp"};break; // 0x4C5BF0 -> 0x4C2AC0
    case 0xc1:out={0x43a,0,3,"China_Husbandry.bmp"};break;
    case 0xc2:case 0xc4:{
        if(!f.imageInputs||!f.imageInputs->randomByte||f.imageInputs->cropWord4<0)return std::nullopt;
        const int word=f.imageInputs->cropWord4;
        // 0x4C2650 vs 0x4C2360: solo uses floor(pct/20); field adds
        // one only for positive pct. 0x4C1290 computes signed word*100/10000.
        const int pct=word*100/10000;
        const int stage=std::min(5,pct/20+(f.type==0xc4&&pct>0));
        out={uint16_t(f.type==0xc2?0x431:0x42c),uint16_t(stage+((*f.imageInputs->randomByte&1u)?6:0)),1,"China_Fields.bmp"};break;}
    case 0xcf:out={0x484,0,2,"China_Safety.bmp"};break; // 0x51BCB0
    case 0xd3:out={0x412,0,2,"China_Entertainment.bmp"};break; // zero override0x416B80 -> default row
    case 0xd6:
        if(!f.imageInputs||f.imageInputs->religiousCode!=0x481)return std::nullopt;
        out={0x481,0,2,"China_Religion.bmp"};break; // 0x5ABEF0 reads saved runtime+0x158
    case 0xe7:{
        if(!f.imageInputs||!f.imageInputs->wallMask||!f.imageInputs->savedView||*f.imageInputs->savedView!=0)return std::nullopt;
        const auto variant=originalResidentialWallVariant(*f.imageInputs->wallMask,*f.imageInputs->savedView,f.x,f.y);
        if(!variant)return std::nullopt;
        out={0x4b5,*variant,1,"China_Aesthetic.bmp"};break;}
    default:return std::nullopt; // Multipart types use the component binder below.
    }
    if(f.size!=out.side||originalFootprintWidth(f)!=out.side||originalFootprintHeight(f)!=out.side)return std::nullopt;
    return out;
}
inline std::optional<OriginalBuildingImageBinding> bindOriginalBuildingBaseImage(const OriginalBuildingFootprint& f,
    const std::string& archivePath,const SG3Archive& archive){
    if(f.type==0x48)return bindOriginalWellBaseImage(f,archivePath,archive);
    const auto selected=originalBuildingBaseSelection(f);if(!selected)return std::nullopt;
    // All closed selectors use slot2 from the 41-slot startup list0x475C3B..0x475D5A.
    if(selected->code>>9!=2||archivePath!="DATA/China_General.sg3")return std::nullopt;
    const size_t group=selected->code&0x1ffu;
    if((archive.version!=213&&archive.version!=214)||archive.numImages!=archive.sprites.size()||group>=archive.imageIndex.size())return std::nullopt;
    const size_t first=archive.imageIndex[group],id=first+selected->variant;
    if(!first||id>archive.numImages)return std::nullopt;
    const auto& s=archive.sprites[id-1];const int side=selected->side;
    if(s.mirrorOffset<0||s.type!=30||s.width!=80*side-2||s.height<40*side||s.uncompressedLength!=uint32_t(3200*side*side)||(s.tileSize&&s.tileSize!=side))return std::nullopt;
    if(s.bitmapId>=archive.bitmaps.size()||archive.bitmaps[s.bitmapId].filename!=selected->bitmap)return std::nullopt;
    // The selected variant must retain the base's independent bitmap family.
    if(archive.sprites[first-1].bitmapId!=s.bitmapId)return std::nullopt;
    OriginalBuildingImageBinding out;out.imageCode=selected->code;out.variant=selected->variant;out.physicalId=uint32_t(id);out.archiveIndex=id-1;return out;
}
struct OriginalBuildingComponent {OriginalBuildingImageBinding binding;uint8_t x=0,y=0,side=0;std::optional<uint8_t> preservedDraw=std::nullopt;};
// Storage0x5D6A80/0x5E1580 first stamp T(424/425) over size^2, then
// 0x5D5FF0 overwrites compartment k at (k+1)%size,(k+1)/size. +2A4
// 0x5D6B50 supplies occupied454/empty456; +2D8=0x546C40 selects data.
// 0x4B04F0 considers a compartment empty iff resource AND amount are zero.
inline std::optional<std::vector<OriginalBuildingComponent>> bindOriginalBuildingComponents(
    const OriginalBuildingFootprint& f,const std::string& archivePath,const SG3Archive& archive,
    const OriginalBuildingProfile* profile=nullptr){
    if(f.type==0x3b){
        if(!profile||!f.imageInputs||!f.imageInputs->marketOrientation||*f.imageInputs->marketOrientation>1||
            f.imageInputs->marketRandom.size()!=28||f.imageInputs->marketDraw.size()!=28||!f.imageInputs->savedView||*f.imageInputs->savedView!=0)return std::nullopt;
        const bool rotated=*f.imageInputs->marketOrientation==1;
        if(f.size!=1||originalFootprintWidth(f)!=(rotated?7:4)||originalFootprintHeight(f)!=(rotated?4:7))return std::nullopt;
        std::vector<OriginalBuildingComponent> out;
        // Descriptor VA0x8574A8/file0x4574A8: two orientations of 28
        // (x,y,role,arg) rows. Active role2 origins: (0,0),(2,0),(0,5),(2,5)
        // transposed for orientation1. Saved child links: market tail+218.
        for(unsigned stall=0;stall<4;++stall){
            const uint8_t x=uint8_t(rotated?(stall/2)*5:(stall%2)*2),y=uint8_t(rotated?(stall%2)*2:(stall/2)*5);
            const auto slot=f.imageInputs->marketStalls[stall];
            auto child=std::find_if(profile->footprints.begin(),profile->footprints.end(),[&](const auto& c){return c.slot==slot;});
            if(child==profile->footprints.end()||child->slot<=f.slot||child->x!=f.x+x||child->y!=f.y+y||child->size!=2||
                (child->type!=0x3e&&child->type!=0x42&&child->type!=0x43))return std::nullopt;
            auto b=bindOriginalBuildingBaseImage(*child,archivePath,archive);if(!b)return std::nullopt;
            out.push_back({*b,x,y,2});
        }
        // Roles1/4 have arg0 in both common-market descriptor lists:
        // direct image=T(422)+(R&3), subTile&=E0, draw|=40. Its low draw
        // bits are serialized input, not an independently predicted output.
        for(unsigned y=0;y<7;++y)for(unsigned x=0;x<4;++x)if(y>=2&&y<=4){
            const auto random=f.imageInputs->marketRandom[y*4+x];
            const uint16_t variant=random&3;const size_t first=archive.imageIndex[0x22],id=first+variant;
            if(archivePath!="DATA/China_General.sg3"||(archive.version!=213&&archive.version!=214)||archive.numImages!=archive.sprites.size()||!first||id>archive.sprites.size())return std::nullopt;
            const auto& s=archive.sprites[id-1];
            if(s.mirrorOffset||s.type!=30||s.width!=78||s.height<40||s.uncompressedLength!=3200||(s.tileSize&&s.tileSize!=1)||s.bitmapId>=archive.bitmaps.size()||archive.bitmaps[s.bitmapId].filename!="China_StorNDist.bmp")return std::nullopt;
            OriginalBuildingImageBinding b;b.imageCode=0x422;b.variant=variant;b.physicalId=uint32_t(id);b.archiveIndex=id-1;
            out.push_back({b,uint8_t(rotated?y:x),uint8_t(rotated?x:y),1,uint8_t(f.imageInputs->marketDraw[y*4+x]|0x40)});
        }
        return out;
    }
    if(f.type!=0x36&&f.type!=0x3a){
        const auto binding=bindOriginalBuildingBaseImage(f,archivePath,archive);if(!binding)return std::nullopt;
        return std::vector<OriginalBuildingComponent>{{*binding,0,0,f.size}};
    }
    const uint8_t side=f.type==0x36?3:4;
    if(f.size!=side||originalFootprintWidth(f)!=side||originalFootprintHeight(f)!=side||!f.imageInputs||
        !f.imageInputs->compartments||f.imageInputs->compartments->size()!=size_t(side)*side-1||
        archivePath!="DATA/China_General.sg3"||(archive.version!=213&&archive.version!=214)||archive.numImages!=archive.sprites.size())return std::nullopt;
    std::vector<OriginalBuildingComponent> out;
    for(unsigned cell=0;cell<unsigned(side)*side;++cell){
        uint16_t code=f.type==0x36?0x424:0x425,variant=0;const char* bitmap="China_StorNDist.bmp";
        if(cell){const auto& c=f.imageInputs->compartments->at(cell-1);bitmap="China_Stor-bays.bmp";
            if(c.capacity!=400||c.amount>400||c.resource>28||(!c.resource&&c.amount))return std::nullopt;
            if(!c.resource&&!c.amount)code=0x456;
            else {code=0x454;variant=uint16_t(4*(c.resource-1)+(c.amount==400?3:c.amount>299?2:c.amount>=200?1:0));}
        }
        const size_t first=archive.imageIndex[code&511],id=first+variant;
        if(!first||id>archive.sprites.size())return std::nullopt;
        const auto& s=archive.sprites[id-1];
        if(s.mirrorOffset||s.type!=30||s.width!=78||s.height<40||s.uncompressedLength!=3200||(s.tileSize&&s.tileSize!=1)||
            s.bitmapId>=archive.bitmaps.size()||archive.bitmaps[s.bitmapId].filename!=bitmap||archive.sprites[first-1].bitmapId!=s.bitmapId)return std::nullopt;
        OriginalBuildingImageBinding b;b.imageCode=code;b.variant=variant;b.physicalId=uint32_t(id);b.archiveIndex=id-1;
        out.push_back({b,uint8_t(cell%side),uint8_t(cell/side),1});
    }
    return out;
}
inline bool validOriginalBuildingFootprint(const OriginalMapTerrain& terrain,const OriginalBuildingFootprint& f){
    const auto w=originalFootprintWidth(f),h=originalFootprintHeight(f);const bool market=f.type==0x3b||f.type==0x3c;
    if(!terrain.valid()||(!originalBuildingSquareType(f.type)&&!market)||!f.size||f.size>8||!w||!h||w>8||h>8||uint32_t(f.x)+w>terrain.activeSide||uint32_t(f.y)+h>terrain.activeSide)return false;
    if(market){const uint8_t side=f.type==0x3b?4:6;if(!((w==7&&h==side)||(w==side&&h==7)))return false;}
    else if(w!=f.size||h!=f.size)return false;
    const size_t origin=(228-terrain.activeSide)/2;
    for(size_t y=0;y<h;++y)for(size_t x=0;x<w;++x){const auto v=terrain.flags[(origin+f.y+y)*228+origin+f.x+x];if((v&0x80000u)||!(v&8u))return false;}
    return true;
}
// Span facts from the pinned public mapper: emperorfile.cpp lines341..732.
// Source branches annotated "assumed" are deliberately excluded.
inline std::optional<size_t> originalBuildingTailBytes(uint16_t type){
    switch(type){
    case 0x0:return 0;
    case 0x2:case 0x3:case 0x4:case 0x5:case 0x6:case 0x7:case 0x8:case 0x9:case 0xa:case 0xb:case 0xc:case 0xd:case 0xe:case 0xf:case 0x10:case 0x11:return 65;
    case 0x5e:return 2;
    case 0x48:case 0x6f:case 0x71:case 0x7c:case 0x7e:case 0x7f:case 0x8a:case 0x97:case 0xb1:case 0xb2:case 0xb3:case 0xb4:case 0xb5:case 0xb7:case 0xb9:case 0xba:case 0xbb:case 0xbc:case 0xbd:case 0xa1:case 0xa2:case 0xa3:case 0xa4:case 0xa5:case 0xa7:case 0xad:case 0xae:case 0xaf:case 0xcf:case 0xd0:return 130;
    case 0x59:case 0x5a:case 0x5b:case 0xe7:case 0x68:case 0x69:case 0x6a:case 0xe8:case 0x74:case 0x75:case 0x76:case 0x77:case 0x78:case 0x79:case 0x7a:case 0x7d:case 0x83:case 0xcb:case 0xd1:case 0xf3:case 0xf4:case 0xf5:case 0xf6:case 0xf7:case 0xf8:case 0xf9:case 0xfa:case 0xfb:case 0xfc:return 132;
    case 0x1f:case 0x21:case 0x26:case 0x2a:case 0x2b:case 0x2c:case 0x2d:case 0x2e:case 0x2f:case 0x31:case 0x34:case 0xd3:case 0xd4:case 0xd5:case 0xe2:case 0xe9:case 0xeb:case 0xec:return 134;
    case 0x23:case 0x24:case 0x25:case 0x27:case 0x28:case 0x29:case 0x30:case 0xdc:case 0xdd:case 0xdf:case 0xe0:case 0xe1:return 136;
    case 0x1a:case 0x1b:case 0x1c:case 0x3e:case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x82:case 0xc2:case 0xc3:case 0xc4:case 0xc5:case 0xc6:case 0xc7:return 140;
    case 0xc0:case 0xed:case 0xee:case 0xef:return 142;
    case 0xd6:case 0xd7:case 0xd9:return 165;
    case 0xd8:case 0xda:return 167;
    case 0xdb:return 169;
    case 0x6e:return 179;
    case 0xc1:return 222;
    case 0x4b:return 236;
    case 0x36:return 248;
    case 0x3b:case 0x3c:return 266;
    case 0x35:return 288;
    case 0x3a:return 348;
    case 0x47:return 352;
    case 0x38:return 356;
    case 0xd2:return 2140;
    default:return std::nullopt;}
}
inline bool originalBuildingMonument(uint16_t type){return (type>=0x4c&&type<=0x54)||type==0x5c||type==0x5d||(type>=0xfd&&type<=0x104);}
inline bool originalBuildingSquareType(uint16_t type){
    if(!type||!originalBuildingTailBytes(type))return false;
    switch(type){case 0x1f:case 0x38:case 0xd2: // docks
    case 0x6e:case 0xd1:case 0x82:case 0x3b:case 0x3c: // explicit nonsquare layouts
    case 0xdc:case 0xdd:case 0xdf:case 0xe0:case 0xe1: // multipart forts
    case 0x47:case 0x6f:case 0x71: // no published ordinary footprint placement
    case 0x8a:case 0x97:case 0xb1:case 0xb2:case 0xb3:case 0xb4:case 0xb5:case 0xb7:
    case 0xb9:case 0xba:case 0xbb:case 0xbc:case 0xbd:case 0xa1:case 0xa2:case 0xa3:
    case 0xa4:case 0xa5:case 0xa7:case 0xad:case 0xae:case 0xaf:return false; // editor placeholders
    default:return true;}
}
// Narrow published SAV building profile. A published ordinary square or explicit market rectangle semantic type with
// a closed record span and matching saved terrain footprint is exposed. Unknown
// type tails preserve the closed prefix; malformed data clears it completely.
// Opaque class-reference tags stay opaque;eligibility comes from the separately stored semantic type.
// No resync or game logic. Cited static-image inputs are retained separately;
// archive binding is an explicit caller operation above.
inline OriginalBuildingProfile parseOriginalBuildingTable(const std::vector<uint8_t>& data,size_t start,
                                                         size_t terrainOffset,const OriginalMapTerrain& terrain,bool localClasses=false){
    const auto failure=[](OriginalBuildingStatus s){OriginalBuildingProfile r;r.status=s;return r;};
    static constexpr std::array<uint8_t,20> preamble{{1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'}};
    if(data.size()>8u*1024u*1024u)return failure(OriginalBuildingStatus::BudgetExceeded);
    if(!terrain.valid()||terrainOffset>data.size()||start>terrainOffset||terrainOffset-start<preamble.size()||!std::equal(preamble.begin(),preamble.end(),data.begin()+start))return failure(OriginalBuildingStatus::Malformed);
    // MAP table-local PID context is verified independently; SAV context is not
    // table-local and must never be seeded with this dictionary.
    if(localClasses&&(start!=1093607||data.size()<8||data[0]!=5||data[1]!=0||data[2]!=0xfe||data[3]!=0xca||data[4]!=0||data[5]!=0||data[6]!=2||data[7]!=0))return failure(OriginalBuildingStatus::Malformed);
    try{
        std::vector<std::string> classes(1);size_t nextPID=1;
        OriginalBuildingProfile result;size_t at=start+6;
        for(size_t slot=0;slot<4000;++slot){
            const size_t begin=at;if(at>terrainOffset||terrainOffset-at<2)return failure(OriginalBuildingStatus::Malformed);
            const auto tag=detail::u16(data.data()+at);at+=2;std::string name,resolvedClass;
            if(tag==0xffff){
                ++result.declaredClasses;
                if(terrainOffset-at<4)return failure(OriginalBuildingStatus::Malformed);
                const auto zero=detail::u16(data.data()+at),length=detail::u16(data.data()+at+2);at+=4;
                if(zero||!length||length>255||length>terrainOffset-at)return failure(OriginalBuildingStatus::Malformed);
                for(size_t i=0;i<length;++i)if(!data[at+i]||data[at+i]>127)return failure(OriginalBuildingStatus::Malformed);
                name.assign(reinterpret_cast<const char*>(data.data()+at),length);at+=length;resolvedClass=name;
                if(localClasses){classes.resize(nextPID+1);classes[nextPID++]=name;}
            }else if(tag<0x8000)return failure(OriginalBuildingStatus::Malformed);
            else if(localClasses){const size_t id=tag&0x7fff;if(id>=classes.size()||classes[id].empty())return failure(OriginalBuildingStatus::Malformed);resolvedClass=classes[id];++result.classReferencesVerified;}
            if(localClasses)++nextPID;
            if(terrainOffset-at<2)return failure(OriginalBuildingStatus::Malformed);
            const auto version=detail::u16(data.data()+at);const size_t common=version==3?179:version==4?181:version==5?182:0;
            if(!common||common>terrainOffset-at)return failure(OriginalBuildingStatus::Malformed);
            const auto type=detail::u16(data.data()+at+16);auto tail=originalBuildingTailBytes(type);
            result.slotTypes[slot]=type;
            if(originalBuildingMonument(type)){
                if(terrainOffset-at-common<4)return failure(OriginalBuildingStatus::Malformed);
                const auto subversion=detail::u16(data.data()+at+common+2);
                if(subversion==9)tail=140;else if(subversion==10)tail=141;else {result.status=OriginalBuildingStatus::UnsupportedTail;return result;}
            }
            if(type==0&&(resolvedClass=="cResWall"||resolvedClass=="cResGate")){
                if(data[at+2]||data[at+5]){result.status=OriginalBuildingStatus::UnsupportedTail;return result;}tail=132;++result.clearedClassRecords;
            }
            if(!tail){result.status=OriginalBuildingStatus::UnsupportedTail;return result;}
            if(*tail>terrainOffset-at-common)return failure(OriginalBuildingStatus::Malformed);
            if(namedOriginalBuilding(type,name)){if(type==0x48)++result.namedWellRecords;else if(type==0x7c)++result.namedInspectorRecords;else ++result.namedWatchtowerRecords;}
            if(type&&data[at+2]&&data[at+5]){
                ++result.liveSizedRecords;++result.liveTypeCounts[type];
                if(!originalBuildingSquareType(type)&&type!=0x3b&&type!=0x3c){
                    ++result.unsupportedFootprintRecords;
                    result.unplacedMarkers.push_back({type,detail::u16(data.data()+at+8),detail::u16(data.data()+at+10),slot});
                    // Entertainment0x48CC80 ->0x48B630 ->0x51CE00. The
                    // current outer version5 reads parent runtime+150 at
                    // 0x48CDB5, tail348, after the two100-byte arrays.
                    const size_t tailAt=at+common;
                    if(type==0x47&&detail::u16(data.data()+tailAt)==1&&detail::u16(data.data()+tailAt+2)==1&&
                        detail::u16(data.data()+tailAt+130)==1&&detail::u16(data.data()+tailAt+132)==1&&detail::u16(data.data()+tailAt+134)==5)
                        result.objectBuildings.push_back({type,detail::u16(data.data()+at+8),detail::u16(data.data()+at+10),data[at+5],slot,begin,
                            detail::u32(data.data()+at+12),detail::u32(data.data()+tailAt+348),std::nullopt});
                    at+=common+*tail;++result.parsedRecords;continue;
                }
                OriginalBuildingFootprint f;f.type=type;f.x=detail::u16(data.data()+at+8);f.y=detail::u16(data.data()+at+10);f.size=data[at+5];f.slot=slot;f.recordOffset=begin;
                f.animationInputs=OriginalBuildingAnimationInputs{detail::i16(data.data()+at+62),detail::i16(data.data()+at+72),data[at+(version==5?98:97)],std::nullopt,std::nullopt,std::nullopt};
                if(type==0x3b||type==0x3c){const auto rotation=data[at+common+96];if(rotation>1){++result.unsupportedFootprintRecords;at+=common+*tail;++result.parsedRecords;continue;}const uint8_t side=type==0x3b?4:6;f.width=rotation?7:side;f.height=rotation?side:7;}
                if(type==0x48||type==0x7c||type==0x7f){OriginalWellImageInputs inputs;inputs.word16=detail::i16(data.data()+at+18);inputs.byte6f=data[at+(version==5?100:99)];f.wellImageInputs=inputs;}
                OriginalBuildingImageInputs inputs;inputs.gridIndex=detail::u32(data.data()+at+12);inputs.word20=detail::i16(data.data()+at+27);
                bool imageLayout=true;
                if(type==2||type==3||type==5||type==7){
                    // 0x518910 -> data vtable0x7B5C44+8 -> 0x517410:
                    // outer+data versions1; runtime data+0x3C is tail+59.
                    imageLayout=detail::u16(data.data()+at+common)==1&&detail::u16(data.data()+at+common+2)==1;inputs.houseByte3c=data[at+common+59];
                }
                if(type==0xc2||type==0xc4){
                    // 0x51CE00 / 0x51C3A0: outer+data headers, word at tail+4.
                    imageLayout=detail::u16(data.data()+at+common)==1&&detail::u16(data.data()+at+common+2)==1;inputs.cropWord4=detail::i16(data.data()+at+common+4);
                }
                if(type==0xd6){
                    // 0x5AB280 -> 0x5AAC80, nested rect0x4F8CA0 (18 bytes):
                    // outer+industrial+religious headers put data+8 at tail+156.
                    imageLayout=detail::u16(data.data()+at+common)==1&&detail::u16(data.data()+at+common+2)==1&&detail::u16(data.data()+at+common+130)==1&&detail::u16(data.data()+at+common+132)==1&&detail::u16(data.data()+at+common+134)==1;
                    inputs.religiousCode=detail::u32(data.data()+at+common+156);
                }
                if(type==0x36||type==0x3a){
                    // 0x5D62D0/0x5E1510 -> 0x5D4890 ->0x51CE00:
                    // industrial tail130, storage version2, then N component
                    // version1 headers and 3 u32s via0x5D26C0 (14 bytes each).
                    const size_t tailAt=at+common,count=type==0x36?8:15;
                    imageLayout=detail::u16(data.data()+tailAt)==1&&detail::u16(data.data()+tailAt+2)==1&&detail::u16(data.data()+tailAt+130)==2;
                    std::vector<OriginalStorageCompartment> compartments;
                    for(size_t c=0;c<count;++c){const size_t pos=tailAt+132+14*c;
                        imageLayout&=detail::u16(data.data()+pos)==1;
                        compartments.push_back({detail::u32(data.data()+pos+2),detail::u32(data.data()+pos+6),detail::u32(data.data()+pos+10)});
                    }
                    if(imageLayout)inputs.compartments=std::move(compartments);
                }
                if(type==0x3b){
                    const size_t tailAt=at+common;
                    // 0x544F10 ->0x5D4890 (6 compartments) then version6,
                    // six slot links0x544FB7 and later scalar fields.
                    imageLayout=detail::u16(data.data()+tailAt)==1&&detail::u16(data.data()+tailAt+2)==1&&
                        detail::u16(data.data()+tailAt+130)==2&&detail::u16(data.data()+tailAt+216)==6;
                    for(unsigned j=0;j<6;++j)imageLayout&=detail::u16(data.data()+tailAt+132+14*j)==1;
                    if(imageLayout){inputs.marketOrientation=data[tailAt+96];for(unsigned j=0;j<4;++j)inputs.marketStalls[j]=detail::u32(data.data()+tailAt+218+4*j);}
                }
                if(imageLayout)f.imageInputs=inputs;
                if(data[at+2]&&validOriginalBuildingFootprint(terrain,f))result.footprints.push_back(f);else ++result.rejectedTerrainFootprints;
            }
            at+=common+*tail;++result.parsedRecords;
        }
        result.tableEnd=at;result.status=OriginalBuildingStatus::Complete;return result;
    }catch(const std::bad_alloc&){return failure(OriginalBuildingStatus::BudgetExceeded);}
}
inline OriginalBuildingProfile parseOriginalSavedBuildings(const std::vector<uint8_t>& data,size_t terrainOffset,const OriginalMapTerrain& terrain){
    const auto failure=[](OriginalBuildingStatus s){OriginalBuildingProfile r;r.status=s;return r;};
    if(data.size()>8u*1024u*1024u)return failure(OriginalBuildingStatus::BudgetExceeded);
    if(data.size()<2||data[0]!=0x13||data[1]!=0||terrainOffset>data.size()||!terrain.valid())return failure(OriginalBuildingStatus::Malformed);
    static constexpr std::array<uint8_t,20> preamble{{1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'}};
    try{
        size_t hits=0,start=0;auto from=data.begin(),end=data.begin()+terrainOffset;
        while(from!=end){auto found=std::search(from,end,preamble.begin(),preamble.end());if(found==end)break;if(++hits>16)return failure(OriginalBuildingStatus::BudgetExceeded);start=size_t(found-data.begin());from=found+1;}
        if(hits!=1)return failure(hits?OriginalBuildingStatus::AmbiguousAnchor:OriginalBuildingStatus::MissingAnchor);
        auto result=parseOriginalBuildingTable(data,start,terrainOffset,terrain,false);
        // SAV19 uses 0x52FDA0, not MAP's 0x52E7C0. Its write calls at
        // 0x52FF32/43/54/65/76 serialize terrain(u32),0xFAA130(u16),
        // subTile(u8),0xF2B290(u8),then runtime random0xF1E780(u8).
        // Read counterparts: 0x531327/38/49/5A/6B (SAV19 branch).
        // Thus mapperRandom at terrain+8*cells IS the runtime random grid;
        // terrain+9*cells is the different 0xF11C70 grid (0x53137C).
        // Owned exe SHA256: 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
        constexpr size_t randomOffset=415872,cells=51984;
        if(terrainOffset<=data.size()&&randomOffset+cells<=data.size()-terrainOffset){
            const size_t origin=(228-terrain.activeSide)/2;
            for(auto& f:result.footprints)if(f.imageInputs){
                const size_t i=(origin+f.y)*228+origin+f.x;
                if(f.imageInputs->gridIndex==i)f.imageInputs->randomByte=data[terrainOffset+randomOffset+i];
            }
        }
        // SAV19 branch0x5311DC: owner0xFC3750 read0x531316 at T-103968;
        // saved view0x115F720 read0x531419 at T+1299324. 0x415464..0x4154D4
        // resolves each signed owner through0x47F1B0 and tests its semantic type.
        if(result.status==OriginalBuildingStatus::Complete&&terrainOffset>=155952&&
            terrainOffset<=data.size()&&original_save19::savedView+4<=data.size()-terrainOffset){
            const uint32_t view=detail::u32(data.data()+terrainOffset+original_save19::savedView);
            result.savedView=view;
            const size_t origin=(228-terrain.activeSide)/2;
            for(auto& f:result.footprints)if(f.animationInputs&&f.x<terrain.activeSide&&uint32_t(f.y)+f.size<=terrain.activeSide&&view==0){
                const size_t corner=(origin+f.y+f.size-1)*228+origin+f.x;
                auto& a=*f.animationInputs;a.phase=data[terrainOffset+363888+corner];a.savedView=0;
                a.owner=detail::u16(data.data()+terrainOffset-103968+2*corner);
            }
            for(auto& object:result.objectBuildings)if(view<=6&&!(view&1)&&object.x<terrain.activeSide&&object.y<terrain.activeSide&&
                object.gridIndex==(origin+object.y)*228+origin+object.x)object.savedView=uint8_t(view);
            for(auto& f:result.footprints)if(f.type==0x3b&&f.imageInputs&&f.imageInputs->marketOrientation&&view==0&&f.imageInputs->gridIndex==(origin+f.y)*228+origin+f.x){
                const bool rotated=*f.imageInputs->marketOrientation==1;bool valid=*f.imageInputs->marketOrientation<=1;std::vector<uint8_t> random,draw;
                for(unsigned y=0;y<7;++y)for(unsigned x=0;x<4;++x){
                    const size_t cell=(origin+f.y+(rotated?x:y))*228+origin+f.x+(rotated?y:x);
                    if(cell>=cells){valid=false;break;}
                    random.push_back(data[terrainOffset+randomOffset+cell]);
                    draw.push_back(data[terrainOffset-155952+cell]);
                }
                if(valid){f.imageInputs->marketRandom=std::move(random);f.imageInputs->marketDraw=std::move(draw);f.imageInputs->savedView=uint8_t(view);}
            }
            for(auto& f:result.footprints)if(f.type==0xe7&&f.imageInputs){
                const size_t cell=(origin+f.y)*228+origin+f.x;
                if(view>6||(view&1)||f.imageInputs->gridIndex!=cell||
                    detail::u16(data.data()+terrainOffset-103968+2*cell)!=f.slot)continue;
                uint8_t mask=0;bool valid=true;const int delta[4]={-228,1,228,-1};
                for(unsigned d=0;d<4;++d){
                    const int adjacent=int(cell)+delta[d];if(adjacent<0||adjacent>=int(cells)){valid=false;break;}
                    const auto owner=detail::u16(data.data()+terrainOffset-103968+2*size_t(adjacent));
                    if(owner>=4000){valid=false;break;}if(!owner)continue;
                    switch(result.slotTypes[owner]){case 0xe7:case 0x5b:case 0x5a:case 0x59:case 0xe8:case 0x6a:case 0x69:case 0x68:mask|=uint8_t(1u<<d);break;default:break;}
                }
                if(valid){f.imageInputs->wallMask=mask;f.imageInputs->savedView=uint8_t(view);}
            }
        }
        return result;
    }catch(const std::bad_alloc&){return failure(OriginalBuildingStatus::BudgetExceeded);}
}
inline OriginalBuildingProfile parseOriginalMapBuildings(const std::vector<uint8_t>& data,const OriginalMapTerrain& terrain){return parseOriginalBuildingTable(data,1093607,data.size(),terrain,true);}
// Additive, opt-in update inputs. No existing image/scene parsing is changed.
// Common5 serializer0x427430: wire+2/+65/+76/+99/+162 =>
// runtime+4/+47/+52/+6F/+B4. Industrial1 data0x51C3A0 is byte-for-byte
// tail+4..129 after two version headers, including +4/+63/+6A.
struct OriginalBuildingUpdateInputs {
    uint8_t alive=0,output=0,byte52=0,upgrade=0;
    uint32_t identity=0;
    std::optional<std::array<uint8_t,126>> industry; // data+4..81 inclusive
    std::optional<std::array<uint8_t,61>> house; // serialized tail+4..64,0x517410
};
inline std::optional<std::map<size_t,OriginalBuildingUpdateInputs>> originalBuildingUpdateInputs(
    const std::vector<uint8_t>& data,const OriginalBuildingProfile& profile){
    if(data.size()>8u*1024u*1024u||profile.status!=OriginalBuildingStatus::Complete||profile.parsedRecords!=4000)return std::nullopt;
    std::map<size_t,size_t> records;
    for(const auto& f:profile.footprints)if(!records.emplace(f.slot,f.recordOffset).second)return std::nullopt;
    for(const auto& f:profile.objectBuildings)if(!records.emplace(f.slot,f.recordOffset).second)return std::nullopt;
    size_t live=0;for(const auto& e:profile.liveTypeCounts)live+=e.second;
    if(records.size()!=live)return std::nullopt;
    std::map<size_t,OriginalBuildingUpdateInputs> out;
    for(const auto& e:records){size_t at=e.second;
        if(e.first>=4000||at>data.size()||data.size()-at<2)return std::nullopt;
        const auto tag=detail::u16(data.data()+at);at+=2;
        if(tag==0xffff){if(data.size()-at<4)return std::nullopt;const size_t n=detail::u16(data.data()+at+2);
            if(n>data.size()-at-4)return std::nullopt;at+=4+n;}
        else if(tag<0x8000)return std::nullopt;
        if(data.size()-at<182||detail::u16(data.data()+at)!=5||detail::u16(data.data()+at+16)!=profile.slotTypes[e.first])return std::nullopt;
        const auto* p=data.data()+at;OriginalBuildingUpdateInputs v;
        v.alive=p[2];v.output=p[65];v.byte52=p[76];v.upgrade=p[99];v.identity=detail::u32(p+162);
        const size_t tail=at+182;const uint16_t type=profile.slotTypes[e.first];
        if(type==0xc2||type==0xc4){
            if(data.size()-tail<140||detail::u16(data.data()+tail)!=1||detail::u16(data.data()+tail+2)!=1||
                detail::u16(data.data()+tail+130)!=1||detail::u16(data.data()+tail+132)!=1||detail::u16(data.data()+tail+138)!=1)return std::nullopt;
            v.industry.emplace();std::copy_n(data.begin()+tail+4,126,v.industry->begin());
        }
        if(type>=2&&type<=17){
            if(data.size()-tail<65||detail::u16(data.data()+tail)!=1||detail::u16(data.data()+tail+2)!=1)return std::nullopt;
            v.house.emplace();std::copy_n(data.begin()+tail+4,61,v.house->begin());
        }
        out.emplace(e.first,v);
    }
    return out;
}
// Additive industry inputs for0x51E4A0; existing record/image readers unchanged.
// Common5 read0x427430: runtime+16/+1C/+40/+44/+6E/+72/+74/+90
// are wire18/23/58/62/98/101/103/129. Data1 is0x51C3A0.
struct OriginalBuildingProductionInputs {
    int16_t primaryResource=0,operational=0,primaryStock=0,workers=0,secondaryStock=0,secondaryResource=0;
    uint8_t laborDisabled=0,enabled=0;
    std::array<uint8_t,126> industry{};
};
inline std::optional<std::map<size_t,OriginalBuildingProductionInputs>> originalBuildingProductionInputs(
    const std::vector<uint8_t>& data,const OriginalBuildingProfile& profile){
    if(data.size()>8u*1024u*1024u||profile.status!=OriginalBuildingStatus::Complete||profile.parsedRecords!=4000)return std::nullopt;
    std::map<size_t,OriginalBuildingProductionInputs> out;
    for(const auto& f:profile.footprints){if(f.type!=33&&f.type!=35&&f.type!=43)continue;
        size_t at=f.recordOffset;if(at>data.size()||data.size()-at<2)return std::nullopt;
        const auto tag=detail::u16(data.data()+at);at+=2;
        if(tag==0xffff){if(data.size()-at<4)return std::nullopt;const size_t n=detail::u16(data.data()+at+2);
            if(n>data.size()-at-4)return std::nullopt;at+=4+n;}
        else if(tag<0x8000)return std::nullopt;
        const size_t tailBytes=f.type==35?136:134;
        if(data.size()-at<182+tailBytes||detail::u16(data.data()+at)!=5||detail::u16(data.data()+at+16)!=f.type||
            detail::u32(data.data()+at+162)!=f.slot)return std::nullopt;
        const auto* p=data.data()+at;const auto* tail=p+182;
        // Hunter5F36E0/Clay5AC400/Kiln5F2850 ->5F1240 ->51CE00.
        // Outer1/data1, industry1, then derived1; Clay adds a final word.
        for(size_t offset:{0u,2u,130u,132u})if(detail::u16(tail+offset)!=1)return std::nullopt;
        OriginalBuildingProductionInputs v;v.primaryResource=detail::i16(p+18);v.operational=detail::i16(p+23);
        v.primaryStock=detail::i16(p+58);v.workers=detail::i16(p+62);v.laborDisabled=p[98];
        v.secondaryStock=detail::i16(p+101);v.secondaryResource=detail::i16(p+103);v.enabled=p[129];
        std::copy_n(tail+4,126,v.industry.begin());if(!out.emplace(f.slot,v).second)return std::nullopt;
    }
    return out;
}
//Opt-in common5 inputs for4AD3D0;427D4A/8B/98/DA5/DB2 and
//517410 restore model/residents/free/distance/peak and embedded+3C.
struct OriginalBuildingHouseCapacityInputs {uint16_t type=0;int16_t model=0,residents=0,free=0,distance=0,peak=0;uint8_t laborDisabled=0;};
inline std::optional<std::map<size_t,OriginalBuildingHouseCapacityInputs>> originalBuildingHouseCapacityInputs(
    const std::vector<uint8_t>& data,const OriginalBuildingProfile& profile){
    if(data.size()>8u*1024u*1024u||profile.status!=OriginalBuildingStatus::Complete||profile.parsedRecords!=4000)return std::nullopt;
    std::map<size_t,OriginalBuildingHouseCapacityInputs> out;
    for(const auto& f:profile.footprints)if(f.type>=2&&f.type<=17){
        size_t at=f.recordOffset;if(at>data.size()||data.size()-at<2)return std::nullopt;const auto tag=detail::u16(data.data()+at);at+=2;
        if(tag==0xffff){if(data.size()-at<4)return std::nullopt;const size_t n=detail::u16(data.data()+at+2);if(n>data.size()-at-4)return std::nullopt;at+=4+n;}
        else if(tag<0x8000)return std::nullopt;
        if(data.size()-at<247||detail::u16(data.data()+at)!=5||detail::u16(data.data()+at+16)!=f.type||
            detail::u32(data.data()+at+162)!=f.slot||detail::u16(data.data()+at+182)!=1||detail::u16(data.data()+at+184)!=1)return std::nullopt;
        const auto* p=data.data()+at;out.emplace(f.slot,OriginalBuildingHouseCapacityInputs{
            f.type,detail::i16(p+18),detail::i16(p+27),detail::i16(p+29),detail::i16(p+31),detail::i16(p+33),p[182+59]});
    }
    return out;
}
inline std::string describeOriginalBuildingProfile(const OriginalBuildingProfile& p){
    const bool prefix=p.status==OriginalBuildingStatus::UnsupportedTail;
    if(p.status!=OriginalBuildingStatus::Complete&&!prefix)return "建筑记录不兼容或锚点不唯一；没有占地可发布";
    return "建筑表已验证 "+std::to_string(p.parsedRecords)+"/4000 记录"+(prefix?"（遇未知尾部停止，仅支持前缀）":"")+"；已验证建筑占地 "+std::to_string(p.footprints.size())+"，未绑定图像";
}
}
