#pragma once
#include "original_figure_image.hpp"

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
    MovementModel,RouteGeneration,RouteBoundary,MovementHeight,EarlierSlot,PoolRemoval,LaterSlotEffects};
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
};
struct OriginalFigurePassability {
    OriginalFigureSpawnServices spawn;
    std::vector<uint16_t> cells;
};
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
};
//0x4189B0 advances both persisted registers31 times;destination0x59EDE0
// consumes first&0x7FFF after each call. No process-start seed is substituted.
inline uint16_t originalFigureRandomNext(OriginalFigureSavedServices& state){
    for(unsigned n=0;n<31;++n){const auto advance=[](uint32_t value){
            return (value>>1)|((((value>>4)^value)&1u)<<30);};
        state.randomFirst=advance(state.randomFirst);state.randomSecond=advance(state.randomSecond);}
    return uint16_t(state.randomFirst&0x7fff);
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
    OriginalFigureUpdateContext out;out.view=profile.savedView;
    for(auto f:profile.slots){
        if(f.slot>=2000)return std::nullopt;
        f.image=f.secondaryImage=0;
        if(!out.figures.emplace(f.slot,f).second)return std::nullopt;
    }
    return out;
}
//0x5D2030 reads eighteen numbers per row;0x4C9310(type,8) reads speed.
// A speed whose unit-type flag requests difficulty scaling is withheld here.
// This bounded parser accepts the documented comma/braces model grammar and
// publishes only after all five sections and their exact row counts close.
inline std::optional<std::map<unsigned,int>> originalFigureModelSpeeds(const std::string& text){
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
    std::map<unsigned,int> out;
    for(size_t type=0;type<rows[2].size();++type){const auto& row=rows[2][type];
        if(row[0]<0||size_t(row[0])>=rows[1].size())return std::nullopt;
        if(rows[1][row[0]][8]<=0&&row[8]>=0&&row[8]<=255)out.emplace(unsigned(type),row[8]);}
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
struct OriginalFigureMovement {OriginalFigureUpdateStatus status;std::optional<OriginalFigure> state;};
//0x4E7EB0/0x4E92C0: one bounded in-cell route-following call. Crossing,
// end-of-segment next direction and bridge table lookups refuse atomically.
inline OriginalFigureMovement originalFigureFollowRoute(const OriginalFigure& saved,unsigned speed,uint32_t terrainFlags){
    if(saved.route<1)return {OriginalFigureUpdateStatus::RouteGeneration,{}};
    const auto schedule=originalFigureMovementSteps(speed,saved.routePhase);
    if(saved.direction>7||saved.progress>19)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
    auto f=saved;
    for(unsigned step=0;step<schedule.first;++step){
        if(f.progress==9||f.progress==19)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
        ++f.progress;
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
inline OriginalFigureFirstUpdate originalPheasantFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x59f310;out.status=OriginalFigureUpdateStatus::PheasantPredicates;
    if(saved.type!=76)return out;
    if(saved.action==30){out.status=OriginalFigureUpdateStatus::PheasantDestination;return out;}
    if(saved.action!=19&&saved.action!=31&&saved.action!=32)return out;
    if(saved.action==19&&(!context||!context->services||!context->passability)){
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
    // The radius<10 branch scans rings0..2. No candidate within3 is a strict
    // sufficient domain; a candidate is refused, never treated as harmless.
    if(saved.target>0||saved.attackerCount){out.status=OriginalFigureUpdateStatus::PheasantThreat;return out;}
    for(const auto& entry:context->figures){const auto& f=entry.second;
        if(f.slot!=saved.slot&&f.alive&&f.action!=4&&!(f.type>=75&&f.type<=77)&&distance(f)<=3){
            out.status=OriginalFigureUpdateStatus::PheasantThreat;return out;}}
    auto f=saved;f.image=f.secondaryImage=0;
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
    if(saved.action==19){
        // No threat/mate; successful herd predicate bypasses0x59E7B0. Whether
        // proximity0x59EBD0 is true or false, the first successful destination
        // call0x59EDE0 ends the ordered predicate chain. No movement follows.
        out.status=OriginalFigureUpdateStatus::PheasantDestination;
        if(!herdQualified)return out;
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
        const auto movement=originalFigureFollowRoute(f,half,terrainFlags);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
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
    f.image=uint32_t(image);out.state=f;out.services=std::move(updatedServices);out.status=OriginalFigureUpdateStatus::Computed;return out;
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
    case 20:out.updaterVA=0x4d4c70;out.status=OriginalFigureUpdateStatus::CartLeaderMovement;return out;
    case 19:case 21:case 22:out.updaterVA=saved.type==19?0x4cbec0:saved.type==21?0x4cd8f0:0x4d13c0;
        out.status=OriginalFigureUpdateStatus::GoodsRouteAndOwner;return out;
    case 11:out.updaterVA=0x4c9fd0;out.status=OriginalFigureUpdateStatus::WorkerOwnerMovement;return out;
    case 30:case 34:case 35:case 36:out.updaterVA=0x4d5d00;out.status=OriginalFigureUpdateStatus::BuildingVirtualUpdate;return out;
    case 13:case 23:case 26:case 28:case 29:case 39:case 40:case 45:case 47:
        out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;return out;
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
    std::optional<OriginalFigureUpdateContext> context;
};
//0x4E27E0,ascending pool slots. Compute the exact supported prefix; an unknown
// earlier updater poisons later context rather than leaving that slot frozen.
// The prepass0x4E2804..2818 resets/recounts external query counters,without
// changing Figure fields read by the currently supported virtual updates.
// No completed city/world pass is asserted when any virtual call is refused.
inline OriginalFigureOrderedPass originalFigureOrderedFirstPass(const OriginalFigureProfile& profile,
    const std::map<uint32_t,const SG3Archive*>& archives,const std::vector<uint32_t>& terrainFlags,
    const std::map<unsigned,int>& modelSpeeds={},const OriginalFigureSavedServices* services=nullptr,
    const OriginalFigurePassability* passability=nullptr,const std::map<size_t,OriginalFigureHunterLink>* hunterLinks=nullptr){
    OriginalFigureOrderedPass out;auto context=originalFigureUpdateContext(profile);
    auto canonical=profile;canonical.live.clear();
    for(const auto& f:profile.slots)if(f.alive)canonical.live.push_back(f);
    const auto callbacks=originalFigurePostLoadStep(canonical,archives);
    if(!context||!callbacks.accepted||callbacks.blocked||terrainFlags.size()!=51984)return out;
    context->modelSpeeds=modelSpeeds;
    if(services)context->services=*services;
    if(passability)context->passability=*passability;
    if(hunterLinks)context->hunterLinks=*hunterLinks;
    for(const auto& entry:callbacks.figures){auto& f=context->figures.at(entry.first);
        f.image=entry.second.images->first;f.secondaryImage=entry.second.images->second;}
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
                ++f.age; //unsigned32 increment and >19584 goods lifetime test.
                if(f.age>19584&&(f.type==19||f.type==21||f.type==22))result.status=OriginalFigureUpdateStatus::PoolRemoval;
                else if(f.leaderLink&& !context->figures.count(size_t(f.leaderLink)))result.status=OriginalFigureUpdateStatus::DispatchGate;
                else {
                    auto before=*context;
                    context->figures.at(slot)=f;
                    if(f.leaderLink){context->figures.at(size_t(f.leaderLink)).follower=int16_t(slot);
                        if(f.leaderLink==int(slot))f.follower=int16_t(slot);}
                    result=originalFigureFirstUpdate(f,archives,&*context,terrainFlags[f.cell]);
                    if(result.state){result.state->priorProgress=result.state->progress;context->figures.at(slot)=*result.state;
                        if(result.services)context->services=result.services;}
                    else *context=std::move(before);
                }
            }
        }
        if(result.state){context->figures.at(slot)=*result.state;++out.computed;}
        else {++out.blocked;if(!priorBlocked)out.firstBlockedSlot=slot;priorBlocked=true;}
        out.figures.emplace(slot,std::move(result));
    }
    out.prefixComputed=out.computed;out.complete=!out.blocked;
    // A later follower may write an earlier leader's+66. Only a complete pass
    // publishes final states. Incomplete prefix progress remains diagnostic.
    for(auto& entry:out.figures)if(entry.second.state){
        if(out.complete)entry.second.state=context->figures.at(entry.first);
        else {entry.second.state.reset();entry.second.services.reset();entry.second.status=OriginalFigureUpdateStatus::LaterSlotEffects;}}
    if(out.complete)out.context=std::move(context);
    else {out.blocked+=out.computed;out.computed=0;}
    return out;
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
