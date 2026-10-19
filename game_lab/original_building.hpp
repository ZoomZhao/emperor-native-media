#pragma once
#include "original_map.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <new>
namespace game_lab {
struct OriginalBuildingFootprint {uint16_t type=0,x=0,y=0;uint8_t size=0;size_t slot=0,recordOffset=0;};
enum class OriginalBuildingStatus {Complete,UnsupportedTail,MissingAnchor,AmbiguousAnchor,Malformed,BudgetExceeded};
struct OriginalBuildingProfile {
    OriginalBuildingStatus status=OriginalBuildingStatus::MissingAnchor;
    size_t parsedRecords=0,namedWellRecords=0,rejectedNamedRecords=0,namedInspectorRecords=0,namedWatchtowerRecords=0;
    std::vector<OriginalBuildingFootprint> footprints;
};
inline const char* originalBuildingName(uint16_t type){
    switch(type){case 0x48:return "Well";case 0x7c:return "Inspector";case 0x7f:return "Watchtower";default:return "Unknown";}
}
inline bool namedOriginalBuilding(uint16_t type,const std::string& name){
    return (type==0x48&&name=="cWellBldg")||(type==0x7c&&name=="cInspectorBldg")||(type==0x7f&&name=="cWatchtower");
}
inline bool validOriginalBuildingFootprint(const OriginalMapTerrain& terrain,const OriginalBuildingFootprint& f){
    if(!terrain.valid()||(f.type!=0x48&&f.type!=0x7c&&f.type!=0x7f)||!f.size||f.size>8||uint32_t(f.x)+f.size>terrain.activeSide||uint32_t(f.y)+f.size>terrain.activeSide)return false;
    const size_t origin=(228-terrain.activeSide)/2;
    for(size_t y=0;y<f.size;++y)for(size_t x=0;x<f.size;++x){const auto v=terrain.flags[(origin+f.y+y)*228+origin+f.x+x];if((v&0x80000u)||!(v&8u))return false;}
    return true;
}
inline std::optional<size_t> originalBuildingTailBytes(uint16_t type){
    if(type==0)return 0;
    if(type>=2&&type<=17)return 65;
    if(type==0x3e||(type>=0x40&&type<=0x46))return 140;
    switch(type){case 0x35:return 288;case 0x3b:return 266;case 0x47:return 352;
    case 0x48:case 0x7c:case 0x7f:case 0x7e:case 0xa2:case 0xa3:case 0xaf:return 130;
    case 0x83:case 0xe7:return 132;default:return std::nullopt;}
}
// Narrow published SAV building profile. Only an explicitly named Well,Inspector or Watchtower with
// a closed record span and matching saved terrain footprint is exposed. Unknown
// type tails preserve the closed prefix; malformed data clears it completely.
// No serialization-reference guessing,resync,SG3 binding or game logic.
inline OriginalBuildingProfile parseOriginalSavedBuildings(const std::vector<uint8_t>& data,
                                                           size_t terrainOffset,const OriginalMapTerrain& terrain){
    const auto failure=[](OriginalBuildingStatus s){OriginalBuildingProfile r;r.status=s;return r;};
    if(data.size()>8u*1024u*1024u)return failure(OriginalBuildingStatus::BudgetExceeded);
    if(data.size()<2||data[0]!=0x13||data[1]!=0||terrainOffset>data.size()||!terrain.valid())return failure(OriginalBuildingStatus::Malformed);
    static constexpr std::array<uint8_t,20> preamble{{1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'}};
    try{
        size_t hits=0,start=0;auto from=data.begin(),end=data.begin()+terrainOffset;
        while(from!=end){auto found=std::search(from,end,preamble.begin(),preamble.end());if(found==end)break;if(++hits>16)return failure(OriginalBuildingStatus::BudgetExceeded);start=size_t(found-data.begin());from=found+1;}
        if(hits!=1)return failure(hits?OriginalBuildingStatus::AmbiguousAnchor:OriginalBuildingStatus::MissingAnchor);
        OriginalBuildingProfile result;size_t at=start+6;
        for(size_t slot=0;slot<4000;++slot){
            const size_t begin=at;if(at>terrainOffset||terrainOffset-at<2)return failure(OriginalBuildingStatus::Malformed);
            const auto tag=detail::u16(data.data()+at);at+=2;std::string name;
            if(tag==0xffff){
                if(terrainOffset-at<4)return failure(OriginalBuildingStatus::Malformed);
                const auto zero=detail::u16(data.data()+at),length=detail::u16(data.data()+at+2);at+=4;
                if(zero||!length||length>255||length>terrainOffset-at)return failure(OriginalBuildingStatus::Malformed);
                for(size_t i=0;i<length;++i)if(!data[at+i]||data[at+i]>127)return failure(OriginalBuildingStatus::Malformed);
                name.assign(reinterpret_cast<const char*>(data.data()+at),length);at+=length;
            }else if(tag<0x8000)return failure(OriginalBuildingStatus::Malformed);
            if(terrainOffset-at<2)return failure(OriginalBuildingStatus::Malformed);
            const auto version=detail::u16(data.data()+at);const size_t common=version==3?179:version==4?181:version==5?182:0;
            if(!common||common>terrainOffset-at)return failure(OriginalBuildingStatus::Malformed);
            const auto type=detail::u16(data.data()+at+16);const auto tail=originalBuildingTailBytes(type);
            if(!tail){result.status=OriginalBuildingStatus::UnsupportedTail;return result;}
            if(*tail>terrainOffset-at-common)return failure(OriginalBuildingStatus::Malformed);
            if(namedOriginalBuilding(type,name)){
                if(type==0x48)++result.namedWellRecords;else if(type==0x7c)++result.namedInspectorRecords;else ++result.namedWatchtowerRecords;OriginalBuildingFootprint f;f.type=type;f.x=detail::u16(data.data()+at+8);f.y=detail::u16(data.data()+at+10);f.size=data[at+5];f.slot=slot;f.recordOffset=begin;
                if(data[at+2]&&validOriginalBuildingFootprint(terrain,f))result.footprints.push_back(f);else ++result.rejectedNamedRecords;
            }
            at+=common+*tail;++result.parsedRecords;
        }
        result.status=OriginalBuildingStatus::Complete;return result;
    }catch(const std::bad_alloc&){return failure(OriginalBuildingStatus::BudgetExceeded);}
}
inline std::string describeOriginalBuildingProfile(const OriginalBuildingProfile& p){
    const bool prefix=p.status==OriginalBuildingStatus::UnsupportedTail;
    if(p.status!=OriginalBuildingStatus::Complete&&!prefix)return "建筑记录不兼容或锚点不唯一；没有占地可发布";
    return "建筑表已验证 "+std::to_string(p.parsedRecords)+"/4000 记录"+(prefix?"（遇未知尾部停止，仅支持前缀）":"")+"；已命名建筑占地 "+std::to_string(p.footprints.size())+"，未绑定图像";
}
}
