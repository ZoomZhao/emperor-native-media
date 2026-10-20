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
    MovementModel,RouteGeneration,RouteBoundary,MovementHeight,EarlierSlot,PoolRemoval,LaterSlotEffects,
    CellContext,CellCombat,SegmentService,NextDirection};
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
    std::optional<std::map<size_t,OriginalFigureBuildingLink>> buildings;
    std::map<unsigned,std::array<int32_t,18>> models;
    std::vector<uint16_t> cellHeads;
    std::vector<uint32_t> terrain;
    std::optional<uint32_t> cropBase;
    std::vector<uint16_t> movementMasks; // Full explicit5AD440 grid; never the reduced51C reachability mask.
    std::optional<std::array<int16_t,22>> factionModels;
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
    OriginalFigureUpdateContext out;out.view=profile.savedView;out.cellHeads=profile.cellHeads;out.factionModels=profile.factionModels;
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
struct OriginalFigureMovement {OriginalFigureUpdateStatus status;std::optional<OriginalFigure> state;};
inline int16_t originalFigureWord(int value){const unsigned bits=unsigned(value)&65535;return int16_t(bits>32767?int(bits)-65536:int(bits));}
// A sufficient false branch of0x4E5340 for a type76 prey: virtual+80=false,
// candidate model attack and missile attack both zero. Other relations refuse.
inline bool originalFigureHarmlessToPheasant(const OriginalFigure& candidate,const OriginalFigureUpdateContext& context){
    const auto model=context.models.find(candidate.type);
    return candidate.runtimeClass==OriginalFigureClass::Figure&&candidate.type!=78&&candidate.type!=79&&
        model!=context.models.end()&&model->second[2]==0&&model->second[5]==0;
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
inline OriginalFigureUpdateStatus originalFigureCrossCell(OriginalFigure& f,OriginalFigureUpdateContext& context){
    if(f.direction>7||context.terrain.size()!=51984||!context.cropBase||
        int64_t(*context.cropBase)+f.x+228LL*f.y!=f.cell)return OriginalFigureUpdateStatus::CellContext;
    constexpr int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
    const int64_t cell=int64_t(f.cell)+dx[f.direction]+228*dy[f.direction];
    if(cell<0||cell>=51984)return OriginalFigureUpdateStatus::CellContext;
    const auto flags=context.terrain[size_t(cell)];
    // Flat land/no bridge is a complete0x4E77C0 branch for these types.
    if(flags&(0x800000|0x10000400|0x1000000|4)||
        (f.type!=11&&f.type!=19&&f.type!=20&&f.type!=21&&f.type!=22&&f.type!=40&&f.type!=76))
        return OriginalFigureUpdateStatus::MovementHeight;
    auto next=f;next.priorX=f.x;next.priorY=f.y;
    next.x=originalFigureWord(int(f.x)+dx[f.direction]);next.y=originalFigureWord(int(f.y)+dy[f.direction]);
    next.priorCell=uint32_t(int64_t(*context.cropBase)+next.priorX+228LL*next.priorY);
    if(!originalFigureRelink(next,uint32_t(cell),context))return OriginalFigureUpdateStatus::CellContext;
    next.water=bool(flags&0x40);next.state85=0;next.height=0;if(f.state85==100||f.state85==101)next.drawClass=0;
    //0x4E57A0 has no combat work for model category0, or an otherwise empty
    // destination list. Nontrivial neighbour qualification remains explicit.
    const auto model=context.models.find(f.type);
    if(next.runtimeClass==OriginalFigureClass::Emissary||
        context.figures.at(context.cellHeads[size_t(cell)]).runtimeClass==OriginalFigureClass::Emissary)
        return OriginalFigureUpdateStatus::CellCombat;
    if(!next.hidden&&next.action!=4&&next.action!=5&&(model==context.models.end()||model->second[0]!=0)){
        for(size_t slot=context.cellHeads[size_t(cell)];slot;slot=context.figures.at(slot).next)
            if(slot!=f.slot)return OriginalFigureUpdateStatus::CellCombat;
    }
    f=next;context.figures.at(f.slot)=f;return OriginalFigureUpdateStatus::Computed;
}
//0x4E7EB0/0x4E92C0: bounded route following with explicit crossing context.
// Owner services, unresolved next-cell validation and bridge tables refuse.
inline OriginalFigureMovement originalFigureFollowRouteInContext(const OriginalFigure& saved,unsigned speed,uint32_t terrainFlags,
    OriginalFigureUpdateContext* context=nullptr){
    if(speed>255)return {OriginalFigureUpdateStatus::MovementModel,{}};
    if(saved.route<1)return {OriginalFigureUpdateStatus::RouteGeneration,{}};
    const auto schedule=originalFigureMovementSteps(speed,saved.routePhase);
    if(saved.direction>7||saved.progress>20)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
    auto f=saved;
    for(unsigned step=0;step<schedule.first;++step){
        if(f.progress>=19){
            if(!context)return {OriginalFigureUpdateStatus::RouteBoundary,{}};
            //0x4EACD0: table service category-1 means no grid write; no owner
            // means no owner virtual+28. Supported callers have both premises.
            if(f.owner||(f.type!=11&&f.type!=19&&f.type!=20&&f.type!=21&&f.type!=22&&f.type!=40&&f.type!=76))return {OriginalFigureUpdateStatus::SegmentService,{}};
            if(!context->services)return {OriginalFigureUpdateStatus::NextDirection,{}};
            f.progress=20;const auto selected=originalFigureSavedRouteDirection(f,*context->services);
            if(!selected)return {OriginalFigureUpdateStatus::NextDirection,{}};
            f=selected->state;context->services=selected->services;
            if(f.direction>7){f.routePhase=schedule.second;return {OriginalFigureUpdateStatus::Computed,f};}
            const int delta[]={-228,-227,1,229,228,227,-1,-229};const int64_t cell=int64_t(f.cell)+delta[f.direction];
            //0x4E8BC0 default movement: grid/bridge gates precede the road
            // shortcut. A reduced51C reachability mask cannot prove them.
            if(cell<0||cell>=51984||context->terrain.size()!=51984||
                context->movementMasks.size()!=51984||f.effectDraw||
                (f.movementMode!=1&&f.movementMode!=3)||(context->movementMasks[size_t(cell)]&0xa00)||
                !(context->terrain[size_t(cell)]&0x40)||(context->terrain[size_t(cell)]&(0x100|4)))
                return {OriginalFigureUpdateStatus::NextDirection,{}};
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
    if(result.state)*context=std::move(local);return result;
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
        if(entry.first!=f.slot&&(entry.second.next!=prior.next||entry.second.secondaryImage!=prior.secondaryImage))out.linkedWrites.emplace(entry);}
    if(before.cellHeads!=after.cellHeads)out.cellHeads=after.cellHeads;
    out.services=after.services;out.state=f;out.status=OriginalFigureUpdateStatus::Computed;
}
inline bool originalFigureBulkGoods(const OriginalFigure& f){const unsigned r=f.subtype?f.subtype:f.resource;return r==10||r==18||r==20;}
inline bool originalFigureSpecialGoods(const OriginalFigure& f){const unsigned r=f.subtype?f.subtype:f.resource;
    return originalFigureBulkGoods(f)||r==11||r==14||r==15||r==16||r==17||r==21||r==23||r==25||r==26||r==27;}
inline OriginalFigureFirstUpdate originalCartFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4d4c70;out.status=OriginalFigureUpdateStatus::CartLeaderMovement;
    if(!context||!context->figures.count(saved.slot)||saved.leaderLink<=0)return out;
    auto local=*context;const auto immediate=local.figures.find(size_t(saved.leaderLink));if(immediate==local.figures.end())return out;
    const auto lead=immediate->second;auto goods=immediate;std::set<size_t> seen;
    for(unsigned n=0;n<8;++n){if(!seen.insert(goods->first).second)return out;
        if(goods->second.type==19||goods->second.type==21||goods->second.type==22)break;
        if(n==7||goods->second.leaderLink<=0)return out;goods=local.figures.find(size_t(goods->second.leaderLink));if(goods==local.figures.end())return out;}
    if(goods->second.alive!=1||lead.alive!=1||lead.action==4||saved.action<6||saved.action>9)return out;
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
    //0x4CB6B0 precedes movement in these branches. Preserve its old cart style;
    //0x4CB910 adds the resulting direction only after movement.
    const auto cargo=figureSecondaryImage(originalFigureImageInputs(f),archives,context->view);
    if(cargo.status!=OriginalFigureImageStatus::Selected){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    if(f.type==19&&f.action==6){if(!context->buildings)return out;const auto owner=context->buildings->find(size_t(f.owner));
        if(owner==context->buildings->end()||owner->second.alive!=1||!(terrainFlags&0x40)||
            (terrainFlags&(0x100|0x800|0x80000|0x10004)))return out;
        f.frame=0;f.counter=originalFigureWord(int(f.counter)+1);if(f.counter>30)return out;
    }else if((f.type==19&&f.action==13)||(f.type==22&&(f.action==7||f.action==9))){
        const auto speed=context->modelSpeeds.find(f.type);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
        const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.direction>=8)return out; // Arrival/rejection need delivery virtuals.
    }else return out;
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
    if(!context||!context->buildings||saved.action!=7||!context->figures.count(saved.slot))return out;
    const auto home=context->buildings->find(size_t(saved.home));
    if(home==context->buildings->end()||(home->second.alive!=1&&home->second.alive!=3)||home->second.worker!=int(saved.slot)||
        home->second.type<2||home->second.type>=18||!home->second.accepting)return out;
    const auto speed=context->modelSpeeds.find(11);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=3;f.hidden=0;f.frame=uint8_t(f.frame+1);if(f.frame>=12)f.frame=0;
    const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
    if(!movement.state){out.status=movement.status;return out;}f=*movement.state;if(f.direction>=8)return out;
    if(!originalFigureWriteWalkingImage(f,f.subtype?0x4c13:0x4c15,archives,context->view,false,false)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    originalFigurePublishWrites(out,f,*context,local);return out;
}
inline OriginalFigureFirstUpdate originalHunterFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives,const OriginalFigureUpdateContext* context,uint32_t terrainFlags){
    OriginalFigureFirstUpdate out;out.updaterVA=0x4d2f10;out.status=OriginalFigureUpdateStatus::CombatRouteAndTarget;
    if(!context||!context->buildings||!context->figures.count(saved.slot))return out;
    const auto owner=context->buildings->find(size_t(saved.owner));
    if(owner==context->buildings->end()||(owner->second.alive!=1&&owner->second.alive!=3))return out;
    auto local=*context;auto f=saved;f.image=f.secondaryImage=0;f.movementMode=3;f.freeMovement=f.hidden=0;
    uint32_t code=0;
    if(f.action==8){f.counter=originalFigureWord(int(f.counter)+1);if(f.counter>12)return out;
        const auto group=originalFigureImageGroup(0x4ec1,archives);if(!group||f.counter<0){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
        f.image=group->first+unsigned(f.counter);
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
            if(!t.hidden&&distance<=model->second[6]){f.counter=originalFigureWord(int(f.counter)-1);if(f.counter<=0)return out;}
            if(distance<=2)return out;code=0x4ebf;
        }else code=0x4ec2;
        const auto speed=context->modelSpeeds.find(40);if(speed==context->modelSpeeds.end()){out.status=OriginalFigureUpdateStatus::MovementModel;return out;}
        const auto movement=originalFigureFollowRoute(f,unsigned(speed->second),terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;if(f.direction>=8)return out;
        if(!originalFigureWriteWalkingImage(f,code,archives,context->view,true,true,true)){out.status=OriginalFigureUpdateStatus::MissingGroup;return out;}
    }else return out;
    originalFigurePublishWrites(out,f,*context,local);return out;
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
    // The radius<10 branch scans rings0..2. Nearby candidates require a cited
    // false combat branch; unresolved candidates withhold the whole update.
    if(saved.target>0||saved.attackerCount){out.status=OriginalFigureUpdateStatus::PheasantThreat;return out;}
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
        const auto movement=originalFigureFollowRoute(f,half,terrainFlags,&local);
        if(!movement.state){out.status=movement.status;return out;}f=*movement.state;
        if(f.direction>=8){out.status=OriginalFigureUpdateStatus::PheasantRoute;return out;}
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
    case 30:case 34:case 35:case 36:out.updaterVA=0x4d5d00;out.status=OriginalFigureUpdateStatus::BuildingVirtualUpdate;return out;
    case 13:case 23:case 26:case 28:case 29:case 39:case 45:case 47:
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
    const OriginalFigurePassability* passability=nullptr,const std::map<size_t,OriginalFigureHunterLink>* hunterLinks=nullptr,
    const std::map<size_t,OriginalFigureBuildingLink>* buildings=nullptr,
    const std::map<unsigned,std::array<int32_t,18>>& models={},std::optional<uint32_t> cropBase=std::nullopt){
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
                        for(const auto& write:result.linkedWrites)context->figures.at(write.first)=write.second;
                        if(result.cellHeads)context->cellHeads=*result.cellHeads;
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
