#pragma once
#include "original_building.hpp"
#include "../file_io/include/city19_figure7_layout.hpp"
#include "../file_io/include/city19_goods1_layout.hpp"
#include "../file_io/include/city19_empire2_layout.hpp"
#include "../file_io/include/city19_empire_grid15_layout.hpp"

namespace game_lab {
enum class OriginalFigureClass {Figure,Pheasant,Emissary};
// Owned executable/hash and every field/branch: docs/SAV_FIGURES.md.
// These are persisted draw inputs; no original factories or simulation execute.
struct OriginalFigure {
    size_t slot=0,recordOffset=0;
    uint32_t image=0,secondaryImage=0,cell=0;
    int16_t x=0,y=0,subX=0,subY=0,extraX=0,extraY=0;
    uint16_t next=0;
    uint8_t type=0,alive=0,frame=0,action=0,progress=0,direction=0,previousDirection=0;
    uint8_t freeMovement=0,height=0,hidden=0,drawClass=0,state85=0,raise=0;
    // Non-image selector inputs: Figure7 calls cited in docs/SAV_FIGURES.md.
    uint8_t subtype=0,attackDirection=0,attackFrame=0,resource=0,cartStyle=0,stationary=0;
    int16_t counter=0,owner=0,variant=0,amount=0,leaderLink=0,target=0;
    uint8_t faction=0;
    // Inline ambient state at+17C, serialized by0x4C8AA0 (not optional FSA).
    uint8_t effectKind=0,effectVariant=0,effectFlags=0;
    int8_t effectX=0,effectY=0,effectVelocityX=0,effectVelocityY=0;
    int16_t effectHeight=0,effectVelocityZ=0;
    uint8_t effectDraw=0; //+84, Figure7 request0x4C877F.
    uint32_t updateOwner=0; //+13C read0x4C8995;nonzero skips virtual+28 at0x4E2899.
    // Pheasant virtual predicates/cooldowns, Figure7's cited dword requests.
    uint32_t threat=0,threatDistance=0,herdTarget=0,herdDistance=0,idleCounter=0;
    std::array<int32_t,3> cooldowns{}; //+D0/+D4/+D8; positive values decrement.
    uint32_t linkedBuilding=0;
    uint8_t movementMode=0,attackerCount=0;
    int16_t route=0,routeCursor=0,routeLength=0,destinationX=0,destinationY=0;
    uint32_t destinationCell=0,age=0,priorProgress=0;
    int16_t reciprocalTarget=0,follower=0;
    uint8_t routePhase=0,speed=0;
    int16_t priorX=0,priorY=0,homeX=0,homeY=0,home=0,delivery=0;
    uint32_t priorCell=0;
    uint8_t water=0; //+49, written by0x4E77C0.
    int16_t roamLimit=0,roamSteps=0; //+4A/+4C, model15*96 and movement accounting.
    uint8_t roamState=0,roamTurns=0,roamTurn=0,roamCountdown=0,searchCounter=0;
    int16_t roamOriginX=0,roamOriginY=0;
    int16_t freeTargetX=0,freeTargetY=0,freeRemainingX=0,freeRemainingY=0,freeError=0;
    uint8_t freeAxis=0,cellMovement=0;
    std::array<int16_t,2> attackers{};
    OriginalFigureClass runtimeClass=OriginalFigureClass::Figure;
};
namespace original_save19 {
// Continue the shared SAV19 cursor after0x53147B. Every term is a read
// request, not a runtime-address delta: five u32s, two144-byte blocks,
// one u32,64000 bytes,three u32s,10 bytes,two80-byte blocks.
inline constexpr size_t fireScalars=g101D0B0+4; //0x531489..0x5314C1
inline constexpr size_t fireBlocks144=fireScalars+5*4; //0x5314D2/4E3
inline constexpr size_t fireBlock64000=fireBlocks144+2*144+4; //0x5314F1 then0x531502
inline constexpr size_t fireTrailingScalars=fireBlock64000+64000; //0x531510/51E/52C
inline constexpr size_t fireBlock10=fireTrailingScalars+3*4; //0x53153A
inline constexpr size_t fireBlocks80=fireBlock10+10; //0x531548/556
inline constexpr size_t fireCount=fireBlocks80+2*80; //0xC82F28,i32,0x531564
}
struct OriginalFigureSavedServices {
    std::array<int16_t,1000> routeOwners{};
    std::vector<uint8_t> directions;
    uint32_t randomFirst=0,randomSecond=0;
    std::optional<int32_t> fireCount;
};
// SAV19 terrain-relative framing:0x5313D1 owners2000 bytes,0x5313E2
// directions500000 bytes,100 version6 records of175 wire bytes, then
//0x531451/45F two RNG registers. T is the caller's
// independently validated terrain anchor, not a byte-pattern search here.
inline std::optional<OriginalFigureSavedServices> originalFigureSavedServices(
    const std::vector<uint8_t>& data,size_t terrain){
    using namespace original_save19;
    if(data.size()>8u*1024u*1024u||terrain>data.size()||randomSecond+4>data.size()-terrain)return std::nullopt;
    for(size_t n=0;n<recordCount;++n)if(detail::u16(data.data()+terrain+records+recordBytes*n)!=6)return std::nullopt;
    OriginalFigureSavedServices out;
    for(size_t n=0;n<1000;++n)out.routeOwners[n]=detail::i16(data.data()+terrain+routeOwners+2*n);
    out.directions.assign(data.begin()+terrain+routeDirections,data.begin()+terrain+routeDirections+500000);
    out.randomFirst=detail::u32(data.data()+terrain+randomFirst);out.randomSecond=detail::u32(data.data()+terrain+randomSecond);
    if(fireCount+4<=data.size()-terrain){const auto bits=detail::u32(data.data()+terrain+fireCount);
        out.fireCount=int32_t(bits>INT32_MAX?int64_t(bits)-4294967296LL:int64_t(bits));}
    return out;
}
struct OriginalFigureSpawnServices {
    uint32_t side=0,cropBase=0;
    std::array<std::pair<int16_t,int16_t>,4> spawns{};
};
struct OriginalFigureHunterLink {uint8_t alive=0,enabled=0;int16_t workers=0;};
struct OriginalFigureBuildingLink {
    uint16_t type=0;uint8_t alive=0;int16_t worker=0;uint8_t accepting=0;
    int16_t primaryFigure=0;uint8_t destinationEnabled=0;
    int16_t serviceCount=0;
    std::array<int16_t,8> cargoFigures{};
    std::array<int16_t,2> secondaryFigures{};
    std::optional<int32_t> marketGoods;
    int16_t x=0,y=0;
    bool inspectorSecondary=false;
};
// Common5:0x427430 reads live+4,type+14,worker+32,accepting+9 individually;
// wire+2/+16/+45/+7 include the version. Read only framed public records.
inline std::optional<std::map<size_t,OriginalFigureBuildingLink>> originalFigureBuildingLinks(
    const std::vector<uint8_t>& data,const OriginalBuildingProfile& buildings){
    if(data.size()>8u*1024u*1024u||buildings.status!=OriginalBuildingStatus::Complete||buildings.parsedRecords!=4000)return std::nullopt;
    std::map<size_t,size_t> records;
    for(const auto& b:buildings.footprints)records.emplace(b.slot,b.recordOffset);
    for(const auto& b:buildings.objectBuildings)records.emplace(b.slot,b.recordOffset);
    std::map<size_t,OriginalFigureBuildingLink> out;
    for(const auto& b:records){size_t at=b.second;
        if(at>data.size()||data.size()-at<2)return std::nullopt;
        const auto tag=detail::u16(data.data()+at);at+=2;
        if(tag==0xffff){if(data.size()-at<4)return std::nullopt;const size_t n=detail::u16(data.data()+at+2);
            if(n>data.size()-at-4)return std::nullopt;at+=4+n;}
        else if(tag<0x8000)continue; // Object aliases need their framed source; never guess a body.
        if(data.size()-at<182||detail::u16(data.data()+at)!=5)return std::nullopt;
        const auto type=detail::u16(data.data()+at+16);
        if(b.first>=4000||type!=buildings.slotTypes[b.first])return std::nullopt;
        OriginalFigureBuildingLink link;link.type=type;link.alive=data[at+2];link.worker=detail::i16(data.data()+at+45);link.accepting=data[at+7];
        link.primaryFigure=detail::i16(data.data()+at+41);link.destinationEnabled=data[at+129];link.serviceCount=detail::i16(data.data()+at+23);
        link.x=detail::i16(data.data()+at+8);link.y=detail::i16(data.data()+at+10);
        link.inspectorSecondary=detail::i16(data.data()+at+18)>0||data[at+100]!=0;
        const size_t tail=at+182;
        if((type==0x3a||type==0x3b||type==0x7c)&&data.size()-at>=312&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1){
            for(unsigned n=0;n<2;++n)link.secondaryFigures[n]=detail::i16(data.data()+tail+106+2*n);
            for(unsigned n=0;n<8;++n)link.cargoFigures[n]=detail::i16(data.data()+tail+112+2*n);
            if(type==0x3b&&data.size()-tail>=218&&detail::u16(data.data()+tail+130)==2&&detail::u16(data.data()+tail+216)==6){
                int64_t sum=0;bool framed=true;for(unsigned n=0;n<6;++n){const size_t p=tail+132+14*n;
                    framed&=detail::u16(data.data()+p)==1;const auto amount=detail::u32(data.data()+p+6);
                    sum+=amount>INT32_MAX?int64_t(amount)-4294967296LL:amount;}
                if(framed&&sum>=INT32_MIN&&sum<=INT32_MAX)link.marketGoods=int32_t(sum);
            }
        }
        out.emplace(b.first,link);
    }
    return out;
}
// Read the already framed Hunter records without changing the building reader.
//0x5F36E0->0x5F1240->0x51CE00->0x51C3A0;embedded data+5C is tail+92.
inline std::optional<std::map<size_t,OriginalFigureHunterLink>> originalFigureHunterLinks(
    const std::vector<uint8_t>& data,const OriginalBuildingProfile& buildings){
    if(data.size()>8u*1024u*1024u||buildings.status!=OriginalBuildingStatus::Complete||buildings.parsedRecords!=4000)return std::nullopt;
    std::map<size_t,OriginalFigureHunterLink> out;
    for(const auto& f:buildings.footprints)if(f.type==0x21){
        size_t at=f.recordOffset;
        if(at>data.size()||data.size()-at<2)return std::nullopt;
        const auto tag=detail::u16(data.data()+at);at+=2;
        if(tag==0xffff){if(data.size()-at<4)return std::nullopt;const size_t n=detail::u16(data.data()+at+2);
            if(n>data.size()-at-4)return std::nullopt;at+=4+n;}
        else if(tag<0x8000)return std::nullopt;
        if(data.size()-at<182+134||detail::u16(data.data()+at)!=5||detail::u16(data.data()+at+16)!=0x21||
            detail::u16(data.data()+at+182)!=1||detail::u16(data.data()+at+184)!=1||
            detail::u16(data.data()+at+312)!=1||detail::u16(data.data()+at+314)!=1)return std::nullopt;
        out.emplace(f.slot,OriginalFigureHunterLink{data[at+2],data[at+182+92],detail::i16(data.data()+at+62)});
    }
    return out;
}
//0x5312CB->0x52CD90 scenario6: individual wire requests, not runtime deltas.
// The caller supplies its independently validated city-size anchor.
inline std::optional<OriginalFigureSpawnServices> originalFigureSpawnServices(
    const std::vector<uint8_t>& data,size_t sizeAnchor){
    if(data.size()>8u*1024u*1024u||sizeAnchor<28||sizeAnchor>data.size())return std::nullopt;
    const size_t at=sizeAnchor-28;
    if(data.size()-at<1461||detail::u16(data.data()+at)!=6)return std::nullopt;
    OriginalFigureSpawnServices out;out.side=detail::u32(data.data()+sizeAnchor);
    out.cropBase=detail::u32(data.data()+sizeAnchor+8);
    if(!out.side||out.side>228||detail::u32(data.data()+sizeAnchor+4)!=out.side||
        out.cropBase!=((228-out.side)/2)*229)return std::nullopt;
    // Type76 is neither regional predator nor prey;0x59CB70 returns category0
    // for all three region rows.0x59C930 reads dword arrays through signed words.
    for(size_t n=0;n<4;++n)out.spawns[n]={detail::i16(data.data()+at+853+4*n),detail::i16(data.data()+at+869+4*n)};
    return out;
}
enum class OriginalFigureStatus {Complete,Unsupported,Malformed,BudgetExceeded};
struct OriginalFigureProfile {
    OriginalFigureStatus status=OriginalFigureStatus::Unsupported;
    size_t start=0,end=0,declared=0,parsed=0;
    uint32_t savedView=0;
    std::vector<OriginalFigure> live;
    std::vector<OriginalFigure> slots; // Includes inactive slots read by the animal-vector ordinal path.
    std::map<uint8_t,size_t> liveTypes;
    std::vector<uint16_t> cellHeads; //0xFAA130, SAV19 T+207936, read0x531338.
    std::optional<std::array<int16_t,22>> factionModels;
};
// SAV19 calls0x506710 then0x480800 immediately after the Figure list.
// Only the complete cited Goods1/Empire3/Grid15/child5 layout publishes models;
// an unknown following layout leaves the already parsed figures usable.
inline std::optional<std::array<int16_t,22>> originalFigureFactionModels(
    const std::vector<uint8_t>& data,size_t at,size_t limit){
    using namespace emperor::file_io::research;
    if(data.size()>8u*1024u*1024u||at>limit||limit>data.size())return std::nullopt;
    const auto take=[&](size_t n){if(n>limit-at)return false;at+=n;return true;};
    const auto version=[&](uint16_t v){return take(2)&&detail::u16(data.data()+at-2)==v;};
    if(!version(1)||!take(4))return std::nullopt;
    const auto goods=detail::u32(data.data()+at-4);if(goods>8192)return std::nullopt;
    for(size_t n=0;n<goods;++n){if(!version(1))return std::nullopt;
        for(const auto& f:goods1_fields)if(!take(f.bytes))return std::nullopt;}
    if(!version(3)||!take(4))return std::nullopt;
    const auto empire=detail::u32(data.data()+at-4);if(empire>8192)return std::nullopt;
    for(size_t n=0;n<empire;++n){if(!version(2))return std::nullopt;
        for(const auto& f:empire2_fields)if(!take(f.bytes))return std::nullopt;}
    std::array<int16_t,22> models{};
    for(auto& model:models){
        if(!version(15))return std::nullopt;
        for(const auto& f:empire_grid15_pre){if(!take(f.bytes))return std::nullopt;
            if(f.receiver_offset==0x4ae)model=detail::i16(data.data()+at-f.bytes);}
        for(size_t child=0;child<22;++child){if(!version(5))return std::nullopt;
            for(const auto& f:empire_grid_child5_fields)if(!take(f.bytes))return std::nullopt;}
        for(const auto& f:empire_grid15_post)if(!take(f.bytes))return std::nullopt;
    }
    if(!take(4))return std::nullopt; //Empire3 trailing parent word0x480960
    return models;
}
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
    using Class=OriginalFigureClass;
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
            out.slots.push_back(f);if(f.alive){out.live.push_back(f);++out.liveTypes[f.type];}++out.parsed;continue;
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
        const auto* r=receiver.data();OriginalFigure f;f.slot=slot;f.recordOffset=begin;f.runtimeClass=kind;
        f.image=detail::u32(r+8);f.secondaryImage=detail::u32(r+404);f.next=detail::u16(r+16);
        f.type=r[18];f.frame=r[5];f.action=r[64];f.progress=r[65];f.alive=r[22];
        f.direction=r[25];f.previousDirection=r[26];f.x=detail::i16(r+28);f.y=detail::i16(r+30);f.cell=detail::u32(r+40);
        f.subX=detail::i16(r+82);f.subY=detail::i16(r+84);f.extraX=detail::i16(r+118);f.extraY=detail::i16(r+120);
        f.freeMovement=r[20];f.raise=r[36];f.drawClass=r[37]!=0;f.hidden=r[111];f.state85=r[133];f.height=r[134];
        f.subtype=r[19];f.attackDirection=r[27];f.attackFrame=r[116];f.resource=r[136];
        f.cartStyle=r[135];f.stationary=r[137];
        f.counter=detail::i16(r+62);f.owner=detail::i16(r+98);f.variant=detail::i16(r+304);f.amount=detail::i16(r+130);
        f.leaderLink=detail::i16(r+114);f.target=detail::i16(r+144);f.faction=r[122];
        f.effectKind=r[380];f.effectVariant=r[381];f.effectFlags=r[382];
        f.effectX=int8_t(r[383]);f.effectY=int8_t(r[384]);
        f.effectHeight=detail::i16(r+386);f.effectVelocityX=int8_t(r[388]);f.effectVelocityY=int8_t(r[389]);
        f.effectVelocityZ=detail::i16(r+390);f.effectDraw=r[132];
        f.updateOwner=detail::u32(r+316);
        f.threat=detail::u32(r+172);f.threatDistance=detail::u32(r+176);
        f.herdTarget=detail::u32(r+188);f.herdDistance=detail::u32(r+192);f.idleCounter=detail::u32(r+204);
        for(size_t k=0;k<3;++k){const auto bits=detail::u32(r+208+4*k);
            f.cooldowns[k]=bits>INT32_MAX?int32_t(int64_t(bits)-4294967296LL):int32_t(bits);}
        f.linkedBuilding=detail::u32(r+328);f.movementMode=r[128];f.attackerCount=r[153];
        f.route=detail::i16(r+66);f.routeCursor=detail::i16(r+68);f.routeLength=detail::i16(r+70);
        f.destinationX=detail::i16(r+44);f.destinationY=detail::i16(r+46);f.destinationCell=detail::u32(r+48);
        f.routePhase=r[368];f.speed=r[97];f.age=detail::u32(r+276);f.priorProgress=detail::u32(r+400);
        f.reciprocalTarget=detail::i16(r+146);f.follower=detail::i16(r+102);
        f.priorX=detail::i16(r+32);f.priorY=detail::i16(r+34);f.priorCell=detail::u32(r+300);
        f.homeX=detail::i16(r+52);f.homeY=detail::i16(r+54);f.home=detail::i16(r+100);f.delivery=detail::i16(r+104);f.water=r[73];
        f.roamLimit=detail::i16(r+74);f.roamSteps=detail::i16(r+76);f.roamState=r[78];f.roamTurns=r[79];
        f.roamTurn=r[80];f.roamCountdown=r[81];f.searchCounter=r[117];
        f.roamOriginX=detail::i16(r+348);f.roamOriginY=detail::i16(r+350);
        f.freeTargetX=detail::i16(r+86);f.freeTargetY=detail::i16(r+88);f.freeRemainingX=detail::i16(r+90);
        f.freeRemainingY=detail::i16(r+92);f.freeError=detail::i16(r+94);f.freeAxis=r[96];f.cellMovement=r[72];
        f.attackers={detail::i16(r+154),detail::i16(r+156)};
        objects.emplace(objectPID,f);out.slots.push_back(f);if(f.alive){out.live.push_back(f);++out.liveTypes[f.type];}++out.parsed;
    }
    out.end=at;out.status=OriginalFigureStatus::Complete;
    out.factionModels=originalFigureFactionModels(data,at,limit);return out;
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
