// SPDX-License-Identifier: MIT
#pragma once
#include "original_scene_timeline.hpp"
#include "original_market_base_codec.hpp"
namespace game_lab {
enum class OriginalKernelLoadKind { OriginalSAV, CompleteValueCheckpoint };
// Complete represented world value, not a new original-SAV disk writer. Resource
// archives are rebound by the owner; unknown process/FSA inputs stay unknown.
struct OriginalKernelCheckpoint {
    unsigned version=1;
    uint64_t sourceRevision=0,modelRevision=0;
    OriginalCombinedState city;
    OriginalKernelIdentities identities;
    uint32_t admissionClock=0;
    OriginalAnimationClock animationClock;
    std::map<size_t,uint8_t> buildingPhases;
    std::map<size_t,OriginalMarketKnownBase> marketBases;
};
inline std::string originalKernelCheckpointIssue(const OriginalKernelCheckpoint& s){
    if(s.version!=1||!s.sourceRevision||!s.modelRevision)return "kernel_checkpoint_header";
    if(auto e=originalKernelAuthorityIssue(s.city);!e.empty())return e;
    for(const auto& [slot,f]:s.city.figures.figures)if(f.alive&&(f.type==24||f.type==25)){
        const auto b=s.marketBases.find(slot);if(b==s.marketBases.end())return "kernel_checkpoint_market_base_missing";
        const auto& raw=b->second;if(raw.sourceRevision!=s.sourceRevision||raw.figure.slot!=slot)return "kernel_checkpoint_market_source";
        if(auto e=originalMarketKnownBaseIssue(raw);!e.empty())return e;
        if(!originalMarketKnownBaseMatches(raw,f))return "kernel_checkpoint_market_mirror";
        // Reject proved reference corruption, retain unqualified transition states.
        for(int ref:{int(f.follower),int(f.leaderLink)})if(ref<0||ref>=2000)return "kernel_checkpoint_market_actor_reference";
        for(int ref:{int(f.owner),int(f.delivery)})if(ref<0||ref>=4000)return "kernel_checkpoint_market_building_reference";
        if(evidenceRouteCheck(f,s.city.figures.services?&*s.city.figures.services:nullptr).referenceValid==false)return "kernel_checkpoint_market_route";
        if(f.alive==1&&f.type==24&&f.follower){const auto& follower=s.city.figures.figures.at(size_t(f.follower));
            if(follower.alive==1&&(follower.type!=25||follower.leaderLink!=int(slot)||follower.owner!=f.owner))return "kernel_checkpoint_market_pair";}
        if(f.alive==1&&f.type==25&&f.leaderLink){const auto& leader=s.city.figures.figures.at(size_t(f.leaderLink));
            if(leader.alive==1&&(leader.type!=24||leader.follower!=int(slot)||leader.owner!=f.owner))return "kernel_checkpoint_market_pair";}

    }
    for(const auto& [slot,b]:s.marketBases){(void)b;const auto f=s.city.figures.figures.find(slot);if(f==s.city.figures.figures.end()||!f->second.alive||(f->second.type!=24&&f->second.type!=25))return "kernel_checkpoint_extra_market_base";}
    for(const auto& [slot,phase]:s.buildingPhases){(void)phase;if(!s.city.buildings.count(slot))return "kernel_checkpoint_animation_identity";}
    return {};
}
inline std::optional<OriginalKernelCheckpoint> prepareOriginalKernelLoad(OriginalKernelLoadKind kind,
    const OriginalKernelCheckpoint& candidate,uint64_t source,uint64_t model,std::string& error){
    if(candidate.sourceRevision!=source||candidate.modelRevision!=model){error="kernel_checkpoint_source_changed";return std::nullopt;}
    error=originalKernelCheckpointIssue(candidate);if(!error.empty())return std::nullopt;
    if(kind==OriginalKernelLoadKind::CompleteValueCheckpoint&&!candidate.identities.consistent(candidate.city.figures.figures)){
        error="kernel_checkpoint_identity_ledger";return std::nullopt;}
    auto out=candidate;
    const auto identities=kind==OriginalKernelLoadKind::OriginalSAV?OriginalKernelIdentities::loaded(out.city.figures.figures,error):out.identities.reloaded(error);
    if(!identities)return std::nullopt;
    out.identities=*identities;error.clear();return out;
}
} // namespace game_lab
