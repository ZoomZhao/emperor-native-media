// SPDX-License-Identifier: MIT
#pragma once
#include "asset_bytes.hpp"
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
    std::optional<int> desirability,desirabilityWithBonus;      // runtime bonus 0/1
    std::optional<int> currentThresholdDown,currentThresholdUp; // current row m0/m1
    std::optional<bool> currentMet,nextMet;                     // 51A3F0 false/true
    std::optional<int> direction;                               // strict only when invariant
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
inline uint32_t nativeHouseResidents(const NativeSessionState& state,size_t slot){
    for(const auto& o:state.objects)if(o.id==slot)return o.residents;return 0;
}
} // namespace detail
// All four strict calls run on copies. The session and saved scene are const.
inline NativeHouseEvolutionReport queryNativeHouseEvolution(
    const NativeSessionSource& src,const NativeSessionState& st,const NativeHouseModels& m) {
    NativeHouseEvolutionReport out;
    if(m.sourceRevision!=src.seed.sourceRevision){out.error="source_changed";return out;}
    if(m.modelRevision!=src.seed.modelRevision){out.error="model_changed";return out;}
    if(m.difficulty>=5||!m.house.count(0)){out.error="models_unparsed";return out;}
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
    // Read-only: no scheduling, no delay counter, no conversion.
    c.evolutionDisabled=false;c.forceHouseEvolution=false;c.monthPart=1;
    c.buildings.emplace();c.buildingInputs.emplace();
    for(const auto& f:scene->buildings.footprints){
        (*c.buildings)[f.slot]=f;OriginalBuildingUpdateInputs i;
        i.alive=1;i.identity=uint32_t(f.slot);
        if(originalBuildingIsHouse(f.type)){
            i.house.emplace();i.house->fill(0);
            i.model=int16_t(std::max(0,int(f.type)-3)); //518DE0 semantic model
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
    const auto& current=m.house.at(0);
    std::optional<std::array<int32_t,24>> target;
    if(m.house.count(1))target=m.house.at(1);
    // Columns whose strict requirement reads an unavailable runtime byte/word.
    // A nonzero demand makes that category unconnected, never a default zero.
    constexpr std::array<unsigned,14> runtimeColumns{2,4,5,6,7,8,9,10,11,12,13,21,22,23};
    const auto runtimeDemand=[&](const std::array<int32_t,24>& row){
        std::vector<std::string> out;
        for(unsigned col:runtimeColumns)if(row[col])out.push_back("house_runtime_col"+std::to_string(col));
        return out;
    };
    const auto currentRuntime=runtimeDemand(current);
    std::vector<std::string> targetRuntime;
    if(target)targetRuntime=runtimeDemand(*target);
    const auto addUnique=[](std::vector<std::string>& v,const std::string& name){
        if(std::find(v.begin(),v.end(),name)==v.end())v.push_back(name);
    };
    const auto nativeCoverage=nativeWaterCoverage(src.seed,st);
    for(const auto& entry:*h[0].buildings){
        const size_t slot=entry.first;const auto& f=entry.second;
        NativeHouseEvolutionView v;v.id=slot;
        v.desirability=int(h[0].buildingInputs->at(slot).desirability);
        v.desirabilityWithBonus=int(h[1].buildingInputs->at(slot).desirability);
        v.currentThresholdDown=current[0];v.currentThresholdUp=current[1];
        for(const auto& object:st.objects)if(object.id==slot){
            v.nativeWatered=nativeHouseWatered(src.seed,object,nativeCoverage);break;}
        const auto residents=detail::nativeHouseResidents(st,slot);
        if(f.type==2||residents==0){
            v.emptyEarlyReturn=true;v.notNeeded.push_back("residents_zero_early_return");
            out.houses.push_back(std::move(v));continue;
        }
        if(!target){
            v.missing.push_back("house_model_row1");
            out.houses.push_back(std::move(v));continue;
        }
        // A row requirement only depends on unavailable runtime bytes when that
        // row carries a nonzero demand; name each one instead of substituting0.
        const auto requirementUnder=[&](bool next,uint8_t water)->std::optional<bool>{
            OriginalBuildingUpdateState s{f,h[0].buildingInputs->at(slot),std::nullopt};
            s.inputs.houseWater=water;
            return originalBuildingHouseRequirements(s,h[0],next);
        };
        const bool currentUnknown=!currentRuntime.empty();
        const bool targetUnknown=!targetRuntime.empty();
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
        v.currentMetIfWater=currentIfWater;v.currentMetIfNoWater=currentIfNoWater;
        v.nextMetIfWater=nextIfWater;v.nextMetIfNoWater=nextIfNoWater;
        for(const auto& name:currentRuntime)addUnique(v.missing,name);
        for(const auto& name:targetRuntime)addUnique(v.missing,name);
        if(currentVaries||targetVaries)addUnique(v.missing,"house_water_phase28");
        if(currentUnknown||targetUnknown){out.houses.push_back(std::move(v));continue;}
        // Same strict direction expression as519120: base threshold, current
        // requirements can force -1, target requirements only apply on +1.
        static constexpr std::array<uint8_t,2> waterValues{0,2}; // dry / watered
        int dir[2][2]{};bool blocked=false;
        for(int k=0;k<2&&!blocked;++k)for(int w=0;w<2;++w){
            const auto& inputs=h[k].buildingInputs->at(slot);
            OriginalBuildingUpdateState s{f,inputs,std::nullopt};
            s.inputs.houseWater=waterValues[size_t(w)];
            const auto threshold=m.house.at(inputs.model);
            int direction=!inputs.modelDesirability?-1:
                *inputs.modelDesirability<=threshold[0]?-1:
                *inputs.modelDesirability>=(inputs.model>=19?1000:threshold[1])?1:0;
            const auto req=originalBuildingHouseRequirements(s,h[k],false);
            if(!req){blocked=true;break;}
            if(!*req)direction=-1;
            else if(direction==1){
                const auto nextReq=originalBuildingHouseRequirements(s,h[k],true);
                if(!nextReq){blocked=true;break;}
                direction=int(*nextReq);
            }
            dir[k][w]=direction;
        }
        if(blocked){addUnique(v.missing,"house_evolution_519120_518ef0");out.houses.push_back(std::move(v));continue;}
        // dir[k][0] is houseWater=0; dir[k][1] is nonzero houseWater=2.
        const bool bonusSame=dir[0][0]==dir[1][0]&&dir[0][1]==dir[1][1];
        const bool waterSame=dir[0][0]==dir[0][1]&&dir[1][0]==dir[1][1];
        v.notNeeded.push_back("house_runtime_tail");
        if(bonusSame)v.notNeeded.push_back("house_desirability_bonus_60");
        else addUnique(v.missing,"house_desirability_bonus_60");
        const bool requirementsNeedWater=(currentIfWater&&currentIfNoWater&&*currentIfWater!=*currentIfNoWater)||
                                         (nextIfWater&&nextIfNoWater&&*nextIfWater!=*nextIfNoWater);
        if(waterSame&&!requirementsNeedWater)v.notNeeded.push_back("house_water_phase28");
        else addUnique(v.missing,"house_water_phase28");
        if(bonusSame){v.directionIfNoWater=dir[0][0];v.directionIfWater=dir[0][1];}
        if(bonusSame&&waterSame)v.direction=dir[0][0];
        out.houses.push_back(std::move(v));
    }
    return out;
}
} // namespace game_lab
