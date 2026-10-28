// SPDX-License-Identifier: MIT
#pragma once
#include "native_house_evolution_plan.hpp"
#include <limits>

namespace game_lab {
// Explicit native policy, disabled by default. Independent scalar observations
// 2658 (type4/model1/residents14/delay9) -> 2659 (same slot/2x2, type3/model0,
// residents14/free0/delay0) support this dry branch. 2660 records the LATER
// capacity/excess-resident work; this helper deliberately does not do that work.
// The delay prefix is the existing migrated strict contract, cross-checked by
// the observed dry delay sequence. It is not evidence for other house branches,
// original calendar timing, wet recovery or desirability-driven upgrades.
// RecoveryV2 is an explicit native policy. Original Save30/2787..2790
// observed real Well tail96, cache0->1, then delay3->0 across c172/phase39,
// retaining type4/model1/14 residents/2x2. Delays1..9 use the same existing
// strict reset contract; only delay3 was independently observed. This does
// not admit higher-grade evolution, empty houses, other services or timing.
enum class NativeObservedDryDowngradePolicy { Disabled, WaterOnlyType4To3V1, WaterOnlyType4To3AndRecoveryV2 };

inline NativeHouseEvolutionPlan planNativeObservedDryDowngrade(
    const OriginalBuildingUpdateState& source,const OriginalBuildingUpdateContext& context,
    NativeObservedDryDowngradePolicy policy=NativeObservedDryDowngradePolicy::Disabled) {
    const auto refuse=[](const char* error,OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::HousingEvolution){
        NativeHouseEvolutionPlan out;out.error=error;out.blocker=blocker;return out;
    };
    if(policy!=NativeObservedDryDowngradePolicy::WaterOnlyType4To3V1&&policy!=NativeObservedDryDowngradePolicy::WaterOnlyType4To3AndRecoveryV2)
        return refuse("observed_dry_downgrade_disabled");
    const auto& b=source.building;const auto& i=source.inputs;
    if(!context.phase||*context.phase!=39)return refuse("observed_dry_downgrade_phase39_required");
    if(b.type!=4||i.model!=1||i.alive!=1||i.residents<=0||i.houseInUse!=1)
        return refuse("observed_dry_downgrade_house_domain");
    // These are the native square constructor's normal form, not inferred
    // rectangular/merged original housing geometry.
    if(b.size!=2||b.width||b.height||!b.slot||b.slot>=4000||i.identity!=b.slot)
        return refuse("observed_dry_downgrade_identity_or_shape");
    if(!i.house)return refuse("observed_dry_downgrade_tail_required",OriginalBuildingUpdateBlocker::MissingContext);
    const uint8_t delay=i.house->at(56-4);
    const bool recovery=policy==NativeObservedDryDowngradePolicy::WaterOnlyType4To3AndRecoveryV2&&i.houseWater==1&&i.house->at(45)>0&&i.house->at(47)==0&&delay>0;
    if(!recovery&&(i.houseWater||i.house->at(49-4)||i.house->at(51-4)))
        return refuse("observed_dry_downgrade_dry_state_required");
    if(!context.evolutionDisabled||*context.evolutionDisabled||!context.forceHouseEvolution||*context.forceHouseEvolution)
        return refuse("observed_dry_downgrade_normal_evolution_required");
    if(!context.houseModels||!context.houseModels->count(1)||!context.houseModels->count(0)||!i.modelDesirability)
        return refuse("observed_dry_downgrade_models_required",OriginalBuildingUpdateBlocker::MissingContext);
    const auto& model=context.houseModels->at(1);
    if(model[3]!=1)return refuse("observed_dry_downgrade_water_only_model_required");
    // Only columns consumed by the shared requirements function are services.
    // Other model columns (capacity/tax/etc.) must not be mistaken for demands.
    for(unsigned n:{2u,4u,5u,6u,7u,8u,9u,10u,11u,12u,13u,21u,22u,23u})
        if(model[n])return refuse("observed_dry_downgrade_water_only_model_required");
    const auto requirements=originalBuildingHouseRequirements(source,context,false);
    if(!requirements||bool(*requirements)!=recovery)return refuse("observed_dry_downgrade_requirements_mismatch");
    if(!context.monthPart||*context.monthPart>15)
        return refuse("observed_dry_downgrade_month_part_required",OriginalBuildingUpdateBlocker::MissingContext);
    if(!context.activeSide||*context.activeSide<2||*context.activeSide>228||
       !context.cropBase||*context.cropBase>=OriginalTerrainState::cells||
       b.x>*context.activeSide-2||b.y>*context.activeSide-2||
       !context.terrainOwners||context.terrainOwners->size()!=OriginalTerrainState::cells||!b.imageInputs)
        return refuse("observed_dry_downgrade_geometry_required",OriginalBuildingUpdateBlocker::MissingContext);
    const uint64_t grid=uint64_t(*context.cropBase)+b.x+228ULL*b.y;
    if(grid!=b.imageInputs->gridIndex||grid+229>=OriginalTerrainState::cells)
        return refuse("observed_dry_downgrade_grid_mismatch",OriginalBuildingUpdateBlocker::InvalidState);
    for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)
        if(context.terrainOwners->at(size_t(grid)+228*dy+dx)!=b.slot)
            return refuse("observed_dry_downgrade_ownership_mismatch",OriginalBuildingUpdateBlocker::InvalidState);
    if(b.imageInputs->word20!=i.residents||b.imageInputs->houseByte3c!=i.house->at(55))
        return refuse("observed_dry_downgrade_image_projection_mismatch",OriginalBuildingUpdateBlocker::InvalidState);
    // Preserve the existing source/target selectors' admission. In particular,
    // this does not broaden the separately evidenced occupied type4 variant.
    if(!originalBuildingBaseSelection(b))return refuse("observed_dry_downgrade_source_selector_unported");
    if(delay>9)return refuse("observed_dry_downgrade_delay_outside_observed_domain");
    const bool converted=!recovery&&delay==9;
    const bool goodsPart=*context.monthPart==0||*context.monthPart==7;
    if(converted&&goodsPart)for(unsigned n:{9u,10u,11u,12u,13u})
        if(context.houseModels->at(0)[n]>0)return refuse("observed_dry_downgrade_target_goods_unported");
    if(context.houseEvolutionCalls==std::numeric_limits<unsigned>::max()||
       (converted&&context.houseConversions==std::numeric_limits<unsigned>::max())||
       (goodsPart&&context.houseRedraws==std::numeric_limits<unsigned>::max()))
        return refuse("observed_dry_downgrade_counter_exhausted",OriginalBuildingUpdateBlocker::InvalidState);
    const size_t slot=b.slot;
    if(bool(context.buildings)!=bool(context.buildingInputs)||
       (context.buildingLinks&&(!context.buildings||!context.buildingInputs))||
       (context.buildings&&!context.buildings->count(slot))||
       (context.buildingInputs&&!context.buildingInputs->count(slot))||
       (context.houses&&!context.houses->count(slot)))
        return refuse("observed_dry_downgrade_pool_required",OriginalBuildingUpdateBlocker::MissingContext);

