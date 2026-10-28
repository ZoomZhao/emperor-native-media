// SPDX-License-Identifier: MIT
#pragma once
#include "native_immigrant_codec.hpp"
#include "original_terrain.hpp"
#include "original_figure_update.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace game_lab {
struct NativeImmigrantArrivalDelta {
    size_t houseSlot=0,figureSlot=0;
    uint32_t residentsBefore=0,residentsAfter=0,arrived=0;
    bool convertedEmpty=false;
};
namespace native_immigrant_arrival_detail {
inline bool zeroBuilding(const OriginalFigureUpdateContext& c){
    if(!c.buildings||!c.buildings->count(0))return false;
    const auto& b=c.buildings->at(0);
    return !b.type&&!b.alive&&!b.worker&&!b.primaryFigure&&!b.residents&&!b.freeSlots;
}
// Complete native pool/list proof. Other arrivals/removals in the same pass
// are legal; dangling/cyclic/duplicate memberships and slot0 aliases are not.
inline bool pool(const OriginalFigureUpdateContext& c){
    if(c.figures.size()!=2000||c.cellHeads.size()!=51984)return false;
    std::vector<uint8_t> seen(2000,0);size_t highest=0;
    for(size_t n=0;n<2000;++n){const auto f=c.figures.find(n);if(f==c.figures.end()||f->second.slot!=n)return false;
        if(f->second.alive){if(!n||f->second.alive!=1)return false;highest=n;}
        else if(f->second.next)return false;}
    const auto& zero=c.figures.at(0);if(zero.alive||zero.type||zero.next||zero.owner||zero.home)return false;
    for(size_t cell=0;cell<c.cellHeads.size();++cell)for(size_t slot=c.cellHeads[cell];slot;){
        if(slot>=2000||seen[slot]++)return false;const auto& f=c.figures.at(slot);
        if(!f.alive||originalFigureDrawCell(f)!=cell)return false;slot=f.next;}
    for(size_t slot=1;slot<2000;++slot)if(bool(c.figures.at(slot).alive)!=bool(seen[slot]))return false;
    std::set<size_t> free;
    for(size_t slot:c.freeSlots)if(!slot||slot>=2000||!free.insert(slot).second||c.figures.at(slot).alive)return false;
    return c.highestLiveSlot==highest;
}
inline std::string fields(const OriginalFigure& value){
    native_walker_detail::Writer writer;auto copy=value;native_walker_detail::visitWellWalker(writer,copy);return writer.out;
}
inline OriginalFigure removedRecord(const OriginalFigure& before){
    OriginalFigure r;r.slot=before.slot;r.recordOffset=before.recordOffset;r.runtimeClass=before.runtimeClass;
    // OrderedNextPass copies the updated progress to priorProgress before its
    // normal cleanup; an exhausted free step does not change progress.
    r.priorProgress=before.progress;
    r.effectKind=before.effectKind;r.effectVariant=before.effectVariant;r.effectFlags=before.effectFlags;
    r.effectX=before.effectX;r.effectY=before.effectY;r.effectHeight=before.effectHeight;
    r.effectVelocityX=before.effectVelocityX;r.effectVelocityY=before.effectVelocityY;r.effectVelocityZ=before.effectVelocityZ;
    return r;
}
} // namespace native_immigrant_arrival_detail

