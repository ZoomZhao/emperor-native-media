// SPDX-License-Identifier: MIT
#pragma once
#include "original_figure.hpp"

namespace game_lab {
// Observational reference contract only, not a complete movement/FSA admission.
// Roam increments the cursor without an allocated route; exhausted-route
// release preserves cursor/length. Never normalize or write observed fields.
struct EvidenceRouteCheck {
    std::optional<bool> referenceValid;
    const char* state="route_fields_unavailable";
    bool cursorIsArrayIndex=false;
    std::optional<bool> reciprocalOwner;
};
inline EvidenceRouteCheck evidenceRouteCheck(const OriginalFigure& f,
    const OriginalFigureSavedServices* services){
    if(f.route==0){
        // CellHeight can take a terrain-dependent lookahead when cursor<len.
        // Without that context do not call it corrupt OR certify movement.
        if(f.routeCursor<f.routeLength)return {std::nullopt,"no_route_lookahead_context_unqualified",false,std::nullopt};
        return {true,(f.type==23||f.type==28)&&f.action==1?"roaming_without_route":"no_active_route_retained_counters",
            false,std::nullopt};}
    if(f.route<0)return {std::nullopt,"negative_route_unqualified",false,std::nullopt};
    if(f.route>=1000)return {false,"route_index_out_of_bounds",false,std::nullopt};
    if(f.routeLength<0||f.routeLength>500||f.routeCursor<0)
        return {false,"positive_route_counter_out_of_contract",false,std::nullopt};
    // SavedRouteDirection releases before indexing when cursor>=length. It
    // clears only its OWN reciprocal owner; a foreign owner is retained.
    if(f.routeCursor>=f.routeLength)return {true,"exhausted_release_pending",false,
        services?std::optional<bool>(services->routeOwners[size_t(f.route)]==int(f.slot)):std::nullopt};
    if(!services||services->directions.size()!=500000)
        return {std::nullopt,"route_services_unavailable",true,std::nullopt};
    const bool owner=services->routeOwners[size_t(f.route)]==int(f.slot);
    return {owner,owner?"active_route":"active_route_owner_mismatch",true,owner};
}
} // namespace game_lab