    auto candidate=source;auto scratch=context;
    candidate.inputs.house->at(56-4)=(converted||recovery)?0:uint8_t(delay+1);
    if(converted){
        candidate.building.type=3;candidate.inputs.model=0;
        if(!originalBuildingBaseSelection(candidate.building))
            return refuse("observed_dry_downgrade_target_selector_unported");
        ++scratch.houseConversions;scratch.buildingRedraws.insert(slot);
    }
    // No capacity update, clamping, migration/allocation, population update,
    // fresh clock, cached-water recomputation or side effects on other houses.
    scratch.houseEvolutionResult=recovery?0:-1;++scratch.houseEvolutionCalls;
    if(goodsPart)++scratch.houseRedraws; // existing strict postlude; goods demands proved zero above
    const auto mirror=[&](OriginalBuildingHouseCapacityInputs& h){
        h.type=candidate.building.type;h.model=candidate.inputs.model;
        h.residents=candidate.inputs.residents;h.free=candidate.inputs.free;
        h.distance=candidate.inputs.distance;h.laborDisabled=candidate.inputs.house->at(55);
    };
    if(candidate.houseCapacity)mirror(*candidate.houseCapacity);
    if(scratch.houses)mirror(scratch.houses->at(slot));
    if(scratch.buildings)scratch.buildings->at(slot)=candidate.building;
    if(scratch.buildingInputs)scratch.buildingInputs->at(slot)=candidate.inputs;
    if(!originalBuildingSynchronizePool(scratch))
        return refuse("observed_dry_downgrade_pool_sync_failed",OriginalBuildingUpdateBlocker::InvalidState);
    NativeHouseEvolutionPlan out;out.direction=recovery?0:-1;out.blocker=OriginalBuildingUpdateBlocker::None;
    out.retainsHouseIdentityAndGeometry=true;
    out.candidate=std::move(candidate);out.candidateContext=std::move(scratch);return out;
}
} // namespace game_lab
