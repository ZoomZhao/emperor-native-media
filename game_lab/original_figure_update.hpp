#pragma once
#include "original_figure_image.hpp"
#include <set>

namespace game_lab {
// Load0x534C78..91 restores the personal speed unless network play is active.
// Neither clock epoch nor fast-forward is serialized/reset by the SAV load.
struct OriginalFigureSchedulerGlobals {uint32_t previousClock=0;int32_t speed=90;bool fastForward=false,paused=false;};
inline OriginalFigureSchedulerGlobals originalFigureSchedulerAfterLoad(OriginalFigureSchedulerGlobals live,
    int32_t personalSpeed,bool networkActive){
    if(!networkActive)live.speed=personalSpeed;
    live.paused=false;return live;
}
// Effective live inputs at0x53A173, after its external event prelude. Clock is
// timeGetTime(), not a SAV tick. No clock epoch or live gate is inferred.
struct OriginalFigureAdmissionInputs {
    uint32_t now=0,last=0,mode=1;
    int32_t speed=0;
    bool paused=false,specialModeReady=false,blocked88ebd8=false,blocked10de038=false;
};
struct OriginalFigureAdmission {
    bool admitted=false;
    uint32_t last=0;
    unsigned limit=0;
};
inline OriginalFigureAdmission originalFigureAdmission(const OriginalFigureAdmissionInputs& in){
    OriginalFigureAdmission out;out.last=in.last;
    const uint32_t speedBits=100u-uint32_t(in.speed);
    const int64_t speedDifference=speedBits>INT32_MAX?int64_t(speedBits)-4294967296LL:speedBits;
    const int64_t speedIndex=speedDifference/10;
    if(speedIndex>=10||in.paused||in.blocked88ebd8||in.blocked10de038)return out;
    if(in.mode==0x12||in.mode==0x23){if(!in.specialModeReady)return out;}
    else switch(in.mode){case 1:case 6:case 0x27:case 0x48:case 0x49:case 0x56:break;default:return out;}
    // VA0x855214/file0x455214;the ten scheduler intervals, not fitted to saves.
    constexpr unsigned intervals[]={0,20,35,55,80,110,160,240,350,500};
    const uint32_t delta=in.now-in.last;
    const uint32_t elapsed=delta>INT32_MAX?10000:delta;
    if(elapsed<intervals[speedIndex<0?0:size_t(speedIndex)]+2)return out;
    out.admitted=true;out.last=in.now;out.limit=10000;return out;
}
// Explicit first draw after k admitted passes. The0x536BC5/0x536C58 loop reads
// this flag AFTER each pass;WM_RBUTTONUP clears it. Supplying the observations
// avoids inventing how messages or called work change live fast-forward state.
inline std::optional<unsigned> originalFigureUpdatesBeforeDraw(const OriginalFigureAdmission& admission,
    const std::vector<bool>& continueAfterPass){
    if(!admission.admitted)return 0;
    if(admission.limit!=10000)return std::nullopt;
    for(unsigned n=0;n<10000;++n){
        if(n>=continueAfterPass.size())return std::nullopt;
        if(!continueAfterPass[n]||n==9999)return n+1;
    }
    return std::nullopt;
}
struct OriginalFigureFrameAdvance {uint8_t counter=0;bool advanced=false;size_t eligibilityIndex=0;};
//0x5CFE60:byte reset, eligibility, byte increment, wrap, then image selection.
// Flags are the caller's explicit51 live0x418AA0 flags, not a new timeline.
inline std::optional<OriginalFigureFrameAdvance> originalFigureAdvanceFrame(uint8_t counter,int16_t frames,
    uint8_t sgSpeed,int speedArgument,const std::array<bool,51>& eligibility){
    if(frames<=0||speedArgument<0)return std::nullopt;
    OriginalFigureFrameAdvance out;out.counter=counter;
    if(int(out.counter)>=frames)out.counter=0;
    if(speedArgument){out.eligibilityIndex=100u*sgSpeed/unsigned(speedArgument);
        if(out.eligibilityIndex>=eligibility.size())return std::nullopt;}
    out.advanced=!speedArgument||eligibility[out.eligibilityIndex];
    if(out.advanced){out.counter=uint8_t(out.counter+1);if(int(out.counter)>=frames)out.counter=0;}
    return out;
}
enum class OriginalFigureUpdateStatus {Computed,DispatchGate,UnsupportedType,EffectState,CellListMutation,MissingGroup,
    PheasantPredicates,CartLeaderMovement,GoodsRouteAndOwner,WorkerOwnerMovement,BuildingVirtualUpdate,CombatRouteAndTarget,
    PheasantContext,PheasantThreat,PheasantRoute,PheasantDestination,PheasantBuilding,OrderedPoolPass,
    MovementModel,RouteGeneration,RouteBoundary,MovementHeight,EarlierSlot,PoolRemoval,LaterSlotEffects,
    CellContext,CellCombat,SegmentService,NextDirection,InspectorFireContext,InspectorFireSearch,WorkerEmployment,SupplierArrival,SupplierSearch,
    GoodsSearch,GoodsArrival,Projectile,CrtRandomRequired};
inline const char* originalFigureUpdateStatusName(OriginalFigureUpdateStatus status){
    switch(status){case OriginalFigureUpdateStatus::Computed:return "computed";
    case OriginalFigureUpdateStatus::DispatchGate:return "dispatch_gate";
    case OriginalFigureUpdateStatus::UnsupportedType:return "ordered_type_update";
    case OriginalFigureUpdateStatus::EffectState:return "inline_effect_state";
    case OriginalFigureUpdateStatus::CellListMutation:return "effect_cell_list_mutation";
    case OriginalFigureUpdateStatus::MissingGroup:return "missing_sg_group";
    case OriginalFigureUpdateStatus::OrderedPoolPass:return "later_ordered_pool_pass";
    case OriginalFigureUpdateStatus::MovementModel:return "figure_model_speed_4c9310";
    case OriginalFigureUpdateStatus::RouteGeneration:return "route_generation_4e83e0_and_map_5af1c0";
    case OriginalFigureUpdateStatus::RouteBoundary:return "route_cell_crossing_4e9100_or_next_direction_4e8bc0";
    case OriginalFigureUpdateStatus::MovementHeight:return "movement_bridge_height_4e92c0";
    case OriginalFigureUpdateStatus::EarlierSlot:return "earlier_ordered_slot_uncomputed";
    case OriginalFigureUpdateStatus::PoolRemoval:return "ordered_pool_removal_4ea080";
    case OriginalFigureUpdateStatus::LaterSlotEffects:return "later_ordered_slot_may_write_earlier_figure";
    case OriginalFigureUpdateStatus::CellContext:return "cell_list_context_4ea610_4ea3b0";
    case OriginalFigureUpdateStatus::CellCombat:return "cell_neighbour_combat_4e57a0";
    case OriginalFigureUpdateStatus::SegmentService:return "segment_owner_service_4eacd0";
    case OriginalFigureUpdateStatus::NextDirection:return "next_direction_map_validation_4e8bc0";
    case OriginalFigureUpdateStatus::InspectorFireContext:return "inspector_saved_fire_count_c82f28";
    case OriginalFigureUpdateStatus::InspectorFireSearch:return "inspector_fire_target_search_4afc50";
    case OriginalFigureUpdateStatus::WorkerEmployment:return "worker_arrival_employment_building_writes_4c9fd0";
    case OriginalFigureUpdateStatus::SupplierArrival:return "supplier_arrival_owner_delay_4cc8aa_4c2dc0";
    case OriginalFigureUpdateStatus::SupplierSearch:return "supplier_crop_search_4c2c70_4bff80_4c0760";
    case OriginalFigureUpdateStatus::GoodsSearch:return "goods_expired_supplier_search_4cbec0_4e2960";
    case OriginalFigureUpdateStatus::GoodsArrival:return "goods_delivery_arrival_virtual_4cd8f0";
    case OriginalFigureUpdateStatus::Projectile:return "projectile_allocation_or_collision_5ea510_5e6200";
    case OriginalFigureUpdateStatus::CrtRandomRequired:return "crt_rand_state_required_wall_clock_seeded_not_in_sav";
    case OriginalFigureUpdateStatus::PheasantContext:return "pheasant_complete_saved_context";
    case OriginalFigureUpdateStatus::PheasantThreat:return "pheasant_threat_qualification_4e5340";
    case OriginalFigureUpdateStatus::PheasantRoute:return "pheasant_route_4e47a0_and_arrival";
    case OriginalFigureUpdateStatus::PheasantDestination:return "pheasant_destination_spawn_reachability_59ec60_5af1c0";
    case OriginalFigureUpdateStatus::PheasantBuilding:return "pheasant_linked_building_59f399";
    case OriginalFigureUpdateStatus::PheasantPredicates:return "pheasant_virtual_predicates_59f31b_then_route";
    case OriginalFigureUpdateStatus::CartLeaderMovement:return "cart_leader_order_and_movement_4e7520";
    case OriginalFigureUpdateStatus::GoodsRouteAndOwner:return "goods_route_4e47a0_and_owner_mutations";
    case OriginalFigureUpdateStatus::WorkerOwnerMovement:return "worker_owner_validity_and_movement_4e47a0_4e9620";
    case OriginalFigureUpdateStatus::BuildingVirtualUpdate:return "building_virtual24_ordered_update";
    case OriginalFigureUpdateStatus::CombatRouteAndTarget:return "type_action_route_target_combat_update";}
    return "ordered_type_update";
}
struct OriginalFigureFirstUpdate {
    OriginalFigureUpdateStatus status=OriginalFigureUpdateStatus::UnsupportedType;
    uint32_t updaterVA=0;
    std::optional<OriginalFigure> state;
    std::optional<OriginalFigureSavedServices> services;
    std::map<size_t,OriginalFigure> linkedWrites;
    std::optional<std::vector<uint16_t>> cellHeads;
    std::optional<std::map<size_t,OriginalFigureBuildingLink>> buildings;
    std::optional<int32_t> watchMaximum;
    std::optional<std::array<int32_t,2>> inspectorMaximum;
    std::optional<OriginalFigureGoodsServices> goods;
    std::optional<std::vector<size_t>> freeSlots;
    std::optional<uint32_t> crtRandom;
    uint64_t crtRandomCalls=0;
    std::optional<int32_t> roadblockGate;
    std::set<size_t> allocatedSlots;
    size_t highestLiveSlot=0;
};
struct OriginalFigurePassability {
    OriginalFigureSpawnServices spawn;
    std::vector<uint16_t> cells;
    bool fullMovementMasks=false;
};
// +CC=416A50 in the individually cited building vtables (SAV_FIGURES).
inline bool originalFigureBuildingPassThroughFalse(uint16_t type){
    switch(type){case 2:case 3:case 5:case 7:case 0x21:case 0x23:case 0x2b:case 0x35:case 0x36:case 0x3a:
    case 0x3e:case 0x42:case 0x43:case 0x47:case 0x48:case 0x7c:case 0x7e:case 0x7f:
    case 0xc0:case 0xc1:case 0xc2:case 0xc4:case 0xcf:case 0xd3:case 0xd6:case 0xe7:return true;
    default:return false;}
}
// Full 5AD920/5AD440 words, after the caller's public ground/building rebuild.
// Pointer-grid consumers need the unported 41F400 object state whenever 10000
// exists. Unknown virtual+CC and D2's final+270 are refused, never flattened.
inline std::optional<OriginalFigurePassability> originalFigureMovementPassability(
    const OriginalTerrainState& rebuilt,const OriginalBuildingProfile& buildings,
    const std::vector<uint16_t>& owners,const OriginalFigureSpawnServices& spawn){
    if(!rebuilt.valid()||owners.size()!=51984||buildings.status!=OriginalBuildingStatus::Complete||buildings.parsedRecords!=4000||
        spawn.side<1||spawn.side>228||spawn.cropBase!=((228-spawn.side)/2)*229||
        std::find(buildings.slotTypes.begin(),buildings.slotTypes.end(),0xd2)!=buildings.slotTypes.end()||
        std::any_of(rebuilt.terrain.begin(),rebuilt.terrain.end(),[](uint32_t t){return (t&0x10000)!=0;}))return std::nullopt;
    OriginalFigurePassability out;out.spawn=spawn;out.cells.assign(51984,0);out.fullMovementMasks=true;
    for(unsigned y=0;y<spawn.side;++y)for(unsigned x=0;x<spawn.side;++x){
        const size_t cell=spawn.cropBase+y*228+x;const uint32_t t=rebuilt.terrain[cell],plain=t&0xafffde6f;uint16_t mask=0;
        if(t&0x100)mask=2;
        else if(t&0x800)mask=t&0x40?4:0x4000;
        else if((t&0x80000)||(t&0x10004)==0x10004)mask=2;
        else if((t&0x400)&&(t&0x40)&&!(t&0x800000))mask=0x100;
        else if(t&0x40)mask=4;
        else if(t&(0x400|0x20))mask=0x10;
        else if((t&0x800000)&&(t&0x200)&&!(t&0x400000))mask=0x400;
        else if((t&0x200)&&!(t&0x400000))mask=0x1000;
        else if(t&0x200)mask=2;
        else if((t&0x20000)&&rebuilt.roadDistance[cell]!=255)mask=0x80;
        else if(t&0x10000000)mask=0x20;
        else if(plain&0x8008){
            const auto owner=owners[cell];if(!owner||owner>=4000)return std::nullopt;
            const auto type=buildings.slotTypes[owner];
            // Cited vtables' +CC=416A50 (false). Market's 544200 and
            // specialized monuments are withheld until their predicates exist.
            if(!originalFigureBuildingPassThroughFalse(type))return std::nullopt;
            if(type==0x7e)mask=4;
            else if(type==0xc2||type==0xc4)mask=0x10; //4C11B0
            else mask=t&0x1000?0x4000:2;
        }else if(plain&0x80061001)mask=(plain&0x4000)&&!(plain&0x1000)?2:0x10;
        else mask=plain?2:0x10;
        out.cells[cell]=mask;
    }
    return out;
}
//0x5AD440 projection of the route gate's A00 bits: road branch is4;
// plain flags0/1/80/81 can reach only low-bit terrain/pointer branches,
// all of which leave A00 clear. Unresolved cells stayFFFF. This does not
// publish a full passability grid or reuse the reduced51C reachability mask.
// The only final+270 caller
// is typeD2 (5AD440 ->4BDB60); withhold this projection if that type exists.
inline std::optional<std::vector<uint16_t>> originalFigureRoadMovementMasks(
    const OriginalMapTerrain& terrain,const OriginalBuildingProfile& buildings){
    if(!terrain.valid()||buildings.status!=OriginalBuildingStatus::Complete||buildings.parsedRecords!=4000||
        std::find(buildings.slotTypes.begin(),buildings.slotTypes.end(),0xd2)!=buildings.slotTypes.end())return std::nullopt;
    std::vector<uint16_t> out(51984,0xffff);
    for(size_t n=0;n<out.size();++n){const auto t=terrain.flags[n];
        if((t&0x40)&&!(t&(0x100|0x800|0x80000|0x10004|0x400)))out[n]=4;
        else if(!(t&~uint32_t(0x81)))out[n]=0;}
    return out;
}
//0x5AD920/0x5AD440, restricted to a completely parsed building-free pool.
// Only mask0x51C is published. River bit10000 refuses:0x41F400 can rewrite it
// and create pointers whose ground image selects passability. With no river
// anywhere that prepass has no writes/pointers. Building virtual+CC/+270 refuse.
inline std::optional<OriginalFigurePassability> originalFigurePheasantPassability(
    const OriginalMapTerrain& terrain,const std::vector<uint8_t>& roadDistance,
    const OriginalBuildingProfile& buildings,const OriginalFigureSpawnServices& spawn){
    if(!terrain.valid()||roadDistance.size()!=51984||buildings.status!=OriginalBuildingStatus::Complete||
        buildings.parsedRecords!=4000||spawn.side!=terrain.activeSide||spawn.cropBase!=((228-spawn.side)/2)*229||
        std::any_of(buildings.slotTypes.begin(),buildings.slotTypes.end(),[](uint16_t type){return type!=0;})||
        std::any_of(terrain.flags.begin(),terrain.flags.end(),[](uint32_t t){return (t&0x10000)!=0;}))return std::nullopt;
    OriginalFigurePassability out;out.spawn=spawn;out.cells.resize(51984);
    for(unsigned y=0;y<spawn.side;++y)for(unsigned x=0;x<spawn.side;++x){
        const size_t cell=spawn.cropBase+y*228+x;const uint32_t t=terrain.flags[cell];uint16_t mask=0;
        if(t&0x100)mask=2;
        else if(t&0x800)mask=t&0x40?4:0x4000;
        else if((t&0x80000)||(t&0x10004)==0x10004)mask=2;
        else if((t&0x400)&&(t&0x40)&&!(t&0x800000))mask=0x100;
        else if(t&0x40)mask=4;
        else if(t&(0x400|0x20))mask=0x10;
        else if((t&0x800000)&&(t&0x200)&&!(t&0x400000))mask=0x400;
        else if((t&0x200)&&!(t&0x400000))mask=0x1000;
        else if(t&0x200)mask=2;
        else if((t&0x20000)&&roadDistance[cell]!=255)mask=0x80;
        else if(t&0x10000000)mask=0x20;
        else if(t&0x8008)return std::nullopt;
        else if(t&0x80061001)mask=(t&0x4000)&&!(t&0x1000)?2:0x10;
        else mask=t&0xafffde6f?2:0x10;
        out.cells[cell]=mask&0x51c;
    }
    return out;
}
//0x5AF1C0/0x5AF2C0: cardinal N,E,S,W flood. Start is visited even when its
// own mask is blocked. Destinations are linear backing cells, not clamped x/y.
inline std::optional<std::vector<uint8_t>> originalFigureReachableCells(
    const OriginalFigurePassability& map,int x,int y){
    const int64_t start=int64_t(map.spawn.cropBase)+x+int64_t(y)*228;
    if(map.cells.size()!=51984||start<0||start>=51984)return std::nullopt;
    std::vector<uint8_t> seen(51984);std::vector<size_t> queue{size_t(start)};seen[size_t(start)]=1;
    constexpr int offsets[]={-228,1,228,-1};
    for(size_t at=0;at<queue.size();++at)for(int delta:offsets){const int64_t next=int64_t(queue[at])+delta;
        if(next<0||next>=51984)continue;
        if(!seen[size_t(next)]&&(map.cells[size_t(next)]&0x51c)){seen[size_t(next)]=1;queue.push_back(size_t(next));}}
    return seen;
}
// Complete saved context, ordered by pool slot. Both image words are erased.
// This models one virtual call against that explicit context, not the global
// pool pass: preceding unsupported slot updates are not silently invented.
struct OriginalFigureUpdateContext {
    std::map<size_t,OriginalFigure> figures;unsigned view=0;
    std::map<unsigned,int> modelSpeeds;
    std::optional<OriginalFigureSavedServices> services;
    std::optional<OriginalFigurePassability> passability;
    std::optional<std::map<size_t,OriginalFigureHunterLink>> hunterLinks;
    std::optional<std::map<size_t,OriginalFigureBuildingLink>> buildings;
    std::map<unsigned,std::array<int32_t,18>> models;
    std::vector<uint16_t> cellHeads;
    std::vector<uint32_t> terrain;
    std::optional<uint32_t> cropBase;
    std::vector<uint16_t> movementMasks; // Explicit5AD440 A00 gates;FFFF denotes unknown. Never the reduced51C mask.
    std::optional<std::array<int16_t,22>> factionModels;
    std::map<int,int> houseCapacities;
    std::map<int,int> houseWatchFloors;
    std::map<unsigned,std::map<int,int>> houseMarketModels;
    std::optional<int32_t> watchtowerReduction;
    std::optional<int32_t> inspectorReduction;
    std::optional<std::array<int32_t,2>> inspectorMaximum; //D95084/88, explicit persistent process scratch.
    std::optional<int32_t> watchMaximum; //D95080 process input; retained between calls, absent refuses.
    std::optional<OriginalFigureCropServices> crops;
    std::vector<uint8_t> elevation;
    std::vector<uint16_t> buildingOwners;
    std::vector<uint8_t> feature;
    std::vector<uint8_t> randomByte; //F1E780,saved public terrain input;4E6690/4E6D80.
    std::optional<unsigned> activeSide;
    std::optional<OriginalFigureGoodsServices> goods;
    std::map<uint16_t,int32_t> buildingEmployment;
    std::optional<int32_t> festivalVenue;
    std::optional<std::array<int16_t,3>> entertainmentAvailable;
    std::optional<bool> waterOverride; //48E110(C701D8),restored Festival4 predicate;absent refuses Well basic service.
    std::vector<size_t> freeSlots; //postload4E9FE0 descending inactive slots; removal appends.
    size_t highestLiveSlot=0;
    std::optional<uint32_t> crtRandom; //Required on consumption:wall-clock seeded,not in SAV;state at timeline entry,not startup srand argument.
    uint64_t crtRandomCalls=0; //Cumulative successfully ported7665FD calls,including discarded sound variation.
    std::optional<std::array<bool,51>> animationFlags; //current418AA0 flags,for5CFE60(speed100).
    std::set<size_t> allocatedSlots;
    // Inputs at each primary image write. A later Hunter can turn an earlier
    // prey without selecting its image again (4D31DE..4D321C).
    std::map<size_t,OriginalFigureImageInputs> primaryImageInputs;
    std::optional<int32_t> roadblockGate; //1032664;4E7F6E clears after route following.
};
//42D790 clears the house vector/date through5178A0->444270 (year-10000),
// then registers loaded live houses. SAV19 tail533E71->517CC0->517DE0
// therefore recounts its accepting houses before publishing130F988.
inline bool originalFigurePopulationAfterLoad(OriginalFigureSavedServices& services,
    const std::map<size_t,OriginalFigureBuildingLink>& buildings){
    if(!services.population)return false;int64_t total=0;
    for(const auto& entry:buildings){const auto& h=entry.second;
        if(h.type>=2&&h.type<18&&h.alive&&h.alive!=2&&h.alive!=5&&h.alive!=6&&h.accepting)total+=h.residents;}
    if(total<INT32_MIN||total>INT32_MAX)return false;services.population->total=int32_t(total);return true;
}
//0x4189B0 advances both persisted registers31 times;destination0x59EDE0
// consumes first&0x7FFF after each call. No process-start seed is substituted.
inline uint16_t originalFigureRandomNext(OriginalFigureSavedServices& state){
    for(unsigned n=0;n<31;++n){const auto advance=[](uint32_t value){
            return (value>>1)|((((value>>4)^value)&1u)<<30);};
        state.randomFirst=advance(state.randomFirst);state.randomSecond=advance(state.randomSecond);}
    return uint16_t(state.randomFirst&0x7fff);
}
//7665FD uses the calling thread's holdrand;7665F0 seeds it from time().
//An absent input refuses at the call;zero is a valid explicitly supplied state.
inline std::optional<uint16_t> originalFigureCrtRandomNext(OriginalFigureUpdateContext& context){
    if(!context.crtRandom)return std::nullopt;
    *context.crtRandom=*context.crtRandom*214013u+2531011u;++context.crtRandomCalls;
    return uint16_t((*context.crtRandom>>16)&0x7fff);
}
struct OriginalFigureRouteDirection {OriginalFigure state;OriginalFigureSavedServices services;};
//0x4E8B40, including0x4E8A30's reciprocal owner release. No route is generated.
inline std::optional<OriginalFigureRouteDirection> originalFigureSavedRouteDirection(
    const OriginalFigure& saved,const OriginalFigureSavedServices& services){
    if(saved.route<1||saved.route>=1000||saved.routeCursor<0||saved.routeLength<0||saved.routeLength>500||
        services.directions.size()!=500000)return std::nullopt;
    OriginalFigureRouteDirection out{saved,services};
    if(saved.routeCursor>=saved.routeLength){
        if(out.services.routeOwners[saved.route]==int(saved.slot))out.services.routeOwners[saved.route]=0;
        out.state.route=0;out.state.direction=8;
    }else out.state.direction=services.directions[size_t(saved.route)*500+size_t(saved.routeCursor)];
    return out;
}
inline std::optional<OriginalFigureUpdateContext> originalFigureUpdateContext(const OriginalFigureProfile& profile){
    if(profile.status!=OriginalFigureStatus::Complete||profile.declared!=2000||profile.parsed!=2000||profile.slots.size()!=2000)return std::nullopt;
    OriginalFigureUpdateContext out;out.view=profile.savedView;out.cellHeads=profile.cellHeads;out.factionModels=profile.factionModels;
    for(auto f:profile.slots){
        if(f.slot>=2000)return std::nullopt;
        f.image=f.secondaryImage=0;
        if(!out.figures.emplace(f.slot,f).second)return std::nullopt;
        if(f.alive)out.highestLiveSlot=std::max(out.highestLiveSlot,f.slot);
    }
    for(size_t slot=1999;slot>0;--slot)if(!out.figures.at(slot).alive)out.freeSlots.push_back(slot);
    return out;
}
//0x5D2030 reads eighteen numbers per row;0x4C9310(type,8) reads speed.
// A speed whose unit-type flag requests difficulty scaling is withheld here.
// This bounded parser accepts the documented comma/braces model grammar and
// publishes only after all five sections and their exact row counts close.
inline std::optional<std::map<unsigned,std::array<int32_t,18>>> originalFigureModels(const std::string& text){
    if(text.size()>100000)return std::nullopt;
    const char* names[]={"ALL MODS","ALL UNITTYPES","ALL FIGURES","ALL ENEMIES","ALL HEROES"};
    const size_t counts[]={5,12,83,42,12};
    std::array<std::vector<std::array<int32_t,18>>,5> rows;
    size_t at=0;
    for(size_t section=0;section<5;++section){
        const auto begin=text.find(names[section],at);if(begin==std::string::npos)return std::nullopt;
        at=begin+std::string(names[section]).size();
        const auto end=section<4?text.find(names[section+1],at):text.size();
        if(end==std::string::npos)return std::nullopt;
        while(true){const auto open=text.find('{',at);if(open==std::string::npos||open>=end)break;
            at=open+1;std::array<int32_t,18> values{};
            for(auto& value:values){
                while(at<end&&(text[at]==','||text[at]==' '||text[at]=='\t'||text[at]=='\r'||text[at]=='\n'))++at;
                bool negative=false;if(at<end&&text[at]=='-'){negative=true;++at;}
                if(at==end||text[at]<'0'||text[at]>'9')return std::nullopt;
                int64_t number=0;while(at<end&&text[at]>='0'&&text[at]<='9'){
                    number=number*10+text[at++]-'0';if(number>2147483648LL)return std::nullopt;}
                if(!negative&&number>INT32_MAX)return std::nullopt;
                value=int32_t(negative?-number:number);
                while(at<end&&(text[at]==' '||text[at]=='\t'||text[at]=='\r'||text[at]=='\n'))++at;
                if(at==end||text[at++]!=',')return std::nullopt;
            }
            while(at<end&&(text[at]==' '||text[at]=='\t'||text[at]=='\r'||text[at]=='\n'))++at;
            if(at==end||text[at++]!='}')return std::nullopt;
            rows[section].push_back(values);if(rows[section].size()>counts[section])return std::nullopt;
        }
        if(rows[section].size()!=counts[section])return std::nullopt;at=end;
    }
    std::map<unsigned,std::array<int32_t,18>> out;
    for(size_t type=0;type<rows[2].size();++type){const auto& row=rows[2][type];
        if(row[0]<0||size_t(row[0])>=rows[1].size())return std::nullopt;
        auto values=row;for(size_t field=1;field<18;++field)if(rows[1][row[0]][field]>0)values[field]=INT32_MIN;
        out.emplace(unsigned(type),values);}
    return out;
}
inline std::optional<std::map<unsigned,int>> originalFigureModelSpeeds(const std::string& text){
    const auto models=originalFigureModels(text);if(!models)return std::nullopt;
    std::map<unsigned,int> out;for(const auto& row:*models)if(row.second[8]>=0&&row.second[8]<=255)out.emplace(row.first,row.second[8]);
    return out;
}
// Shared0x4E47A0 speed schedule. The phase byte is persisted; fractional
// speeds are not rounded into a constant number of steps.
inline std::pair<unsigned,uint8_t> originalFigureMovementSteps(unsigned speed,uint8_t phase){
    unsigned steps=3;uint8_t next=phase;
    switch(speed){
    case 0:steps=0;break;
    case 1:case 2:{const unsigned threshold=speed==1?2:1;steps=phase>threshold?1:0;next=phase>threshold?0:uint8_t(phase+1);break;}
    case 3:steps=phase==0?1:0;next=phase==0?1:0;break;
    case 4:case 5:steps=phase>(speed==4?1:2)?0:1;next=steps?uint8_t(phase+1):0;break;
    case 6:steps=1;break;
    case 7:case 8:case 9:steps=phase>(speed==7?2:speed==8?1:0)?2:1;next=steps==2?0:uint8_t(phase+1);break;
    case 10:case 11:steps=phase>(speed==10?1:2)?1:2;next=steps==1?0:uint8_t(phase+1);break;
    case 12:steps=2;break;
    case 13:case 14:steps=phase>(speed==13?2:1)?3:2;next=steps==3?0:uint8_t(phase+1);break;
    case 15:case 16:case 17:steps=phase>(speed==15?0:speed==16?1:2)?2:3;next=steps==2?0:uint8_t(phase+1);break;
    default:break;
    }
    return {steps,next};
}
//0x4E4C10 is the follower's observation of a leader phase. It does not
// advance that phase and its case13 differs from0x4E47A0.
inline unsigned originalFigureFollowerSteps(unsigned speed,uint8_t phase){
    if(speed<=6)return 1;
    switch(speed){case 7:return 1+unsigned(phase>2);case 8:return 1+unsigned(phase>1);
    case 9:return 1+unsigned(phase!=0);case 10:return 2-unsigned(phase>1);case 11:return 2-unsigned(phase>2);
    case 12:return 2;case 13:return 2-unsigned(phase>2);case 14:return 2+unsigned(phase>1);
    case 15:return 3-unsigned(phase!=0);case 16:return 3-unsigned(phase>1);case 17:return 3-unsigned(phase>2);
    default:return 3;}
}
struct OriginalFigureMovement {OriginalFigureUpdateStatus status;std::optional<OriginalFigure> state;};
inline int16_t originalFigureWord(int value){const unsigned bits=unsigned(value)&65535;return int16_t(bits>32767?int(bits)-65536:int(bits));}
inline uint8_t originalFigureDirectionTo(int x,int y,int targetX,int targetY){
    if(targetX==x)return targetY<y?0:targetY>y?4:8;
    if(targetX>x)return targetY<y?1:targetY>y?3:2;
    return targetY<y?7:targetY>y?5:6;
}
//5B2910/408BA0: eight sectors with axis threshold400 percent.
inline uint8_t originalFigureAttackDirection(int x,int y,int tx,int ty){
    const int dx=std::abs(tx-x),dy=std::abs(ty-y);
    if(!dx)return ty>y?4:0;if(!dy)return tx>x?2:6;
    if(dx>=4*dy)return tx>x?2:6;if(dy>=4*dx)return ty>y?4:0;
    return tx>x?(ty>y?3:1):(ty>y?5:7);
}
//5B2710 doubles5B2790's sixteen sectors; ratio uses408BA0 integer percent.
inline uint8_t originalFigureProjectileDirection(int x,int y,int tx,int ty){
    const int dx=std::abs(tx-x),dy=std::abs(ty-y);if(!dx)return ty>y?16:0;if(!dy)return tx>x?8:24;
    const int ratio=dx==dy?100:dx>dy?100*dx/dy:-100*dy/dx;
    unsigned sector=0;
    if(tx>x&&ty<y)sector=ratio>=500?4:ratio>=200?3:ratio>-200?2:ratio>-500?1:0;
    else if(tx>x)sector=ratio>=500?4:ratio>=200?5:ratio>-200?6:ratio>-500?7:8;
    else if(ty>y)sector=ratio>=500?12:ratio>=200?11:ratio>-200?10:ratio>-500?9:8;
    else sector=ratio>=500?12:ratio>=200?13:ratio>-200?14:ratio>-500?15:0;
    return uint8_t(2*sector);
}
// 5AF1C0 distances stop when the destination is dequeued. 5B18B0 then
// walks that field backwards, preferring the direction toward the source on
// equal distance, excluding the just-used reverse, and reversing the output.
inline std::optional<std::vector<uint8_t>> originalFigureEngineRoute(const OriginalFigurePassability& map,
    int x,int y,int targetX,int targetY,uint16_t allowed=0x51c,unsigned directions=8,bool* reached=nullptr){
    if(!map.fullMovementMasks||map.cells.size()!=51984)return std::nullopt;
    if(directions!=4&&directions!=8)return std::nullopt;
    const int64_t start=int64_t(map.spawn.cropBase)+x+228LL*y,target=int64_t(map.spawn.cropBase)+targetX+228LL*targetY;
    if(start<0||start>=51984||target<0||target>=51984)return std::nullopt;
    std::vector<int> distances(51984);std::vector<size_t> queue{size_t(start)};distances[size_t(start)]=1;
    constexpr int delta[]={-228,-227,1,229,228,227,-1,-229};
    for(size_t at=0;at<queue.size();++at){const size_t cell=queue[at];if(cell==size_t(target))break;
        for(unsigned d=0;d<8;d+=2){const int64_t next=int64_t(cell)+delta[d];if(next<0||next>=51984)continue;
            if(!distances[size_t(next)]&&(map.cells[size_t(next)]&allowed)){
                distances[size_t(next)]=distances[cell]+1;queue.push_back(size_t(next));}}}
    if(reached)*reached=distances[size_t(target)]!=0;
    std::vector<uint8_t> reverse;if(!distances[size_t(target)])return reverse;
    size_t cell=size_t(target);unsigned exclude=8;int cx=targetX,cy=targetY;
    constexpr int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
    while(distances[cell]>1&&reverse.size()<500){int best=distances[cell];unsigned chosen=8;
        const unsigned toward=originalFigureDirectionTo(cx,cy,x,y);
        for(unsigned d=0;d<8;d+=directions==8?1:2){const int64_t next=int64_t(cell)+delta[d];if(d==exclude||next<0||next>=51984)continue;
            const int value=distances[size_t(next)];if(value&&(value<best||(value==best&&(d==toward||chosen==8)))){best=value;chosen=d;}}
        if(chosen==8)return std::vector<uint8_t>{};
        cell=size_t(int64_t(cell)+delta[chosen]);cx+=dx[chosen];cy+=dy[chosen];exclude=(chosen+4)&7;reverse.push_back(uint8_t(exclude));
    }
    if(reverse.size()>=500)return std::vector<uint8_t>{};
    std::reverse(reverse.begin(),reverse.end());return reverse;
}
inline bool originalFigureGenerateRoute(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if((f.movementMode!=1&&f.movementMode!=3&&f.movementMode!=5&&f.movementMode!=7&&f.movementMode!=17)||f.effectDraw||!context.services||context.services->directions.size()!=500000||!context.passability)return false;
    auto route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY);if(!route)return false;
    if(f.movementMode==1){ //4E8523/5B00D0(1)/5B0360:B0C flood,4 then8 backtrack.
        bool reached=false;route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0xb0c,4,&reached);
        if(reached&&route&&route->empty())route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0xb0c);
    }
    if(f.movementMode==7){ //5AFB00->5AFC00,then4E8940's4/8 backtrack.
        bool reached=false;route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0x12c,4,&reached);
        if(reached&&route&&route->empty())route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0x12c);}
    if(f.movementMode==3){
        const int64_t target=int64_t(context.passability->spawn.cropBase)+f.destinationX+228LL*f.destinationY;
        if(target<0||target>=51984)return false;
        //4E83E0 mode3:4FA3D0=true, destination B1C, road B0C flood,
        //four-direction backtrack then eight; if road flood fails, B1D.
        if(!(context.passability->cells[size_t(target)]&0xb1c))route=std::vector<uint8_t>{};
        else {bool roadReached=false;route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0xb0c,4,&roadReached);
            if(roadReached){if(route&&route->empty())route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0xb0c);}
            else route=originalFigureEngineRoute(*context.passability,f.x,f.y,f.destinationX,f.destinationY,0xb1d);}
    }
    if(f.movementMode==17){ //4E83E0->5AEA60->5AEB60/5AEBB0.
        auto map=*context.passability;const int64_t target=int64_t(map.spawn.cropBase)+f.destinationX+228LL*f.destinationY;
        if(target<0||target>=51984||context.terrain.size()!=51984||context.buildingOwners.size()!=51984||!context.buildings)return false;
        if(!(map.cells[size_t(target)]&0xb3c))route=std::vector<uint8_t>{};
        else {for(size_t cell=0;cell<51984;++cell)if((map.cells[cell]&0x13d)&&(context.terrain[cell]&0x10000000)){
                const auto owner=context.buildings->find(context.buildingOwners[cell]);
                if(owner==context.buildings->end()||(owner->second.type!=0xc2&&owner->second.type!=0xc4))map.cells[cell]=0;}
            route=originalFigureEngineRoute(map,f.x,f.y,f.destinationX,f.destinationY,0x13d);}
    }
    if(!route)return false;
    f.route=f.routeCursor=f.routeLength=0;size_t selected=0;
    for(size_t n=1;n<1000;++n){const int owner=context.services->routeOwners[n];const auto found=context.figures.find(size_t(owner));
        if(!owner||owner<0||found==context.figures.end()||found->second.route!=int(n)||found->second.alive!=1){
            if(found!=context.figures.end()&&found->second.route==int(n)&&found->second.alive!=1)found->second.route=0;
            context.services->routeOwners[n]=0;selected=n;break;}}
    if(selected&&!route->empty()){std::copy(route->begin(),route->end(),context.services->directions.begin()+selected*500);
        context.services->routeOwners[selected]=int16_t(f.slot);f.route=int16_t(selected);f.routeLength=int16_t(route->size());f.progress=20;}
    if(f.route)f.direction=context.services->directions[size_t(f.route)*500];
    else {const auto direction=originalFigureDirectionTo(f.x,f.y,f.destinationX,f.destinationY);f.direction=direction==8?8:10;}
    return true;
}
// 4E8BC0's default branch with full transport gates and cited owner predicates.
// Bridge, ferry creation and uncited building predicates refuse.
inline bool originalFigureValidateNextDirection(OriginalFigure& f,const OriginalFigureUpdateContext& context){
    if(f.direction>7)return true;
    constexpr int delta[]={-228,-227,1,229,228,227,-1,-229};const int64_t cell=int64_t(f.cell)+delta[f.direction];
    if(cell<0||cell>=51984||context.terrain.size()!=51984||context.movementMasks.size()!=51984||f.effectDraw||
        (f.type>=58&&f.type<=62)||f.type==78||f.type==79)return false;
    const auto mask=context.movementMasks[size_t(cell)];const auto t=context.terrain[size_t(cell)]&0xaffeef5fu;
    if(mask==0xffff||(mask&0x200)||(t&4)||
        (f.movementMode!=1&&f.movementMode!=3&&f.movementMode!=5&&f.movementMode!=7&&f.movementMode!=17&&f.movementMode!=18))return false;
    if(t&0x100){f.direction=9;return true;}
    if(mask&0x800)return true; //4E8D7F jumps to final terrain100 test
    if(t&(0x40|0x400)){
        if(!(t&8))return true;
        if(!context.roadblockGate)return false;
        if(!*context.roadblockGate)return true; //4E907C,zero bypasses owner gates.
        return false;
    }
    if(t&0xc000){f.direction=9;return true;}
    if(t&8){
        if(t&0x10000000){f.direction=9;return true;}
        if(context.buildingOwners.size()!=51984||!context.buildings)return false;
        const auto owner=context.buildingOwners[size_t(cell)];if(!owner)return true;
        const auto found=context.buildings->find(owner);if(found==context.buildings->end())return false;
        const auto type=found->second.type;
        if(type==0xc2||type==0xc4)return true; //4C11B0 jumps to final terrain100 gate
        if(type==0x7e)return true; //4E90AD type79 separately refused
        if(!originalFigureBuildingPassThroughFalse(type))return false;
        f.direction=9;return true;
    }
    return true;
}
// Sufficient false branches of0x4E5340 for type76: virtual+80=false,
// zero candidate attacks, or relation(category6,category2)=0 at84E5C2.
inline bool originalFigureHarmlessToPheasant(const OriginalFigure& candidate,const OriginalFigureUpdateContext& context){
    const auto model=context.models.find(candidate.type),prey=context.models.find(76);
    return candidate.runtimeClass==OriginalFigureClass::Figure&&candidate.type!=78&&candidate.type!=79&&
        model!=context.models.end()&&((model->second[2]==0&&model->second[5]==0)||
            (prey!=context.models.end()&&prey->second[0]==6&&model->second[0]==2)); //84E5C2: relation(6,2)=0.
}
// Complete, correctly linked acyclic lists are the bounded domain of the
// original repair-capable unlink/append helpers. Mutate a private context only.
inline bool originalFigureRelink(OriginalFigure& f,uint32_t cell,OriginalFigureUpdateContext& context){
    if(context.cellHeads.size()!=51984||f.cell>=51984||cell>=51984||!context.figures.count(f.slot))return false;
    const auto list=[&](uint32_t at)->std::optional<std::vector<size_t>>{
        std::vector<size_t> out;std::set<size_t> seen;
        for(size_t slot=context.cellHeads[at];slot;){const auto it=context.figures.find(slot);
            if(slot>=2000||it==context.figures.end()||!seen.insert(slot).second||it->second.cell!=at||it->second.type==1)return std::nullopt;
            out.push_back(slot);slot=it->second.next;}
        return out;};
    auto old=list(f.cell),next=list(cell);if(!old||!next||std::count(old->begin(),old->end(),f.slot)!=1)return false;
    old->erase(std::find(old->begin(),old->end(),f.slot));
    const auto write=[&](uint32_t at,const std::vector<size_t>& slots){context.cellHeads[at]=slots.empty()?0:uint16_t(slots.front());
        for(size_t n=0;n<slots.size();++n)context.figures.at(slots[n]).next=n+1<slots.size()?uint16_t(slots[n+1]):0;};
    write(f.cell,*old);if(cell==f.cell)next=old;
    next->push_back(f.slot);f.cell=cell;f.next=0;context.figures.at(f.slot)=f;write(cell,*next);return true;
}
//4E77C0: signed elevation comparisons and lookahead use the current route
//cursor (already advanced at the segment boundary), never the saved image.
inline bool originalFigureCellHeight(OriginalFigure& f,const OriginalFigureUpdateContext& context){
    if(f.cell>=51984||context.terrain.size()!=51984)return false;
    const uint32_t flags=context.terrain[f.cell];const auto prior=f.state85;
    if(flags&4)return false; //bridge virtual+84/table remains separate.
    if((flags&0x40)&&context.buildings&&context.buildingOwners.size()==51984){
        const auto owner=context.buildings->find(context.buildingOwners[f.cell]);
        if(owner!=context.buildings->end()&&owner->second.type==0x53)return false;}
    f.water=bool(flags&0x40);
    if(flags&(0x800000|0x10000400)){
        if(context.elevation.size()!=51984||!context.cropBase)return false;
        unsigned direction=f.direction<8?f.direction:f.previousDirection<8?f.previousDirection:0;
        if(f.routeCursor<f.routeLength){if(f.route<1||f.route>=1000||f.routeCursor<0||f.routeCursor>=500||
                !context.services||context.services->directions.size()!=500000)return false;
            direction=context.services->directions[size_t(f.route)*500+size_t(f.routeCursor)]&7;}
        constexpr int delta[]={-228,-227,1,229,228,227,-1,-229};
        const int64_t old=int64_t(*context.cropBase)+f.priorX+228LL*f.priorY;
        const int64_t ahead=int64_t(f.cell)+delta[direction]*((flags&0x800000)?2:1);
        if(old<0||old>=51984||ahead<0||ahead>=51984)return false;
        const auto height=[&](size_t cell){const unsigned n=context.elevation[cell];return n>127?int(n)-256:int(n);};
        const int current=height(f.cell),before=height(size_t(old)),after=height(size_t(ahead));
        if(flags&0x800000){if(!f.state85){f.state85=40;
                if(current<before){f.cartStyle=0;f.height=40;}
                else if(current<after){f.cartStyle=40;f.height=0;}
                else if(before<current){f.cartStyle=0;f.height=40;}
                else if(current>after){f.cartStyle=40;f.height=0;}}}
        else if(current<before){f.state85=20;f.cartStyle=0;f.height=40;}
        else if(current<after){f.state85=20;f.cartStyle=40;f.height=0;}
        else {f.state85=0;f.cartStyle=f.height=0;}
    }else {f.state85=0;f.height=0;}
    if(prior==100||prior==101)f.drawClass=0;
    return true;
}
inline OriginalFigureUpdateStatus originalFigureCrossCell(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(f.direction>7||context.terrain.size()!=51984||!context.cropBase||
        int64_t(*context.cropBase)+f.x+228LL*f.y!=f.cell)return OriginalFigureUpdateStatus::CellContext;
    constexpr int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
    const int64_t cell=int64_t(f.cell)+dx[f.direction]+228*dy[f.direction];
    if(cell<0||cell>=51984)return OriginalFigureUpdateStatus::CellContext;
    if((f.type!=11&&f.type!=13&&f.type!=19&&f.type!=20&&f.type!=21&&f.type!=22&&f.type!=23&&f.type!=26&&
         f.type!=28&&f.type!=29&&f.type!=30&&f.type!=34&&f.type!=35&&f.type!=36&&f.type!=39&&f.type!=40&&f.type!=45&&f.type!=47&&f.type!=76))
        return OriginalFigureUpdateStatus::MovementHeight;
    auto next=f;next.priorX=f.x;next.priorY=f.y;
    next.x=originalFigureWord(int(f.x)+dx[f.direction]);next.y=originalFigureWord(int(f.y)+dy[f.direction]);
    next.priorCell=uint32_t(int64_t(*context.cropBase)+next.priorX+228LL*next.priorY);
    if(!originalFigureRelink(next,uint32_t(cell),context))return OriginalFigureUpdateStatus::CellContext;
    if(!originalFigureCellHeight(next,context))return OriginalFigureUpdateStatus::MovementHeight;
    //0x4E57A0 has no combat work for model category0, or an otherwise empty
    // destination list. Nontrivial neighbour qualification remains explicit.
    const auto model=context.models.find(f.type);
    if(next.runtimeClass==OriginalFigureClass::Emissary||
        context.figures.at(context.cellHeads[size_t(cell)]).runtimeClass==OriginalFigureClass::Emissary)
        return OriginalFigureUpdateStatus::CellCombat;
    if(!next.hidden&&next.action!=4&&next.action!=5&&(model==context.models.end()||model->second[0]!=0)){
        for(size_t slot=context.cellHeads[size_t(cell)];slot;slot=context.figures.at(slot).next)
            if(slot!=f.slot){const auto& other=context.figures.at(slot);
                //4E5340: both Pheasants have virtual+80=false; the other's
                // model attack columns5/2 zero return before relation lookup.
                if(next.type==76&&other.type==76&&model!=context.models.end()&&model->second[2]==0&&model->second[5]==0)continue;
                const auto otherModel=context.models.find(other.type);
                const auto ordinary=[](unsigned type){switch(type){case 11:case 13:case 19:case 20:case 21:case 22:case 23:case 26:
                    case 28:case 29:case 30:case 34:case 35:case 36:case 39:case 40:case 45:case 47:return true;default:return false;}};
                //84E578+12*category+other: rows1/2 columns0/1/2 are0.
                // These types have none of4E57A0's special-pair callbacks or
                //4E5340's animal qualifications; zero relation has no writes.
                if(ordinary(next.type)&&ordinary(other.type)&&model!=context.models.end()&&otherModel!=context.models.end()&&
                    (model->second[0]==1||model->second[0]==2)&&otherModel->second[0]>=0&&otherModel->second[0]<=2)continue;
                return OriginalFigureUpdateStatus::CellCombat;}
    }
    f=next;context.figures.at(f.slot)=f;return OriginalFigureUpdateStatus::Computed;
}
//0x4E7EB0/0x4E92C0: bounded route following with explicit crossing context.
// Owner services, unresolved next-cell validation and bridge tables refuse.
inline bool originalFigureEntertainmentSegment(OriginalFigure&,OriginalFigureUpdateContext&);
inline OriginalFigureMovement originalFigureFollowRouteInContext(const OriginalFigure& saved,unsigned speed,uint32_t terrainFlags,
    OriginalFigureUpdateContext* context=nullptr){
    if(speed>255)return {OriginalFigureUpdateStatus::MovementModel,{}};
    const auto schedule=originalFigureMovementSteps(speed,saved.routePhase);
    auto f=saved;
    if(f.route<1){if(!context||!originalFigureGenerateRoute(f,*context))return {OriginalFigureUpdateStatus::RouteGeneration,{}};
        if(!originalFigureValidateNextDirection(f,*context))return {OriginalFigureUpdateStatus::NextDirection,{}};
        if(f.direction>7){f.routePhase=schedule.second;return {OriginalFigureUpdateStatus::Computed,f};}}
    if(f.direction>7||f.progress>20)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
    for(unsigned step=0;step<schedule.first;++step){
        if(f.progress>=19){
            if(!context)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
            //0x4EACD0: table service category-1 means no service-grid write.
            // A present owner needs the cited zero-return virtual chain below.
            if(f.type==39||f.type==28||f.type==23||f.type==30||f.type==35||f.type==36){if(!originalFigureEntertainmentSegment(f,*context))return {OriginalFigureUpdateStatus::SegmentService,{}};}
            else if(f.type!=11&&f.type!=13&&f.type!=19&&f.type!=20&&f.type!=21&&f.type!=22&&f.type!=26&&f.type!=40&&f.type!=45&&f.type!=47&&f.type!=76)
                return {OriginalFigureUpdateStatus::SegmentService,{}};
            if(f.owner&&f.type!=39&&f.type!=28&&f.type!=23&&f.type!=30&&f.type!=35&&f.type!=36&&f.type!=26){if(!context->buildings)return {OriginalFigureUpdateStatus::SegmentService,{}};
                const auto owner=context->buildings->find(size_t(f.owner));
                // Individually cited ordinary owner vtables:+28=429DF0,
                // +2C=42A210 returns0;4EACD0 still caps signed+1C at300.
                if(owner==context->buildings->end()||(owner->second.type!=0x21&&owner->second.type!=0x23&&owner->second.type!=0x2b&&owner->second.type!=0x35&&
                    owner->second.type!=0x36&&owner->second.type!=0x3a&&owner->second.type!=0x3e&&owner->second.type!=0xc0&&owner->second.type!=0xc1))
                    return {OriginalFigureUpdateStatus::SegmentService,{}};
                if(owner->second.serviceCount>300)owner->second.serviceCount=300;}
            if(!context->services)return {OriginalFigureUpdateStatus::NextDirection,{}};
            f.progress=20;const auto selected=originalFigureSavedRouteDirection(f,*context->services);
            if(!selected)return {OriginalFigureUpdateStatus::NextDirection,{}};
            f=selected->state;context->services=selected->services;
            if(f.direction>7){f.routePhase=schedule.second;return {OriginalFigureUpdateStatus::Computed,f};}
            if(!originalFigureValidateNextDirection(f,*context))return {OriginalFigureUpdateStatus::NextDirection,{}};
            if(f.direction>7){f.routePhase=schedule.second;return {OriginalFigureUpdateStatus::Computed,f};}
            f.routeCursor=originalFigureWord(int(f.routeCursor)+1);f.progress=0;f.previousDirection=f.direction;
        }else ++f.progress;
        if(f.progress==10){if(!context)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
            const auto crossed=originalFigureCrossCell(f,*context);if(crossed!=OriginalFigureUpdateStatus::Computed)return {crossed,{}};
            terrainFlags=context->terrain[f.cell];}
        constexpr int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
        const auto word=[](int n){const unsigned bits=unsigned(n)&65535;return int16_t(bits>32767?int(bits)-65536:int(bits));};
        f.subX=word(int(f.subX)+dx[f.direction]);f.subY=word(int(f.subY)+dy[f.direction]);
        if(!f.state85){if(f.height)--f.height;}
        else if(f.state85==100||f.state85==101)return {OriginalFigureUpdateStatus::MovementHeight,{}};
        else {--f.state85;
            if(!f.state85){f.hidden=0;f.drawClass=0;}
            else {const auto approach=[&](){if(f.height<f.cartStyle)++f.height;else if(f.height>f.cartStyle)--f.height;};
                approach();if(!(terrainFlags&0x800000))approach();}
        }
    }
    f.routePhase=schedule.second;
    return {OriginalFigureUpdateStatus::Computed,f};
}
inline OriginalFigureMovement originalFigureFollowRoute(const OriginalFigure& saved,unsigned speed,uint32_t terrainFlags,
    OriginalFigureUpdateContext* context=nullptr){
    if(!context)return originalFigureFollowRouteInContext(saved,speed,terrainFlags);
    auto local=*context;auto result=originalFigureFollowRouteInContext(saved,speed,terrainFlags,&local);
    if(result.state){if(saved.route>0||result.state->direction<8)local.roadblockGate=0;
        *context=std::move(local);}return result;
}
//0x4EACD0 ->429DF0 ->429E10 ->48B7F0 or5B3ED0.
//44E550's sixteen angular sectors, in44E770 first-match order. Inclusive
//integer cross products are exact for this radius2 query; no atan rounding.
inline int originalFigureServiceSector(int x,int y){
    if(!x&&!y)return -1;
    constexpr int boundary[17][2]={{-3,-4},{-4,-3},{-4,-1},{-4,1},{-4,3},{-3,4},{-1,4},{1,4},{3,4},
        {4,3},{4,1},{4,-1},{4,-3},{3,-4},{1,-4},{-1,-4},{-3,-4}};
    for(int n=0;n<16;++n)if(boundary[n][0]*y-boundary[n][1]*x<=0&&x*boundary[n+1][1]-y*boundary[n+1][0]<=0)return n;
    return -1;
}
//5437B0, common Market7B6F3C. 5D4A60 sums matching resources;
//5D50C0 consumes smallest positive compartments first, preserving resource
//identity when caller's clear flag is0. Market5D61C0=false, so warehouse
//accounting+288 and image+2A0 callbacks are not reached.
inline bool originalFigureMarketHouse(OriginalFigureBuildingLink& market,OriginalFigureBuildingLink& h,
    const OriginalFigureUpdateContext& context,int& count){
    if(h.type<2||h.type>17|| (h.residents<1&&h.type<11))return true;
    if(!market.marketStorage||market.marketStorage->size()!=6||!market.marketDemand||!market.marketQuality||!h.houseGoods||!h.foodQuality)return false;
    if(*market.marketQuality<0||*market.marketQuality>100)return false;
    auto& storage=*market.marketStorage;for(const auto& c:storage)if(c.resource>28||c.amount>unsigned(INT32_MAX/6)||c.capacity>INT32_MAX||(!c.resource&&c.amount))return false;
    const auto amount=[&](unsigned resource){int n=0;for(const auto& c:storage)if(c.resource==resource)n+=int(c.amount);return n;};
    const auto consume=[&](unsigned resource,int n){while(n>0){size_t selected=storage.size();uint32_t minimum=10000;
            for(size_t k=0;k<storage.size();++k)if(storage[k].resource==resource&&storage[k].amount&&storage[k].amount<minimum){selected=k;minimum=storage[k].amount;}
            if(selected==storage.size())break;const auto take=std::min(unsigned(n),storage[selected].amount);storage[selected].amount-=take;n-=int(take);}};
    auto& goods=*h.houseGoods;const int currentFood=h.accepting?goods[0]:0;
    const int maximum=h.residents?int(h.residents)*2:10,delivery=h.residents?std::max(1,int(h.residents)/2):5;
    const int entryQuality=*market.marketQuality;const bool foodAllowed=h.type<11||entryQuality>49;int food=0;
    if(currentFood<maximum&&foodAllowed&&amount(28)){food=std::min({maximum-currentFood,delivery,amount(28)});
        if(food>=0){goods[0]=originalFigureWord(int(goods[0])+food);consume(28,food);if(amount(28)<1)market.marketQuality=0;}}
    if(foodAllowed){int quality=entryQuality;const int old=*h.foodQuality;
        if(quality>0&&quality>old)h.foodQuality=uint8_t(quality);
        else if(food>0){const float ratio=currentFood<1?10.0f:float(food)/float(currentFood);
            if(ratio>3.0f)quality=(old+3*quality)/4;
            else if(ratio>2.0f)quality=(old+2*quality)/3;
            else if(ratio>0.5f)quality=(old+quality)/2;
            else if(ratio>0.33f)quality=(quality+2*old)/3;
            else quality=(quality+3*old)/4;
            h.foodQuality=uint8_t(quality);}}
    //857334 six64-byte descriptors: resource,house index,market index,model column.
    constexpr unsigned resources[]={19,13,25,23,22,24},indices[]={1,6,2,4,5,3},columns[]={9,11,10,13,13,12};
    const int model=h.houseModel==7?7:h.type==11?h.houseModel+2:(h.houseModel==14||h.houseModel==8)?h.houseModel:h.houseModel+1;
    for(unsigned k=0;k<6;++k){int wanted=10;
        if(h.residents||k>1){const auto col=context.houseMarketModels.find(columns[k]);if(col==context.houseMarketModels.end())return false;
            const auto row=col->second.find(model);if(row==col->second.end())return false;
            wanted=row->second*(h.size>1?4:1);if(h.type>10)wanted*=2;}
        auto& counter=market.marketDemand->at(k+1);counter=originalFigureWord(int(counter)-1);if(counter<0)counter=0;
        if(!wanted)continue;counter=2000;const int available=amount(resources[k]);if(!available)continue;
        const int limit=std::max(10,wanted);int give=limit-goods[indices[k]];
        if(give<1||give>available)give=wanted-goods[indices[k]];
        give=std::min(limit,std::max(0,give));if(give>0){give=std::min(give,available);
            goods[indices[k]]=originalFigureWord(int(goods[indices[k]])+give);consume(resources[k],give);}
    }
    int total=0;for(const auto& c:storage)total+=int(c.amount);market.marketGoods=total;++count;return true;
}
// Full clipped radius1 box then radius2 box, preserving repeated callbacks,
// wall shadow sectors and signed service cap (429E10/42A1A0).
inline bool originalFigureEntertainmentSegment(OriginalFigure& f,OriginalFigureUpdateContext& context){
    const bool watch=f.type==29,herb=f.type==30,market=f.type==23,inspector=f.type==39,shrine=f.type==35,well=f.type==28;
    if((f.type!=36&&!watch&&!herb&&!market&&!inspector&&!shrine&&!well)||!context.buildings||!context.services||context.services->serviceGrid.size()!=51984||
        context.buildingOwners.size()!=51984||context.feature.size()!=51984||context.terrain.size()!=51984||
        !context.cropBase||!context.activeSide||(!watch&&!herb&&!market&&!inspector&&!shrine&&!well&&!context.festivalVenue))return false;
    auto owner=context.buildings->find(size_t(f.owner));if(owner==context.buildings->end()||
        (watch?(owner->second.type!=0x7f||!context.watchtowerReduction||!context.watchMaximum):herb?owner->second.type!=0xcf:market?owner->second.type!=0x3b:
            inspector?(owner->second.type!=0x7c||!context.inspectorReduction||!context.inspectorMaximum):
             shrine?(owner->second.type!=0xd6||!owner->second.shrineReligion||!owner->second.shrineElite||*owner->second.shrineReligion>=4):well?owner->second.type!=0x48:
            (owner->second.type!=0x47||!owner->second.entertainment)))return false;
    int count=0;std::array<int,16> shadows{};
    for(int radius=1;radius<=2;++radius)for(int y=std::max(0,int(f.y)-radius);y<=std::min(int(*context.activeSide)-1,int(f.y)+radius);++y)
        for(int x=std::max(0,int(f.x)-radius);x<=std::min(int(*context.activeSide)-1,int(f.x)+radius);++x){
            const int64_t cell=int64_t(*context.cropBase)+x+228LL*y;if(cell<0||cell>=51984)return false;
            const unsigned slot=context.buildingOwners[size_t(cell)];if(!slot)continue;
            auto house=context.buildings->find(slot);if(house==context.buildings->end())return false;auto& h=house->second;
            const int dx=x-int(f.x),dy=y-int(f.y),sector=originalFigureServiceSector(dx,dy);
            const bool occludes=(context.terrain[size_t(cell)]&1)||((context.feature[size_t(cell)]&4)&&
                ((context.terrain[size_t(cell)]&0x4000)||h.type==0xe7||h.type==0xe8||
                 h.type==0x59||h.type==0x5a||h.type==0x5b||h.type==0x68||h.type==0x69||h.type==0x6a));
            if(occludes&&sector>=0){if(!shadows[size_t(sector)]||radius<shadows[size_t(sector)])shadows[size_t(sector)]=radius;
                if(radius==1&&!(sector&1)){
                    if(shadows[size_t((sector+2)&15)]==1)shadows[size_t((sector+1)&15)]=1;
                    if(shadows[size_t((sector+14)&15)]==1)shadows[size_t((sector+15)&15)]=1;}
                continue;}
            if(sector>=0&&shadows[size_t(sector)]&&std::abs(dx)+std::abs(dy)>shadows[size_t(sector)])continue;
            if(inspector){auto* target=&h;unsigned hops=0;
                while(target->parent>0){auto parent=context.buildings->find(size_t(target->parent));if(parent==context.buildings->end()||++hops>=500)return false;target=&parent->second;}
                if(target->type==0x47){if(!target->inspectorParent)return false;auto parent=context.buildings->find(*target->inspectorParent);
                    if(parent==context.buildings->end())return false;target=&parent->second;}
                //4B3930's ordinary identity branch; unported monument parent chains refuse.
                if(target->type==0x82||target->type==0x83||target->type==0x84)return false;
                auto& maximum=*context.inspectorMaximum;maximum[0]=std::max(maximum[0],int32_t(target->fireRisk));
                maximum[1]=std::max(maximum[1],int32_t(target->damageRisk));
                target->fireRisk=std::max<int16_t>(0,originalFigureWord(int(target->fireRisk)-int16_t(*context.inspectorReduction)));
                target->damageRisk=std::max<int16_t>(0,originalFigureWord(int(target->damageRisk)-int16_t(*context.inspectorReduction)));
                //House+B8=42DD40 reads accepting+9; cited ordinary vtables use413A00.
                if(target->type>=2&&target->type<18){if(target->accepting&&target->residents>0)++count;}
                else if(!originalFigureBuildingPassThroughFalse(target->type)&&target->type!=0x3b)return false;
                continue;}
            if(market){if(!originalFigureMarketHouse(owner->second,h,context,count))return false;continue;}
            if(shrine){if((h.alive!=1&&h.alive!=3)||h.type<2||h.type>=18||!h.accepting||(h.residents<1&&h.type<11)||
                    (*owner->second.shrineElite&&h.type<11))continue;
                if(!h.religion)return false;h.religion->at(*owner->second.shrineReligion)=40;++count;continue;}
            if((h.alive!=1&&h.alive!=3)||h.type<2||h.type>=18||!h.accepting||h.residents<1)continue;
            if(well){if(!h.waterService)return false;unsigned index=1;
                if(owner->second.houseModel<=0&&!owner->second.upgrade){if(!context.waterOverride)return false;index=*context.waterOverride?1:0;}
                h.waterService->at(index)=96;++count;continue;}
            if(herb){if(!h.herbalist)return false;h.herbalist=96;++count;continue;}
            if(watch){const auto floor=context.houseWatchFloors.find(h.houseModel);if(floor==context.houseWatchFloors.end())return false;
                context.watchMaximum=std::max(*context.watchMaximum,h.watchRisk);
                const uint32_t bits=uint32_t(h.watchRisk)-uint32_t(*context.watchtowerReduction);
                const int32_t reduced=int32_t(bits>INT32_MAX?int64_t(bits)-4294967296LL:bits);
                h.watchRisk=std::min(100,std::max(floor->second,reduced));++count;continue;}
            if(!h.entertainment)return false;
            if(*context.festivalVenue==f.owner){
                if(!context.entertainmentAvailable||std::any_of(context.entertainmentAvailable->begin(),context.entertainmentAvailable->end(),[](int n){return n!=0;}))return false;
            }else for(unsigned n=0;n<3;++n)if(owner->second.entertainment->at(n))h.entertainment->at(n)=96;
            ++count;
        }
    owner->second.serviceCount=originalFigureWord(int(owner->second.serviceCount)+count);
    if(owner->second.serviceCount>300)owner->second.serviceCount=300;
    if(watch){if(*context.watchMaximum>f.watchCoverage)f.watchCoverage=uint8_t(*context.watchMaximum);
        else f.watchCoverage=f.watchCoverage>10?uint8_t(f.watchCoverage-10):0;}
    if(inspector){const int32_t maximum=std::max(context.inspectorMaximum->at(0),context.inspectorMaximum->at(1));
        if(maximum>f.watchCoverage)f.watchCoverage=uint8_t(maximum);else f.watchCoverage=f.watchCoverage>10?uint8_t(f.watchCoverage-10):0;}
    if(f.cell>=51984)return false;
    if(watch)context.services->serviceGrid[f.cell]|=7u<<6;
    else if(herb||well)context.services->serviceGrid[f.cell]|=7u;
    else if(inspector)context.services->serviceGrid[f.cell]|=7u<<3;
    else if(market)context.services->serviceGrid[f.cell]|=7u<<21;
    else if(shrine)context.services->serviceGrid[f.cell]|=7u<<15;
    else context.services->serviceGrid[f.cell]|=7u<<9; //category3,4B9460.
    return true;
}
//4B9590 cardinal road exits. Preserve the original grid-category minimum
// filtering at >2 exits. Building/roadblock special paths remain refused.
inline std::optional<std::array<bool,8>> originalFigureRoamRoads(const OriginalFigure& f,const OriginalFigureUpdateContext& context){
    if(context.terrain.size()!=51984||!context.services||context.services->serviceGrid.size()!=51984)return std::nullopt;
    std::array<bool,8> roads{};constexpr int delta[]={-228,-227,1,229,228,227,-1,-229};unsigned count=0,min=7;
    const unsigned shift=f.type==29?6:(f.type==28||f.type==30)?0:f.type==39?3:f.type==23?21:f.type==35?15:9; //84E78C:2,0,1,7,5,3.
    for(unsigned d=0;d<8;d+=2){const int64_t cell=int64_t(f.cell)+delta[d];if(cell<0||cell>=51984)return std::nullopt;
        const uint32_t t=context.terrain[size_t(cell)];
        roads[d]=(t&0x40)!=0;
        if(roads[d]&&(t&8)){if(context.buildingOwners.size()!=51984||!context.buildings)return std::nullopt;
            const auto found=context.buildings->find(context.buildingOwners[size_t(cell)]);if(found==context.buildings->end())return std::nullopt;
            const auto type=found->second.type;
            if(type==0x82||type==0x7e)roads[d]=false;
            else if(type==0xe8||type==0x68||type==0x69||type==0x6a)return std::nullopt; //415770/+278.
        }
        if(t&0x100)roads[d]=false;
        count+=roads[d];if(roads[d])min=std::min(min,(context.services->serviceGrid[size_t(cell)]>>shift)&7);}
    if(count>2)for(unsigned d=0;d<8;d+=2)if(roads[d]){
        const auto cell=size_t(int64_t(f.cell)+delta[d]);if(((context.services->serviceGrid[cell]>>shift)&7)>min)roads[d]=false;}
    return roads;
}
//4E71D0 service-category branch: raw cardinal road bits, minimum grid
//score, original nonzero-remainder tie replacement, then one turn-sign draw.
inline bool originalFigureRoamTargetTurn(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(!context.services||context.services->serviceGrid.size()!=51984||context.terrain.size()!=51984)return false;
    const unsigned shift=f.type==29?6:(f.type==28||f.type==30)?0:f.type==39?3:f.type==23?21:f.type==35?15:9;
    if(f.type!=28&&f.type!=29&&f.type!=30&&f.type!=39&&f.type!=23&&f.type!=35&&f.type!=36)return false;
    constexpr int delta[]={-228,1,228,-1};unsigned minimum=8,ties=1;f.direction=0;
    for(unsigned k=0;k<4;++k){const int64_t cell=int64_t(f.cell)+delta[k];if(cell<0||cell>=51984)return false;
        if(!(context.terrain[size_t(cell)]&0x40))continue;
        const unsigned score=(context.services->serviceGrid[size_t(cell)]>>shift)&7;
        if(score<minimum){minimum=score;ties=2;f.direction=uint8_t(2*k);}
        else if(score==minimum){const unsigned random=originalFigureRandomNext(*context.services)&127; //010C713C,not the15-bit010C7138.
            ++ties;
            if(random%ties)f.direction=uint8_t(2*k);}}
    f.roamTurn=originalFigureRandomNext(*context.services)&1?254:2;f.roamCountdown=5;return true;
}
//4E6690 uses terrain440 before the later4B9590 category filter. Only
//individually checked +E8=416A70 owners are admitted;unknown gates refuse.
inline std::optional<int> originalFigureInitialRoamDirection(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(context.terrain.size()!=51984||!context.buildings||context.buildingOwners.size()!=51984)return std::nullopt;
    std::array<bool,8> roads{};constexpr int delta[]={-228,1,228,-1};unsigned count=0;
    for(unsigned k=0;k<4;++k){const int64_t cell=int64_t(f.cell)+delta[k];if(cell<0||cell>=51984)return std::nullopt;
        const uint32_t terrain=context.terrain[size_t(cell)];bool open=(terrain&0x440)!=0;
        if(terrain&8){const auto b=context.buildings->find(context.buildingOwners[size_t(cell)]);if(b==context.buildings->end())return std::nullopt;
            if(!originalFigureBuildingPassThroughFalse(b->second.type)&&b->second.type!=0x3b)return std::nullopt;
            if(b->second.type==0x7e)open=false;}
        roads[2*k]=open;count+=open;}
    if(count==1){for(unsigned d=0;d<8;d+=2)if(roads[d]){f.direction=uint8_t(d);return int(d);}}
    int reverse=(f.previousDirection+4)&7;
    if(count==2){for(unsigned attempt=0;attempt<5;++attempt){if(f.direction<8&&roads[f.direction]&&f.direction!=reverse)return int(f.direction);
            const int d=int(int8_t(f.direction))+int(int8_t(f.roamTurn));f.direction=uint8_t(d>6?0:d<0?6:d);}
        return -1;}
    if(context.randomByte.size()!=51984||f.cell>=51984)return std::nullopt;
    f.direction=uint8_t((context.randomByte[f.cell]+f.roamTurns)&6);
    if(roads[f.direction]&&f.direction!=reverse)return int(f.direction);
    f.roamCountdown=uint8_t(f.roamCountdown-1);
    if(int8_t(f.roamCountdown)<=0){if(!originalFigureRoamTargetTurn(f,context))return std::nullopt;reverse=-1;}
    for(unsigned attempt=0;attempt<5;++attempt){if(roads[f.direction]&&f.direction!=reverse)return int(f.direction);
        const int d=int(f.direction)+int(int8_t(f.roamTurn));f.direction=uint8_t(d>6?0:d<0?6:d);}
    return -1;
}
//4E6A70: four attempts, owner+38 mutation, clipped home-road target.
//Market3B/3C uses544910 and remains outside this ordinary-owner branch.
inline bool originalFigureBeginRoam(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(!context.buildings||!context.activeSide||!*context.activeSide)return false;
    auto b=context.buildings->find(size_t(f.owner));if(b==context.buildings->end()||b->second.type==0x3b||b->second.type==0x3c)return false;
    f.progress=20;f.roamState=0;f.roamCountdown=255;f.roamTurn=2;f.roamOriginX=f.x;f.roamOriginY=f.y;
    for(unsigned attempt=0;attempt<4;++attempt){const auto d=originalFigureInitialRoamDirection(f,context);if(!d)return false;
        if(*d<0)continue;b->second.roamDirection=*d>6?0:uint8_t(*d);
        f.destinationX=int16_t(std::clamp(int(b->second.roadX),0,int(*context.activeSide)-1));
        f.destinationY=int16_t(std::clamp(int(b->second.roadY),0,int(*context.activeSide)-1));break;}
    f.roamState=1;return true;
}
//0x4E6B70 ->0x4E6D80, including the bounded entertainment segment.
inline OriginalFigureMovement originalFigureRoam(const OriginalFigure& saved,unsigned speed,uint32_t terrainFlags,
    OriginalFigureUpdateContext& context){
    if(!saved.roamState||saved.direction>7||saved.progress>20)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
    auto local=context;auto f=saved;const auto schedule=originalFigureMovementSteps(speed,f.routePhase);
    f.roamSteps=originalFigureWord(int(f.roamSteps)+int(speed));
    for(unsigned n=0;n<schedule.first;++n){if(f.progress>=19){
            f.progress=20;f.roamTurns=uint8_t(f.roamTurns+1);
            if(!originalFigureEntertainmentSegment(f,local))return {OriginalFigureUpdateStatus::SegmentService,{}};
            const auto roads=originalFigureRoamRoads(f,local);if(!roads)return {OriginalFigureUpdateStatus::NextDirection,{}};
            const unsigned count=unsigned(std::count(roads->begin(),roads->end(),true));
            if(!count){f.direction=9;f.roamSteps=f.roamLimit;f.routePhase=schedule.second;context=std::move(local);return {OriginalFigureUpdateStatus::Computed,f};}
            if(count==1){for(unsigned d=0;d<8;d+=2)if((*roads)[d])f.direction=uint8_t(d);}
            else if(count==2){int reverse=(f.previousDirection+4u)&7;
                if(f.roamCountdown==255){if(!originalFigureRoamTargetTurn(f,local))return {OriginalFigureUpdateStatus::NextDirection,{}};reverse=-1;}
                if(!f.roamTurn)f.roamTurn=2;unsigned attempt=0;
                for(;attempt<4;++attempt){if((*roads)[f.direction]&&f.direction!=reverse)break;
                    int d=int(f.direction)+int(int8_t(f.roamTurn));if(d>6)d=0;if(d<0)d=6;f.direction=uint8_t(d);}
                if(attempt==4)return {OriginalFigureUpdateStatus::NextDirection,{}};
            }else {if(local.randomByte.size()!=51984)return {OriginalFigureUpdateStatus::NextDirection,{}};
                int reverse=(f.previousDirection+4u)&7;f.direction=uint8_t((local.randomByte[f.cell]+f.roamTurns)&6);
                if(!(*roads)[f.direction]||f.direction==reverse){f.roamCountdown=uint8_t(f.roamCountdown-1);
                    if(int8_t(f.roamCountdown)<=0){if(!originalFigureRoamTargetTurn(f,local))return {OriginalFigureUpdateStatus::NextDirection,{}};reverse=-1;}
                    for(unsigned attempt=0;attempt<4;++attempt){if((*roads)[f.direction]&&f.direction!=reverse)break;
                        const int d=int(f.direction)+int(int8_t(f.roamTurn));f.direction=uint8_t(d>6?0:d<0?6:d);}}}
            f.routeCursor=originalFigureWord(int(f.routeCursor)+1);f.previousDirection=f.direction;f.progress=0;
        }else ++f.progress;
        //0x4E6D80 moves subcoordinates/height BEFORE the progress10 crossing.
        auto interior=f;interior.progress=0;interior.route=1;interior.routePhase=0;
        const auto step=originalFigureFollowRoute(interior,6,terrainFlags);if(!step.state)return step;
        f.subX=step.state->subX;f.subY=step.state->subY;f.height=step.state->height;f.state85=step.state->state85;
        f.hidden=step.state->hidden;f.drawClass=step.state->drawClass;
        if(f.progress==10){const auto crossed=originalFigureCrossCell(f,local);if(crossed!=OriginalFigureUpdateStatus::Computed)return {crossed,{}};
            terrainFlags=local.terrain[f.cell];}
    }
    f.routePhase=schedule.second;context=std::move(local);return {OriginalFigureUpdateStatus::Computed,f};
}
//0x4E9620 single free step; exhaustion is tested BEFORE movement. The caller
// handles arrival writes. Unlink/append happens even without a cell change.
inline std::optional<std::pair<OriginalFigure,bool>> originalFigureFreeStep(const OriginalFigure& saved,
    OriginalFigureUpdateContext& context){
    if(!context.cropBase)return std::nullopt;auto local=context;auto f=saved;
    if(f.raise)--f.raise;const bool arrived=int(f.freeRemainingX)+f.freeRemainingY<1;
    if(!arrived){
        if(f.freeAxis==1){f.freeError=originalFigureWord(int(f.freeError)+2*(f.freeError<0?int(f.freeRemainingY):int(f.freeRemainingY)-f.freeRemainingX));
            f.freeRemainingX=originalFigureWord(int(f.freeRemainingX)-1);}
        else {f.freeError=originalFigureWord(int(f.freeError)+2*(f.freeError<0?int(f.freeRemainingX):int(f.freeRemainingX)-f.freeRemainingY));
            f.freeRemainingY=originalFigureWord(int(f.freeRemainingY)-1);}
        const auto approach=[](int16_t& value,int16_t target){if(value<target)++value;else if(value>target)--value;};
        if(f.freeAxis==2){approach(f.subY,f.freeTargetY);if(f.freeError>=0){f.freeRemainingX=originalFigureWord(int(f.freeRemainingX)-1);approach(f.subX,f.freeTargetX);}}
        else {approach(f.subX,f.freeTargetX);if(f.freeError>=0){f.freeRemainingY=originalFigureWord(int(f.freeRemainingY)-1);approach(f.subY,f.freeTargetY);}}
    }
    f.x=int16_t(f.subX/20);f.y=int16_t(f.subY/20);f.cellMovement=0;
    const int64_t cell=int64_t(*context.cropBase)+f.x+228LL*f.y;if(cell<0||cell>=51984)return std::nullopt;
    if(!originalFigureRelink(f,uint32_t(cell),local))return std::nullopt;
    context=std::move(local);return std::make_pair(f,arrived);
}
//4E98A0,shared non-reversed integer free-movement initialization.
inline void originalFigureFreeTarget(OriginalFigure& f){
    f.freeTargetX=originalFigureWord(int(f.destinationX)*20);f.freeTargetY=originalFigureWord(int(f.destinationY)*20);
    f.freeRemainingX=originalFigureWord(std::abs(int(f.freeTargetX)-f.subX));f.freeRemainingY=originalFigureWord(std::abs(int(f.freeTargetY)-f.subY));
    f.freeError=f.freeRemainingX==f.freeRemainingY?0:originalFigureWord(2*std::min(int(f.freeRemainingX),int(f.freeRemainingY))-std::max(int(f.freeRemainingX),int(f.freeRemainingY)));
    f.direction=originalFigureDirectionTo(f.subX,f.subY,f.freeTargetX,f.freeTargetY);
    if(2*int(f.freeRemainingX)<f.freeRemainingY){if(f.direction==1||f.direction==7)f.direction=0;else if(f.direction==3||f.direction==5)f.direction=4;}
    if(2*int(f.freeRemainingY)<f.freeRemainingX){if(f.direction==1||f.direction==3)f.direction=2;else if(f.direction==5||f.direction==7)f.direction=6;}
    f.freeAxis=f.freeRemainingY<=f.freeRemainingX?1:2;
}
//4BA580 radius0..2. Music48AA.. uses only success, then targets its
//owner's serialized road coordinates, not the query's ranked scratch target.
inline std::optional<bool> originalFigureMusicRoad(const OriginalFigureBuildingLink& b,const OriginalFigureUpdateContext& context){
    if(!context.cropBase||!context.activeSide||context.terrain.size()!=51984||context.buildingOwners.size()!=51984||!context.buildings)return std::nullopt;
    for(int radius=0;radius<=2;++radius){bool found=false;
        for(int y=std::max(0,int(b.y)-radius);y<=std::min(int(*context.activeSide)-1,int(b.y)+b.size-1+radius);++y)
            for(int x=std::max(0,int(b.x)-radius);x<=std::min(int(*context.activeSide)-1,int(b.x)+b.size-1+radius);++x){
                const size_t cell=*context.cropBase+y*228+x;const auto owner=context.buildings->find(context.buildingOwners[cell]);
                if(owner!=context.buildings->end()&&(owner->second.type==0x6f||owner->second.type==0x71))return std::nullopt;
                found|=(context.terrain[cell]&0x44)==0x40;}
        if(found)return true;}
    return false;
}
inline OriginalFigureImageState originalFigureUpdateImageState(const OriginalFigureUpdateContext& context){
    OriginalFigureImageState out;out.factionModels=context.factionModels;
    for(const auto& entry:context.figures)out.figures.emplace(entry.first,originalFigureImageInputs(entry.second));return out;
}
inline bool originalFigureWriteWalkingImage(OriginalFigure& f,uint32_t code,
    const std::map<uint32_t,const SG3Archive*>& archives,unsigned view,bool advance=false,bool fallback=true,bool clamped=false){
    const auto group=originalFigureImageGroup(code,archives);if(!group||view>6||(view&1)||f.direction>127||f.previousDirection>127)return false;
    if(advance){const auto frame=originalFigureAdvanceFrame(f.frame,group->second->animationSpriteCount,group->second->animationSpeedId,0,{});
        if(!frame)return false;f.frame=frame->counter;}
    int direction=f.direction;if(fallback&&direction>=8)direction=f.previousDirection;
    direction-=int(view);if(direction<0)direction+=8;
    unsigned frame=f.frame,dirs=8;
    if(clamped){if(group->second->animationDirections<1||group->second->animationSpriteCount<1)return false;
        dirs=unsigned(group->second->animationDirections);direction=std::min(direction,int(dirs)-1);frame=std::min(frame,unsigned(group->second->animationSpriteCount)-1);}
    const uint64_t image=uint64_t(group->first)+frame*dirs+unsigned(direction);
    if(image>>14!=group->first>>14||(image&0x3fff)>=archives.at(code>>9)->sprites.size())return false;
    f.image=uint32_t(image);return true;
}
inline void originalFigurePublishWrites(OriginalFigureFirstUpdate& out,const OriginalFigure& f,
    const OriginalFigureUpdateContext& before,OriginalFigureUpdateContext& after){
    after.figures.at(f.slot)=f;
    for(const auto& entry:after.figures){const auto& prior=before.figures.at(entry.first);
        if(entry.first!=f.slot&&(entry.second.next!=prior.next||entry.second.secondaryImage!=prior.secondaryImage||entry.second.route!=prior.route||
            entry.second.alive!=prior.alive||entry.second.action!=prior.action||entry.second.direction!=prior.direction||entry.second.counter!=prior.counter||
            entry.second.previousAction!=prior.previousAction||entry.second.target!=prior.target||
            entry.second.flags120!=prior.flags120||entry.second.linkedBuilding!=prior.linkedBuilding))out.linkedWrites.emplace(entry);}
    if(before.cellHeads!=after.cellHeads)out.cellHeads=after.cellHeads;
    out.watchMaximum=after.watchMaximum;
    out.inspectorMaximum=after.inspectorMaximum;
    out.goods=after.goods;
    out.freeSlots=after.freeSlots;out.crtRandom=after.crtRandom;out.crtRandomCalls=after.crtRandomCalls;
    out.allocatedSlots=after.allocatedSlots;out.highestLiveSlot=after.highestLiveSlot;
    out.roadblockGate=after.roadblockGate;
    out.services=after.services;out.buildings=after.buildings;out.state=f;out.status=OriginalFigureUpdateStatus::Computed;
}
inline bool originalFigureBulkGoods(const OriginalFigure& f){const unsigned r=f.subtype?f.subtype:f.resource;return r==10||r==18||r==20;}
inline bool originalFigureSpecialGoods(const OriginalFigure& f){const unsigned r=f.subtype?f.subtype:f.resource;
    return originalFigureBulkGoods(f)||r==11||r==14||r==15||r==16||r==17||r==21||r==23||r==25||r==26||r==27;}
