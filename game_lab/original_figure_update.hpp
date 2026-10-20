#pragma once
#include "original_figure_image.hpp"

namespace game_lab {
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
    PheasantPredicates,CartLeaderMovement,GoodsRouteAndOwner,WorkerOwnerMovement,BuildingVirtualUpdate,CombatRouteAndTarget};
inline const char* originalFigureUpdateStatusName(OriginalFigureUpdateStatus status){
    switch(status){case OriginalFigureUpdateStatus::Computed:return "computed";
    case OriginalFigureUpdateStatus::DispatchGate:return "dispatch_gate";
    case OriginalFigureUpdateStatus::UnsupportedType:return "ordered_type_update";
    case OriginalFigureUpdateStatus::EffectState:return "inline_effect_state";
    case OriginalFigureUpdateStatus::CellListMutation:return "effect_cell_list_mutation";
    case OriginalFigureUpdateStatus::MissingGroup:return "missing_sg_group";
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
};
// One per-type virtual update, after callback clearing, with an explicit k=1
// premise. This does not claim to run the entire figure-pool/city pass. Kind13
// with no cell crossing has no world, RNG, speed or owner dependency. Crossing
// needs the cell-list mutations at0x4EA610/0x4EA3B0 and is explicitly refused.
inline OriginalFigureFirstUpdate originalFigureFirstUpdate(const OriginalFigure& saved,
    const std::map<uint32_t,const SG3Archive*>& archives){
    OriginalFigureFirstUpdate out;
    if(!saved.slot||saved.slot>=2000||saved.alive!=1||saved.updateOwner){out.status=OriginalFigureUpdateStatus::DispatchGate;return out;}
    if(saved.runtimeClass==OriginalFigureClass::Pheasant){out.updaterVA=0x59f310;out.status=OriginalFigureUpdateStatus::PheasantPredicates;return out;}
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
} // namespace game_lab
