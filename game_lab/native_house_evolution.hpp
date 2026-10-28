// SPDX-License-Identifier: MIT
#pragma once
#include "asset_bytes.hpp"
#include "native_house_evolution_plan.hpp"
#include "native_house_runtime.hpp"
#include "native_session_source.hpp"
#include "original_figure.hpp"
#include "original_map.hpp"
#include "original_save.hpp"
#include "resource_root.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace game_lab {
// Read-only projection of the existing strict house desirability, requirement
// and direction functions. Nothing here is scheduled, persisted or applied to
// the native session; callers receive a copy of the current projection.
struct NativeHouseModels {
    uint64_t sourceRevision=0,modelRevision=0;unsigned difficulty=0;
    std::map<uint16_t,std::array<int32_t,13>> building;
    std::map<int,std::array<int32_t,24>> house;
};
// missing: source_unreadable/source_changed/model_changed/
// saved_difficulty_unreadable/models_unparsed
inline std::optional<NativeHouseModels> loadNativeHouseModels(
    const ResourceRoot& root,const NativeSessionSeed& seed,std::string& missing) {
    missing.clear();
    if(!root.configured()){missing="source_unreadable";return std::nullopt;}
    std::vector<uint8_t> raw,text;
    if(!detail::readFile(root.resolve(seed.source),4194304,raw)){missing="source_unreadable";return std::nullopt;}
    const auto save=decodeOriginalSave(raw);
    if(save.status!=OriginalSaveStatus::LoadedSnapshot){missing="source_unreadable";return std::nullopt;}
    if(save.snapshotRevision!=seed.sourceRevision){missing="source_changed";return std::nullopt;}
    const auto payload=decodeOriginalMapContainer(raw);
    if(!payload.complete){missing="source_unreadable";return std::nullopt;}
    const auto difficulty=originalFigureSavedDifficulty(payload.payload,save.sizeOffset);
    if(!difficulty||*difficulty>=5){missing="saved_difficulty_unreadable";return std::nullopt;}
    if(!detail::readFile(root.resolve("Model/EmperorBuildingModels.txt"),1048576,text)){
        missing="models_unparsed";return std::nullopt;}
    const std::string modelText(text.begin(),text.end());
    if(nativeTextRevision(modelText)!=seed.modelRevision){missing="model_changed";return std::nullopt;}
    auto buildings=originalBuildingModels(modelText,*difficulty);
    auto houses=originalBuildingHouseModels(modelText,*difficulty);
    if(!buildings||!houses){missing="models_unparsed";return std::nullopt;}
    NativeHouseModels out;
    out.sourceRevision=seed.sourceRevision;out.modelRevision=seed.modelRevision;
    out.difficulty=*difficulty;out.building=std::move(*buildings);out.house=std::move(*houses);
    return out;
}
struct NativeHouseEvolutionView {
    uint64_t id=0;bool emptyEarlyReturn=false;
    bool observedReturnEnabled=false,observedDryEnabled=false,nonemptyRehousingEnabled=false;uint32_t pendingExcess=0;
    bool runtimeEnabled=false;uint32_t runtimePhase=0;uint16_t runtimeType=0,runtimeModel=0;uint8_t runtimeDelay=0;std::string runtimeHold;
    std::optional<int> desirability,desirabilityWithBonus;      // runtime bonus 0/1
    std::optional<int> currentThresholdDown,currentThresholdUp; // current row m0/m1
    std::optional<bool> currentMet,nextMet;                     // 51A3F0 false/true
    std::optional<int> direction;                               // intent only, strict when invariant
    // These are hypothetical strict-step outcomes, not a publishable session.
    // v1-v6 projection does not contain the real runtime tail/clock.
    std::optional<bool> candidateAcceptedIfWater,candidateAcceptedIfNoWater;
    std::optional<uint16_t> candidateTypeIfWater,candidateTypeIfNoWater;
    std::optional<int16_t> candidateModelIfWater,candidateModelIfNoWater;
    bool candidatePublishable=false; // no query publishes or enables automatic evolution
    bool runtimeStateKnown=false,scenarioClockAssumed=true;
    std::optional<uint8_t> actualWater;
    std::optional<uint32_t> actualMonthPart;
    // Uses only persisted v7 state. A missing clock is never replaced with the
    // what-if phase used by direction-only scenarios.
    NativeHouseEvolutionPlan actualPlan;
    std::vector<std::string> candidateMissing,scenarioMissing;
    // Strict 519120 results under the two original phase28 values. Water is
    // binary for requirements; a nonzero value (1 or 2) is the watered branch.
    std::optional<int> directionIfWater,directionIfNoWater;
    std::optional<bool> currentMetIfWater,currentMetIfNoWater,nextMetIfWater,nextMetIfNoWater;
    // Self-written native coverage. Never feeds any strict field above.
    std::optional<bool> nativeWatered;
    std::vector<std::string> notNeeded,missing;
};
struct NativeHouseEvolutionReport {
    std::string error;std::vector<NativeHouseEvolutionView> houses;
    const NativeHouseEvolutionView* find(uint64_t id) const {
        for(const auto& h:houses)if(h.id==id)return &h;return nullptr;
    }
};
namespace detail {
inline const NativeSessionObject* nativeHouseObject(const NativeSessionState& state,size_t slot){
    for(const auto& o:state.objects)if(o.id==slot)return &o;return nullptr;
}
inline uint32_t nativeHouseResidents(const NativeSessionState& state,size_t slot){
    const auto* object=nativeHouseObject(state,slot);return object?object->residents:0;
}
inline bool sameNativeHouseCandidate(const std::optional<OriginalBuildingUpdateState>& a,
                                    const std::optional<OriginalBuildingUpdateState>& b){
    if(bool(a)!=bool(b))return false;if(!a)return true;
    if(a->building.type!=b->building.type||a->building.x!=b->building.x||
       a->building.y!=b->building.y||a->inputs.model!=b->inputs.model||
       a->inputs.house!=b->inputs.house||a->inputs.houseWater!=b->inputs.houseWater||
       a->inputs.alive!=b->inputs.alive||a->inputs.residents!=b->inputs.residents||
       bool(a->building.imageInputs)!=bool(b->building.imageInputs))return false;
    if(!a->building.imageInputs)return true;
    return a->building.imageInputs->gridIndex==b->building.imageInputs->gridIndex&&
           a->building.imageInputs->word20==b->building.imageInputs->word20&&
           a->building.imageInputs->houseByte3c==b->building.imageInputs->houseByte3c;
}
} // namespace detail
// All four strict calls run on copies. The session and saved scene are const.
inline NativeHouseEvolutionReport queryNativeHouseEvolution(
    const NativeSessionSource& src,const NativeSessionState& st,const NativeHouseModels& m) {
    NativeHouseEvolutionReport out;
    if(m.sourceRevision!=src.seed.sourceRevision){out.error="source_changed";return out;}
    if(m.modelRevision!=src.seed.modelRevision){out.error="model_changed";return out;}
    if(m.difficulty>=5||!m.house.count(0)){out.error="models_unparsed";return out;}
    if(st.rulesVersion==8){
        if(!src.runtime){out.error="runtime_source_unavailable";return out;}
        const auto contract=resolveNativeHouseRuntimeContract(st,out.error);if(!contract)return out;
        const auto phaseInputs=contract->phaseInputs(st);
        const auto converted=nativeToStrict(src.seed,st,src.runtime->strictTemplate,nativeStrictPersisted(st));
        if(!converted.city){out.error=converted.refusal;return out;}
        const auto& city=*converted.city;const auto& c=*city.buildingContext;
        for(const auto& o:st.objects)if(o.house){
            NativeHouseEvolutionView v;v.id=o.id;v.runtimeEnabled=true;v.observedReturnEnabled=nativeUsesObservedWellReturn(st);v.runtimePhase=st.houseSchedule->phase;v.runtimeType=o.house->type;v.runtimeModel=o.house->model;
            v.nonemptyRehousingEnabled=nativeUsesNonemptyMigrantRehousing(st);v.observedDryEnabled=st.observedDry.has_value();if(st.observedDry&&st.observedDry->pendingExcess.count(o.id))v.pendingExcess=st.observedDry->pendingExcess.at(o.id);
            v.runtimeDelay=o.house->remaining[56-4];v.runtimeStateKnown=true;v.scenarioClockAssumed=false;v.actualWater=o.house->houseWater;v.actualMonthPart=st.houseSchedule->monthPart;
            const auto held=st.heldHouseReasons.find(o.id);if(held!=st.heldHouseReasons.end())v.runtimeHold=held->second;
            const auto& b=city.buildings.at(size_t(o.id)).state;v.emptyEarlyReturn=(b.building.type==2||b.building.type==3)&&!o.residents;
            if(b.inputs.modelDesirability)v.desirabilityWithBonus=int(*b.inputs.modelDesirability);
            v.desirability=int(b.inputs.desirability);
            const auto row=c.houseModels->find(b.inputs.model);if(row!=c.houseModels->end()){v.currentThresholdDown=row->second[0];v.currentThresholdUp=row->second[1];}
            v.currentMet=originalBuildingHouseRequirements(b,c,false);v.nextMet=originalBuildingHouseRequirements(b,c,true);
            auto atEvolution=c;atEvolution.phase=39;
            // Same pure admission/plan as runtime, using CURRENT saved inputs;
            // this never predicts service changes before the next phase39.
            v.actualPlan=planNativePhaseHouseEvolution(b,atEvolution,phaseInputs,v.runtimeHold);
            v.direction=v.actualPlan.direction;
            if(!v.actualPlan.error.empty())v.candidateMissing.push_back(v.actualPlan.error);
            out.houses.push_back(std::move(v));
        }
        return out;
    }
    const auto scene=nativeSessionScene(src,st);
    if(!scene){out.error="scene_invalid";return out;}
    const auto& g=scene->ground;
    if(g.state.terrain.size()!=OriginalTerrainState::cells||
       g.state.feature.size()!=OriginalTerrainState::cells||
       g.state.subTile.size()!=OriginalTerrainState::cells||
       g.owner.size()!=OriginalTerrainState::cells){out.error="scene_invalid";return out;}
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    c.buildingModels=m.building;c.houseModels=m.house;
    c.terrain=g.state.terrain;c.terrainOwners=g.owner;
    c.activeSide=g.activeSide;c.cropBase=g.cropStorageBase;
    c.resourceGrowth.emplace();c.resourceGrowth->surface=g.state;
    // Phase1 is only a legacy/unknown-clock what-if for direction display;
    // it never supplies actualPlan or pretends to be a persisted original clock.
    const std::optional<uint32_t> actualMonthPart=
        st.rulesVersion==8&&st.houseSchedule?std::optional<uint32_t>(st.houseSchedule->monthPart):st.rulesVersion>=7&&st.houseClock?std::optional<uint32_t>(st.houseClock->monthPart):std::nullopt;
    c.evolutionDisabled=false;c.forceHouseEvolution=false;
    c.monthPart=actualMonthPart.value_or(1);
    if(st.rulesVersion==8&&st.houseSchedule)c.phase=st.houseSchedule->phase;
    else if(st.rulesVersion>=7&&st.houseClock)c.phase=st.houseClock->phase;
    c.buildings.emplace();c.buildingInputs.emplace();
    for(const auto& f:scene->buildings.footprints){
        (*c.buildings)[f.slot]=f;OriginalBuildingUpdateInputs i;
        i.alive=1;i.identity=uint32_t(f.slot);
        if(originalBuildingIsHouse(f.type)){
            const auto* object=detail::nativeHouseObject(st,f.slot);
            if(st.rulesVersion>=7){
                if(!object||!object->house)return {"house_runtime_state_unconnected",{}};
                const auto& runtime=*object->house;
                if(runtime.type!=f.type)return {"house_projection_mismatch",{}};
                if(runtime.model>uint16_t(INT16_MAX))return {"house_model_out_of_range",{}};
                i.house=runtime.remaining;i.model=int16_t(runtime.model);
                // houseWater is independently cached; decay of remaining[]
                // never changes it. Unknown water stays unknown in the view.
                i.houseWater=runtime.houseWater.value_or(0);
            }else{
                i.house.emplace();i.house->fill(0); // legacy direction-only scenario
                i.model=int16_t(std::max(0,int(f.type)-3));
            }
            i.residents=int16_t(detail::nativeHouseResidents(st,f.slot));
        }
        (*c.buildingInputs)[f.slot]=std::move(i);
    }
    if(auto b=originalBuildingDesirabilityStep(c);b!=OriginalBuildingUpdateBlocker::None)
        return {originalBuildingUpdateBlockerName(b),{}};
    std::array<OriginalBuildingUpdateContext,2> h{c,c};
    for(uint8_t k=0;k<2;++k){
        h[k].buildings->clear();h[k].buildingInputs->clear();
        for(const auto& [slot,f]:*c.buildings)if(originalBuildingIsHouse(f.type)){
            (*h[k].buildings)[slot]=f;auto i=c.buildingInputs->at(slot);
            i.desirabilityBonus=k;i.modelDesirabilityBonus=k;
            (*h[k].buildingInputs)[slot]=std::move(i);
        }
        if(auto b=originalBuildingApplyDesirability(h[k]);b!=OriginalBuildingUpdateBlocker::None)
            return {originalBuildingUpdateBlockerName(b),{}};
    }
    // Columns whose strict requirement reads an unavailable runtime byte/word.
    // A nonzero demand makes that category unconnected, never a default zero.
    constexpr std::array<unsigned,14> runtimeColumns{2,4,5,6,7,8,9,10,11,12,13,21,22,23};
    const auto runtimeDemand=[&](const std::array<int32_t,24>& row){
        std::vector<std::string> out;
        for(unsigned col:runtimeColumns)if(row[col])out.push_back("house_runtime_col"+std::to_string(col));
        return out;
    };
    const auto addUnique=[](std::vector<std::string>& v,const std::string& name){
        if(std::find(v.begin(),v.end(),name)==v.end())v.push_back(name);
    };
    const auto nativeCoverage=nativeWaterCoverage(src.seed,st);
    for(const auto& entry:*h[0].buildings){
        const size_t slot=entry.first;const auto& f=entry.second;
        NativeHouseEvolutionView v;v.id=slot;
        const auto* object=detail::nativeHouseObject(st,slot);
        v.actualMonthPart=actualMonthPart;v.scenarioClockAssumed=!actualMonthPart;
        if(st.rulesVersion>=7&&object&&object->house){
            v.runtimeStateKnown=object->house->servicesKnown;
            v.actualWater=object->house->houseWater;
        }
        if(!v.runtimeStateKnown)addUnique(v.candidateMissing,"house_runtime_state_unconnected");
        if(!v.actualWater)addUnique(v.candidateMissing,"house_water_unconnected");
        if(!v.actualMonthPart)addUnique(v.candidateMissing,"house_cycle_unmapped");
        if(!v.runtimeStateKnown)v.actualPlan.error="house_runtime_state_unconnected";
        else if(!v.actualWater)v.actualPlan.error="house_water_unconnected";
        else if(!v.actualMonthPart)v.actualPlan.error="house_cycle_unmapped";
        v.desirability=int(h[0].buildingInputs->at(slot).desirability);
        v.desirabilityWithBonus=int(h[1].buildingInputs->at(slot).desirability);
        const int model=h[0].buildingInputs->at(slot).model;
        const auto currentRow=m.house.find(model);
        if(currentRow==m.house.end()){
            v.missing.push_back("house_model_row0");out.houses.push_back(std::move(v));continue;
        }
        const auto& current=currentRow->second;
        const int nextModel=f.type==11?model+2:model==14||model==8?model:model+1;
        std::optional<std::array<int32_t,24>> target;
        if(m.house.count(nextModel))target=m.house.at(nextModel);
        const auto currentRuntime=runtimeDemand(current);
        std::vector<std::string> targetRuntime;
        if(target)targetRuntime=runtimeDemand(*target);
        v.currentThresholdDown=current[0];v.currentThresholdUp=current[1];
        for(const auto& object:st.objects)if(object.id==slot){
            v.nativeWatered=nativeHouseWatered(src.seed,object,nativeCoverage);break;}
        const auto residents=detail::nativeHouseResidents(st,slot);
        if((f.type==2||f.type==3)&&residents==0){
            v.emptyEarlyReturn=true;v.notNeeded.push_back("residents_zero_early_return");
            out.houses.push_back(std::move(v));continue;
        }
        if(!target){
            v.missing.push_back("house_model_row1");v.actualPlan.error="house_model_row1";
            out.houses.push_back(std::move(v));continue;
        }
        if(st.rulesVersion>=7&&!v.runtimeStateKnown){
            // Persisted bytes marked unknown are not evidence, even when they
            // happen to be zero or a model demand happens to be zero.
            v.missing.push_back("house_runtime_state_unconnected");
            out.houses.push_back(std::move(v));continue;
        }
        // A row requirement only depends on unavailable runtime bytes when that
        // row carries a nonzero demand; name each one instead of substituting0.
        const auto requirementUnder=[&](bool next,uint8_t water)->std::optional<bool>{
            OriginalBuildingUpdateState s{f,h[0].buildingInputs->at(slot),std::nullopt};
            s.inputs.houseWater=water;
            return originalBuildingHouseRequirements(s,h[0],next);
        };
        const bool currentUnknown=!v.runtimeStateKnown&&!currentRuntime.empty();
        const bool targetUnknown=!v.runtimeStateKnown&&!targetRuntime.empty();
        bool currentVaries=false,targetVaries=false;
        std::optional<bool> currentKnown,nextKnown;
        std::optional<bool> currentIfWater,currentIfNoWater,nextIfWater,nextIfNoWater;
        if(!currentUnknown){
            currentIfWater=requirementUnder(false,2);currentIfNoWater=requirementUnder(false,0);
            if(currentIfWater&&currentIfNoWater){
                if(*currentIfWater==*currentIfNoWater)currentKnown=currentIfWater;else currentVaries=true;
            }else currentVaries=true;
        }
        if(!targetUnknown){
            nextIfWater=requirementUnder(true,2);nextIfNoWater=requirementUnder(true,0);
            if(nextIfWater&&nextIfNoWater){
                if(*nextIfWater==*nextIfNoWater)nextKnown=nextIfWater;else targetVaries=true;
            }else targetVaries=true;
        }
        v.currentMet=currentKnown;v.nextMet=nextKnown;
        if(v.actualWater){
            if(!currentUnknown)v.currentMet=requirementUnder(false,*v.actualWater);
            if(!targetUnknown)v.nextMet=requirementUnder(true,*v.actualWater);
        }
        v.currentMetIfWater=currentIfWater;v.currentMetIfNoWater=currentIfNoWater;
        v.nextMetIfWater=nextIfWater;v.nextMetIfNoWater=nextIfNoWater;
        if(!v.runtimeStateKnown){
            for(const auto& name:currentRuntime)addUnique(v.missing,name);
            for(const auto& name:targetRuntime)addUnique(v.missing,name);
        }
        if(!v.actualWater&&(currentVaries||targetVaries))addUnique(v.missing,"house_water_phase28");
        if(currentUnknown||targetUnknown){out.houses.push_back(std::move(v));continue;}
        // Ask the existing strict step on copies for all four scenarios.
        // Its direction remains an intent if selecting the target is refused.
        static constexpr std::array<uint8_t,2> waterValues{0,2}; // dry / watered
        NativeHouseEvolutionPlan plans[2][2];
        int dir[2][2]{};bool blocked=false;
        for(int k=0;k<2;++k)for(int w=0;w<2;++w){
            OriginalBuildingUpdateState source{f,h[k].buildingInputs->at(slot),std::nullopt};
            plans[k][w]=planNativeHouseEvolution(source,h[k],waterValues[size_t(w)]);
            if(!plans[k][w].direction)blocked=true;
            else dir[k][w]=*plans[k][w].direction;
            if(!plans[k][w].error.empty()){
                addUnique(v.scenarioMissing,plans[k][w].error);
                if(!v.runtimeStateKnown)addUnique(v.candidateMissing,plans[k][w].error);
            }
        }
        if(v.runtimeStateKnown&&v.actualWater&&v.actualMonthPart){
            NativeHouseEvolutionPlan actual[2];
            for(int k=0;k<2;++k){
                auto actualContext=h[k];actualContext.monthPart=v.actualMonthPart;
                OriginalBuildingUpdateState source{f,h[k].buildingInputs->at(slot),std::nullopt};
                actual[k]=planNativeHouseEvolution(source,actualContext,v.actualWater);
            }
            if(actual[0].direction==actual[1].direction&&actual[0].blocker==actual[1].blocker&&
               actual[0].error==actual[1].error&&
               detail::sameNativeHouseCandidate(actual[0].candidate,actual[1].candidate)){
                v.actualPlan=std::move(actual[0]);
            }else v.actualPlan.error="house_desirability_bonus_unconnected";
            if(!v.actualPlan.error.empty())addUnique(v.candidateMissing,v.actualPlan.error);
        }
        for(int w=0;w<2;++w){
            const auto& a=plans[0][w].candidate;const auto& b=plans[1][w].candidate;
            auto& accepted=w?v.candidateAcceptedIfWater:v.candidateAcceptedIfNoWater;
            if(bool(a)==bool(b))accepted=bool(a);
            if(a&&b&&a->building.type==b->building.type&&a->inputs.model==b->inputs.model){
                (w?v.candidateTypeIfWater:v.candidateTypeIfNoWater)=a->building.type;
                (w?v.candidateModelIfWater:v.candidateModelIfNoWater)=a->inputs.model;
            }
        }
        if(blocked){addUnique(v.missing,"house_evolution_519120_518ef0");out.houses.push_back(std::move(v));continue;}
        // dir[k][0] is houseWater=0; dir[k][1] is nonzero houseWater=2.
        const bool bonusSame=dir[0][0]==dir[1][0]&&dir[0][1]==dir[1][1];
        const bool waterSame=dir[0][0]==dir[0][1]&&dir[1][0]==dir[1][1];
        if(!v.runtimeStateKnown)v.notNeeded.push_back("house_runtime_tail");
        const bool relevantBonusSame=v.actualWater?
            dir[0][*v.actualWater?1:0]==dir[1][*v.actualWater?1:0]:bonusSame;
        if(relevantBonusSame)v.notNeeded.push_back("house_desirability_bonus_60");
        else addUnique(v.missing,"house_desirability_bonus_60");
        const bool requirementsNeedWater=(currentIfWater&&currentIfNoWater&&*currentIfWater!=*currentIfNoWater)||
                                         (nextIfWater&&nextIfNoWater&&*nextIfWater!=*nextIfNoWater);
        if(waterSame&&!requirementsNeedWater)v.notNeeded.push_back("house_water_phase28");
        else if(!v.actualWater)addUnique(v.missing,"house_water_phase28");
        if(dir[0][0]==dir[1][0])v.directionIfNoWater=dir[0][0];
        if(dir[0][1]==dir[1][1])v.directionIfWater=dir[0][1];
        if(v.actualWater&&relevantBonusSame)v.direction=dir[0][*v.actualWater?1:0];
        else if(bonusSame&&waterSame)v.direction=dir[0][0];
        out.houses.push_back(std::move(v));
    }
    return out;
}
} // namespace game_lab
