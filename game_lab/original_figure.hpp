#pragma once
#include "original_building.hpp"
#include "../file_io/include/city19_figure7_layout.hpp"

namespace game_lab {
// Owned executable/hash and every field/branch: docs/SAV_FIGURES.md.
// These are persisted draw inputs; no original factories or simulation execute.
struct OriginalFigure {
    size_t slot=0,recordOffset=0;
    uint32_t image=0,secondaryImage=0,cell=0;
    int16_t x=0,y=0,subX=0,subY=0,extraX=0,extraY=0;
    uint16_t next=0;
    uint8_t type=0,alive=0,frame=0,action=0,progress=0,direction=0,previousDirection=0;
    uint8_t freeMovement=0,height=0,hidden=0,drawClass=0,state85=0,raise=0;
};
enum class OriginalFigureStatus {Complete,Unsupported,Malformed,BudgetExceeded};
struct OriginalFigureProfile {
    OriginalFigureStatus status=OriginalFigureStatus::Unsupported;
    size_t start=0,end=0,declared=0,parsed=0;
    uint32_t savedView=0;
    std::vector<OriginalFigure> live;
    std::map<uint8_t,size_t> liveTypes;
    std::vector<uint16_t> cellHeads; //0xFAA130, SAV19 T+207936, read0x531338.
};
// List0x4E1E40, called0x531265 immediately after Building0x42D790.
// Class IDs use the SAME archive cache: goal cache + building declarations +
// building objects. No byte-pattern resync, inferred class ID or default record.
inline OriginalFigureProfile parseOriginalSavedFigures(const std::vector<uint8_t>& data,
    size_t start,size_t limit,size_t nextPID,uint32_t savedView){
    OriginalFigureProfile out;out.start=start;out.savedView=savedView;
    const auto fail=[&](OriginalFigureStatus status){OriginalFigureProfile r;r.start=start;r.status=status;return r;};
    if(data.size()>8u*1024u*1024u||nextPID>32768)return fail(OriginalFigureStatus::BudgetExceeded);
    if(start>limit||limit>data.size()||limit-start<6||!nextPID)return fail(OriginalFigureStatus::Malformed);
    size_t at=start;
    const auto take=[&](size_t n){if(n>limit-at)return false;at+=n;return true;};
    if(detail::u16(data.data()+at)!=1)return fail(OriginalFigureStatus::Unsupported);
    out.declared=detail::u32(data.data()+at+2);at+=6;
    if(out.declared>8192)return fail(OriginalFigureStatus::BudgetExceeded);
    enum class Class {Figure,Pheasant,Emissary};
    std::map<size_t,Class> classes;std::map<size_t,OriginalFigure> objects;
    for(size_t slot=0;slot<out.declared;++slot){
        const size_t begin=at;if(!take(2))return fail(OriginalFigureStatus::Malformed);
        uint32_t tag=detail::u16(data.data()+at-2);
        const bool declaration=tag==0xffff;
        if(tag==0x7fff){if(!take(4))return fail(OriginalFigureStatus::Malformed);tag=detail::u32(data.data()+at-4);}
        else if(tag!=0xffff)tag=(tag&0x7fff)|((tag&0x8000)<<16);
        Class kind=Class::Figure;
        if(declaration){
            if(!take(4))return fail(OriginalFigureStatus::Malformed);
            const auto schema=detail::u16(data.data()+at-4),n=detail::u16(data.data()+at-2);
            if(schema||!n||n>=64||!take(n))return fail(OriginalFigureStatus::Malformed);
            const std::string name(reinterpret_cast<const char*>(data.data()+at-n),n);
            if(name=="Figure")kind=Class::Figure;
            else if(name=="cPheasantFig")kind=Class::Pheasant;
            else if(name=="cEmissaryDude")kind=Class::Emissary;
            else return fail(OriginalFigureStatus::Unsupported);
            classes.emplace(nextPID++,kind);
        }else if(tag&0x80000000u){
            const auto found=classes.find(tag&0x7fffffffu);
            if(found==classes.end())return fail(OriginalFigureStatus::Malformed);kind=found->second;
        }else{
            const auto found=objects.find(tag);if(found==objects.end())return fail(OriginalFigureStatus::Malformed);
            auto f=found->second;f.slot=slot;f.recordOffset=begin;
            if(f.alive){out.live.push_back(f);++out.liveTypes[f.type];}++out.parsed;continue;
        }
        if(nextPID>=32768)return fail(OriginalFigureStatus::BudgetExceeded);
        const size_t objectPID=nextPID++;
        if(kind!=Class::Figure){if(!take(2))return fail(OriginalFigureStatus::Malformed);if(detail::u16(data.data()+at-2)!=2)return fail(OriginalFigureStatus::Unsupported);}
        if(!take(2))return fail(OriginalFigureStatus::Malformed);
        if(detail::u16(data.data()+at-2)!=7)return fail(OriginalFigureStatus::Unsupported);
        std::array<uint8_t,408> receiver{};
        // Reuse the existing source-cited Figure7 wire widths, including enums
        // which consume TWO wire bytes but occupy ONE receiver byte.
        for(const auto& field:emperor::file_io::research::figure7_fields){
            if(!take(field.bytes))return fail(OriginalFigureStatus::Malformed);
            const bool narrow=std::string(field.helper)=="0x503db0";
            std::copy_n(data.data()+at-field.bytes,narrow?1:field.bytes,receiver.data()+field.receiver_offset);
        }
        if(!take(1))return fail(OriginalFigureStatus::Malformed);
        if(data[at-1])return fail(OriginalFigureStatus::Unsupported); //0x4C8AC7/DC ->0x4D72F0, variable FSA.
        if(kind==Class::Emissary){ //0x488C20: own2, Figure7, u32 length, bytes.
            if(!take(4))return fail(OriginalFigureStatus::Malformed);
            const uint32_t n=detail::u32(data.data()+at-4);
            if(n>8192)return fail(OriginalFigureStatus::BudgetExceeded);
            if(!take(n))return fail(OriginalFigureStatus::Malformed);
        }
        const auto* r=receiver.data();OriginalFigure f;f.slot=slot;f.recordOffset=begin;
        f.image=detail::u32(r+8);f.secondaryImage=detail::u32(r+404);f.next=detail::u16(r+16);
        f.type=r[18];f.frame=r[5];f.action=r[64];f.progress=r[65];f.alive=r[22];
        f.direction=r[25];f.previousDirection=r[26];f.x=detail::i16(r+28);f.y=detail::i16(r+30);f.cell=detail::u32(r+40);
        f.subX=detail::i16(r+82);f.subY=detail::i16(r+84);f.extraX=detail::i16(r+118);f.extraY=detail::i16(r+120);
        f.freeMovement=r[20];f.raise=r[36];f.drawClass=r[37]!=0;f.hidden=r[111];f.state85=r[133];f.height=r[134];
        objects.emplace(objectPID,f);if(f.alive){out.live.push_back(f);++out.liveTypes[f.type];}++out.parsed;
    }
    out.end=at;out.status=OriginalFigureStatus::Complete;return out;
}
inline const char* originalFigureArchiveName(uint32_t slot){
    // Startup0x475CA3 (14),0x475D44/4F/5A (38/39/40).
    switch(slot){case 14:return "SprAmbient.sg3";case 38:return "SprMain.sg3";
        case 39:return "SprMain2.sg3";case 40:return "SprMain3.sg3";default:return nullptr;}
}
// 0x5CFDF0: groups contain frame-major, interleaved direction records.
// Caller-supplied counts are SG metadata; this is not a type/action selector.
inline std::optional<uint32_t> originalFigureFrameIndex(uint32_t base,unsigned frames,unsigned directions,unsigned frame,unsigned direction){
    if(!base||!frames||frames>255||!directions||directions>8||frame>=frames||direction>=directions||
       uint64_t(base)+uint64_t(frames)*directions>UINT32_MAX)return std::nullopt;
    return base+frame*directions+direction;
}
//0x46BA96..0x46BC1A /0x469690. Tables0x81E8D0/EA8C and
//0x81EFC0/F068 are represented algebraically, not copied table listings.
inline std::optional<std::pair<int32_t,int32_t>> originalFigureSubcell(const OriginalFigure& f){
    if(f.direction>127||f.previousDirection>127||f.type==1||f.type==9)return std::nullopt;
    unsigned direction=f.direction;if(direction>7)direction=f.previousDirection>7?0:f.previousDirection;
    if(f.freeMovement){
        if(f.subX<0||f.subY<0)return std::nullopt;
        const int x=f.subX%20,y=f.subY%20;
        if(f.type==1||f.type==54)return std::nullopt; // special extra offset branches.
        return std::make_pair(2*(x-y),x+y-int(f.raise));
    }
    if(f.progress>20)return std::nullopt;
    const int step=f.progress<10?int(f.progress)+1:f.progress<20?int(f.progress)-19:0;
    static constexpr int8_t dx[8]={2,4,2,0,-2,-4,-2,0},dy[8]={-1,0,1,2,1,0,-1,-2};
    return std::make_pair(dx[direction]*step,dy[direction]*step-int(f.height));
}
} // namespace game_lab
