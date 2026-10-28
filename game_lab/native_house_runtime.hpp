// SPDX-License-Identifier: MIT
#pragma once
#include "native_house_runtime_bridge.hpp"
#include "native_house_runtime_contract.hpp"
#include <memory>
namespace game_lab {
struct NativeHouseRuntimeResources {
    NativeStrictTemplate strictTemplate;
    std::map<uint32_t,SG3Archive> archives;
    std::map<uint32_t,const SG3Archive*> archiveRefs() const {
        std::map<uint32_t,const SG3Archive*> out;for(const auto& e:archives)out.emplace(e.first,&e.second);return out;
    }
};
inline NativeStrictPersisted nativeStrictPersisted(const NativeSessionState& state){return {state.strictRecords,state.strictContext};}
inline void nativeRuntimePolicy(NativeSessionState& s,const std::string& p){
    if(std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),p)==s.runtimePolicies.end())s.runtimePolicies.push_back(p);
}
// Resource-aware, read-only admission for loading an existing v8 state.
// Pure wire decoding remains independent of templates/archives. This proves
// current representation/graph consistency, not all future unsupported paths.
inline std::string validateNativeHouseRuntimeReady(const NativeSessionSeed& seed,const NativeSessionState& state,const NativeHouseRuntimeResources& resources){
    if(auto e=validateNativeSession(seed,state);!e.empty())return e;
    std::string error;const auto contract=resolveNativeHouseRuntimeContract(state,error);if(!contract)return error;
    const auto input=nativeToStrict(seed,state,resources.strictTemplate,nativeStrictPersisted(state));
    if(!input.city)return input.refusal;
    if(auto e=native_phase_detail::checkRetainedIsolation(*input.city,input.isolation,contract->observedDry);!e.empty())return e;
    const auto output=strictToNative(seed,state,*input.city,input.isolation,resources.strictTemplate);
    if(!output.state)return output.refusal;
    if(encodeNativeSession(seed,state).empty())return "runtime_save_budget_exceeded";
    return {};
}
inline std::optional<NativeSessionState> startNativeHouseRuntime(const NativeSessionSeed& seed,const NativeHouseRuntimeResources& resources,std::string& error,NativeWellReturnPolicy returnPolicy=NativeWellReturnPolicy::Hold,bool observedDry=false,bool nonemptyRehousing=false,bool observedRewater=false,bool immigration=false,bool cityContract=false,bool inspection=false){
    error="runtime_source_clock_unknown";if(!seed.sourceClock)return std::nullopt;
    if(returnPolicy!=NativeWellReturnPolicy::Hold&&returnPolicy!=NativeWellReturnPolicy::ObservedHomeRoad){error="well_return_policy_unsupported";return std::nullopt;}
    if(nonemptyRehousing&&!observedDry){error="nonempty_rehousing_requires_observed_dry_state";return std::nullopt;}
    if(observedRewater&&(!observedDry||!nonemptyRehousing)){error="water_recovery_requires_r3_runtime";return std::nullopt;}
    if(immigration&&!observedRewater){error="immigration_requires_r4_runtime";return std::nullopt;}
    if(cityContract&&!immigration){error="city_context_requires_immigration_runtime";return std::nullopt;}
    if(inspection&&!cityContract){error="inspection_requires_city_context";return std::nullopt;}
    auto s=startNativeSession(seed,8);const auto& clock=*seed.sourceClock;
    if(observedDry){s.observedDry.emplace();nativeRuntimePolicy(s,nativeObservedDryTag);}
    if(nonemptyRehousing)nativeRuntimePolicy(s,nativeMigrantNonemptyTag);
    if(observedRewater)nativeRuntimePolicy(s,nativeObservedRewaterTag);
    nativeRuntimePolicy(s,returnPolicy==NativeWellReturnPolicy::ObservedHomeRoad?nativeWellReturnObservedTag:nativeWellReturnHoldTag);
    if(clock.phase>50)return std::nullopt;
    s.houseSchedule=NativeHouseSchedule{uint8_t(clock.phase),clock.cycle,clock.monthPart,clock.month,NativeHouseClockOrigin::SourceSaved,clock.cycleWrapTailSkipped};
    const auto& tpl=resources.strictTemplate;const auto& f=tpl.emptyCity.figures;
    if(!f.services||!f.speechCounters||!f.speechCounts||!tpl.emptyCity.buildingContext){error="runtime_template_incomplete";return std::nullopt;}
    auto& fr=s.figureRuntime.emplace();fr.cellHeads=f.cellHeads;fr.highestFigureSlot=f.highestLiveSlot;
    for(size_t slot:f.freeSlots)fr.freeFigureSlots.push_back(uint16_t(slot));
    const auto& sv=*f.services;auto& dv=fr.services;
    for(size_t n=0;n<100;++n)dv.formationImpacts[n]={sv.formationImpacts[n].alert,sv.formationImpacts[n].source};
    dv.routeOwners=sv.routeOwners;dv.directions=sv.directions;dv.randomFirst=sv.randomFirst;dv.randomSecond=sv.randomSecond;
    if(sv.randomHistory){dv.randomRing=sv.randomHistory->ring;dv.randomCursor=sv.randomHistory->cursor;dv.randomPrevious=sv.randomHistory->previous7;}
    dv.fireCount=sv.fireCount;dv.criminalCount=sv.criminalCount;dv.allocationSerial=sv.allocationSerial;dv.serviceGrid=sv.serviceGrid;
    fr.speechCounters=*f.speechCounters;fr.speechCounts=*f.speechCounts;
    s.buildingHookCountdown=tpl.emptyCity.buildingContext->buildingHookCountdown;
    if(immigration){const auto& bc=*tpl.emptyCity.buildingContext;
        if(!bc.labor||!bc.cityEmployment||!sv.population||sv.population->total!=0){error="immigration_empty_source_labor_required";return std::nullopt;}
        s.populationRuntime.emplace();s.populationRuntime->labor=*bc.labor;s.populationRuntime->labor.formationLabor.fill(0);s.populationRuntime->employment=*bc.cityEmployment;nativeRuntimePolicy(s,nativeImmigrationTag);nativeRuntimePolicy(s,"native_empty_formations_zero_init_v1");nativeRuntimePolicy(s,"native_v6_staffing_after_strict_arrival_v1");
    }
    NativeStrictContextState c;c.population=sv.population;
    if(observedDry){c.nativePopulation=0;c.laborCounters=std::make_pair(0,0);}c.desirability=tpl.emptyCity.buildingContext->desirability;
    s.strictContext=encodeNativeStrictContext(c);
    if(cityContract){
        // The SAV API does not restore these process-only notification cursors.
        // A newly constructed C1 city names its native initialization; old B
        // saves are never backfilled or admitted to phase25 by this path.
        auto& jobs=s.populationRuntime->employment;bool noticeInit=false;
        if(!jobs.shortageNotice){jobs.shortageNotice=true;noticeInit=true;}
        if(!jobs.unemploymentNotice){jobs.unemploymentNotice=true;noticeInit=true;}
        if(noticeInit)nativeRuntimePolicy(s,"native_city_employment_notice_native_init_v1");
        auto city=captureNativeCityContext(*tpl.emptyCity.buildingContext);city.workers.clear();city.goods=f.goods?f.goods:tpl.cityGoodsSource;
        if(!city.goods){error="city_goods_source_required";return std::nullopt;}
        // Native buildings form a new, empty goods pool. Retain source policy
        // and unavailable/priority inputs, not discarded source stock credits.
        city.goods->warehouse.fill(0);city.goods->trade5.fill(0);city.goods->trade7.fill(0);
        city.goods->savedPhase=clock.phase;
        const auto payload=encodeNativeCityContext(city,error);if(!payload)return std::nullopt;
        s.cityContext=*payload;nativeRuntimePolicy(s,nativeCityContextTag);
        nativeRuntimePolicy(s,"native_city_goods_empty_owned_pool_v1");
        s.runtimePolicies.erase(std::remove(s.runtimePolicies.begin(),s.runtimePolicies.end(),"native_v6_staffing_after_strict_arrival_v1"),s.runtimePolicies.end());
    }
    if(inspection){s.inspection.emplace();nativeRuntimePolicy(s,nativeInspectionTag);nativeRuntimePolicy(s,"native_inspector_process_state_zero_init_v1");}
    for(const char* p:{"native_empty_well_pool_zero_init","native_random_history_zero_init","native_economy_v6_retained","native_first_arrival_type3","native_one_phase_per_tick","native_cycle_tail_preserve_month_part"})nativeRuntimePolicy(s,p);
    if(auto invalid=validateNativeSession(seed,s);!invalid.empty()){error=invalid;return std::nullopt;}
    const auto converted=nativeToStrict(seed,s,tpl,nativeStrictPersisted(s));
    if(!converted.city){error=converted.refusal;return std::nullopt;}
    const auto projected=strictToNative(seed,s,*converted.city,converted.isolation,tpl);
    if(!projected.state){error=projected.refusal;return std::nullopt;}
    error.clear();return projected.state;
}
// One owned candidate includes native ledger, all strict services/figures,
// schedule and rendering inputs. No failure mutates the caller's state.
inline NativeSessionResult transactNativeHouseRuntime(const NativeSessionSeed& seed,NativeSessionState& state,const NativeSessionCommand& cmd,const NativeHouseRuntimeResources& resources){
    if(state.rulesVersion!=8)return transactNativeSession(seed,state,cmd);
    if(auto e=validateNativeSession(seed,state);!e.empty())return {false,e};
    std::string policyError;const auto contract=resolveNativeHouseRuntimeContract(state,policyError);
    if(!contract)return {false,policyError};
    if(cmd.expectedRevision!=state.revision)return {false,"stale_command"};
    auto next=state;
    if(cmd.action==NativeSessionAction::Tick){
        if(next.paused)return {false,"paused"};if(next.ticks>=1000000000ULL)return {false,"tick_limit"};
        if(next.revision>=1000000000000ULL)return {false,"revision_exhausted"};
        if(auto e=advanceNativeSessionLedger(seed,state,next);!e.empty())return {false,e};
        ++next.revision;
        // Native arrivals own resident count; keep its strict image projection
        // coherent before conversion, without inventing a house grade.
        for(auto& o:next.objects)if(o.house)o.house->imageWord20=int16_t(o.residents);
        const auto input=nativeToStrict(seed,next,resources.strictTemplate,nativeStrictPersisted(next));
        if(!input.city)return {false,input.refusal};
        const auto refs=resources.archiveRefs();const auto inputs=contract->phaseInputs(next,&refs);
        const auto phase=stepNativeHousePhase(*input.city,input.isolation,inputs);
        if(!phase.candidate)return {false,phase.refusal};
        const auto output=strictToNative(seed,next,*phase.candidate,phase.isolation,resources.strictTemplate,&phase.departedResidents,&phase.arrivedResidents,&phase.immigrantOffers);
        if(!output.state)return {false,output.refusal};
        next=*output.state;
        if(phase.executedPhase==50)next.houseSchedule->cycleWrapTailSkipped=true;
        for(const auto& e:phase.events)if(e.kind=="native_well_return_observed_home_road")nativeRuntimePolicy(next,nativeWellReturnTransitionTag);
        for(const auto& e:phase.events)if(e.kind=="native_inspector_secondary_cancelled_on_downgrade_v1")nativeRuntimePolicy(next,e.kind);
        for(const auto& p:input.policies)nativeRuntimePolicy(next,p);
    }else{
        // Explicit normal deletion: remove this Well's own worker through the
        // existing reciprocal route/cell cleanup before deleting its owner.
        if(cmd.action==NativeSessionAction::Demolish){
            if(nativeUsesNonemptyMigrantRehousing(next)){
                // Removing a constructed target cancels its reservation, not
                // the already-departed people. Reuse the same owned candidate.
                const auto strict=nativeToStrict(seed,next,resources.strictTemplate,nativeStrictPersisted(next));if(!strict.city)return {false,strict.refusal};
                auto city=*strict.city;auto isolation=strict.isolation;bool changed=false;
                for(const auto& [slot,w]:strict.city->figures.figures)if(nativeUsesImmigration(next)&&w.alive==1&&w.type==11&&w.home>0&&uint64_t(w.home)==cmd.objectId){
                    auto dead=w;dead.alive=2;if(!originalFigureRemoveArrivedWorker(dead,city.figures))return {false,"immigrant_target_cleanup_refused"};
                    city.figures.buildings->at(size_t(w.home)).worker=0;isolation.heldWalkers.erase(slot);changed=true;
                }
                if(changed)if(auto e=originalCombinedSyncBuildings(city);!e.empty())return {false,e};
                for(const auto& [slot,w]:strict.city->figures.figures)if(w.alive==1&&w.type==13&&w.home>0&&uint64_t(w.home)==cmd.objectId){
                    if(auto e=native_phase_detail::cancelMigrantReservation(city,slot,nativeUsesImmigration(next));!e.empty())return {false,e};
                    isolation.heldWalkers.erase(slot);changed=true;
                }
                if(changed){const auto p=strictToNative(seed,next,city,isolation,resources.strictTemplate);if(!p.state)return {false,p.refusal};next=*p.state;
                    nativeRuntimePolicy(next,"native_migrant_target_removed_reservation_released_v1");}
            }
            const auto found=std::find_if(next.objects.begin(),next.objects.end(),[&](const auto& o){return o.id==cmd.objectId;});
            if(found!=next.objects.end()&&found->well&&found->well->primaryFigure){
                const auto strict=nativeToStrict(seed,next,resources.strictTemplate,nativeStrictPersisted(next));
                if(!strict.city)return {false,strict.refusal};auto city=*strict.city;
                const size_t slot=size_t(found->well->primaryFigure);auto walker=city.figures.figures.at(slot);walker.alive=2;
                if(!originalFigureRemoveArrivedWorker(walker,city.figures))return {false,"well_walker_demolition_cleanup_refused"};
                auto isolation=strict.isolation;isolation.heldWalkers.erase(slot);
                if(auto e=originalCombinedSyncBuildings(city);!e.empty())return {false,e};
                const auto projected=strictToNative(seed,next,city,isolation,resources.strictTemplate);
                if(!projected.state)return {false,projected.refusal};next=*projected.state;
            }
        }
        if(next.inspection&&cmd.action==NativeSessionAction::Demolish){
            const auto object=std::find_if(next.objects.begin(),next.objects.end(),[&](const auto& o){return o.id==cmd.objectId;});
            if(object!=next.objects.end()){
                const auto input=nativeToStrict(seed,next,resources.strictTemplate,nativeStrictPersisted(next));if(!input.city)return {false,input.refusal};auto city=*input.city;bool changed=false;
                const bool roadEdit=object->kind==NativeObjectKind::Road;
                for(const auto& [slot,actor]:input.city->figures.figures)if(actor.alive&&actor.type==39){
                    // Explicit native edit policy: a road deletion cancels
                    // patrols before changing the graph. Keep risk/history,
                    // clear owned routes/links and let phase31 redispatch.
                    // This does not claim original live-road-edit semantics.
                    if(object->kind!=NativeObjectKind::Road&&(object->kind!=NativeObjectKind::Inspector||uint64_t(actor.owner)!=object->id))continue;
                    auto dead=actor;dead.alive=2;if(!originalFigureRemoveArrivedWorker(dead,city.figures))return {false,"inspection_demolition_cleanup_refused"};changed=true;
                }
                if(changed){if(auto e=originalCombinedSyncBuildings(city);!e.empty())return {false,e};const auto p=strictToNative(seed,next,city,input.isolation,resources.strictTemplate);if(!p.state)return {false,p.refusal};next=*p.state;
                    if(roadEdit)nativeRuntimePolicy(next,"native_inspector_road_edit_cancels_patrols_v1");}
            }
        }
        const auto result=transactNativeSession(seed,next,cmd);if(!result.committed)return result;
        if(next.inspection&&cmd.action==NativeSessionAction::Demolish)next.inspection->visits.erase(size_t(cmd.objectId));
        if(nativeUsesCityContext(next)&&(cmd.action==NativeSessionAction::HuntingPause||cmd.action==NativeSessionAction::HuntingResume)){
            for(const auto& o:next.objects)if(o.kind==NativeObjectKind::Hunter){
                std::string error;const auto old=next.strictRecords.find(o.id);if(old==next.strictRecords.end())continue;
                auto record=decodeNativeStrictRecord(old->second,error);if(!record)return {false,error};
                record->enabled=next.huntingEnabled?1:0;next.strictRecords[o.id]=encodeNativeStrictRecord(*record);
            }
        }
        if(cmd.action==NativeSessionAction::Demolish&&next.populationRuntime){
            const auto removed=std::find_if(state.objects.begin(),state.objects.end(),[&](const auto& o){return o.id==cmd.objectId;});
            if(removed==state.objects.end())return {false,"demolition_population_source_missing"};
            std::string error;auto context=next.strictContext?decodeNativeStrictContext(*next.strictContext,error):std::nullopt;
            if(!context||!context->nativePopulation||!context->population||*context->nativePopulation!=nativePopulation(next)+removed->residents||context->population->total<int64_t(removed->residents))return {false,"demolition_population_context_mismatch"};
            *context->nativePopulation-=removed->residents;context->population->total-=int32_t(removed->residents);next.strictContext=encodeNativeStrictContext(*context);
        }
        const auto input=nativeToStrict(seed,next,resources.strictTemplate,nativeStrictPersisted(next));if(!input.city)return {false,input.refusal};
        const auto output=strictToNative(seed,next,*input.city,input.isolation,resources.strictTemplate);if(!output.state)return {false,output.refusal};next=*output.state;
        for(const auto& p:input.policies)nativeRuntimePolicy(next,p);
    }
    if(auto e=validateNativeSession(seed,next);!e.empty())return {false,e};
    if(encodeNativeSession(seed,next).empty())return {false,"runtime_save_budget_exceeded"};
    state=std::move(next);return {true,{}};
}
// Real v8 command validation on a discarded candidate. Never advances a
// tick, renders, writes a save, or alters the caller's session/resources.
inline NativeSessionResult previewNativeHouseCommand(const NativeSessionSeed& seed,const NativeSessionState& state,const NativeSessionCommand& command,const NativeHouseRuntimeResources& resources){
    switch(command.action){case NativeSessionAction::Road:case NativeSessionAction::House:case NativeSessionAction::Well:case NativeSessionAction::Hunter:case NativeSessionAction::Inspector:case NativeSessionAction::RoadPath:case NativeSessionAction::Demolish:break;
    default:return {false,"no_construction_tool"};}
    auto copy=state;return transactNativeHouseRuntime(seed,copy,command,resources);
}
// Shared App preview routing. Legacy modes keep their original command
// implementation; v8 can never silently fall back when resources are absent.
inline NativeSessionResult previewNativeSessionCommand(const NativeSessionSeed& seed,const NativeSessionState& state,const NativeSessionCommand& command,const NativeHouseRuntimeResources* resources=nullptr){
    switch(command.action){case NativeSessionAction::Road:case NativeSessionAction::House:case NativeSessionAction::Well:case NativeSessionAction::Hunter:case NativeSessionAction::Inspector:case NativeSessionAction::RoadPath:case NativeSessionAction::Demolish:break;
    default:return {false,"no_construction_tool"};}
    if(state.rulesVersion==8){if(!resources)return {false,"runtime_source_unavailable"};return previewNativeHouseCommand(seed,state,command,*resources);}
    auto copy=state;return transactNativeSession(seed,copy,command);
}
} // namespace game_lab
