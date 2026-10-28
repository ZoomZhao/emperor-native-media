// SPDX-License-Identifier: MIT
#pragma once
#include "native_session.hpp"
#include "native_house_phase_runtime.hpp"
namespace game_lab {
inline std::optional<NativeWellReturnPolicy> nativeSavedWellReturnPolicy(const NativeSessionState& s,std::string& error){
    bool hold=false,observed=false,started=false;
    for(const auto& p:s.runtimePolicies){
        if(p==nativeWellReturnHoldTag)hold=true;
        else if(p==nativeWellReturnObservedTag)observed=true;
        else if(p==nativeWellReturnTransitionTag)started=true;
        else if(p.rfind("native_well_return_policy_",0)==0){error="well_return_policy_version_unsupported";return std::nullopt;}
    }
    if(hold&&observed){error="well_return_policy_conflict";return std::nullopt;}
    if(started&&!observed){error="well_return_policy_missing_for_saved_transition";return std::nullopt;}
    // Untagged earlier v8 drafts retain their old Hold behavior.
    error.clear();return observed?NativeWellReturnPolicy::ObservedHomeRoad:NativeWellReturnPolicy::Hold;
}

// Capabilities describe the current saved mode; they never enable another
// phase, figure family or source. Source admission stays empty-city only.
struct NativeHouseRuntimeContract {
    NativeWellReturnPolicy wellReturnPolicy=NativeWellReturnPolicy::Hold;
    bool observedDry=false,nonemptyRehousing=false,observedRewatering=false,immigration=false,cityContext=false,inspection=false;
    unsigned wireVersion=1;
    bool acceptsPopulatedOriginalSource() const {return false;}
    bool supportsBuildingPhase(uint32_t phase) const {
        return (inspection&&(phase==43||phase==44||phase==45))||native_phase_detail::supportedPhase(phase)||(phase==24&&observedDry)||(phase==23&&immigration)||(cityContext&&native_phase_detail::cityContextPhase(phase));
    }
    bool supportsLiveFigure(uint16_t type) const {return (inspection&&type==39)||type==28||(type==13&&observedDry)||(type==11&&immigration);}
    std::set<std::pair<uint16_t,int16_t>> houseGrades() const {
        std::set<std::pair<uint16_t,int16_t>> out;
        for(uint16_t type=2;type<18;++type)if(auto model=nativeHouseModelForType(type,8))out.emplace(type,int16_t(*model));
        return out;
    }
    NativePhaseStepInputs phaseInputs(const NativeSessionState& state,
        const std::map<uint32_t,const SG3Archive*>* archives=nullptr) const {
        NativePhaseStepInputs out;out.archives=archives;out.wrap=NativePhaseWrapPolicy::WrapPreserveMonthPart;
        out.wellReturnPolicy=wellReturnPolicy;out.retainNativeHunterEconomy=true;out.retainNativeHouseFootprints=true;
        out.observedDryDowngrade=observedDry;out.nonemptyMigrantRehousing=nonemptyRehousing;out.observedRewatering=observedRewatering;
        out.immigration=immigration;out.cityContextContract=cityContext;out.inspection=inspection;out.admittedHouseGrades=houseGrades();if(state.observedDry)out.pendingExcess=state.observedDry->pendingExcess;
        return out;
    }
};
inline std::optional<NativeHouseRuntimeContract> resolveNativeHouseRuntimeContract(const NativeSessionState& state,std::string& error){
    error.clear();if(state.rulesVersion!=8){error="runtime_requires_v8";return std::nullopt;}
    const auto has=[&](const char* tag){return std::find(state.runtimePolicies.begin(),state.runtimePolicies.end(),tag)!=state.runtimePolicies.end();};
    if(has(nativeObservedDryTag)!=state.observedDry.has_value()){error="observed_dry_policy_state_mismatch";return std::nullopt;}
    NativeHouseRuntimeContract out;out.observedDry=state.observedDry.has_value();out.nonemptyRehousing=nativeUsesNonemptyMigrantRehousing(state);out.observedRewatering=nativeUsesObservedRewatering(state);out.immigration=nativeUsesImmigration(state);
    if(out.nonemptyRehousing&&!out.observedDry){error="nonempty_rehousing_requires_observed_dry_state";return std::nullopt;}
    if(out.observedRewatering&&(!out.observedDry||!out.nonemptyRehousing)){error="water_recovery_requires_r3_runtime";return std::nullopt;}
    for(const auto& p:state.runtimePolicies){
        if(p.rfind("native_city_context_contract_",0)==0&&p!=nativeCityContextTag){error="city_context_policy_unsupported";return std::nullopt;}
        if(p.rfind("native_migrant_nonempty_rehousing_policy_",0)==0&&p!=nativeMigrantNonemptyTag){error="nonempty_rehousing_policy_unsupported";return std::nullopt;}
        if(p.rfind("native_house_water_only_recovery_policy_",0)==0&&p!=nativeObservedRewaterTag){error="water_recovery_policy_unsupported";return std::nullopt;}
        if(p.rfind("native_house_observed_dry_",0)==0&&p!=nativeObservedDryTag){error="observed_dry_policy_unsupported";return std::nullopt;}
    }
    if(out.immigration!=state.populationRuntime.has_value()||(out.immigration&&!out.observedRewatering)){error="immigration_policy_state_mismatch";return std::nullopt;}
    out.cityContext=nativeUsesCityContext(state);out.inspection=nativeUsesInspection(state);
    if(out.inspection!=state.inspection.has_value()||(out.inspection&&!out.cityContext)){error="inspection_policy_state_mismatch";return std::nullopt;}
    if(out.cityContext&&std::count(state.runtimePolicies.begin(),state.runtimePolicies.end(),nativeCityContextTag)!=1){error="city_context_policy_duplicate";return std::nullopt;}
    if(out.cityContext!=state.cityContext.has_value()||(out.cityContext&&!out.immigration)){error="city_context_policy_state_mismatch";return std::nullopt;}
    const auto well=nativeSavedWellReturnPolicy(state,error);if(!well)return std::nullopt;out.wellReturnPolicy=*well;
    out.wireVersion=nativeHouseRuntimeWireVersion(state);return out;
}
} // namespace game_lab
