// SPDX-License-Identifier: MIT
#pragma once
#include "native_session.hpp"
#include "native_population_feedback.hpp"
#include "native_entry_access.hpp"
namespace game_lab {
// Read-only projections of the existing simulation planners. No original-game
// event semantics, future population prediction or independent food allocation.
struct NativeObjectFeedback {
    uint64_t id=0;NativeObjectKind kind=NativeObjectKind::Road;
    uint32_t residents=0;bool connected=false,watered=false,full=false;
    bool foodShortage=false,arrivalPlanned=false,productionEnabled=false;
    uint32_t workers=0,requiredWorkers=0; // v6 staffing; zero before v6
    std::optional<NativeHouseFlow> housing;
    std::optional<NativeWellFlow> wellFlow;
    std::optional<bool> entryReachable;
    std::optional<bool> entryRoadConnected;
    std::string entryAccessError;
    NativeInspectorFeedback inspection;
};
struct NativeCityFeedback {
    uint32_t rulesVersion=0,untilMeal=0,untilTax=0,activeHunters=0;
    uint32_t foodStock=0,productionPotential=0;bool mealBlockedByTax=false;NativeFoodPlan food;NativeTaxPlan tax;NativeLaborPlan labor;
    std::vector<NativeObjectFeedback> objects;std::string error;
    NativePopulationFeedback population;
    NativeEntranceFeedback entrance;
    const NativeObjectFeedback* find(uint64_t id)const{
        const auto i=std::lower_bound(objects.begin(),objects.end(),id,[](const auto&o,uint64_t n){return o.id<n;});
        return i!=objects.end()&&i->id==id?&*i:nullptr;
    }
};
inline NativeCityFeedback queryNativeCityFeedback(const NativeSessionSeed& seed,const NativeSessionState& state,const NativeStrictTemplate* source=nullptr){
    NativeCityFeedback q;q.error=validateNativeSession(seed,state);if(!q.error.empty())return q;
    q.rulesVersion=state.rulesVersion;q.untilMeal=uint32_t(2-state.ticks%2);q.untilTax=uint32_t(nativeTaxPeriod-state.ticks%nativeTaxPeriod);q.foodStock=state.foodStock;
    if(state.rulesVersion>=3)q.food=planNativeFoodCycle(seed,state);
    if(state.rulesVersion>=5){q.tax=planNativeTaxCycle(seed,state);q.mealBlockedByTax=!q.tax.error.empty()&&q.untilTax<=q.untilMeal;}
    if(state.rulesVersion>=6)q.labor=planNativeLabor(seed,state);
    q.population=queryNativePopulationFeedback(seed,state);
    if(!q.population.error.empty()){q.error=q.population.error;return q;}
    q.entrance=queryNativeEntrance(seed,state,source);
    const auto roads=nativeRoads(seed,state),water=nativeWaterCoverage(seed,state);
    for(size_t n=0;n<state.objects.size();++n){const auto&o=state.objects[n];NativeObjectFeedback f;
        f.id=o.id;f.kind=o.kind;f.residents=o.residents;f.connected=nativeHouseConnected(seed,o,roads);
        f.watered=state.rulesVersion==8&&o.house?o.house->houseWater.value_or(0)!=0:state.rulesVersion==1||nativeHouseWatered(seed,o,water);
        if(o.kind==NativeObjectKind::House){const auto capacity=nativeHouseCapacity(seed,state,o);f.full=capacity&&o.residents>=*capacity;}
        if(const auto h=q.population.houses.find(o.id);h!=q.population.houses.end())f.housing=h->second;
        if(const auto w=q.population.wells.find(o.id);w!=q.population.wells.end())f.wellFlow=w->second;
        if(q.entrance.required&&o.kind==NativeObjectKind::House){
            const auto r=q.entrance.houses.find(o.id);if(r!=q.entrance.houses.end())f.entryReachable=r->second;
            const auto road=q.entrance.roadHouses.find(o.id);if(road!=q.entrance.roadHouses.end())f.entryRoadConnected=road->second;
            f.entryAccessError=q.entrance.error;
        }
        f.productionEnabled=o.kind==NativeObjectKind::Hunter&&f.connected&&state.huntingEnabled;
        if(const auto* job=q.labor.find(o.id)){f.workers=job->workers;f.requiredWorkers=job->required;}
        if(nativeUsesInspection(state)&&(o.kind==NativeObjectKind::House||o.kind==NativeObjectKind::Inspector))f.inspection=queryNativeInspectorFeedback(state,o.id);
        if(f.productionEnabled){++q.activeHunters;q.productionPotential+=state.rulesVersion>=6?(q.labor.find(o.id)?nativeHunterYield(*q.labor.find(o.id)):0):nativeFoodYield;}
        if(o.kind==NativeObjectKind::House){
            const bool eligible=!f.full&&f.connected&&f.watered;
            f.arrivalPlanned=q.population.scheduled?bool(f.housing&&f.housing->incoming):state.rulesVersion<3?eligible:std::binary_search(q.food.arrivals.begin(),q.food.arrivals.end(),n);
            // A disconnected/dry house has no allocation request. Report its
            // structural causes, plus a city-wide existing-resident shortfall.
            f.foodShortage=!q.population.scheduled&&state.rulesVersion>=3&&!f.full&&((q.food.cycle.fed<q.food.cycle.demand&&!q.food.laborOverride)||(eligible&&!f.arrivalPlanned));
        }
        q.objects.push_back(f);
    }
    if(state.ticks>=1000000000ULL)q.error="tick_limit";
    else if(!q.food.error.empty())q.error=q.food.error;
    else if(state.rulesVersion>=6&&!q.labor.error.empty())q.error=q.labor.error;
    return q;
}
} // namespace game_lab
