// SPDX-License-Identifier: MIT
#pragma once
#include "native_house_runtime_bridge.hpp"

namespace game_lab {
// View-only: use the same source endpoints and reconstructed movement as the
// runtime, never an adjacency-only approximation or a second road algorithm.
struct NativeEntranceFeedback {
    bool required=false;
    std::optional<std::pair<int,int>> entry,exit;
    std::map<uint64_t,bool> houses;
    std::map<uint64_t,bool> roadHouses;
    std::string error;
};
inline NativeEntranceFeedback queryNativeEntrance(const NativeSessionSeed& seed,const NativeSessionState& state,const NativeStrictTemplate* source){
    NativeEntranceFeedback out;out.required=nativeUsesImmigration(state);
    if(!out.required)return out;
    const auto fail=[&](const std::string& e){out.error=e;out.houses.clear();out.roadHouses.clear();return out;};
    if(!source)return fail("entrance_source_not_available");
    if(source->sourceRevision!=seed.sourceRevision||source->modelRevision!=seed.modelRevision)return fail("entrance_source_changed");
    if(!source->emptyCity.buildingContext||!source->emptyCity.buildingContext->roadEndpoints)return fail("entrance_endpoints_missing");
    const auto ep=*source->emptyCity.buildingContext->roadEndpoints;
    for(const auto value:ep)if(value<0||value>=int(seed.side))return fail("entrance_outside_native_view");
    out.entry=std::make_pair(int(ep[0]),int(ep[1]));out.exit=std::make_pair(int(ep[2]),int(ep[3]));
    const auto bridge=nativeToStrict(seed,state,*source,{state.strictRecords,state.strictContext});
    if(!bridge.city)return fail("entrance_runtime:"+bridge.refusal);
    const auto& context=bridge.city->buildingContext;
    if(!context||!context->poolRoadReachable)return fail("entrance_reachability_missing");
    for(const auto& o:state.objects)if(o.kind==NativeObjectKind::House){
        const auto i=context->poolRoadReachable->find(size_t(o.id));
        if(i==context->poolRoadReachable->end())return fail("entrance_house_access_missing");
        out.houses.emplace(o.id,i->second);
        // The runtime's B7C distance traverses ordinary ground too. A
        // continuous road component is a separate fact, never an arrival gate.
        const auto& f=bridge.city->figures;const auto base=f.cropBase.value_or(0);
        const auto entryCell=size_t(base+ep[0]+228*ep[1]);
        if(f.roadNetworks.size()==51984&&f.roadNetworks[entryCell]&&context->poolHookRoadAccess){
            const auto road=context->poolHookRoadAccess->find(size_t(o.id));
            if(road!=context->poolHookRoadAccess->end()){
                const auto& r=road->second;const int64_t cell=int64_t(base)+r.x+228LL*r.y;
                if(cell<0||cell>=51984)return fail("entrance_selected_road_outside_grid");
                out.roadHouses.emplace(o.id,r.selected&&f.roadNetworks[size_t(cell)]==f.roadNetworks[entryCell]);
            }
        }
    }
    return out;
}
} // namespace game_lab
