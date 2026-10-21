#pragma once
#include "original_building.hpp"
#include "../file_io/include/city19_figure7_layout.hpp"
#include "../file_io/include/city19_goods1_layout.hpp"
#include "../file_io/include/city19_empire2_layout.hpp"
#include "../file_io/include/city19_empire_grid15_layout.hpp"
#include "../file_io/include/city19_later7_layout.hpp"
#include <sstream>

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
    uint8_t faction=0,employmentAmount=0; //+6E, read0x4C86DC.
    uint8_t watchCoverage=0; //+70,4C86F2;5B3E70 writes the figure, not its owner.
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
    uint32_t flags120=0; //4C891C, cleanup target-reference gate.
    uint8_t water=0; //+49, written by0x4E77C0.
    int16_t roamLimit=0,roamSteps=0; //+4A/+4C, model15*96 and movement accounting.
    uint8_t roamState=0,roamTurns=0,roamTurn=0,roamCountdown=0,searchCounter=0;
    int16_t roamOriginX=0,roamOriginY=0;
    int16_t freeTargetX=0,freeTargetY=0,freeRemainingX=0,freeRemainingY=0,freeError=0;
    uint8_t freeAxis=0,cellMovement=0;
    uint8_t previousAction=0; //+18,Hunter4D31DE preserves prey action before18.
    uint16_t allocationSerial=0; //+94,4C9187.
    uint8_t allocationPhase=0,allocationPhaseCopy=0; //+8A/+8C,4C9204..212 RNG&3.
    bool projectile=false; //+129,5EA5AE;only type54 consumes it.
    int16_t projectileDistance=0,projectileStartHeight=0,projectileTargetHeight=0,projectileEndHeight=0; //+AC/+AE/+B0/+B2.
    std::array<int16_t,2> attackers{};
    uint32_t damage=0; //+A8,Figure7 request4C88BD;4E5A60 adds modulo32.
    uint8_t projectileHits=0; //+A0,5E63C6 increments modulo8.
    int16_t formation=0; //+6A,Figure7 request4C86BB;combat formation modifiers.
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
inline constexpr size_t criminalCount=fireCount+5*4+8960+8800+4+1000+1000+8000+8; //10AEECC,531629.
// Continue from road-distance0x5317E3, then32 bytes0x5317F1 and36
// bytes0x531805. The preceding variable records are already framed at T.
inline constexpr size_t serviceGrid=1510985+51984+32+36; //0x531816,D62440
//531824/832/843/854/862/873/884/895/8A3/8B4/8C5/8D6/
//8E7/8F8/909/91A precede the269-byte10BC7E0 table at53192B.
inline constexpr size_t buildingDisabled=serviceGrid+207936+26+13+2*51984+4+2*51984+1436+4+224+3600+624+256+224+448+200;
}
struct OriginalFigureCropServices {
    int32_t month=0;
    std::array<int16_t,36> unavailable{}; //1312014, population7 +26B4.
    std::array<uint8_t,269> disabled{}; //10BC7E0,53192B.
};
inline std::optional<OriginalFigureCropServices> originalFigureSavedCropServices(
    const std::vector<uint8_t>& data,size_t terrain,size_t sizeAnchor){
    size_t bytes=2;for(const auto& f:emperor::file_io::research::later7_requests)bytes+=f.bytes;
    if(data.size()>8u*1024u*1024u||terrain>data.size()||original_save19::buildingDisabled+269>data.size()-terrain||
        sizeAnchor>data.size()||sizeAnchor<28+152+bytes)return std::nullopt;
    const size_t population=sizeAnchor-28-152-bytes;
    if(detail::u16(data.data()+population)!=7||detail::u16(data.data()+sizeAnchor-28)!=6)return std::nullopt;
    OriginalFigureCropServices out;const uint32_t month=detail::u32(data.data()+sizeAnchor-176); //5312AF,C82EEC.
    out.month=int32_t(month>INT32_MAX?int64_t(month)-4294967296LL:month);
    //5944BF reads72 bytes from population+26B4; preceding requests total9904.
    for(unsigned n=0;n<36;++n)out.unavailable[n]=detail::i16(data.data()+population+9904+2*n);
    std::copy_n(data.begin()+terrain+original_save19::buildingDisabled,269,out.disabled.begin());
    return out;
}
struct OriginalFigurePopulation {int32_t total=0,peak=0;std::optional<uint32_t> lastArrival;};
struct OriginalFigureStoragePolicy {
    std::array<uint8_t,36> accept{},trade{};
    std::array<int16_t,36> limit{};
};
struct OriginalFigureGoodsServices {
    std::vector<OriginalFigureStoragePolicy> policies;
    std::array<uint32_t,36> unavailable{},trade7{},trade5{},warehouse{};
    uint32_t savedPhase=0;
    // 1E4EE08 and10BC928 are persistent process caches, never SAV fields.
    std::optional<std::array<uint8_t,36>> storageSearch,industrySearch;
    std::optional<uint32_t> godGate; //Scenario6+2C0,C5CDA0;52D366..371 reads4.5350A5 is SAVE,not load.
    std::array<int32_t,10> industryPriority{}; //Population7 runtime1312148,ten20-byte category rows,first dword.
};
//5312CB Scenario6 starts at S-28:2 version bytes+1459 requested body,
//embedded52C480 own1+16 bytes,then5312D9 byte and5312E7 difficulty.
inline std::optional<unsigned> originalFigureSavedDifficulty(const std::vector<uint8_t>& data,size_t sizeAnchor){
    if(sizeAnchor<28||sizeAnchor>data.size()||data.size()-sizeAnchor<1456||detail::u16(data.data()+sizeAnchor-28)!=6||
        detail::u16(data.data()+sizeAnchor+1433)!=1)return std::nullopt;
    const auto value=detail::u32(data.data()+sizeAnchor+1452);if(value>4)return std::nullopt;return unsigned(value);
}
inline std::optional<OriginalFigureGoodsServices> originalFigureSavedGoodsServices(
    const std::vector<uint8_t>& data,size_t figuresEnd,size_t sizeAnchor,size_t terrain){
    size_t bytes=2;for(const auto& f:emperor::file_io::research::later7_requests)bytes+=f.bytes;
    if(data.size()>8u*1024u*1024u||figuresEnd>data.size()||data.size()-figuresEnd<6||sizeAnchor>data.size()||
        sizeAnchor<28+152+bytes||terrain>data.size()||original_save19::c82ef8+4>data.size()-terrain)return std::nullopt;
    const size_t population=sizeAnchor-28-152-bytes;
    if(detail::u16(data.data()+population)!=7||detail::u16(data.data()+figuresEnd)!=1)return std::nullopt;
    OriginalFigureGoodsServices out;size_t at=figuresEnd+6;const auto count=detail::u32(data.data()+figuresEnd+2);
    if(count>8192)return std::nullopt;
    for(size_t n=0;n<count;++n){if(at>data.size()||data.size()-at<270||detail::u16(data.data()+at)!=1)return std::nullopt;
        OriginalFigureStoragePolicy policy; //506B0A/58/78;explicit Goods1 field requests.
        std::copy_n(data.begin()+at+12,36,policy.accept.begin());
        for(unsigned r=0;r<36;++r)policy.limit[r]=detail::i16(data.data()+at+54+2*r);
        std::copy_n(data.begin()+at+198,36,policy.trade.begin());out.policies.push_back(policy);at+=270;}
    // Population7 reads594522/594868/59487B/59488E; runtime bases
    // 1312098/13123D0/131265C/13126EC. Unsigned bits retain x86 wrap.
    for(unsigned r=0;r<36;++r){out.unavailable[r]=detail::u32(data.data()+population+10034+4*r);
        out.trade7[r]=detail::u32(data.data()+population+10578+4*r);
        out.trade5[r]=detail::u32(data.data()+population+10722+4*r);
        out.warehouse[r]=detail::u32(data.data()+population+10866+4*r);}
    for(unsigned n=0;n<10;++n){const auto bits=detail::u32(data.data()+population+10194+20*n);
        out.industryPriority[n]=bits>INT32_MAX?int32_t(int64_t(bits)-4294967296LL):int32_t(bits);}
    const size_t scenario=sizeAnchor-28;
    if(data.size()-scenario<708||detail::u16(data.data()+scenario)!=6)return std::nullopt;
    out.godGate=detail::u32(data.data()+scenario+704); //2-byte archive version+702 preceding requested bytes.
    out.savedPhase=detail::u32(data.data()+terrain+original_save19::c82ef8);return out;
}
// Same ten bounded goal lists as SAV19's existing prefix walker, followed
// by Flood1 (4EDDA0) and Festival4 (48ED30). Active festival needs date
// qualification48E020; an inactive byte proves48ED00 returns null.
inline std::optional<int32_t> originalFigureSavedFestivalVenue(const std::vector<uint8_t>& data,size_t terrain,size_t sizeAnchor,
    std::optional<bool>* waterOverride=nullptr){
    if(waterOverride)waterOverride->reset();
    if(data.size()<3355||data.size()>8u*1024u*1024u)return std::nullopt;
    size_t at=3355;std::vector<std::string> cache(1);
    const auto take=[&](size_t n){if(at>data.size()||n>data.size()-at)return false;at+=n;return true;};
    const auto version=[&](unsigned v){return take(2)&&detail::u16(data.data()+at-2)==v;};
    for(unsigned member=0;member<10;++member){if(!version(1)||!take(4))return std::nullopt;const auto count=detail::u32(data.data()+at-4);
        if(count>128)return std::nullopt;
        for(unsigned n=0;n<count;++n){if(!take(2))return std::nullopt;const auto tag=detail::u16(data.data()+at-2);std::string name;
            if(tag==0xffff){if(!take(4))return std::nullopt;const auto len=detail::u16(data.data()+at-2);
                if(detail::u16(data.data()+at-4)||!len||len>=64||!take(len))return std::nullopt;
                name.assign(reinterpret_cast<const char*>(data.data()+at-len),len);cache.push_back(name);}
            else if(tag&0x8000){const auto id=tag&0x7fff;if(size_t(id)>=cache.size()||cache[id].empty())return std::nullopt;name=cache[id];}
            else {if(!tag||tag>=cache.size()||!cache[tag].empty())return std::nullopt;continue;}
            if(cache.size()>=256||!version(1)||!take(4)||!take(2))return std::nullopt;cache.emplace_back();
            const auto v=detail::u16(data.data()+at-2);size_t bytes=0;
            if(name=="cHousingGoal"||name=="cYearlyProductionGoal"){if(v!=1)return std::nullopt;bytes=12;}
            else if(name=="cTradingPartnersGoal"||name=="cPopulationGoal"||name=="cTreasuryGoal"){if(v!=1)return std::nullopt;bytes=4;}
            else if(name=="cHomageGoal"||name=="cAlliedCitiesGoal"||name=="cMenagerieGoal"){if(v!=3)return std::nullopt;bytes=8;}
            else if(name=="cYearlyProfitGoal"){if(v!=1)return std::nullopt;bytes=8;}
            else if(name=="cConquerGoal"||name=="cMonumentGoal"){if(v!=2)return std::nullopt;bytes=8;}else return std::nullopt;
            if(!take(bytes))return std::nullopt;}}
    if(!version(1)||!take(4+1+4+4+4+51984))return std::nullopt;
    const size_t festival=at;
    if(!version(4)||!version(1)||!take(16+32+8+1))return std::nullopt;
    const bool active=data[at-1]!=0;if(!take(1+4))return std::nullopt;const auto venue=detail::u32(data.data()+at-4);
    if(venue>3999||!take(1+4+4+4+4))return std::nullopt;
    //48ED30 restores+50/+54/+58 at wire66/67/71;48E110 tests enabled,3,4.
    if(waterOverride)*waterOverride=data[festival+66]&&detail::u32(data.data()+festival+67)==3&&detail::u32(data.data()+festival+71)==4;
    if(!active)return 0;
    if(terrain>data.size()||original_save19::c82ef0+4>data.size()-terrain||sizeAnchor<180+152||sizeAnchor>data.size())return std::nullopt;
    const auto i32=[&](size_t pos){const uint32_t v=detail::u32(data.data()+pos);return v>INT32_MAX?int64_t(v)-4294967296LL:int64_t(v);};
    // Following boundary gives the two date scalars after593140.
    const int64_t nowTick=i32(terrain+original_save19::c82ef8),nowPhase=i32(terrain+original_save19::c82ef0);
    int64_t nowMonth=i32(sizeAnchor-180+4),startMonth=i32(festival+12);
    if(!nowMonth)nowMonth=12;if(!startMonth)startMonth=12;
    const int64_t elapsed=192*(i32(sizeAnchor-180)-i32(festival+16))+nowPhase-i32(festival+8)-
        (nowTick<i32(festival+4))+16*(nowMonth-startMonth);
    return elapsed>i32(festival+52)?0:int32_t(venue);
}
struct OriginalFigureSavedServices {
    struct FormationImpact {int16_t alert=0,source=0;}; //runtime+5E/+60,5E63A2/A8.
    std::array<FormationImpact,100> formationImpacts{};
    std::array<int16_t,1000> routeOwners{};
    std::vector<uint8_t> directions;
    uint32_t randomFirst=0,randomSecond=0;
    std::optional<int32_t> fireCount;
    std::optional<int32_t> criminalCount;
    std::optional<uint32_t> allocationSerial; //10AEEE8,531580,T+fireCount+8.
    std::vector<uint32_t> serviceGrid;
    std::optional<OriginalFigurePopulation> population;
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
    //550C54/550C61:the version6 request prefix is contiguous through+88.
    for(size_t n=0;n<recordCount;++n){const auto* r=data.data()+terrain+records+recordBytes*n;
        out.formationImpacts[n]={detail::i16(r+96),detail::i16(r+98)};}
    for(size_t n=0;n<1000;++n)out.routeOwners[n]=detail::i16(data.data()+terrain+routeOwners+2*n);
    out.directions.assign(data.begin()+terrain+routeDirections,data.begin()+terrain+routeDirections+500000);
    out.randomFirst=detail::u32(data.data()+terrain+randomFirst);out.randomSecond=detail::u32(data.data()+terrain+randomSecond);
    if(fireCount+4<=data.size()-terrain){const auto bits=detail::u32(data.data()+terrain+fireCount);
        out.fireCount=int32_t(bits>INT32_MAX?int64_t(bits)-4294967296LL:int64_t(bits));}
    if(criminalCount+4<=data.size()-terrain){const auto bits=detail::u32(data.data()+terrain+criminalCount);
        out.criminalCount=int32_t(bits>INT32_MAX?int64_t(bits)-4294967296LL:int64_t(bits));}
    if(fireCount+12<=data.size()-terrain)out.allocationSerial=detail::u32(data.data()+terrain+fireCount+8);
    if(serviceGrid+207936<=data.size()-terrain){out.serviceGrid.resize(51984);
        for(size_t n=0;n<51984;++n)out.serviceGrid[n]=detail::u32(data.data()+terrain+serviceGrid+4*n);}
    return out;
}
//0x531293->593140 precedes two u32s and144 bytes (53129F/2AF/2C0),
// then scenario6 (5312CB). Use the validated scenario start S-28 as the
// following boundary; qualify the complete version7 request extent backwards.
// Its wire+38 total and+11271 peak are read5941AB/594C68.
inline std::optional<OriginalFigurePopulation> originalFigureSavedPopulation(const std::vector<uint8_t>& data,size_t sizeAnchor){
    size_t bytes=2;for(const auto& f:emperor::file_io::research::later7_requests)bytes+=f.bytes;
    if(data.size()>8u*1024u*1024u||sizeAnchor>data.size()||sizeAnchor<28+152+bytes)return std::nullopt;
    const size_t at=sizeAnchor-28-152-bytes;
    if(detail::u16(data.data()+sizeAnchor-28)!=6||detail::u16(data.data()+at)!=7)return std::nullopt;
    const auto signed32=[](uint32_t bits){return int32_t(bits>INT32_MAX?int64_t(bits)-4294967296LL:bits);};
    return OriginalFigurePopulation{signed32(detail::u32(data.data()+at+38)),signed32(detail::u32(data.data()+at+11271)),{}};
}
//5D1830 reads five24-column HOUSE MODS then15 ALL HOUSES rows.
//44CC80(row,17) reads capacity. Difficulty adds then clamps[-99,100];
// publish only capacities identical across all five modifiers.
inline std::optional<std::map<int,int>> originalFigureHouseModelColumn(const std::string& text,unsigned column){
    if(column>=24)return std::nullopt;
    if(text.size()>1024u*1024u||text.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream input(text);std::string line;unsigned section=0;std::vector<int> mods;std::map<int,int> out;
    while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line=="HOUSE MODS"){if(section)return std::nullopt;section=1;continue;}
        if(line=="ALL HOUSES"){if(section!=1||mods.size()!=5)return std::nullopt;section=2;continue;}
        const auto open=line.find("{,");if(!section||open==std::string::npos)continue;
        std::string values=line.substr(open+2);std::replace(values.begin(),values.end(),',',' ');std::istringstream fields(values);
        std::array<int32_t,24> row{};for(auto& value:row)if(!(fields>>value))return std::nullopt;
        if(section==1){mods.push_back(row[column]);if(mods.size()>5)return std::nullopt;}
        else {const auto cap=[&](int m){return int(std::max<int64_t>(-99,std::min<int64_t>(100,int64_t(row[column])+m)));};
            const int c=cap(mods[0]);for(int m:mods)if(cap(m)!=c)return std::nullopt;out.emplace(int(out.size()),c);if(out.size()>15)return std::nullopt;}}
    if(section!=2||out.size()!=15)return std::nullopt;return out;
}
inline std::optional<std::map<int,int>> originalFigureHouseCapacities(const std::string& text){return originalFigureHouseModelColumn(text,17);}
//44CC50(type,9) reads the raw thirteen-column building row.
inline std::optional<int32_t> originalFigureWatchtowerReduction(const std::string& text,unsigned type=127){
    if(type>268)return std::nullopt;
    if(text.size()>1024u*1024u||text.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream input(text);std::string line;bool active=false;unsigned count=0;std::optional<int32_t> out;
    while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line=="ALL BUILDINGS"){if(active)return std::nullopt;active=true;continue;}
        if(active&&line=="HOUSE MODS")return count>type?out:std::nullopt;
        const auto open=line.find("{,");if(!active||open==std::string::npos)continue;
        std::string fields=line.substr(open+2);std::replace(fields.begin(),fields.end(),',',' ');std::istringstream values(fields);
        std::array<int32_t,13> row{};for(auto& v:row)if(!(values>>v))return std::nullopt;
        if(count++==type)out=row[9];}
    return std::nullopt;
}
struct OriginalFigureSpawnServices {
    uint32_t side=0,cropBase=0;
    std::array<std::pair<int16_t,int16_t>,4> spawns{};
    std::array<int16_t,3> entertainmentAvailable{};
};
struct OriginalFigureHunterLink {uint8_t alive=0,enabled=0;int16_t workers=0;};
struct OriginalFigureBuildingLink {
    uint16_t type=0;uint8_t alive=0;int16_t worker=0;uint8_t accepting=0;
    int16_t primaryFigure=0;uint8_t destinationEnabled=0;
    int16_t serviceCount=0;
    std::array<int16_t,8> cargoFigures{};
    std::array<int16_t,2> secondaryFigures{};
    int16_t hunterReturnGoods=0; //common+72,wire101;4D3408 adds50 on action10 arrival.
    std::optional<int32_t> marketGoods;
    std::optional<std::vector<OriginalStorageCompartment>> marketStorage;
    std::optional<std::array<int16_t,7>> marketDemand;
    std::optional<int32_t> marketQuality;
    int16_t x=0,y=0;uint32_t cell=0;
    bool inspectorSecondary=false;
    int16_t residents=0,freeSlots=0,houseModel=0;uint8_t size=0;
    std::optional<std::array<int16_t,12>> houseGoods;
    std::optional<uint8_t> foodQuality;
    std::optional<std::array<uint8_t,4>> religion;
    std::optional<uint32_t> shrineReligion;std::optional<uint8_t> shrineElite;
    std::optional<uint8_t> laborDisabled;
    std::optional<std::array<uint8_t,3>> entertainment; //house+2B/+2C/+2E; owner+5D/+5E/+5F.
    std::optional<uint8_t> herbalist; //house embedded+2D,517410 wire tail+44.
    std::optional<std::array<uint8_t,2>> waterService; //embedded+32/+34,517410 tail49/51.
    uint8_t upgrade=0; //common+6F,427430 wire99;Well+224=5B3AD0.
    int32_t watchRisk=0; //common+8C,427430; mutable5B3ED0.
    int16_t fireRisk=0,damageRisk=0; //common+4A/+4C, wire68/70.
    std::optional<size_t> inspectorParent;int16_t parent=0; //common+3C,wire54;4B3930.
    struct CropDelivery {uint16_t flags=0;uint32_t pending=0;};
    std::optional<CropDelivery> cropDelivery; //embedded+6A and outer+150.
    int16_t roadX=0,roadY=0;uint8_t roamDirection=0; //common+38,427E2C wire51;4E6A70 writes.
    std::optional<int32_t> suppliers;
    std::optional<uint8_t> delegatedCount; //Music data+5C,51C520;removal4C8BD1.
    std::optional<uint8_t> entertainmentRoad; //Entertainment data+69,48B950.
    bool cropSearchComplete=false;
    std::optional<std::vector<OriginalStorageCompartment>> storage;
    std::optional<size_t> storagePolicy;
    uint8_t outputResource=0,roadAccess=0,category=0,storageLaborDisabled=0;
    int16_t workers=0;
    int16_t roadDistance=0,suppress=0;
    int16_t incomingFigures=0,incomingAmount=0;
    int16_t traderFaction=0; //common+16,wire18;5D3510 station lookup.
    std::optional<size_t> currentTrader; //building+160:5DDBDC constructs null;station serializers do not restore it.
    int16_t industryInput=0; //common runtime+40,wire58;Kiln main input stock.
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
    //4C909E also addresses Building slot0 when an animal's+64 timer is
    //nonzero and its+62 owner is0. Preserve that serialized common+32 word;
    //slot0 has no live footprint and is therefore absent from records above.
    constexpr std::array<uint8_t,20> preamble{{1,0,0xa0,0x0f,0,0,0xff,0xff,0,0,8,0,'B','u','i','l','d','i','n','g'}};
    const auto first=std::search(data.begin(),data.end(),preamble.begin(),preamble.end());
    if(first!=data.end()){
        const size_t at=size_t(first-data.begin())+preamble.size();
        if(data.size()-at<182||detail::u16(data.data()+at)!=5||detail::u16(data.data()+at+16)!=0||
            std::search(first+1,data.end(),preamble.begin(),preamble.end())!=data.end())return std::nullopt;
        OriginalFigureBuildingLink zero;zero.worker=detail::i16(data.data()+at+45);out.emplace(0,zero);
    }
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
        link.cell=detail::u32(data.data()+at+12);
        link.size=data[at+5];link.upgrade=data[at+99];
        link.outputResource=data[at+65];link.roadAccess=data[at+66];link.roadDistance=detail::i16(data.data()+at+31);
        link.category=data[at+20];link.workers=detail::i16(data.data()+at+62);link.storageLaborDisabled=data[at+98];
        link.suppress=detail::i16(data.data()+at+72);
        link.hunterReturnGoods=detail::i16(data.data()+at+101);
        link.incomingFigures=detail::i16(data.data()+at+156);link.incomingAmount=detail::i16(data.data()+at+158);
        link.industryInput=detail::i16(data.data()+at+58);
        link.traderFaction=detail::i16(data.data()+at+18);
        if(type==0x38||type==0x3a)link.currentTrader=0; //5E1420->5DDBC0,5E1510->5DDC60->5D5BC0:load-reconstructed null.
        const int policy=detail::i16(data.data()+at+88);if(policy>=0)link.storagePolicy=size_t(policy);
        link.roadX=detail::i16(data.data()+at+37);link.roadY=detail::i16(data.data()+at+39);link.roamDirection=data[at+51];
        link.inspectorSecondary=detail::i16(data.data()+at+18)>0||data[at+100]!=0;
        link.fireRisk=detail::i16(data.data()+at+68);link.damageRisk=detail::i16(data.data()+at+70);
        link.parent=detail::i16(data.data()+at+54);
        link.residents=detail::i16(data.data()+at+27);link.freeSlots=detail::i16(data.data()+at+29);link.houseModel=detail::i16(data.data()+at+18);
        const auto risk=detail::u32(data.data()+at+125);link.watchRisk=int32_t(risk>INT32_MAX?int64_t(risk)-4294967296LL:risk);
        const size_t tail=at+182;
        if((type==0xd3||type==0x21)&&data.size()-tail>=130&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1)
            link.delegatedCount=data[tail+92];
        for(const auto& footprint:buildings.footprints)if(footprint.slot==b.first&&footprint.imageInputs&&footprint.imageInputs->compartments)
            link.storage=footprint.imageInputs->compartments;
        if(type==0x35&&data.size()-tail>=288&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1&&detail::u16(data.data()+tail+130)==2){
            std::vector<OriginalStorageCompartment> storage;bool framed=true;
            for(unsigned n=0;n<8;++n){const size_t pos=tail+132+14*n;framed&=detail::u16(data.data()+pos)==1;
                storage.push_back({detail::u32(data.data()+pos+2),detail::u32(data.data()+pos+6),detail::u32(data.data()+pos+10)});}
            if(framed)link.storage=std::move(storage);}
        if((type==0xc2||type==0xc4)&&data.size()-tail>=140&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1&&
            detail::u16(data.data()+tail+130)==1&&detail::u16(data.data()+tail+132)==1&&detail::u16(data.data()+tail+138)==1)
            link.cropDelivery=OriginalFigureBuildingLink::CropDelivery{detail::u16(data.data()+tail+106),detail::u32(data.data()+tail+134)};
        if((type==0xc0||type==0xc1)&&data.size()-tail>=142&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1&&
            detail::u16(data.data()+tail+130)==1&&detail::u16(data.data()+tail+140)==1){
            const auto n=detail::u32(data.data()+tail+136);link.suppliers=int32_t(n>INT32_MAX?int64_t(n)-4294967296LL:n);}
        if(type>=2&&type<18&&data.size()-tail>=64&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1){
            link.laborDisabled=data[tail+59];link.entertainment=std::array<uint8_t,3>{data[tail+42],data[tail+43],data[tail+45]};link.herbalist=data[tail+44];
            link.houseGoods.emplace();for(unsigned n=0;n<12;++n)link.houseGoods->at(n)=detail::i16(data.data()+tail+17+2*n);
            link.foodQuality=data[tail+53];link.waterService=std::array<uint8_t,2>{data[tail+49],data[tail+51]};}
        if(type>=2&&type<18&&link.houseGoods)link.religion=std::array<uint8_t,4>{data[tail+13],data[tail+14],data[tail+15],data[tail+16]};
        if(type==0xd6&&data.size()-tail>=165&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1&&
            detail::u16(data.data()+tail+130)==1&&detail::u16(data.data()+tail+132)==1&&detail::u16(data.data()+tail+134)==1){
            link.shrineReligion=detail::u32(data.data()+tail+152);link.shrineElite=data[tail+164];}
        if(type==0x47&&data.size()-tail>=352&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1)
            {link.entertainment=std::array<uint8_t,3>{data[tail+93],data[tail+94],data[tail+95]};link.inspectorParent=detail::u32(data.data()+tail+348);
             link.entertainmentRoad=data[tail+105];}
        if((type==0x36||type==0x3a||type==0x3b||type==0x47||type==0x7c)&&data.size()-at>=312&&detail::u16(data.data()+tail)==1&&detail::u16(data.data()+tail+2)==1){
            for(unsigned n=0;n<2;++n)link.secondaryFigures[n]=detail::i16(data.data()+tail+106+2*n);
            for(unsigned n=0;n<8;++n)link.cargoFigures[n]=detail::i16(data.data()+tail+112+2*n);
            if(type==0x3b&&data.size()-tail>=266&&detail::u16(data.data()+tail+130)==2&&detail::u16(data.data()+tail+216)==6){
                int64_t sum=0;bool framed=true;std::vector<OriginalStorageCompartment> storage;
                for(unsigned n=0;n<6;++n){const size_t p=tail+132+14*n;
                    framed&=detail::u16(data.data()+p)==1;const auto amount=detail::u32(data.data()+p+6);
                    storage.push_back({detail::u32(data.data()+p+2),amount,detail::u32(data.data()+p+10)});
                    sum+=amount>INT32_MAX?int64_t(amount)-4294967296LL:amount;}
                if(framed&&sum>=INT32_MIN&&sum<=INT32_MAX){link.marketGoods=int32_t(sum);link.marketStorage=std::move(storage);
                    link.marketDemand.emplace();for(unsigned n=0;n<7;++n)link.marketDemand->at(n)=detail::i16(data.data()+tail+78+2*n);
                    const auto quality=detail::u32(data.data()+tail+242);link.marketQuality=int32_t(quality>INT32_MAX?int64_t(quality)-4294967296LL:quality);}
            }
        }
        out.emplace(b.first,link);
    }
    bool complete=true;for(size_t slot=1;slot<4000;++slot)
        if(buildings.slotTypes[slot]>=0xc2&&buildings.slotTypes[slot]<=0xc7&&!out.count(slot))complete=false;
    for(auto& e:out)if(e.second.type==0xc0||e.second.type==0xc1)e.second.cropSearchComplete=complete;
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
    for(size_t n=0;n<3;++n)out.entertainmentAvailable[n]=detail::i16(data.data()+at+919+2*n); //37C block+22,52D521
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
    struct EmpireTraderSlots {std::array<int16_t,3> figures{};int16_t active=0;};
    std::optional<std::array<EmpireTraderSlots,22>> empireTraders; //Grid15 runtime+2C..30,+4C;43E7F1/43E880.
};
// SAV19 calls0x506710 then0x480800 immediately after the Figure list.
// Only the complete cited Goods1/Empire3/Grid15/child5 layout publishes models;
// an unknown following layout leaves the already parsed figures usable.
inline std::optional<std::array<int16_t,22>> originalFigureFactionModels(
    const std::vector<uint8_t>& data,size_t at,size_t limit,
    std::optional<std::array<OriginalFigureProfile::EmpireTraderSlots,22>>* traders=nullptr){
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
    std::array<int16_t,22> models{};std::array<OriginalFigureProfile::EmpireTraderSlots,22> traderSlots{};
    for(size_t faction=0;faction<models.size();++faction){auto& model=models[faction];
        if(!version(15))return std::nullopt;
        for(const auto& f:empire_grid15_pre){if(!take(f.bytes))return std::nullopt;
            if(f.receiver_offset==0x4ae)model=detail::i16(data.data()+at-f.bytes);
            if(f.receiver_offset==44)for(unsigned k=0;k<3;++k)traderSlots[faction].figures[k]=detail::i16(data.data()+at-f.bytes+2*k);
            if(f.receiver_offset==76)traderSlots[faction].active=detail::i16(data.data()+at-f.bytes);}
        for(size_t child=0;child<22;++child){if(!version(5))return std::nullopt;
            for(const auto& f:empire_grid_child5_fields)if(!take(f.bytes))return std::nullopt;}
        for(const auto& f:empire_grid15_post)if(!take(f.bytes))return std::nullopt;
    }
    if(!take(4))return std::nullopt; //Empire3 trailing parent word0x480960
    if(traders)*traders=traderSlots;return models;
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
        f.damage=detail::u32(r+168);f.projectileHits=r[160];
        f.formation=detail::i16(r+106);
        f.employmentAmount=r[110];f.watchCoverage=r[112];
        f.flags120=detail::u32(r+288);
        f.previousAction=r[24];f.allocationSerial=detail::u16(r+148);f.projectile=r[297]!=0;
        f.allocationPhase=r[138];f.allocationPhaseCopy=r[140];
        f.projectileDistance=detail::i16(r+172);f.projectileStartHeight=detail::i16(r+174);
        f.projectileTargetHeight=detail::i16(r+176);f.projectileEndHeight=detail::i16(r+178);
        objects.emplace(objectPID,f);out.slots.push_back(f);if(f.alive){out.live.push_back(f);++out.liveTypes[f.type];}++out.parsed;
    }
    out.end=at;out.status=OriginalFigureStatus::Complete;
    out.factionModels=originalFigureFactionModels(data,at,limit,&out.empireTraders);return out;
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
    if(f.direction>127||f.previousDirection>127||f.type==9)return std::nullopt;
    if(f.type==1&&(f.effectKind!=13||f.effectVariant||f.effectFlags!=0x24||f.secondaryImage))return std::nullopt;
    unsigned direction=f.direction;if(direction>7)direction=f.previousDirection>7?0:f.previousDirection;
    if(f.freeMovement){
        if(f.subX<0||f.subY<0)return std::nullopt;
        const int x=f.subX%20,y=f.subY%20;
        if(f.type==54)return std::make_pair(2*(x-y),x+y-int(f.raise)-int(f.height)); //46BBC1.
        //0x46BBCB..0x46BBE3: type1 adds +76/+78 and subtracts +86
        // AFTER the free-position table and +24 raise. Inline kind13's
        // updater supplies +52/+54/+24; drawing does not advance it.
        if(f.type==1)return std::make_pair(2*(x-y)+f.extraX,x+y-int(f.raise)+f.extraY-int(f.height));
        return std::make_pair(2*(x-y),x+y-int(f.raise));
    }
    if(f.progress>20)return std::nullopt;
    const int step=f.progress<10?int(f.progress)+1:f.progress<20?int(f.progress)-19:0;
    static constexpr int8_t dx[8]={2,4,2,0,-2,-4,-2,0},dy[8]={-1,0,1,2,1,0,-1,-2};
    return std::make_pair(dx[direction]*step,dy[direction]*step-int(f.height));
}
inline std::optional<int> originalFigureDepthScore(const OriginalFigure& f){
    if(f.alive!=1||f.action==4)return -100;
    //0x469690 reads only the position table: no visual raise, extras or SG offsets.
    if(f.freeMovement){if(f.subX<0||f.subY<0)return std::nullopt;return f.subX%20+f.subY%20;}
    const auto sub=originalFigureSubcell(f);if(!sub)return std::nullopt;return sub->second+int(f.height);
}
//46C3FF..4FE, constants7ACA30/34/38/3C: projectile arc after
//the free-position height subtraction.7650FC truncates toward zero.
inline std::optional<int> originalFigureProjectileArc(const OriginalFigure& f){
    if(f.type!=54||f.projectileDistance<0)return std::nullopt;
    const int half=f.projectileDistance/2;if(!half)return 0;
    const int remaining=std::max(std::abs(int(f.x)-f.destinationX),std::abs(int(f.y)-f.destinationY));
    const long double ratio=f.projectileDistance?static_cast<long double>(remaining)/f.projectileDistance:0.5L;
    const long double arc=remaining>half?(2.0L-static_cast<long double>(remaining)/half)*0.60000002384185791015625L+0.4000000059604644775390625L:
        1.0L-static_cast<long double>(half-remaining)/half;
    return int((1.0L-ratio)*f.projectileTargetHeight)-int(arc*f.projectileEndHeight)+int(ratio*f.projectileStartHeight);
}
inline uint64_t originalFigureDrawCell(const OriginalFigure& f){return uint64_t(f.cell)+(f.type==1?229u:0u);}
} // namespace game_lab
