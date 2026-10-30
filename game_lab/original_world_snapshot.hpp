// SPDX-License-Identifier: MIT
#pragma once
#include "original_kernel_checkpoint.hpp"
#include "original_world_checkpoint_binding.hpp"
#include "app/city_notifications.hpp"
namespace game_lab {
struct AppCitySessionState {
    OriginalCityClocks cityClocks;
    OriginalFigureRandomHistory randomHistory{{},0,0};
    uint32_t crtRandom=0, wildlifeCounter=0, wildlifePeriod=30, admissionClock=0;
    uint64_t crtRandomCalls=0;
    OriginalAnimationClock animationClock;
    int32_t watchMaximum=0;
    std::array<int32_t,2> inspectorMaximum{};
    std::array<uint8_t,36> storageSearch{},industrySearch{};
};
struct OriginalWorldPreferences {int32_t speed=80,personalSpeed=80;bool soundEffects=true;std::optional<uint8_t> annualDemandResponseMode;};
struct OriginalWorldPolicy {uint32_t schema=1;bool experimental=false;std::string provider="app-documented-inputs-1";OriginalCombinedProcessInputs initial;std::array<uint8_t,39> origins{};OriginalWorldPreferences preferences;};
struct OriginalWorldControllerStatus {uint64_t refusedUpdate=0;size_t decodeFailures=0;std::string refusal,error,premise;};
struct OriginalWorldContinuation {OriginalWorldControllerStatus controller;std::optional<OriginalFigureUpdateContext> initialFigures;bool chained=false;uint32_t admission=0;OriginalAnimationClock clocks;AppCitySessionState session;std::vector<size_t> buildingDraws;std::vector<uint32_t> figureDrawCells;unsigned updates=0;size_t frames=0,draws=0,advances=0,reselections=0,preyDeaths=0,preyCollections=0;};
struct OriginalWorldScene {OriginalBuildingScene shown;std::map<size_t,OriginalBuildingPlacement> selections;std::map<size_t,uint8_t> phases;};
struct OriginalWorldTerrain {OriginalMapTerrain map;std::optional<OriginalTerrainState> state;};
struct OriginalWorldSnapshot {
    uint64_t commitRevision=0;
    OriginalWorldSourceBinding source;
    OriginalWorldPolicy policy;
    OriginalCombinedState city;
    std::map<size_t,OriginalMarketKnownBase> marketBases;
    OriginalWorldTerrain terrain;
    OriginalWorldScene scene;
    OriginalWorldContinuation continuation;
    OriginalWorldIdentityLedger ledger;
    OriginalWorldNotifications notifications;
    std::optional<OriginalMapCamera> camera;
};
inline std::string originalWorldSnapshotIssue(const OriginalWorldSnapshot& s){
    if(s.source.schema!=1||s.source.contract!="original-world-1"||!s.source.legacyRevision||!originalWorldRelative(s.source.source.relative))return "world_snapshot_source";
    if(s.policy.schema!=1||s.policy.provider!="app-documented-inputs-1"||std::any_of(s.policy.origins.begin(),s.policy.origins.end(),[](uint8_t o){return o>3;}))return "world_snapshot_policy";
    const auto speed=[](int32_t v){return v>=1&&v<=100;};if(!speed(s.policy.preferences.speed)||!speed(s.policy.preferences.personalSpeed))return "world_snapshot_preferences";
    if(!s.policy.experimental){const auto closed=[](const OriginalCombinedProcessInputs& p){return !p.eventGate17&&!p.eventGate16&&!p.eventSlots&&!p.managerGate8;};
        if(!closed(s.policy.initial)||!closed(s.city.process)||std::find(s.policy.origins.begin(),s.policy.origins.end(),uint8_t(3))!=s.policy.origins.end())return "world_snapshot_policy_promotion";}
    if(!s.commitRevision||s.terrain.map.flags.size()!=51984||!s.terrain.map.activeSide||s.terrain.map.activeSide>228)return "world_snapshot_dimensions";
    const auto rng=[](const OriginalFigureRandomHistory& h){return h.cursor<100;};
    if(!rng(s.continuation.session.randomHistory)||(s.city.process.randomHistory&&!rng(*s.city.process.randomHistory))||(s.city.figures.services&&s.city.figures.services->randomHistory&&!rng(*s.city.figures.services->randomHistory)))return "world_snapshot_rng_cursor";
    if(s.camera&&(!std::isfinite(s.camera->centerX)||!std::isfinite(s.camera->centerY)||std::abs(s.camera->centerX)>51984||std::abs(s.camera->centerY)>51984||!s.camera->width||s.camera->width>4096||!s.camera->height||s.camera->height>4096||!s.camera->scale||s.camera->scale>16))return "world_snapshot_camera";
    if(s.city.figures.terrain!=s.terrain.map.flags||s.scene.shown.surface.terrain!=s.terrain.map.flags||!s.scene.shown.accepted)return "world_snapshot_scene_terrain";
    OriginalKernelCheckpoint represented;represented.sourceRevision=s.source.legacyRevision;represented.modelRevision=1;represented.city=s.city;represented.marketBases=s.marketBases;
    if(auto e=originalKernelCheckpointIssue(represented);!e.empty())return e;
    std::string error;if(!OriginalKernelIdentities::validPersistent(s.ledger,s.city.figures.figures,error))return error;
    if(!s.city.scenarioMessages||!AppCityNotifications::validPersistent(s.notifications,*s.city.scenarioMessages))return "world_snapshot_notifications";
    if(s.continuation.chained==s.continuation.initialFigures.has_value())return "world_snapshot_initial_role";
    if(s.continuation.initialFigures){const auto& f=*s.continuation.initialFigures;if(auto e=originalKernelPoolIssue(f);!e.empty())return e;
        if(f.terrain!=s.city.figures.terrain||f.cellHeads!=s.city.figures.cellHeads)return "world_snapshot_initial_pool_alias";}
    if(s.continuation.buildingDraws.size()>40000||s.continuation.figureDrawCells.size()>4*51984||s.scene.selections.size()>3999||s.scene.phases.size()>3999)return "world_snapshot_draw_bounds";
    for(auto cell:s.continuation.figureDrawCells)if(cell>=51984)return "world_snapshot_draw_cell";
    for(auto slot:s.continuation.buildingDraws)if(!slot||slot>=4000)return "world_snapshot_draw_building";
    if(s.city.process.cityClocks&&s.city.process.cityClocks->value!=s.continuation.session.cityClocks.value)return "world_snapshot_clock_alias";
    if(s.city.process.randomHistory){const auto& a=*s.city.process.randomHistory;const auto& b=s.continuation.session.randomHistory;if(a.ring!=b.ring||a.cursor!=b.cursor||a.previous7!=b.previous7)return "world_snapshot_rng_alias";}
    if(s.city.figures.crtRandom&&*s.city.figures.crtRandom!=s.continuation.session.crtRandom)return "world_snapshot_crt_alias";
    return {};
}
}
