// SPDX-License-Identifier: MIT
#pragma once
#include "original_building_update.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
namespace game_lab {
// Pure adapter over the existing strict step. The supplied water is an
// internal state/test input, never native radius coverage or a player setting.
// A direction is an intent: the strict selector can still refuse its target.
// Only a successful step exposes a candidate; neither input nor context is
// mutated. The caller still owns scheduling, admission and publication.
struct NativeHouseEvolutionPlan {
    std::optional<int> direction;
    std::optional<OriginalBuildingUpdateState> candidate;
    // Present together with candidate only after the strict step and supplied
    // pool mirrors succeed. Includes conversion/redraw/ownership side effects.
    std::optional<OriginalBuildingUpdateContext> candidateContext;
    // This is a limited invariant, not permission to publish. Ownership repair,
    // relocation or retirement needs an explicit caller transaction policy.
    bool retainsHouseIdentityAndGeometry=false;
    std::string invariantError;
    OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::MissingContext;
    std::string error;
};
inline NativeHouseEvolutionPlan planNativeHouseEvolution(
    const OriginalBuildingUpdateState& source,const OriginalBuildingUpdateContext& context,
    std::optional<uint8_t> water=std::nullopt) {
    NativeHouseEvolutionPlan out;
    const auto refuse=[&](const char* error){out.error=error;return out;};
    if(!water)return refuse("house_water_unconnected");
    if(*water>2)return refuse("invalid_house_water");
    const auto type=source.building.type;
    if(type!=2&&type!=3&&type!=5&&type!=7)return refuse("house_type_unsupported");
    if(!source.inputs.house)return refuse("house_runtime_unconnected");
    if(!context.monthPart)return refuse("house_cycle_unmapped");
    if(*context.monthPart>15)return refuse("invalid_house_month_part");
    if(!context.cropBase||!context.activeSide||!*context.activeSide||*context.activeSide>228||
       *context.cropBase>=OriginalTerrainState::cells||!context.terrainOwners||
       context.terrainOwners->size()!=OriginalTerrainState::cells||!source.building.imageInputs)
        return refuse("house_geometry_unconnected");
    const bool empty=(type==2||type==3)&&source.inputs.residents<=0;
    if(!empty&&(!context.evolutionDisabled||
       (!*context.evolutionDisabled&&(!context.forceHouseEvolution||!context.houseModels||
                                     !source.inputs.modelDesirability))))
        return refuse("house_evolution_context_unconnected");
    auto candidate=source;auto scratch=context;candidate.inputs.houseWater=*water;
    scratch.houseEvolutionResult=0;
    out.blocker=originalBuildingHouseEvolutionStep(candidate,scratch);
    if(out.blocker==OriginalBuildingUpdateBlocker::None){
        out.direction=scratch.houseEvolutionResult;
        // Match the existing phase39 publication order on copies: the local
        // record first, then the supplied pool and its figure-service mirrors.
        // A partial/missing supplied pool refuses without exposing its prefix.
        const size_t slot=candidate.building.slot;
        if(bool(scratch.buildings)!=bool(scratch.buildingInputs)||
           (scratch.buildingLinks&&(!scratch.buildings||!scratch.buildingInputs))||
           (scratch.buildings&&!scratch.buildings->count(slot))||
           (scratch.buildingInputs&&!scratch.buildingInputs->count(slot))){
            out.blocker=OriginalBuildingUpdateBlocker::InvalidState;
            out.error="house_candidate_pool_unconnected";return out;
        }
        if(scratch.buildings)scratch.buildings->at(slot)=candidate.building;
        if(scratch.buildingInputs)scratch.buildingInputs->at(slot)=candidate.inputs;
        if(!originalBuildingSynchronizePool(scratch)){
            out.blocker=OriginalBuildingUpdateBlocker::InvalidState;
            out.error="house_candidate_pool_sync_failed";return out;
        }
        if(candidate.inputs.alive!=1&&candidate.inputs.alive!=3)
            out.invariantError="house_candidate_inactive";
        else if(candidate.building.slot!=source.building.slot||
                candidate.inputs.identity!=source.inputs.identity||
                candidate.inputs.identity!=candidate.building.slot)
            out.invariantError="house_candidate_identity_changed";
        else if(candidate.building.x!=source.building.x||candidate.building.y!=source.building.y||
                candidate.building.size!=source.building.size||
                candidate.building.width!=source.building.width||
                candidate.building.height!=source.building.height||
                candidate.building.imageInputs->gridIndex!=source.building.imageInputs->gridIndex)
            out.invariantError="house_candidate_geometry_changed";
        else if(scratch.ownershipRepairs!=context.ownershipRepairs)
            out.invariantError="house_candidate_ownership_repaired";
        out.retainsHouseIdentityAndGeometry=out.invariantError.empty();
        out.candidate=std::move(candidate);
        out.candidateContext=std::move(scratch);return out;
    }
    // The step records its chosen direction before target selection. Preserve
    // this intent while refusing the unsupported conversion, rather than
    // duplicating the strict direction expression or publishing partial state.
    if(out.blocker==OriginalBuildingUpdateBlocker::HousingEvolution&&
       scratch.houseEvolutionResult!=0){
        out.direction=scratch.houseEvolutionResult;
        out.error="house_evolution_target_unsupported";
    }else out.error=originalBuildingUpdateBlockerName(out.blocker);
    return out;
}
} // namespace game_lab
