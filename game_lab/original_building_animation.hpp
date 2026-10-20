#pragma once
#include "original_building.hpp"
#include <sstream>
#include <limits>

namespace game_lab {
using OriginalBuildingEmployment=std::map<uint16_t,int32_t>;
//0x418AA0: add modulo32 then compare as signed32 (not elapsed subtraction).
// Clock-array origin is an explicit process input and is never read from SAV.
struct OriginalAnimationClock {
    std::array<uint32_t,51> last{};
    std::array<bool,51> flags{};
};
inline OriginalAnimationClock originalAnimationClockFrame(OriginalAnimationClock clock,uint32_t now){
    const auto signed32=[](uint32_t bits){return bits>INT32_MAX?int64_t(bits)-4294967296LL:int64_t(bits);};
    for(size_t k=0;k<51;++k){clock.flags[k]=signed32(now)>=signed32(clock.last[k]+uint32_t(20*k));
        if(clock.flags[k])clock.last[k]=now;}
    return clock;
}
// 0x5D1830 loads BUILDING MODS/ALL BUILDINGS into0xA5B398, thirteen
// integers per row;0x44CC50(type,5) reads employment. Only publish rows
// whose employment is identical for all five difficulty modifiers. A changed
// model requiring saved difficulty is explicitly unsupported, not defaulted.
inline std::optional<OriginalBuildingEmployment> parseOriginalBuildingEmployment(const std::string& text){
    if(text.size()>1024u*1024u||text.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream input(text);std::string line;unsigned section=0,mods=0;std::array<int32_t,5> percent{};
    OriginalBuildingEmployment out;
    while(std::getline(input,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line=="BUILDING MODS"){if(section)return std::nullopt;section=1;continue;}
        if(line=="ALL BUILDINGS"){if(section!=1||mods!=5)return std::nullopt;section=2;continue;}
        if(line=="HOUSE MODS"){if(section!=2)return std::nullopt;section=3;break;}
        if((section!=1&&section!=2)||line.empty()||line[0]<'0'||line[0]>'9')continue;
        const auto comma=line.find(','),open=line.find("{,"),close=line.find(",}",open);
        if(comma==std::string::npos||open==std::string::npos||close==std::string::npos)return std::nullopt;
        const auto number=[](const std::string& s)->std::optional<int32_t>{
            std::istringstream in(s);int64_t n;char extra;
            if(!(in>>n)||(in>>extra)||n<std::numeric_limits<int32_t>::min()||n>std::numeric_limits<int32_t>::max())return std::nullopt;
            return int32_t(n);
        };
        const auto id=number(line.substr(0,comma));if(!id||*id<0||*id>65535)return std::nullopt;
        std::string fields=line.substr(open+2,close-open-2);std::replace(fields.begin(),fields.end(),',',' ');
        std::istringstream values(fields);std::array<int32_t,13> row{};for(auto& n:row)if(!(values>>n))return std::nullopt;
        char extra;if(values>>extra)return std::nullopt;
        if(section==1){if(*id!=int(mods)||mods>=5||row[5]<0)return std::nullopt;percent[mods++]=row[5];}
        else{
            // The original loader walks rows in order, not their printed IDs.
            if(row[5]<0||size_t(*id)!=out.size())return std::nullopt;
            const int64_t required=int64_t(row[5])*percent[0]/100;
            if(required>std::numeric_limits<int32_t>::max())return std::nullopt;
            for(auto p:percent)if(int64_t(row[5])*p/100!=required)return std::nullopt;
            out.emplace(uint16_t(*id),int32_t(required));
        }
    }
    if(section!=3||out.empty())return std::nullopt;return out;
}
//0x418B5E..0x418C61 chooses +1BC for these bound types; all other bound
// records with nonzero SG counts are unsupported until their dispatch is cited.
inline bool originalBuildingAnimationLaborGate(uint16_t type){
    switch(type){case 0x21:case 0x23:case 0x35:case 0x48:case 0xc0:case 0xc1:return true;default:return false;}
}
inline std::optional<bool> originalBuildingAnimationEnabled(const OriginalBuildingFootprint& f,
    const OriginalBuildingEmployment* employment){
    if(!f.animationInputs)return std::nullopt;
    const auto& a=*f.animationInputs;
    if(!a.savedView||*a.savedView!=0||!a.owner||*a.owner!=f.slot||!a.phase)return std::nullopt;
    if(a.suppress)return false; // +1C0:0x51E2D0,0x42B680,0x5AB390.
    if(!employment)return std::nullopt;
    const auto found=employment->find(f.type);if(found==employment->end()||found->second<0)return std::nullopt;
    const int32_t required=a.laborDisabled?0:found->second; //0x428EB0.
    if(originalBuildingAnimationLaborGate(f.type)){
        // +1BC=0x416B60 -> +1B4=0x428ED0, +1B8=0x416B10.
        if(required<=0||int64_t(a.workers)*100/required==0)return false;
    }else switch(f.type){
        // +C8=0x427180(-1) is false; +1B0 still enforces workers.
        case 0x7c:case 0x7f:case 0xcf:case 0xd3:case 0xd6:break;
        default:return std::nullopt;
    }
    return !(a.workers<=0&&required!=0); //0x418C63..77.
}
// Clock flags0x418AA0 use timeGetTime() from0x536B20, interval20*speedId.
// tickMs is caller-supplied elapsed time with regular eligible draw calls and
// a fresh interval origin. Original timing is draw-driven, does not catch up,
// and its clock phase is not serialized. tick0 preserves the SAV byte exactly.
inline std::optional<uint8_t> originalBuildingAnimationState(const SG3Sprite& base,uint8_t saved,uint64_t tickMs=0){
    const uint32_t count=base.animationSpriteCount;
    if(!count||count>127||base.animationSpeedId>=51||(saved&127)>count||(!base.animationReversible&&(saved&128)))return std::nullopt;
    // speed0 fires on every draw; elapsed milliseconds cannot specify its calls.
    if(!base.animationSpeedId&&tickMs)return std::nullopt;
    uint64_t steps=base.animationSpeedId?tickMs/(20u*base.animationSpeedId):0;
    if(!steps)return saved;
    uint32_t frame=saved&127;
    if(!base.animationReversible){frame=frame?1+uint32_t(((frame-1)+steps%count)%count):1+uint32_t((steps-1)%count);return uint8_t(frame);}
    // Reversible endpoints repeat:1,2,..,N,N,..,2,1,1. Zero is an
    // initialization state, not an extra visible animation record.
    if(!frame){frame=1;--steps;saved=1;}
    const uint32_t cycle=2*count;
    uint32_t phase=(saved&128)?2*count-frame:frame-1;
    phase=uint32_t((phase+steps%cycle)%cycle);
    return phase<count?uint8_t(phase+1):uint8_t((2*count-phase)|128);
}
// One actual enabled draw0x418C79..0x418D37, using the shared current flags.
// Unlike the elapsed preview this can represent irregular draws and speed0.
inline std::optional<uint8_t> originalBuildingAnimationDraw(const SG3Sprite& base,uint8_t saved,
    const std::array<bool,51>& flags){
    if(!originalBuildingAnimationState(base,saved))return std::nullopt;
    if(!flags[base.animationSpeedId])return saved;
    unsigned frame=saved&127;bool reverse=(saved&128)!=0;
    if(!base.animationReversible){if(++frame>base.animationSpriteCount)frame=1;return uint8_t(frame);}
    if(reverse){if(frame<=1){frame=1;reverse=false;}else --frame;}
    else if(++frame>base.animationSpriteCount){frame=base.animationSpriteCount;reverse=true;}
    return uint8_t(frame|(reverse?128:0));
}
//0x429903: physical base+positive frame.0x42994B/0x429BF3 horizontal
// placement and0x429C11..2A vertical placement read BASE offsets/dimensions.
inline std::optional<std::pair<int32_t,int32_t>> originalBuildingAnimationOffset(
    const SG3Sprite& base,const SG3Sprite& frame,unsigned side){
    if(side<1||side>6||base.width!=int(80*side-2)||base.height<int(40*side)||(frame.type!=256&&frame.type!=0)||frame.mirrorOffset||
        frame.tileSize||frame.width<=0||frame.height<=0||frame.width>4096||frame.height>4096||frame.bitmapId!=base.bitmapId)return std::nullopt;
    // In view0,+1F0 is false (0x429090 ->0x423D40). The temporary
    // base/frame half-width terms cancel at0x4299D7..0x429BFD.
    return std::make_pair(int32_t(base.animationOffsetX),
        int32_t(base.animationOffsetY)-base.height+20*int32_t(side+1));
}
}
