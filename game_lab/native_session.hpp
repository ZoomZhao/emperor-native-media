// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "original_building_update.hpp"

namespace game_lab {
// A deliberately authored ruleset over original scene data. Never an EMP1 or
// original-engine continuation. Every object is native-owned and source-bound.
inline constexpr const char* nativeSessionRules = "original-data-native-v1";
inline constexpr const char* nativeWaterRules = "original-data-native-v2";
inline constexpr const char* nativeFoodRules = "original-data-native-v3";
inline constexpr const char* nativeDemolitionRules = "original-data-native-v4";
inline constexpr const char* nativeTaxRules = "original-data-native-v5";
// v6: original workforce share (4AD850) and job allocation (4F1590/51E310)
// staff Hunters/Wells; staffing scales Hunter meat. Growth gating is authored.
inline constexpr const char* nativeLaborRules = "original-data-native-v6";
inline constexpr uint32_t nativeLatestRulesVersion=6;
inline constexpr uint32_t nativeTaxPeriod=4,nativeTaxPerResident=1; // authored gold per connected resident, not original taxation
inline constexpr uint32_t nativeFoodYield=4,nativeFoodCapacity=100; // meat units, independently authored
inline constexpr int nativeWaterRadius=4; // authored v2 coverage, not original walkers/capacity
inline const char* nativeRulesName(uint32_t version){return version==1?nativeSessionRules:version==2?nativeWaterRules:version==3?nativeFoodRules:version==4?nativeDemolitionRules:version==5?nativeTaxRules:version==6?nativeLaborRules:nullptr;}
struct NativeSessionSeed {
    std::string source;
    uint64_t sourceRevision=0, modelRevision=0;
    uint32_t side=0, roadCost=0, houseCost=0, houseCapacity=0, wellCost=0, hunterCost=0;
    int64_t initialFunds=0;
    std::vector<uint8_t> buildable, sourceRoad;
    // v6 inputs from the source SAV and original model employment column5.
    // Zero means unavailable: v6 is then refused, earlier versions unaffected.
    uint32_t laborPercent=0, hunterWorkers=0, wellWorkers=0;
    std::array<int32_t,10> laborPriority{};
};
enum class NativeObjectKind : uint32_t { Road=22, House=2, Well=72, Hunter=33 };
struct NativeSessionObject {
    uint64_t id=0; NativeObjectKind kind=NativeObjectKind::Road;
    uint32_t x=0,y=0,residents=0;
};
struct NativeRetiredObject { uint64_t id=0; NativeObjectKind kind=NativeObjectKind::Road; };
struct NativeFoodCycle {uint32_t produced=0,demand=0,fed=0,arrivals=0;};
struct NativeTaxCycle {uint32_t eligible=0,disconnected=0,emptyHouses=0;uint64_t income=0;};
struct NativeSessionState {
    uint32_t rulesVersion=1; // decoded v1 always keeps its original behavior
    uint64_t revision=0,ticks=0,nextId=1;
    bool paused=true,huntingEnabled=true;
    uint32_t foodStock=0;uint64_t foodProduced=0,foodConsumed=0;NativeFoodCycle lastFood;
    uint64_t taxCycles=0,taxReceived=0;NativeTaxCycle lastTax;
    int64_t funds=0;
    std::vector<NativeSessionObject> objects;
    std::vector<NativeRetiredObject> retired; // v4 no-refund cost/identity history; no slot reuse
    uint32_t departedSinceMeal=0; // preserves the historical food receipt after demolition
};
enum class NativeSessionAction { Road,House,Tick,Pause,Resume,RoadPath,Well,Hunter,HuntingPause,HuntingResume,Demolish };
struct NativeSessionCommand { NativeSessionAction action; uint64_t expectedRevision; int64_t x=0,y=0,toX=0,toY=0; uint64_t objectId=0; };
struct NativeSessionResult { bool committed=false; std::string error; };
inline bool validNativeSeed(const NativeSessionSeed& s) {
    return !s.source.empty()&&s.source.size()<1024&&s.source.find_first_of("\r\n\0",0,3)==std::string::npos&&
        s.source.front()!='/'&&s.source.find("..")==std::string::npos&&s.sourceRevision&&s.modelRevision&&
        s.side>0&&s.side<=228&&s.buildable.size()==size_t(s.side)*s.side&&s.sourceRoad.size()==s.buildable.size()&&
        s.initialFunds>=0&&s.initialFunds<=INT32_MAX&&s.roadCost>0&&s.roadCost<=100000&&s.houseCost>0&&s.houseCost<=100000&&
        s.houseCapacity>0&&s.houseCapacity<=100&&
        std::all_of(s.buildable.begin(),s.buildable.end(),[](auto v){return v<=1;})&&
        std::all_of(s.sourceRoad.begin(),s.sourceRoad.end(),[](auto v){return v<=1;});
}
inline unsigned nativeObjectSide(NativeObjectKind k){return k==NativeObjectKind::Road?1:2;}
inline std::vector<uint8_t> nativeRoads(const NativeSessionSeed& seed,const NativeSessionState& s) {
    auto roads=seed.sourceRoad;
    for(const auto& o:s.objects)if(o.kind==NativeObjectKind::Road)roads[size_t(o.y)*seed.side+o.x]=1;
    return roads;
}
inline bool nativeHouseConnected(const NativeSessionSeed& seed,const NativeSessionObject& h,const std::vector<uint8_t>& roads) {
    for(int d=0;d<2;++d)for(auto p:{std::pair<int,int>{int(h.x)+d,int(h.y)-1},{int(h.x)+d,int(h.y)+2},
        {int(h.x)-1,int(h.y)+d},{int(h.x)+2,int(h.y)+d}})
        if(p.first>=0&&p.second>=0&&p.first<int(seed.side)&&p.second<int(seed.side)&&roads[size_t(p.second)*seed.side+p.first])return true;
    return false;
}
inline bool nativeWellActive(const NativeSessionSeed& seed,const NativeSessionObject& well,const std::vector<uint8_t>& roads){
    return well.kind==NativeObjectKind::Well&&nativeHouseConnected(seed,well,roads); // both footprints are2x2
}
inline std::vector<uint8_t> nativeWaterCoverage(const NativeSessionSeed& seed,const NativeSessionState& s) {
    std::vector<uint8_t> covered(size_t(seed.side)*seed.side,0);if(s.rulesVersion<2)return covered;
    const auto roads=nativeRoads(seed,s);
    for(const auto& w:s.objects)if(nativeWellActive(seed,w,roads)){
        const int left=std::max(0,int(w.x)-nativeWaterRadius),right=std::min(int(seed.side)-1,int(w.x)+1+nativeWaterRadius);
        const int top=std::max(0,int(w.y)-nativeWaterRadius),bottom=std::min(int(seed.side)-1,int(w.y)+1+nativeWaterRadius);
        for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x)covered[size_t(y)*seed.side+x]=1;
    }
    return covered;
}
inline bool nativeHouseWatered(const NativeSessionSeed& seed,const NativeSessionObject& house,const std::vector<uint8_t>& covered){
    if(house.kind!=NativeObjectKind::House||house.x>=seed.side||house.y>=seed.side||seed.side-house.x<2||seed.side-house.y<2||covered.size()!=size_t(seed.side)*seed.side)return false;
    for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)if(!covered[size_t(house.y+dy)*seed.side+house.x+dx])return false;return true;
}
inline uint32_t nativeObjectCost(const NativeSessionSeed& seed,NativeObjectKind kind){return kind==NativeObjectKind::Road?seed.roadCost:kind==NativeObjectKind::Well?seed.wellCost:kind==NativeObjectKind::Hunter?seed.hunterCost:seed.houseCost;}
inline std::string validateNativeSession(const NativeSessionSeed& seed,const NativeSessionState& s) {
    if(!validNativeSeed(seed))return "unsupported_source_or_model";
    if(!nativeRulesName(s.rulesVersion))return "unsupported_native_rules";
    if(s.rulesVersion>=2&&(!seed.wellCost||seed.wellCost>100000))return "missing_well_model_price";
    if(s.rulesVersion>=3&&(!seed.hunterCost||seed.hunterCost>100000))return "missing_hunter_model_price";
    if(s.rulesVersion>=6&&(!seed.laborPercent||seed.laborPercent>100||!seed.hunterWorkers||seed.hunterWorkers>1000||seed.wellWorkers>1000||
        std::any_of(seed.laborPriority.begin(),seed.laborPriority.end(),[](int32_t p){return p<0||p>5;})))return "missing_labor_model";
    if(s.objects.size()+s.retired.size()>3999||s.nextId!=s.objects.size()+s.retired.size()+1||s.revision<s.ticks||s.revision>1000000000000ULL||s.ticks>1000000000ULL)return "invalid_session_counters";
    if(s.rulesVersion<4&&(!s.retired.empty()||s.departedSinceMeal))return "demolition_requires_native_v4";
    const auto validKind=[&](NativeObjectKind k){return k==NativeObjectKind::Road||k==NativeObjectKind::House||(s.rulesVersion>=2&&k==NativeObjectKind::Well)||(s.rulesVersion>=3&&k==NativeObjectKind::Hunter);};
    auto occupied=seed.sourceRoad;int64_t spent=0;uint64_t previous=0;std::set<uint64_t> identities;size_t retiredHouses=0;
    for(const auto& o:s.retired){
        if(!o.id||o.id>=s.nextId||o.id<=previous||!validKind(o.kind)||!identities.insert(o.id).second)return "invalid_retired_identity";
        previous=o.id;spent+=nativeObjectCost(seed,o.kind);retiredHouses+=o.kind==NativeObjectKind::House;
    }
    previous=0;
    for(const auto& o:s.objects){
        if(!o.id||o.id>=s.nextId||o.id<=previous||!validKind(o.kind)||!identities.insert(o.id).second)return "invalid_object_identity";previous=o.id;
        const auto n=nativeObjectSide(o.kind);
        if(o.x>=seed.side||o.y>=seed.side||n>seed.side-o.x||n>seed.side-o.y)return "outside_scene";
        if((o.kind!=NativeObjectKind::House&&o.residents)||o.residents>seed.houseCapacity||o.residents>s.ticks/2)return "invalid_residents";
        for(unsigned y=o.y;y<o.y+n;++y)for(unsigned x=o.x;x<o.x+n;++x){const size_t at=size_t(y)*seed.side+x;
            if(!seed.buildable[at]||occupied[at])return "occupied_or_unsupported_terrain";occupied[at]=1;}
        spent+=nativeObjectCost(seed,o.kind);
    }
    const auto& tax=s.lastTax;
    if(s.rulesVersion<5){if(s.taxCycles||s.taxReceived||tax.eligible||tax.disconnected||tax.emptyHouses||tax.income)return "tax_requires_native_v5";}
    else {
        if(s.taxCycles!=s.ticks/nativeTaxPeriod||s.taxReceived>s.taxCycles*399900ULL*nativeTaxPerResident||
           uint64_t(tax.eligible)+tax.disconnected>399900||tax.emptyHouses>3999||tax.income!=uint64_t(tax.eligible)*nativeTaxPerResident||tax.income>s.taxReceived||
           (!s.taxCycles&&(tax.eligible||tax.disconnected||tax.emptyHouses||tax.income)))return "invalid_tax_ledger";
    }
    // Validate the bounded cumulative income before any signed conversion.
    const int64_t available=seed.initialFunds+int64_t(s.taxReceived);
    if(spent>available||s.funds!=available-spent||s.funds<0||s.funds>INT32_MAX)return "invalid_balance";
    const auto roads=nativeRoads(seed,s);
    for(const auto& o:s.objects)if(s.rulesVersion<4&&o.residents&&!nativeHouseConnected(seed,o,roads))return "inhabited_house_without_road";
    uint64_t population=0;size_t houses=0;for(const auto& o:s.objects){population+=o.residents;houses+=o.kind==NativeObjectKind::House;}
    const auto& meal=s.lastFood;
    if(s.rulesVersion<3){if(!s.huntingEnabled||s.foodStock||s.foodProduced||s.foodConsumed||meal.produced||meal.demand||meal.fed||meal.arrivals)return "food_requires_native_v3";}
    else {
        // v6 arrivals are not charged meat, so consumption no longer bounds population.
        const bool paid=s.rulesVersion<6;const uint64_t charged=uint64_t(meal.fed)+(paid?meal.arrivals:0);
        if(s.foodStock>nativeFoodCapacity||s.foodProduced>uint64_t(nativeFoodCapacity)*(s.ticks/2)||s.foodConsumed>s.foodProduced||s.foodProduced-s.foodConsumed!=s.foodStock||(paid&&s.foodConsumed<population))return "invalid_food_ledger";
        if(meal.produced>nativeFoodCapacity||meal.fed>meal.demand||meal.demand>399900||meal.arrivals>houses+retiredHouses||charged>nativeFoodCapacity||
            (paid&&meal.fed<meal.demand&&meal.arrivals)||meal.produced>s.foodProduced||charged>s.foodConsumed||uint64_t(meal.demand)+meal.arrivals!=population+s.departedSinceMeal||s.departedSinceMeal>retiredHouses*seed.houseCapacity||
            (s.ticks<2&&(meal.produced||meal.demand||meal.fed||meal.arrivals)))return "invalid_food_cycle";
    }
    return {};
}
// A path is determined solely by its endpoints: X first, then Y, inclusive.
// Existing original/native roads are retained and free. Every other cell must
// be buildable and vacant. No path-history edits or original SAV writes.
struct NativeRoadPathCell {uint32_t x=0,y=0;bool existing=false;};
struct NativeRoadPathPlan {std::vector<NativeRoadPathCell> cells;size_t newCells=0;int64_t cost=0;std::string error;};
inline NativeRoadPathPlan planNativeRoadPath(const NativeSessionSeed& seed,const NativeSessionState& state,int64_t x,int64_t y,int64_t toX,int64_t toY) {
    NativeRoadPathPlan plan;plan.error=validateNativeSession(seed,state);if(!plan.error.empty())return plan;
    if(x<0||y<0||toX<0||toY<0||x>=seed.side||y>=seed.side||toX>=seed.side||toY>=seed.side){plan.error="outside_scene";return plan;}
    const auto roads=nativeRoads(seed,state);std::vector<uint8_t> houses(size_t(seed.side)*seed.side,0);
    for(const auto& o:state.objects)if(o.kind!=NativeObjectKind::Road)for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)houses[size_t(o.y+dy)*seed.side+o.x+dx]=1;
    const auto add=[&](int64_t px,int64_t py){const size_t at=size_t(py)*seed.side+size_t(px);const bool existing=roads[at]!=0;
        plan.cells.push_back({uint32_t(px),uint32_t(py),existing});if(existing)return;
        ++plan.newCells;if(!seed.buildable[at]||houses[at])plan.error="occupied_or_unsupported_terrain";};
    add(x,y);while(x!=toX){x+=x<toX?1:-1;add(x,y);}while(y!=toY){y+=y<toY?1:-1;add(x,y);}
    // Max455 cells * bounded100000 price; computed in64 bits regardless.
    plan.cost=int64_t(plan.newCells)*seed.roadCost;
    if(plan.error.empty()){
        if(state.objects.size()+state.retired.size()+plan.newCells>3999)plan.error="object_limit";
        else if(state.funds<plan.cost)plan.error="insufficient_funds";
        else if(!plan.newCells)plan.error="road_path_already_present";
    }
    return plan;
}
// Native-only ownership: a source road/terrain/building can never produce an ID.
inline std::optional<uint64_t> nativeObjectAt(const NativeSessionState& state,int64_t x,int64_t y){
    if(x<0||y<0)return std::nullopt;
    for(const auto& o:state.objects)if(x>=o.x&&y>=o.y&&uint64_t(x)-o.x<nativeObjectSide(o.kind)&&uint64_t(y)-o.y<nativeObjectSide(o.kind))return o.id;
    return std::nullopt;
}
struct NativeServices {uint32_t wells=0,wateredHouses=0,hunters=0,connectedHouses=0;};
inline NativeServices nativeServices(const NativeSessionSeed& seed,const NativeSessionState& state){
    NativeServices result;const auto roads=nativeRoads(seed,state),water=nativeWaterCoverage(seed,state);
    for(const auto& o:state.objects){const bool connected=nativeHouseConnected(seed,o,roads);
        if(o.kind==NativeObjectKind::Well)result.wells+=connected;
        if(o.kind==NativeObjectKind::Hunter)result.hunters+=connected&&state.huntingEnabled;
        if(o.kind==NativeObjectKind::House){result.connectedHouses+=connected;result.wateredHouses+=nativeHouseWatered(seed,o,water);}}
    return result;
}
struct NativeDemolitionPlan {
    NativeSessionObject object;NativeServices before,after;std::optional<NativeSessionState> candidate;std::string error;
};
inline NativeDemolitionPlan planNativeDemolition(const NativeSessionSeed& seed,const NativeSessionState& state,uint64_t id){
    NativeDemolitionPlan plan;plan.error=validateNativeSession(seed,state);if(!plan.error.empty())return plan;
    if(state.rulesVersion<4){plan.error="demolition_requires_native_v4";return plan;}
    if(!state.paused){plan.error="pause_before_demolition";return plan;}
    const auto found=std::find_if(state.objects.begin(),state.objects.end(),[&](const auto& o){return o.id==id;});
    if(found==state.objects.end()){plan.error="native_object_not_found";return plan;}
    plan.object=*found;plan.before=nativeServices(seed,state);auto next=state;
    next.retired.push_back({found->id,found->kind});std::sort(next.retired.begin(),next.retired.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    next.departedSinceMeal+=found->residents;next.objects.erase(next.objects.begin()+std::distance(state.objects.begin(),found));
    plan.error=validateNativeSession(seed,next);if(!plan.error.empty())return plan;
    plan.after=nativeServices(seed,next);plan.candidate=std::move(next);return plan;
}
// v6 staffing. Labor is the original share of current residents; demand is
// model column5 for road-connected Wells (category3) and enabled Hunters
// (category0) in ascending native id, the original ascending-slot order.
struct NativeLaborJob {uint64_t id=0;NativeObjectKind kind=NativeObjectKind::Road;bool eligible=false;uint32_t required=0,workers=0;};
struct NativeLaborPlan {int32_t available=0,demand=0,employed=0,unemployed=0,shortage=0;std::vector<NativeLaborJob> jobs;std::string error;
    bool hunterUnderstaffed()const{return std::any_of(jobs.begin(),jobs.end(),[](const auto& j){return j.kind==NativeObjectKind::Hunter&&j.eligible&&j.workers<j.required;});}
    const NativeLaborJob* find(uint64_t id)const{for(const auto& j:jobs)if(j.id==id)return &j;return nullptr;}};
inline NativeLaborPlan planNativeLabor(const NativeSessionSeed& seed,const NativeSessionState& state){
    NativeLaborPlan plan;plan.error=validateNativeSession(seed,state);if(!plan.error.empty())return plan;
    if(state.rulesVersion<6){plan.error="labor_requires_native_v6";return plan;}
    const auto roads=nativeRoads(seed,state);uint64_t population=0;for(const auto& o:state.objects)population+=o.residents;
    OriginalBuildingCityEmployment city;city.shortageNotice=city.unemploymentNotice=true; // notification effects are not used
    city.available=std::max(0,originalBuildingPercent(int32_t(population),int32_t(seed.laborPercent)));
    for(unsigned n=0;n<10;++n)city.rows[n].priority=seed.laborPriority[n];
    struct Candidate{size_t job;unsigned category;int required;};std::vector<Candidate> candidates;
    for(const auto& o:state.objects){if(o.kind!=NativeObjectKind::Hunter&&o.kind!=NativeObjectKind::Well)continue;
        NativeLaborJob job{o.id,o.kind,false,o.kind==NativeObjectKind::Hunter?seed.hunterWorkers:seed.wellWorkers,0};
        job.eligible=nativeHouseConnected(seed,o,roads)&&(o.kind!=NativeObjectKind::Hunter||state.huntingEnabled);
        const unsigned category=unsigned(*originalBuildingEmploymentCategory(uint16_t(o.kind)));
        if(job.eligible){auto& row=city.rows[category];row.demand=originalBuildingAdd32(row.demand,int32_t(job.required));candidates.push_back({plan.jobs.size(),category,int(job.required)});}
        plan.jobs.push_back(job);}
    const auto allocated=originalBuildingCityEmploymentStep(city);if(!allocated){plan.error="labor_allocation_refused";return plan;}
    const auto& c=allocated->state;
    // 51E310: proportional share per category, then ordered integer remainders.
    std::array<int32_t,10> left{};
    for(unsigned n=0;n<10;++n){const auto& r=c.rows[n];if(r.allocated<r.demand)left[n]=r.allocated-r.preferredAllocated;}
    for(const auto& k:candidates){const auto& r=c.rows[k.category];
        const auto percent=originalBuildingRatio(r.allocated-r.preferredAllocated,r.demand-r.preferred);if(!percent){plan.error="labor_allocation_refused";return plan;}
        auto& job=plan.jobs[k.job];job.workers=uint32_t(std::min(k.required,originalBuildingPercent(k.required,*percent)));left[k.category]=originalBuildingSub32(left[k.category],int32_t(job.workers));}
    for(const auto& k:candidates){auto& job=plan.jobs[k.job];if(int(job.workers)>=k.required)continue;
        const int amount=std::min(k.required-int(job.workers),left[k.category]);if(amount<0){plan.error="labor_allocation_refused";return plan;}
        job.workers+=uint32_t(amount);left[k.category]-=amount;}
    plan.available=c.available;plan.employed=c.employed;plan.unemployed=c.unemployed;plan.shortage=c.shortage;
    for(unsigned n=0;n<9;++n)plan.demand=originalBuildingAdd32(plan.demand,c.rows[n].demand);
    return plan;
}
// Authored v6 production: floor(yield x workers / required) per staffed Hunter.
inline uint32_t nativeHunterYield(const NativeLaborJob& job){return job.kind==NativeObjectKind::Hunter&&job.eligible&&job.required?uint32_t(uint64_t(nativeFoodYield)*job.workers/job.required):0;}
struct NativeFoodPlan {uint32_t stock=0;NativeFoodCycle cycle;std::vector<size_t> arrivals;bool laborOverride=false;std::string error;};
inline NativeFoodPlan planNativeFoodCycle(const NativeSessionSeed& seed,const NativeSessionState& state){
    NativeFoodPlan plan;plan.error=validateNativeSession(seed,state);if(!plan.error.empty())return plan;
    if(state.rulesVersion<3){plan.error="food_requires_native_v3";return plan;}
    const auto roads=nativeRoads(seed,state),water=nativeWaterCoverage(seed,state);uint32_t offered=0;
    std::optional<NativeLaborPlan> labor;if(state.rulesVersion>=6){labor=planNativeLabor(seed,state);if(!labor->error.empty()){plan.error=labor->error;return plan;}}
    for(const auto& o:state.objects){plan.cycle.demand+=o.residents;
        if(labor){if(const auto* job=labor->find(o.id))offered+=nativeHunterYield(*job);}
        else if(state.huntingEnabled&&o.kind==NativeObjectKind::Hunter&&nativeHouseConnected(seed,o,roads))offered+=nativeFoodYield;}
    plan.cycle.produced=std::min(offered,nativeFoodCapacity-state.foodStock);plan.stock=state.foodStock+plan.cycle.produced;
    plan.cycle.fed=std::min(plan.stock,plan.cycle.demand);plan.stock-=plan.cycle.fed;
    // v6: an unfed city keeps growing while an enabled Hunter lacks workers;
    // arrivals are free and not limited by stock. Earlier versions unchanged.
    plan.laborOverride=labor&&plan.cycle.fed<plan.cycle.demand&&labor->hunterUnderstaffed();
    if(plan.cycle.fed==plan.cycle.demand||plan.laborOverride)for(size_t n=0;n<state.objects.size()&&(labor||plan.stock);++n){const auto& o=state.objects[n];
        if(o.kind==NativeObjectKind::House&&o.residents<seed.houseCapacity&&nativeHouseConnected(seed,o,roads)&&nativeHouseWatered(seed,o,water)){
            if(!labor)--plan.stock;plan.arrivals.push_back(n);++plan.cycle.arrivals;}}
    return plan;
}
struct NativeTaxPlan {NativeTaxCycle cycle;std::string error;};
inline NativeTaxPlan planNativeTaxCycle(const NativeSessionSeed& seed,const NativeSessionState& state){
    NativeTaxPlan plan;plan.error=validateNativeSession(seed,state);if(!plan.error.empty())return plan;
    if(state.rulesVersion<5){plan.error="tax_requires_native_v5";return plan;}
    const auto roads=nativeRoads(seed,state);
    for(const auto& o:state.objects)if(o.kind==NativeObjectKind::House){
        plan.cycle.emptyHouses+=!o.residents;
        (nativeHouseConnected(seed,o,roads)?plan.cycle.eligible:plan.cycle.disconnected)+=o.residents;}
    plan.cycle.income=uint64_t(plan.cycle.eligible)*nativeTaxPerResident;
    if(plan.cycle.income>uint64_t(INT32_MAX-state.funds)||plan.cycle.income>UINT64_MAX-state.taxReceived)plan.error="tax_overflow";
    return plan;
}
inline NativeSessionState startNativeSession(const NativeSessionSeed& seed,uint32_t rulesVersion=1) {NativeSessionState s;s.rulesVersion=rulesVersion;s.funds=seed.initialFunds;return s;}
inline NativeSessionResult transactNativeSession(const NativeSessionSeed& seed,NativeSessionState& state,const NativeSessionCommand& cmd) {
    const auto invalid=validateNativeSession(seed,state);if(!invalid.empty())return {false,invalid};
    if(cmd.expectedRevision!=state.revision)return {false,"stale_command"};
    if(state.revision==UINT64_MAX)return {false,"revision_exhausted"};
    auto next=state;
    switch(cmd.action){
    case NativeSessionAction::Road:case NativeSessionAction::House:case NativeSessionAction::Well:case NativeSessionAction::Hunter:{
        if(cmd.action==NativeSessionAction::Well&&state.rulesVersion<2)return {false,"well_requires_native_v2"};
        if(cmd.action==NativeSessionAction::Hunter&&state.rulesVersion<3)return {false,"hunter_requires_native_v3"};
        if(cmd.x<0||cmd.y<0||cmd.x>=seed.side||cmd.y>=seed.side)return {false,"outside_scene"};
        const auto kind=cmd.action==NativeSessionAction::Road?NativeObjectKind::Road:cmd.action==NativeSessionAction::Well?NativeObjectKind::Well:cmd.action==NativeSessionAction::Hunter?NativeObjectKind::Hunter:NativeObjectKind::House;
        const auto cost=nativeObjectCost(seed,kind);
        if(next.funds<cost)return {false,"insufficient_funds"};
        if(next.objects.size()+next.retired.size()>=3999)return {false,"object_limit"};
        next.objects.push_back({next.nextId++,kind,uint32_t(cmd.x),uint32_t(cmd.y),0});next.funds-=cost;break;}
    case NativeSessionAction::RoadPath:{
        const auto plan=planNativeRoadPath(seed,state,cmd.x,cmd.y,cmd.toX,cmd.toY);if(!plan.error.empty())return {false,plan.error};
        for(const auto& cell:plan.cells)if(!cell.existing)next.objects.push_back({next.nextId++,NativeObjectKind::Road,cell.x,cell.y,0});
        next.funds-=plan.cost;break;}
    case NativeSessionAction::Demolish:{
        const auto plan=planNativeDemolition(seed,state,cmd.objectId);if(!plan.candidate)return {false,plan.error};next=*plan.candidate;break;}
    case NativeSessionAction::Pause:if(next.paused)return {false,"already_paused"};next.paused=true;break;
    case NativeSessionAction::Resume:if(!next.paused)return {false,"already_running"};next.paused=false;break;
    case NativeSessionAction::HuntingPause:case NativeSessionAction::HuntingResume:
        if(state.rulesVersion<3)return {false,"food_requires_native_v3"};
        if(next.huntingEnabled==(cmd.action==NativeSessionAction::HuntingResume))return {false,"hunting_already_in_requested_state"};
        next.huntingEnabled=cmd.action==NativeSessionAction::HuntingResume;break;
    case NativeSessionAction::Tick:{
        if(next.paused)return {false,"paused"};if(next.ticks>=1000000000ULL)return {false,"tick_limit"};
        // Tax assesses pre-tick residents. Receipt, funds, meal and population
        // are one candidate; any later validation/drawing failure publishes none.
        if(next.rulesVersion>=5&&(next.ticks+1)%nativeTaxPeriod==0){
            const auto plan=planNativeTaxCycle(seed,state);if(!plan.error.empty())return {false,plan.error};
            ++next.taxCycles;next.taxReceived+=plan.cycle.income;next.funds+=int64_t(plan.cycle.income);next.lastTax=plan.cycle;
        }
        ++next.ticks;if(next.ticks%2==0){
            if(next.rulesVersion>=3){const auto plan=planNativeFoodCycle(seed,next);if(!plan.error.empty())return {false,plan.error};
                next.foodStock=plan.stock;next.foodProduced+=plan.cycle.produced;next.foodConsumed+=uint64_t(plan.cycle.fed)+(next.rulesVersion<6?plan.cycle.arrivals:0);next.lastFood=plan.cycle;next.departedSinceMeal=0;
                for(const auto n:plan.arrivals)++next.objects[n].residents;
            }else{const auto roads=nativeRoads(seed,next);const auto water=nativeWaterCoverage(seed,next);
                for(auto& o:next.objects)if(o.kind==NativeObjectKind::House&&o.residents<seed.houseCapacity&&nativeHouseConnected(seed,o,roads)&&(next.rulesVersion==1||nativeHouseWatered(seed,o,water)))++o.residents;}}
        break;}
    default:return {false,"unsupported_command"};
    }
    ++next.revision;const auto error=validateNativeSession(seed,next);if(!error.empty())return {false,error};
    state=std::move(next);return {true,{}};
}
inline uint64_t nativePopulation(const NativeSessionState& s){uint64_t n=0;for(const auto& o:s.objects)n+=o.residents;return n;}
inline uint64_t nativeTextRevision(const std::string& bytes){uint64_t h=14695981039346656037ULL;for(unsigned char c:bytes){h^=c;h*=1099511628211ULL;}return h;}
inline std::string encodeNativeSession(const NativeSessionSeed& seed,const NativeSessionState& s) {
    if(!validateNativeSession(seed,s).empty())return {};
    std::ostringstream out;out<<"ENCITY "<<s.rulesVersion<<' '<<nativeRulesName(s.rulesVersion)<<'\n'<<std::quoted(seed.source)<<' '<<seed.sourceRevision<<' '<<seed.modelRevision<<'\n'
        <<seed.side<<' '<<seed.initialFunds<<' '<<seed.roadCost<<' '<<seed.houseCost<<' '<<seed.houseCapacity;
    if(s.rulesVersion>=2)out<<' '<<seed.wellCost<<' '<<nativeWaterRadius;
    if(s.rulesVersion>=3)out<<' '<<seed.hunterCost;
    out<<'\n'<<s.revision<<' '<<s.ticks<<' '<<s.nextId<<' '<<s.paused<<' '<<s.funds<<' '<<s.objects.size()<<'\n';
    if(s.rulesVersion>=3){out<<"FOOD meat "<<nativeFoodYield<<' '<<nativeFoodCapacity<<' '<<s.huntingEnabled<<' '<<s.foodStock<<' '<<s.foodProduced<<' '<<s.foodConsumed<<' '<<s.lastFood.produced<<' '<<s.lastFood.demand<<' '<<s.lastFood.fed<<' '<<s.lastFood.arrivals;if(s.rulesVersion>=4)out<<' '<<s.departedSinceMeal;out<<'\n';}
    if(s.rulesVersion>=5)out<<"TAX "<<nativeTaxPeriod<<' '<<nativeTaxPerResident<<' '<<s.taxCycles<<' '<<s.taxReceived<<' '<<s.lastTax.eligible<<' '<<s.lastTax.disconnected<<' '<<s.lastTax.emptyHouses<<' '<<s.lastTax.income<<'\n';
    if(s.rulesVersion>=6){out<<"LABOR "<<seed.laborPercent<<' '<<seed.hunterWorkers<<' '<<seed.wellWorkers;for(auto p:seed.laborPriority)out<<' '<<p;out<<'\n';}
    for(const auto& o:s.objects)out<<o.id<<' '<<uint32_t(o.kind)<<' '<<o.x<<' '<<o.y<<' '<<o.residents<<'\n';
    if(s.rulesVersion>=4){out<<"RETIRED "<<s.retired.size()<<'\n';for(const auto& o:s.retired)out<<o.id<<' '<<uint32_t(o.kind)<<'\n';}
    return out.str();
}
// Bounded strict parser: never restores original memory, event bytes or asset
// payloads. The original source/model must still match the externally loaded seed.
inline std::optional<NativeSessionState> decodeNativeSession(const NativeSessionSeed& seed,const std::string& bytes,std::string& error) {
    error="invalid_native_session";if(bytes.size()>512000||bytes.find('\0')!=std::string::npos||!validNativeSeed(seed))return std::nullopt;
    std::istringstream in(bytes);std::string magic,rules,source;unsigned version=0;
    uint64_t revision=0,model=0;uint32_t side=0,road=0,house=0,cap=0;int64_t funds=0;
    if(!(in>>magic>>version>>rules)||magic!="ENCITY"||!nativeRulesName(version)||rules!=nativeRulesName(version)){error="unsupported_session_version";return std::nullopt;}
    if(!(in>>std::quoted(source)>>revision>>model>>side>>funds>>road>>house>>cap)||source!=seed.source||revision!=seed.sourceRevision||model!=seed.modelRevision||
        side!=seed.side||funds!=seed.initialFunds||road!=seed.roadCost||house!=seed.houseCost||cap!=seed.houseCapacity){error="source_or_model_mismatch";return std::nullopt;}
    if(version>=2){uint32_t well=0,radius=0;if(!(in>>well>>radius)||well!=seed.wellCost||radius!=nativeWaterRadius){error="water_rule_or_model_mismatch";return std::nullopt;}}
    if(version>=3){uint32_t hunter=0;if(!(in>>hunter)||hunter!=seed.hunterCost){error="hunter_model_mismatch";return std::nullopt;}}
    NativeSessionState s;s.rulesVersion=version;unsigned paused=0;size_t count=0;
    if(!(in>>s.revision>>s.ticks>>s.nextId>>paused>>s.funds>>count)||paused>1||count>3999)return std::nullopt;s.paused=paused!=0;
    if(version>=3){std::string tag,food;uint32_t yield=0,capacity=0,enabled=0;
        if(!(in>>tag>>food>>yield>>capacity>>enabled>>s.foodStock>>s.foodProduced>>s.foodConsumed>>s.lastFood.produced>>s.lastFood.demand>>s.lastFood.fed>>s.lastFood.arrivals)||tag!="FOOD"||food!="meat"||yield!=nativeFoodYield||capacity!=nativeFoodCapacity||enabled>1){error="invalid_food_rule_record";return std::nullopt;}s.huntingEnabled=enabled!=0;if(version>=4&&!(in>>s.departedSinceMeal))return std::nullopt;}
    if(version>=5){std::string tag;uint32_t period=0,rate=0;
        if(!(in>>tag>>period>>rate>>s.taxCycles>>s.taxReceived>>s.lastTax.eligible>>s.lastTax.disconnected>>s.lastTax.emptyHouses>>s.lastTax.income)||tag!="TAX"||period!=nativeTaxPeriod||rate!=nativeTaxPerResident){error="invalid_tax_rule_record";return std::nullopt;}}
    if(version>=6){std::string tag;uint32_t percent=0,hunter=0,well=0;std::array<int32_t,10> priority{};
        if(!(in>>tag>>percent>>hunter>>well)||tag!="LABOR"){error="invalid_labor_rule_record";return std::nullopt;}for(auto& p:priority)if(!(in>>p)){error="invalid_labor_rule_record";return std::nullopt;}
        if(percent!=seed.laborPercent||hunter!=seed.hunterWorkers||well!=seed.wellWorkers||priority!=seed.laborPriority){error="labor_model_mismatch";return std::nullopt;}}
    for(size_t n=0;n<count;++n){NativeSessionObject o;uint32_t kind=0;if(!(in>>o.id>>kind>>o.x>>o.y>>o.residents))return std::nullopt;o.kind=NativeObjectKind(kind);s.objects.push_back(o);}
    if(version>=4){std::string tag;size_t retired=0;if(!(in>>tag>>retired)||tag!="RETIRED"||retired>3999-count)return std::nullopt;
        for(size_t n=0;n<retired;++n){NativeRetiredObject o;uint32_t kind=0;if(!(in>>o.id>>kind))return std::nullopt;o.kind=NativeObjectKind(kind);s.retired.push_back(o);}}
    in>>std::ws;if(!in.eof())return std::nullopt;error=validateNativeSession(seed,s);if(!error.empty())return std::nullopt;return s;
}
} // namespace game_lab
