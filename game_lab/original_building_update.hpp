#pragma once
#include "original_building.hpp"
#include "original_building_animation.hpp"
#include "original_terrain.hpp"
#include "original_figure_update.hpp"
#include "../file_io/include/city19_later7_layout.hpp"
#include "../file_io/include/city19_religion_archive.hpp"
#include <set>

namespace game_lab {
inline int16_t originalBuildingSignedWord(uint16_t v);
inline bool originalBuildingIsHouse(uint16_t type);
inline uint16_t originalBuildingCropFlags(const OriginalBuildingUpdateInputs& i);
inline void originalBuildingSetCropFlags(OriginalBuildingUpdateInputs& i,uint16_t flags);
// All executable facts: SHA256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
// docs/SAV.md#building-update-pass records call order, read-order and limits.
enum class OriginalBuildingUpdateScope {AdmittedTicks,ProductionPasses,HouseServiceDecayPasses};
//0x4F1590 reads the first NINE20-byte rows of the ten-row saved block;
//the tenth category is deliberately preserved. Population7 request594575.
struct OriginalBuildingCityEmployment {
    struct Row {int32_t demand=0,allocated=0,preferred=0,preferredAllocated=0,priority=0;};
    std::array<Row,10> rows{};
    int32_t available=0,employed=0,unemployed=0,unemploymentPercent=0,shortage=0;
    //8501FC/FD have no SAV read. They gate notification548210/548270.
    std::optional<bool> shortageNotice,unemploymentNotice;
};
inline int32_t originalBuildingI32(uint32_t bits){return bits>INT32_MAX?int32_t(int64_t(bits)-4294967296LL):int32_t(bits);}
inline int32_t originalBuildingAdd32(int32_t a,int32_t b){return originalBuildingI32(uint32_t(a)+uint32_t(b));}
inline int32_t originalBuildingSub32(int32_t a,int32_t b){return originalBuildingI32(uint32_t(a)-uint32_t(b));}
inline int32_t originalBuildingPercent(int32_t a,int32_t b){return originalBuildingI32(uint32_t(a)*uint32_t(b))/100;}//408B80
inline std::optional<int32_t> originalBuildingRatio(int32_t a,int32_t b){ //408BA0; numerator wraps32 BEFORE idiv
    if(!b)return 0;const int32_t numerator=originalBuildingI32(uint32_t(a)*100u);
    if(numerator==INT32_MIN&&b==-1)return std::nullopt;return numerator/b;
}
inline std::optional<OriginalBuildingCityEmployment> originalBuildingSavedCityEmployment(const std::vector<uint8_t>& data,size_t side){
    size_t bytes=2;for(const auto& f:emperor::file_io::research::later7_requests)bytes+=f.bytes;
    if(data.size()>8u*1024u*1024u||side>data.size()||side<28+152+bytes)return std::nullopt;
    const size_t population=side-28-152-bytes;if(detail::u16(data.data()+population)!=7)return std::nullopt;
    OriginalBuildingCityEmployment out;out.available=originalBuildingI32(detail::u32(data.data()+population+10190)); //594562
    for(unsigned n=0;n<10;++n){auto& r=out.rows[n];const auto* p=data.data()+population+10194+20*n;
        r={originalBuildingI32(detail::u32(p)),originalBuildingI32(detail::u32(p+4)),originalBuildingI32(detail::u32(p+8)),originalBuildingI32(detail::u32(p+12)),originalBuildingI32(detail::u32(p+16))};}
    out.employed=originalBuildingI32(detail::u32(data.data()+population+10394));
    out.unemployed=originalBuildingI32(detail::u32(data.data()+population+10398));
    out.unemploymentPercent=originalBuildingI32(detail::u32(data.data()+population+10402));
    out.shortage=originalBuildingI32(detail::u32(data.data()+population+10410));return out;
}
struct OriginalBuildingCityEmploymentResult {OriginalBuildingCityEmployment state;bool shortageNotification=false,unemploymentNotification=false;};
//Faithful priority/remainder allocation4F1590..4F1994, bounded to the
//nonnegative saved domain. Notifications are explicit effects for the caller.
inline std::optional<OriginalBuildingCityEmploymentResult> originalBuildingCityEmploymentStep(OriginalBuildingCityEmployment city){
    if(!city.shortageNotice||!city.unemploymentNotice||city.available<0||city.available>INT32_MAX/100)return std::nullopt;
    int64_t demand=0;for(unsigned n=0;n<9;++n){auto& r=city.rows[n];
        if(r.demand<0||r.preferred<0||r.preferred>r.demand||r.priority<0)return std::nullopt;
        r.allocated=0;if(r.priority>5)r.priority=3;demand+=r.demand;}
    //Bound this entry point to nonoverflowing nonnegative allocations. The
    //standalone percentage/ratio helpers retain the original wrap contract.
    if(demand>INT32_MAX/100)return std::nullopt;
    OriginalBuildingCityEmploymentResult out;int32_t left=city.available;
    const auto allocate=[&](int priority,bool zeroPass)->bool{
        int32_t preferred=0,ordinary=0;
        for(unsigned n=0;n<9;++n){const auto& r=city.rows[n];if(r.priority==priority){preferred=originalBuildingAdd32(preferred,r.preferred);ordinary=originalBuildingAdd32(ordinary,r.demand-r.preferred);}}
        int32_t preferredPercent=0,ordinaryPercent=0;
        if(!zeroPass&&priority==0){} //original4F1660 skips priority0 in the first traversal
        else if(preferred<left){preferredPercent=100;const auto ratio=originalBuildingRatio(left-preferred,ordinary);if(!ratio)return false;ordinaryPercent=std::min(100,*ratio);}
        else {const auto ratio=originalBuildingRatio(left,zeroPass?0:preferred);if(!ratio)return false;preferredPercent=*ratio;}
        for(unsigned n=0;n<9;++n){auto& r=city.rows[n];if(r.priority!=priority)continue;
            const int32_t p=originalBuildingPercent(r.preferred,preferredPercent),o=originalBuildingPercent(r.demand-r.preferred,ordinaryPercent);
            r.preferredAllocated=p;r.allocated=originalBuildingAdd32(p,o);left=originalBuildingSub32(left,r.allocated);
            preferred=originalBuildingSub32(preferred,p);ordinary=originalBuildingSub32(ordinary,o);if(left<=0)break;}
        if(!zeroPass&&priority==0)return true;
        //Original round-robin remainders in ascending category order.
        for(bool p:{true,false}){auto& remainder=p?preferred:ordinary;size_t budget=0;
            while(remainder>0&&left>0){bool changed=false;
                for(unsigned n=0;n<9&&remainder>0&&left>0;++n){auto& r=city.rows[n];if(r.priority!=priority)continue;
                    if(p?r.preferredAllocated<r.preferred:r.allocated<r.demand){if(p)++r.preferredAllocated;++r.allocated;--left;--remainder;changed=true;}}
                if(!changed||++budget>1000000)return false;}}
        return true;
    };
    if(demand<=city.available){for(unsigned n=0;n<9;++n){auto& r=city.rows[n];r.allocated=r.demand;r.preferredAllocated=r.preferred;}
        city.employed=int32_t(demand);city.shortage=0;
    }else {
        for(int priority=5;priority>=0;--priority){if(!allocate(priority,false))return std::nullopt;if(left<=0)break;}
        if(!allocate(0,true))return std::nullopt;
        city.employed=city.available;city.shortage=0;for(unsigned n=0;n<9;++n)city.shortage=originalBuildingAdd32(city.shortage,city.rows[n].demand-city.rows[n].allocated);
        if(city.shortage<10)city.shortageNotice=true;
        else if(*city.shortageNotice){city.shortageNotice=false;out.shortageNotification=true;}}
    city.unemployed=originalBuildingSub32(city.available,city.employed);const auto ratio=originalBuildingRatio(city.unemployed,city.available);if(!ratio)return std::nullopt;city.unemploymentPercent=*ratio;
    if(city.unemploymentPercent<10)city.unemploymentNotice=true;
    else if(*city.unemploymentNotice){city.unemploymentNotice=false;out.unemploymentNotification=true;}
    out.state=city;return out;
}
//5501C0 source6 reader: runtime0..89 are wire+2; padding8A..8B
//and96..97 is omitted.550E3F reads+98 at wire150,not148.
struct OriginalBuildingFormation {
    uint8_t alive=0,allStationary=0,count=0,limit=0,special=0,skipLifecycle=0;
    int16_t kind=0,owner=0,damage=0,strength=0;
    std::array<int16_t,16> members{};
    uint8_t spawnTimer=0;
    uint32_t lifecycleOwner=0; //runtime+98,wire150;positive branch is not an animal no-op.
};
inline std::optional<std::array<OriginalBuildingFormation,100>> originalBuildingSavedFormations(
    const std::vector<uint8_t>& data,size_t terrain){
    if(data.size()>8u*1024u*1024u||terrain>data.size()||original_save19::recordsEnd>data.size()-terrain)return std::nullopt;
    std::array<OriginalBuildingFormation,100> out{};
    for(size_t n=0;n<100;++n){const auto* r=data.data()+terrain+original_save19::records+175*n;
        if(detail::u16(r)!=6)return std::nullopt;auto& f=out[n];f.alive=r[2];f.allStationary=r[5];
        f.kind=detail::i16(r+6);f.owner=detail::i16(r+8);f.count=r[42];f.limit=r[43];f.special=r[68];f.skipLifecycle=r[107];
        f.damage=detail::i16(r+76);f.strength=detail::i16(r+78);
        f.spawnTimer=r[119];f.lifecycleOwner=detail::u32(r+150);
        for(unsigned k=0;k<16;++k)f.members[k]=detail::i16(r+10+2*k);}
    return out;
}
struct OriginalBuildingFormationResult {
    std::array<OriginalBuildingFormation,100> formations{};
    std::map<size_t,uint8_t> figureOrdinals;
    uint16_t friendlyStrength=0,hostileStrength=0;
    //10AEEC0/C4/C8/CC and1312568/6B/6C/6D/6F are reset in54CC60.
    std::array<uint32_t,4> groupTotals{};
    std::array<uint8_t,5> groupFlags{};
};
//506710->506A90 Goods1: runtime+8/+C/+E are wire6/10/12.
//The entire32-entry supplied registry is restored,including inactive entries.
struct OriginalBuildingGoodsRecord {int16_t owner=0;uint8_t alive=0;std::array<uint8_t,36> mode{};};
inline std::optional<std::vector<OriginalBuildingGoodsRecord>> originalBuildingSavedGoodsRecords(
    const std::vector<uint8_t>& data,size_t figuresEnd){
    if(figuresEnd>data.size()||data.size()-figuresEnd<6||detail::u16(data.data()+figuresEnd)!=1)return std::nullopt;
    const size_t count=detail::u32(data.data()+figuresEnd+2);if(count>8192||count>(data.size()-figuresEnd-6)/270)return std::nullopt;
    std::vector<OriginalBuildingGoodsRecord> out;for(size_t n=0;n<count;++n){const auto* p=data.data()+figuresEnd+6+270*n;
        if(detail::u16(p)!=1)return std::nullopt;OriginalBuildingGoodsRecord r;r.owner=detail::i16(p+6);r.alive=p[10];
        std::copy_n(p+12,36,r.mode.begin());out.push_back(r);}return out;
}
inline std::optional<std::map<size_t,uint8_t>> originalBuildingSavedFigureStationary(
    const std::vector<uint8_t>& data,const OriginalFigureProfile& profile){
    if(data.size()>8u*1024u*1024u||profile.status!=OriginalFigureStatus::Complete||profile.slots.size()!=2000)return std::nullopt;
    std::map<size_t,uint8_t> out;
    for(const auto& f:profile.slots){size_t at=f.recordOffset;if(at>data.size()||data.size()-at<2)return std::nullopt;
        const auto tag=detail::u16(data.data()+at);at+=2;
        if(tag==0xffff){if(data.size()-at<4)return std::nullopt;const size_t len=detail::u16(data.data()+at+2);if(len>data.size()-at-4)return std::nullopt;at+=4+len;}
        else if(tag<0x8000)return std::nullopt;
        if(f.runtimeClass!=OriginalFigureClass::Figure)at+=2;
        if(at>data.size()||data.size()-at<2||detail::u16(data.data()+at)!=7)return std::nullopt;at+=2;
        bool found=false;for(const auto& field:emperor::file_io::research::figure7_fields){if(field.bytes>data.size()-at)return std::nullopt;
            if(field.receiver_offset==109){out.emplace(f.slot,data[at]);found=true;break;}at+=field.bytes;}
        if(!found)return std::nullopt;}
    return out;
}
//54CC60 reset -> ascending live-figure gather -> lifecycle -> flag totals.
//The saved animal domain skips lifecycle via serialized+69. A positive
//unported lifecycle refuses,never an inferred empty-formation premise.
inline std::optional<OriginalBuildingFormationResult> originalBuildingFormationStep(
    std::array<OriginalBuildingFormation,100> groups,const std::vector<OriginalFigure>& figures,
    const std::map<unsigned,std::array<int32_t,18>>& models,const std::map<size_t,uint8_t>& stationary){
    if(figures.size()!=2000)return std::nullopt;OriginalBuildingFormationResult out;
    for(size_t n=1;n<100;++n)if(groups[n].alive==1){auto& g=groups[n];g.members.fill(0);g.count=0;g.allStationary=1;g.damage=g.strength=0;}
    for(size_t n=1;n<2000;++n){const auto& f=figures[n];if(f.slot!=n)return std::nullopt;if(f.alive!=1)continue;
        const unsigned t=f.type;const bool friendly=t==56||t==57||(t>=64&&t<=68)||t==79;
        const bool hostile=(t>=58&&t<=62)||t==78;
        if(!friendly&&!hostile&&(t<69||t>77))continue;
        if(t==56||t==57)continue;
        const uint16_t strength=t==63||t==64?10:t==65?12:t==66?25:t==67?40:t==68?100:t==79?500:0;
        const uint16_t enemy=t==58?10:t==59?12:t==60?25:t==61?40:t==62?100:t==78?500:0;
        out.friendlyStrength=uint16_t(out.friendlyStrength+strength);out.hostileStrength=uint16_t(out.hostileStrength+enemy);
        if(f.formation<=0)continue;if(f.formation>=100)return std::nullopt;auto& g=groups[size_t(f.formation)];
        ++g.count;g.damage=int16_t(uint16_t(uint32_t(uint16_t(g.damage))+uint16_t(f.damage)));
        //+114=4C9310 reads model column1 via4EB9C0(id,1).
        const auto model=models.find(t);const auto flag=stationary.find(n);
        if(model==models.end()||flag==stationary.end())return std::nullopt;
        g.strength=int16_t(uint16_t(uint32_t(uint16_t(g.strength))+uint32_t(model->second[1])));
        if(flag->second!=1)g.allStationary=0;
        const unsigned cap=g.kind==58||g.kind==59||g.kind==64||g.kind==65?16:g.kind==61||g.kind==62||g.kind==67||g.kind==68?4:g.kind==78||g.kind==79?1:8;
        for(unsigned k=0;k<cap;++k)if(!g.members[k]){g.members[k]=int16_t(n);out.figureOrdinals.emplace(n,uint8_t(k));break;}}
    for(size_t n=1;n<100;++n){const auto& g=groups[n];if(g.alive!=1)continue;
        if(!g.skipLifecycle)return std::nullopt;
        if(g.special)return std::nullopt; //positive special totals also read runtime+67,not retained yet.
    }
    out.formations=std::move(groups);return out;
}
enum class OriginalBuildingUpdateBlocker {None,InvalidState,MissingContext,ExternalWrites,
    ProductionVirtuals,RoadRefresh,Employment,HousingEvolution,BuildingVirtual20,
    RiskAndFire,MonthlyCity,Availability,CityPhase,UnclosedPhase,HousingCapacity,CityLabor,Trade};
inline const char* originalBuildingUpdateBlockerName(OriginalBuildingUpdateBlocker b){
    switch(b){case OriginalBuildingUpdateBlocker::None:return "computed";
    case OriginalBuildingUpdateBlocker::InvalidState:return "invalid_saved_building_update_inputs";
    case OriginalBuildingUpdateBlocker::MissingContext:return "explicit_phase_terrain_disabled_context_required";
    case OriginalBuildingUpdateBlocker::ExternalWrites:return "intervening_figure_city_writes_not_supplied";
    case OriginalBuildingUpdateBlocker::ProductionVirtuals:return "production_virtuals_51e4a0_74_208";
    case OriginalBuildingUpdateBlocker::RoadRefresh:return "road_refresh_4acfc0_426df0_1391fe0";
    case OriginalBuildingUpdateBlocker::Employment:return "employment_4f19a0_51e310";
    case OriginalBuildingUpdateBlocker::HousingEvolution:return "house_evolution_519120_518ef0";
    case OriginalBuildingUpdateBlocker::BuildingVirtual20:return "building_virtual20_416d20";
    case OriginalBuildingUpdateBlocker::RiskAndFire:return "risk_fire_4af5d0_42a220";
    case OriginalBuildingUpdateBlocker::MonthlyCity:return "monthly_city_4ac650";
    case OriginalBuildingUpdateBlocker::Availability:return "availability_4ace60_virtual1a0";
    case OriginalBuildingUpdateBlocker::CityPhase:return "city_accounting_4f1560_4ac2fa";
    case OriginalBuildingUpdateBlocker::HousingCapacity:return "housing_capacity_4ad3d0_44cc80";
    case OriginalBuildingUpdateBlocker::CityLabor:return "city_labor_4ad4a0_4ada10_4adc90";
    case OriginalBuildingUpdateBlocker::Trade:return "empire_trade_5dbcc0_4b02a0";
    case OriginalBuildingUpdateBlocker::UnclosedPhase:return "scheduled_phase_callee_not_closed";}
    return "invalid_saved_building_update_inputs";
}
struct OriginalBuildingUpdateState {
    OriginalBuildingFootprint building;
    OriginalBuildingUpdateInputs inputs;
    std::optional<OriginalBuildingProductionInputs> production=std::nullopt;
    std::optional<uint16_t> roadDistance=std::nullopt;
    std::optional<int16_t> roadX=std::nullopt,roadY=std::nullopt;
    std::optional<uint8_t> roadNetwork=std::nullopt;
    std::optional<int16_t> roadFailureCount=std::nullopt;
    std::optional<OriginalBuildingHouseCapacityInputs> houseCapacity=std::nullopt;
};
//0x5AE140 ->521140 clears51984 dwords, then5AE240 visits N,E,S,W.
//13789C0 is a rebuilt movement-word grid, NOT serialized road distance.
//The caller must provide that entire grid and the scenario entry coordinate.
inline std::optional<std::vector<uint32_t>> originalBuildingRoadDistances(
    const std::vector<uint16_t>& movement,uint32_t entry){
    if(movement.size()!=51984||entry>=51984)return std::nullopt;
    std::vector<uint32_t> distance(51984);std::array<uint32_t,51984> queue{};
    size_t head=0,tail=1;queue[0]=entry;distance[entry]=1;constexpr int delta[]={-228,1,228,-1};
    while(head!=tail){const uint32_t cell=queue[head];head=(head+1)%queue.size();
        //Original has no edge guard; accept only inputs for which every
        //original memory access remains in the declared grid.
        if(cell<228||cell>=51984-228)return std::nullopt;
        for(int d:delta){const size_t next=size_t(int(cell)+d);
            if(!distance[next]&&(movement[next]&0xb7c)){
                distance[next]=distance[cell]+1;queue[tail]=uint32_t(next);tail=(tail+1)%queue.size();}}
    }
    return distance;
}
struct OriginalBuildingRoadInputs {
    uint32_t cropBase=0;
    std::vector<uint32_t> terrain,distance;
    std::vector<uint16_t> owners;
    std::vector<uint8_t> networks;
    std::array<uint32_t,10> rankedNetworks{};
    std::optional<std::array<uint16_t,4000>> ownerTypes;
    //4BAF40 calls owner+D0 with(incoming origin,&relative candidate).
    //Unresolved owner handling refuses; explicit complete outcomes are a
    //caller service, never a blanket claim that every owner is traversable.
    struct OwnerProbe {uint16_t owner=0;uint32_t origin=0;int32_t relative=0,resolvedRelative=0;bool rejected=false;};
    std::vector<OwnerProbe> ownerProbes;
};
struct OriginalBuildingRoadAccess {uint16_t distance=0;int16_t x=0,y=0;std::optional<uint8_t> network;bool selected=false;};
//543DC0 ->544EA0 ->542350; common-market descriptor7AB800 has
//two entrances,42CD30/857D88: orientation0(0,3),(3,3),1(3,0),(3,3).
//Strict Manhattan comparison chooses the first entrance from the origin.
inline std::optional<OriginalBuildingRoadAccess> originalBuildingMarketRoadAccess(
    const OriginalBuildingFootprint& f,const OriginalBuildingRoadInputs& c){
    if(f.type!=59||!f.imageInputs||!f.imageInputs->marketOrientation||*f.imageInputs->marketOrientation>1||
        c.distance.size()!=51984||c.networks.size()!=51984)return std::nullopt;
    const unsigned rotation=*f.imageInputs->marketOrientation;
    const uint64_t origin=uint64_t(c.cropBase)+f.x+228u*f.y,cell=origin+(rotation?3u:684u);
    if(origin!=f.imageInputs->gridIndex||cell>=51984)return std::nullopt;
    return OriginalBuildingRoadAccess{uint16_t(c.distance[size_t(cell)]),int16_t(f.x+(rotation?3:0)),
        int16_t(f.y+(rotation?0:3)),c.networks[size_t(cell)],true};
}
//5312CB ->52CD90 Scenario6; C5CAF0+31C..322 are the four shorts
//C5CDFC/FE/CE00/CE02.52D411/421/431/441 restore wire794/796/798/800.
inline std::optional<std::array<int16_t,4>> originalBuildingSavedRoadEndpoints(const std::vector<uint8_t>& data,size_t side){
    if(side<28||side>data.size()||data.size()-side<774||detail::u16(data.data()+side-28)!=6)return std::nullopt;
    std::array<int16_t,4> out{};for(unsigned n=0;n<4;++n)out[n]=detail::i16(data.data()+side-28+794+2*n);return out;
}
//4BAF40 perimeter table820038 (24 signed dwords per footprint side).
//Checked geometric order is clockwise top,right,bottom,left; corners excluded.
inline std::optional<OriginalBuildingRoadAccess> originalBuildingRoadAccess(
    const OriginalBuildingFootprint& f,const OriginalBuildingRoadInputs& c){
    if(!f.imageInputs||!f.size||f.size>6||c.terrain.size()!=51984||c.distance.size()!=51984||
        c.owners.size()!=51984||c.networks.size()!=51984)return std::nullopt;
    const uint64_t wide=uint64_t(c.cropBase)+f.y*228u+f.x;if(wide>=51984||wide!=f.imageInputs->gridIndex)return std::nullopt;
    const int origin=int(wide);std::vector<int> offsets;
    for(unsigned n=0;n<f.size;++n)offsets.push_back(-228+int(n));
    for(unsigned n=0;n<f.size;++n)offsets.push_back(int(n)*228+f.size);
    for(unsigned n=f.size;n>0;--n)offsets.push_back(int(f.size)*228+int(n)-1);
    for(unsigned n=f.size;n>0;--n)offsets.push_back((int(n)-1)*228-1);
    int bestRank=12,chosen=-1;
    for(int relative:offsets){int cell=origin+relative;if(cell<0||cell>=51984)return std::nullopt;
        const uint16_t owner=c.owners[size_t(cell)];
        if(owner){
            bool resolved=false;
            if(c.ownerTypes&&owner<4000){const auto type=c.ownerTypes->at(owner);
                //Individually cited+D0=426D80;415700 walls and7E reject.
                if(type==126||type==231||type==89||type==90||type==91||type==104||type==105||type==106||type==232)continue;
                switch(type){case 2:case 3:case 5:case 7:case 33:case 35:case 43:case 53:case 54:case 58:
                case 62:case 66:case 67:case 71:case 72:case 124:case 127:case 192:case 193:case 194:case 196:
                case 207:case 211:case 214:resolved=true;break;default:break;}}
            if(!resolved){const auto p=std::find_if(c.ownerProbes.begin(),c.ownerProbes.end(),[&](const auto& p){return p.owner==owner&&p.origin==unsigned(origin)&&p.relative==relative;});
                if(p==c.ownerProbes.end())return std::nullopt;if(p->rejected)continue;
                relative=p->resolvedRelative;cell=origin+relative;if(cell<0||cell>=51984)return std::nullopt;}}
        if((c.terrain[size_t(cell)]&0x44)!=0x40||!c.distance[size_t(cell)]||c.distance[size_t(cell)]>INT32_MAX)continue;
        const auto rank=std::find(c.rankedNetworks.begin(),c.rankedNetworks.end(),c.networks[size_t(cell)]);
        const int r=rank==c.rankedNetworks.end()?11:int(rank-c.rankedNetworks.begin());
        if(r<bestRank){bestRank=r;chosen=cell;}
    }
    if(chosen<0){uint32_t nearest=100000;
        for(int d:offsets){const int cell=origin+d;if(cell<0||cell>=51984)return std::nullopt;const auto distance=c.distance[size_t(cell)];
            if(distance&&distance<=INT32_MAX&&distance<nearest){nearest=distance;chosen=cell;}}}
    OriginalBuildingRoadAccess out;if(chosen<0)return out;
    out.distance=uint16_t(c.distance[size_t(chosen)]);const int coordinate=chosen-int(c.cropBase);
    out.x=int16_t(coordinate%228);out.y=int16_t(coordinate/228);
    //426DF0 always dispatches+1AC, including its nearest-distance fallback.
    out.network=c.networks[size_t(chosen)];out.selected=true;return out;
}
//House+84=518A50 uses4BA6F0: rank roads WITHOUT distance qualification,
//then tests distance at the selected exit. The reachable branch resets+28;
//unreachable paths can spawn departures or remove houses and remain refused.
inline std::optional<OriginalBuildingRoadAccess> originalBuildingHouseRoadAccess(
    const OriginalBuildingFootprint& f,const OriginalBuildingRoadInputs& c,bool requireDistance=true){
    if(!f.imageInputs||!f.size||f.size>6||c.terrain.size()!=51984||
        c.distance.size()!=51984||c.networks.size()!=51984)return std::nullopt;
    const uint64_t origin=uint64_t(c.cropBase)+f.y*228u+f.x;if(origin>=51984||origin!=f.imageInputs->gridIndex)return std::nullopt;
    std::vector<int> offsets;for(unsigned n=0;n<f.size;++n)offsets.push_back(-228+int(n));
    for(unsigned n=0;n<f.size;++n)offsets.push_back(int(n)*228+f.size);
    for(unsigned n=f.size;n>0;--n)offsets.push_back(int(f.size)*228+int(n)-1);
    for(unsigned n=f.size;n>0;--n)offsets.push_back((int(n)-1)*228-1);
    int best=12,chosen=-1;
    for(int delta:offsets){const int64_t cell=int64_t(origin)+delta;if(cell<0||cell>=51984)return std::nullopt;
        if(c.terrain[size_t(cell)]&8){
            if(!c.ownerTypes||c.owners.size()!=51984||c.owners[size_t(cell)]>=4000)return std::nullopt;
            const auto type=c.ownerTypes->at(c.owners[size_t(cell)]);
            //Individually verified E4=416A60(false),190=426D30(false)
            //for these types;562F70's76..268 table is false for this domain.
            switch(type){case 2:case 3:case 5:case 7:case 33:case 35:case 43:case 53:case 54:case 58:
            case 62:case 66:case 67:case 72:case 124:case 126:case 127:case 192:case 193:case 194:case 196:
            case 207:case 211:case 214:case 231:break;default:return std::nullopt;}}
        if((c.terrain[size_t(cell)]&0x44)!=0x40)continue;
        const auto found=std::find(c.rankedNetworks.begin(),c.rankedNetworks.end(),c.networks[size_t(cell)]);
        const int rank=found==c.rankedNetworks.end()?11:int(found-c.rankedNetworks.begin());
        if(rank<best){best=rank;chosen=int(cell);}}
    if(chosen<0)return requireDistance?std::nullopt:std::optional<OriginalBuildingRoadAccess>(OriginalBuildingRoadAccess{});
    if(requireDistance&&!c.distance[size_t(chosen)])return std::nullopt;
    OriginalBuildingRoadAccess out;out.distance=uint16_t(c.distance[size_t(chosen)]);
    out.x=int16_t((chosen-int(c.cropBase))%228);out.y=int16_t((chosen-int(c.cropBase))/228);out.selected=true;return out;
}
//540880 copies parent+24/+2A/+2C in registered slot order,without
//dispatching+1AC. The framed market child table544FB7 validates parentage.
inline std::optional<OriginalBuildingRoadAccess> originalBuildingMultipartRoadAccess(
    const OriginalBuildingFootprint& f,const OriginalBuildingProfile& profile,const OriginalBuildingRoadInputs& c,
    std::optional<uint32_t> savedParent=std::nullopt){
    if(f.type==59)return originalBuildingMarketRoadAccess(f,c);
    if((f.type!=62&&f.type!=66&&f.type!=67)||!savedParent)return std::nullopt;
    const OriginalBuildingFootprint* parent=nullptr;
    for(const auto& p:profile.footprints)if(p.type==59&&p.imageInputs&&
        std::find(p.imageInputs->marketStalls.begin(),p.imageInputs->marketStalls.end(),f.slot)!=p.imageInputs->marketStalls.end()){
        if(parent||p.slot>=f.slot)return std::nullopt;parent=&p;}
    if(!parent||parent->slot!=*savedParent)return std::nullopt;auto out=originalBuildingMarketRoadAccess(*parent,c);if(out)out->network.reset();return out;
}
//4AF350 keeps the largest ten networks, strict-greater insertion (ties retain
//the earlier label). Counts include every labelled cell, not just road seeds.
inline std::optional<std::array<uint32_t,10>> originalBuildingRankedRoadNetworks(const std::vector<uint8_t>& networks){
    if(networks.size()!=51984)return std::nullopt;std::array<uint32_t,256> counts{};
    for(auto n:networks)if(n)++counts[n];std::array<uint32_t,10> ranked{},sizes{};
    for(unsigned label=1;label<256;++label)if(counts[label]){
        unsigned rank=0;while(rank<10&&counts[label]<=sizes[rank])++rank;if(rank==10)continue;
        for(unsigned n=9;n>rank;--n){ranked[n]=ranked[n-1];sizes[n]=sizes[n-1];}
        ranked[rank]=label;sizes[rank]=counts[label];}
    return ranked;
}
struct OriginalBuildingUpdateContext {
    // Explicit operation count: admitted ticks are distinct from phase20 calls.
    OriginalBuildingUpdateScope scope;
    // Required isolation premise: normal live-mode city scheduler is entered;
    // scenario/event early return is false; no intervening figure/service/city writes.
    // Retained fields and existing selectors only; this is not a full opaque
    // runtime object snapshot (unserialized scratch+92 is outside this API).
    // This is a building-only experiment, never a complete future city state.
    bool isolateBuildingWrites;
    std::optional<uint32_t> phase,monthPart,cycle,month;
    std::optional<uint32_t> terrainFlags;
    std::optional<bool> typeDisabled;
    // Population7 unavailable[output], model5, and Festival4+50/+54/+58.
    // These are explicit immutable services in the isolated production scope.
    std::optional<int16_t> unavailable;
    std::optional<int32_t> requiredWorkers;
    std::optional<bool> festivalProductionBoost;
    std::optional<OriginalBuildingRoadAccess> roadAccess;
    //True only after5AE140 reaches the scenario exit and every live+84
    //leaves the shared output location0. Otherwise4ACFC0 can repair city data.
    std::optional<bool> roadRefreshOrdinaryExit;
    //Whole ordered house pool for4AD3D0; caller-owned, copied before writes.
    std::optional<std::map<size_t,OriginalBuildingHouseCapacityInputs>> houses;
    std::optional<std::map<int,int>> houseModelCapacity;
    std::optional<uint32_t> cityHouseCapacity,cityHouseFree;
    //54CC60 requires the entire ordered figure pool and source6 groups.
    std::optional<std::array<OriginalBuildingFormation,100>> formations;
    std::optional<std::vector<OriginalFigure>> figures;
    std::optional<std::map<unsigned,std::array<int32_t,18>>> figureModels;
    std::optional<std::map<size_t,uint8_t>> figureStationary;
    //Figure+6C writes from54CC60; retained separately until Figure exposes it.
    std::map<size_t,uint8_t> figureOrdinals;
    std::optional<OriginalBuildingFormationResult> formationResult;
    std::optional<OriginalBuildingCityEmployment> cityEmployment;
    // Explicit already-computed city services used by the accounting pass.
    // Values cannot be inferred from the record under test.
    struct Accounting {
        int32_t taxRate=0,taxableOrdinary=0,taxableElite=0,taxReceived=0,taxOther=0;
        int32_t wages=0,wagesPaid=0,incomeOther=0,expensesOther=0,treasuryStart=0;
        int32_t incomeReceived=0,expensesPaid=0,actualProfit=0;
        int32_t ordinaryTax=0,eliteTax=0,taxTotal=0,taxForecast=0,wageTotal=0,wageForecast=0;
        int32_t income=0,expenses=0,profit=0,treasuryForecast=0,festivalCount=0,festivalCapacity=0;
    };
    std::optional<Accounting> accounting;
    unsigned accountingCalls=0,formationCalls=0;
    int32_t citySignal=0,cityCountA=0,cityCountB=0;
    //Derived from the complete framed pool,not per-record assumptions.
    std::optional<std::map<size_t,OriginalBuildingUpdateInputs>> buildingInputs;
    std::optional<std::map<size_t,OriginalBuildingFootprint>> buildings;
    std::optional<std::map<int,int>> houseTaxModel;
    std::optional<int32_t> scenarioFoodOverride;
    struct CityDerived {std::array<int32_t,11> serviceCoverage{};int32_t population=0,food=0,foodMonths=0,foodConsumption=0;
        std::array<int32_t,10> millFood{};std::array<uint32_t,36> warehouse{},trade5{},trade7{};};
    std::optional<CityDerived> cityDerived;
    int32_t serviceRating=100,serviceWeakest=0,servicePenalty=0;
    std::optional<std::array<int32_t,11>> serviceCoverage;
    std::optional<OriginalFigureGoodsServices> goods;
    std::optional<std::map<size_t,uint8_t>> storageRoadStatus;
    std::optional<std::vector<OriginalBuildingGoodsRecord>> goodsRecords;
    std::array<int32_t,36> millAvailable{};std::array<int16_t,100> millCandidates{};size_t millCandidateCount=0;
    std::optional<OriginalFigurePassability> movement;
    std::optional<uint32_t> activeSide;
    std::vector<uint8_t> roadNetworks;std::array<uint32_t,10> rankedNetworks{};
    unsigned networkCalls=0,goodsPoolCalls=0;
    std::optional<uint32_t> highestBuildingSlot,buildingRegistrySize,highestEverBuildingSlot,goodsRegistryLast;
    std::optional<bool> applyLoadRoadRefresh;
    bool loadRoadRefreshApplied=false;
    std::optional<std::map<size_t,OriginalFigureBuildingLink>> buildingLinks;
    std::optional<std::map<size_t,OriginalBuildingRoadAccess>> poolRoadAccess;
    std::optional<std::map<size_t,bool>> poolRoadReachable;
    std::optional<std::map<size_t,bool>> poolRoadSelected;
    std::optional<std::map<size_t,OriginalBuildingProductionInputs>> productionInputs;
    std::optional<std::array<int16_t,36>> unavailableResources;
    std::optional<OriginalBuildingEmployment> employmentModel;
    std::optional<std::map<size_t,bool>> buildingUnavailable;
    std::optional<int32_t> unemploymentTrend,shortageNoticeCountdown,elapsedYears,scenarioStartYears;
    unsigned employmentCalls=0;
    std::optional<int32_t> populationTotal,populationPeak,treasury;
    std::optional<uint32_t> cropBase;
    struct Labor {int32_t sentiment=0,health=0,migration=0,incoming=0,outgoing=0,inRemainder=0,outRemainder=0;
        int32_t inDelay=0,outDelay=0,arrivals=0,departures=0,unplaced=0,yearArrivals=0,spacing=0,statisticsPending=0;
        int32_t populationSum=0,sampleCount=0,averagePopulation=0,growthCursor=0;
        int16_t deathCursor=0;int32_t lastPopulationDelta=0,excessDepartureTotal=0; //591900/591950,1311F90/8C.
        int32_t difficulty=-1,wageBand=-1;std::optional<std::array<bool,10>> populationNotices;
        int32_t hostileCount=0;std::array<int32_t,4> formationLabor{};};
    std::optional<Labor> labor;
    std::optional<bool> loadAccountingTransition;
    bool loadAccountingApplied=false;
    std::optional<uint32_t> crtRandom,wallClock;
    uint64_t crtRandomCalls=0;
    size_t highestFigureSlot=0,highestEverFigureSlot=0;
    std::vector<size_t> freeFigureSlots;
    //4C92D0->4C92A0 chooses speech+7E from saved10AEEF8 and
    //EmperorText.eng group lengths10DB89C;neither is a CRT random draw.
    std::optional<std::array<int32_t,359>> figureSpeechCounters;
    std::optional<std::array<uint32_t,359>> figureSpeechCounts;
    std::map<size_t,int16_t> figureSpeechChoices;
    std::optional<OriginalFigureSavedServices> figureServices;
    std::optional<std::array<int16_t,4>> roadEndpoints;
    std::optional<std::vector<uint32_t>> terrain;
    std::optional<bool> soundEnabled;
    std::optional<bool> scenarioFigurePlacementCheck; //A98370+3F770,4EBCB0;not a SAV field.
    std::optional<std::vector<uint16_t>> figureCellHeads;
    std::vector<size_t> allocatedFigures;
    std::vector<unsigned> notificationEffects;
    std::optional<int32_t> engineRegion;
    int32_t laborCountdownHouses=0;
    struct Irrigation {
        std::vector<uint32_t> image;
        std::vector<uint8_t> subTile;
        std::vector<uint16_t> owners;
        std::array<uint16_t,4000> ownerTypes{};
        uint32_t baseImage=0; //408170(435), supplied from the archive table.
        struct Source {uint32_t cell=0;int16_t workers=0;uint8_t orientation=0;};
        std::vector<Source> sources; //complete524310 registry,not all buildings.
    };
    std::optional<Irrigation> irrigation;
    unsigned irrigationCalls=0;
    int32_t waterScratch=0;
    struct Statistics {
        int32_t primaryBuilding=0,alternateBuilding=0,emptyCycles=0,currentFaction=-1,capacity=0;
        int16_t primaryPresent=0;
        std::array<int32_t,4> demand{},groups{};
        std::array<uint8_t,22> empireState{};
        //5A8470/5A8450 read process-only10BFB50/40.
        std::optional<bool> religionBonusEnabled;
        std::optional<int32_t> religionBonus;
    };
    std::optional<Statistics> statistics;
    std::optional<std::array<bool,36>> empireProduction;
    int32_t statisticsScratch=0,hostileCount=0;
    uint8_t hostileFlag=0;
    unsigned statisticsCalls=0;
    unsigned storageCalls=0,foodCalls=0;
    struct Religion {
        struct Entry {int32_t minimum=0,maximum=0,status=0,type=0,value=0;};
        struct Worship {std::array<Entry,3> entries{};bool enabled=false;int32_t minimum=0,maximum=0,average=0,served=0;bool temple=false;};
        std::array<Worship,4> worship{};
        std::array<uint8_t,12> scenarioEnabled{};
        std::optional<int32_t> previousRating,selected;
        unsigned calls=0;
    };
    std::optional<Religion> religion;
    //4AC3C4->4BC440 is an opt-in additive city service. Older isolated
    //per-record callers retain their scope unless they explicitly request it.
    struct ResourceGrowth {
        OriginalTerrainState surface;
        std::vector<uint16_t> owners;
        std::map<uint16_t,uint32_t> imageBases; //408170(code), explicit archive lookup
        std::array<uint8_t,6> flags{}; //D62438..3D, reset on every call
        std::vector<uint32_t> matureTrees,matureReeds;
        unsigned calls=0;uint64_t randomCalls=0;size_t changed=0,imageWrites=0;
    };
    std::optional<ResourceGrowth> resourceGrowth;
    std::optional<bool> walkersDisabled; //saved10BC7E0[172],53192B
    std::optional<int32_t> buildingHookCountdown; //saved1032658,53166F
    unsigned hookPreambleCalls=0;
    struct Trade {
        struct Partner {
            uint8_t alive=0,kind=0,faction=0,sea=0,blocked=0;
            int32_t enabled=0,relationship=0;
            int16_t timer=0,suspended=0;
            std::array<int16_t,3> traders{};
            std::array<uint8_t,36> quota{},quotaBonus{};
            std::array<uint8_t,4> buys{},sells{};
        };
        std::array<Partner,22> partners{};
        std::array<int16_t,4> counters{}; //Population7 59498E..5949BE.
        //5DC500 returns process service1E4FBD8; its timer words+8/+C
        //have no SAV reader. Required only when a saved timer expires.
        std::optional<std::array<int32_t,2>> dispatchDelay;
        std::optional<std::array<int16_t,2>> seaEntry;
        unsigned calls=0,eligible=0,timersChanged=0;
    };
    std::optional<Trade> trade;
    struct BuildingTotals {
        std::array<int32_t,269> total{},staffed{};
        std::array<int32_t,36> producers{},staffedProducers{};
        unsigned calls=0;
    };
    std::optional<BuildingTotals> buildingTotals;
    //51CF90 positive service creation uses4EA050 then4E6A70. The
    //existing cited figure roam service supplies terrain owners/random bytes.
    std::optional<std::vector<uint16_t>> terrainOwners;
    std::optional<std::vector<uint8_t>> terrainRandom;
    std::optional<std::map<uint16_t,std::array<int32_t,13>>> buildingModels;
    std::optional<std::vector<int8_t>> desirability;
    unsigned desirabilityCalls=0;
    //4BA6F0 hook query ranks adjacent roads before checking distance;
    //its shared10C72A8/AC differs from4BAF40's refresh selection.
    std::optional<std::map<size_t,OriginalBuildingRoadAccess>> poolHookRoadAccess;
    OriginalBuildingUpdateContext(OriginalBuildingUpdateScope operation,bool isolated)
        :scope(operation),isolateBuildingWrites(isolated){}
};
//4B05F0 rebuilds the active diamond's row spans from the saved side. The
//two middle rows both have full width for an even side (4B0694..4B06DC).
inline std::optional<std::vector<std::pair<uint32_t,uint32_t>>> originalBuildingResourceRows(uint32_t side,uint32_t crop){
    if(!side||side>228||crop!=((228-side)/2)*229)return std::nullopt;
    const uint32_t middle=(side-1)/2;std::vector<std::pair<uint32_t,uint32_t>> out;
    for(uint32_t y=0;y<side;++y){const uint32_t inset=y<=middle?middle-y:y-middle-1;
        const int64_t width=int64_t(2*middle)+2-2*inset;
        if(width<0||width>228||uint64_t(crop)+228*y+inset+uint64_t(width)>51984)return std::nullopt;
        out.emplace_back(crop+228*y+inset,uint32_t(width));}return out;
}
//4BC440 consumes one ENGINE random draw for every immature resource,
//including masked/owned cells whose image helper subsequently returns early.
//No CRT seed or wall clock is involved. Positive woodcutter routing5AE970
//is refused until its complete caller-owned registry service is ported.
inline OriginalBuildingUpdateBlocker originalBuildingResourceGrowthStep(OriginalBuildingUpdateContext& c){
    if(!c.resourceGrowth||!c.terrain||!c.activeSide||!c.cropBase||!c.figureServices||!c.buildings||!c.buildingInputs)
        return OriginalBuildingUpdateBlocker::MissingContext;
    auto& g=*c.resourceGrowth;auto& s=g.surface;
    if(c.irrigation)s.image=c.irrigation->image;
    if(c.terrain->size()!=51984||s.feature.size()!=51984||s.randomByte.size()!=51984||s.roadDistance.size()!=51984||s.image.size()!=51984||g.owners.size()!=51984)
        return OriginalBuildingUpdateBlocker::InvalidState;
    if(c.figureServices->randomHistory&&c.figureServices->randomHistory->cursor>=100)return OriginalBuildingUpdateBlocker::InvalidState;
    const auto rows=originalBuildingResourceRows(*c.activeSide,*c.cropBase);if(!rows)return OriginalBuildingUpdateBlocker::InvalidState;
    for(const auto& b:*c.buildings)if(b.second.type==38){const auto i=c.buildingInputs->find(b.first);
        if(i==c.buildingInputs->end())return OriginalBuildingUpdateBlocker::InvalidState;
        if((i->second.alive==1||i->second.alive==3)&&i->second.workers)return OriginalBuildingUpdateBlocker::UnclosedPhase;}
    s.terrain=*c.terrain;g.flags.fill(0);g.matureTrees.clear();g.matureReeds.clear();g.changed=g.imageWrites=0;
    for(const auto& row:*rows)for(uint32_t n=0;n<row.second;++n){const size_t cell=row.first+n;const uint32_t flags=s.terrain[cell];
        const unsigned kind=(flags&1)?0:(flags&0x40000)?1:(flags&0x80)?2:3;if(kind==3)continue;
        g.flags[2*kind+1]=1;auto& value=s.roadDistance[cell];
        if(value==255){if(kind==0)g.matureTrees.push_back(uint32_t(cell));else if(kind==1)g.matureReeds.push_back(uint32_t(cell));continue;}
        const uint32_t random=originalFigureRandomNext(*c.figureServices);++g.randomCalls;
        //4BDA70/80/90/A0 use first&1 plus2/4/35/15. Their this
        //pointer D62420 is unused: saved words at that address are NOT rates.
        const unsigned delta=(random&1)+(kind==0?((s.feature[cell]&1)?2u:4u):kind==1?35u:15u);
        const uint8_t old=value,next=uint8_t(unsigned(value)+delta);value=next<old?255:next;++g.changed;
        if(kind==1){if((flags&0xc064)||(flags&0x80000))continue;
            const auto base=g.imageBases.find(0x620);if(base==g.imageBases.end())return OriginalBuildingUpdateBlocker::MissingContext;
            s.image[cell]=base->second+s.randomByte[cell]%9;++g.imageWrites;continue;}
        //4BC6A0 gates precede the owner/bit1 tests,including for farms.
        if(flags&0xc064)continue;
        if(flags&0x100){size_t count=0;for(int d:{-228,-227,1,229,228,227,-1,-229}){
                const int64_t at=int64_t(cell)+d;if(at<0||at>=51984)return OriginalBuildingUpdateBlocker::InvalidState;
                count+=(s.terrain[size_t(at)]&0x100)!=0;}if(count==8)continue;}
        if(g.owners[cell]||!(flags&1)||(flags&0x80000))continue;
        uint16_t code=0x627;unsigned variant=s.randomByte[cell]&7;
        if(value==255){code=(s.feature[cell]&2)?0x625:0x624;bool ring=true;
            const int x=int(cell%228)-int(*c.cropBase%228),y=int(cell/228)-int(*c.cropBase/228);
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){if(!dx&&!dy)continue;
                //4BB390 accepts the rectangular margin -1..side inclusive.
                const int yy=y+dy,xx=x+dx;if(yy< -1||yy>int(*c.activeSide)||xx< -1||xx>int(*c.activeSide))continue;
                const int64_t at=int64_t(cell)+228*dy+dx;if(at<0||at>=51984)return OriginalBuildingUpdateBlocker::InvalidState;
                if(!(s.terrain[size_t(at)]&3))ring=false;}
            variant=unsigned(ring)*9+s.randomByte[cell]%9;}
        const auto base=g.imageBases.find(code);if(base==g.imageBases.end())return OriginalBuildingUpdateBlocker::MissingContext;
        s.image[cell]=base->second+variant;++g.imageWrites;
    }
    if(c.irrigation)c.irrigation->image=s.image;
    ++g.calls;return OriginalBuildingUpdateBlocker::None;
}
//429700/429780/429800 clear stale reciprocal links before later gates.
inline std::optional<bool> originalBuildingHookFigure(int16_t& id,unsigned type,size_t owner,const OriginalBuildingUpdateContext& c){
    if(id<=0)return false;if(!c.figures||id>=int(c.figures->size()))return std::nullopt;
    const auto& f=c.figures->at(size_t(id));if(!f.alive||(f.type!=type&&f.type!=0)||f.owner!=int(owner)){id=0;return false;}return true;
}
inline OriginalBuildingUpdateBlocker originalBuildingHookPreamble(OriginalBuildingUpdateContext& c){
    if(!c.buildingInputs||!c.buildings||!c.figures||c.figures->size()!=2000||!c.buildingHookCountdown||!c.highestBuildingSlot)return OriginalBuildingUpdateBlocker::MissingContext;
    if(*c.buildingHookCountdown>0)--*c.buildingHookCountdown;
    for(const auto& b:*c.buildings)if(b.second.type==52)return OriginalBuildingUpdateBlocker::BuildingVirtual20;
    for(size_t slot=1;slot<c.figures->size();++slot){const auto& f=c.figures->at(slot);if(!f.alive||f.type!=57||f.owner<=0||f.owner>int(*c.highestBuildingSlot))continue;
        const auto i=c.buildingInputs->find(size_t(f.owner));if(i==c.buildingInputs->end()||!i->second.hookIndustry)return OriginalBuildingUpdateBlocker::BuildingVirtual20;
        ++i->second.hookIndustry->at(0x5c-4);}
    ++c.hookPreambleCalls;return OriginalBuildingUpdateBlocker::None;
}
inline std::optional<size_t> originalBuildingAllocateService(OriginalBuildingUpdateContext& c,uint8_t type,uint8_t faction,int16_t x,int16_t y,uint8_t direction){
    if(!c.figures||c.figures->size()!=2000||!c.figureServices||!c.figureServices->allocationSerial||!c.cropBase||
        !c.figureCellHeads||c.figureCellHeads->size()!=51984||!c.figureStationary||!c.scenarioFigurePlacementCheck||*c.scenarioFigurePlacementCheck)return std::nullopt;
    size_t slot=0;for(unsigned attempt=0;attempt<6&&!c.freeFigureSlots.empty();++attempt){const auto candidate=c.freeFigureSlots.front();c.freeFigureSlots.erase(c.freeFigureSlots.begin());
        if(attempt==5)break;if(candidate&&candidate<2000&&!c.figures->at(candidate).alive){slot=candidate;break;}}
    if(!slot)return size_t(0);
    //4C9250 reads84E798+40*type. These group IDs are executable metadata.
    const int speech=type==28?271:type==39?275:type==40?254:-1;
    if(speech<0||!c.figureSpeechCounters||!c.figureSpeechCounts)return std::nullopt;
    const uint32_t divisor=c.figureSpeechCounts->at(unsigned(speech));if(!divisor||divisor>INT32_MAX)return std::nullopt;
    const int64_t cell=int64_t(*c.cropBase)+x+228LL*y;if(cell<0||cell>=51984)return std::nullopt;
    //4C9160+17 receives caller faction;represented faction+7A remains
    //the constructor0. The +15 constructor byte is not freeMovement+14.
    OriginalFigure f;f.slot=slot;f.type=type;f.alive=1;f.direction=direction;f.progress=20;
    (void)faction;
    f.x=f.priorX=f.homeX=f.destinationX=x;f.y=f.priorY=f.homeY=f.destinationY=y;f.cell=uint32_t(cell);
    f.subX=originalBuildingSignedWord(uint16_t(uint32_t(uint16_t(x))*20));f.subY=originalBuildingSignedWord(uint16_t(uint32_t(uint16_t(y))*20));
    f.allocationSerial=uint16_t((*c.figureServices->allocationSerial)++);f.allocationPhase=f.allocationPhaseCopy=uint8_t(originalFigureRandomNext(*c.figureServices)&3);
    auto& counter=c.figureSpeechCounters->at(unsigned(speech));c.figureSpeechChoices[slot]=originalBuildingSignedWord(uint16_t(counter%int32_t(divisor)));
    counter=originalBuildingAdd32(counter,1);
    auto& head=c.figureCellHeads->at(size_t(cell));if(!head)head=uint16_t(slot);else{size_t tail=head;std::set<size_t> seen;
        for(;;){if(tail>=2000||!seen.insert(tail).second)return std::nullopt;auto& prior=c.figures->at(tail);if(prior.cell!=uint32_t(cell))return std::nullopt;
            if(!prior.next){prior.next=uint16_t(slot);break;}tail=prior.next;}}
    c.figures->at(slot)=f;c.figureStationary->operator[](slot)=0;c.allocatedFigures.push_back(slot);
    c.highestFigureSlot=std::max(c.highestFigureSlot,slot);c.highestEverFigureSlot=std::max(c.highestEverFigureSlot,slot);return slot;
}
inline bool originalBuildingBeginServiceRoam(OriginalFigure& f,OriginalBuildingUpdateInputs& i,OriginalBuildingUpdateContext& c){
    if(!c.terrain||!c.terrainOwners||!c.terrainRandom||!c.buildingLinks||!c.activeSide||!c.figureServices)return false;
    OriginalFigureUpdateContext roam;roam.terrain=*c.terrain;roam.buildingOwners=*c.terrainOwners;roam.randomByte=*c.terrainRandom;
    roam.buildings=c.buildingLinks;roam.activeSide=c.activeSide;roam.services=c.figureServices;
    if(!originalFigureBeginRoam(f,roam))return false;c.figureServices=roam.services;i.direction=roam.buildings->at(size_t(f.owner)).roamDirection;return true;
}
//53166F reads1032658 after the four saved10AEE* dwords following
//criminalCount;both gates are serialized,not additional process premises.
inline bool originalBuildingSavedHookContext(const std::vector<uint8_t>& data,size_t terrain,OriginalBuildingUpdateContext& c){
    constexpr size_t countdown=original_save19::criminalCount+20;
    if(terrain>data.size()||original_save19::buildingDisabled+269>data.size()-terrain||countdown+4>data.size()-terrain)return false;
    c.walkersDisabled=data[terrain+original_save19::buildingDisabled+172]!=0;
    c.buildingHookCountdown=originalBuildingI32(detail::u32(data.data()+terrain+countdown));return true;
}
//Per-type+20 targets are individually cited in SAV.md. Guarded service/Hunter
//allocation executes;unported positive dispatch retains the named refusal.
inline OriginalBuildingUpdateBlocker originalBuildingHookStep(OriginalBuildingUpdateState& s,OriginalBuildingUpdateContext& c){
    auto& i=s.inputs;const auto type=s.building.type;
    if(i.alive!=1&&i.alive!=3)return OriginalBuildingUpdateBlocker::None;
    if(type==194||type==196){if(!i.industry)return OriginalBuildingUpdateBlocker::InvalidState;
        if(*c.monthPart==0&&*c.month==(type==194?8u:10u))originalBuildingSetCropFlags(i,uint16_t(originalBuildingCropFlags(i)|0x8000));
        return OriginalBuildingUpdateBlocker::None;}
    if(type==126||type==231)return OriginalBuildingUpdateBlocker::None; //428F00
    if(type>=2&&type<=10)return OriginalBuildingUpdateBlocker::None; //519D60->518D90 false BEFORE road query
    unsigned figureType=0;
    switch(type){case 72:figureType=28;break;case 124:figureType=39;break;case 127:figureType=29;break;
    case 207:figureType=30;break;case 211:figureType=34;break;case 214:figureType=35;break;case 71:figureType=36;break;
    default:break;}
    if(type==71){ //48D950 with param36 bypasses its festival-only37 arm.
        if(!i.parentSlot||!c.buildings||!c.buildings->count(*i.parentSlot)||c.buildings->at(*i.parentSlot).type!=59)
            return OriginalBuildingUpdateBlocker::BuildingVirtual20; //48D9B0 grand-market three-link branch
    }
    if(figureType){ //51CF90 shared+234,exact lazy gate order
        if(i.operational<=0)i.roadFailure=2;
        auto present=originalBuildingHookFigure(i.primaryFigure,figureType,s.building.slot,c);if(!present)return OriginalBuildingUpdateBlocker::MissingContext;
        bool full=*present;
        const bool extra=(type==124||type==127)&& (i.model>0||i.upgrade!=0);
        if((extra||type==71)&&full){if(!i.hookIndustry)return OriginalBuildingUpdateBlocker::MissingContext;
            int16_t second=detail::i16(i.hookIndustry->data()+0x6a-4);auto next=originalBuildingHookFigure(second,figureType,s.building.slot,c);if(!next)return OriginalBuildingUpdateBlocker::MissingContext;
            i.hookIndustry->at(0x6a-4)=uint8_t(second);i.hookIndustry->at(0x6b-4)=uint8_t(uint16_t(second)>>8);full=*next;}
        if(full)return OriginalBuildingUpdateBlocker::None;
        if(!c.walkersDisabled)return OriginalBuildingUpdateBlocker::MissingContext;if(*c.walkersDisabled)return OriginalBuildingUpdateBlocker::None;
        if(!c.poolRoadSelected||!c.poolRoadSelected->count(s.building.slot))return OriginalBuildingUpdateBlocker::MissingContext;
        if(!c.poolRoadSelected->at(s.building.slot))return OriginalBuildingUpdateBlocker::None;
        if(type==71)return OriginalBuildingUpdateBlocker::BuildingVirtual20; //48B780 qualification needs descriptor service
        if(!c.employmentModel||!c.employmentModel->count(type))return OriginalBuildingUpdateBlocker::MissingContext;
        const int required=i.laborDisabled?0:c.employmentModel->at(type);
        int percent=required>0?int(i.workers)*100/required:0;if(percent<1)return OriginalBuildingUpdateBlocker::None;
        if(type==72&&(i.model>0||i.upgrade))percent*=2;
        if(extra)percent=percent*120/100;
        const int limit=type==211||type==214?(percent>=100?3:percent>=75?6:percent>=50?12:percent>=25?24:32):
            percent>=100?1:percent>=75?3:percent>=50?5:percent>=25?10:15;
        ++i.spawnCounter;if(i.spawnCounter>limit){
            if(type!=72&&type!=124)return OriginalBuildingUpdateBlocker::BuildingVirtual20;
            if(!c.poolHookRoadAccess||!c.poolHookRoadAccess->count(s.building.slot))return OriginalBuildingUpdateBlocker::BuildingVirtual20;
            const auto& road=c.poolHookRoadAccess->at(s.building.slot);i.spawnCounter=0;
            const auto slot=originalBuildingAllocateService(c,uint8_t(figureType),i.faction,road.x,road.y,i.direction);
            if(!slot)return OriginalBuildingUpdateBlocker::BuildingVirtual20;if(!*slot)return OriginalBuildingUpdateBlocker::None;
            if(i.primaryFigure>0){if(!i.hookIndustry)return OriginalBuildingUpdateBlocker::MissingContext;
                i.hookIndustry->at(0x6a-4)=uint8_t(*slot);i.hookIndustry->at(0x6b-4)=uint8_t(*slot>>8);}else i.primaryFigure=int16_t(*slot);
            auto& f=c.figures->at(*slot);f.owner=int16_t(s.building.slot);f.action=6;f.counter=30;
            i.direction=uint8_t((i.direction+4)&7);f.previousDirection=i.direction;
            if(!originalBuildingBeginServiceRoam(f,i,c))return OriginalBuildingUpdateBlocker::BuildingVirtual20;
        }
        return OriginalBuildingUpdateBlocker::None;
    }
    if(type==35||type==43){ //5F1280,+29C=5F14A0
        if(i.operational<=0)i.roadFailure=2;
        if(!c.poolRoadSelected||!c.poolRoadSelected->count(s.building.slot))return OriginalBuildingUpdateBlocker::MissingContext;
        if(!c.poolRoadSelected->at(s.building.slot))return OriginalBuildingUpdateBlocker::None;
        const auto present=originalBuildingHookFigure(i.primaryFigure,19,s.building.slot,c);if(!present)return OriginalBuildingUpdateBlocker::MissingContext;
        if(*present)return OriginalBuildingUpdateBlocker::None;
        if(!s.production)return OriginalBuildingUpdateBlocker::MissingContext;
        if(detail::i16(s.production->industry.data())<(type==43?320:160))return OriginalBuildingUpdateBlocker::None;
        return OriginalBuildingUpdateBlocker::BuildingVirtual20;
    }
    if(type==33){ //5F3730:counter gate precedes every cart/hunter allocation.
        if(i.operational<=0)i.roadFailure=2;
        if(!c.poolRoadSelected||!c.poolRoadSelected->count(s.building.slot))return OriginalBuildingUpdateBlocker::MissingContext;
        if(!c.poolRoadSelected->at(s.building.slot))return OriginalBuildingUpdateBlocker::None;
        if(!c.employmentModel||!c.employmentModel->count(type))return OriginalBuildingUpdateBlocker::MissingContext;
        const int required=i.laborDisabled?0:c.employmentModel->at(type),percent=required>0?int(i.workers)*100/required:0;
        if(percent<1)return OriginalBuildingUpdateBlocker::None;
        const unsigned threshold=percent>=100?1:percent>=75?4:percent>=50?16:percent>=25?32:64;
        ++i.spawnCounter;if(i.spawnCounter<=threshold)return OriginalBuildingUpdateBlocker::None;
        i.spawnCounter=0;
        const auto present=originalBuildingHookFigure(i.primaryFigure,19,s.building.slot,c);if(!present)return OriginalBuildingUpdateBlocker::MissingContext;
        if(!*present&&s.production&&s.production->secondaryStock>99)return OriginalBuildingUpdateBlocker::BuildingVirtual20;
        if(!s.production||!i.hookIndustry||!c.figures||!c.poolHookRoadAccess||!c.poolHookRoadAccess->count(s.building.slot))return OriginalBuildingUpdateBlocker::BuildingVirtual20;
        int hunters=percent>=100?3:percent>=75?2:percent>=25?1:0; //4F1520 cap400.
        hunters=std::min(hunters,(400-int(s.production->secondaryStock))/100);
        auto& active=i.hookIndustry->at(0x5c-4);
        if(active>5){active=0;for(const auto& f:*c.figures)if(f.alive==1&&f.type==40&&f.owner==int(s.building.slot))++active;}
        const auto& road=c.poolHookRoadAccess->at(s.building.slot);
        while(int(active)<hunters){const auto slot=originalBuildingAllocateService(c,40,i.faction,road.x,road.y,0);if(!slot)return OriginalBuildingUpdateBlocker::MissingContext;if(!*slot)break;
            auto& f=c.figures->at(*slot);f.owner=int16_t(s.building.slot);f.action=12;f.counter=int16_t((originalFigureRandomNext(*c.figureServices)&31)+20);++active;}
        return OriginalBuildingUpdateBlocker::None;
    }
    if(type==192||type==193){ //4C50E0/4C4B90 ->4C3550 old-byte threshold.
        if(i.operational<=0)i.roadFailure=2;
        if(!c.poolRoadSelected||!c.poolRoadSelected->count(s.building.slot)||!c.employmentModel)return OriginalBuildingUpdateBlocker::MissingContext;
        if(!c.poolRoadSelected->at(s.building.slot))return OriginalBuildingUpdateBlocker::None;
        if(!c.typeDisabled||!c.unavailableResources)return OriginalBuildingUpdateBlocker::MissingContext;if(*c.typeDisabled)return OriginalBuildingUpdateBlocker::None;
        if(type==192){if(c.unavailableResources->at(19)||!i.enabled)return OriginalBuildingUpdateBlocker::None;}
        else{bool all=true;for(unsigned r:{1u,3u,5u,6u,7u})all=all&&c.unavailableResources->at(r)!=0;if(all||!i.enabled)return OriginalBuildingUpdateBlocker::None;}
        const auto required=c.employmentModel->find(type);if(required==c.employmentModel->end())return OriginalBuildingUpdateBlocker::MissingContext;
        const int workers=i.laborDisabled?0:required->second,percent=workers>0?int(i.workers)*100/workers:0;
        if(!percent)return OriginalBuildingUpdateBlocker::None;
        const unsigned limit=percent>=100?1:percent>=75?4:percent>=50?8:percent>=25?16:32;
        const unsigned old=i.spawnCounter;++i.spawnCounter;if(old<limit)return OriginalBuildingUpdateBlocker::None;
        //Positive4C36A0/4C3980 farmer/field allocation requires its registry.
        return OriginalBuildingUpdateBlocker::BuildingVirtual20;
    }
    if(type==53||type==54||type==58||type==59||type==62||type==66||type==67){
        if(!c.walkersDisabled)return OriginalBuildingUpdateBlocker::MissingContext;
        if(*c.walkersDisabled)return OriginalBuildingUpdateBlocker::None;
        if(type==59){const auto present=originalBuildingHookFigure(i.primaryFigure,23,s.building.slot,c);
            if(!present)return OriginalBuildingUpdateBlocker::MissingContext;if(*present)return OriginalBuildingUpdateBlocker::None;}
        if(type==62)return OriginalBuildingUpdateBlocker::None; //541B80 C8(-1)=5408D0 false for empty62.
        if(type==53){ //556130 validates all3 links before its50-percent gate.
            if(!i.hookIndustry||!c.employmentModel||!c.employmentModel->count(type))return OriginalBuildingUpdateBlocker::MissingContext;
            auto first=originalBuildingHookFigure(i.primaryFigure,21,s.building.slot,c);if(!first)return OriginalBuildingUpdateBlocker::MissingContext;
            for(unsigned offset:{0x6au,0x6cu}){int16_t id=detail::i16(i.hookIndustry->data()+offset-4);
                const auto live=originalBuildingHookFigure(id,offset==0x6a?22:21,s.building.slot,c);if(!live)return OriginalBuildingUpdateBlocker::MissingContext;
                i.hookIndustry->at(offset-4)=uint8_t(id);i.hookIndustry->at(offset-3)=uint8_t(uint16_t(id)>>8);}
            if(i.operational<=0)i.roadFailure=2;
            const int required=i.laborDisabled?0:c.employmentModel->at(type),percent=required>0?int(i.workers)*100/required:0;
            if(percent<50)return OriginalBuildingUpdateBlocker::None;
        }
        //Storage/market/stall dispatches have positive supplier/reservation
        //effects;retain the named refusal until those complete services exist.
        return OriginalBuildingUpdateBlocker::BuildingVirtual20;
    }
    return OriginalBuildingUpdateBlocker::BuildingVirtual20;
}
//52A983/52A995 read28-byte header then1000(offset,count) pairs.
//Only the359 groups addressed by the saved counter array are retained.
inline std::optional<std::array<uint32_t,359>> originalBuildingSpeechCounts(const std::vector<uint8_t>& data){
    if(data.size()<8028||data.size()>8u*1024u*1024u||std::string(data.begin(),data.begin()+16)!="Emperor textfile")return std::nullopt;
    std::array<uint32_t,359> out{};for(unsigned n=0;n<359;++n){const size_t at=28+8*n;const uint32_t offset=detail::u32(data.data()+at),count=detail::u32(data.data()+at+4);
        if(offset>data.size()-8028||count>data.size()-8028-offset)return std::nullopt;out[n]=count;}return out;
}
//531895 restores1436 bytes10AEEF8 after three byte grids and terrain services.
inline std::optional<std::array<int32_t,359>> originalBuildingSavedSpeechCounters(const std::vector<uint8_t>& data,size_t terrain){
    constexpr size_t offset=original_save19::serviceGrid+207936+26+13+4*51984+4;
    if(terrain>data.size()||offset+1436>data.size()-terrain)return std::nullopt;std::array<int32_t,359> out{};
    for(unsigned n=0;n<359;++n)out[n]=originalBuildingI32(detail::u32(data.data()+terrain+offset+4*n));return out;
}
//5D3CB0 resets29 entries,then visits active Goods records from1.
inline OriginalBuildingUpdateBlocker originalBuildingStorageStep(OriginalBuildingUpdateContext& c){
    if(!c.goodsRecords||!c.buildingLinks||!c.buildingInputs||!c.storageRoadStatus||!c.goods)return OriginalBuildingUpdateBlocker::CityPhase;
    for(unsigned n=0;n<29;++n)c.goods->warehouse[n]=c.goods->trade5[n]=c.goods->trade7[n]=0;
    for(size_t n=1;n<c.goodsRecords->size();++n){const auto& g=c.goodsRecords->at(n);if(!g.alive)continue;
        const auto found=c.buildingLinks->find(size_t(g.owner));if(found==c.buildingLinks->end())return OriginalBuildingUpdateBlocker::CityPhase;
        auto& b=found->second;if((b.type!=53&&b.type!=54&&b.type!=58)||(b.alive!=1&&b.alive!=3))continue;
        const auto road=c.storageRoadStatus->find(found->first);auto input=c.buildingInputs->find(found->first);
        if(road==c.storageRoadStatus->end()||input==c.buildingInputs->end()||!b.storage)return OriginalBuildingUpdateBlocker::CityPhase;
        b.roadAccess=input->second.available=road->second;if(!road->second)continue;
        if(!b.storagePolicy||*b.storagePolicy>=c.goods->policies.size())return OriginalBuildingUpdateBlocker::CityPhase;
        const auto& policy=c.goods->policies[*b.storagePolicy];for(const auto& compartment:*b.storage){if(compartment.resource>=36)return OriginalBuildingUpdateBlocker::CityPhase;
            if(b.type==53||b.type==54)c.goods->warehouse[compartment.resource]+=compartment.amount;
            else if(policy.trade[compartment.resource]==7)c.goods->trade7[compartment.resource]+=compartment.amount;
            else if(policy.trade[compartment.resource]==5)c.goods->trade5[compartment.resource]+=compartment.amount;}}
    ++c.storageCalls;return OriginalBuildingUpdateBlocker::None;
}
//4AEDF0 ->591490 includes storage reset,then Mill+58 and house+12.
inline OriginalBuildingUpdateBlocker originalBuildingFoodStep(OriginalBuildingUpdateContext& c){
    if(!c.scenarioFoodOverride||*c.scenarioFoodOverride||!c.buildingInputs||!c.buildingLinks||!c.poolRoadSelected||!c.populationTotal)return OriginalBuildingUpdateBlocker::CityPhase;
    const auto storage=originalBuildingStorageStep(c);if(storage!=OriginalBuildingUpdateBlocker::None)return storage;
    if(!c.cityDerived)c.cityDerived.emplace();auto& d=*c.cityDerived;d.food=0;d.millFood.fill(0);
    for(const auto& e:*c.buildingInputs){const auto& i=e.second;if(i.house&&(i.alive==1||i.alive==3)&&i.residents)d.food=originalBuildingAdd32(d.food,detail::i16(i.house->data()+14));}
    for(auto& e:*c.buildingLinks){auto& b=e.second;if(b.type!=53||(b.alive!=1&&b.alive!=3))continue;
        const auto road=c.poolRoadSelected->find(e.first);const auto input=c.buildingInputs->find(e.first);
        if(road==c.poolRoadSelected->end()||input==c.buildingInputs->end()||!b.storage)return OriginalBuildingUpdateBlocker::CityPhase;
        b.roadAccess=input->second.available=uint8_t(road->second);if(!road->second)continue;
        for(const auto& stock:*b.storage)if(stock.resource>=1&&stock.resource<=9)d.millFood[stock.resource]=originalBuildingAdd32(d.millFood[stock.resource],originalBuildingI32(stock.amount));}
    for(unsigned n=1;n<10;++n)d.food=originalBuildingAdd32(d.food,d.millFood[n]);d.foodConsumption=originalBuildingPercent(*c.populationTotal,25);
    d.foodMonths=d.foodConsumption>0?d.food/d.foodConsumption:int32_t(d.food>0);
    d.warehouse=c.goods->warehouse;d.trade5=c.goods->trade5;d.trade7=c.goods->trade7;++c.foodCalls;return OriginalBuildingUpdateBlocker::None;
}
inline OriginalBuildingUpdateBlocker originalBuildingAccountingStep(OriginalBuildingUpdateContext& c){
    if(!c.cityEmployment||!c.accounting||!c.formations||!c.figures||!c.figureModels||!c.figureStationary)
        return OriginalBuildingUpdateBlocker::CityPhase;
    auto employment=originalBuildingCityEmploymentStep(*c.cityEmployment);if(!employment)return OriginalBuildingUpdateBlocker::CityPhase;
    auto formation=originalBuildingFormationStep(*c.formations,*c.figures,*c.figureModels,*c.figureStationary);
    if(!formation)return OriginalBuildingUpdateBlocker::CityPhase;
    if(!c.buildingInputs||!c.buildings||!c.houseTaxModel||!c.scenarioFoodOverride||!c.month||!c.populationTotal||!c.serviceCoverage)
        return OriginalBuildingUpdateBlocker::CityPhase;
    auto& a=*c.accounting;a.taxableOrdinary=a.taxableElite=0;OriginalBuildingUpdateContext::CityDerived derived;
    derived.serviceCoverage=*c.serviceCoverage;std::array<int32_t,11> served{};
    for(const auto& entry:*c.buildingInputs){const auto& i=entry.second;const auto f=c.buildings->find(entry.first);
        if(f==c.buildings->end())return OriginalBuildingUpdateBlocker::CityPhase;
        if(f->second.type>=2&&f->second.type<=17){if(!i.house)return OriginalBuildingUpdateBlocker::CityPhase;
            if(i.alive==1&&i.byte52){auto model=c.houseTaxModel->find(i.model);if(model==c.houseTaxModel->end())return OriginalBuildingUpdateBlocker::CityPhase;
                auto& taxable=f->second.type>10?a.taxableElite:a.taxableOrdinary;taxable=originalBuildingAdd32(taxable,originalBuildingI32(uint32_t(i.residents)*uint32_t(model->second)));}
            if((i.alive==1||i.alive==3)&&i.residents){derived.population=originalBuildingAdd32(derived.population,i.residents);
                //590C00(+2B/+2C/+2E/+2A),590B30(+33). wire offsets -1.
                 constexpr unsigned wire[]={42,43,45,41,50},columns[]={1,2,3,0,10};for(unsigned k=0;k<5;++k)if(i.house->at(wire[k]-4))served[columns[k]]=originalBuildingAdd32(served[columns[k]],i.residents);
                   }}}
    //4AF2D0 writes only0..3 and7..10;4..6 survive the call.
    for(unsigned k:{0u,1u,2u,3u,7u,8u,9u,10u}){auto ratio=originalBuildingRatio(served[k],derived.population);if(!ratio)return OriginalBuildingUpdateBlocker::CityPhase;derived.serviceCoverage[k]=*ratio;}
    c.cityDerived=derived;c.serviceCoverage=derived.serviceCoverage;
    const auto food=originalBuildingFoodStep(c);if(food!=OriginalBuildingUpdateBlocker::None)return food;
    //44B9B0/44BAB0 table81E748:7 rows,index,7 ratings,6 bounds.
    constexpr int index[]={0,1,2,3,7,9,10};
    constexpr int ratings[7][7]={{5,15,25,40,55,75,100},{10,20,30,45,60,80,100},{15,25,35,50,65,85,100},
        {50,55,65,75,85,95,100},{15,20,25,35,50,65,100},{20,25,30,40,55,70,100},{35,45,55,70,85,100,100}};
    c.serviceRating=100;c.servicePenalty=0;std::array<int,7> rowRatings{};
    for(unsigned n=0;n<7;++n){const int input=derived.serviceCoverage[unsigned(index[n])];
        unsigned band=input<=0?0:input<20?1:input<40?2:input<60?3:input<80?4:input<(n==4||n==5?90:100)?5:6;
        rowRatings[n]=ratings[n][band];if(rowRatings[n]<c.serviceRating){c.serviceRating=rowRatings[n];c.serviceWeakest=int32_t(n);}}
    for(unsigned n=0;n<7;++n){const int input=derived.serviceCoverage[unsigned(index[n])];const unsigned band=input<1?0:input<20?1:input<40?2:input<60?3:input<80?4:input<100?5:6;
        if(ratings[n][band]+30<c.serviceRating)c.servicePenalty+=5;}
    const int32_t months=*c.month==0?1:13-int32_t(*c.month);
    a.ordinaryTax=originalBuildingPercent(a.taxableOrdinary,a.taxRate);a.eliteTax=originalBuildingPercent(a.taxableElite,a.taxRate);
    a.taxTotal=originalBuildingAdd32(a.taxReceived,a.taxOther);
    a.taxForecast=originalBuildingAdd32(originalBuildingI32(uint32_t(months)*uint32_t(originalBuildingAdd32(a.ordinaryTax,a.eliteTax))),a.taxTotal);
    a.wageTotal=a.wagesPaid;
    const int32_t monthly=originalBuildingI32(uint32_t(a.wages)*uint32_t(employment->state.employed))/120;
    a.wageForecast=originalBuildingAdd32(originalBuildingI32(uint32_t(months)*uint32_t(monthly)),a.wagesPaid);
    a.income=originalBuildingAdd32(a.incomeOther,a.taxTotal);a.expenses=originalBuildingAdd32(a.expensesOther,a.wageTotal);
    a.profit=originalBuildingSub32(a.income,a.expenses);a.actualProfit=originalBuildingSub32(a.incomeReceived,a.expensesPaid);
    a.treasuryForecast=originalBuildingAdd32(a.treasuryStart,a.profit);
    a.festivalCapacity=std::min(a.festivalCount,9);c.cityEmployment=employment->state;c.formations=formation->formations;c.formationResult=*formation;c.figureOrdinals=formation->figureOrdinals;
    if(employment->shortageNotification)c.notificationEffects.push_back(0x548210);
    if(employment->unemploymentNotification)c.notificationEffects.push_back(0x548270);
    ++c.accountingCalls;++c.formationCalls;return OriginalBuildingUpdateBlocker::None;
}
//4AEEB0:scan the complete Goods registry from1,mode2 suppresses
//stock contribution;the nine calls with resource0 count all matching food.
inline OriginalBuildingUpdateBlocker originalBuildingGoodsPoolStep(OriginalBuildingUpdateContext& c){
    if(!c.goodsRecords||!c.buildingLinks)return OriginalBuildingUpdateBlocker::CityPhase;
    c.millAvailable.fill(0);c.millCandidates.fill(0);c.millCandidateCount=0;
    for(size_t n=1;n<c.goodsRecords->size();++n){const auto& g=c.goodsRecords->at(n);if(!g.alive||!g.owner)continue;
        const auto found=c.buildingLinks->find(size_t(g.owner));if(found==c.buildingLinks->end())return OriginalBuildingUpdateBlocker::CityPhase;
        const auto& b=found->second;if((b.alive!=1&&b.alive!=3)||b.type!=53||!b.roadAccess||b.roadDistance<=0)continue;
        if(!b.storage)return OriginalBuildingUpdateBlocker::CityPhase;int32_t total=0,all=0;
        for(const auto& stock:*b.storage)if(stock.resource>=1&&stock.resource<=9)all=originalBuildingAdd32(all,originalBuildingI32(stock.amount));
        for(unsigned r=1;r<10;++r)if(g.mode[r]!=2){total=originalBuildingAdd32(total,all);
            for(const auto& stock:*b.storage)if(stock.resource==r)c.millAvailable[r]=originalBuildingAdd32(c.millAvailable[r],originalBuildingI32(stock.amount));}
        if(total>100){c.millCandidates[c.millCandidateCount]=g.owner;if(c.millCandidateCount<98)++c.millCandidateCount;}}
    ++c.goodsPoolCalls;return OriginalBuildingUpdateBlocker::None;
}
inline OriginalBuildingUpdateBlocker originalBuildingNetworkStep(OriginalBuildingUpdateContext& c){
    if(!c.movement||!c.terrain||!c.cropBase||!c.activeSide)return OriginalBuildingUpdateBlocker::RoadRefresh;
    OriginalFigureUpdateContext input;input.passability=c.movement;input.terrain=*c.terrain;input.cropBase=c.cropBase;input.activeSide=c.activeSide;
    const auto networks=originalFigureRoadNetworks(input);if(!networks)return OriginalBuildingUpdateBlocker::RoadRefresh;
    const auto ranks=originalBuildingRankedRoadNetworks(*networks);if(!ranks)return OriginalBuildingUpdateBlocker::RoadRefresh;
    c.roadNetworks=*networks;c.rankedNetworks=*ranks;++c.networkCalls;return OriginalBuildingUpdateBlocker::None;
}
inline OriginalBuildingUpdateBlocker originalBuildingPoolRoadStep(OriginalBuildingUpdateContext& c){
    if(!c.poolRoadAccess||!c.buildingInputs)return OriginalBuildingUpdateBlocker::RoadRefresh;
    for(auto& entry:*c.buildingInputs){auto& i=entry.second;if(i.alive!=1&&i.alive!=3)continue;
        const auto access=c.poolRoadAccess->find(entry.first);if(access==c.poolRoadAccess->end())return OriginalBuildingUpdateBlocker::RoadRefresh;
        i.distance=originalBuildingSignedWord(access->second.distance);i.roadX=access->second.x;i.roadY=access->second.y;
        if(access->second.network)i.network=*access->second.network;}
    return OriginalBuildingUpdateBlocker::None;
}
//5253A0 ->525540 then525470. The reset scans active rows; each
//staffed irrigation source clears its own visited grid and walks N,E,S,W.
//Use an explicit DFS stack to retain recursive order without host stack limits.
inline OriginalBuildingUpdateBlocker originalBuildingIrrigationStep(OriginalBuildingUpdateContext& c){
    if(!c.irrigation||!c.terrain||c.terrain->size()!=51984||!c.cropBase||!c.activeSide||*c.activeSide>228)
        return OriginalBuildingUpdateBlocker::MissingContext;
    auto& s=*c.irrigation;if(s.image.size()!=51984||s.subTile.size()!=51984||s.owners.size()!=51984||s.baseImage>INT32_MAX-18)
        return OriginalBuildingUpdateBlocker::InvalidState;
    const auto base=int32_t(s.baseImage);
    for(unsigned y=0;y<*c.activeSide;++y)for(unsigned x=0;x<*c.activeSide;++x){const uint64_t cell=uint64_t(*c.cropBase)+228u*y+x;
        if(cell>=51984)return OriginalBuildingUpdateBlocker::InvalidState;const size_t p=size_t(cell);const auto flags=c.terrain->at(p);
        //525540's final owner==0 gate excludes the earlier Roadblock allowance.
        if((flags&0x800)&&!(flags&4)&&!s.owners[p]&&!(s.subTile[p]&0x80)&&originalBuildingI32(s.image[p])<base+18)s.image[p]+=18;}
    constexpr int offsets[4][4]={{456,-1,-228,2},{457,227,-227,230},{227,-228,2,-227},{230,456,-1,457}}; //820CE0..820D1F
    for(const auto& source:s.sources){if(source.workers<=0)continue;if(source.orientation>=4||source.cell>=51984)return OriginalBuildingUpdateBlocker::InvalidState;
        std::vector<uint8_t> visited(51984);for(unsigned entrance=0;entrance<4;++entrance){
            std::vector<int64_t> stack{int64_t(source.cell)+offsets[entrance][source.orientation]};
            while(!stack.empty()){const auto cell=stack.back();stack.pop_back();if(cell<0||cell>=51984)return OriginalBuildingUpdateBlocker::InvalidState;
                const size_t p=size_t(cell);if(visited[p])continue;const auto flags=c.terrain->at(p);if(!(flags&0x800)||(flags&0x80004))continue;
                const auto owner=s.owners[p];if(owner){if(owner>=4000)return OriginalBuildingUpdateBlocker::InvalidState;if(s.ownerTypes[owner]!=126)continue;}
                else if(originalBuildingI32(s.image[p])>=base+18)s.image[p]-=18;
                visited[p]=1;for(int d:{-1,228,1,-228})stack.push_back(cell+d);}}}
    if(c.resourceGrowth)c.resourceGrowth->surface.image=s.image;
    ++c.irrigationCalls;return OriginalBuildingUpdateBlocker::None;
}
//4ACE30 resets C82F14, then5179B0 derives runtime+39/+43.
inline OriginalBuildingUpdateBlocker originalBuildingWaterStep(OriginalBuildingUpdateContext& c){
    if(!c.buildingInputs)return OriginalBuildingUpdateBlocker::MissingContext;c.waterScratch=0;
    for(auto& e:*c.buildingInputs){auto& i=e.second;if((i.alive!=1&&i.alive!=3)||!i.house)continue;
        //Common wire52/61 are runtime+39/+43 (427E41/427E8F).
        i.houseWater=i.house->at(51-4)?2:i.house->at(49-4)?1:0;i.houseService=0;}
    return OriginalBuildingUpdateBlocker::None;
}
//480800 Empire3 framing after Goods1;43E7A3 restores Grid15+5.
inline std::optional<std::array<uint8_t,22>> originalBuildingSavedEmpireStates(const std::vector<uint8_t>& data,size_t at,
    OriginalBuildingUpdateContext* city=nullptr){
    using namespace emperor::file_io::research;if(data.size()>8u*1024u*1024u||at>data.size())return std::nullopt;
    const auto take=[&](size_t n){if(n>data.size()-at)return false;at+=n;return true;};
    const auto version=[&](uint16_t v){return take(2)&&detail::u16(data.data()+at-2)==v;};
    if(!version(1)||!take(4))return std::nullopt;const auto goods=detail::u32(data.data()+at-4);if(goods>8192)return std::nullopt;
    for(size_t n=0;n<goods;++n){if(!version(1))return std::nullopt;for(const auto& f:goods1_fields)if(!take(f.bytes))return std::nullopt;}
    if(!version(3)||!take(4))return std::nullopt;const auto count=detail::u32(data.data()+at-4);if(count>8192)return std::nullopt;
    struct Place {uint8_t alive=0,faction=0;};std::vector<Place> places(count);
    for(size_t n=0;n<count;++n){if(!version(2))return std::nullopt;for(const auto& f:empire2_fields){if(!take(f.bytes))return std::nullopt;
        if(f.receiver_offset==4)places[n].alive=data[at-f.bytes];if(f.receiver_offset==28)places[n].faction=data[at-f.bytes];}}
    std::array<uint8_t,22> out{};
    std::array<std::array<uint8_t,29>,22> production{};
    std::array<int16_t,22> placeIDs{};
    OriginalBuildingUpdateContext::Trade trade;
    for(unsigned n=0;n<22;++n){if(!version(15))return std::nullopt;
        auto& partner=trade.partners[n];
        for(const auto& f:empire_grid15_pre){if(!take(f.bytes))return std::nullopt;const auto* p=data.data()+at-f.bytes;if(f.receiver_offset==5)out[n]=p[0];
            if(f.receiver_offset==4)partner.alive=p[0];if(f.receiver_offset==6)partner.kind=p[0];
            if(f.receiver_offset==40)partner.faction=p[0];if(f.receiver_offset==51)partner.sea=p[0];
            if(f.receiver_offset==44)for(unsigned k=0;k<3;++k)partner.traders[k]=detail::i16(p+2*k);
            if(f.receiver_offset==74)partner.timer=detail::i16(p);if(f.receiver_offset==76)partner.suspended=detail::i16(p);
            if(f.receiver_offset==152)for(unsigned k=0;k<36;++k)partner.quotaBonus[k]=p[24*k];
            if(f.receiver_offset==1016)std::copy_n(p,36,partner.quota.begin());
            if(f.receiver_offset==1118)std::copy_n(p,4,partner.buys.begin());
            if(f.receiver_offset==1122)std::copy_n(p,4,partner.sells.begin());
            if(f.receiver_offset==42)placeIDs[n]=detail::i16(data.data()+at-f.bytes);
            if(f.receiver_offset<=0x440&&f.receiver_offset+f.bytes>=0x440+29)std::copy_n(data.begin()+at-f.bytes+(0x440-f.receiver_offset),29,production[n].begin());}
        for(unsigned k=0;k<22;++k){if(!version(5))return std::nullopt;for(const auto& f:empire_grid_child5_fields){if(!take(f.bytes))return std::nullopt;
            if(city&&city->statistics&&int(k)==city->statistics->currentFaction&&f.receiver_offset==116)
                partner.relationship=originalBuildingI32(detail::u32(data.data()+at-f.bytes));}}
        for(const auto& f:empire_grid15_post){if(!take(f.bytes))return std::nullopt;
            if(f.receiver_offset==15052)partner.enabled=originalBuildingI32(detail::u32(data.data()+at-f.bytes));
            if(f.receiver_offset==15056)partner.blocked=data[at-f.bytes];}}
    if(!take(4))return std::nullopt;
    if(city){if(!city->statistics||city->statistics->currentFaction<0||city->statistics->currentFaction>=22)return std::nullopt;
        const auto id=placeIDs[size_t(city->statistics->currentFaction)];if(id<0||size_t(id)>=places.size())return std::nullopt;
        const auto place=places[size_t(id)];city->empireProduction=std::array<bool,36>{};
        if(place.alive==1&&place.faction<22)for(auto resource:production[place.faction])if(resource<36)city->empireProduction->at(resource)=true;
        city->trade=std::move(trade);}
    return out;
}
inline OriginalBuildingUpdateBlocker originalBuildingUnavailableStep(OriginalBuildingUpdateContext& c){
    if(!c.buildings||!c.buildingInputs||!c.unavailableResources||!c.empireProduction)return OriginalBuildingUpdateBlocker::Employment;
    c.buildingUnavailable.emplace();for(const auto& e:*c.buildingInputs){const auto type=c.buildings->at(e.first).type;const auto& i=e.second;bool unavailable=!i.enabled;
        if(!unavailable){if(type==194||type==196)unavailable=false;
            else if(type==192)unavailable=c.unavailableResources->at(19)!=0||!c.empireProduction->at(19);
            else if(type==193){unavailable=true;for(unsigned r:{1u,3u,5u,6u,7u})if(!c.unavailableResources->at(r)&&c.empireProduction->at(r))unavailable=false;}
            else {if(i.output>=36)return OriginalBuildingUpdateBlocker::Employment;unavailable=c.unavailableResources->at(i.output)!=0;}}
        c.buildingUnavailable->emplace(e.first,unavailable);}
    return OriginalBuildingUpdateBlocker::None;
}
//The four adjacent Population7 shorts are serialized by59498E..5949BE;
//the outer5311DC reader alone cannot establish whether a global is saved.
inline bool originalBuildingSavedTradeContext(const std::vector<uint8_t>& data,size_t side,OriginalBuildingUpdateContext& c){
    if(!c.trade||side<12287||side>data.size())return false;const size_t p=side-12287;
    if(detail::u16(data.data()+p)!=7)return false;
    for(unsigned n=0;n<4;++n)c.trade->counters[n]=detail::i16(data.data()+p+11091+2*n);
    return true;
}
//5DBCC0:eligibility441690->441640/440470,resource classification
//443E20/443E50->441590,then the first vacant reciprocal trader slot.
//Positive creation remains an explicit refusal until its complete constructor
//and three-figure caravan registration are supplied;nonempty quotas/timers run.
inline OriginalBuildingUpdateBlocker originalBuildingTradeStep(OriginalBuildingUpdateContext& c){
    if(!c.trade||!c.statistics||!c.figures||c.figures->size()!=2000||!c.buildings||!c.buildingInputs)
        return OriginalBuildingUpdateBlocker::Trade;
    const int faction=c.statistics->currentFaction;if(faction<0||faction>=22)return OriginalBuildingUpdateBlocker::InvalidState;
    auto& t=*c.trade;t.counters[0]=t.counters[1]=0;
    for(unsigned n=2;n<4;++n)t.counters[n]=int16_t(std::max(0,int(t.counters[n])-1));
    t.eligible=0;
    size_t seaBuildings=0;for(const auto& b:*c.buildings)if(b.second.type==56)++seaBuildings;
    for(auto& p:t.partners){
        if(!p.alive||p.kind==5||p.kind==6||(p.kind==0&&p.faction==faction)||!p.enabled||(p.blocked&4)||p.relationship)continue;
        if(p.sea==1){if(!seaBuildings)continue;if(!t.seaEntry)return OriginalBuildingUpdateBlocker::MissingContext;
            if(t.seaEntry->at(0)==-1&&t.seaEntry->at(1)==-1)continue;}
        ++t.eligible;auto& total=t.counters[p.sea==1?0:1];total=originalBuildingSignedWord(uint16_t(uint16_t(total)+1));
        unsigned resources=0,weight=0;
        for(unsigned r=1;r<29;++r){if(std::find(p.buys.begin(),p.buys.end(),r)==p.buys.end()&&std::find(p.sells.begin(),p.sells.end(),r)==p.sells.end())continue;
            unsigned quota=p.quota[r];if(p.quotaBonus[r]&&quota)quota=quota==12?24:36;
            if(!quota)continue;++resources;weight+=quota>=25?3:quota>=13?2:1;}
        const unsigned wanted=resources>1?(weight+resources-1)/resources:weight;
        if(!wanted||p.suspended)continue;
        bool vacancy=false;for(unsigned k=0;k<std::min(3u,wanted);++k){const int id=p.traders[k];
            if(id<0||id>=2000)return OriginalBuildingUpdateBlocker::InvalidState;
            if(!id||!c.figures->at(size_t(id)).alive||c.figures->at(size_t(id)).action==4){vacancy=true;break;}}
        if(!vacancy)continue;
        if(p.timer>0){--p.timer;++t.timersChanged;continue;}
        if(!t.dispatchDelay||!c.statistics->religionBonusEnabled)return OriginalBuildingUpdateBlocker::MissingContext;
        int32_t delay=t.dispatchDelay->at(p.sea==1?0:1);
        if(p.faction!=faction&&*c.statistics->religionBonusEnabled){if(!c.statistics->religionBonus)return OriginalBuildingUpdateBlocker::MissingContext;
            if(*c.statistics->religionBonus==11)delay=delay>=0?delay/2:-int32_t((-(int64_t(delay))+1)/2);}
        p.timer=originalBuildingSignedWord(uint16_t(delay));
        if(t.counters[p.sea==1?2:3])continue;
        const int partnerIndex=int(&p-t.partners.data()); //5DBB00 receives the Grid ordinal.
        for(const auto& b:*c.buildings)if((b.second.type==56||b.second.type==58)&&c.buildingInputs->at(b.first).model==partnerIndex&&
            (c.buildingInputs->at(b.first).alive==1||c.buildingInputs->at(b.first).alive==3))return OriginalBuildingUpdateBlocker::Trade;
    }
    //4B02A0's562F70 predicate is a literal type switch,not an empty-pool premise.
    for(const auto& b:*c.buildings){const auto type=b.second.type;
        if((type>=76&&type<=86)||type==92||type==93||(type>=253&&type<=268))return OriginalBuildingUpdateBlocker::Trade;}
    ++t.calls;return OriginalBuildingUpdateBlocker::None;
}
//4AF230 resets all269 type rows and30 resource rows,then ordered+9C.
//The resource arrays retain entries30..35;ordinary type arrays reset entirely.
inline OriginalBuildingUpdateBlocker originalBuildingTotalsStep(OriginalBuildingUpdateContext& c){
    if(!c.buildingInputs||!c.buildings||!c.figures||!c.employmentModel)return OriginalBuildingUpdateBlocker::MissingContext;
    if(!c.buildingTotals)return OriginalBuildingUpdateBlocker::MissingContext;auto& totals=*c.buildingTotals;totals.total.fill(0);totals.staffed.fill(0);
    std::fill_n(totals.producers.begin(),30,0);std::fill_n(totals.staffedProducers.begin(),30,0);
    for(auto& e:*c.buildingInputs){auto& i=e.second;if(i.alive!=1&&i.alive!=3)continue;
        const auto found=c.buildings->find(e.first);if(found==c.buildings->end()||found->second.type>=269)return OriginalBuildingUpdateBlocker::InvalidState;
        const auto type=found->second.type;
        if(originalBuildingIsHouse(type)){if(!i.house)return OriginalBuildingUpdateBlocker::InvalidState;
            if(i.houseInUse){++totals.total[type];if(i.residents)++totals.staffed[type];}
            continue;}
        if(type==56)return OriginalBuildingUpdateBlocker::UnclosedPhase;
        ++totals.total[type];
        if(type==71){++totals.staffed[type];} //+1B4=4F1540 returns100.
        else {const auto model=c.employmentModel->find(type);if(model==c.employmentModel->end())return OriginalBuildingUpdateBlocker::MissingContext;
            if(!i.laborDisabled&&model->second>0&&int(i.workers)*100/model->second>0)++totals.staffed[type];}
        if(type==33||type==35||type==43||type==194||type==196){
            const unsigned resource=type==194?19:type==196?1:i.output;if(resource>=36)return OriginalBuildingUpdateBlocker::InvalidState;
            ++totals.producers[resource];if(i.workers>0)++totals.staffedProducers[resource];}
        if(type==71||type==211){if(!i.hookIndustry)return OriginalBuildingUpdateBlocker::MissingContext;unsigned active=0;
            for(unsigned off:{0x5du,0x5eu,0x5fu}){auto& v=i.hookIndustry->at(off-4);if(v){--v;++active;}}i.hookIndustry->at(0x5c-4)=uint8_t(active);}
        if(i.migrant){if(i.migrant<0||size_t(i.migrant)>=c.figures->size())return OriginalBuildingUpdateBlocker::InvalidState;
            const auto& f=c.figures->at(size_t(i.migrant));if(f.alive!=1||f.delivery!=int(e.first))i.migrant=0;}
    }
    ++totals.calls;return OriginalBuildingUpdateBlocker::None;
}
//5D1830 multiplies each ALL BUILDINGS column by its selected BUILDING
//MODS percentage,then44CC50 indexes13*dword. No executable/table IO here.
inline std::optional<std::map<uint16_t,std::array<int32_t,13>>> originalBuildingModels(const std::string& text,unsigned difficulty){
    if(difficulty>=5||text.size()>1048576||text.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream input(text);std::string line;unsigned section=0,mods=0;std::array<int32_t,13> modifier{};
    std::map<uint16_t,std::array<int32_t,13>> out;
    while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line=="BUILDING MODS"){section=1;continue;}if(line=="ALL BUILDINGS"){if(mods!=5)return std::nullopt;section=2;continue;}
        if(line=="HOUSE MODS"){if(section!=2||out.size()!=269)return std::nullopt;return out;}
        const auto open=line.find("{,");if(!section||open==std::string::npos)continue;
        std::string fields=line.substr(open+2);std::replace(fields.begin(),fields.end(),',',' ');std::istringstream values(fields);std::array<int32_t,13> row{};
        for(auto& v:row)if(!(values>>v))return std::nullopt;
        if(section==1){if(mods==difficulty)modifier=row;if(++mods>5)return std::nullopt;}
        else{if(out.size()>=269)return std::nullopt;for(unsigned k=0;k<13;++k)row[k]=originalBuildingPercent(row[k],modifier[k]);out.emplace(uint16_t(out.size()),row);}}
    return std::nullopt;
}
//Size1 uses4B0710's y/x perimeter. Size2..6 use44CDE0's generated
//top from x0,then right,bottom,left,remainder top. Order affects shadows.
inline std::vector<std::pair<int,int>> originalBuildingInfluenceRing(unsigned size,unsigned radius){
    std::vector<std::pair<int,int>> out;if(!size||size>6||!radius||radius>10)return out;
    const int left=-int(radius),right=int(size)+int(radius)-1;int x=0,y=left;
    if(size==1){for(int yy=left;yy<=right;++yy)for(int xx=left;xx<=right;++xx)if(xx==left||xx==right||yy==left||yy==right)out.emplace_back(xx,yy);return out;}
    for(;x<right;++x)out.emplace_back(x,y);for(;y<right;++y)out.emplace_back(x,y);
    for(;x>left;--x)out.emplace_back(x,y);for(;y>left;--y)out.emplace_back(x,y);
    for(;x<0;++x)out.emplace_back(x,y);return out;
}
inline OriginalBuildingUpdateBlocker originalBuildingDesirabilityStep(OriginalBuildingUpdateContext& c){
    if(!c.buildingModels||!c.terrain||!c.terrainOwners||!c.resourceGrowth||!c.activeSide||!c.cropBase||!c.buildings||!c.buildingInputs)
        return OriginalBuildingUpdateBlocker::MissingContext;
    const auto& feature=c.resourceGrowth->surface.feature;auto& sub=c.resourceGrowth->surface.subTile;
    if(feature.size()!=51984||sub.size()!=51984||c.terrain->size()!=51984||c.terrainOwners->size()!=51984)return OriginalBuildingUpdateBlocker::InvalidState;
    std::vector<int8_t> grid(51984);
    const auto influence=[&](int x,int y,unsigned size,int value,int step,int change,int range)->bool{
        if(!size||size>6||range>10)return false;if(!value)return true;const bool negative=value<0;std::array<int,16> shadows{};
        for(int radius=1;radius<=range;++radius){const auto ring=originalBuildingInfluenceRing(size,unsigned(radius));for(const auto& offset:ring){
            const int xx=x+offset.first,yy=y+offset.second;if(xx< -1||yy< -1||xx>int(*c.activeSide)||yy>int(*c.activeSide))continue;
            const bool clipped=x-radius< -1||y-radius< -1||x+int(size)+radius-1>int(*c.activeSide)||y+int(size)+radius-1>int(*c.activeSide);
            //44F0DE's clipped branch tests each perimeter coordinate,but
            //repeatedly writes101C560 (the source origin),without shadows.
            const int64_t cell=int64_t(*c.cropBase)+228LL*(clipped?y:yy)+(clipped?x:xx);if(cell<0||cell>=51984)return false;const size_t at=size_t(cell);
            //44ED90 uses x*4/y*4 (+size*2 for size>1) in its sector call.
            const int sector=originalFigureServiceSector(xx-4*x-(size>1?int(2*size):0),yy-4*y-(size>1?int(2*size):0));
            if(!clipped&&value<0){if(feature[at]&4){if(sector>=0){if(!shadows[size_t(sector)]||radius<shadows[size_t(sector)])shadows[size_t(sector)]=radius;
                        if(ring.size()<16&&!(sector&1)){if(shadows[size_t((sector+2)&15)]==1)shadows[size_t((sector+1)&15)]=radius;if(shadows[size_t((sector+14)&15)]==1)shadows[size_t((sector+15)&15)]=radius;}}continue;}
                if(sector>=0&&shadows[size_t(sector)]&&radius>shadows[size_t(sector)])continue;}
            grid[at]=int8_t(std::clamp(originalBuildingAdd32(int(grid[at]),value),-100,100));}
            if(step<=0||radius%step==0){const int next=originalBuildingAdd32(value,change);value=negative?std::min(0,next):std::max(0,next);}}
        return true;};
    for(const auto& b:*c.buildings){const auto& i=c.buildingInputs->at(b.first);if((i.alive!=1&&i.alive!=3)||i.parent)continue;
        const auto model=c.buildingModels->find(b.second.type);if(model==c.buildingModels->end())return OriginalBuildingUpdateBlocker::MissingContext;
        const auto& m=model->second;
        if(!m[1]||m[4]<=0)continue;
        //Market+16C delegates542D10 to the28-row orientation descriptor.
        //Its last row is(3,6)/(6,3),so these are inclusive bounds.
        if(b.second.type==59){
            if(!b.second.imageInputs||!b.second.imageInputs->marketOrientation||*b.second.imageInputs->marketOrientation>1)return OriginalBuildingUpdateBlocker::InvalidState;
            //44E7F0 consults process8254E4 for nonnegative influence. Until
            //that input is supplied,positive rectangular influence refuses.
            if(m[1]>=0)return OriginalBuildingUpdateBlocker::MissingContext;
            const bool rotated=*b.second.imageInputs->marketOrientation==1;
            int left=b.second.x,top=b.second.y,right=left+(rotated?6:3),bottom=top+(rotated?3:6),value=m[1],counter=1;
            std::array<int,16> shadows{};
            for(int radius=1;radius<=std::min(10,m[4]);++radius){
                if(value>=0)return OriginalBuildingUpdateBlocker::MissingContext; //8254E4,including sign changes.
                const int centerX=((left+right)/2)*4,centerY=((top+bottom)/2)*4;
                for(int y=top-1;y<=bottom+1;++y)for(int x=left-1;x<=right+1;++x){
                    if(y!=top-1&&y!=bottom+1&&x!=left-1&&x!=right+1)continue;
                    const int64_t cell=int64_t(*c.cropBase)+228LL*y+x;if(cell<0||cell>=51984)return OriginalBuildingUpdateBlocker::InvalidState;
                    const size_t at=size_t(cell);const int sector=originalFigureServiceSector(x-centerX,y-centerY);
                    if(value<0&&sector>=0){if(feature[at]&4){
                            if(!shadows[size_t(sector)]||radius<shadows[size_t(sector)])shadows[size_t(sector)]=radius;
                            if(radius==1&&!(sector&1)){if(shadows[size_t((sector+2)&15)]==1)shadows[size_t((sector+1)&15)]=1;if(shadows[size_t((sector+14)&15)]==1)shadows[size_t((sector+15)&15)]=1;}continue;}
                        if(shadows[size_t(sector)]&&radius>shadows[size_t(sector)])continue;}
                    grid[at]=int8_t(std::clamp(originalBuildingAdd32(int(grid[at]),value),-100,100));}
                //44EAED increments its initial1 counter BEFORE comparison;
                //unlike44ECD0,the rectangle delta is not clamped atzero.
                if(++counter>=m[2]){value=originalBuildingAdd32(value,m[3]);counter=0;}
                --left;--top;++right;++bottom;
            }
            continue;
        }
        if(!influence(b.second.x,b.second.y,b.second.size,m[1],m[2],m[3],std::min(10,m[4])))return OriginalBuildingUpdateBlocker::InvalidState;}
    const auto rows=originalBuildingResourceRows(*c.activeSide,*c.cropBase);if(!rows)return OriginalBuildingUpdateBlocker::InvalidState;
    for(unsigned y=0;y<rows->size();++y)for(unsigned n=0;n<rows->at(y).second;++n){const size_t cell=rows->at(y).first+n;const unsigned owner=c.terrainOwners->at(cell);unsigned model=0;
        if(owner){const auto b=c.buildings->find(owner);if(b==c.buildings->end())return OriginalBuildingUpdateBlocker::InvalidState;
            if(b->second.type==111||b->second.type==113)model=b->second.type;}
        if(!model){if(sub[cell]&0x80){if(c.terrain->at(cell)&2)model=2;else{sub[cell]&=0x7f;continue;}}else if(c.terrain->at(cell)&0x20)model=115;
            else if(c.terrain->at(cell)&0x1000){const int xx=int(cell-*c.cropBase)%228;
                if(!influence(xx,int(y),1,-2,1,1,2))return OriginalBuildingUpdateBlocker::InvalidState;continue;}else continue;}
        const auto m=c.buildingModels->find(uint16_t(model));if(m==c.buildingModels->end())return OriginalBuildingUpdateBlocker::MissingContext;
        if(!influence(int(cell-*c.cropBase)%228,int(y),1,m->second[1],m->second[2],m->second[3],std::min(10,m->second[4])))return OriginalBuildingUpdateBlocker::InvalidState;}
    c.desirability=std::move(grid);++c.desirabilityCalls;return OriginalBuildingUpdateBlocker::None;
}
//4ACD10:single-cell+1F8=4273D0 reads origin;larger footprints
//take the signed maximum over the size-square,then byte+60 adds10.
inline OriginalBuildingUpdateBlocker originalBuildingApplyDesirability(OriginalBuildingUpdateContext& c){
    if(!c.desirability||c.desirability->size()!=51984||!c.buildings||!c.buildingInputs)return OriginalBuildingUpdateBlocker::MissingContext;
    for(auto& e:*c.buildingInputs){auto& i=e.second;if(i.alive!=1&&i.alive!=3)continue;auto b=c.buildings->find(e.first);
        if(b==c.buildings->end()||!b->second.imageInputs||!b->second.size||b->second.size>6)return OriginalBuildingUpdateBlocker::InvalidState;
        const uint32_t origin=b->second.imageInputs->gridIndex;int value=-9999;
        for(unsigned y=0;y<b->second.size;++y)for(unsigned x=0;x<b->second.size;++x){const uint64_t cell=uint64_t(origin)+228*y+x;if(cell>=51984)return OriginalBuildingUpdateBlocker::InvalidState;
            value=std::max(value,int(c.desirability->at(size_t(cell))));}
        i.desirability=int8_t(uint8_t(value+(i.desirabilityBonus?10:0)));}
    return OriginalBuildingUpdateBlocker::None;
}
//4AC36A..394 ->591EC0/5925A0. Positive empire transitions remain
//refused;zero strength is derived from the complete nonmilitary formations.
inline OriginalBuildingUpdateBlocker originalBuildingStatisticsStep(OriginalBuildingUpdateContext& c){
    if(!c.statistics||!c.formations||!c.figures)return OriginalBuildingUpdateBlocker::CityPhase;auto& s=*c.statistics;
    c.hostileCount=0;for(const auto& f:*c.figures)if(f.alive==1&&((f.type>=58&&f.type<=62)||f.type==78))++c.hostileCount;
    c.hostileFlag=uint8_t(c.hostileCount!=0);
    if(!c.hostileCount){c.statisticsScratch=0;s.demand.fill(0);s.groups.fill(0);
        if(!s.primaryPresent&&!s.primaryBuilding)s.emptyCycles=originalBuildingAdd32(s.emptyCycles,1);
        else if(s.primaryBuilding||s.alternateBuilding)return OriginalBuildingUpdateBlocker::CityPhase;}
    if(s.currentFaction<0||s.currentFaction>=22||!s.religionBonusEnabled)return OriginalBuildingUpdateBlocker::CityPhase;
    for(const auto& g:*c.formations)if(g.alive==1&&!g.skipLifecycle)return OriginalBuildingUpdateBlocker::CityPhase;
    int32_t own=0;if(*s.religionBonusEnabled){if(!s.religionBonus)return OriginalBuildingUpdateBlocker::CityPhase;
        if(*s.religionBonus==2||*s.religionBonus==5||*s.religionBonus==7||*s.religionBonus==8)own=200;}
    for(unsigned n=0;n<22;++n)if(int(n)!=s.currentFaction&&s.empireState[n]&&own>0)return OriginalBuildingUpdateBlocker::CityPhase;
    ++c.statisticsCalls;return OriginalBuildingUpdateBlocker::None;
}
//5A8170(0)->5A7580(1)->50FFE0 updates values BEFORE min/max/status.
//Status2 uses853C18's difficulty row;515720 wraps then50FFC0 clamps.
inline OriginalBuildingUpdateBlocker originalBuildingReligionStep(OriginalBuildingUpdateContext& c){
    if(!c.religion||!c.populationTotal||!c.buildingInputs||!c.buildings||!c.labor||!c.month||!c.statistics||!c.statistics->religionBonusEnabled)return OriginalBuildingUpdateBlocker::CityPhase;
    auto& r=*c.religion;if(!r.previousRating||!r.selected)return OriginalBuildingUpdateBlocker::CityPhase;
    int highest=-800,best=-1;constexpr int lower[]={-50,0,0,0};
    for(unsigned n=0;n<4;++n){auto& w=r.worship[n];if(!w.enabled)continue;w.minimum=150;w.maximum=lower[n];w.served=0;w.temple=false;int total=0,count=0;
        for(auto& e:w.entries){if(e.minimum>e.maximum)return OriginalBuildingUpdateBlocker::InvalidState;
            if(e.status==2&&(c.labor->difficulty<0||c.labor->difficulty>4))return OriginalBuildingUpdateBlocker::MissingContext;
            constexpr int activeDelta[]={-4,-5,-6,-7,-8};
            e.value=std::clamp(originalBuildingAdd32(e.value,e.status==2?activeDelta[c.labor->difficulty]:-1),e.minimum,e.maximum);
            w.minimum=std::min(w.minimum,e.value);w.maximum=std::max(w.maximum,e.value);
            if(e.type<0||e.type>=12)return OriginalBuildingUpdateBlocker::CityPhase;
            if(!r.scenarioEnabled[unsigned(e.type)])e.status=0;else{total=originalBuildingAdd32(total,e.value);++count;if(e.status!=2)e.status=1;}}
        w.average=count?total/count:lower[n];if(!count)w.minimum=w.maximum=lower[n];
        for(const auto& e:*c.buildingInputs){const auto& i=e.second;if((i.alive!=1&&i.alive!=3)||!i.house)continue;
            if(i.house->at(9+n)&&(n<3||c.buildings->at(e.first).type>10))w.served=originalBuildingAdd32(w.served,i.residents);}
        const uint16_t temple=n==0?214:n==1?216:n==2?218:219;
        for(const auto& e:*c.buildings)if(e.second.type==temple&&(c.buildingInputs->at(e.first).alive==1||c.buildingInputs->at(e.first).alive==3))w.temple=true;
        const int rating=originalBuildingAdd32(originalBuildingAdd32(w.minimum/6,w.maximum/2),w.average/3);if(rating>highest){highest=rating;best=int(n);}}
    if(r.worship[0].enabled){const auto& w=r.worship[0];const int rating=originalBuildingAdd32(originalBuildingAdd32(w.minimum/6,w.maximum/2),w.average/3);
        const int limit=*c.populationTotal<100?-100:*c.populationTotal<400?-45:-40;if(rating<limit)return OriginalBuildingUpdateBlocker::CityPhase;}
    if(best>=0&&highest>40&&!*c.statistics->religionBonusEnabled&&(*c.month==2||*c.month==5||*c.month==8||*c.month==11))return OriginalBuildingUpdateBlocker::CityPhase;
    //5A8330 writes EBX=-1 to the SEH stack slot,NOT1312F78.
    //Only alternate mode at5A832A writes previousRating;ordinary preserves it.
    ++r.calls;return OriginalBuildingUpdateBlocker::None;
}
//8235A8 type rows;only independently traced dispatch types are admitted.
inline std::optional<int> originalBuildingEmploymentCategory(uint16_t type){
    switch(type){case 2:case 3:case 5:case 7:case 59:case 126:case 231:return -1;
    case 33:case 192:case 193:case 194:case 196:return 0;
    case 35:case 43:return 1;case 53:case 54:case 58:case 62:case 66:case 67:case 71:return 2;
    case 72:case 124:case 127:case 207:return 3;case 211:return 5;case 214:return 6;default:return std::nullopt;}
}
inline OriginalBuildingUpdateBlocker originalBuildingAvailabilityStep(OriginalBuildingUpdateContext& c){
    if(!c.buildings||!c.buildingInputs||!c.terrain||!c.cropBase)return OriginalBuildingUpdateBlocker::Availability;
    for(auto& e:*c.buildingInputs){auto& i=e.second;if(i.alive!=1&&i.alive!=3)continue;const auto& b=c.buildings->at(e.first);
        if(!originalBuildingEmploymentCategory(b.type)||!b.imageInputs||b.imageInputs->gridIndex>=c.terrain->size())return OriginalBuildingUpdateBlocker::Availability;
        bool inputIndustry=false;if(b.type==33||b.type==35||b.type==43){if(!c.productionInputs)return OriginalBuildingUpdateBlocker::Availability;
            const auto p=c.productionInputs->find(e.first);if(p==c.productionInputs->end())return OriginalBuildingUpdateBlocker::Availability;
            inputIndustry=p->second.primaryStock!=0||p->second.secondaryStock!=0;}
        if(inputIndustry){if(!c.poolRoadSelected)return OriginalBuildingUpdateBlocker::Availability;
            const auto road=c.poolRoadSelected->find(e.first);if(road==c.poolRoadSelected->end())return OriginalBuildingUpdateBlocker::Availability;
            i.available=uint8_t(road->second);if(!road->second){i.operational=0;continue;}}
        //4273F0 uses common road coordinates,not the building origin.
        const int64_t roadCell=int64_t(*c.cropBase)+i.roadX+228LL*i.roadY;
        if(roadCell<0||uint64_t(roadCell)>=c.terrain->size())return OriginalBuildingUpdateBlocker::Availability;
        i.operational=(c.terrain->at(size_t(roadCell))&0x40)?100:0;
    }
    return OriginalBuildingUpdateBlocker::None;
}
//4AE220->4AE7F0->4F19A0:all supplied+188=4271D0,+18C=51E310.
//Preferred rows use+F4=4E1C40 for Trading/Inspector/Watchtower/Herbalist.
inline OriginalBuildingUpdateBlocker originalBuildingEmploymentStep(OriginalBuildingUpdateContext& c){
    if(c.empireProduction){const auto blocker=originalBuildingUnavailableStep(c);if(blocker!=OriginalBuildingUpdateBlocker::None)return blocker;}
    if(!c.buildings||!c.buildingInputs||!c.cityEmployment||!c.employmentModel||!c.buildingUnavailable||!c.buildingLinks||
        !c.unemploymentTrend||!c.shortageNoticeCountdown||!c.elapsedYears||!c.scenarioStartYears)return OriginalBuildingUpdateBlocker::Employment;
    auto& city=*c.cityEmployment;for(auto& r:city.rows)r.demand=r.allocated=r.preferred=r.preferredAllocated=0;
    struct Candidate{size_t slot;int category,required;bool preferred;};std::vector<Candidate> candidates;
    for(auto& e:*c.buildingInputs){auto& i=e.second;const auto type=c.buildings->at(e.first).type;const auto cat=originalBuildingEmploymentCategory(type);
        if(!cat)return OriginalBuildingUpdateBlocker::Employment;i.employmentCategory=uint8_t(*cat);
        const auto unavailable=c.buildingUnavailable->find(e.first);const auto link=c.buildingLinks->find(e.first);
        if(unavailable==c.buildingUnavailable->end()||link==c.buildingLinks->end())return OriginalBuildingUpdateBlocker::Employment;
        const bool enabled=type==71?link->second.entertainmentRoad&&*link->second.entertainmentRoad:i.enabled!=0;
        const bool eligible=*cat>=0&&*cat!=9&&(i.alive==1||i.alive==3)&&!i.suppression&&type!=62&&!i.parent&&
            ((*cat!=0&&*cat!=1&&*cat!=7)||!unavailable->second)&&i.operational>0&&enabled;
        if(!eligible){if(*cat!=9)i.workers=0;continue;}
        const auto model=c.employmentModel->find(type);if(model==c.employmentModel->end())return OriginalBuildingUpdateBlocker::Employment;
        const int required=i.laborDisabled?0:model->second;if(required<0)return OriginalBuildingUpdateBlocker::Employment;
        const bool preferred=type==58||type==124||type==127||type==207;auto& row=city.rows[unsigned(*cat)];
        row.demand=originalBuildingAdd32(row.demand,required);if(preferred)row.preferred=originalBuildingAdd32(row.preferred,required);
        candidates.push_back({e.first,*cat,required,preferred});}
    const int oldShortage=city.shortage;const auto allocated=originalBuildingCityEmploymentStep(city);if(!allocated)return OriginalBuildingUpdateBlocker::Employment;
    city=allocated->state;if(allocated->shortageNotification)c.notificationEffects.push_back(0x548210);
    if(allocated->unemploymentNotification)c.notificationEffects.push_back(0x548270);
    *c.unemploymentTrend=std::min(100,city.unemploymentPercent>=originalBuildingAdd32(*c.unemploymentTrend,5)?
        originalBuildingAdd32(*c.unemploymentTrend,5):city.unemploymentPercent);
    if(!oldShortage&&city.shortage>0&&*c.elapsedYears>=*c.scenarioStartYears&&*c.shortageNoticeCountdown<1){*c.shortageNoticeCountdown=6;c.notificationEffects.push_back(0x542);}
    std::array<int32_t,10> preferredLeft{},ordinaryLeft{};
    for(unsigned n=0;n<10;++n){const auto& r=city.rows[n];if(r.allocated<r.demand){preferredLeft[n]=r.preferredAllocated;ordinaryLeft[n]=r.allocated-r.preferredAllocated;}}
    for(const auto& candidate:candidates){auto& i=c.buildingInputs->at(candidate.slot);const auto& r=city.rows[unsigned(candidate.category)];
        const auto percent=originalBuildingRatio(candidate.preferred?r.preferredAllocated:r.allocated-r.preferredAllocated,
            candidate.preferred?r.preferred:r.demand-r.preferred);if(!percent)return OriginalBuildingUpdateBlocker::Employment;
        i.workers=originalBuildingSignedWord(uint16_t(std::min(candidate.required,originalBuildingPercent(candidate.required,*percent))));
        auto& left=candidate.preferred?preferredLeft[unsigned(candidate.category)]:ordinaryLeft[unsigned(candidate.category)];left=originalBuildingSub32(left,i.workers);}
    for(const auto& candidate:candidates){auto& i=c.buildingInputs->at(candidate.slot);if(i.workers>=candidate.required)continue;
        auto& left=candidate.preferred?preferredLeft[unsigned(candidate.category)]:ordinaryLeft[unsigned(candidate.category)];
        const int amount=std::min(candidate.required-int(i.workers),left);if(amount<0)return OriginalBuildingUpdateBlocker::Employment;
        i.workers=originalBuildingSignedWord(uint16_t(int(i.workers)+amount));left-=amount;}
    ++c.employmentCalls;return OriginalBuildingUpdateBlocker::None;
}
inline OriginalBuildingUpdateBlocker originalBuildingHouseCapacityStep(OriginalBuildingUpdateState& state,OriginalBuildingUpdateContext& c){
    if(!c.houses||!c.houseModelCapacity)return OriginalBuildingUpdateBlocker::HousingCapacity;
    uint32_t capacity=0,free=0;
    for(auto& entry:*c.houses){auto& h=entry.second;h.free=0;
        if(c.buildingInputs){const auto found=c.buildingInputs->find(entry.first);if(found==c.buildingInputs->end()||!found->second.house)return OriginalBuildingUpdateBlocker::HousingCapacity;
            const auto& i=found->second;h.model=i.model;h.residents=i.residents;h.distance=i.distance;h.laborDisabled=i.house->at(55);}
        if(h.distance<=0)continue;
        const int model=h.model+(h.type==11?1:0);
        const auto found=c.houseModelCapacity->find(model);if(found==c.houseModelCapacity->end())return OriginalBuildingUpdateBlocker::HousingCapacity;
        const int allowed=h.laborDisabled?h.residents:found->second;
        capacity+=uint32_t(allowed);free+=uint32_t(allowed)-uint32_t(int32_t(h.residents));
        h.free=int16_t(uint16_t(uint32_t(allowed)-uint32_t(int32_t(h.residents))));if(h.peak<h.residents)h.peak=h.residents;
    }
    if(c.buildingInputs)for(const auto& entry:*c.houses){const auto found=c.buildingInputs->find(entry.first);
        if(found==c.buildingInputs->end())return OriginalBuildingUpdateBlocker::HousingCapacity;found->second.free=entry.second.free;}
    c.cityHouseCapacity=capacity;c.cityHouseFree=free;
    const auto own=c.houses->find(state.building.slot);if(own!=c.houses->end())state.houseCapacity=own->second;
    return OriginalBuildingUpdateBlocker::None;
}
//Population7 scalar requests594143..594FBB,Scenario6 request52D366.
//This restores named inputs only;unserialized notification history remains absent.
inline bool originalBuildingSavedCityContext(const std::vector<uint8_t>& data,size_t side,OriginalBuildingUpdateContext& c){
    if(side<12287||side>data.size()||data.size()-side<1500)return false;const size_t p=side-12287;
    if(detail::u16(data.data()+p)!=7||detail::u16(data.data()+side-28)!=6)return false;
    const auto value=[&](size_t wire){return originalBuildingI32(detail::u32(data.data()+p+wire));};
    c.cityEmployment=originalBuildingSavedCityEmployment(data,side);if(!c.cityEmployment)return false;
    c.populationTotal=value(38);c.populationPeak=value(11271);c.treasury=value(14);
    c.elapsedYears=originalBuildingI32(detail::u32(data.data()+side-180)); //5312A1 D623FC
    c.scenarioStartYears=detail::i16(data.data()+side-28+2); //52D201 C5CAE0
    c.engineRegion=detail::i16(data.data()+side-28+632); //52D301 C5CD58
    c.scenarioFoodOverride=originalBuildingI32(detail::u32(data.data()+side-28+704));
    c.unavailableResources.emplace();for(unsigned n=0;n<36;++n)c.unavailableResources->at(n)=detail::i16(data.data()+p+9904+2*n);
    OriginalBuildingUpdateContext::Accounting a;a.taxRate=value(10);a.taxReceived=value(10458);a.taxOther=value(10462);
    a.wages=value(10414);a.wagesPaid=value(10426);a.treasuryStart=value(10570);a.festivalCount=value(11177);
    a.incomeReceived=value(10546);a.expensesPaid=value(10554);
    for(size_t off:{12027u,11440u,11061u,10498u})a.incomeOther=originalBuildingAdd32(a.incomeOther,value(off));
    //591B50 copies1312220(wire10426) to1312224 before4F1A90 consumes it.
    for(size_t off:{11400u,11161u,10534u,10514u,10506u})a.expensesOther=originalBuildingAdd32(a.expensesOther,value(off));
    c.unemploymentTrend=value(10406);
    OriginalBuildingUpdateContext::Statistics stats;stats.primaryBuilding=value(6);stats.alternateBuilding=value(9900);
    stats.emptyCycles=value(11049);stats.currentFaction=value(11065);stats.primaryPresent=detail::i16(data.data()+p+11101);stats.capacity=value(11821);
    for(unsigned n=0;n<4;++n){stats.demand[n]=value(11789+4*n);stats.groups[n]=value(11841+4*n);}c.statistics=stats;
    c.accounting=a;OriginalBuildingUpdateContext::Labor l;l.sentiment=value(18);l.health=value(26);l.incoming=value(9754);l.outgoing=value(9758);
    l.outRemainder=value(9762);l.inRemainder=value(9766);l.averagePopulation=value(9778);l.populationSum=value(9782);l.sampleCount=value(9790);
    l.statisticsPending=value(9794);l.growthCursor=value(9798);l.deathCursor=detail::i16(data.data()+p+9802);l.lastPopulationDelta=value(9774);l.excessDepartureTotal=value(9770);
    l.arrivals=value(9806);l.departures=value(9810);
    l.unplaced=value(9814);l.migration=value(9818);l.outDelay=value(9826);l.inDelay=value(9830);l.yearArrivals=value(9834);
    constexpr int wages[]={0,20,26,30,34,40};int64_t nearest=INT64_MAX;
    for(unsigned n=0;n<6;++n){const int64_t delta=std::abs(int64_t(a.wages)-wages[n]);if(delta<nearest){nearest=delta;l.wageBand=int32_t(n);}}
    const auto difficulty=originalFigureSavedDifficulty(data,side);if(!difficulty)return false;l.difficulty=int32_t(*difficulty);
    //1192B04..B0D ARE saved by53153A at T+1363680; caller restores
    //them with the terrain anchor (not from Population7).
    //1312564 is rebuilt from live hostile figures;formation labor12112..12124
    //uses the explicitly serialized four dwords (59500B).
    for(unsigned n=0;n<4;++n)l.formationLabor[n]=value(12083+4*n);
    c.labor=l;c.roadEndpoints=originalBuildingSavedRoadEndpoints(data,side);return true;
}
inline bool originalBuildingSavedNoticeCountdown(const std::vector<uint8_t>& data,size_t terrain,OriginalBuildingUpdateContext& c){
    //531556 restores80 bytes1192A64;shortage notice1192A84 is+32.
    constexpr size_t offset=original_save19::fireBlocks80+80+32;
    if(terrain>data.size()||offset+4>data.size()-terrain)return false;
    c.shortageNoticeCountdown=originalBuildingI32(detail::u32(data.data()+terrain+offset));return true;
}
inline OriginalBuildingUpdateBlocker originalBuildingLaborCountdown(OriginalBuildingUpdateContext& c){
    if(!c.buildingInputs||!c.buildings)return OriginalBuildingUpdateBlocker::HousingEvolution;
    c.laborCountdownHouses=0;
    for(auto& entry:*c.buildingInputs){auto& i=entry.second;const auto f=c.buildings->find(entry.first);
        if(f==c.buildings->end())return OriginalBuildingUpdateBlocker::InvalidState;
        if(f->second.type<2||f->second.type>17||(i.alive!=1&&i.alive!=3))continue;
        if(!i.house)return OriginalBuildingUpdateBlocker::InvalidState;auto& disabled=i.house->at(55);
        if(disabled&&i.laborCountdown>0){if(!i.residents){disabled=0;i.laborCountdown=0;}else{++c.laborCountdownHouses;if(!--i.laborCountdown)disabled=0;}}}
    return OriginalBuildingUpdateBlocker::None;
}
//54C6F0 modes0/1:the measured source6 animal groups have+69=3,
//which gates54D450/54C970;54EF10 walks only the first19 groups.
//4EA8E0 increments an underfilled animal group's timer before allocation27.
inline OriginalBuildingUpdateBlocker originalBuildingFormationPhase(OriginalBuildingUpdateContext& c,bool gather){
    if(!c.formations||!c.figures||!c.figureModels||!c.figureStationary)return OriginalBuildingUpdateBlocker::CityPhase;
    //54C8D0 independently decays active63..66 damage. Those military
    //branches are outside this guarded animal-only phase,even with no group.
    for(const auto& f:*c.figures)if(f.alive==1&&f.type>=63&&f.type<=66)return OriginalBuildingUpdateBlocker::CityPhase;
    if(gather){auto next=originalBuildingFormationStep(*c.formations,*c.figures,*c.figureModels,*c.figureStationary);
        if(!next)return OriginalBuildingUpdateBlocker::CityPhase;c.formations=next->formations;c.formationResult=*next;c.figureOrdinals=next->figureOrdinals;++c.formationCalls;}
    for(size_t n=1;n<100;++n){auto& g=c.formations->at(n);if(g.alive!=1)continue;
        if(n<23||g.special||g.lifecycleOwner||g.skipLifecycle!=3||g.kind<75||g.kind>77)return OriginalBuildingUpdateBlocker::CityPhase;
        if(!c.engineRegion)return OriginalBuildingUpdateBlocker::CityPhase;
        g.kind=int16_t(*c.engineRegion==0?77:*c.engineRegion==1?76:75);g.limit=7;
        if(g.count<g.limit){++g.spawnTimer;if(g.spawnTimer>26)return OriginalBuildingUpdateBlocker::CityPhase;}}
    return OriginalBuildingUpdateBlocker::None;
}
inline std::optional<size_t> originalBuildingAllocateMigrant(OriginalBuildingUpdateContext& c,uint8_t type,int16_t x,int16_t y){
    if(!c.figures||c.figures->size()!=2000||!c.figureServices||!c.figureServices->allocationSerial||!c.cropBase||
        !c.terrain||c.terrain->size()!=51984||!c.figureCellHeads||c.figureCellHeads->size()!=51984||!c.figureStationary||
        !c.scenarioFigurePlacementCheck||*c.scenarioFigurePlacementCheck)return std::nullopt;
    size_t slot=0;for(unsigned attempt=0;attempt<6&&!c.freeFigureSlots.empty();++attempt){const auto candidate=c.freeFigureSlots.front();c.freeFigureSlots.erase(c.freeFigureSlots.begin());
        //4E1472 pops once more before checking the fifth failed attempt.
        if(attempt==5)break;if(candidate&&candidate<2000&&!c.figures->at(candidate).alive){slot=candidate;break;}}
    if(!slot)return size_t(0);if(!c.figureSpeechCounters||!c.figureSpeechCounts||type<11||type>13)return std::nullopt;
    const unsigned speech=type==13?254:265;const auto divisor=c.figureSpeechCounts->at(speech);
    if(!divisor||divisor>INT32_MAX)return std::nullopt;
    const int64_t cell=int64_t(*c.cropBase)+x+228LL*y;if(cell<0||cell>=51984)return std::nullopt;
    auto& services=*c.figureServices;
    if(services.randomHistory){auto& h=*services.randomHistory;if(h.cursor>=100)return std::nullopt;h.ring[h.cursor]=h.previous7;h.cursor=(h.cursor+1)%100;}
    for(unsigned k=0;k<31;++k){const auto next=[](uint32_t v){return(v>>1)|((((v>>4)^v)&1u)<<30);};services.randomFirst=next(services.randomFirst);services.randomSecond=next(services.randomSecond);}
    if(services.randomHistory)services.randomHistory->previous7=services.randomFirst&127;
    OriginalFigure f;f.slot=slot;f.type=type;f.alive=1;f.progress=20;f.direction=0;f.previousDirection=0;
    f.x=f.priorX=f.homeX=f.destinationX=x;f.y=f.priorY=f.homeY=f.destinationY=y;f.cell=uint32_t(cell);
    f.subX=int16_t(uint16_t(uint32_t(uint16_t(x))*20));f.subY=int16_t(uint16_t(uint32_t(uint16_t(y))*20));
    f.allocationSerial=uint16_t((*services.allocationSerial)++);f.allocationPhase=f.allocationPhaseCopy=uint8_t(services.randomFirst&3);
    //4EA3B0 and4C92D0 leave constructor speed+61 at0.
    auto& speechCounter=c.figureSpeechCounters->at(speech);c.figureSpeechChoices[slot]=originalBuildingSignedWord(uint16_t(speechCounter%int32_t(divisor)));
    speechCounter=originalBuildingAdd32(speechCounter,1);
    auto& head=c.figureCellHeads->at(size_t(cell));if(!head)head=uint16_t(slot);
    else {size_t tail=head;std::set<size_t> seen;for(;;){if(tail>=2000||!seen.insert(tail).second)return std::nullopt;auto& prior=c.figures->at(tail);
        if(prior.cell!=uint32_t(cell))return std::nullopt;if(!prior.next){prior.next=uint16_t(slot);break;}tail=prior.next;}}
    //4E1420 itself never calls4EA120;unlike Hunter projectile creation,
    //4ADE10/4ADED0/4AE150 do not make that separate sound/CRT call.
    c.figures->at(slot)=f;c.figureStationary->operator[](slot)=0;c.allocatedFigures.push_back(slot);
    c.highestFigureSlot=std::max(c.highestFigureSlot,slot);c.highestEverFigureSlot=std::max(c.highestEverFigureSlot,slot);return slot;
}
//4ADE10 does not change residents;the newcomer owns the reciprocal link.
inline bool originalBuildingImmigrant(OriginalBuildingUpdateContext& c,size_t house,int count){
    if(!c.buildingInputs||!c.buildings||!c.labor||!c.roadEndpoints)return false;auto& i=c.buildingInputs->at(house);
    const auto slot=originalBuildingAllocateMigrant(c,11,c.roadEndpoints->at(0),c.roadEndpoints->at(1));if(!slot)return false;if(!*slot)return true;
    auto& f=c.figures->at(*slot);f.action=6;f.home=int16_t(house);i.migrant=int16_t(*slot);
    f.counter=int16_t(uint16_t(uint32_t(i.random&127)+uint32_t(c.labor->spacing)));f.employmentAmount=uint8_t(count);f.subtype=i.random&1;
    f.water=uint8_t((c.terrain->at(f.cell)>>6)&1);c.labor->spacing=originalBuildingAdd32(c.labor->spacing,50);return true;
}
inline bool originalBuildingEmigrant(OriginalBuildingUpdateContext& c,size_t house,int count){
    if(!c.buildingInputs||!c.buildings||!c.populationTotal||!c.populationPeak||!c.labor)return false;auto& i=c.buildingInputs->at(house);
    //4ADED0 reaches519060 only for emptied ordinary houses;that changes
    //the record class/image and is refused until the caller supplies it.
    if(count>=i.residents&&c.buildings->at(house).type<=10)return false;
    c.labor->lastPopulationDelta=originalBuildingSub32(0,count); //4ADED0->591900 stores signed arrival before5919A0.
    i.residents=originalBuildingSignedWord(uint16_t(int(i.residents)-count));*c.populationTotal=std::max(0,originalBuildingSub32(*c.populationTotal,count));
    *c.populationPeak=std::max(*c.populationPeak,*c.populationTotal);const auto& b=c.buildings->at(house);
    const auto slot=originalBuildingAllocateMigrant(c,12,int16_t(b.x),int16_t(b.y));if(!slot)return false;
    if(*slot){auto& f=c.figures->at(*slot);f.action=6;f.counter=0;f.employmentAmount=uint8_t(count);}return true;
}
//4ADA10's three ordered admission scans;attempted people are accounted
//even if4EA050 returns0. Existing live type11 links suppress duplicates.
inline bool originalBuildingImmigration(OriginalBuildingUpdateContext& c,int requested){
    if(!c.buildingInputs||!c.buildings||!c.figures||!c.labor)return false;int left=requested;
    for(auto& e:*c.buildingInputs)if(e.second.house&&e.second.migrant){auto& id=e.second.migrant;
        if(id<0||id>=2000)return false;if(c.figures->at(size_t(id)).alive!=1)id=0;}
    for(unsigned pass=0;pass<3&&left>0;++pass)for(auto& e:*c.buildingInputs){auto& h=e.second;if(!h.house||left<=0||h.distance<=0)continue;
        if(pass==0){if(h.residents||!h.free)continue;if(h.migrant){const auto& f=c.figures->at(size_t(h.migrant));if(f.type==11&&f.alive)continue;h.migrant=0;}}
        else if(h.migrant||(pass==1?h.free<=11:h.free<=0))continue;
        const int amount=std::min(left,pass<2?6:int(h.free));if(!originalBuildingImmigrant(c,e.first,amount))return false;left-=amount;}
    auto& l=*c.labor;l.arrivals=originalBuildingAdd32(l.arrivals,requested-left);l.yearArrivals=originalBuildingAdd32(l.yearArrivals,l.arrivals);
    if(left&&left==requested)l.unplaced=originalBuildingAdd32(l.unplaced,left);return true;
}
inline bool originalBuildingEmigration(OriginalBuildingUpdateContext& c,int requested){
    if(!c.buildingInputs||!c.labor)return false;int left=requested;
    for(int model=0;model<14&&left>0;++model)for(auto& e:*c.buildingInputs){auto& h=e.second;
        if(!h.house||h.residents<=0||h.model!=model||left<=0)continue;const int amount=std::min({left,6,int(h.residents)});
        if(!originalBuildingEmigrant(c,e.first,amount))return false;left-=amount;}
    c.labor->departures=originalBuildingAdd32(c.labor->departures,requested-left);return true;
}
//4AE1A0 only writes residents when free<0;4AE150 allocates type13,
//and subtracts population only after successful allocation.
inline OriginalBuildingUpdateBlocker originalBuildingExcessResidents(OriginalBuildingUpdateContext& c){
    if(!c.buildingInputs||!c.buildings||!c.populationTotal||!c.populationPeak||!c.labor)return OriginalBuildingUpdateBlocker::CityLabor;
    for(auto& e:*c.buildingInputs){auto& i=e.second;if(!i.house||i.free>=0)continue;const auto& b=c.buildings->at(e.first);
        const auto slot=originalBuildingAllocateMigrant(c,13,int16_t(b.x),int16_t(b.y));if(!slot)return OriginalBuildingUpdateBlocker::CityLabor;
        if(*slot){auto& f=c.figures->at(*slot);f.action=6;f.counter=0;f.employmentAmount=uint8_t(-int(i.free));
            c.labor->excessDepartureTotal=originalBuildingSub32(c.labor->excessDepartureTotal,int(i.free)); //4AE150->591950.
            *c.populationTotal=std::max(0,originalBuildingAdd32(*c.populationTotal,i.free));*c.populationPeak=std::max(*c.populationPeak,*c.populationTotal);}
        i.residents=originalBuildingSignedWord(uint16_t(-int(i.free)<i.residents?int(i.residents)+i.free:1));}
    return OriginalBuildingUpdateBlocker::None;
}
//Common5 mirrors shared with originalFigureBuildingLinks. Publish after every
//successful building phase so later food/storage/allocator consumers see writes.
inline bool originalBuildingSynchronizePool(OriginalBuildingUpdateContext& c){
    if(!c.buildingInputs)return true;
    for(const auto& e:*c.buildingInputs){const auto& i=e.second;
        if(c.buildingLinks){const auto found=c.buildingLinks->find(e.first);if(found==c.buildingLinks->end())return false;auto& b=found->second;
            b.alive=i.alive;b.residents=i.residents;b.freeSlots=i.free;b.houseModel=i.model;b.worker=i.migrant;
            b.workers=i.workers;b.roadAccess=i.available;b.roadDistance=i.distance;b.roadX=i.roadX;b.roadY=i.roadY;b.roamDirection=i.direction;
            b.primaryFigure=i.primaryFigure;
            b.suppress=i.suppression;b.storageLaborDisabled=i.laborDisabled;b.category=i.employmentCategory;
            if(i.house){b.laborDisabled=i.house->at(55);b.religion=std::array<uint8_t,4>{i.house->at(9),i.house->at(10),i.house->at(11),i.house->at(12)};
                b.entertainment=std::array<uint8_t,3>{i.house->at(38),i.house->at(39),i.house->at(41)};b.herbalist=i.house->at(40);
                b.waterService=std::array<uint8_t,2>{i.house->at(45),i.house->at(47)};}}
        if(c.productionInputs){const auto found=c.productionInputs->find(e.first);if(found!=c.productionInputs->end()){found->second.workers=i.workers;found->second.operational=i.operational;}}
    }
    if(c.figureServices&&c.figureServices->population&&c.populationTotal&&c.populationPeak){c.figureServices->population->total=*c.populationTotal;c.figureServices->population->peak=*c.populationPeak;}
    return true;
}
inline OriginalBuildingUpdateBlocker originalBuildingCityLaborStep(OriginalBuildingUpdateContext& c){
    if(!c.labor||!c.populationTotal||!c.buildingInputs||!c.buildings||!c.figures||!c.cityEmployment||!c.formations||!c.figureModels||!c.figureStationary||!c.labor->populationNotices)
        return OriginalBuildingUpdateBlocker::CityLabor;
    auto& l=*c.labor;const int old=l.migration;l.spacing=std::max(0,originalBuildingSub32(l.spacing,51));
    l.migration=l.sentiment>=71?100:l.sentiment>=61?75:l.sentiment>=50?50:l.sentiment>=36?0:l.sentiment>=26?-8:l.sentiment>=16?-17:-25;
    l.incoming=l.outgoing=0;
    //5917E0 early returns bypass the migration-change notification.
    bool notify=true;l.hostileCount=0;for(const auto& f:*c.figures)if(f.alive==1&&((f.type>=58&&f.type<=62)||f.type==78))++l.hostileCount;
    if(*c.populationTotal>199999){l.migration=0;notify=false;}
    else if(l.migration>0&&l.hostileCount>=4){l.migration=0;notify=false;}
    else {if(l.migration>0){if(l.inDelay){--l.inDelay;notify=false;}else{l.incoming=(12*l.migration+99)/100;l.outDelay=2;}}
        else if(l.migration<0){if(l.outDelay){--l.outDelay;notify=false;}else if(*c.populationTotal<101)notify=false;
            else{l.outgoing=(-12*l.migration+99)/100;l.inDelay=2;}}}
    if(notify&&old!=l.migration)c.notificationEffects.push_back(0x548340);
    l.arrivals=l.departures=l.unplaced=0;
    if(l.incoming>0){int incoming=l.incoming;if(incoming<6){l.inRemainder=originalBuildingAdd32(l.inRemainder,incoming);incoming=l.inRemainder>5?l.inRemainder:0;if(incoming)l.inRemainder=0;}
        if(incoming&&!originalBuildingImmigration(c,incoming))return OriginalBuildingUpdateBlocker::CityLabor;}
    if(l.outgoing>0){int outgoing=l.outgoing;if(outgoing<6){l.outRemainder=originalBuildingAdd32(l.outRemainder,outgoing);outgoing=l.outRemainder>5?l.outRemainder:0;if(outgoing)l.outRemainder=0;}
        if(outgoing&&!originalBuildingEmigration(c,outgoing))return OriginalBuildingUpdateBlocker::CityLabor;}
    l.incoming=l.outgoing=0;
    if(l.statisticsPending){
        //590E00 positive band->4ADFB0:ordered housing registry contains
        //sentinel0,then every framed house in ascending registered order.
        if(l.health>=51){if(!c.houseModelCapacity)return OriginalBuildingUpdateBlocker::CityLabor;
            const int rate=l.health>=91?6:l.health>=81?4:l.health>=71?3:l.health>=61?2:1;
            int left=originalBuildingPercent(*c.populationTotal,rate),added=0;
            std::vector<size_t> pool{0};for(const auto& e:*c.buildingInputs)if(e.second.house)pool.push_back(e.first);
            if(l.growthCursor<0)l.growthCursor=0;if(size_t(l.growthCursor)>=pool.size())l.growthCursor=int32_t(pool.size()-1);
            size_t position=size_t(l.growthCursor);
            for(size_t k=0;k<pool.size()&&left;++k){if(++position>=pool.size())position=1;if(position>=pool.size())break;
                auto& h=c.buildingInputs->at(pool[position]);const auto& b=c.buildings->at(pool[position]);
                if((h.alive!=1&&h.alive!=3)||h.distance<=0||h.residents<=0||b.type>10)continue;
                const auto model=c.houseModelCapacity->find(h.model);if(model==c.houseModelCapacity->end())return OriginalBuildingUpdateBlocker::CityLabor;
                const int amount=std::min(left,model->second-int(h.residents));if(amount<0)return OriginalBuildingUpdateBlocker::CityLabor;
                l.growthCursor=int32_t(position);h.residents=originalBuildingSignedWord(uint16_t(int(h.residents)+amount));
                h.free=originalBuildingSignedWord(uint16_t(model->second-int(h.residents)));left-=amount;added+=amount;}
            *c.populationTotal=std::max(0,originalBuildingAdd32(*c.populationTotal,added));
        }else if(l.health>0&&l.health<=40)return OriginalBuildingUpdateBlocker::CityLabor;
        l.populationSum=originalBuildingAdd32(l.populationSum,*c.populationTotal);l.sampleCount=originalBuildingAdd32(l.sampleCount,1);
        if(!l.sampleCount||(l.populationSum==INT32_MIN&&l.sampleCount==-1))return OriginalBuildingUpdateBlocker::CityLabor;
        l.statisticsPending=0;l.averagePopulation=l.populationSum/l.sampleCount;
        if(!c.populationPeak)return OriginalBuildingUpdateBlocker::CityLabor;*c.populationPeak=std::max(*c.populationPeak,*c.populationTotal);}
    constexpr int difficulty[]={50,45,40,37,35},wage[]={-10,-6,-3,0,3,5},health[]={-2,-1,0,1,2};
    if(l.difficulty<0||l.difficulty>4||l.wageBand<0||l.wageBand>5)return OriginalBuildingUpdateBlocker::CityLabor;
    const unsigned band=l.health>=80?4:l.health>=65?3:l.health>=35?2:l.health>=20?1:0;int32_t residents=0;
    for(const auto& e:*c.buildingInputs)if(e.second.house&&e.second.residents>0&&c.buildings->at(e.first).type<=10)residents=originalBuildingAdd32(residents,e.second.residents);
    c.cityEmployment->available=originalBuildingPercent(residents,difficulty[l.difficulty]+wage[l.wageBand]+health[band]);
    const auto formation=originalBuildingFormationStep(*c.formations,*c.figures,*c.figureModels,*c.figureStationary);if(!formation)return OriginalBuildingUpdateBlocker::CityLabor;
    c.formations=formation->formations;c.formationResult=*formation;c.figureOrdinals=formation->figureOrdinals;++c.formationCalls;
    for(unsigned n=0;n<4;++n)c.cityEmployment->available=originalBuildingSub32(c.cityEmployment->available,
        originalBuildingI32(uint32_t(n==2?2:n==3?4:1)*uint32_t(l.formationLabor[n])));
    for(const auto& e:*c.buildings)if(e.second.type==130||e.second.type==131)return OriginalBuildingUpdateBlocker::CityLabor;
    c.cityEmployment->available=std::max(0,c.cityEmployment->available);
    constexpr int thresholds[]={100,500,1000,2000,3000,5000,10000,15000,20000,25000};
    for(unsigned n=0;n<10;++n)if(*c.populationTotal>=thresholds[n]&&!l.populationNotices->at(n)){l.populationNotices->at(n)=true;c.notificationEffects.push_back(0x40e +6*n);}
    return OriginalBuildingUpdateBlocker::None;
}
//0x5944BF reads72 bytes at Population7+26B4. The serialized population
// immediately precedes date8+block144+Scenario6 (531293..5312CB).
inline std::optional<int16_t> originalBuildingSavedUnavailable(const std::vector<uint8_t>& data,size_t side,uint8_t output){
    size_t bytes=2;for(const auto& f:emperor::file_io::research::later7_requests)bytes+=f.bytes;
    if(output>=36||side>data.size()||side<28+152+bytes)return std::nullopt;
    const size_t population=side-28-152-bytes;
    if(detail::u16(data.data()+population)!=7||detail::u16(data.data()+side-28)!=6)return std::nullopt;
    return detail::i16(data.data()+population+9904+2*output);
}
//5311DC's ten goal lists precede Flood1(4EDDA0), then Festival4
//48ED30. +50/+54/+58 become wire66/67/71.5F1440 calls48E0F0,
//not date-qualified48ED00;608620 reads+50,48E680 deity2 accepts35/43.
inline std::optional<bool> originalBuildingSavedProductionBoost(const std::vector<uint8_t>& data,uint16_t type,size_t* end=nullptr){
    if(type==33)return false; //48E680 has no Hunter entry in any deity row.
    if((type!=35&&type!=43)||data.size()<3355||data.size()>8u*1024u*1024u)return std::nullopt;
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
    const size_t festival=at;if(!version(4)||!version(1)||!take(16+32+8+1+1+4+1+4+4+4+4))return std::nullopt;
    if(end)*end=at;return data[festival+66]&&detail::u32(data.data()+festival+67)==3&&detail::u32(data.data()+festival+71)==2;
}
//5A7B90 child3/embedded2 follows Festival4;only the three5-dword
//entries are serialized. Enabled+14 is rebuilt from Scenario6 by5A7DE0.
inline std::optional<OriginalBuildingUpdateContext::Religion> originalBuildingSavedReligion(const std::vector<uint8_t>& data,size_t side){
    size_t at=0;if(!originalBuildingSavedProductionBoost(data,35,&at)||side<28||side>data.size()||data.size()-side<16)return std::nullopt;
    const auto take=[&](size_t n){if(at>data.size()||n>data.size()-at)return false;at+=n;return true;};
    const auto version=[&](uint16_t v){return take(2)&&detail::u16(data.data()+at-2)==v;};
    if(!version(1))return std::nullopt;OriginalBuildingUpdateContext::Religion r;
    for(unsigned n=0;n<4;++n){if(!version(3))return std::nullopt;auto& w=r.worship[n];w.enabled=data[side-28+6+n]!=0;
        for(auto& e:w.entries){if(!version(2)||!take(20))return std::nullopt;const auto* p=data.data()+at-20;
            e={originalBuildingI32(detail::u32(p)),originalBuildingI32(detail::u32(p+4)),originalBuildingI32(detail::u32(p+8)),originalBuildingI32(detail::u32(p+12)),originalBuildingI32(detail::u32(p+16))};}}
    if(!take(20))return std::nullopt;for(unsigned n=0;n<12;++n)r.scenarioEnabled[n]=data[side-28+10+n];
    for(unsigned n=0;n<4;++n)for(auto& e:r.worship[n].entries){if(e.type<0||e.type>=12||e.minimum>e.maximum)return std::nullopt;
        if(!r.worship[n].enabled||!r.scenarioEnabled[unsigned(e.type)]){e.status=0;e.value=std::clamp(0,e.minimum,e.maximum);}
        else if(e.status!=2)e.status=1;}
    return r;
}
// SAV19 531427/435/443 => T+1299328/+1299332/+1299336.
// Month C82EEC is5312AF => S-176. No runtime clock/CRT value is inferred.
inline std::optional<OriginalBuildingUpdateContext> originalBuildingSavedUpdateContext(
    const std::vector<uint8_t>& data,size_t terrain,size_t side,
    const OriginalBuildingFootprint& building,OriginalBuildingUpdateScope scope,bool isolated){
    // Continuation of531816 through53191A; the269 bytes are53192B.
    constexpr size_t disabled=1563037+207936+26+13+2*51984+4+2*51984+1436+4+224+3600+624+256+224+448+200;
    if(data.size()>8u*1024u*1024u||data.size()<2||detail::u16(data.data())!=19||
        terrain>data.size()||disabled+269>data.size()-terrain||side<176||side>data.size()||
        !building.imageInputs||building.imageInputs->gridIndex>=51984||building.type>=269)return std::nullopt;
    OriginalBuildingUpdateContext c(scope,isolated);
    c.phase=detail::u32(data.data()+terrain+original_save19::c82ef8);
    c.monthPart=detail::u32(data.data()+terrain+original_save19::c82ef0);
    c.cycle=detail::u32(data.data()+terrain+original_save19::c82ef4);
    c.month=detail::u32(data.data()+side-176);
    c.terrainFlags=detail::u32(data.data()+terrain+4*building.imageInputs->gridIndex);
    c.typeDisabled=data[terrain+disabled+building.type]!=0;
    return c;
}
struct OriginalBuildingUpdateResult {
    OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::InvalidState;
    std::optional<OriginalBuildingUpdateState> state;
    std::optional<OriginalBuildingUpdateContext> context;
    unsigned operations=0,productionCalls=0,virtual20Calls=0,serviceDecayCalls=0;
    size_t changedSelectorInputs=0;
    bool selectorChanged=false;
    std::optional<OriginalBuildingBaseSelection> beforeSelection,afterSelection;
    std::optional<uint32_t> refusedPhase;
};
inline int16_t originalBuildingSignedWord(uint16_t v){return int16_t(v>32767?int(v)-65536:int(v));}
inline uint16_t originalBuildingCropFlags(const OriginalBuildingUpdateInputs& i){
    return i.industry?detail::u16(i.industry->data()+0x6a-4):0;
}
inline int16_t originalBuildingCropProgress(const OriginalBuildingUpdateInputs& i){
    return i.industry?detail::i16(i.industry->data()):0;
}
inline void originalBuildingSetCropFlags(OriginalBuildingUpdateInputs& i,uint16_t flags){
    i.industry->at(0x6a-4)=uint8_t(flags);i.industry->at(0x6b-4)=uint8_t(flags>>8);
}
//0x4C1BC0: saturating LOW BYTE decay, preserve only flags C000.
inline void originalBuildingDecayCropFlags(OriginalBuildingUpdateInputs& i){
    const auto flags=originalBuildingCropFlags(i);const unsigned low=flags&255;
    originalBuildingSetCropFlags(i,uint16_t((flags&0xc000)|(low?low-1:0)));
}
inline bool originalBuildingIsHouse(uint16_t type){return type>=2&&type<=17;}
//0x517280, tail bytes from0x517410 (runtime data offsets+1 in wire).
inline void originalBuildingDecayHouseServices(OriginalBuildingUpdateInputs& i){
    for(unsigned offset:{41u,42u,43u,44u,45u,49u,50u,51u,13u,14u,15u,16u}){
        auto& v=i.house->at(offset-4);if(v)--v;
    }
}
//518D60 ->517330/5173E0. House1 omits runtime padding11;
//runtime32/34/36/37/38 map to wire49/51/53/54/55 respectively.
inline void originalBuildingAggregateHouseServices(OriginalBuildingUpdateInputs& i){
    auto& h=*i.house;const auto get=[&](unsigned wire){return h.at(wire-4);};
    const unsigned entertainment=get(53);const unsigned band=entertainment>89?5:entertainment>69?4:entertainment>49?3:entertainment>29?2:entertainment>0?1:0;
    constexpr unsigned entertainmentValue[]={0,10,20,30,35,40};
    h.at(55-4)=uint8_t((get(51)?15:get(49)?5:0)+(get(44)?30:0)+(get(41)?15:0)+entertainmentValue[band]);
    h.at(54-4)=uint8_t(33u*(unsigned(get(43)!=0)+unsigned(get(42)!=0)+unsigned(get(45)!=0)));
}
//0x51E4A0 -> C8(-4)=4C11F0 ->4C0F60.0x4C2600 gives
// hemp125/63/0;0x4C2260 gives millet157/79/0 except final harvest
// month10(table0x7AE1AC). Addition wraps16 BEFORE signed10000 clamp.
inline OriginalBuildingUpdateBlocker originalBuildingProductionStep(
    OriginalBuildingUpdateState& s,const OriginalBuildingUpdateContext& c){
    auto& i=s.inputs;const auto type=s.building.type;
    //4C1A90 ->4C1A20 precedes all per-record output/terrain gates.
    if((type==194||type==196)&&i.alive==1&&c.monthPart&&*c.monthPart==0&&c.month&&*c.month==(type==194?8u:10u)){
        if(!i.industry)return OriginalBuildingUpdateBlocker::InvalidState;
        originalBuildingSetCropFlags(i,uint16_t(originalBuildingCropFlags(i)|0x8000));
    }
    if((i.alive!=1&&i.alive!=3)||!i.output)return OriginalBuildingUpdateBlocker::None;
    if(type!=194&&type!=196){
        if(type!=33&&type!=35&&type!=43)return OriginalBuildingUpdateBlocker::ProductionVirtuals;
        if(!s.production)return OriginalBuildingUpdateBlocker::MissingContext;
        auto& p=*s.production;
        //+198=416AE0 ->+F8=416A90; +78=4289F0. These precede
        //4AE560 and+208, so unavailable/festival/model are lazy inputs.
        if(p.operational<=0||p.workers<=0||!p.enabled)return OriginalBuildingUpdateBlocker::None;
        if(!c.unavailable)return OriginalBuildingUpdateBlocker::MissingContext;
        if(*c.unavailable!=0)return OriginalBuildingUpdateBlocker::None; //4AE560 default
        //+208=5F14D0,+26C/+270=4F1540(100), signed stock comparisons.
        if((p.primaryResource&&p.primaryStock<100)||(p.secondaryResource&&p.secondaryStock<100))return OriginalBuildingUpdateBlocker::None;
        auto& delay=p.industry[0x63-4];if(delay){--delay;return OriginalBuildingUpdateBlocker::None;}
        if(!c.requiredWorkers||*c.requiredWorkers<0||!c.festivalProductionBoost)return OriginalBuildingUpdateBlocker::MissingContext;
        //+1BC=416B60 ->428ED0 ->428EB0; byte6E disables requirement.
        const int32_t required=p.laborDisabled?0:*c.requiredWorkers;
        const int64_t percent=required>0?int64_t(p.workers)*100/required:0;
        if(percent>INT32_MAX||percent<INT32_MIN)return OriginalBuildingUpdateBlocker::InvalidState;
        auto& timer=p.industry[0x5f-4];if(timer)--timer;
        //+74=5F1440: festival-qualified division5, otherwise10.
        //51E581 is a word addition BEFORE the signed virtual+204 cap.
        const int delta=int(percent)/(*c.festivalProductionBoost?5:10);
        auto progress=originalBuildingSignedWord(uint16_t(uint32_t(detail::u16(p.industry.data()))+uint32_t(delta)));
        const int cap=type==43?320:160; //416B70 /558FF0
        if(progress>cap)progress=int16_t(cap);
        p.industry[0]=uint8_t(progress);p.industry[1]=uint8_t(uint16_t(progress)>>8);
        return OriginalBuildingUpdateBlocker::None;
    }
    if(!i.industry||!s.building.imageInputs)return OriginalBuildingUpdateBlocker::InvalidState;
    if(!c.terrainFlags||!c.typeDisabled||!c.month||*c.month>11)return OriginalBuildingUpdateBlocker::MissingContext;
    if(*c.terrainFlags&0x100)return OriginalBuildingUpdateBlocker::None;
    auto& delay=i.industry->at(0x63-4);
    if(delay){--delay;return OriginalBuildingUpdateBlocker::None;}
    const unsigned low=originalBuildingCropFlags(i)&255;
    const int growth=type==196&&*c.month==10?0:low>32?(type==194?125:157):low?(type==194?63:79):0;
    int16_t progress=originalBuildingCropProgress(i);
    if(growth){progress=originalBuildingSignedWord(uint16_t(uint32_t(uint16_t(progress))+unsigned(growth)));
        if(progress>10000)progress=10000;}
    if(*c.typeDisabled)progress=0;
    i.industry->at(0)=uint8_t(progress);i.industry->at(1)=uint8_t(uint16_t(progress)>>8);
    s.building.imageInputs->cropWord4=progress;
    return OriginalBuildingUpdateBlocker::None;
}
// The function publishes only the complete requested operation sequence for this
// record. Refusals have no prefix state. Scope distinguishes a verified subpass
// from the admitted city tick, which includes currently unported later phases.
inline OriginalBuildingUpdateResult originalBuildingStateAfterUpdates(
    const OriginalBuildingUpdateState& saved,const OriginalBuildingUpdateContext& context,unsigned count){
    OriginalBuildingUpdateResult out;
    if(count>10000||saved.building.slot>=4000||saved.inputs.identity!=saved.building.slot)return out;
    if(!context.isolateBuildingWrites){out.blocker=OriginalBuildingUpdateBlocker::ExternalWrites;return out;}
    if(!context.phase||*context.phase>50||!context.monthPart||*context.monthPart>15||!context.cycle||!context.month||*context.month>11){
        out.blocker=OriginalBuildingUpdateBlocker::MissingContext;return out;}
    auto s=saved;auto c=context;out.beforeSelection=originalBuildingBaseSelection(saved.building);
    const bool crop=s.building.type==194||s.building.type==196,house=originalBuildingIsHouse(s.building.type);
    if((crop&&!s.inputs.industry)||(house&&!s.inputs.house))return out;
    if(crop&&(!s.building.imageInputs||s.building.imageInputs->cropWord4!=originalBuildingCropProgress(s.inputs)))return out;
    if(c.scope==OriginalBuildingUpdateScope::AdmittedTicks&&count){
        if(c.applyLoadRoadRefresh&&*c.applyLoadRoadRefresh&&!c.loadRoadRefreshApplied){
            if(!c.roadRefreshOrdinaryExit||!*c.roadRefreshOrdinaryExit||!c.roadAccess){out.blocker=OriginalBuildingUpdateBlocker::RoadRefresh;return out;}
            //534EBA runs4AF350,4ACFC0,4AEEB0 unconditionally in this order.
            auto blocker=originalBuildingNetworkStep(c);if(blocker!=OriginalBuildingUpdateBlocker::None){out.blocker=blocker;return out;}
            blocker=originalBuildingPoolRoadStep(c);if(blocker!=OriginalBuildingUpdateBlocker::None){out.blocker=blocker;return out;}
            if(s.inputs.alive==1||s.inputs.alive==3){s.roadDistance=c.roadAccess->distance;s.roadX=c.roadAccess->x;s.roadY=c.roadAccess->y;
                s.inputs.distance=originalBuildingSignedWord(*s.roadDistance);s.inputs.roadX=*s.roadX;s.inputs.roadY=*s.roadY;
                if(c.roadAccess->network){s.roadNetwork=c.roadAccess->network;s.inputs.network=*s.roadNetwork;}
                if(house)s.roadFailureCount=0;}
            if(!originalBuildingSynchronizePool(c)){out.blocker=OriginalBuildingUpdateBlocker::InvalidState;return out;}
            blocker=originalBuildingGoodsPoolStep(c);if(blocker!=OriginalBuildingUpdateBlocker::None){out.blocker=blocker;return out;}
            c.loadRoadRefreshApplied=true;}
        if(c.loadAccountingTransition&&*c.loadAccountingTransition&&!c.loadAccountingApplied){
            const auto blocker=originalBuildingAccountingStep(c);if(blocker!=OriginalBuildingUpdateBlocker::None){out.blocker=blocker;return out;}c.loadAccountingApplied=true;}}
    for(unsigned n=0;n<count;++n){
        OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::None;
        bool localWrites=false;
        if(c.buildingInputs){const auto own=c.buildingInputs->find(s.building.slot);if(own==c.buildingInputs->end())return out;own->second=s.inputs;}
        if(c.scope==OriginalBuildingUpdateScope::ProductionPasses){blocker=originalBuildingProductionStep(s,c);++out.productionCalls;}
        else if(c.scope==OriginalBuildingUpdateScope::HouseServiceDecayPasses){
            if(house&&(s.inputs.alive==1||s.inputs.alive==3)){originalBuildingDecayHouseServices(s.inputs);++out.serviceDecayCalls;}}
        else {
            const unsigned phase=*c.phase;
            // Fail at the first missing write producer, not at an invented
            // per-type no-op. Phase21 rewrites road fields of every live type.
             if(phase==21){localWrites=true;
                 if(!c.roadRefreshOrdinaryExit||!*c.roadRefreshOrdinaryExit)blocker=OriginalBuildingUpdateBlocker::RoadRefresh;
                 else if(c.poolRoadAccess&&c.buildingInputs)blocker=originalBuildingPoolRoadStep(c);
                 if(blocker==OriginalBuildingUpdateBlocker::None&&(s.inputs.alive==1||s.inputs.alive==3)){
                    if(!c.roadAccess)
                        blocker=OriginalBuildingUpdateBlocker::RoadRefresh;
                    else {s.roadDistance=c.roadAccess->distance;s.roadX=c.roadAccess->x;s.roadY=c.roadAccess->y;
                    if(house)s.roadFailureCount=0;
                    s.inputs.distance=originalBuildingSignedWord(*s.roadDistance);s.inputs.roadX=*s.roadX;s.inputs.roadY=*s.roadY;
                    if(c.roadAccess->network){s.roadNetwork=c.roadAccess->network;s.inputs.network=*s.roadNetwork;}}}}
            else if(phase==22)blocker=originalBuildingHouseCapacityStep(s,c);
            else if(phase==23)blocker=originalBuildingCityLaborStep(c);
            else if(phase==24)blocker=originalBuildingExcessResidents(c);
            else if(phase==25)blocker=originalBuildingEmploymentStep(c);
            else if(phase==27)blocker=originalBuildingIrrigationStep(c);
            else if(phase==28)blocker=originalBuildingWaterStep(c);
            else if(phase==32)blocker=originalBuildingTradeStep(c);
            else if(phase==33)blocker=originalBuildingTotalsStep(c);
            else if(phase==37)blocker=originalBuildingDesirabilityStep(c);
            else if(phase==38)blocker=originalBuildingApplyDesirability(c);
            else if(phase==39&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution;
            else if(phase==6)blocker=originalBuildingLaborCountdown(c);
            else if(phase==7)blocker=originalBuildingNetworkStep(c);
            else if(phase==8)blocker=originalBuildingGoodsPoolStep(c);
            else if(phase==44)blocker=OriginalBuildingUpdateBlocker::RiskAndFire;
             else if(phase==31&&(s.inputs.alive==1||s.inputs.alive==3)){localWrites=true;
                 if(c.buildingHookCountdown)blocker=originalBuildingHookPreamble(c);
                 if(blocker==OriginalBuildingUpdateBlocker::None&&c.buildingHookCountdown&&c.buildingInputs)s.inputs=c.buildingInputs->at(s.building.slot);
                 if(blocker==OriginalBuildingUpdateBlocker::None)blocker=originalBuildingHookStep(s,c);
                 if(blocker==OriginalBuildingUpdateBlocker::None)++out.virtual20Calls;
            }else if(phase==20){localWrites=true;blocker=originalBuildingProductionStep(s,c);++out.productionCalls;
                if(blocker==OriginalBuildingUpdateBlocker::None&&c.resourceGrowth)blocker=originalBuildingResourceGrowthStep(c);}
            else if(phase==35&&house&&(s.inputs.alive==1||s.inputs.alive==3)){localWrites=true;originalBuildingDecayHouseServices(s.inputs);++out.serviceDecayCalls;}
            // Other phases may dispatch further virtuals. They remain named
            // blockers; a complete state never crosses them silently.
            else if(phase==12)blocker=originalBuildingAvailabilityStep(c);
            else if(phase==13)blocker=originalBuildingStatisticsStep(c);
            else if(phase==16)blocker=originalBuildingStorageStep(c);
            else if(phase==17)blocker=originalBuildingFoodStep(c);
            else if(phase==18)blocker=originalBuildingReligionStep(c);
            else if(phase==36&&house){localWrites=true;if(s.inputs.houseInUse&&(s.inputs.alive==1||s.inputs.alive==3))originalBuildingAggregateHouseServices(s.inputs);}
            else if(phase==1)blocker=originalBuildingAccountingStep(c);
            else if(phase==3||phase==30)c.citySignal=1;
            else if(phase==4){if(!c.treasury)blocker=OriginalBuildingUpdateBlocker::CityPhase;else if(*c.treasury>=0)c.cityCountA=c.cityCountB=0;}
            else if(phase==5||phase==29)blocker=originalBuildingFormationPhase(c,phase==5);
            else if(phase==9){} //40FC20 RET
            else if(phase==10){if(!c.buildingInputs||!c.buildings||!c.highestEverBuildingSlot||!c.buildingRegistrySize)blocker=OriginalBuildingUpdateBlocker::CityPhase;
                else{uint32_t highest=0;for(const auto& e:*c.buildingInputs)if(e.second.alive)highest=uint32_t(e.first);
                    c.highestBuildingSlot=highest;*c.highestEverBuildingSlot=std::max(*c.highestEverBuildingSlot,highest);
                    if(!c.goodsRecords)blocker=OriginalBuildingUpdateBlocker::CityPhase;else c.goodsRegistryLast=uint32_t(c.goodsRecords->size()-1);}}
            else if(phase==11){if(!c.populationTotal||!c.populationPeak)blocker=OriginalBuildingUpdateBlocker::CityPhase;else *c.populationPeak=std::max(*c.populationPeak,*c.populationTotal);}
            else if(phase!=0&&phase!=2&&phase!=14&&phase!=15&&phase!=19&&phase!=26&&phase!=34&&
                phase!=35&&phase!=36&&phase!=39&&phase!=41&&phase!=42&&phase!=46&&phase!=48&&phase!=49&&phase!=50)
                blocker=OriginalBuildingUpdateBlocker::UnclosedPhase;
            if(blocker==OriginalBuildingUpdateBlocker::None){
                if(phase==48&&s.inputs.alive==1&&s.inputs.byte52){localWrites=true;--s.inputs.byte52;} //4AE8B0
                if(phase==50){
                    //4AC650 includes independent city work even on an ordinary
                    // month part. The closed flag-decay helper above is exposed
                    // separately; do not turn it into a complete wrap claim.
                    blocker=OriginalBuildingUpdateBlocker::MonthlyCity;
                }else c.phase=phase+1;
            }
        }
        if(blocker!=OriginalBuildingUpdateBlocker::None){out.blocker=blocker;
            if(c.scope==OriginalBuildingUpdateScope::AdmittedTicks)out.refusedPhase=c.phase;return out;}
        ++out.operations;
        if(c.buildingInputs){auto found=c.buildingInputs->find(s.building.slot);if(found!=c.buildingInputs->end()){
            if(c.scope==OriginalBuildingUpdateScope::AdmittedTicks&&!localWrites)s.inputs=found->second;
            if(house&&s.inputs.house&&s.building.imageInputs){s.building.imageInputs->houseByte3c=s.inputs.house->at(55);s.building.imageInputs->word20=s.inputs.residents;}
            if(s.production&&c.scope==OriginalBuildingUpdateScope::AdmittedTicks){s.production->workers=s.inputs.workers;s.production->operational=s.inputs.operational;}
            found->second=s.inputs;}}
        if(c.scope==OriginalBuildingUpdateScope::AdmittedTicks&&!originalBuildingSynchronizePool(c)){out.blocker=OriginalBuildingUpdateBlocker::InvalidState;out.refusedPhase=c.phase;return out;}
    }
    out.afterSelection=originalBuildingBaseSelection(s.building);
    if(crop&&saved.building.imageInputs&&s.building.imageInputs&&saved.building.imageInputs->cropWord4!=s.building.imageInputs->cropWord4)++out.changedSelectorInputs;
    if(house&&saved.inputs.house&&s.inputs.house&&saved.inputs.house->at(55)!=s.inputs.house->at(55))++out.changedSelectorInputs;
    if(house&&saved.inputs.residents!=s.inputs.residents)++out.changedSelectorInputs;
    if(out.beforeSelection&&out.afterSelection)out.selectorChanged=out.beforeSelection->code!=out.afterSelection->code||out.beforeSelection->variant!=out.afterSelection->variant;
    out.blocker=OriginalBuildingUpdateBlocker::None;out.state=std::move(s);out.context=c;return out;
}
} // namespace game_lab
