#pragma once
#include "original_building.hpp"
#include "original_building_animation.hpp"
#include "../file_io/include/city19_later7_layout.hpp"

namespace game_lab {
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
enum class OriginalBuildingUpdateBlocker {None,InvalidState,MissingContext,ExternalWrites,
    ProductionVirtuals,RoadRefresh,Employment,HousingEvolution,BuildingVirtual20,
    RiskAndFire,MonthlyCity,Availability,CityPhase,UnclosedPhase,HousingCapacity,CityLabor};
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
struct OriginalBuildingRoadAccess {uint16_t distance=0;int16_t x=0,y=0;std::optional<uint8_t> network;};
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
    out.network=c.networks[size_t(chosen)];return out;
}
//House+84=518A50 uses4BA6F0: rank roads WITHOUT distance qualification,
//then tests distance at the selected exit. The reachable branch resets+28;
//unreachable paths can spawn departures or remove houses and remain refused.
inline std::optional<OriginalBuildingRoadAccess> originalBuildingHouseRoadAccess(
    const OriginalBuildingFootprint& f,const OriginalBuildingRoadInputs& c){
    if(f.type<2||f.type>17||!f.imageInputs||!f.size||f.size>6||c.terrain.size()!=51984||
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
    if(chosen<0||!c.distance[size_t(chosen)])return std::nullopt;
    OriginalBuildingRoadAccess out;out.distance=uint16_t(c.distance[size_t(chosen)]);
    out.x=int16_t((chosen-int(c.cropBase))%228);out.y=int16_t((chosen-int(c.cropBase))/228);return out;
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
    OriginalBuildingUpdateContext(OriginalBuildingUpdateScope operation,bool isolated)
        :scope(operation),isolateBuildingWrites(isolated){}
};
inline OriginalBuildingUpdateBlocker originalBuildingHouseCapacityStep(OriginalBuildingUpdateState& state,OriginalBuildingUpdateContext& c){
    if(!c.houses||!c.houseModelCapacity)return OriginalBuildingUpdateBlocker::HousingCapacity;
    uint32_t capacity=0,free=0;
    for(auto& entry:*c.houses){auto& h=entry.second;h.free=0;
        if(h.distance<=0)continue;
        const int model=h.model+(h.type==11?1:0);
        const auto found=c.houseModelCapacity->find(model);if(found==c.houseModelCapacity->end())return OriginalBuildingUpdateBlocker::HousingCapacity;
        const int allowed=h.laborDisabled?h.residents:found->second;
        capacity+=uint32_t(allowed);free+=uint32_t(allowed)-uint32_t(int32_t(h.residents));
        h.free=int16_t(uint16_t(uint32_t(allowed)-uint32_t(int32_t(h.residents))));if(h.peak<h.residents)h.peak=h.residents;
    }
    c.cityHouseCapacity=capacity;c.cityHouseFree=free;
    const auto own=c.houses->find(state.building.slot);if(own!=c.houses->end())state.houseCapacity=own->second;
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
inline std::optional<bool> originalBuildingSavedProductionBoost(const std::vector<uint8_t>& data,uint16_t type){
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
    return data[festival+66]&&detail::u32(data.data()+festival+67)==3&&detail::u32(data.data()+festival+71)==2;
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
    for(unsigned n=0;n<count;++n){
        OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::None;
        if(c.scope==OriginalBuildingUpdateScope::ProductionPasses){blocker=originalBuildingProductionStep(s,c);++out.productionCalls;}
        else if(c.scope==OriginalBuildingUpdateScope::HouseServiceDecayPasses){
            if(house&&(s.inputs.alive==1||s.inputs.alive==3)){originalBuildingDecayHouseServices(s.inputs);++out.serviceDecayCalls;}}
        else {
            const unsigned phase=*c.phase;
            // Fail at the first missing write producer, not at an invented
            // per-type no-op. Phase21 rewrites road fields of every live type.
            if(phase==21){
                if(!c.roadRefreshOrdinaryExit||!*c.roadRefreshOrdinaryExit)blocker=OriginalBuildingUpdateBlocker::RoadRefresh;
                else if(s.inputs.alive==1||s.inputs.alive==3){
                    if(!c.roadAccess||s.building.type==59||s.building.type==62||s.building.type==66||s.building.type==67)
                        blocker=OriginalBuildingUpdateBlocker::RoadRefresh;
                    else {s.roadDistance=c.roadAccess->distance;s.roadX=c.roadAccess->x;s.roadY=c.roadAccess->y;
                    if(house)s.roadFailureCount=0;
                    if(c.roadAccess->network)s.roadNetwork=c.roadAccess->network;}}}
            else if(phase==22)blocker=originalBuildingHouseCapacityStep(s,c);
            else if(phase==23)blocker=OriginalBuildingUpdateBlocker::CityLabor;
            else if(phase==25)blocker=OriginalBuildingUpdateBlocker::Employment;
            else if(phase==39&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution;
            else if(phase==6&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution; //5185C0 countdown
            else if(phase==44)blocker=OriginalBuildingUpdateBlocker::RiskAndFire;
            else if(phase==31&&(s.inputs.alive==1||s.inputs.alive==3)){
                if(crop){ //4C1040 ->4C12E0, harvest flag on first month part
                    if(*c.monthPart==0&&*c.month==(s.building.type==194?8u:10u))
                        originalBuildingSetCropFlags(s.inputs,uint16_t(originalBuildingCropFlags(s.inputs)|0x8000));
                    ++out.virtual20Calls;
                }else if(s.building.type==231)++out.virtual20Calls; //428F00 false
                else blocker=OriginalBuildingUpdateBlocker::BuildingVirtual20;
            }else if(phase==20){blocker=originalBuildingProductionStep(s,c);++out.productionCalls;}
            else if(phase==35&&house&&(s.inputs.alive==1||s.inputs.alive==3)){originalBuildingDecayHouseServices(s.inputs);++out.serviceDecayCalls;}
            // Other phases may dispatch further virtuals. They remain named
            // blockers; a complete state never crosses them silently.
            else if(phase==12)blocker=OriginalBuildingUpdateBlocker::Availability; //4ACE60 availability
            else if(phase==36&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution;
            else if(phase==1)blocker=OriginalBuildingUpdateBlocker::CityPhase;
            else if(phase!=0&&phase!=2&&phase!=14&&phase!=15&&phase!=19&&phase!=26&&phase!=34&&
                phase!=35&&phase!=36&&phase!=39&&phase!=41&&phase!=42&&phase!=46&&phase!=48&&phase!=49&&phase!=50)
                blocker=OriginalBuildingUpdateBlocker::UnclosedPhase;
            if(blocker==OriginalBuildingUpdateBlocker::None){
                if(phase==48&&s.inputs.alive==1&&s.inputs.byte52)--s.inputs.byte52; //4AE8B0
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
    }
    out.afterSelection=originalBuildingBaseSelection(s.building);
    if(crop&&saved.building.imageInputs&&s.building.imageInputs&&saved.building.imageInputs->cropWord4!=s.building.imageInputs->cropWord4)++out.changedSelectorInputs;
    if(house&&saved.inputs.house&&s.inputs.house&&saved.inputs.house->at(55)!=s.inputs.house->at(55))++out.changedSelectorInputs;
    if(out.beforeSelection&&out.afterSelection)out.selectorChanged=out.beforeSelection->code!=out.afterSelection->code||out.beforeSelection->variant!=out.afterSelection->variant;
    out.blocker=OriginalBuildingUpdateBlocker::None;out.state=std::move(s);out.context=c;return out;
}
} // namespace game_lab
