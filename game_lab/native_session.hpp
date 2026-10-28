// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <charconv>
#include <type_traits>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "original_building_update.hpp"
#include "native_city_context.hpp"

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
// v7 is an explicit, non-default persistent house foundation. It carries only
// caller-supplied runtime state; it is not an original-simulation milestone and
// does not enable automatic house evolution.
inline constexpr const char* nativeHouseStateRules = "original-data-native-v7-foundation";
// v8 is a separate development runtime. It never relabels v7 files, and v8
// records carry state that v7 deliberately leaves unknown.
inline constexpr const char* nativeHouseRuntimeRules = "original-data-native-v8-house-runtime-dev";
inline constexpr uint32_t nativeLatestRulesVersion=6;
inline constexpr uint32_t nativeDevelopmentRulesVersion=8;
inline constexpr uint32_t nativeTaxPeriod=4,nativeTaxPerResident=1; // authored gold per connected resident, not original taxation
inline constexpr uint32_t nativeFoodYield=4,nativeFoodCapacity=100; // meat units, independently authored
inline constexpr int nativeWaterRadius=4; // authored v2 coverage, not original walkers/capacity
inline constexpr size_t nativeSessionLegacyByteLimit=512000;
inline constexpr size_t nativeSessionFoundationByteLimit=2u*1024u*1024u;
// v8 carries complete helper rows plus shared route/service grids. The exact
// worst-case admission is checked at encode/decode; no writer truncates and
// the prior v7 2 MiB envelope is untouched.
inline constexpr size_t nativeSessionRuntimeByteLimit=64u*1024u*1024u;
inline size_t nativeSessionByteLimit(uint32_t version){return version>=8?nativeSessionRuntimeByteLimit:version>=7?nativeSessionFoundationByteLimit:nativeSessionLegacyByteLimit;}
inline const char* nativeRulesName(uint32_t version){return version==1?nativeSessionRules:version==2?nativeWaterRules:version==3?nativeFoodRules:version==4?nativeDemolitionRules:version==5?nativeTaxRules:version==6?nativeLaborRules:version==7?nativeHouseStateRules:version==8?nativeHouseRuntimeRules:nullptr;}
// v8 source clock, read from the SAV before any native tick. It is kept
// separate from the v7 opaque clock so v7 bytes and semantics never move.
struct NativeHouseClockSeed {uint32_t phase=0,cycle=0,monthPart=0,month=0;bool cycleWrapTailSkipped=false;};
enum class NativeHouseClockOrigin : uint8_t {SourceSaved=1,NativeConstructedZeroInit=2};
inline const char* nativeHouseClockOriginName(NativeHouseClockOrigin o){
    return o==NativeHouseClockOrigin::SourceSaved?"source-sav":o==NativeHouseClockOrigin::NativeConstructedZeroInit?"native-constructed-zero-init":nullptr;
}
inline std::optional<NativeHouseClockOrigin> nativeHouseClockOriginFromName(const std::string& name){
    if(name=="source-sav")return NativeHouseClockOrigin::SourceSaved;
    if(name=="native-constructed-zero-init")return NativeHouseClockOrigin::NativeConstructedZeroInit;
    return std::nullopt;
}
struct NativeHouseSchedule {
    uint8_t phase=0;
    uint32_t cycle=0,monthPart=0,month=0;
    NativeHouseClockOrigin origin=NativeHouseClockOrigin::SourceSaved;
    bool cycleWrapTailSkipped=false;
};
struct NativeSessionSeed {
    std::string source;
    uint64_t sourceRevision=0, modelRevision=0;
    uint32_t side=0, roadCost=0, houseCost=0, houseCapacity=0, wellCost=0, hunterCost=0;
    int64_t initialFunds=0;
    std::vector<uint8_t> buildable, sourceRoad;
    // v6 inputs from the source SAV and original model employment column5.
    // Zero means unavailable: v6 is then refused, earlier versions unaffected.
    uint32_t laborPercent=0, hunterWorkers=0, wellWorkers=0;
    uint32_t inspectorCost=0,inspectorWorkers=0; // only the explicit inspection policy consumes these model inputs
    std::array<int32_t,10> laborPriority{};
    // v7 only: the single original house-model capacity table. Missing rows are
    // refused rather than mapped onto row0. Legacy versions ignore this field.
    std::map<uint16_t,uint32_t> houseCapacities;
    // v8 immutable source/model inputs. sourceClock is the saved phase/cycle/
    // month state; its absence is retained as unknown. waterOverride is the
    // restored festival predicate, never a guessed false. houseModels is the
    // complete 24-column strict row table; no row0 fallback is permitted.
    std::optional<NativeHouseClockSeed> sourceClock;
    std::optional<bool> waterOverride;
    std::map<int,std::array<int32_t,24>> houseModels;
};
enum class NativeObjectKind : uint32_t { Road=22, House=2, Well=72, Hunter=33, Inspector=124 };
inline constexpr size_t nativeHouseTailBytes=61;
// Persisted v7 house state. houseWater is the caller-known phase28 value and is
// never recomputed from the decaying 61-byte service tail. servicesKnown only
// describes that tail. No field here is produced by a native Tick.
struct NativeHouseRuntime {
    uint16_t type=2, model=0;
    std::array<uint8_t,nativeHouseTailBytes> remaining{};
    bool servicesKnown=false;
    std::optional<uint8_t> houseWater;
    int16_t imageWord20=0;
    uint8_t imageHouseByte3c=0;
};
// Explicitly caller-supplied original clock fields; absent means unknown.
struct NativeHouseClock { uint8_t phase=0; uint32_t monthPart=0; };
// v8 carries lossless helper payloads for complete OriginalFigure rows plus the
// shared strict services those rows read. It never reconstructs a process-only
// default and never uses a cursor-only substitute.
struct NativeFigureRuntimeState {
    std::vector<std::string> wellWalkerPayloads;
    std::vector<std::string> immigrantPayloads; // RUNTIME8 5 only: type11, separate from type13
    std::vector<std::string> migrantPayloads; // explicit observed-dry policy only; v1 runtime bytes stay unchanged
    std::vector<uint16_t> cellHeads;
    std::vector<uint16_t> freeFigureSlots;
    std::optional<size_t> highestFigureSlot;
    struct SavedServices {
        std::array<std::pair<int16_t,int16_t>,100> formationImpacts{};
        std::array<int16_t,1000> routeOwners{};
        std::vector<uint8_t> directions;
        uint32_t randomFirst=0,randomSecond=0;
        std::optional<std::array<uint32_t,100>> randomRing;
        std::optional<uint32_t> randomCursor,randomPrevious;
        std::optional<int32_t> fireCount,criminalCount;
        std::optional<uint32_t> allocationSerial;
        std::vector<uint32_t> serviceGrid;
    } services;
    std::array<int32_t,359> speechCounters{};
    std::array<uint32_t,359> speechCounts{};
    std::map<size_t,int16_t> speechChoices;
};
struct NativeWellRuntime {int16_t primaryFigure=0;uint8_t spawnCounter=0,roamDirection=0;};
// v8-only per-house cache. cachedWater is independent of the decaying tail.
// held/holdReason make an isolated stop visible instead of silently respawning.
struct NativeHouseAutomation {
    std::optional<uint8_t> cachedWater;
    std::optional<uint64_t> wellId;
    uint32_t delay=0,routeCursor=0;
    bool held=false;
    std::string holdReason;
};
struct NativeSessionObject {
    uint64_t id=0; NativeObjectKind kind=NativeObjectKind::Road;
    uint32_t x=0,y=0,residents=0;
    std::optional<NativeHouseRuntime> house = std::nullopt;
    std::optional<NativeHouseAutomation> automation = std::nullopt;
    std::optional<NativeWellRuntime> well = std::nullopt;
};
struct NativeRetiredObject { uint64_t id=0; NativeObjectKind kind=NativeObjectKind::Road; };
struct NativeFoodCycle {uint32_t produced=0,demand=0,fed=0,arrivals=0;};
struct NativeTaxCycle {uint32_t eligible=0,disconnected=0,emptyHouses=0;uint64_t income=0;};
// Opt-in observed dry downgrade; absent preserves every earlier v8 behavior.
// pendingExcess preserves the observed phase39->22->24 overcapacity interval.
struct NativeObservedDryState {
    std::map<uint64_t,uint32_t> pendingExcess;
    uint32_t departedSinceMeal=0,arrivedSinceMeal=0;
};
// Explicit native immigration strategy; old modes keep meal-cycle growth.
// Source labor inputs are preserved, not invented original timing/health.
struct NativePopulationRuntime {
    OriginalBuildingUpdateContext::Labor labor;
    OriginalBuildingCityEmployment employment;
    uint64_t admitted=0,arrived=0,unplaced=0;
    uint64_t populationArrived=0,populationDeparted=0;
};
struct NativeInspectionVisit {
    uint64_t tick=0;size_t figure=0;int16_t collapseBefore=0,ignitionBefore=0,collapseAfter=0,ignitionAfter=0;
};
struct NativeInspectionRuntime {
    std::vector<std::string> payloads;std::array<int32_t,2> maximum{};uint32_t decayCounter=0;
    uint64_t dispatches=0,returns=0,services=0;std::map<size_t,NativeInspectionVisit> visits;
};
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
    // v7: absent is the only default. No Tick invents a phase or month part.
    std::optional<NativeHouseClock> houseClock;
    // v8-only runtime. These fields are rejected by every legacy/v7 decoder
    // and by validation unless the exact v8 wire identity is present.
    std::optional<NativeHouseSchedule> houseSchedule;
    std::optional<NativeFigureRuntimeState> figureRuntime;
    std::optional<int32_t> buildingHookCountdown;
    std::map<uint64_t,std::string> heldHouseReasons;
    std::map<size_t,std::string> heldWalkerReasons;
    // Full mutable strict records/context are plain versioned payloads. The
    // bridge owns their field schema; no process-only cache is authoritative.
    std::map<uint64_t,std::string> strictRecords;
    std::optional<std::string> strictContext;
    std::vector<std::string> runtimePolicies;
    std::optional<NativeObservedDryState> observedDry;
    std::optional<NativePopulationRuntime> populationRuntime;
    // Explicit C1 domain. Old runtime schemas have no payload and never gain
    // scheduled staffing or normalized goods merely by loading an old save.
    std::optional<std::string> cityContext;
    std::optional<NativeInspectionRuntime> inspection;
};
inline constexpr const char* nativeInspectionTag="native_inspection_prevention_v1";
inline bool nativeUsesInspection(const NativeSessionState& s){return s.rulesVersion==8&&std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeInspectionTag)!=s.runtimePolicies.end();}
inline constexpr const char* nativeCityContextTag="native_city_context_contract_v1";
inline bool nativeUsesCityContext(const NativeSessionState& s){
    return s.rulesVersion==8&&std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeCityContextTag)!=s.runtimePolicies.end();
}
// Construction/deletion changes only the set of native-owned job keys. Actual
// workers remain the saved phase25 result; creating a building never allocates
// people in a query, render, codec or bridge rebuild.
inline std::string nativeRefreshCityWorkerKeys(NativeSessionState& s){
    if(!nativeUsesCityContext(s))return {};
    if(!s.cityContext)return "city_context_missing";
    std::string error;auto c=decodeNativeCityContext(*s.cityContext,error);if(!c)return error;
    std::map<size_t,int16_t> jobs;
    for(const auto& o:s.objects)if(o.kind==NativeObjectKind::Well||o.kind==NativeObjectKind::Hunter||o.kind==NativeObjectKind::Inspector){
        const auto old=c->workers.find(size_t(o.id));jobs.emplace(size_t(o.id),old==c->workers.end()?0:old->second);
    }
    c->workers=std::move(jobs);const auto payload=encodeNativeCityContext(*c,error);if(!payload)return error;
    s.cityContext=*payload;return {};
}
inline constexpr const char* nativeImmigrationTag="native_population_immigrant11_policy_v1";
inline bool nativeUsesImmigration(const NativeSessionState& s){
    return s.rulesVersion==8&&std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeImmigrationTag)!=s.runtimePolicies.end();
}
inline constexpr const char* nativeObservedRewaterTag="native_house_water_only_recovery_policy_v1";
inline bool nativeUsesObservedRewatering(const NativeSessionState& s){
    return s.rulesVersion==8&&std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeObservedRewaterTag)!=s.runtimePolicies.end();
}
inline constexpr const char* nativeMigrantNonemptyTag="native_migrant_nonempty_rehousing_policy_v1";
inline bool nativeUsesNonemptyMigrantRehousing(const NativeSessionState& s){
    return s.rulesVersion==8&&std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeMigrantNonemptyTag)!=s.runtimePolicies.end();
}
// Pure wire identity selection shared with capability reporting. Layout and
// valid legacy bytes do not change; framing validation stays in the codec.
inline unsigned nativeHouseRuntimeWireVersion(const NativeSessionState& s){
    return nativeUsesInspection(s)?7:nativeUsesCityContext(s)?6:nativeUsesImmigration(s)?5:nativeUsesObservedRewatering(s)?4:nativeUsesNonemptyMigrantRehousing(s)?3:s.observedDry?2:1;
}
#include "native_session_runtime_wire.inc"
inline constexpr const char* nativeObservedDryTag="native_house_observed_dry_type4_to3_v1";
inline constexpr const char* nativeWellReturnHoldTag="native_well_return_policy_hold_v1";
inline constexpr const char* nativeWellReturnObservedTag="native_well_return_policy_observed_home_road_v1";
inline constexpr const char* nativeWellReturnTransitionTag="native_well_return_observed_home_road_transition_v1";
inline bool nativeUsesObservedWellReturn(const NativeSessionState& s){
    return s.rulesVersion==8&&std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeWellReturnObservedTag)!=s.runtimePolicies.end();
}
enum class NativeSessionAction { Road,House,Tick,Pause,Resume,RoadPath,Well,Hunter,HuntingPause,HuntingResume,Demolish,Inspector };
struct NativeSessionCommand { NativeSessionAction action; uint64_t expectedRevision; int64_t x=0,y=0,toX=0,toY=0; uint64_t objectId=0; };
struct NativeSessionResult { bool committed=false; std::string error; };
// v7 grade/model binding. Type4 and every other unlisted type are explicitly
// unsupported; they are never downgraded to type3 or given a fabricated image.
inline std::optional<uint16_t> nativeHouseModelForType(uint16_t type,uint32_t rulesVersion=7){
    switch(type){case 2:case 3:return 0;case 4:return rulesVersion>=8?std::optional<uint16_t>{1}:std::nullopt;
    case 5:return 2;case 7:return 4;default:return std::nullopt;}
}
// One capacity source for validation, meals, feedback, details and demolition.
// v1-v6 keep the historical single seed.houseCapacity result.
inline std::optional<uint32_t> nativeHouseCapacity(const NativeSessionSeed& seed,const NativeSessionState& state,const NativeSessionObject& object){
    if(object.kind!=NativeObjectKind::House)return std::nullopt;
    if(state.rulesVersion<7)return seed.houseCapacity?std::optional<uint32_t>{seed.houseCapacity}:std::nullopt;
    if(!object.house)return std::nullopt;
    const auto expected=nativeHouseModelForType(object.house->type,state.rulesVersion);
    if(!expected||*expected!=object.house->model)return std::nullopt;
    const auto row=seed.houseCapacities.find(object.house->model);
    if(row==seed.houseCapacities.end()||row->second==0||row->second>100)return std::nullopt;
    return row->second;
}
inline std::optional<uint16_t> nativeHouseProjectionType(const NativeSessionState& state,const NativeSessionObject& object){
    if(object.kind!=NativeObjectKind::House)return std::nullopt;
    if(state.rulesVersion<7)return uint16_t(object.residents?3:2);
    if(!object.house)return std::nullopt;
    const auto expected=nativeHouseModelForType(object.house->type,state.rulesVersion);
    if(!expected||*expected!=object.house->model)return std::nullopt;
    return object.house->type;
}
// Retired houses have no runtime grade, so the historical departure bound uses
// the largest admitted per-house capacity. Legacy versions keep their old bound.
inline uint32_t nativeHouseCapacityCeiling(const NativeSessionSeed& seed,const NativeSessionState& state){
    if(state.rulesVersion<7)return seed.houseCapacity;
    uint32_t maximum=0;
    // Only models bound to an admitted grade can have housed residents; an
    // unsupported extra table row must not enlarge the historical bound.
    for(uint16_t model:{uint16_t(0),uint16_t(1),uint16_t(2),uint16_t(4)}){
        if(model==1&&state.rulesVersion<8)continue;
        const auto row=seed.houseCapacities.find(model);
        if(row!=seed.houseCapacities.end()&&row->second>0&&row->second<=100&&row->second>maximum)maximum=row->second;
    }
    return maximum;
}
// Always blocked: no state turns on automatic evolution. The text names what is
// still unconnected instead of pretending a native Tick is the original cycle.
inline std::string nativeHouseAutomaticEvolutionBlocker(const NativeSessionState& state){
    if(state.rulesVersion==8){
        std::string out="native_v8_house_phase_runtime";
        if(state.houseSchedule)out+=": phase "+std::to_string(state.houseSchedule->phase);
        for(const auto& h:state.heldHouseReasons)out+=": house "+std::to_string(h.first)+" "+h.second;
        for(const auto& w:state.heldWalkerReasons)out+=": walker "+std::to_string(w.first)+" "+w.second;
        return out;
    }
    std::string out="automatic_house_evolution_disabled";
    if(state.rulesVersion<7){out+=": rules v"+std::to_string(state.rulesVersion)+" carry no original house runtime";}
    else {
        if(!state.houseClock)out+=": original house clock unknown";
        bool services=false,water=false;
        for(const auto& o:state.objects)if(o.kind==NativeObjectKind::House&&o.house){
            services=services||!o.house->servicesKnown;water=water||!o.house->houseWater;}
        if(services)out+=": house service tail unknown";
        if(water)out+=": phase28 water unknown";
    }
    out+=": original walker return and scheduling unconnected";
    return out;
}
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
inline uint32_t nativeObjectCost(const NativeSessionSeed& seed,NativeObjectKind kind){return kind==NativeObjectKind::Road?seed.roadCost:kind==NativeObjectKind::Well?seed.wellCost:kind==NativeObjectKind::Hunter?seed.hunterCost:kind==NativeObjectKind::Inspector?seed.inspectorCost:kind==NativeObjectKind::House?seed.houseCost:0;}
inline std::string validateNativeSession(const NativeSessionSeed& seed,const NativeSessionState& s) {
    if(!validNativeSeed(seed))return "unsupported_source_or_model";
    if(!nativeRulesName(s.rulesVersion))return "unsupported_native_rules";
    const bool houseState=s.rulesVersion>=7;
    if(!houseState){
        if(s.houseClock||std::any_of(s.objects.begin(),s.objects.end(),[](const auto& o){return o.house.has_value()||o.automation.has_value()||o.well.has_value();})||
           s.houseSchedule||s.figureRuntime||s.buildingHookCountdown||!s.heldHouseReasons.empty()||!s.heldWalkerReasons.empty()||!s.strictRecords.empty()||s.strictContext||!s.runtimePolicies.empty()||s.observedDry||s.populationRuntime||s.cityContext||s.inspection)
            return "house_state_requires_native_v7";
    }else if(s.houseClock&&s.houseClock->phase>=51)return "invalid_house_clock";
    if(s.rulesVersion<8){
        if(s.houseSchedule||s.figureRuntime||s.buildingHookCountdown||!s.heldHouseReasons.empty()||!s.heldWalkerReasons.empty()||!s.strictRecords.empty()||s.strictContext||!s.runtimePolicies.empty()||s.observedDry||s.populationRuntime||s.cityContext||s.inspection||
           std::any_of(s.objects.begin(),s.objects.end(),[](const auto& o){return o.automation.has_value()||o.well.has_value();}))
            return "house_runtime_requires_native_v8";
    }else{
        if(nativeUsesInspection(s)!=s.inspection.has_value()||(s.inspection&&!nativeUsesCityContext(s)))return "inspection_policy_state_mismatch";
        for(const auto& p:s.runtimePolicies)if(p.rfind("native_inspection_",0)==0&&p!=nativeInspectionTag)return "inspection_policy_unsupported";
        if(s.inspection){const auto& p=*s.inspection;
            if(!seed.inspectorCost||seed.inspectorCost>100000||!seed.inspectorWorkers||seed.inspectorWorkers>1000)return "inspection_model_inputs_missing";
            if(p.decayCounter>7||p.payloads.size()>1999||p.visits.size()>3999||p.dispatches>1000000000000ULL||p.returns>p.dispatches||p.services>1000000000000ULL||std::any_of(p.maximum.begin(),p.maximum.end(),[](int32_t n){return n<0||n>32767;}))return "inspection_state_range";
            for(const auto& bytes:p.payloads)if(bytes.empty()||bytes.size()>16384||bytes.find('\0')!=std::string::npos)return "inspection_payload_framing";
            for(const auto& [slot,v]:p.visits)if(!slot||slot>=4000||!v.figure||v.figure>=2000||v.tick>s.ticks||v.collapseAfter>v.collapseBefore||v.ignitionAfter>v.ignitionBefore)return "inspection_visit_invalid";
        }
        if(s.strictRecords.size()>3999||s.heldHouseReasons.size()>3999||s.heldWalkerReasons.size()>1999||s.runtimePolicies.size()>64)return "invalid_runtime_record_count";
        for(const auto& entry:s.strictRecords){
            if(entry.second.empty()||entry.second.size()>65536||entry.second.find('\0')!=std::string::npos)return "invalid_strict_record";
            if(!std::any_of(s.objects.begin(),s.objects.end(),[&](const auto& o){return o.id==entry.first&&o.kind!=NativeObjectKind::Road;}))return "unowned_strict_record";
        }
        if(s.strictContext&&(s.strictContext->size()>262144||s.strictContext->find('\0')!=std::string::npos))return "invalid_strict_context";
        for(const auto& p:s.runtimePolicies)if(p.empty()||p.size()>256||p.find('\0')!=std::string::npos)return "invalid_runtime_policy";
        for(const auto& e:s.heldWalkerReasons)if(!e.first||e.first>=2000||e.second.empty()||e.second.size()>1024||e.second.find('\0')!=std::string::npos)return "invalid_held_walker";
        for(const auto& e:s.heldHouseReasons)if(e.second.empty()||e.second.size()>1024||e.second.find('\0')!=std::string::npos||!std::any_of(s.objects.begin(),s.objects.end(),[&](const auto& o){return o.id==e.first&&o.kind==NativeObjectKind::House;}))return "invalid_held_house";
        const bool taggedDry=std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeObservedDryTag)!=s.runtimePolicies.end();
        if(nativeUsesImmigration(s)!=s.populationRuntime.has_value())return "immigration_policy_state_mismatch";
        if(nativeUsesCityContext(s)!=s.cityContext.has_value()||(s.cityContext&&!nativeUsesImmigration(s)))return "city_context_policy_state_mismatch";
        if(nativeUsesCityContext(s)&&std::count(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeCityContextTag)!=1)return "city_context_policy_duplicate";
        for(const auto& p:s.runtimePolicies)if(p.rfind("native_city_context_contract_",0)==0&&p!=nativeCityContextTag)return "city_context_policy_unsupported";
        if(s.cityContext){std::string why;const auto context=decodeNativeCityContext(*s.cityContext,why);if(!context)return "invalid_city_context:"+why;
            if(context->goods&&s.houseSchedule&&context->goods->savedPhase!=s.houseSchedule->phase)return "city_goods_phase_mirror_mismatch";
            size_t jobs=0;for(const auto& o:s.objects)if(o.kind==NativeObjectKind::Well||o.kind==NativeObjectKind::Hunter||o.kind==NativeObjectKind::Inspector){
                ++jobs;const auto worker=context->workers.find(size_t(o.id));
                const uint32_t required=o.kind==NativeObjectKind::Well?seed.wellWorkers:o.kind==NativeObjectKind::Inspector?seed.inspectorWorkers:seed.hunterWorkers;
                if(worker==context->workers.end()||worker->second<0||uint32_t(worker->second)>required)return "invalid_city_worker_projection";
            }
            if(context->workers.size()!=jobs)return "unowned_city_worker_projection";
        }
        if(s.populationRuntime){const auto& p=*s.populationRuntime;
            constexpr uint64_t limit=399900000000000ULL;
            if(p.admitted>limit||p.arrived>limit||p.unplaced>limit||p.populationArrived>limit||p.populationDeparted>limit||p.labor.spacing<0||p.labor.spacing>10000000||p.labor.inDelay<0||p.labor.outDelay<0)return "invalid_immigration_counter_range";
            if(!nativeUsesObservedRewatering(s)||!s.observedDry)return "immigration_requires_r4_runtime";
            uint64_t total=0;for(const auto& o:s.objects)total+=o.residents;
            if(p.populationDeparted>p.populationArrived||p.populationArrived-p.populationDeparted!=total||p.arrived>p.admitted||p.unplaced>p.admitted-p.arrived)return "invalid_immigration_ledger";
        }
        for(const auto& p:s.runtimePolicies)if(p.rfind("native_population_immigrant11_policy_",0)==0&&p!=nativeImmigrationTag)return "immigration_policy_unsupported";
        if(taggedDry!=s.observedDry.has_value())return "observed_dry_policy_state_mismatch";
        if(nativeUsesObservedRewatering(s)&&(!s.observedDry||!nativeUsesNonemptyMigrantRehousing(s)))return "water_recovery_requires_r3_runtime";
        for(const auto& p:s.runtimePolicies)if(p.rfind("native_house_water_only_recovery_policy_",0)==0&&p!=nativeObservedRewaterTag)return "water_recovery_policy_unsupported";
        if(nativeUsesNonemptyMigrantRehousing(s)&&!s.observedDry)return "nonempty_rehousing_requires_observed_dry_state";
        for(const auto& p:s.runtimePolicies)if(p.rfind("native_migrant_nonempty_rehousing_policy_",0)==0&&p!=nativeMigrantNonemptyTag)return "nonempty_rehousing_policy_unsupported";
        for(const auto& p:s.runtimePolicies)if(p.rfind("native_house_observed_dry_",0)==0&&p!=nativeObservedDryTag)return "observed_dry_policy_unsupported";
        if(s.observedDry&&(s.observedDry->pendingExcess.size()>3999||s.observedDry->departedSinceMeal>399900||s.observedDry->arrivedSinceMeal>399900))return "invalid_observed_dry_ledger";
        if(s.houseClock)return "v8_forbids_v7_house_clock";
        if(!s.houseSchedule)return "missing_house_schedule";
        const auto& sc=*s.houseSchedule;
        if(sc.phase>50||sc.monthPart>15||sc.month>11||!nativeHouseClockOriginName(sc.origin))return "invalid_house_schedule";
        if(!s.figureRuntime)return "missing_figure_runtime";
        if(seed.houseModels.empty())return "missing_house_model_rows";
        for(const auto& [model,row]:seed.houseModels){if(model<0||model>14)return "invalid_house_model_rows";(void)row;}
        const auto& fr=*s.figureRuntime;
        if(fr.wellWalkerPayloads.size()+fr.migrantPayloads.size()+fr.immigrantPayloads.size()+(s.inspection?s.inspection->payloads.size():0)>1999)return "invalid_walker_payload_count";
        if(!s.populationRuntime&&!fr.immigrantPayloads.empty())return "immigrant_requires_population_policy";
        for(const auto& payload:fr.immigrantPayloads)if(payload.empty()||payload.size()>16384||payload.find('\0')!=std::string::npos)return "invalid_immigrant_payload";
        for(const auto& payload:fr.migrantPayloads)if(payload.empty()||payload.size()>16384||payload.find('\0')!=std::string::npos)return "invalid_migrant_payload";
        if(!s.observedDry&&!fr.migrantPayloads.empty())return "migrant_requires_observed_dry_policy";
        for(const auto& payload:fr.wellWalkerPayloads)if(payload.empty()||payload.size()>16384||payload.find('\0')!=std::string::npos)return "invalid_walker_payload";
        if(fr.cellHeads.size()!=51984)return "invalid_figure_cell_heads";
        if(fr.highestFigureSlot&&*fr.highestFigureSlot>=2000)return "invalid_highest_figure_slot";
        std::set<uint16_t> freeSlots;
        for(uint16_t slot:fr.freeFigureSlots)if(!slot||slot>=2000||!freeSlots.insert(slot).second)return "invalid_free_figure_slots";
        const auto& sv=fr.services;
        if(sv.directions.size()!=500000)return "invalid_route_directions";
        if(sv.serviceGrid.size()!=51984)return "invalid_service_grid";
        if(sv.randomRing){
            if(!sv.randomCursor||!sv.randomPrevious||*sv.randomCursor>99||*sv.randomPrevious>127)return "invalid_figure_random_history";
            if(std::any_of(sv.randomRing->begin(),sv.randomRing->end(),[](uint32_t v){return v>127;}))return "invalid_figure_random_history";
        }else if(sv.randomCursor||sv.randomPrevious)return "invalid_figure_random_history";
        if(s.buildingHookCountdown&&*s.buildingHookCountdown<0)return "invalid_building_hook";
    }
    if(s.rulesVersion>=2&&(!seed.wellCost||seed.wellCost>100000))return "missing_well_model_price";
    if(s.rulesVersion>=3&&(!seed.hunterCost||seed.hunterCost>100000))return "missing_hunter_model_price";
    if(s.rulesVersion>=6&&(!seed.laborPercent||seed.laborPercent>100||!seed.hunterWorkers||seed.hunterWorkers>1000||seed.wellWorkers>1000||
        std::any_of(seed.laborPriority.begin(),seed.laborPriority.end(),[](int32_t p){return p<0||p>5;})))return "missing_labor_model";
    if(s.objects.size()+s.retired.size()>3999||s.nextId!=s.objects.size()+s.retired.size()+1||s.revision<s.ticks||s.revision>1000000000000ULL||s.ticks>1000000000ULL)return "invalid_session_counters";
    if(s.rulesVersion<4&&(!s.retired.empty()||s.departedSinceMeal))return "demolition_requires_native_v4";
    const auto validKind=[&](NativeObjectKind k){return k==NativeObjectKind::Road||k==NativeObjectKind::House||(s.rulesVersion>=2&&k==NativeObjectKind::Well)||(s.rulesVersion>=3&&k==NativeObjectKind::Hunter)||(nativeUsesInspection(s)&&k==NativeObjectKind::Inspector);};
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
        if(o.kind==NativeObjectKind::House){
            if(houseState&&!o.house)return "missing_house_runtime";
            if(houseState){
                const auto expected=nativeHouseModelForType(o.house->type,s.rulesVersion);
                if(!expected||*expected!=o.house->model)return "unsupported_house_type_or_model";
                if(o.house->houseWater&&*o.house->houseWater>2)return "invalid_house_water";
            }
            if(o.well)return "unexpected_well_runtime";
            if(s.rulesVersion>=8){
                if(!o.automation)return "missing_house_automation";
                const auto& a=*o.automation;
                if(a.cachedWater.has_value()!=o.house->houseWater.has_value()||
                   (a.cachedWater&&*a.cachedWater!=*o.house->houseWater))return "house_cached_water_mismatch";
                if(a.cachedWater&&*a.cachedWater>2)return "invalid_house_cached_water";
                if(a.holdReason.size()>1024||a.holdReason.find('\0')!=std::string::npos)return "invalid_house_hold_reason";
                if(a.held&&a.holdReason.empty())return "missing_house_hold_reason";
                if(a.wellId&&!std::any_of(s.objects.begin(),s.objects.end(),[&](const auto& w){return w.id==*a.wellId&&w.kind==NativeObjectKind::Well;}))return "invalid_house_well_reference";
            }else if(o.automation)return "unexpected_house_automation";
            const auto capacity=nativeHouseCapacity(seed,s,o);
            if(!capacity)return "invalid_house_capacity";
            if(o.residents>*capacity){
                if(!s.observedDry||!o.house||o.house->type!=3||o.house->model!=0||!s.observedDry->pendingExcess.count(o.id)||
                   s.observedDry->pendingExcess.at(o.id)!=o.residents-*capacity||!seed.houseCapacities.count(1)||o.residents>seed.houseCapacities.at(1))return "invalid_residents";
            }
            if(!s.populationRuntime&&o.residents>s.ticks/2)return "invalid_residents";
        }else{
            if(o.house)return "unexpected_house_runtime";
            if(o.automation)return "unexpected_house_automation";
            if(s.rulesVersion>=8&&o.kind==NativeObjectKind::Well&&!o.well)return "missing_well_runtime";
            if((s.rulesVersion<8||o.kind!=NativeObjectKind::Well)&&o.well)return "unexpected_well_runtime";
            if(o.residents||o.residents>s.ticks/2)return "invalid_residents";
        }
        for(unsigned y=o.y;y<o.y+n;++y)for(unsigned x=o.x;x<o.x+n;++x){const size_t at=size_t(y)*seed.side+x;
            if(!seed.buildable[at]||occupied[at])return "occupied_or_unsupported_terrain";occupied[at]=1;}
        spent+=nativeObjectCost(seed,o.kind);
    }
    if(s.observedDry)for(const auto& entry:s.observedDry->pendingExcess){
        const uint64_t id=entry.first;const uint32_t excess=entry.second;
        const auto found=std::find_if(s.objects.begin(),s.objects.end(),[&](const auto& o){return o.id==id;});
        if(found==s.objects.end()||found->kind!=NativeObjectKind::House||!found->house||found->house->type!=3||found->house->model!=0||!excess)return "invalid_pending_excess";
        const auto cap=nativeHouseCapacity(seed,s,*found);
        if(!cap||found->residents<=*cap||found->residents-*cap!=excess)return "invalid_pending_excess";
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
            (paid&&meal.fed<meal.demand&&meal.arrivals)||meal.produced>s.foodProduced||charged>s.foodConsumed||uint64_t(meal.demand)+meal.arrivals+(s.observedDry?s.observedDry->arrivedSinceMeal:0)!=population+s.departedSinceMeal+(s.observedDry?s.observedDry->departedSinceMeal:0)||s.departedSinceMeal>retiredHouses*nativeHouseCapacityCeiling(seed,s)||
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
        if(o.kind==NativeObjectKind::House){result.connectedHouses+=connected;result.wateredHouses+=state.rulesVersion==8&&o.house?o.house->houseWater.value_or(0)!=0:nativeHouseWatered(seed,o,water);}}
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
    if(next.populationRuntime)next.populationRuntime->populationDeparted+=found->residents;
    next.departedSinceMeal+=found->residents;next.objects.erase(next.objects.begin()+std::distance(state.objects.begin(),found));
    next.strictRecords.erase(id);next.heldHouseReasons.erase(id);if(next.observedDry)next.observedDry->pendingExcess.erase(id);
    plan.error=nativeRefreshCityWorkerKeys(next);if(!plan.error.empty())return plan;
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
    if(nativeUsesCityContext(state)){
        std::string error;const auto saved=decodeNativeCityContext(*state.cityContext,error);
        if(!saved||!state.populationRuntime){plan.error="city_staffing_state_missing:"+error;return plan;}
        plan.available=state.populationRuntime->employment.available;
        for(const auto& o:state.objects)if(o.kind==NativeObjectKind::Hunter||o.kind==NativeObjectKind::Well||o.kind==NativeObjectKind::Inspector){
            const auto w=saved->workers.find(size_t(o.id));if(w==saved->workers.end()){plan.error="city_staffing_job_missing";return plan;}
            NativeLaborJob job{o.id,o.kind,false,o.kind==NativeObjectKind::Hunter?seed.hunterWorkers:o.kind==NativeObjectKind::Inspector?seed.inspectorWorkers:seed.wellWorkers,uint32_t(w->second)};
            job.eligible=nativeHouseConnected(seed,o,roads)&&(o.kind!=NativeObjectKind::Hunter||state.huntingEnabled);
            if(job.eligible)plan.demand+=int32_t(job.required);
            plan.employed+=int32_t(job.workers);plan.jobs.push_back(job);
        }
        plan.unemployed=std::max(0,plan.available-plan.employed);plan.shortage=std::max(0,plan.demand-plan.employed);
        return plan;
    }
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
    if(!nativeUsesImmigration(state)&&(plan.cycle.fed==plan.cycle.demand||plan.laborOverride))for(size_t n=0;n<state.objects.size()&&(labor||plan.stock);++n){const auto& o=state.objects[n];
        if(o.kind==NativeObjectKind::House){
            const auto capacity=nativeHouseCapacity(seed,state,o);
            if(!capacity){plan.error="invalid_house_capacity";return plan;}
            if(o.residents<*capacity&&nativeHouseConnected(seed,o,roads)&&nativeHouseWatered(seed,o,water)){
                if(!labor)--plan.stock;plan.arrivals.push_back(n);++plan.cycle.arrivals;}
        }
    }
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
// Existing native economy, explicitly retained as a native policy in v8.
// The caller owns revision and whole-runtime publication.
inline std::string advanceNativeSessionLedger(const NativeSessionSeed& seed,const NativeSessionState& before,NativeSessionState& next){
        // Tax assesses pre-tick residents. Receipt, funds, meal and population
        // are one candidate; any later validation/drawing failure publishes none.
        if(next.rulesVersion>=5&&(next.ticks+1)%nativeTaxPeriod==0){
            const auto plan=planNativeTaxCycle(seed,before);if(!plan.error.empty())return plan.error;
            ++next.taxCycles;next.taxReceived+=plan.cycle.income;next.funds+=int64_t(plan.cycle.income);next.lastTax=plan.cycle;
        }
        ++next.ticks;if(next.ticks%2==0){
            if(next.rulesVersion>=3){const auto plan=planNativeFoodCycle(seed,next);if(!plan.error.empty())return plan.error;
                next.foodStock=plan.stock;next.foodProduced+=plan.cycle.produced;next.foodConsumed+=uint64_t(plan.cycle.fed)+(next.rulesVersion<6?plan.cycle.arrivals:0);next.lastFood=plan.cycle;next.departedSinceMeal=0;if(next.observedDry){next.observedDry->departedSinceMeal=0;next.observedDry->arrivedSinceMeal=0;}
                for(const auto n:plan.arrivals){auto& house=next.objects[n];++house.residents;
                    // Existing native first-arrival visual only: never grade3->4.
                    if(next.rulesVersion>=7&&house.house&&house.house->type==2)house.house->type=3;}
            }else{const auto roads=nativeRoads(seed,next);const auto water=nativeWaterCoverage(seed,next);
                for(auto& o:next.objects)if(o.kind==NativeObjectKind::House&&o.residents<seed.houseCapacity&&nativeHouseConnected(seed,o,roads)&&(next.rulesVersion==1||nativeHouseWatered(seed,o,water)))++o.residents;}}
    return {};
}
inline NativeSessionResult transactNativeSession(const NativeSessionSeed& seed,NativeSessionState& state,const NativeSessionCommand& cmd) {
    const auto invalid=validateNativeSession(seed,state);if(!invalid.empty())return {false,invalid};
    if(cmd.expectedRevision!=state.revision)return {false,"stale_command"};
    if(state.rulesVersion>=8&&cmd.action==NativeSessionAction::Tick)return {false,"runtime_bridge_required"};
    if(state.revision==UINT64_MAX)return {false,"revision_exhausted"};
    auto next=state;
    switch(cmd.action){
    case NativeSessionAction::Road:case NativeSessionAction::House:case NativeSessionAction::Well:case NativeSessionAction::Hunter:case NativeSessionAction::Inspector:{
        if(cmd.action==NativeSessionAction::Inspector&&!nativeUsesInspection(state))return {false,"inspection_policy_required"};
        if(cmd.action==NativeSessionAction::Well&&state.rulesVersion<2)return {false,"well_requires_native_v2"};
        if(cmd.action==NativeSessionAction::Hunter&&state.rulesVersion<3)return {false,"hunter_requires_native_v3"};
        if(cmd.x<0||cmd.y<0||cmd.x>=seed.side||cmd.y>=seed.side)return {false,"outside_scene"};
        const auto kind=cmd.action==NativeSessionAction::Road?NativeObjectKind::Road:cmd.action==NativeSessionAction::Well?NativeObjectKind::Well:cmd.action==NativeSessionAction::Hunter?NativeObjectKind::Hunter:cmd.action==NativeSessionAction::Inspector?NativeObjectKind::Inspector:NativeObjectKind::House;
        const auto cost=nativeObjectCost(seed,kind);
        if(next.funds<cost)return {false,"insufficient_funds"};
        if(next.objects.size()+next.retired.size()>=3999)return {false,"object_limit"};
        next.objects.push_back({next.nextId++,kind,uint32_t(cmd.x),uint32_t(cmd.y),0});next.funds-=cost;
        if(next.rulesVersion>=7&&kind==NativeObjectKind::House)next.objects.back().house.emplace();
        if(next.rulesVersion>=8&&kind==NativeObjectKind::House){auto& o=next.objects.back();o.house->servicesKnown=true;o.house->houseWater=0;o.automation.emplace();o.automation->cachedWater=0;}
        if(next.rulesVersion>=8&&kind==NativeObjectKind::Well)next.objects.back().well.emplace();
        break;}
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
        if(const auto e=advanceNativeSessionLedger(seed,state,next);!e.empty())return {false,e};
        break;}
    default:return {false,"unsupported_command"};
    }
    if(auto e=nativeRefreshCityWorkerKeys(next);!e.empty())return {false,e};
    ++next.revision;const auto error=validateNativeSession(seed,next);if(!error.empty())return {false,error};
    state=std::move(next);return {true,{}};
}
inline uint64_t nativePopulation(const NativeSessionState& s){uint64_t n=0;for(const auto& o:s.objects)n+=o.residents;return n;}
inline uint64_t nativeTextRevision(const std::string& bytes){uint64_t h=14695981039346656037ULL;for(unsigned char c:bytes){h^=c;h*=1099511628211ULL;}return h;}
// Inspect only the versioned header; acceptance still requires the full decoder.
inline std::optional<uint32_t> nativeSessionWireVersion(const std::string& bytes){
    if(bytes.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream in(bytes);std::string magic,rules;uint32_t version=0;
    if(!(in>>magic>>version>>rules)||magic!="ENCITY"||!nativeRulesName(version)||rules!=nativeRulesName(version))return std::nullopt;
    return version;
}
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
    if(s.rulesVersion>=7){
        out<<"HOUSEMODELS "<<seed.houseCapacities.size();
        for(const auto& [model,capacity]:seed.houseCapacities)out<<' '<<model<<' '<<capacity;
        out<<'\n';
        if(s.houseClock)out<<"HOUSECLOCK "<<unsigned(s.houseClock->phase)<<' '<<s.houseClock->monthPart<<'\n';
        else out<<"HOUSECLOCK unknown\n";
    }
    for(const auto& o:s.objects)out<<o.id<<' '<<uint32_t(o.kind)<<' '<<o.x<<' '<<o.y<<' '<<o.residents<<'\n';
    if(s.rulesVersion>=4){out<<"RETIRED "<<s.retired.size()<<'\n';for(const auto& o:s.retired)out<<o.id<<' '<<uint32_t(o.kind)<<'\n';}
    if(s.rulesVersion>=7){
        size_t houses=0;for(const auto& o:s.objects)houses+=o.kind==NativeObjectKind::House;
        out<<"HOUSES "<<houses<<'\n';
        static constexpr char digits[]="0123456789abcdef";
        for(const auto& o:s.objects)if(o.kind==NativeObjectKind::House){
            const auto& h=*o.house;out<<o.id<<' '<<h.type<<' '<<h.model<<' '<<(h.servicesKnown?1:0)<<' ';
            if(h.houseWater)out<<unsigned(*h.houseWater);else out<<'-';
            out<<' ';for(uint8_t byte:h.remaining)out<<digits[byte>>4]<<digits[byte&15];
            out<<' '<<h.imageWord20<<' '<<unsigned(h.imageHouseByte3c)<<'\n';
        }
    }
    if(s.rulesVersion==8)native_runtime_wire::encode(out,s);
    const auto bytes=out.str();return bytes.size()<=nativeSessionByteLimit(s.rulesVersion)?bytes:std::string{};
}
// Bounded strict parser: never restores original memory, event bytes or asset
// payloads. The original source/model must still match the externally loaded seed.
inline std::optional<NativeSessionState> decodeNativeSession(const NativeSessionSeed& seed,const std::string& bytes,std::string& error) {
    error="invalid_native_session";if(bytes.size()>nativeSessionRuntimeByteLimit||bytes.find('\0')!=std::string::npos||!validNativeSeed(seed))return std::nullopt;
    std::istringstream in(bytes);std::string magic,rules,source;unsigned version=0;
    uint64_t revision=0,model=0;uint32_t side=0,road=0,house=0,cap=0;int64_t funds=0;
    if(!(in>>magic>>version>>rules)||magic!="ENCITY"||!nativeRulesName(version)||rules!=nativeRulesName(version)){error="unsupported_session_version";return std::nullopt;}
    if(bytes.size()>nativeSessionByteLimit(version))return std::nullopt;
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
    if(version>=7){
        std::string tag;size_t rows=0;
        if(!(in>>tag>>rows)||tag!="HOUSEMODELS"||rows>seed.houseCapacities.size()){error="house_model_record_mismatch";return std::nullopt;}
        std::map<uint16_t,uint32_t> parsed;
        for(size_t n=0;n<rows;++n){
            uint64_t modelKey=0;int64_t capacity=0;
            if(!(in>>modelKey>>capacity)||modelKey>UINT16_MAX||capacity<0||capacity>UINT32_MAX)return std::nullopt;
            if(!parsed.emplace(uint16_t(modelKey),uint32_t(capacity)).second)return std::nullopt;
        }
        if(parsed!=seed.houseCapacities){error="house_model_record_mismatch";return std::nullopt;}
        std::string clockTag;
        if(!(in>>clockTag)||clockTag!="HOUSECLOCK"){error="invalid_house_clock_record";return std::nullopt;}
        std::string phaseToken;if(!(in>>phaseToken)){error="invalid_house_clock_record";return std::nullopt;}
        if(phaseToken!="unknown"){
            uint64_t phase=0,monthPart=0;std::istringstream phaseText(phaseToken);
            if(!(phaseText>>phase)||!phaseText.eof()){error="invalid_house_clock_record";return std::nullopt;}
            if(!(in>>monthPart)){error="invalid_house_clock_record";return std::nullopt;}
            if(phase>50||monthPart>UINT32_MAX)return std::nullopt;
            NativeHouseClock clock;clock.phase=uint8_t(phase);clock.monthPart=uint32_t(monthPart);
            s.houseClock=clock;
        }
    }
    for(size_t n=0;n<count;++n){NativeSessionObject o;uint32_t kind=0;if(!(in>>o.id>>kind>>o.x>>o.y>>o.residents))return std::nullopt;o.kind=NativeObjectKind(kind);s.objects.push_back(o);}
    if(version>=4){std::string tag;size_t retired=0;if(!(in>>tag>>retired)||tag!="RETIRED"||retired>3999-count)return std::nullopt;
        for(size_t n=0;n<retired;++n){NativeRetiredObject o;uint32_t kind=0;if(!(in>>o.id>>kind))return std::nullopt;o.kind=NativeObjectKind(kind);s.retired.push_back(o);}}
    if(version>=7){
        size_t houseObjects=0;for(const auto& o:s.objects)houseObjects+=o.kind==NativeObjectKind::House;
        std::string tag;size_t houses=0;
        if(!(in>>tag>>houses)||tag!="HOUSES"||houses!=houseObjects){error="invalid_house_state_count";return std::nullopt;}
        std::set<uint64_t> seen;size_t nextHouse=0;
        for(size_t n=0;n<houses;++n){
            uint64_t id=0,type=0,model=0,services=0,imageByte=0;int64_t imageWord=0;std::string waterToken,tail;
            if(!(in>>id>>type>>model>>services>>waterToken>>tail>>imageWord>>imageByte))return std::nullopt;
            if(type>UINT16_MAX||model>UINT16_MAX||services>1||imageByte>UINT8_MAX||imageWord<INT16_MIN||imageWord>INT16_MAX||tail.size()!=nativeHouseTailBytes*2)return std::nullopt;
            while(nextHouse<s.objects.size()&&s.objects[nextHouse].kind!=NativeObjectKind::House)++nextHouse;
            if(nextHouse>=s.objects.size()||s.objects[nextHouse].id!=id||!seen.insert(id).second){error="invalid_house_state_identity";return std::nullopt;}
            std::array<uint8_t,nativeHouseTailBytes> remaining{};
            for(size_t at=0;at<remaining.size();++at){
                const auto digit=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
                const int high=digit(tail[at*2]),low=digit(tail[at*2+1]);
                if(high<0||low<0)return std::nullopt;remaining[at]=uint8_t(high*16+low);
            }
            NativeHouseRuntime runtime;runtime.type=uint16_t(type);runtime.model=uint16_t(model);runtime.servicesKnown=services!=0;
            runtime.remaining=remaining;runtime.imageWord20=int16_t(imageWord);runtime.imageHouseByte3c=uint8_t(imageByte);
            if(waterToken=="-")runtime.houseWater=std::nullopt;
            else if(waterToken=="0"||waterToken=="1"||waterToken=="2")runtime.houseWater=uint8_t(waterToken[0]-'0');
            else return std::nullopt;
            s.objects[nextHouse].house=runtime;++nextHouse;
        }
    }
    if(version==8&&!native_runtime_wire::decode(in,s)){error="invalid_runtime_record";return std::nullopt;}
    in>>std::ws;if(!in.eof())return std::nullopt;error=validateNativeSession(seed,s);if(!error.empty())return std::nullopt;return s;
}
} // namespace game_lab