//5D50C0 returns the amount NOT removed. Smallest positive compartment,
//first tie, then repeat. 5D2760 clears identity only when clear is true.
inline std::optional<unsigned> originalFigureStorageWithdraw(OriginalFigureBuildingLink& b,unsigned resource,
    unsigned wanted,bool clear,OriginalFigureGoodsServices& goods){
    if((b.type!=0x36&&b.type!=0x3a)||!b.storage||!b.storagePolicy||*b.storagePolicy>=goods.policies.size()||resource>28)return std::nullopt;
    for(const auto& c:*b.storage)if(c.capacity!=400||c.amount>400||c.resource>28||(!c.resource&&c.amount))return std::nullopt;
    while(wanted){size_t selected=b.storage->size();uint32_t minimum=10000;
        for(size_t k=0;k<b.storage->size();++k){const auto& c=b.storage->at(k);
            if(c.resource==resource&&c.amount&&c.amount<minimum){minimum=c.amount;selected=k;}}
        if(selected==b.storage->size())break;
        auto& c=b.storage->at(selected);const unsigned take=std::min(wanted,c.amount);c.amount-=take;wanted-=take;
        if(!c.amount&&clear)c.resource=0;
        if(b.type==0x36)goods.warehouse[resource]-=take; //vtable7BE1BC+288=5D50A0.
        else {const auto mode=goods.policies[*b.storagePolicy].trade[resource];
            if(mode==7)goods.trade7[resource]-=take;else if(mode==5)goods.trade5[resource]-=take;}
    }
    return wanted;
}
// Warehouse7BE1BC+154=5D4E80: policy gate, then fullest compatible bay
// (first tie) one unit at a time. +284=5D5080 credits each attempted unit;
// 5D2790 assigns identity and clamps to capacity. Existing scene selector
// rebinds changed compartments; no stock-based image formula is added here.
inline std::optional<unsigned> originalFigureWarehouseDeliver(OriginalFigureBuildingLink& b,unsigned resource,
    unsigned amount,OriginalFigureGoodsServices& goods){
    if(b.type!=0x36||!b.storage||b.storage->size()!=8||!b.storagePolicy||*b.storagePolicy>=goods.policies.size()||resource>28||amount>32767)return std::nullopt;
    for(const auto& c:*b.storage)if(c.capacity!=400||c.amount>400||c.resource>28||(!c.resource&&c.amount))return std::nullopt;
    if((b.alive!=1&&b.alive!=3)||b.workers<=0||b.suppress)return 0;
    const auto& policy=goods.policies[*b.storagePolicy];
    const auto stock=[&](unsigned r){unsigned n=0;for(const auto& c:*b.storage)if(c.resource==r&&c.resource)n+=c.amount;return n;};
    const auto rejects=[&](unsigned r,unsigned n){return (policy.accept[r]!=0&&policy.accept[r]!=2)||
        (n?int64_t(stock(r))+n>policy.limit[r]:int64_t(stock(r))>=policy.limit[r]);};
    if(rejects(resource,0))return 0;
    unsigned withheld=0;
    if(rejects(resource,amount)){const int64_t n=int64_t(stock(resource))+amount-policy.limit[resource];
        if(n<0||n>amount)return std::nullopt;withheld=unsigned(n);}
    const unsigned wanted=amount-withheld;if(!wanted)return 0;
    for(unsigned n=0;n<wanted;++n){size_t selected=b.storage->size();int maximum=-1;
        for(size_t k=0;k<b.storage->size();++k){const auto& c=b.storage->at(k);
            if(((c.resource==0&&c.amount==0)||c.resource==resource)&&c.amount<c.capacity&&int(c.amount)>maximum){maximum=int(c.amount);selected=k;}}
        if(selected==b.storage->size())return 0; //original retains partial bay/accounting writes but returns0.
        auto& c=b.storage->at(selected);++goods.warehouse[resource];c.resource=resource;++c.amount;
    }
    return wanted;
}
//506F2E ->4E7FD0 ->4E25A0 allocates even with zero candidates. Retain
//stale-owner cleanup;5B04A0 with count0 has no target or route-buffer write.
inline bool originalFigureEmptyGoodsRoute(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(!context.services||context.services->directions.size()!=500000||f.effectDraw||f.movementMode!=1)return false;
    f.route=f.routeCursor=f.routeLength=0;
    for(size_t n=1;n<1000;++n){const auto owner=context.services->routeOwners[n];const auto it=context.figures.find(size_t(owner));
        if(!owner||it==context.figures.end()||it->second.route!=int(n)||it->second.alive!=1){
            if(it!=context.figures.end()&&it->second.route==int(n)&&it->second.alive!=1)it->second.route=0;
            context.services->routeOwners[n]=0;break;}}
    return true;
}
inline std::optional<size_t> originalFigureGoodsSupplier(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(!context.goods||!context.goods->storageSearch||!context.buildings||!context.goods->godGate||!context.cropBase)return std::nullopt;
    auto& goods=*context.goods;const unsigned resource=f.subtype;if(resource>28)return std::nullopt;
    auto& cache=goods.storageSearch->at(resource);unsigned noLabor=0;
    if(resource>0&&resource<10&&!*goods.godGate&&!goods.unavailable[resource]&&cache!=1){
        if(!cache||(cache&0x1c))cache=uint8_t((cache&0x1c)|4);
        for(auto& entry:*context.buildings){auto& b=entry.second;if(b.type!=0x35||(b.alive!=1&&b.alive!=3)||!b.roadAccess||b.roadDistance<=0)continue;
            const auto required=context.buildingEmployment.find(b.type);if(required==context.buildingEmployment.end()||!b.storage||!b.storagePolicy||*b.storagePolicy>=goods.policies.size())return std::nullopt;
            if(b.storageLaborDisabled||required->second<=0||int64_t(b.workers)*100/required->second<=0){++noLabor;continue;}
            int amount=0,occupied=0;for(const auto& c:*b.storage){if(c.amount>400||c.resource>28)return std::nullopt;
                occupied+=bool(c.resource||c.amount);if(c.resource==resource)amount+=int(c.amount);}
            const auto& policy=goods.policies[*b.storagePolicy];
            if((amount%400==0&&occupied==int(b.storage->size()))||amount>=policy.limit[resource])continue;
            cache=2;if(policy.accept[resource]!=0&&policy.accept[resource]!=2)continue;
            if(b.incomingFigures>0&&int(b.incomingAmount)+policy.limit[resource]-amount<=0)continue;
            // Positive Mill selection also depends on the original road
            // component ranks and vector order, which are not carried here.
            return std::nullopt;}
    }
    //506CD0: the supplied industrial vector's Hunter/Clay have both input
    //predicates0;Kiln5F5130 main resource18,secondary4FA3F0=0. Rejecting
    //a different resource happens before capacity/scoring/route side effects.
    if(!goods.unavailable[resource]){if(!goods.industrySearch)return std::nullopt;
        auto& industry=goods.industrySearch->at(resource);if(industry!=1){industry=1;
            for(const auto& e:*context.buildings){const auto& b=e.second;
                if(b.type>=0x21&&b.type<=0x2e&&b.alive==1&&b.destinationEnabled){
                    if(b.type!=0x21&&b.type!=0x23&&b.type!=0x2b)return std::nullopt;
                    if(b.type==0x2b&&resource==18)return std::nullopt;}}
            //506F2E invokes4E7FD0 even for an empty candidate vector.
            if(!originalFigureEmptyGoodsRoute(f,context))return std::nullopt;}}
    //5D2F60 first asks accepting mode2,then0.5D31A0 asks trade mode7.
    //Positive candidates require further route/accounting predicates;none
    //of those are elided by accepting a no-candidate result.
    for(unsigned query=0;query<3;++query){if(cache==1)break;
        const uint8_t bit=query<2?8:16;if(!cache||(cache&0x1c))cache=uint8_t((cache&0x1c)|bit);
        for(const auto& e:*context.buildings){const auto& b=e.second;
            if(query==2&&b.type==0x38&&b.alive==1&&b.roadAccess&&b.roadDistance>0)return std::nullopt; //5DDAF0 admits38 too.
            if(b.alive!=1||b.type!=(query<2?0x36:0x3a)||!b.roadAccess||b.roadDistance<=0)continue;
            const auto required=context.buildingEmployment.find(b.type);if(required==context.buildingEmployment.end())return std::nullopt;
            if(b.storageLaborDisabled||required->second<=0||int64_t(b.workers)*100/required->second<=0){++noLabor;continue;}
            if(!b.storage||!b.storagePolicy||*b.storagePolicy>=goods.policies.size())return std::nullopt;
            int amount=0,occupied=0;for(const auto& c:*b.storage){occupied+=bool(c.resource||c.amount);if(c.resource==resource)amount+=int(c.amount);}
            const auto& policy=goods.policies[*b.storagePolicy];
            if((amount%400==0&&occupied==int(b.storage->size()))||amount>=policy.limit[resource])continue;
            cache=2;const auto mode=query<2?policy.accept[resource]:policy.trade[resource];
            if(mode==(query==0?2:query==1?0:7))return std::nullopt;
        }}
    //4E29E6 retries Mill in mode1 even when unavailable[resource]!=0.
    // The bounded unavailable branch refuses rather than inventing candidates.
    if(resource>0&&resource<10&&!*goods.godGate&&cache!=1){
        if(goods.unavailable[resource])return std::nullopt;
        for(const auto& e:*context.buildings){const auto& b=e.second;
            if(b.type==0x35&&(b.alive==1||b.alive==3)&&b.roadAccess&&b.roadDistance>0){
                const auto required=context.buildingEmployment.find(b.type);if(required==context.buildingEmployment.end())return std::nullopt;
                if(b.storageLaborDisabled||required->second<=0||int64_t(b.workers)*100/required->second<=0)++noLabor;}}}
    if(cache==28)cache=1;f.delivery=0;f.watchCoverage=noLabor?2:1;return size_t(0);
}
inline OriginalFigureFirstUpdate originalCartFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4d4c70;out.status=OriginalFigureUpdateStatus::CartLeaderMovement;
    if(!context||!context->figures.count(saved.slot)||saved.leaderLink<=0)return out;
    auto local=*context;const auto immediate=local.figures.find(size_t(saved.leaderLink));if(immediate==local.figures.end())return out;
    const auto lead=immediate->second;auto goods=immediate;std::set<size_t> seen;
    const auto missingGoods=[&](){auto f=saved;f.alive=2;originalFigurePublishWrites(out,f,*context,local);return out;};
    for(unsigned n=0;n<8;++n){if(!seen.insert(goods->first).second)return out;
        if(goods->second.type==19||goods->second.type==21||goods->second.type==22)break;
        if(n==7||goods->second.leaderLink==0)return missingGoods();
        if(goods->second.leaderLink<0)return out;goods=local.figures.find(size_t(goods->second.leaderLink));if(goods==local.figures.end())return out;}
    if(goods->second.alive!=1)return missingGoods(); //4D4C9E..B4:missing root removes before movement/image selection.
    if(lead.alive!=1||lead.action==4||saved.action<6||saved.action>9)return out;
    auto f=saved;f.image=f.secondaryImage=0;f.hidden=f.freeMovement=0;f.stationary=bool(goods->second.stationary);
    bool follow=false;unsigned distance=saved.action==9?13:18;
    if(f.action==6||f.action==7){if(f.progress!=f.priorProgress){f.frame=uint8_t(f.frame+1);if(f.frame>=12)f.frame=0;}
        if(lead.type==19||lead.type==21||lead.type==22){if(!originalFigureSpecialGoods(lead))return out;
            if(goods->second.action==6||goods->second.direction>=8){f.frame=0;f.direction=lead.direction;}else follow=true;}
        else if(lead.type==20&&(lead.action==6||lead.action==7))follow=true;else return out;
    }else {if(lead.type!=20||(lead.action!=(f.action==8?6:7)))return out;
        if(goods->second.action==6||goods->second.direction>=8){f.frame=0;f.direction=lead.direction;}else follow=true;}
    if(follow){
        const bool atHome=f.x==f.homeX&&f.y==f.homeY;
        bool copyFrame=true;if(atHome)f.hidden=1;
        if(f.progress==20&&atHome&&(lead.progress<distance||lead.progress==20))copyFrame=false;
        else {int progress=int(lead.progress)-int(distance);if(progress<0)progress=uint8_t(progress+20);f.progress=uint8_t(progress);}
        if((f.progress==10||f.progress==11)&&(f.x!=lead.x||f.y!=lead.y)){
            if(local.terrain.size()!=51984||lead.cell>=51984||!local.cropBase||
                int64_t(*local.cropBase)+f.x+228LL*f.y!=f.cell||
                int64_t(*local.cropBase)+lead.x+228LL*lead.y!=lead.cell){out.status=OriginalFigureUpdateStatus::CellContext;return out;}
            terrainFlags=local.terrain[lead.cell];
            if(terrainFlags&(0x800000|0x10000400|0x1000000|4)){out.status=OriginalFigureUpdateStatus::MovementHeight;return out;}
            f.priorX=f.x;f.priorY=f.y;f.priorCell=uint32_t(int64_t(*local.cropBase)+f.x+228LL*f.y);f.x=lead.x;f.y=lead.y;
            if(!originalFigureRelink(f,lead.cell,local)){out.status=OriginalFigureUpdateStatus::CellContext;return out;}
            f.water=bool(terrainFlags&0x40);if(f.state85==100||f.state85==101)f.drawClass=0;f.state85=f.height=0;
        }
        if(f.progress<2){f.previousDirection=f.direction;
            f.direction=lead.direction<8?lead.direction:lead.previousDirection<8?lead.previousDirection:f.direction;}
        if(!f.state85){if(f.height)--f.height;else f.hidden=0;}
        else if(f.state85==100||f.state85==101){out.status=OriginalFigureUpdateStatus::MovementHeight;return out;}
        else {--f.state85;if(!f.state85)f.hidden=f.drawClass=0;
            else {const auto approach=[&](){if(f.height<f.cartStyle)++f.height;else if(f.height>f.cartStyle)--f.height;};approach();if(!(terrainFlags&0x800000))approach();}}
        if(copyFrame)f.frame=lead.frame;
    }
    const bool indirect=goods->first!=size_t(saved.leaderLink),bulk=originalFigureBulkGoods(goods->second);
    const unsigned style=(f.state85==0||f.state85==100||f.state85==101)?0:2-unsigned(f.cartStyle!=0);
    uint32_t code=0;if(indirect)code=(bulk?(goods->second.amount>400?0x4e8c:0x4e88):0x4c99)+style;
    else if(bulk&&f.action==6)code=0x4e38+(goods->second.amount>400?0x58:0)+style;
    else if(!bulk&&f.action==7)code=0x4c90+(style?3-style:0);else return out;
    auto draw=f;if(draw.direction>=8&&draw.previousDirection>=8)draw.previousDirection=0;
    if(!originalFigureWriteWalkingImage(draw,code,archives,context->view)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}f.image=draw.image;
    if(indirect){const auto images=originalFigureUpdateImageState(local);const auto cargo=figureSecondaryImage(originalFigureImageInputs(f),archives,context->view,&images);
        if(cargo.status!=OriginalFigureImageStatus::Selected){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}f.secondaryImage=cargo.image;}
    f.extraX=f.extraY=0;goods->second.secondaryImage=0;
    originalFigurePublishWrites(out,f,*context,local);return out;
}
inline OriginalFigureFirstUpdate originalGoodsFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=saved.type==19?0x4cbec0:saved.type==21?0x4cd8f0:0x4d13c0;out.status=OriginalFigureUpdateStatus::GoodsRouteAndOwner;
    if(!context||!context->figures.count(saved.slot))return out;auto local=*context;auto f=saved;f.image=f.secondaryImage=0;
    f.movementMode=1;f.frame=uint8_t(f.frame+1);if(f.frame>=12)f.frame=0;
    if(f.type==21){if(!context->buildings)return out;const auto owner=context->buildings->find(size_t(f.owner));
        if(owner==context->buildings->end()||owner->second.type!=0x3a||owner->second.alive!=1||
            std::find(owner->second.cargoFigures.begin(),owner->second.cargoFigures.end(),int(f.slot))==owner->second.cargoFigures.end())return out;}
    //0x4CB6B0 precedes movement in these branches. Preserve its old cart style;
    //0x4CB910 adds the resulting direction only after movement.
    OriginalFigureImageSelection cargo;
    if(f.type!=21){cargo=figureSecondaryImage(originalFigureImageInputs(f),archives,context->view);
        if(cargo.status!=OriginalFigureImageStatus::Selected){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}}
    if(f.type==19&&f.action==6){if(!context->buildings)return out;const auto owner=context->buildings->find(size_t(f.owner));
        if(owner==context->buildings->end()||owner->second.alive!=1||!(terrainFlags&0x40)||
            (terrainFlags&(0x100|0x800|0x80000|0x10004)))return out;
        f.frame=0;f.counter=originalFigureWord(int(f.counter)+1);if(f.counter>30){out.status=OriginalFigureUpdateStatus::GoodsSearch;
            //4CBFFC may release a prior route before search;this branch is
            //bounded to route0 until its output-resource predicate is supplied.
            if(f.route)return out;
            const auto selected=originalFigureGoodsSupplier(f,local);if(!selected)return out;
            if(*selected){f.action=local.buildings->at(*selected).type==0x35?8:7;f.stationary=0;}
            else f.stationary=1;f.counter=0;}
    }else if(f.type==19&&(f.action==10||f.action==11)){
        f.counter=originalFigureWord(int(f.counter)+1);f.frame=0;
        if(f.counter>=(f.action==10?11:6)){out.status=OriginalFigureUpdateStatus::GoodsArrival;
            if(f.action!=10||!local.buildings||!local.goods)return out;
            auto destination=local.buildings->find(size_t(f.delivery));if(destination==local.buildings->end())return out;
            const auto delivered=originalFigureWarehouseDeliver(destination->second,f.subtype,unsigned(uint16_t(f.amount)),*local.goods);if(!delivered)return out;
            //4E2A20:Warehouse+C8(-8)=false,+D8=false,so no incoming
            //counter write. A full delivery returns home without search.
            if(*delivered!=unsigned(uint16_t(f.amount)))return out;
            f.action=13;f.counter=0;f.destinationX=f.homeX;f.destinationY=f.homeY;f.amount=0;
        }
    }else if(f.type==19&&f.action==12){ //4CC493..515:zero cargo removes before image write.
        if(f.amount)return out;f.alive=2;
        originalFigurePublishWrites(out,f,*context,local);return out;
    }else if(f.type==21&&f.action==12){
        out.status=OriginalFigureUpdateStatus::GoodsArrival;
        if(!local.buildings)return out;auto destination=local.buildings->find(size_t(f.delivery));
        if(destination==local.buildings->end())return out;
        if(destination->second.type!=0x35&&destination->second.type!=0x36&&destination->second.type!=0x3a)return out;
        f.counter=originalFigureWord(int(f.counter)+1);f.frame=0;
        if(f.counter>4){if(!local.goods)return out;
            const auto remaining=originalFigureStorageWithdraw(destination->second,f.resource,400,true,*local.goods);if(!remaining)return out;
            f.amount=originalFigureWord(int(400-*remaining));
            if(!f.amount){f.subtype=0;f.action=6;}
            else {f.subtype=f.resource;f.action=10;f.counter=0;f.destinationX=f.homeX;f.destinationY=f.homeY;}
            if(f.route>0){if(f.route>=1000||!local.services)return out;
                if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}
    }else if((f.type==19&&(f.action==7||f.action==8||f.action==13))||(f.type==21&&(f.action==11||f.action==10))||(f.type==22&&(f.action==7||f.action==9))){
        if(f.type==21)f.movementMode=3;
        if(f.type==19&&f.action!=13){if(!context->buildings)return out;
            const auto dest=context->buildings->find(size_t(f.delivery));
            if(dest==context->buildings->end()||(dest->second.type!=0x35&&dest->second.type!=0x36)||!dest->second.destinationEnabled)return out;
            f.hidden=0;}
        const auto speed=context->modelSpeeds.find(f.type);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
        const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.type==21&&f.action==11&&f.direction==8)f.action=12; //4CDDF4: withdrawal is a later action12 call.
        else if(f.type==19&&(f.action==7||f.action==8)&&f.direction==8){out.status=OriginalFigureUpdateStatus::GoodsArrival;
            const auto destination=local.buildings->find(size_t(f.delivery));if(destination==local.buildings->end()||destination->second.alive!=1)return out;
            //4CC03A->418D60: Mill35 enters11;5D36E0:Warehouse36 enters10.
            // No withdrawal or accounting happens on this arrival call.
            if(destination->second.type==0x35)f.action=11;
            else if(destination->second.type==0x36)f.action=10;else return out;}
        else if(f.type==19&&f.action==13&&f.direction==8)f.action=12; //4CC5D6.
        else if(f.type==19&&f.action==13&&f.direction==9){if(!local.services||f.route<0||f.route>=1000)return out;
            if(f.route>0){if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}
        else if(f.type==22&&f.action==9&&f.direction==8){ //4D1740:deliver remaining cargo to owner,then remove on full acceptance.
            if(!local.buildings||!local.goods)return out;auto owner=local.buildings->find(size_t(f.owner));
            if(owner==local.buildings->end())return out;
            if(f.amount){const auto delivered=originalFigureWarehouseDeliver(owner->second,f.subtype,unsigned(uint16_t(f.amount)),*local.goods);
                if(!delivered||*delivered!=unsigned(uint16_t(f.amount)))return out;f.amount=0;}f.alive=2;}
        else if(f.direction>=8){out.status=OriginalFigureUpdateStatus::GoodsArrival;return out;}
    }else return out;
    if(f.type==21){cargo=figureSecondaryImage(originalFigureImageInputs(f),archives,context->view);
        if(cargo.status!=OriginalFigureImageStatus::Selected){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}}
    const bool special=originalFigureSpecialGoods(f);
    if(f.direction>=8||(special&&f.stationary))f.frame=0;
    const uint32_t code=special?0x4ca6:(f.state85==0||f.state85==100||f.state85==101)?0x4e60:f.cartStyle?0x4e61:0x4e62;
    auto draw=f;if(special&&f.stationary)draw.direction=draw.previousDirection;
    if(!originalFigureWriteWalkingImage(draw,code,archives,context->view)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}f.image=draw.image;
    if(cargo.image){int direction=f.direction;if(direction>=8||(special&&f.stationary))direction=f.previousDirection;
        direction-=int(context->view);if(direction<0)direction+=8;
        f.secondaryImage=cargo.image-cargo.direction+unsigned(direction);f.extraX=f.extraY=0;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
inline OriginalFigureFirstUpdate originalWorkerFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4c9fd0;out.status=OriginalFigureUpdateStatus::WorkerOwnerMovement;
    if(!context||!context->buildings||(saved.action!=7&&saved.action!=8)||!context->figures.count(saved.slot))return out;
    const auto home=context->buildings->find(size_t(saved.home));
    if(home==context->buildings->end()||(home->second.alive!=1&&home->second.alive!=3)||home->second.worker!=int(saved.slot)||
        home->second.type<2||home->second.type>=18||!home->second.accepting)return out;
    const auto speed=context->modelSpeeds.find(11);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=3;f.hidden=0;f.frame=uint8_t(f.frame+1);if(f.frame>=12)f.frame=0;
    if(f.action==8){f.freeMovement=1;const auto movement=originalFigureFreeStep(f,local);
        if(!movement)return out;f=movement->first;
        if(movement->second){out.status=OriginalFigureUpdateStatus::WorkerEmployment;
            auto& house=local.buildings->at(size_t(f.home));
            const auto capacity=context->houseCapacities.find(house.type==11?11:house.houseModel);
            if(capacity==context->houseCapacities.end()||!house.laborDisabled)return out;
            // Empty-house arrival invokes notification virtual+230; its broader
            // event effects remain refused. An occupied house bypasses it.
            if(!house.residents)return out;
            f.alive=2;
            if(!*house.laborDisabled){if(!local.services||!local.services->population)return out;
                house.residents=originalFigureWord(int(house.residents)+f.employmentAmount);
                house.freeSlots=originalFigureWord(capacity->second-int(house.residents));
                auto& pop=*local.services->population;pop.lastArrival=f.employmentAmount;
                const uint32_t bits=uint32_t(pop.total)+f.employmentAmount;
                pop.total=bits>INT32_MAX?0:int32_t(bits);pop.peak=std::max(pop.peak,pop.total);}
            house.worker=0;
        }f.hidden=bool(f.cellMovement);
    }else {const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.direction==8){f.action=8;f.destinationX=home->second.x;f.destinationY=home->second.y;f.roamSteps=0;
            f.freeTargetX=originalFigureWord(int(f.destinationX)*20);f.freeTargetY=originalFigureWord(int(f.destinationY)*20);
            f.freeRemainingX=originalFigureWord(std::abs(int(f.freeTargetX)-f.subX));f.freeRemainingY=originalFigureWord(std::abs(int(f.freeTargetY)-f.subY));
            f.freeError=originalFigureWord(2*std::min(int(f.freeRemainingX),int(f.freeRemainingY))-std::max(int(f.freeRemainingX),int(f.freeRemainingY)));
            f.direction=originalFigureDirectionTo(f.subX,f.subY,f.freeTargetX,f.freeTargetY);
            if(2*int(f.freeRemainingX)<f.freeRemainingY){if(f.direction==1||f.direction==7)f.direction=0;else if(f.direction==3||f.direction==5)f.direction=4;}
            if(2*int(f.freeRemainingY)<f.freeRemainingX){if(f.direction==1||f.direction==3)f.direction=2;else if(f.direction==5||f.direction==7)f.direction=6;}
            f.freeAxis=f.freeRemainingY<=f.freeRemainingX?1:2;
        }else if(f.direction>=8)return out;}
    if(!originalFigureWriteWalkingImage(f,f.subtype?0x4c13:0x4c15,archives,context->view,false,false)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
//5EA510->4EA050->4E1420->4E23A0 pops the oldest queued inactive slot,
//not the numerically smallest slot.4C9160 advances saved game RNG once;
//4EA120 advances the separate CRT RNG once even when sound is disabled.
inline OriginalFigureUpdateStatus originalFigureHunterProjectile(const OriginalFigure& hunter,int x,int y,OriginalFigureUpdateContext& context){
    if(!context.services||!context.services->allocationSerial||!context.cropBase||context.cellHeads.size()!=51984)return OriginalFigureUpdateStatus::Projectile;
    auto& queue=context.freeSlots;size_t slot=0;
    for(unsigned attempt=0;attempt<5&&!queue.empty();++attempt){const size_t candidate=queue.front();queue.erase(queue.begin());
        const auto found=context.figures.find(candidate);if(candidate&&found!=context.figures.end()&&!found->second.alive){slot=candidate;break;}}
    if(!slot)return OriginalFigureUpdateStatus::Computed; //allocation failure:caller still changes Hunter/prey;no CRT call.
    if(!context.crtRandom)return OriginalFigureUpdateStatus::CrtRandomRequired;
    auto p=OriginalFigure{};p.slot=slot;p.runtimeClass=OriginalFigureClass::Figure;p.type=54;p.alive=1;p.progress=20;
    p.x=p.priorX=p.homeX=hunter.x;p.y=p.priorY=p.homeY=hunter.y;p.destinationX=int16_t(x);p.destinationY=int16_t(y);
    p.cell=*context.cropBase+p.x+228*p.y;if(p.cell>=51984)return OriginalFigureUpdateStatus::Projectile;p.subX=originalFigureWord(20*p.x);p.subY=originalFigureWord(20*p.y);
    p.allocationSerial=uint16_t((*context.services->allocationSerial)++);
    p.allocationPhase=p.allocationPhaseCopy=uint8_t(originalFigureRandomNext(*context.services)&3);
    if(!originalFigureCrtRandomNext(context))return OriginalFigureUpdateStatus::CrtRandomRequired;
    p.raise=10;p.owner=int16_t(hunter.slot);p.freeMovement=1;p.height=uint8_t(hunter.height+18);p.projectile=true;
    p.projectileDistance=int16_t(std::max(std::abs(x-int(hunter.x)),std::abs(y-int(hunter.y))));
    p.projectileStartHeight=-int16_t(hunter.height);p.projectileEndHeight=hunter.variant; //virtual+14=4C9970,-height.
    originalFigureFreeTarget(p);p.direction=originalFigureProjectileDirection(hunter.x,hunter.y,x,y);
    p.next=0;size_t tail=context.cellHeads[p.cell];std::set<size_t> seen;
    if(!tail)context.cellHeads[p.cell]=uint16_t(slot);
    else {for(;;){const auto found=context.figures.find(tail);
            if(found==context.figures.end()||!seen.insert(tail).second||found->second.cell!=p.cell)return OriginalFigureUpdateStatus::Projectile;
            if(!found->second.next)break;tail=found->second.next;}
        context.figures.at(tail).next=uint16_t(slot);}
    context.figures.at(slot)=p;context.allocatedSlots.insert(slot);context.highestLiveSlot=std::max(context.highestLiveSlot,slot);return OriginalFigureUpdateStatus::Computed;
}
//59CC00(1000,0,0): load-reconstructed animal vector is ascending; collect
//at most30 qualifying prey,5AE970 floods B1D in N,E,S,W order and checks
//candidate indices in reverse at each dequeued cell. Distance includes start1.
inline std::optional<size_t> originalFigureHunterPrey(const OriginalFigure& hunter,const OriginalFigureUpdateContext& context){
    if(!context.passability||!context.passability->fullMovementMasks||context.passability->cells.size()!=51984||!context.cropBase)return std::nullopt;
    std::vector<const OriginalFigure*> candidates;
    for(const auto& e:context.figures){const auto& p=e.second;
        if(p.type<75||p.type>77||p.alive!=1||p.hidden||p.action==4||p.action==10||
            (p.linkedBuilding&&p.linkedBuilding!=unsigned(hunter.owner)))continue;
        if(p.type!=76)return std::nullopt; //Pheasant virtual+3C=416A50;other prey predicates withheld.
        candidates.push_back(&p);if(candidates.size()==30)break;}
    if(candidates.empty())return size_t(0);
    if(hunter.cell>=51984)return std::nullopt;
    std::vector<unsigned> distance(51984);std::vector<size_t> queue{hunter.cell};distance[hunter.cell]=1;
    constexpr int offsets[]={-228,1,228,-1};
    for(size_t at=0;at<queue.size();++at){const auto cell=queue[at];
        for(auto p=candidates.rbegin();p!=candidates.rend();++p)if((*p)->cell==cell)return distance[cell]<=1000?(*p)->slot:size_t(0);
        for(int delta:offsets){const int64_t next=int64_t(cell)+delta;if(next<0||next>=51984)continue;
            if(!distance[size_t(next)]&&(context.passability->cells[size_t(next)]&0xb1d)){
                distance[size_t(next)]=distance[cell]+1;queue.push_back(size_t(next));}}}
    return size_t(0);
}
inline OriginalFigureFirstUpdate originalHunterFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4d2f10;out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;
    if(!context||!context->buildings||!context->figures.count(saved.slot))return out;
    const auto owner=context->buildings->find(size_t(saved.owner));
    if(owner==context->buildings->end()||(owner->second.alive!=1&&owner->second.alive!=3))return out;
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=3;f.freeMovement=f.hidden=0;
    uint32_t code=0;
    if(f.action==6){f.counter=originalFigureWord(int(f.counter)-1);f.direction=f.previousDirection;
        if(f.counter<=0){const auto prey=originalFigureHunterPrey(f,local);if(!prey)return out;
            if(*prey){auto& p=local.figures.at(*prey);f.destinationX=p.x;f.destinationY=p.y;f.target=int16_t(*prey);p.flags120|=2;p.linkedBuilding=unsigned(f.owner);f.action=7;}
            else {f.action=11;f.destinationX=owner->second.roadX;f.destinationY=owner->second.roadY;
                if(f.x==f.destinationX&&f.y==f.destinationY){f.hidden=1;f.alive=2;}}
            if(f.route>0){if(f.route>=1000||!local.services)return out;if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}
        if(!originalFigureWriteWalkingImage(f,f.action==7?0x4ebf:0x4ebe,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    }else if(f.action==13){const auto model=context->models.find(40);const auto group=originalFigureImageGroup(0x4ec0,archives);
        if(model==context->models.end()||model->second[7]==INT32_MIN||!group)return out;
        f.counter=originalFigureWord(int(f.counter)-1);
        if(f.counter<model->second[7]-group->second->animationSpriteCount-8)f.action=7;
        if(f.action==7){const auto target=context->figures.find(size_t(f.target));
            if(target==context->figures.end()||target->second.alive!=1||target->second.action==4||target->second.action==10||
                target->second.type<75||target->second.type>77||
                std::max(std::abs(int(f.x)-target->second.x),std::abs(int(f.y)-target->second.y))<=2)return out;}
        if(!originalFigureWriteWalkingImage(f,f.action==7?0x4ebf:0x4ec0,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    }else if(f.action==8){f.counter=originalFigureWord(int(f.counter)+1);
        if(f.counter>12){if(!local.services||f.route<0||f.route>=1000)return out;
            f.action=10;f.destinationX=owner->second.roadX;f.destinationY=owner->second.roadY;
            if(f.route>0){if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}
            if(!originalFigureWriteWalkingImage(f,0x4ec2,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}}
        else {const auto group=originalFigureImageGroup(0x4ec1,archives);if(!group||f.counter<0){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
            f.image=group->first+unsigned(f.counter);}
    }else if(f.action==12){f.counter=originalFigureWord(int(f.counter)-1);f.hidden=1;
        if(f.counter<1){f.action=6;f.frame=0;code=0x4ebe;}else code=0x4ebe;
        if(f.action==12){auto draw=f;draw.frame=0;if(!originalFigureWriteWalkingImage(draw,code,archives,context->view)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}f.image=draw.image;}
        else if(!originalFigureWriteWalkingImage(f,code,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    }else if(f.action==7||f.action==10){
        if(f.action==7){const auto target=context->figures.find(size_t(f.target));const auto model=context->models.find(40);
            if(target==context->figures.end()||model==context->models.end()||model->second[6]==INT32_MIN)return out;
            const auto& t=target->second;
            if(t.alive!=1||t.action==4||t.action==10||t.type<75||t.type>77)return out;
            const int distance=std::max(std::abs(int(f.x)-t.x),std::abs(int(f.y)-t.y));
            if(!t.hidden&&distance<=model->second[6]){f.counter=originalFigureWord(int(f.counter)-1);if(f.counter<=0){
                    out.status=OriginalFigureUpdateStatus::Projectile;
                    if(model->second[7]==INT32_MIN)return out;
                    const auto allocation=originalFigureHunterProjectile(f,t.x,t.y,local);
                    if(allocation!=OriginalFigureUpdateStatus::Computed){out.status=allocation;return out;}
                    f.direction=originalFigureAttackDirection(f.x,f.y,t.x,t.y);f.action=13;f.counter=originalFigureWord(model->second[7]);f.frame=0;
                    auto& prey=local.figures.at(size_t(f.target));prey.previousAction=prey.action;prey.action=18;
                    prey.direction=originalFigureAttackDirection(prey.x,prey.y,f.x,f.y);prey.counter=100;
                    if(f.route>0){if(!local.services||f.route>=1000)return out;
                        if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;} //4D31DC->4D3519.
                    f.next=local.figures.at(f.slot).next;
                    if(!originalFigureWriteWalkingImage(f,0x4ec0,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
                    originalFigurePublishWrites(out,f,*context,local);return out;}}
            if(distance<=2)return out;code=0x4ebf;
        }else code=0x4ec2;
        const auto speed=context->modelSpeeds.find(40);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
        const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if((f.direction==9||f.direction==8)&&f.action==7){ //4D3249/4D3299:retarget a still-qualified prey after route end.
            const auto target=local.figures.find(size_t(f.target));if(target==local.figures.end()||!local.services)return out;
            f.destinationX=target->second.x;f.destinationY=target->second.y;
            if(f.route>0){if(f.route>=1000)return out;if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}
        else if(f.action==10&&f.direction==8){ //4D3408:common+72 receives50 before dispatcher removal.
            auto& b=local.buildings->at(size_t(f.owner));if(b.type!=0x21)return out;
            b.hunterReturnGoods=originalFigureWord(int(b.hunterReturnGoods)+50);f.alive=2;}
        else if(f.direction==10)f.alive=2;
        else if(f.direction>=8)return out;
        if(!originalFigureWriteWalkingImage(f,code,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    }else return out;
    originalFigurePublishWrites(out,f,*context,local);return out;
}
//5E6200 type54 ordinary shooter branch. Empty-cell arrival retains the
//projectile for its final ten lifetime calls and clears the shadow. Positive
//combat/building impacts remain refused.
inline OriginalFigureFirstUpdate originalProjectileFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context){
    OriginalFigureFirstUpdate out;out.updaterVA=0x5e6200;out.status=OriginalFigureUpdateStatus::Projectile;
    if(!context||!context->animationFlags||!context->models.count(40)||!context->figures.count(saved.slot)||saved.type!=54||saved.owner<=0||
        !context->services||context->buildingOwners.size()!=51984)return out;
    auto local=*context;auto f=saved;const auto shooter=local.figures.find(size_t(f.owner));
    if(shooter==local.figures.end()||shooter->second.type!=40||shooter->second.alive==2||f.home<0)return out;
    const int range=local.models.at(40)[6];if(range==INT32_MIN||range<1)return out;
    f.home=originalFigureWord(int(f.home)+1);if(f.home>10*(3*range/4))return out;
    f.freeMovement=1;bool arrived=false;
    for(unsigned n=0;n<7;++n){const auto moved=originalFigureFreeStep(f,local);if(!moved)return out;f=moved->first;
        if(moved->second){arrived=true;break;}}
    if(f.cell>=51984||local.buildingOwners[f.cell])return out;
    if(arrived){ //5E34D0 then5E3400: prove both queries have no candidate.
        std::set<size_t> seen;
        for(size_t slot=local.cellHeads[f.cell];slot;){const auto it=local.figures.find(slot);
            if(it==local.figures.end()||!seen.insert(slot).second)return out;
            const auto& other=it->second;
            if(other.action!=4&&other.type!=0&&other.type!=1&&other.type!=2&&other.type!=4&&other.type!=5&&other.type!=9&&other.type!=52&&other.type!=54)return out;
            slot=other.next;}
        f.home=std::max<int16_t>(f.home,int16_t(10*(3*range/4)-10));}
    //5E670F default shooter40:32-direction code1C6D;secondary1C6C
    //uses a copy of the already advanced primary counter.
    const auto group=originalFigureImageGroup(0x1c6d,archives),secondary=originalFigureImageGroup(0x1c6c,archives);
    if(!group||!secondary)return out;
    const auto phase=originalFigureAdvanceFrame(f.frame,group->second->animationSpriteCount,group->second->animationSpeedId,100,*context->animationFlags);
    if(!phase)return out;f.frame=phase->counter;
    const auto secondaryPhase=originalFigureAdvanceFrame(f.frame,secondary->second->animationSpriteCount,secondary->second->animationSpeedId,100,*context->animationFlags);
    if(!secondaryPhase)return out;if(!arrived)f.attackFrame=secondaryPhase->counter;
    auto images=originalFigureUpdateImageState(local);auto input=originalFigureImageInputs(f);
    const auto primary=figureImage(input,archives,0,context->view,std::nullopt,&images);
    const auto cargo=figureSecondaryImage(input,archives,context->view,&images);
    if(primary.status!=OriginalFigureImageStatus::Selected||cargo.status!=OriginalFigureImageStatus::Selected)return out;
    f.image=primary.image;f.secondaryImage=arrived?0:cargo.image;originalFigurePublishWrites(out,f,*context,local);return out;
}
//4C0760 reads four half-open month intervals from7AE22C/7AE19C.
//4C1060 maps C3..C7 to rows0..4;4C18C0 maps them to goods7/5/6/3/1.
inline std::optional<bool> originalFigureCropInSeason(unsigned type,int32_t month){
    if(type==0xc2)return month>=3&&month<8; //row5,7AE27C/7AE1EC.
    if(type<0xc3||type>0xc7)return std::nullopt;
    constexpr int starts[5]={2,6,5,7,4},ends[5]={6,10,9,11,8};
    return month>=starts[type-0xc3]&&month<ends[type-0xc3];
}
//42D790 reads ascending pool slots, invokes+C0=4C1770 on live records;
//4C5A60->4C5AD0->5F01F0 appends at vector end. No building update runs
//between this load reconstruction and the explicitly supplied figure passes.
inline std::optional<size_t> originalFigureSupplierCrop(const OriginalFigure& f,const OriginalFigureUpdateContext& context){
    if(!context.buildings)return std::nullopt;
    const auto owner=context.buildings->find(size_t(f.owner));if(owner==context.buildings->end())return std::nullopt;
    const auto compatible=[&](unsigned type){return owner->second.type==0xc0?type==0xc2:type>=0xc3&&type<=0xc7;};
    bool reciprocal=false;for(const auto& e:*context.buildings)if(e.second.alive==1&&compatible(e.second.type)&&
        e.second.primaryFigure==int(f.slot))reciprocal=true;
    if(!reciprocal)return size_t(0);
    if(!context.crops||context.terrain.size()!=51984)return std::nullopt;
    constexpr unsigned goods[5]={7,5,6,3,1};
    for(const auto& e:*context.buildings){const auto& b=e.second;
        if(b.alive!=1||!compatible(b.type))continue;
        if(b.cell>=51984)return std::nullopt;
        if((context.terrain[b.cell]&0x100)||context.crops->unavailable[b.type==0xc2?19:goods[b.type-0xc3]]||
            !b.destinationEnabled||context.crops->disabled[b.type]||b.primaryFigure!=int(f.slot))continue;
        const auto season=originalFigureCropInSeason(b.type,context.crops->month);
        if(!season)return std::nullopt;
        //4C2C70 tests the FIRST qualifying candidate's calendar, not a
        //calendar-filtered vector search. An out-of-season first returns home.
        return *season?e.first:size_t(0);
    }
    return size_t(0);
}
//0x4CC670 owner-live1 movement, countdown and cited crop delivery writes.
inline OriginalFigureFirstUpdate originalSupplierFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4cc670;out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;
    if(!context||!context->buildings||!context->figures.count(saved.slot))return out;
    const auto owner=context->buildings->find(size_t(saved.owner));
    if(owner==context->buildings->end()||owner->second.alive!=1)return out;
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=17;f.freeMovement=f.hidden=0;
    if(f.action==7||f.action==9){const auto speed=context->modelSpeeds.find(26);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
        const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.action==9&&f.direction>=8){
            if(f.direction==8){f.hidden=1;f.alive=2;}
            else if(f.direction==9){if(!local.services||f.route<0||f.route>=1000)return out;
                if(f.route>0){if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}
            else if(f.direction==10)f.alive=2;else return out;
        }
        if(f.action==7&&f.direction==8){if(owner->second.type!=0xc0&&owner->second.type!=0xc1){out.status=OriginalFigureUpdateStatus::SupplierArrival;return out;}
            //4CC8AA: hold+ C8=4C2A90(-4) is false for typeC1, so
            //counter=0. The float-to-int helper7650FC is not reached.
            f.action=8;f.counter=0;
        }else if(f.action==7&&f.direction>=8)return out;
    }else if(f.action==6){f.hidden=1;f.counter=originalFigureWord(int(f.counter)-1);
        if(f.counter<1){f.counter=0;
            if((owner->second.type!=0xc0&&owner->second.type!=0xc1)||!owner->second.cropSearchComplete||!owner->second.suppliers){out.status=OriginalFigureUpdateStatus::SupplierSearch;return out;}
            const auto crop=originalFigureSupplierCrop(f,local);if(!crop){out.status=OriginalFigureUpdateStatus::SupplierSearch;return out;}
            if(f.route>0){if(f.route>=1000||!local.services)return out;
                if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}
            if(*crop){const auto& selected=local.buildings->at(*crop);f.destinationX=selected.x;f.destinationY=selected.y;
                f.action=7;f.delivery=int16_t(*crop);}
            else {f.destinationX=owner->second.roadX;f.destinationY=owner->second.roadY;f.action=9;
                auto& count=*local.buildings->at(size_t(f.owner)).suppliers;
                const uint32_t bits=uint32_t(count)-1;count=bits>INT32_MAX?0:int32_t(bits);}
        }
    }else if(f.action==8){f.counter=originalFigureWord(int(f.counter)-1);
        if(f.counter<1){auto delivery=local.buildings->find(size_t(f.delivery));
            if(delivery==local.buildings->end()||(delivery->second.type!=0xc2&&delivery->second.type!=0xc4)||!delivery->second.cropDelivery){
                out.status=OriginalFigureUpdateStatus::SupplierArrival;return out;}
            delivery->second.cropDelivery->flags|=0x40;delivery->second.primaryFigure=0;delivery->second.cropDelivery->pending=0;f.action=6;}}
    else return out;
    unsigned base=0x4e65+4*std::min(unsigned(f.variant),3u);
    if(owner->second.type==0xee)base=0x4e7d;else if(owner->second.type==0xef)base=0x4e75;else if(owner->second.type==0xed)base=0x4e79;
    if(!originalFigureWriteWalkingImage(f,base+unsigned(f.action==8),archives,context->view,true,true,true)){
        out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
//0x4D5D00 -> owner virtual+24. Cited owner countdown,roam and return
// branches carry their service callbacks;positive MusicSchool venue search
// and other uncited virtual branches remain refused.
inline OriginalFigureFirstUpdate originalDelegatedFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4d5d00;out.status=OriginalFigureUpdateStatus::BuildingVirtualUpdate;
    if(!context||!context->buildings||!context->figures.count(saved.slot))return out;
    const auto owner=context->buildings->find(size_t(saved.owner));if(owner==context->buildings->end()||owner->second.alive!=1)return out;
    const auto type=owner->second.type;uint32_t code=0;
    if(type==0xd3&&saved.type==34)code=0x4c67;
    else if(type==0xcf)code=0x4c1e;else if(type==0xd6)code=0x4c11;else if(type==0x47)code=0x4ca0;else return out;
    const auto model=context->models.find(saved.type);
    if(model==context->models.end()||model->second[15]==INT32_MIN)return out;
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.freeMovement=0;f.movementMode=type==0xd3?18:7;
    f.roamLimit=originalFigureWord(int((int64_t(model->second[15])*96)%65536));
    f.frame=uint8_t(f.frame+1);if(type!=0xd3)f.hidden=0;
    if(f.action==6){f.counter=originalFigureWord(int(f.counter)-1);f.hidden=1;f.frame=0;
        if(type==0xd3)f.searchCounter=0;if(f.counter<1){
            const auto road=originalFigureMusicRoad(owner->second,local);if(!road||!*road)return out;
            f.action=7;f.destinationX=owner->second.roadX;f.destinationY=owner->second.roadY;f.roamSteps=0;originalFigureFreeTarget(f);}
    }else if(f.action==7){f.freeMovement=f.hidden=1;if(type==0xd3)f.movementMode=1;
        const auto moved=originalFigureFreeStep(f,local);if(!moved)return out;f=moved->first;f.hidden=1;
        if(moved->second){if(!local.buildings)return out;
            if(type!=0xd3){
                if(type==0xcf||type==0xd6||type==0x47){if(local.buildingOwners.size()!=51984||f.cell>=51984)return out;
                    if(local.buildingOwners[f.cell]==f.owner)f.alive=2;
                    else {f.action=8;if(!originalFigureBeginRoam(f,local))return out;f.roamSteps=0;}}
                else {f.action=8;if(!originalFigureBeginRoam(f,local))return out;f.roamSteps=0;}}
            else {
            //48A520: entertainment47+264 accepts34;road+78 precedes
            //employment and service score. A positive venue remains refused.
            for(const auto& e:*local.buildings){const auto& b=e.second;
                if(b.type==0x47&&(b.alive==1||b.alive==3)){
                    if(!b.entertainmentRoad)return out;if(!*b.entertainmentRoad)continue;
                    //47+1B4=4F1540 returns100;48A950(type34) reads+5E.
                    //48A520 tries thresholds8,16;larger scores never qualify.
                    if(!b.entertainment||b.entertainment->at(1)<=16)return out;}}
            f.alive=2;}}
    }else if(type!=0xd3&&f.action==8){if(f.roamSteps>=f.roamLimit){ //51D13C..19D still roams after choosing the return road.
            if(type!=0xcf&&type!=0xd6&&type!=0x47)return out;
            const auto road=originalFigureMusicRoad(owner->second,local);if(!road)return out;
            if(!*road)f.alive=2;
            else {if(!local.services||f.route<0||f.route>=1000)return out;
                f.action=9;f.destinationX=owner->second.roadX;f.destinationY=owner->second.roadY;
                if(f.route>0){if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}}
        const auto movement=originalFigureRoam(f,6,terrainFlags,local);if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
    }else if(type!=0xd3&&f.action==9){const auto movement=originalFigureFollowRoute(f,6,terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.direction==8&&(type==0xcf||type==0xd6||type==0x47)){ //+23C=66EFA0 false: return from road to occupied cell.
            f.action=7;f.destinationX=owner->second.x;f.destinationY=owner->second.y;originalFigureFreeTarget(f);f.roamSteps=0;}
        else if(f.direction>=8)return out;
    }else return out;
    if(f.frame>11)f.frame=0;
    if(!originalFigureWriteWalkingImage(f,code,archives,context->view)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
//0x4D0270/0x4D3A70/0x4D3B50/0x4CD230 bounded owner-valid walk/roam.
inline OriginalFigureFirstUpdate originalServiceFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;
    out.updaterVA=saved.type==23?0x4d0270:saved.type==28?0x4d3a70:saved.type==29?0x4d3b50:0x4cd230;
    if(!context||!context->buildings||!context->figures.count(saved.slot))return out;
    const auto owner=context->buildings->find(size_t(saved.owner));const auto model=context->models.find(saved.type);
    if(owner==context->buildings->end()||model==context->models.end()||owner->second.alive!=1||
        (owner->second.primaryFigure!=int(saved.slot)&&
         !((saved.type==23||(saved.type==39&&owner->second.inspectorSecondary))&&
           std::find(owner->second.secondaryFigures.begin(),owner->second.secondaryFigures.end(),int(saved.slot))!=owner->second.secondaryFigures.end()))||
        model->second[15]==INT32_MIN)return out;
    // These vtables all use0x4295C0/+48; a matching primary+2E proves
    // membership before the optional secondary slots for category2/3.
    const unsigned ownerType=owner->second.type;
    if((saved.type==23&&ownerType!=0x3b)||(saved.type==28&&ownerType!=0x48)||
        (saved.type==29&&ownerType!=0x7f)||(saved.type==39&&ownerType!=0x7c))return out;
    const auto speed=context->modelSpeeds.find(saved.type);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=18;f.freeMovement=0;
    f.roamLimit=originalFigureWord(int((int64_t(model->second[15])*96)%65536));
    if(f.type==39){if(f.action==8){
            if(!context->services||!context->services->fireCount){out.status=OriginalFigureUpdateStatus::InspectorFireContext;return out;}
            //0x4E2E90 reads the SAV19-restored signed count first. Positive
            // counts increment+75 modulo256; search starts at the new20.
            if(*context->services->fireCount>0){f.searchCounter=uint8_t(f.searchCounter+1);
                if(f.searchCounter>=20){out.status=OriginalFigureUpdateStatus::InspectorFireSearch;return out;}}
            if(f.roamSteps>=f.roamLimit)return out;f.hidden=0;
            const auto moved=originalFigureRoam(f,unsigned(speed->second),terrainFlags,local);
            if(!moved.state){out.status=moved.status;return out;}f=*moved.state;
        }else if(f.action==7){ //0x4E2E90 has no side effect for action7 at any count.
            f.freeMovement=f.hidden=1;const auto moved=originalFigureFreeStep(f,local);if(!moved)return out;f=moved->first;
            if(moved->second){ //4CD3E0..408: reaching the owner's occupied cell removes;otherwise begin ordinary roam.
                if(local.buildingOwners.size()!=51984||f.cell>=51984)return out;
                if(local.buildingOwners[f.cell]==f.owner)f.alive=2;
                else {f.action=8;if(!originalFigureBeginRoam(f,local))return out;}}
        }else return out;
        if(!originalFigureWriteWalkingImage(f,0x4c09,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
        originalFigurePublishWrites(out,f,*context,local);return out;
    }
    f.frame=uint8_t(f.frame+1);if(f.frame>11){f.frame=0;
        if(f.type==29&&(!local.services||!local.services->criminalCount||*local.services->criminalCount>0))return out;}
    if(f.type==23||f.type==28){if(f.action!=1&&f.action!=2)return out;
        if(f.type==23&&f.action==1&&(!owner->second.marketGoods||*owner->second.marketGoods<1))return out;
        if(f.action==1){if(f.roamSteps>=f.roamLimit||(f.roamSteps>=int(f.roamLimit)*4/5&&f.x==f.roamOriginX&&f.y==f.roamOriginY))return out;
            f.hidden=0;const auto movement=originalFigureRoam(f,unsigned(speed->second),terrainFlags,local);
            if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        }else{const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
            if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
            if(f.direction>=8){if(f.type==28&&f.direction<=10)f.alive=2;else return out;}}
    }else {if(f.action!=8||f.roamSteps>=f.roamLimit)return out;
        f.hidden=0;const auto movement=originalFigureRoam(f,unsigned(speed->second),terrainFlags,local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
    }
    const unsigned code=f.type==23?0x4c01:f.type==28?0x4c55:f.type==29?0x4c1b:0x4c09;
    if(!originalFigureWriteWalkingImage(f,code,archives,context->view,false,f.type==29)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
//0x4CA960/action9, below the post-movement house-search threshold30.
inline OriginalFigureFirstUpdate originalMigratingFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4ca960;out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;
    if(!context||!context->figures.count(saved.slot)||saved.action!=9)return out;
    const auto speed=context->modelSpeeds.find(13);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=3;
    f.frame=uint8_t(f.frame+1);if(f.frame>11)f.frame=0;
    const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
    if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
    if(f.direction==9){if(!local.services||f.route<0||f.route>=1000)return out;
        if(f.route>0){if(local.services->routeOwners[f.route]==int(f.slot))local.services->routeOwners[f.route]=0;f.route=0;}}
    else if(f.direction==8)f.alive=2;
    else if(f.direction>=10)return out;
    f.counter=originalFigureWord(int(f.counter)+1);if(f.counter>30){
        if(!local.buildings)return out;
        //4ADD60 scans the complete house pool;positive vacancy candidates
        //need original nearest-house and road selection,never a no-target fit.
        for(const auto& e:*local.buildings){const auto& b=e.second;
            if(b.type>=2&&b.type<18&&(b.alive==1||b.alive==3)&&b.accepting&&b.roadDistance>0&&b.freeSlots>0&&!b.worker)return out;}
        f.counter=0;
    }
    if(!originalFigureWriteWalkingImage(f,0x4c0f,archives,context->view,false,false)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
//0x4CE2D0/action9 and0x4CF380 valid trader crew. Direct crews use
//4E7520's phase synchronization; indirect crews use4E7460's substeps.
inline OriginalFigureFirstUpdate originalTraderFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;
    out.updaterVA=saved.type==45?0x4ce2d0:0x4cf380;
    if(!context||!context->figures.count(saved.slot))return out;
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.hidden=0;f.movementMode=3;
    f.frame=uint8_t(f.frame+1);if(f.frame>11)f.frame=0;
    const auto live=[](const OriginalFigure& figure){return figure.alive==1&&figure.action!=4;};
    if(f.type==45){const auto follower=context->figures.find(size_t(f.follower));
        if(f.action!=9||follower==context->figures.end()||!live(follower->second))return out;
        const auto speed=context->modelSpeeds.find(45);if(speed==context->modelSpeeds.end())return out;
        const auto moved=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!moved.state){out.status=moved.status;return out;}f=*moved.state;if(f.direction>=8)return out;
    }else {const auto immediate=context->figures.find(size_t(f.leaderLink));if(f.action!=6||immediate==context->figures.end()||!live(immediate->second))return out;
        const auto& lead=immediate->second;const OriginalFigure* trader=&lead;
        if(trader->type==47){const auto next=context->figures.find(size_t(trader->leaderLink));if(next==context->figures.end())return out;trader=&next->second;}
        if(trader->type!=45||!live(*trader))return out;
        if(lead.type==45){const bool atHome=f.x==f.homeX&&f.y==f.homeY;bool copy=true;if(atHome)f.hidden=1;
            if(f.progress==20&&atHome&&(lead.progress<18||lead.progress==20))copy=false;
            else {int p=int(lead.progress)-18;f.progress=uint8_t(p<0?p+20:p);}
            if((f.progress==10||f.progress==11)&&(f.x!=lead.x||f.y!=lead.y)){
                if(!local.cropBase){out.status=OriginalFigureUpdateStatus::CellContext;return out;}
                f.priorX=f.x;f.priorY=f.y;f.priorCell=f.cell;f.x=lead.x;f.y=lead.y;
                const int64_t cell=int64_t(*local.cropBase)+f.x+228LL*f.y;
                if(cell<0||cell>=51984||!originalFigureRelink(f,uint32_t(cell),local)){out.status=OriginalFigureUpdateStatus::CellContext;return out;}
                if(!originalFigureCellHeight(f,local)){out.status=OriginalFigureUpdateStatus::MovementHeight;return out;}}
            if(f.progress<2){f.previousDirection=f.direction;f.direction=lead.direction<8?lead.direction:lead.previousDirection<8?lead.previousDirection:f.direction;}
            if(!f.state85){if(f.height)--f.height;else f.hidden=0;}
            else if(f.state85==100||f.state85==101){out.status=OriginalFigureUpdateStatus::MovementHeight;return out;}
            else {--f.state85;if(!f.state85)f.hidden=f.drawClass=0;
                else {const auto approach=[&](){if(f.height<f.cartStyle)++f.height;else if(f.height>f.cartStyle)--f.height;};
                    approach();if(!(local.terrain[f.cell]&0x800000))approach();}}
            if(copy)f.frame=lead.frame;
        }else {if(lead.type!=47)return out;const auto speed=context->modelSpeeds.find(45);if(speed==context->modelSpeeds.end())return out;
            unsigned steps=std::max(1u,std::min(3u,originalFigureFollowerSteps(unsigned(speed->second),trader->routePhase)));
            if(std::max(std::abs(int(f.x)-lead.x),std::abs(int(f.y)-lead.y))>2)++steps;
            if((f.x==f.homeX&&f.y==f.homeY)||(f.x==lead.x&&f.y==lead.y))f.hidden=1;
            for(unsigned n=0;n<steps;++n){if(f.progress>=19){f.progress=20;f.direction=originalFigureDirectionTo(f.x,f.y,lead.priorX,lead.priorY);
                    if(f.direction>7)break;f.previousDirection=f.direction;f.progress=0;}
                else ++f.progress;
                auto interior=f;interior.progress=0;interior.route=1;interior.routePhase=0;
                const auto step=originalFigureFollowRoute(interior,6,terrainFlags);if(!step.state){out.status=step.status;return out;}
                f.subX=step.state->subX;f.subY=step.state->subY;f.height=step.state->height;f.state85=step.state->state85;
                f.hidden=step.state->hidden;f.drawClass=step.state->drawClass;
                if(f.progress==10){const auto crossed=originalFigureCrossCell(f,local);if(crossed!=OriginalFigureUpdateStatus::Computed){out.status=crossed;return out;}
                    terrainFlags=local.terrain[f.cell];}
            }
        }
        // Both live image branches explicitly clear hidden after following.
        if(lead.hidden)f.hidden=1;
    }
    const OriginalFigure* selectedLeader=&f;
    if(f.type==47){selectedLeader=&local.figures.at(size_t(f.leaderLink));
        if(selectedLeader->type==47)selectedLeader=&local.figures.at(size_t(selectedLeader->leaderLink));}
    if(!context->factionModels||selectedLeader->faction>=22)return out;
    const int model=context->factionModels->at(selectedLeader->faction);if(model<0||model>2)return out;
    const uint32_t code=f.type==45?(model==0?0x4e81:model==1?0x4eb4:0x4eb2):(model==0?0x1c12:model==1?0x1c18:0x1c16);
    auto draw=f;if(f.type==47&&selectedLeader->action==8)draw.frame=0;
    if(!originalFigureWriteWalkingImage(draw,code,archives,context->view)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    if(f.type==47)f.hidden=0;f.image=draw.image;originalFigurePublishWrites(out,f,*context,local);return out;
}
inline OriginalFigureFirstUpdate originalPheasantFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x59f310;out.status=OriginalFigureUpdateStatus::PheasantPredicates;
    if(saved.type!=76)return out;
    if(saved.action!=18&&saved.action!=19&&saved.action!=30&&saved.action!=31&&saved.action!=32)return out;
    if((saved.action==19||saved.action==30)&&(!context||!context->services||!context->passability)){
        out.status=OriginalFigureUpdateStatus::PheasantDestination;return out;}
    if(!context||context->view>6||(context->view&1)||!context->figures.count(saved.slot)){
        out.status=OriginalFigureUpdateStatus::PheasantContext;return out;}
    uint32_t linkedBuilding=saved.linkedBuilding;
    if(linkedBuilding){
        if(!context->hunterLinks){out.status=OriginalFigureUpdateStatus::PheasantBuilding;return out;}
        const auto found=context->hunterLinks->find(linkedBuilding);
        if(found==context->hunterLinks->end()){out.status=OriginalFigureUpdateStatus::PheasantBuilding;return out;}
        const auto& b=found->second;
        if(!b.enabled||(b.alive!=1&&b.alive!=3)||!b.workers)linkedBuilding=0;
    }
    const auto distance=[&](const OriginalFigure& f){return std::max(std::abs(int(saved.x)-f.x),std::abs(int(saved.y)-f.y));};
    const auto linked=[&](uint32_t slot)->const OriginalFigure*{
        const auto found=context->figures.find(slot);return found==context->figures.end()?nullptr:&found->second;};
    //0x59DB70 rejects types75..77 before the unresolved combat qualification.
    //0x59DDE0 scans all live slots for radius>=10; threat search radius is3.
    // The radius<10 branch scans rings0..2. Nearby candidates require a cited
    // false combat branch; unresolved candidates withhold the whole update.
    const auto harmlessLink=[&](int slot){const auto it=context->figures.find(size_t(slot));return it!=context->figures.end()&&
        (!it->second.alive||it->second.action==4||(it->second.type>=75&&it->second.type<=77)||originalFigureHarmlessToPheasant(it->second,*context));};
    if((saved.target>0&&!harmlessLink(saved.target))||(saved.threat>0&&!harmlessLink(int(saved.threat)))||saved.attackerCount>2||
        (saved.attackerCount&&std::any_of(saved.attackers.begin(),saved.attackers.end(),[&](int slot){return !harmlessLink(slot);})))
        {out.status=OriginalFigureUpdateStatus::PheasantThreat;return out;}
    for(const auto& entry:context->figures){const auto& f=entry.second;
        if(f.slot!=saved.slot&&f.alive&&f.action!=4&&!(f.type>=75&&f.type<=77)&&distance(f)<=3){
            if(!originalFigureHarmlessToPheasant(f,*context)){out.status=OriginalFigureUpdateStatus::PheasantThreat;return out;}}}
    auto f=saved;f.image=f.secondaryImage=0;auto local=*context;
    std::optional<OriginalFigureSavedServices> updatedServices;
    f.linkedBuilding=linkedBuilding;
    f.threat=0;f.threatDistance=100; //0x59EB33..4A: no qualified threat in this domain.
    //0x59E670 preserves a valid target INCLUDING self. Search0x59DBD0 excludes
    // self, visits the saved/load animal vector ascending, and returns the
    // first<=12 (early threshold16). Its output is the VECTOR ORDINAL, which
    //0x59E773 then uses as a pool slot, even0. Preserve that original quirk.
    const auto* target=linked(f.herdTarget);bool herdQualified=true;
    if(!f.herdTarget||!target||!target->alive||target->action==4||target->type!=76||distance(*target)>12){
        f.herdTarget=0;f.herdDistance=100;herdQualified=false;
        uint32_t ordinal=0;
        for(const auto& entry:context->figures){const auto& candidate=entry.second;
            if(!candidate.slot||!candidate.alive||candidate.type<69||candidate.type>77)continue;
            if(candidate.slot!=f.slot&&candidate.type==76&&candidate.action!=4&&distance(candidate)<=12){
                const auto* reread=linked(ordinal);
                if(!reread){out.status=OriginalFigureUpdateStatus::PheasantContext;return out;}
                f.herdTarget=ordinal;f.herdDistance=uint32_t(distance(*reread));herdQualified=true;break;}
            ++ordinal;
        }
    }
    //0x59E350 is side-effect-free false for this class:virtual+1AC=0x413A00.
    //0x59EBD0's read-only spawn proximity result is unused by actions31/32.
    f.movementMode=5; //VA0x85CD84/file0x45CD84, type76 row+2C.
    f.freeMovement=0;f.hidden=0;
    for(auto& cooldown:f.cooldowns)if(cooldown>0)--cooldown;
    if(saved.action==19||saved.action==30){
        // No threat/mate; successful herd predicate bypasses0x59E7B0. Whether
        // proximity0x59EBD0 is true or false, the first successful destination
        // call0x59EDE0 ends the ordered predicate chain. No movement follows.
        out.status=OriginalFigureUpdateStatus::PheasantDestination;
        const bool nearSpawn=std::any_of(context->passability->spawn.spawns.begin(),context->passability->spawn.spawns.end(),[&](const auto& p){
            return std::max(std::abs(int(f.x)-p.first),std::abs(int(f.y)-p.second))<=14;});
        if(!herdQualified&&!nearSpawn){
            //59E7B0 retries herd search with maximum100/early12. Its search
            // returns an animal-vector ordinal, then pool-rereads that ordinal.
            // EBX/EDI stay -1 on this search branch (59E860..59E8D7).
            uint32_t ordinal=0,chosen=0;int nearest=100;bool found=false;
            for(const auto& e:local.figures){const auto& candidate=e.second;
                if(!candidate.slot||!candidate.alive||candidate.type<69||candidate.type>77)continue;
                if(candidate.slot!=f.slot&&candidate.type==76&&candidate.action!=4&&distance(candidate)<=nearest){
                    nearest=distance(candidate);chosen=ordinal;if(nearest<=12){found=true;break;}}
                ++ordinal;}
            found|=chosen!=0;
            if(found){const auto reread=local.figures.find(chosen);if(reread==local.figures.end())return out;
                f.herdTarget=chosen;f.herdDistance=uint32_t(distance(reread->second));auto services=*local.services;
                f.destinationX=originalFigureWord(-1+12-originalFigureRandomNext(services)%26);
                f.destinationY=originalFigureWord(-1+12-originalFigureRandomNext(services)%26);
                if(!local.cropBase)return out;
                f.destinationCell=uint32_t(int64_t(*local.cropBase)+f.destinationX+228LL*f.destinationY);
                if(f.route>0){if(f.route>=1000)return out;if(services.routeOwners[f.route]==int(f.slot))services.routeOwners[f.route]=0;f.route=0;}
                f.action=32;updatedServices=std::move(services);
            }
        }
        if(!updatedServices){
            const auto& map=*context->passability;
            const auto reached=originalFigureReachableCells(map,f.x,f.y);
            if(!reached)return out;
            const auto reachable=[&](int x,int y){
                const int64_t cell=int64_t(map.spawn.cropBase)+x+int64_t(y)*228;
                if(cell<0||cell>=51984)return false;
                return (*reached)[size_t(cell)]!=0;
            };
            int best=500,spawnX=0,spawnY=0;
            for(const auto& spawn:map.spawn.spawns){const int distance=std::max(std::abs(int(f.x)-spawn.first),std::abs(int(f.y)-spawn.second));
                if(distance>=best)continue;
                if(reachable(spawn.first,spawn.second)){best=distance;spawnX=spawn.first;spawnY=spawn.second;}}
            if(best==500)return out;
            auto services=*context->services;bool found=false;unsigned scale=3;
            for(unsigned attempt=0;attempt<=1000;++attempt){
                if(attempt==500||attempt==750)++scale;
                int x=spawnX,y=spawnY;
                if(attempt<1000){x+=int(14*scale/4)-originalFigureRandomNext(services)%int(15*scale/2);
                    y+=int(14*scale/4)-originalFigureRandomNext(services)%int(15*scale/2);}
                if(reachable(x,y)){f.destinationX=int16_t(x);f.destinationY=int16_t(y);
                    f.destinationCell=uint32_t(int64_t(map.spawn.cropBase)+x+int64_t(y)*228);found=true;break;}
            }
            if(!found)return out; // Success at the reachable spawn is guaranteed at1000.
            if(f.route>0){if(f.route>=1000)return out;
                if(services.routeOwners[f.route]==int(f.slot))services.routeOwners[f.route]=0;f.route=0;}
            f.action=32;updatedServices=std::move(services);
        }
    }else if(saved.action==18){
        //59F7EC[action-4]=3 ->59F445: enter31, then the already
        //qualified false threat and false+E0=59E550 leave it there.
        f.action=31;
    }else if(saved.action==31){
        f.idleCounter+=1; //0x4C9750 increments modulo32 before signed >=200.
        const int64_t idle=f.idleCounter>INT32_MAX?int64_t(f.idleCounter)-4294967296LL:f.idleCounter;
        if(idle>=200)f.action=30;
    }else if(!herdQualified&&!f.herdTarget){f.action=30;} //0x59F48D..496;the predicate result is distinct from vector ordinal0.
    else {
        const auto speed=context->modelSpeeds.find(76);
        if(speed==context->modelSpeeds.end()||speed->second<0||speed->second>255){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
        //0x59F498 halves model speed before the shared fractional schedule.
        const unsigned half=speed->second<=3?1:unsigned(speed->second)/2;
        const auto movement=originalFigureFollowRoute(f,half,terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.direction==9){if(f.target>0){out.status=OriginalFigureUpdateStatus::PheasantRoute;return out;}f.action=30;}
        else if(f.direction==8){
            //59F1C0 ->59E550: Pheasant+1B4=413A00(false), then
            //4C9740 clears idle counter and enters31, without movement.
            f.idleCounter=0;f.action=31;
        }else if(f.direction>=8){out.status=OriginalFigureUpdateStatus::PheasantRoute;return out;}
        //0x4E1B00/0x59D880/0x59D860/0x4E1AF0 inspect only direction
        //10/9/8 respectively;in-cell direction0..7 reaches no arrival writes.
    }
    //Action31 uses entry3;the just-entered30 uses entry9. Terrain replacement
    // applies before frame advance;both speed arguments are0.
    const uint32_t code=(terrainFlags&0x0c000100)?0x4c2b:f.action==31?0x4c2c:0x4c2b;
    const auto group=originalFigureImageGroup(code,archives);
    if(!group){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    const auto advanced=originalFigureAdvanceFrame(f.frame,group->second->animationSpriteCount,group->second->animationSpeedId,0,{});
    if(!advanced){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    f.frame=advanced->counter;
    int direction=f.direction<8?f.direction:f.previousDirection;
    direction-=int(context->view);if(direction<0)direction+=8;
    const auto directions=group->second->animationDirections;
    if(f.direction>127||f.previousDirection>127||direction<0||!directions||directions>8){
        out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    const uint64_t image=uint64_t(group->first)+f.frame*directions+std::min(unsigned(direction),unsigned(directions)-1);
    if(image>>14!=group->first>>14||(image&0x3fff)>=archives.at(code>>9)->sprites.size()){
        out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    f.image=uint32_t(image);originalFigurePublishWrites(out,f,*context,local);
    if(updatedServices)out.services=std::move(updatedServices);return out;
}
// One per-type virtual update, after callback clearing, with an explicit k=1
// premise. This does not claim to run the entire figure-pool/city pass. Kind13
// with no cell crossing has no world, RNG, speed or owner dependency. Crossing
// needs the cell-list mutations at0x4EA610/0x4EA3B0 and is explicitly refused.
inline OriginalFigureFirstUpdate originalFigureFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context=nullptr,uint32_t terrainFlags=0){
    OriginalFigureFirstUpdate out;
    if(!saved.slot||saved.slot>=2000||saved.alive!=1||saved.updateOwner){out.status=OriginalFigureUpdateStatus::DispatchGate;return out;}
    if(saved.runtimeClass==OriginalFigureClass::Pheasant)return originalPheasantFirstUpdate(saved,archives,context,terrainFlags);
    if(saved.runtimeClass!=OriginalFigureClass::Figure)return out;
    switch(saved.type){
    case 1:break;
    case 20:return originalCartFirstUpdate(saved,archives,context,terrainFlags);
    case 19:case 21:case 22:return originalGoodsFirstUpdate(saved,archives,context,terrainFlags);
    case 11:return originalWorkerFirstUpdate(saved,archives,context,terrainFlags);
    case 40:return originalHunterFirstUpdate(saved,archives,context,terrainFlags);
    case 54:return originalProjectileFirstUpdate(saved,archives,context);
    case 26:return originalSupplierFirstUpdate(saved,archives,context,terrainFlags);
    case 13:return originalMigratingFirstUpdate(saved,archives,context,terrainFlags);
    case 30:case 34:case 35:case 36:return originalDelegatedFirstUpdate(saved,archives,context,terrainFlags);
    case 23:case 28:case 29:case 39:return originalServiceFirstUpdate(saved,archives,context,terrainFlags);
    case 45:case 47:return originalTraderFirstUpdate(saved,archives,context,terrainFlags);
    default:return out;
    }
    out.updaterVA=0x4cd210;
    if(saved.effectKind!=13||saved.effectVariant||saved.effectFlags!=0x24){out.status=OriginalFigureUpdateStatus::EffectState;return out;}
    const int localX=int(saved.effectX)+saved.effectVelocityX,localY=int(saved.effectY)+saved.effectVelocityY;
    if((saved.effectVelocityX&&(localX<0||localX>=120))||(saved.effectVelocityY&&(localY<0||localY>=120))){
        out.status=OriginalFigureUpdateStatus::CellListMutation;return out;}
    const auto group=originalFigureImageGroup(0x1c0f,archives);
    if(!group||!group->second->animationSpriteCount){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    auto f=saved;f.image=f.secondaryImage=0;
    f.effectX=int8_t(localX);f.effectY=int8_t(localY);
    if(f.effectVelocityZ){ //16-bit addition precedes signed ground clamp0x417C1F.
        const unsigned bits=unsigned(int(f.effectHeight)+f.effectVelocityZ)&0xffff;
        f.effectHeight=int16_t(bits>32767?int(bits)-65536:int(bits));
        if(f.effectHeight<0)f.effectHeight=0;
    }
    const auto word=[](int value){const unsigned bits=unsigned(value)&0xffff;return int16_t(bits>32767?int(bits)-65536:int(bits));};
    f.subX=word(int(f.effectX)/8+int(f.x)*15);f.subY=word(int(f.effectY)/8+int(f.y)*15);
    f.raise=uint8_t(int(f.effectHeight)/8+30);
    f.frame=uint8_t(f.frame+1);
    if(unsigned(f.frame)>=3u*group->second->animationSpriteCount)f.frame=0;
    f.hidden=0;f.effectDraw=1;f.secondaryImage=0;
    f.image=group->first+f.frame/3;
    if(f.image>>14!=group->first>>14||(f.image&0x3fff)>=archives.at(14)->sprites.size()){
        out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    out.state=f;out.status=OriginalFigureUpdateStatus::Computed;return out;
}
struct OriginalFigureOrderedPass {
    bool accepted=false,complete=false;
    size_t computed=0,blocked=0,prefixComputed=0,firstBlockedSlot=0;
    std::map<size_t,OriginalFigureFirstUpdate> figures;
    std::set<size_t> allocatedSlots; //diagnostic denominator,including a refused pass;no state is published.
    uint64_t crtRandomCalls=0;
    std::optional<OriginalFigureUpdateContext> context;
};
//4E291B..32 removes a newly state2 worker before advancing the pool cursor.
//4E1FA0 vectors reject type11 (59A3B0/4E2360/4E2380);+158=4C8B70
// releases the route, unlinks and clears target references, then+E8=4C72B0
// resets represented fields. The inline17C state and priorProgress190 survive.
inline bool originalFigureRemoveArrivedWorker(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if((f.type!=11&&f.type!=13&&f.type!=19&&f.type!=20&&f.type!=22&&f.type!=26&&f.type!=28&&f.type!=30&&f.type!=34&&f.type!=35&&f.type!=36&&f.type!=39&&f.type!=40)||f.alive!=2||f.runtimeClass!=OriginalFigureClass::Figure||f.updateOwner||f.linkedBuilding||
        !context.services||context.cellHeads.size()!=51984||f.cell>=51984)return false;
    if(f.type==34||f.type==40){if(!context.buildings)return false;const auto owner=context.buildings->find(size_t(f.owner));
        if(owner==context.buildings->end()||owner->second.type!=(f.type==34?0xd3:0x21)||!owner->second.delegatedCount)return false;
        if(*owner->second.delegatedCount)--*owner->second.delegatedCount;
        if(f.type==40){if(!context.hunterLinks||!context.hunterLinks->count(size_t(f.owner)))return false;
            context.hunterLinks->at(size_t(f.owner)).enabled=*owner->second.delegatedCount;}
    }else if(f.type==19||f.type==22){ //4C8D4C primary cleanup;zero cargo bypasses delivery counts.
        if(f.amount||!context.buildings)return false;const auto owner=context.buildings->find(size_t(f.owner));
        if(owner==context.buildings->end()||
            (f.type==22?owner->second.type!=0x36:(owner->second.type!=0x21&&owner->second.type!=0x23&&owner->second.type!=0x2b)))return false;
        if(f.type==19&&owner->second.primaryFigure!=int(f.slot))return false; //uncited industrial secondary cleanup remains refused.
        if(owner->second.primaryFigure==int(f.slot))owner->second.primaryFigure=0;
        else if(f.type==22&&owner->second.secondaryFigures[1]==int(f.slot))owner->second.secondaryFigures[1]=0;
    }else if(f.type==26){ //4CC9CF/4CCA07 then4C8BFA->4BFDC0.
        if(!context.buildings)return false;
        for(auto& e:*context.buildings)if(e.second.alive==1&&(e.second.type>=0xc2&&e.second.type<=0xc7)&&e.second.primaryFigure==int(f.slot))e.second.primaryFigure=0;
    }else if(f.type==28||f.type==30||f.type==35||f.type==36){ //ordinary primary-first/embedded+6C cleanup.
        if(!context.buildings)return false;const auto owner=context.buildings->find(size_t(f.owner));
        if(owner==context.buildings->end()||owner->second.type!=(f.type==28?0x48:f.type==30?0xcf:f.type==35?0xd6:0x47))return false;
        if(f.type!=36&&owner->second.primaryFigure!=int(f.slot))return false;
        if(owner->second.primaryFigure==int(f.slot))owner->second.primaryFigure=0;
        else if(f.type==36&&owner->second.secondaryFigures[1]==int(f.slot))owner->second.secondaryFigures[1]=0;
    }else if(f.type==39){ //4C8D4C..4C8E08: primary first, then embedded+6C.
        if(!context.buildings)return false;const auto owner=context.buildings->find(size_t(f.owner));
        if(owner==context.buildings->end()||owner->second.type!=0x7c)return false;
        if(owner->second.primaryFigure==int(f.slot))owner->second.primaryFigure=0;
        else if(owner->second.secondaryFigures[1]==int(f.slot))owner->second.secondaryFigures[1]=0;
    }else if(f.owner)return false;
    //4C909E tests+64,then addresses the+62 owner (not the home index).
    if(f.home&&f.type!=11)return false; //worker arrival already cleared its reciprocal home link.
    if(f.route>0){if(f.route>=1000)return false;if(context.services->routeOwners[f.route]==int(f.slot))context.services->routeOwners[f.route]=0;}
    size_t previous=0,slot=context.cellHeads[f.cell];std::set<size_t> seen;bool found=false;
    while(slot){const auto it=context.figures.find(slot);if(it==context.figures.end()||!seen.insert(slot).second||it->second.cell!=f.cell)return false;
        if(slot==f.slot){if(previous)context.figures.at(previous).next=f.next;else context.cellHeads[f.cell]=f.next;found=true;break;}
        previous=slot;slot=it->second.next;}
    if(!found)return false;
    if(f.flags120&2)for(auto& entry:context.figures)if(entry.second.target==int(f.slot))entry.second.target=0;
    auto reset=OriginalFigure{};reset.slot=f.slot;reset.recordOffset=f.recordOffset;reset.runtimeClass=f.runtimeClass;reset.priorProgress=f.priorProgress;
    reset.effectKind=f.effectKind;reset.effectVariant=f.effectVariant;reset.effectFlags=f.effectFlags;reset.effectX=f.effectX;reset.effectY=f.effectY;
    reset.effectHeight=f.effectHeight;reset.effectVelocityX=f.effectVelocityX;reset.effectVelocityY=f.effectVelocityY;reset.effectVelocityZ=f.effectVelocityZ;
    f=reset;context.figures.at(f.slot)=f;context.freeSlots.push_back(f.slot);
    while(context.highestLiveSlot&&!context.figures.at(context.highestLiveSlot).alive)--context.highestLiveSlot;
    return true;
}
//0x4E27E0,ascending pool slots. Compute the exact supported prefix; an unknown
// earlier updater poisons later context rather than leaving that slot frozen.
// The prepass0x4E2804..2818 resets/recounts external query counters,without
// changing Figure fields read by the currently supported virtual updates.
// No completed city/world pass is asserted when any virtual call is refused.
inline OriginalFigureOrderedPass originalFigureOrderedNextPass(const OriginalFigureUpdateContext& prior,
    const std::map<uint32_t,const SG3Archive*>& archives){
    OriginalFigureOrderedPass out;
    if(prior.figures.size()!=2000||prior.terrain.size()!=51984)return out;
    for(size_t slot=0;slot<2000;++slot){const auto found=prior.figures.find(slot);
        if(found==prior.figures.end()||found->second.slot!=slot)return out;}
    auto context=std::optional<OriginalFigureUpdateContext>(prior);
    if(context->goods&&context->goods->savedPhase==2){
        if(context->goods->storageSearch)context->goods->storageSearch->fill(0);
        if(context->goods->industrySearch)context->goods->industrySearch->fill(0);}
    const auto& terrainFlags=context->terrain;
    out.accepted=true;bool priorBlocked=false;
    for(size_t slot=1;slot<2000;++slot){auto f=context->figures.at(slot);if(!f.alive)continue;
        OriginalFigureFirstUpdate result;
        if(priorBlocked){result.status=OriginalFigureUpdateStatus::EarlierSlot;}
        else if(f.cell>=terrainFlags.size()){result.status=OriginalFigureUpdateStatus::DispatchGate;}
        else if(!f.type){result.status=OriginalFigureUpdateStatus::PoolRemoval;}
        else {
            bool validLink=true;
            if(f.reciprocalTarget){const auto target=context->figures.find(size_t(f.reciprocalTarget));
                if(target==context->figures.end()){result.status=OriginalFigureUpdateStatus::DispatchGate;validLink=false;}
                else if(target->second.alive!=1||target->second.target!=int(slot))f.reciprocalTarget=0;
            }
            if(!validLink){result.status=OriginalFigureUpdateStatus::DispatchGate;}
            else if(f.updateOwner){result.status=OriginalFigureUpdateStatus::Computed;result.state=f;}
            else if(f.alive!=1){result.status=OriginalFigureUpdateStatus::PoolRemoval;}
            else {
                ++f.age;
                if(f.age>19584&&(f.type==19||f.type==21||f.type==22))result.status=OriginalFigureUpdateStatus::PoolRemoval;
                else if(f.leaderLink&& !context->figures.count(size_t(f.leaderLink)))result.status=OriginalFigureUpdateStatus::DispatchGate;
                else {
                    auto before=*context;
                    context->figures.at(slot)=f;
                    if(f.leaderLink){context->figures.at(size_t(f.leaderLink)).follower=int16_t(slot);
                        if(f.leaderLink==int(slot))f.follower=int16_t(slot);}
                    result=originalFigureFirstUpdate(f,archives,&*context,terrainFlags[f.cell]);
                    if(result.state){result.state->priorProgress=result.state->progress;context->figures.at(slot)=*result.state;
                        for(const auto& write:result.linkedWrites)context->figures.at(write.first)=write.second;
                        if(result.cellHeads)context->cellHeads=*result.cellHeads;
                        if(result.services)context->services=result.services;
                        if(result.buildings)context->buildings=result.buildings;
                        if(result.watchMaximum)context->watchMaximum=*result.watchMaximum;
                        if(result.inspectorMaximum)context->inspectorMaximum=*result.inspectorMaximum;
                        if(result.goods)context->goods=result.goods;
                        if(result.roadblockGate)context->roadblockGate=result.roadblockGate;
                        if(result.freeSlots){context->freeSlots=*result.freeSlots;context->crtRandom=result.crtRandom;context->crtRandomCalls=result.crtRandomCalls;
                            context->allocatedSlots=result.allocatedSlots;context->highestLiveSlot=result.highestLiveSlot;}
                        if(result.state->alive==2){if(!originalFigureRemoveArrivedWorker(*result.state,*context)){
                                 result.state.reset();result.services.reset();result.buildings.reset();result.goods.reset();result.cellHeads.reset();result.linkedWrites.clear();
                                 result.freeSlots.reset();result.crtRandom.reset();result.roadblockGate.reset();result.allocatedSlots.clear();result.highestLiveSlot=0;
                                result.status=OriginalFigureUpdateStatus::PoolRemoval;*context=std::move(before);}
                            else {result.services=context->services;result.buildings=context->buildings;result.cellHeads=context->cellHeads;}}
                    }
                    else *context=std::move(before);
                }
            }
        }
        if(result.state){context->figures.at(slot)=*result.state;
            context->primaryImageInputs[slot]=originalFigureImageInputs(*result.state);++out.computed;}
        else {++out.blocked;if(!priorBlocked)out.firstBlockedSlot=slot;priorBlocked=true;}
        out.figures.emplace(slot,std::move(result));
    }
    out.allocatedSlots=context->allocatedSlots;out.crtRandomCalls=context->crtRandomCalls;
    out.prefixComputed=out.computed;out.complete=!out.blocked;
    for(auto& entry:out.figures)if(entry.second.state){
        if(out.complete)entry.second.state=context->figures.at(entry.first);
        else {entry.second.state.reset();entry.second.services.reset();entry.second.buildings.reset();entry.second.goods.reset();entry.second.cellHeads.reset();entry.second.linkedWrites.clear();
            entry.second.freeSlots.reset();entry.second.crtRandom.reset();entry.second.roadblockGate.reset();entry.second.allocatedSlots.clear();entry.second.highestLiveSlot=0;
            entry.second.status=OriginalFigureUpdateStatus::LaterSlotEffects;}}
    if(out.complete)out.context=std::move(context);
    else {out.blocked+=out.computed;out.computed=0;}
    return out;
}
inline OriginalFigureOrderedPass originalFigurePostLoadContext(const OriginalFigureProfile& profile,
    const std::map<uint32_t,const SG3Archive*>& archives,const std::vector<uint32_t>& terrainFlags,
    const std::map<unsigned,int>& modelSpeeds={},const OriginalFigureSavedServices* services=nullptr,
    const OriginalFigurePassability* passability=nullptr,const std::map<size_t,OriginalFigureHunterLink>* hunterLinks=nullptr,
    const std::map<size_t,OriginalFigureBuildingLink>* buildings=nullptr,
    const std::map<unsigned,std::array<int32_t,18>>& models={},std::optional<uint32_t> cropBase=std::nullopt,
    const std::vector<uint16_t>& movementMasks={},const OriginalFigureUpdateContext* additional=nullptr){
    OriginalFigureOrderedPass out;auto context=originalFigureUpdateContext(profile);
    auto canonical=profile;canonical.live.clear();
    for(const auto& f:profile.slots)if(f.alive)canonical.live.push_back(f);
    const auto callbacks=originalFigurePostLoadStep(canonical,archives);
    if(!context||!callbacks.accepted||callbacks.blocked||terrainFlags.size()!=51984)return out;
    context->modelSpeeds=modelSpeeds;
    if(services)context->services=*services;
    if(passability)context->passability=*passability;
    if(hunterLinks)context->hunterLinks=*hunterLinks;
    if(buildings)context->buildings=*buildings;context->models=models;context->terrain=terrainFlags;context->cropBase=cropBase;
    context->movementMasks=movementMasks;
    if(additional){context->houseCapacities=additional->houseCapacities;context->buildingOwners=additional->buildingOwners;
        context->feature=additional->feature;context->randomByte=additional->randomByte;context->activeSide=additional->activeSide;context->festivalVenue=additional->festivalVenue;
        context->entertainmentAvailable=additional->entertainmentAvailable;context->elevation=additional->elevation;
        context->waterOverride=additional->waterOverride;
         context->houseWatchFloors=additional->houseWatchFloors;context->watchtowerReduction=additional->watchtowerReduction;
         context->houseMarketModels=additional->houseMarketModels;
         context->inspectorReduction=additional->inspectorReduction;context->inspectorMaximum=additional->inspectorMaximum;
          context->watchMaximum=additional->watchMaximum;context->crops=additional->crops;
           context->goods=additional->goods;context->buildingEmployment=additional->buildingEmployment;}
    if(additional){context->crtRandom=additional->crtRandom;context->animationFlags=additional->animationFlags;}
    if(context->services&&context->services->population&&context->buildings&&
        !originalFigurePopulationAfterLoad(*context->services,*context->buildings))return out;
    for(const auto& entry:callbacks.figures){auto& f=context->figures.at(entry.first);
        f.image=entry.second.images->first;f.secondaryImage=entry.second.images->second;}
    out.accepted=out.complete=true;out.computed=callbacks.computed;out.context=std::move(context);return out;
}
inline OriginalFigureOrderedPass originalFigureOrderedFirstPass(const OriginalFigureProfile& profile,
    const std::map<uint32_t,const SG3Archive*>& archives,const std::vector<uint32_t>& terrainFlags,
    const std::map<unsigned,int>& modelSpeeds={},const OriginalFigureSavedServices* services=nullptr,
    const OriginalFigurePassability* passability=nullptr,const std::map<size_t,OriginalFigureHunterLink>* hunterLinks=nullptr,
    const std::map<size_t,OriginalFigureBuildingLink>* buildings=nullptr,
    const std::map<unsigned,std::array<int32_t,18>>& models={},std::optional<uint32_t> cropBase=std::nullopt,
    const std::vector<uint16_t>& movementMasks={},const OriginalFigureUpdateContext* additional=nullptr){
    auto initial=originalFigurePostLoadContext(profile,archives,terrainFlags,modelSpeeds,services,passability,hunterLinks,buildings,
        models,cropBase,movementMasks,additional);
    if(!initial.context)return initial;return originalFigureOrderedNextPass(*initial.context,archives);
}
// Opt-in post-callback state after explicit k admitted virtual updates. For
// context-dependent figures k>1 needs intervening pool/world mutations, which
// are refused. The independent kind13 branch can repeat until its cell gate.
inline OriginalFigureFirstUpdate originalFigureStateAfterUpdates(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,unsigned k,
    const OriginalFigureUpdateContext* context=nullptr,uint32_t terrainFlags=0,unsigned view=0){
    OriginalFigureFirstUpdate out;
    if(k>10000){out.status=OriginalFigureUpdateStatus::OrderedPoolPass;return out;}
    const auto callback=originalFigurePostLoadImages(saved.slot,originalFigureImageInputs(saved),archives,view);
    if(!callback.images){out.status=OriginalFigureUpdateStatus::DispatchGate;return out;}
    auto f=saved;f.image=callback.images->first;f.secondaryImage=callback.images->second;
    if(!k){out.status=OriginalFigureUpdateStatus::Computed;out.state=f;return out;}
    if(k>1&&saved.type!=1){out.status=OriginalFigureUpdateStatus::OrderedPoolPass;return out;}
    for(unsigned step=0;step<k;++step){out=originalFigureFirstUpdate(f,archives,context,terrainFlags);
        if(!out.state)return out;f=*out.state;}
    return out;
}
} // namespace game_lab
