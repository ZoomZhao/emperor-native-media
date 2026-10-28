// SPDX-License-Identifier: MIT
#pragma once
#include "native_inspector_feedback.hpp"

namespace game_lab {
// Committed facts only. Never executes scheduling, routing, RNG or an arrival.
struct NativeResidentJourney {
    size_t slot=0;uint8_t type=0,action=0;uint32_t people=0;
    std::string held;
};
struct NativeHouseFlow {
    uint32_t capacity=0;std::optional<NativeResidentJourney> incoming;
};
struct NativeWellFlow {
    std::optional<NativeResidentJourney> walker;
    uint8_t dispatchCounter=0;
};
struct NativePopulationFeedback {
    bool scheduled=false;uint64_t incomingPeople=0,waitingRehousing=0;
    std::map<uint64_t,NativeHouseFlow> houses;
    std::map<uint64_t,NativeWellFlow> wells;
    std::string error;
};
inline NativePopulationFeedback queryNativePopulationFeedback(const NativeSessionSeed& seed,const NativeSessionState& state){
    NativePopulationFeedback out;out.scheduled=nativeUsesImmigration(state);
    if(state.rulesVersion!=8)return out;
    const auto fail=[&](const std::string& e){out.error=e;return out;};
    if(!state.figureRuntime)return fail("population_feedback_runtime_missing");
    std::map<uint64_t,const NativeSessionObject*> objects;
    for(const auto& o:state.objects){objects.emplace(o.id,&o);
        if(o.kind==NativeObjectKind::House&&out.scheduled){const auto cap=nativeHouseCapacity(seed,state,o);
            if(!cap)return fail("population_feedback_capacity_missing");out.houses.emplace(o.id,NativeHouseFlow{*cap,{}});}
        if(o.kind==NativeObjectKind::Well)out.wells.emplace(o.id,NativeWellFlow{{},o.well?o.well->spawnCounter:uint8_t(0)});
    }
    std::set<size_t> slots;
    const auto held=[&](size_t slot){const auto i=state.heldWalkerReasons.find(slot);return i==state.heldWalkerReasons.end()?std::string():i->second;};
    const auto arrive=[&](const OriginalFigure& w)->std::string{
        if(!slots.insert(w.slot).second)return "population_feedback_duplicate_slot";
        if(w.type==13&&!w.home){out.waitingRehousing+=w.employmentAmount;return {};}
        if(!out.scheduled)return {};
        if(w.type==13&&w.action!=7&&w.action!=8)return "population_feedback_migrant_action";
        const auto h=out.houses.find(uint64_t(w.home));
        if(w.home<=0||h==out.houses.end())return "population_feedback_target_missing";
        if(h->second.incoming)return "population_feedback_duplicate_home";
        h->second.incoming=NativeResidentJourney{w.slot,w.type,w.action,w.employmentAmount,held(w.slot)};
        out.incomingPeople+=w.employmentAmount;return {};
    };
    for(const auto& payload:state.figureRuntime->immigrantPayloads){std::string e;const auto w=decodeNativeImmigrant(payload,e);
        if(!w)return fail("population_feedback_immigrant:"+e);e=arrive(*w);if(!e.empty())return fail(e);}
    for(const auto& payload:state.figureRuntime->migrantPayloads){std::string e;const auto w=decodeNativeMigrant(payload,e);
        if(!w)return fail("population_feedback_migrant:"+e);e=arrive(*w);if(!e.empty())return fail(e);}
    for(const auto& payload:state.figureRuntime->wellWalkerPayloads){std::string e;const auto w=decodeNativeWellWalker(payload,e);
        if(!w)return fail("population_feedback_well:"+e);
        if(!slots.insert(w->slot).second)return fail("population_feedback_duplicate_slot");
        const auto o=objects.find(uint64_t(w->owner));const auto h=out.wells.find(uint64_t(w->owner));
        if(w->owner<=0||o==objects.end()||h==out.wells.end()||!o->second->well||o->second->well->primaryFigure!=int(w->slot)||h->second.walker)
            return fail("population_feedback_well_reference");
        h->second.walker=NativeResidentJourney{w->slot,w->type,w->action,0,held(w->slot)};
    }
    if(state.inspection)for(const auto& payload:state.inspection->payloads){std::string e;const auto w=decodeNativeInspector(payload,e);
        if(!w)return fail("population_feedback_inspector:"+e);if(!slots.insert(w->slot).second)return fail("population_feedback_duplicate_slot");}
    if(out.scheduled)for(const auto& [id,flow]:out.houses){
        const auto object=objects.at(id);const auto rec=state.strictRecords.find(id);std::string e;
        if(rec==state.strictRecords.end()||!object->house||!object->house->houseWater)return fail("population_feedback_house_unknown");
        const auto record=decodeNativeStrictRecord(rec->second,e);
        if(!record||record->identity!=id||record->model!=int(object->house->model)||record->residents!=int(object->residents)||record->houseWater!=*object->house->houseWater)
            return fail("population_feedback_house_mirror");
        if(record->migrant!=(flow.incoming?int(flow.incoming->slot):0))return fail("population_feedback_reservation_mismatch");
    }
    for(const auto& [id,flow]:out.wells){const auto object=objects.at(id);std::string e;
        const auto rec=state.strictRecords.find(id);
        const auto record=rec==state.strictRecords.end()?std::optional<OriginalBuildingUpdateInputs>{}:decodeNativeStrictRecord(rec->second,e);
        if(!object->well||!record||record->identity!=id||record->primaryFigure!=object->well->primaryFigure)return fail("population_feedback_well_mirror");
        if(object->well->primaryFigure>0&&!flow.walker)return fail("population_feedback_well_missing");
    }
    return out;
}
} // namespace game_lab
