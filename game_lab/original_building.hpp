#pragma once
#include "original_map.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <new>
namespace game_lab {
struct OriginalBuildingFootprint {uint16_t type=0,x=0,y=0;uint8_t size=0,width=0,height=0;size_t slot=0,recordOffset=0;};
enum class OriginalBuildingStatus {Complete,UnsupportedTail,MissingAnchor,AmbiguousAnchor,Malformed,BudgetExceeded};
struct OriginalBuildingProfile {
    OriginalBuildingStatus status=OriginalBuildingStatus::MissingAnchor;
    size_t parsedRecords=0,namedWellRecords=0,rejectedTerrainFootprints=0,namedInspectorRecords=0,namedWatchtowerRecords=0,liveSizedRecords=0,unsupportedFootprintRecords=0,clearedClassRecords=0,classReferencesVerified=0;
    std::vector<OriginalBuildingFootprint> footprints;
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
// No resync,SG3 binding or game logic.
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
                ++result.liveSizedRecords;
                if(!originalBuildingSquareType(type)&&type!=0x3b&&type!=0x3c){++result.unsupportedFootprintRecords;at+=common+*tail;++result.parsedRecords;continue;}
                OriginalBuildingFootprint f;f.type=type;f.x=detail::u16(data.data()+at+8);f.y=detail::u16(data.data()+at+10);f.size=data[at+5];f.slot=slot;f.recordOffset=begin;
                if(type==0x3b||type==0x3c){const auto rotation=data[at+common+96];if(rotation>1){++result.unsupportedFootprintRecords;at+=common+*tail;++result.parsedRecords;continue;}const uint8_t side=type==0x3b?4:6;f.width=rotation?7:side;f.height=rotation?side:7;}
                if(data[at+2]&&validOriginalBuildingFootprint(terrain,f))result.footprints.push_back(f);else ++result.rejectedTerrainFootprints;
            }
            at+=common+*tail;++result.parsedRecords;
        }
        result.status=OriginalBuildingStatus::Complete;return result;
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
        return parseOriginalBuildingTable(data,start,terrainOffset,terrain,false);
    }catch(const std::bad_alloc&){return failure(OriginalBuildingStatus::BudgetExceeded);}
}
inline OriginalBuildingProfile parseOriginalMapBuildings(const std::vector<uint8_t>& data,const OriginalMapTerrain& terrain){return parseOriginalBuildingTable(data,1093607,data.size(),terrain,true);}
inline std::string describeOriginalBuildingProfile(const OriginalBuildingProfile& p){
    const bool prefix=p.status==OriginalBuildingStatus::UnsupportedTail;
    if(p.status!=OriginalBuildingStatus::Complete&&!prefix)return "建筑记录不兼容或锚点不唯一；没有占地可发布";
    return "建筑表已验证 "+std::to_string(p.parsedRecords)+"/4000 记录"+(prefix?"（遇未知尾部停止，仅支持前缀）":"")+"；已验证建筑占地 "+std::to_string(p.footprints.size())+"，未绑定图像";
}
}