// One house's positive type11/action8 arrival over a COMPLETE ordered figure
// pass. Pure proof: no mutation, IO, asset lookup or population publication.
// The caller must deduplicate returned house/figure IDs and compare the SUM of
// all arrival/departure events with global population. Neither shared total nor
// lastArrival is required to equal this one event (several houses may arrive).
// Native nonempty capacity clipping happens BEFORE `before`; strict worker
// code itself clips only an empty house. No type12/type13 behavior is inferred.
namespace native_immigrant_arrival_detail {
inline std::optional<NativeImmigrantArrivalDelta> proveArrivalCore(
    const OriginalFigureUpdateContext& before,const OriginalFigureUpdateContext& after,
    size_t houseSlot,std::string& error,bool allowMigrant=false,bool checkPools=true){
    error.clear();
    const auto refuse=[&](const char* reason)->std::optional<NativeImmigrantArrivalDelta>{error=reason;return std::nullopt;};
    if(!houseSlot||houseSlot>=4000||!before.buildings||!after.buildings||
       !before.buildings->count(houseSlot)||!after.buildings->count(houseSlot))return refuse("immigrant_arrival_house_missing");
    if(!zeroBuilding(before)||!zeroBuilding(after))return refuse("immigrant_arrival_slot0_changed");
    if(checkPools&&(!pool(before)||!pool(after)))return refuse("immigrant_arrival_pool_graph_invalid");
    const auto& a=before.buildings->at(houseSlot);const auto& b=after.buildings->at(houseSlot);
    if(a.type<2||a.type>=18||(a.alive!=1&&a.alive!=3)||!a.accepting||a.residents<0||
       !a.laborDisabled||*a.laborDisabled||a.worker<=0||a.worker>=2000)return refuse("immigrant_arrival_target_not_eligible");
    const size_t slot=size_t(a.worker);const auto& w=before.figures.at(slot);
    const bool migrant=allowMigrant&&w.type==13;
    if(!(migrant?native_walker_detail::validateMigrant(w):native_walker_detail::validateImmigrant(w)).empty()||
       w.action!=8||w.updateOwner||w.home!=int(houseSlot)||!w.employmentAmount)return refuse("immigrant_arrival_actor_not_reserved");
    if(w.freeRemainingX!=0||w.freeRemainingY!=0)return refuse("immigrant_arrival_not_exhausted");
    if(!before.cropBase||!after.cropBase||before.cropBase!=after.cropBase||a.x<0||a.y<0||a.x>=228||a.y>=228||!a.size||a.size>6)
        return refuse("immigrant_arrival_geometry_missing");
    const uint64_t cell=uint64_t(*before.cropBase)+unsigned(a.x)+228ULL*unsigned(a.y);
    if(cell>=51984||a.cell!=cell||w.cell!=cell||w.x!=a.x||w.y!=a.y||w.destinationX!=a.x||w.destinationY!=a.y||
       w.subX!=20*int(a.x)||w.subY!=20*int(a.y)||w.freeTargetX!=w.subX||w.freeTargetY!=w.subY)
        return refuse("immigrant_arrival_not_at_home");
    // A worker cannot be reserved by two houses, and a house cannot silently
    // accept another immigrant/migrant already pointing at the same target.
    for(const auto& [other,h]:*before.buildings)if(other!=houseSlot&&h.worker==int(slot))return refuse("immigrant_arrival_duplicate_worker_target");
    for(const auto& [other,f]:before.figures)if(other!=slot&&f.alive&&(f.type==11||f.type==13)&&f.home==int(houseSlot))
        return refuse("immigrant_arrival_duplicate_house_target");
    const int key=a.type==11?11:a.houseModel;const auto cap=before.houseCapacities.find(key);
    if(cap==before.houseCapacities.end()||cap->second<=0||cap->second>INT16_MAX||before.houseCapacities!=after.houseCapacities)
        return refuse("immigrant_arrival_capacity_missing");
    const bool empty=a.residents==0;
    if(empty&&(migrant||a.type!=2||a.houseModel!=0||a.size!=2||!before.buildingEditMode||*before.buildingEditMode))
        return refuse("immigrant_arrival_empty_conversion_unported");
    // Empty ordinary workers apply the existing min(amount,capacity). For a
    // nonempty target, require the native caller's already-clipped amount.
    const int arrived=migrant?std::min<int>(w.employmentAmount,std::max(0,cap->second-int(a.residents))):
        empty?std::min<int>(w.employmentAmount,cap->second):int(w.employmentAmount);
    if(arrived<=0||(!empty&&arrived>cap->second-int(a.residents))||int(a.residents)+arrived>INT16_MAX)
        return refuse("immigrant_arrival_amount_not_admitted");
    if(b.type!=(empty?3:a.type)||b.houseModel!=(empty?0:a.houseModel)||b.residents!=int(a.residents)+arrived||
       b.freeSlots!=cap->second-int(b.residents)||b.x!=a.x||b.y!=a.y||b.cell!=a.cell||b.size!=a.size||
       b.alive!=a.alive||b.accepting!=a.accepting||b.laborDisabled!=a.laborDisabled)
        return refuse("immigrant_arrival_house_delta_unproved");
    if(fields(after.figures.at(slot))!=fields(removedRecord(w)))return refuse("immigrant_arrival_actor_not_reset");
    if(before.freeSlots.size()>after.freeSlots.size()||!std::equal(before.freeSlots.begin(),before.freeSlots.end(),after.freeSlots.begin())||
       std::count(before.freeSlots.begin(),before.freeSlots.end(),slot)||std::count(after.freeSlots.begin(),after.freeSlots.end(),slot)!=1)
        return refuse("immigrant_arrival_fifo_cleanup_unproved");
    for(size_t n=before.freeSlots.size();n<after.freeSlots.size();++n){const auto freed=after.freeSlots[n];
        if(!before.figures.at(freed).alive||after.figures.at(freed).alive)return refuse("immigrant_arrival_fifo_suffix_unproved");}
    for(const auto& [id,h]:*after.buildings){(void)id;if(h.worker==int(slot))return refuse("immigrant_arrival_worker_not_released");}
    if(!before.services||!after.services||!before.services->population||!after.services->population)return refuse("immigrant_arrival_population_missing");
    if(w.route<0||w.route>=1000)return refuse("immigrant_arrival_route_invalid");
    for(size_t n=0;n<before.services->routeOwners.size();++n)if(before.services->routeOwners[n]==int(slot)&&(!w.route||n!=size_t(w.route)))
        return refuse("immigrant_arrival_duplicate_route");
    if(w.route&&before.services->routeOwners[size_t(w.route)]!=int(slot))return refuse("immigrant_arrival_route_not_owned");
    for(auto owner:after.services->routeOwners)if(owner==int(slot))return refuse("immigrant_arrival_route_not_released");
    return NativeImmigrantArrivalDelta{houseSlot,slot,uint32_t(a.residents),uint32_t(b.residents),uint32_t(arrived),empty};
}

// The only permitted replacement is a later slot's ordinary initial-wait
// search. It reserves a NONEMPTY house and returns immediately as action7;
// it cannot also arrive during the same ascending ordered pass.
inline bool waitingReservation(const OriginalFigureUpdateContext& before,
    const OriginalFigureUpdateContext& after,size_t houseSlot,size_t slot,std::string& error){
    const auto fail=[&](const char* why){error=why;return false;};
    const auto& f=before.figures.at(slot);const auto& g=after.figures.at(slot);
    const int16_t nextCounter=originalFigureWord(int(f.counter)+1);
    if(!native_walker_detail::validateMigrant(f).empty()||f.action!=6||f.updateOwner||f.home||
       f.leaderLink||f.reciprocalTarget||!f.employmentAmount||nextCounter<=51||
       !before.modelSpeeds.count(13)||before.modelSpeeds!=after.modelSpeeds)
        return fail("immigrant_arrival_replacement_not_waiting");
    for(const auto& [id,h]:*before.buildings){(void)id;if(h.worker==int(slot))return fail("immigrant_arrival_replacement_already_reserved");}
    for(const auto& [id,h]:*after.buildings)if(id!=houseSlot&&h.worker==int(slot))return fail("immigrant_arrival_replacement_duplicate_worker");
    for(const auto& [id,other]:after.figures)if(id!=slot&&other.alive&&(other.type==11||other.type==13)&&other.home==int(houseSlot))
        return fail("immigrant_arrival_replacement_duplicate_home");
    const auto road=before.workerRoadExits.find(houseSlot),lastRoad=after.workerRoadExits.find(houseSlot);
    if(road==before.workerRoadExits.end()||!road->second||lastRoad==after.workerRoadExits.end()||road->second!=lastRoad->second)
        return fail("immigrant_arrival_replacement_road_unknown");
    auto expected=f;++expected.age;expected.frame=0;expected.counter=nextCounter;expected.secondaryImage=0;expected.movementMode=3;
    expected.home=int16_t(houseSlot);expected.action=7;expected.destinationX=road->second->first;expected.destinationY=road->second->second;
    expected.roamSteps=0;expected.priorProgress=expected.progress;
    // Image selection is independently checked by the ordered updater. Other
    // removed/moving actors may splice this member's `next`; the full pool
    // graph above proves its resulting membership, cell and non-aliasing.
    expected.image=g.image;expected.next=g.next;
    if(fields(expected)!=fields(g)||g.alive!=1||g.home!=int(houseSlot)||g.action!=7)
        return fail("immigrant_arrival_replacement_transition_unproved");
    if(f.route<0||f.route>=1000)return fail("immigrant_arrival_replacement_route_invalid");
    for(size_t route=0;route<before.services->routeOwners.size();++route){
        const auto a=before.services->routeOwners[route],b=after.services->routeOwners[route];
        if((a==int(slot)||b==int(slot))&&(!f.route||route!=size_t(f.route)||a!=int(slot)||b!=int(slot)))
            return fail("immigrant_arrival_replacement_route_invalid");
    }
    if(f.route&&before.services->routeOwners[size_t(f.route)]!=int(slot))return fail("immigrant_arrival_replacement_route_invalid");
    return true;
}
inline bool sameSelectionBody(const OriginalFigureBuildingLink& a,const OriginalFigureBuildingLink& b){
    return a.type==b.type&&a.houseModel==b.houseModel&&a.residents==b.residents&&a.freeSlots==b.freeSlots;
}
inline bool proveReplacement(const OriginalFigureUpdateContext& before,const OriginalFigureUpdateContext& after,
    size_t houseSlot,size_t arrivedSlot,std::string& error){
    const auto fail=[&](const char* why){error=why;return false;};
    const int replacement=after.buildings->at(houseSlot).worker;
    if(replacement<=int(arrivedSlot)||replacement>=2000)return fail("immigrant_arrival_replacement_slot_order");
    const size_t limit=size_t(replacement);
    struct Event {size_t house;bool arrival;};std::map<size_t,Event> events;
    auto selection=before;
    if(before.buildings->size()!=after.buildings->size())return fail("immigrant_arrival_replacement_house_set_changed");
    for(const auto& [id,a]:*before.buildings){
        const auto last=after.buildings->find(id);if(last==after.buildings->end())return fail("immigrant_arrival_replacement_house_set_changed");
        const auto& b=last->second;
        if(!id||((a.type<2||a.type>=18)&&(b.type<2||b.type>=18)))continue;
        if(a.type<2||a.type>=18||a.x!=b.x||a.y!=b.y||a.cell!=b.cell||a.size!=b.size||
           a.alive!=b.alive||a.accepting!=b.accepting||a.roadDistance!=b.roadDistance||a.worker<0||a.worker>=2000)
            return fail("immigrant_arrival_replacement_selection_geometry_changed");
        if(a.worker>0&&size_t(a.worker)<limit){
            const auto& old=before.figures.at(size_t(a.worker));
            if(!after.figures.at(size_t(a.worker)).alive){
                // Reconstruct a completed lower-slot arrival using the same
                // cleanup/delta proof, not the final worker alone. Type13 is
                // accepted here only for its already-supported nonempty case.
                std::string detail;
                if(!proveArrivalCore(before,after,id,detail,true,false))return fail("immigrant_arrival_replacement_earlier_arrival_unproved");
                if(!events.emplace(size_t(a.worker),Event{id,true}).second)return fail("immigrant_arrival_replacement_duplicate_event");
            }else if(b.worker!=a.worker||old.home!=int(id)||!sameSelectionBody(a,b))
                return fail("immigrant_arrival_replacement_earlier_release_unproved");
        }else if(!a.worker&&!sameSelectionBody(a,b))return fail("immigrant_arrival_replacement_unreserved_house_changed");
        // A later old worker has not yet run, so retain BEFORE vacancy and
        // reservation. A newly reserved worker only becomes visible at its
        // own slot. This handles several houses arriving/reserving in a pass.
        if(b.worker>0&&b.worker!=a.worker&&size_t(b.worker)<=limit){
            const size_t next=size_t(b.worker);
            if(!waitingReservation(before,after,id,next,error))return false;
            if(!events.emplace(next,Event{id,false}).second)return fail("immigrant_arrival_replacement_duplicate_event");
        }
    }
    for(const auto& [slot,event]:events){
        auto& house=selection.buildings->at(event.house);
        if(event.arrival){house=after.buildings->at(event.house);house.worker=0;continue;}
        if(house.worker||house.residents<=0)return fail("immigrant_arrival_replacement_target_not_nonempty_vacant");
        const auto selected=originalFigureNearestHouse(before.figures.at(slot),selection);
        if(!selected||*selected!=event.house)return fail("immigrant_arrival_replacement_nearest_house_unproved");
        house.worker=int16_t(slot);
    }
    if(selection.buildings->at(houseSlot).worker!=replacement)return fail("immigrant_arrival_replacement_not_reconstructed");
    return true;
}
} // namespace native_immigrant_arrival_detail

inline std::optional<NativeImmigrantArrivalDelta> proveNativeImmigrantArrival(
    const OriginalFigureUpdateContext& before,const OriginalFigureUpdateContext& after,
    size_t houseSlot,std::string& error){
    auto arrival=native_immigrant_arrival_detail::proveArrivalCore(before,after,houseSlot,error);
    if(!arrival)return std::nullopt;
    if(after.buildings->at(houseSlot).worker&&
       !native_immigrant_arrival_detail::proveReplacement(before,after,houseSlot,arrival->figureSlot,error))return std::nullopt;
    return arrival;
}
} // namespace game_lab
