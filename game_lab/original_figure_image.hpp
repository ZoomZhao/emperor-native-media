#pragma once
#include "original_figure.hpp"
#include "emperor_assets.hpp"

namespace game_lab {
// Deliberately has no primary or secondary image field. The saved words are
// comparison targets only; selection never sees them. Evidence: SAV_FIGURES.md.
struct OriginalFigureImageInputs {
    uint8_t type=0,action=0,direction=0,previousDirection=0,frame=0,subtype=0;
    uint8_t attackDirection=0,attackFrame=0,state85=0,cartStyle=0,resource=0,stationary=0;
    int16_t counter=0,variant=0,amount=0;
    OriginalFigureClass runtimeClass=OriginalFigureClass::Figure;
    int16_t leaderLink=0,target=0,x=0,y=0;
    uint8_t alive=0,faction=0,height=0;
    uint8_t effectKind=0,effectVariant=0,effectFlags=0;
    uint32_t updateOwner=0;
    int16_t owner=0;
};
inline OriginalFigureImageInputs originalFigureImageInputs(const OriginalFigure& f){
    return {f.type,f.action,f.direction,f.previousDirection,f.frame,f.subtype,f.attackDirection,
        f.attackFrame,f.state85,f.cartStyle,f.resource,f.stationary,f.counter,f.variant,f.amount,f.runtimeClass,
        f.leaderLink,f.target,f.x,f.y,f.alive,f.faction,f.height,f.effectKind,f.effectVariant,f.effectFlags,f.updateOwner,f.owner};
}
// Cross-figure state contains only the same non-image inputs as the selector.
struct OriginalFigureImageState {
    std::map<size_t,OriginalFigureImageInputs> figures;
    std::optional<std::array<int16_t,22>> factionModels;
};
inline OriginalFigureImageState originalFigureImageState(const OriginalFigureProfile& profile){
    OriginalFigureImageState out;
    for(const auto& f:profile.live)out.figures.emplace(f.slot,originalFigureImageInputs(f));
    out.factionModels=profile.factionModels;
    return out;
}
enum class OriginalFigureImageStatus {Selected,UnsupportedType,OwnerState,ExternalState,InvalidInput,MissingGroup};
inline const char* originalFigureImageStatusName(OriginalFigureImageStatus s){
    switch(s){case OriginalFigureImageStatus::Selected:return "selected";
        case OriginalFigureImageStatus::UnsupportedType:return "unsupported_type";
        case OriginalFigureImageStatus::OwnerState:return "owner_state";
        case OriginalFigureImageStatus::ExternalState:return "external_state";
        case OriginalFigureImageStatus::InvalidInput:return "invalid_input";
        case OriginalFigureImageStatus::MissingGroup:return "missing_group";}
    return "invalid_input";
}
struct OriginalFigureImageSelection {
    OriginalFigureImageStatus status=OriginalFigureImageStatus::UnsupportedType;
    uint32_t image=0,code=0,base=0;unsigned frame=0,direction=0,frames=0,directions=0,speed=0;
    uint32_t selectorVA=0;
};
// Only the two investigated saved-zero populations. Postload0x534D13 calls
//0x4D6C40(1), table0x84E788+40*type ->0x4D6CF0 for both: clear+8/+194.
// This says nothing about whether the scheduler runs a tick before first draw.
inline bool originalFigureImageClearedAtLoad(const OriginalFigureImageInputs& f){
    return (f.type==1&&f.runtimeClass==OriginalFigureClass::Figure)||
        (f.type==76&&f.runtimeClass==OriginalFigureClass::Pheasant);
}
// Explicit post-load step: only the two image words at the return of the
// 0x4D6C40(1) callback pass, before any admitted simulation update. This is
// separate from saved/forward snapshot rendering and is not a complete Figure
// update. SAV_FIGURES.md records 0x4D6CF0 for these nineteen supplied types;
// Type11's callback selects its fixed table group without advancing the frame.
enum class OriginalFigurePostLoadStatus {ImagesCleared,ImagesSelected,MissingGroup,InvalidInput,TypeUnspecified,OutsideLivePool};
struct OriginalFigurePostLoadImages {
    OriginalFigurePostLoadStatus status=OriginalFigurePostLoadStatus::TypeUnspecified;
    uint32_t callbackVA=0;
    // Absent means unknown, never "retain the saved image" or an implicit zero.
    std::optional<std::pair<uint32_t,uint32_t>> images;
};
inline OriginalFigurePostLoadImages originalFigurePostLoadImages(size_t slot,const OriginalFigureImageInputs& f,
    const std::map<uint32_t,const SG3Archive*>& archives={},unsigned view=0);
struct OriginalFigurePostLoadStep {
    bool accepted=false;
    size_t computed=0,blocked=0;
    std::map<size_t,OriginalFigurePostLoadImages> figures;
};
inline OriginalFigurePostLoadStep originalFigurePostLoadStep(const OriginalFigureProfile& saved,
    const std::map<uint32_t,const SG3Archive*>& archives={}){
    OriginalFigurePostLoadStep out;
    if(saved.status!=OriginalFigureStatus::Complete||saved.declared!=2000||saved.parsed!=2000||saved.live.size()>2000)return out;
    for(const auto& f:saved.live)
        if(!out.figures.emplace(f.slot,originalFigurePostLoadImages(f.slot,originalFigureImageInputs(f),archives,saved.savedView)).second)return {};
    for(const auto& entry:out.figures){if(entry.second.images)++out.computed;else ++out.blocked;}
    out.accepted=true;return out;
}
// Resolve T(code) from supplied SG metadata. 0x408170 and loader0x5CD51B..524:
// lookup entries become physicalID-1, packed with startup archive slot<<14.
// Refuse sparse preceding lookup slots (engine compacts these at load).
inline std::optional<std::pair<uint32_t,const SG3Sprite*>> originalFigureImageGroup(
    uint32_t code,const std::map<uint32_t,const SG3Archive*>& archives){
    const unsigned slot=code>>9,group=code&511;
    const auto found=archives.find(slot);
    if(!group||group>300||!originalFigureArchiveName(slot)||found==archives.end()||!found->second)return std::nullopt;
    const auto& a=*found->second;
    if(a.version!=214||a.imageIndex[0]!=1||a.bitmaps.empty()||a.bitmaps[0].filename=="Zeus_system.bmp"||
       a.sprites.size()!=a.numImages)return std::nullopt;
    for(unsigned i=0;i<group;++i)if(!a.imageIndex[i])return std::nullopt;
    const unsigned id=a.imageIndex[group-1];
    if(id>a.sprites.size())return std::nullopt;
    return std::make_pair((slot<<14)+id-1,&a.sprites[id-1]);
}
inline OriginalFigurePostLoadImages originalFigurePostLoadImages(size_t slot,const OriginalFigureImageInputs& f,
    const std::map<uint32_t,const SG3Archive*>& archives,unsigned view){
    OriginalFigurePostLoadImages out;
    if(!slot||slot>=2000||!f.alive){out.status=OriginalFigurePostLoadStatus::OutsideLivePool;return out;}
    switch(f.type){
    case 11:{ //0x4D6D30 ->0x4D6E30;fixed table entries0x84E954/5C, not subtype selection.
        out.callbackVA=0x4d6d30;
        if(view>6||(view&1)||f.direction>127||(f.action==4&&f.counter<0)){
            out.status=OriginalFigurePostLoadStatus::InvalidInput;return out;}
        const auto group=originalFigureImageGroup(f.action==4?0x4c14:0x4c13,archives);
        if(!group||!group->second->animationSpriteCount){out.status=OriginalFigurePostLoadStatus::MissingGroup;return out;}
        const int stride=f.action==4?1:8;
        int direction=int(f.direction)-int(view);if(direction<0)direction+=stride;
        const unsigned frame=f.action==4?std::min(unsigned(f.counter),unsigned(group->second->animationSpriteCount)-1):f.frame;
        const int64_t word=int64_t(group->first)+frame*stride+direction;
        if(word<0||uint64_t(word)>>14!=group->first>>14||
           (uint64_t(word)&0x3fff)>=archives.at(38)->sprites.size()){
            out.status=OriginalFigurePostLoadStatus::InvalidInput;return out;}
        out.status=OriginalFigurePostLoadStatus::ImagesSelected;
        out.images=std::make_pair(uint32_t(word),0u);return out;
    }
    case 1:case 13:case 19:case 20:case 21:case 22:case 23:case 26:case 28:case 29:
    case 30:case 34:case 35:case 36:case 39:case 40:case 45:case 47:case 76:
        out.status=OriginalFigurePostLoadStatus::ImagesCleared;out.callbackVA=0x4d6cf0;
        out.images=std::make_pair(0u,0u);return out;
    default:return out;
    }
}
// Reconstruct the SELECTED saved phase, after the original updater's counter
// changes. This does not execute the updater or infer elapsed ticks.
inline OriginalFigureImageSelection figureImage(const OriginalFigureImageInputs& f,
    const std::map<uint32_t,const SG3Archive*>& archives,uint32_t terrainFlags=0,unsigned view=0,
    std::optional<uint16_t> ownerType=std::nullopt,const OriginalFigureImageState* state=nullptr){
    OriginalFigureImageSelection out;
    const auto refuse=[&](OriginalFigureImageStatus status){out.status=status;return out;};
    const auto linked=[&](int slot)->const OriginalFigureImageInputs*{
        if(!state||slot<=0)return nullptr;const auto found=state->figures.find(size_t(slot));
        return found==state->figures.end()?nullptr:&found->second;
    };
    if(view>6||(view&1)||f.direction>127||f.previousDirection>127)
        return refuse(OriginalFigureImageStatus::InvalidInput);
    if(f.runtimeClass==OriginalFigureClass::Pheasant&&f.type!=76)return refuse(OriginalFigureImageStatus::ExternalState);
    int direction=f.direction;unsigned frame=f.frame;uint32_t code=0;bool clamp=false,linear=false;
    if(f.runtimeClass==OriginalFigureClass::Emissary){ //vtable7ACD68+28=488E70
        out.selectorVA=0x488e70;code=0x4c17;
        if(f.action==4){code=0x4c18;frame=f.counter;linear=true;clamp=true;}
    }else switch(f.type){
    case 1: //0x4CD222 supplies inline+17C to0x417550; kind13 store0x418383.
        out.selectorVA=0x417c50;
        if(f.effectKind!=13||f.effectVariant!=0)return refuse(OriginalFigureImageStatus::ExternalState);
        code=0x1c0f;frame=f.frame/3;linear=true;clamp=true;direction=0;break;
    case 11: //0x4CA28B..32A, subtype+13 selects two walking groups.
        out.selectorVA=0x4c9fd0;code=f.subtype?0x4c13:0x4c15;
        if(f.action==4){++code;frame=f.counter;linear=true;clamp=true;}break;
    case 13:
        out.selectorVA=0x4ca960;code=0x4c0f;
        if(f.action==4){code=0x4c10;frame=f.counter;linear=true;clamp=true;}break;
    case 19:case 21:case 22:{ //0x4CB910; goods predicates0x4D6A00/40/80/C0.
        out.selectorVA=0x4cb910;const unsigned resource=f.subtype?f.subtype:f.resource;
        const bool special=resource==10||resource==18||resource==20||resource==11||resource==14||resource==15||
            resource==16||resource==17||resource==21||resource==23||resource==25||resource==26||resource==27;
        if(direction>=8||(special&&f.stationary)){direction=f.previousDirection;frame=0;}
        code=special?0x4ca6:((f.state85==0||f.state85==100||f.state85==101)?0x4e60:(f.cartStyle?0x4e61:0x4e62));
        if(f.action==4){code=special?0x4ca7:0x4e64;frame=f.counter;linear=true;clamp=true;direction=0;}break;
    }
    case 20:{
        //0x4D4BE0/4C50 follows at most eight +72 links to types19/21/22.
        auto leader=linked(f.leaderLink);bool direct=true;
        for(unsigned n=0;leader&&leader->type!=19&&leader->type!=21&&leader->type!=22&&n<7;++n){
            direct=false;leader=linked(leader->leaderLink);
        }
        if(!leader||(leader->type!=19&&leader->type!=21&&leader->type!=22)||leader->alive!=1)
            return refuse(OriginalFigureImageStatus::OwnerState);
        out.selectorVA=0x4d4c70;
        const unsigned resource=leader->subtype?leader->subtype:leader->resource;
        const bool bulk=resource==10||resource==18||resource==20; //0x4D6AC0
        const bool large=leader->amount>400; //0x4D6B00
        const unsigned cart=(f.state85==0||f.state85==100||f.state85==101)?0:2-unsigned(f.cartStyle!=0);
        if(direction>=8)direction=f.previousDirection;if(direction>=8)direction=0;
        if(f.action==4){ //0x4D4F01..76
            code=bulk?(direct?(large?0x4e93:0x4e3b):0x4e8b):0x4c93;
            frame=f.counter;linear=true;clamp=true;direction=0;
        }else if(!direct){ //0x4D4FA3..506E
            code=(bulk?(large?0x4e8c:0x4e88):0x4c99)+cart;
        }else if(bulk&&f.action==6){ //0x4D50AD..513C
            code=0x4e38+(large?0x58:0)+cart;
        }else if(!bulk&&f.action==7){ //0x4D5141..5185
            code=0x4c90+(cart?3-cart:0);
        }else return refuse(OriginalFigureImageStatus::OwnerState); //no +8 store in this branch
        break;
    }
    case 23:
        out.selectorVA=0x4d0270;code=0x4c01;
        if(f.action==4){code=0x4c02;frame=f.counter/2;direction=0;clamp=true;}break;
    case 26:{
        out.selectorVA=0x4cc670;if(!ownerType)return refuse(OriginalFigureImageStatus::OwnerState);
        const auto t=*ownerType;unsigned base;
        if(t==0xee)base=0x4e7d;else if(t==0xef)base=0x4e75;else if(t==0xed)base=0x4e79;
        else base=0x4e65+4*std::min(unsigned(f.variant),3u);
        code=base+(f.action==4?3:f.action==8?1:f.action==12||f.action==15?2:0);
        if(direction>=8)direction=f.previousDirection;
        if(f.action==4){frame=f.counter;linear=true;clamp=true;direction=0;}else clamp=true;break;
    }
    case 28:
        out.selectorVA=0x4d3a70;code=0x4c55;
        if(f.action==4){code=0x4c56;frame=f.counter/2;direction=0;clamp=true;}break;
    case 29:
        out.selectorVA=0x4d3b50;code=0x4c1b;
        if(direction>=8)direction=f.previousDirection;
        if(f.action==5){code=0x4c1c;direction=f.attackDirection;frame=f.attackFrame<12?0:(f.attackFrame-12)/2;}
        if(f.action==4){code=0x4c1d;frame=f.counter/2;direction=0;clamp=true;}break;
    case 39:
        out.selectorVA=0x4cd230;code=f.action==10?0x4c0a:f.action==11?0x4c0b:f.action==4?0x4c0c:0x4c09;
        if(direction>=8)direction=f.previousDirection;
        if(f.action==5||f.action==11)direction=f.attackDirection;
        if(f.action==5)frame=0;
        if(f.action==4){frame=f.counter/2;direction=0;}clamp=true;break;
    case 40:
        out.selectorVA=0x4d2f10;if(direction>=8)direction=f.previousDirection;
        if(f.action==7){ //0x4D3023..3133,0x4D358B..35D3: surviving action7 uses max(|dx|,|dy|)>2.
            const auto target=linked(f.target);
            if(!target||target->alive!=1||target->action==4||target->action==10||target->type<75||target->type>77)
                return refuse(OriginalFigureImageStatus::ExternalState);
            const int distance=std::max(std::abs(int(f.x)-target->x),std::abs(int(f.y)-target->y));
            // An expired counter may have just entered action7 from6, whose
            // local bVar4 stays true. The near-target case then needs history.
            if(distance<=2&&f.counter<=0)return refuse(OriginalFigureImageStatus::ExternalState);
            code=distance>2?0x4ebf:0x4ec0;
            if(code==0x4ec0)frame=0;clamp=true;break;
        }
        code=f.action==4?0x4ec3:f.action==5||f.action==16||f.action==13?0x4ec0:
            f.action==8?0x4ec1:f.action==10?0x4ec2:0x4ebe;
        if(f.action==8){frame=f.counter;direction=0;linear=true;}
        else if(f.action==12)frame=0;
        else if(f.action==4){frame=f.counter/2;direction=0;clamp=true;}
        else clamp=true;break;
    case 54:{ //5E6200->5E670F/5CFE60/5CFDF0;32-direction projectile.
        out.selectorVA=0x5e6200;code=0x1c6d;
        const auto shooter=linked(f.owner);if(!shooter||shooter->type!=40)return refuse(OriginalFigureImageStatus::ExternalState);
        if(direction>=32)return refuse(OriginalFigureImageStatus::InvalidInput);
        direction=(direction+4-4*int(view)+32)%32;
        const auto group=originalFigureImageGroup(code,archives);if(!group||group->second->animationSpriteCount<1||group->second->animationDirections!=32)
            return refuse(OriginalFigureImageStatus::MissingGroup);
        frame=std::min(frame,unsigned(group->second->animationSpriteCount)-1);
        const uint64_t word=uint64_t(group->first)+32*frame+unsigned(direction);
        if((word>>14)!=(group->first>>14)||(word&0x3fff)>=archives.at(14)->sprites.size())return refuse(OriginalFigureImageStatus::InvalidInput);
        out.code=code;out.base=group->first;out.frames=group->second->animationSpriteCount;out.directions=32;out.frame=frame;out.direction=unsigned(direction);
        out.image=uint32_t(word);out.status=OriginalFigureImageStatus::Selected;return out;}
    case 76:{
        if(f.runtimeClass!=OriginalFigureClass::Pheasant)return refuse(OriginalFigureImageStatus::ExternalState);
        out.selectorVA=0x59f310;if(direction>=8)direction=f.previousDirection;
        // Pheasant row1:0x85CF60 (type pointer0x85CF18 ->0x85CD58).
        if(f.action==4||f.action==10){code=0x4c2d;frame=std::min(int(f.counter),7);linear=true;direction=0;}
        else {const bool entry0=f.action==5||f.action==33||f.action==34;
            code=entry0||f.action==31?0x4c2c:0x4c2b;
            if(!entry0&&(terrainFlags&0x0c000100u))code=0x4c2b;clamp=true;}break;
    }
    case 30:case 34:case 35:case 36:{
        if(!ownerType)return refuse(OriginalFigureImageStatus::OwnerState);
        const auto owner=*ownerType;
        if(owner==0xd3){ //+24=0x48A9A0;semantic type34 selects the second image pair.
            if(f.type!=34)return refuse(OriginalFigureImageStatus::OwnerState);
            out.selectorVA=0x48a9a0;code=f.action==4?0x4c6a:0x4c67;
        }else{ //+24=0x51D0C0 (type47 thunk0x48B7E0);+24C=0x416A50 returns false.
            if(owner==0xcf)code=0x4c1e; //vtable7B6114+244=0x51BD50
            else if(owner==0x47)code=0x4ca0; //vtable7AD878+244=0x48B7C0
            else if(owner==0xd6)code=0x4c11; //vtable7BC0B4+244=0x5AB520
            else return refuse(OriginalFigureImageStatus::OwnerState);
            out.selectorVA=0x51d0c0;if(f.action==4)++code;
        }
        if(direction>=8)direction=f.previousDirection;if(f.action==5&&owner!=0xd3)direction=f.attackDirection;
        if(f.action==4){frame=owner==0xd3?std::min(int(f.counter),7):f.counter;linear=true;clamp=true;direction=0;}
        break;
    }
    case 45:case 47:{
        const auto* leader=&f;
        if(f.type==47){ //0x4E39D0: direct45 or one intermediate47, both via saved+72.
            leader=linked(f.leaderLink);
            if(leader&&leader->type==47)leader=linked(leader->leaderLink);
            if(!leader||leader->type!=45)return refuse(OriginalFigureImageStatus::ExternalState);
        }
        if(!state||!state->factionModels||leader->faction>=state->factionModels->size())
            return refuse(OriginalFigureImageStatus::ExternalState);
        const int model=state->factionModels->at(leader->faction);
        //0x440120 lazily resolves3 through0x4403E0; that external choice is not guessed.
        if(model<0||model>2)return refuse(OriginalFigureImageStatus::ExternalState);
        if(direction>=8)direction=f.previousDirection;
        if(f.type==45){ //0x4CE9A0..4D; faction virtual chain0x480740→0x481F10→0x440120.
            out.selectorVA=0x4ce2d0;code=model==0?0x4e81:model==1?0x4eb4:0x4eb2;
            if(f.action==4){++code;frame=f.counter/2;direction=0;clamp=true;}
        }else{ //0x4CF4D9..5AE;state2 chooses bounded death, leader action8 static walk.
            out.selectorVA=0x4cf380;code=model==0?0x1c12:model==1?0x1c18:0x1c16;
            if(f.alive==2){++code;frame=f.counter/2;direction=0;clamp=true;}
            else if(leader->action==8)frame=0;
        }
        break;
    }
    default:return out;
    }
    if(direction<0||direction>15)return refuse(OriginalFigureImageStatus::InvalidInput);
    if((f.action==4||(f.type==40&&f.action==8))&&f.counter<0)return refuse(OriginalFigureImageStatus::InvalidInput);
    direction-=int(view);if(direction<0)direction+=8;
    const auto group=originalFigureImageGroup(code,archives);out.code=code;
    if(!group)return refuse(OriginalFigureImageStatus::MissingGroup);
    const auto& s=*group->second;out.base=group->first;out.frames=s.animationSpriteCount;
    out.directions=s.animationDirections;out.speed=s.animationSpeedId;
    if(!out.frames||out.frames>255||!out.directions||out.directions>8)return refuse(OriginalFigureImageStatus::MissingGroup);
    if(clamp){if(!linear)frame&=255; //0x5CFDF0 masks the frame argument to u8.
        frame=std::min(frame,out.frames-1);direction=std::min(unsigned(direction),out.directions-1);}
    // Direct updaters explicitly multiply by8, including saved direction8..10.
    const uint64_t image=uint64_t(out.base)+(linear?frame:uint64_t(frame)*(clamp?out.directions:8)+direction);
    const auto& archive=*archives.at(code>>9);
    if(image>>14!=out.base>>14||(image&0x3fff)>=archive.sprites.size())return refuse(OriginalFigureImageStatus::InvalidInput);
    out.image=uint32_t(image);out.frame=frame;out.direction=direction;out.status=OriginalFigureImageStatus::Selected;return out;
}
// Diagnostic only; never installed by either tick-0 scene render source.
// Kind13 has no external-state branch in0x4175B0;0x417C50 increments+5,
//0x4181B1..209 wraps at3*SGframes and selects T(1C0F)+counter/3.
// Pheasant action19 instead enters state30 at0x59F5DA and runs virtual
// predicates before selecting; applying figureImage to its SAV state is not
// a first-tick prediction. Refuse that missing update state explicitly.
inline OriginalFigureImageSelection originalFigureFirstTickImage(OriginalFigureImageInputs f,
    const std::map<uint32_t,const SG3Archive*>& archives){
    OriginalFigureImageSelection out;out.status=OriginalFigureImageStatus::ExternalState;
    if(f.type!=1||f.runtimeClass!=OriginalFigureClass::Figure||f.alive!=1||f.updateOwner||
       f.effectKind!=13||f.effectVariant!=0||f.effectFlags!=0x24)return out;
    const auto group=originalFigureImageGroup(0x1c0f,archives);
    if(!group||!group->second->animationSpriteCount){out.status=OriginalFigureImageStatus::MissingGroup;return out;}
    f.frame=uint8_t(f.frame+1);
    if(unsigned(f.frame)>=3u*group->second->animationSpriteCount)f.frame=0;
    return figureImage(f,archives);
}
// Carried goods are independently selected by0x4CB6B0→0x4CB320, then
// direction is added by0x4CB910 or0x4D5093. No saved cargo word is available.
inline OriginalFigureImageSelection figureSecondaryImage(const OriginalFigureImageInputs& f,
    const std::map<uint32_t,const SG3Archive*>& archives,unsigned view=0,
    const OriginalFigureImageState* state=nullptr){
    OriginalFigureImageSelection out;out.status=OriginalFigureImageStatus::Selected;
    const auto refuse=[&](OriginalFigureImageStatus status){out.status=status;return out;};
    const OriginalFigureImageInputs* goods=&f;
    if(f.runtimeClass!=OriginalFigureClass::Figure)return out;
    if(f.type==54){ //5E6739 copies primary counter before a second5CFE60(speed100).
        //The secondary advances are supplied by the update,not reconstructed
        //from a saved image. Its phase is carried as attackFrame.
        auto secondary=f;secondary.frame=f.attackFrame;
        if(!state||!state->figures.count(size_t(f.owner))||state->figures.at(size_t(f.owner)).type!=40)return refuse(OriginalFigureImageStatus::ExternalState);
        const auto group=originalFigureImageGroup(0x1c6c,archives);
        if(!group||group->second->animationDirections!=32||!group->second->animationSpriteCount||f.direction>=32||view>6||(view&1))return refuse(OriginalFigureImageStatus::MissingGroup);
        const unsigned direction=(unsigned(f.direction)+36-4*view)%32,frame=std::min(unsigned(secondary.frame),unsigned(group->second->animationSpriteCount)-1);
        const uint64_t word=uint64_t(group->first)+frame*32+direction;
        if((word>>14)!=(group->first>>14)||(word&0x3fff)>=archives.at(14)->sprites.size())return refuse(OriginalFigureImageStatus::InvalidInput);
        out.code=0x1c6c;out.image=uint32_t(word);out.status=OriginalFigureImageStatus::Selected;return out;}
    if(f.type!=19&&f.type!=20&&f.type!=21&&f.type!=22)return out;
    out.selectorVA=0x4cb6b0;
    if(f.action==4)return out;
    if(f.type==20){
        if(!state)return refuse(OriginalFigureImageStatus::OwnerState);
        int slot=f.leaderLink;bool direct=true;goods=nullptr;
        for(unsigned n=0;n<8;++n){
            const auto found=state->figures.find(size_t(slot));
            if(slot<=0||found==state->figures.end())break;
            const auto& candidate=found->second;
            if(candidate.type==19||candidate.type==21||candidate.type==22){goods=&candidate;break;}
            direct=false;slot=candidate.leaderLink;
        }
        if(!goods||goods->alive!=1)return refuse(OriginalFigureImageStatus::OwnerState);
        if(direct)return out; //0x4D513F/5188 clears secondary on direct carts.
    }
    const unsigned resource=goods->subtype?goods->subtype:goods->resource;
    const bool special=resource==10||resource==18||resource==20||resource==11||resource==14||resource==15||
        resource==16||resource==17||resource==21||resource==23||resource==25||resource==26||resource==27;
    if(!goods->amount||(goods==&f&&special))return out; //0x4CB7A0..CF
    if(goods->amount<0||goods->subtype<1||goods->subtype>27)return refuse(OriginalFigureImageStatus::InvalidInput);
    unsigned style=0; //0x4CB723..79 compares cart+87 to saved height+86.
    if(f.state85!=0&&f.state85!=100&&f.state85!=101)
        style=f.cartStyle<f.height?2:f.cartStyle>f.height?1:0;
    const uint32_t code=(style==0?0x4c71:style==1?0x4e12:0x4e3b)+goods->subtype;
    int direction=f.direction;if(direction>=8||(goods==&f&&special&&f.stationary))direction=f.previousDirection;
    if(f.type==20&&direction>=8)direction=0;
    if(view>6||(view&1)||direction<0||direction>15)return refuse(OriginalFigureImageStatus::InvalidInput);
    direction-=int(view);if(direction<0)direction+=8;
    const auto group=originalFigureImageGroup(code,archives);out.code=code;
    if(!group)return refuse(OriginalFigureImageStatus::MissingGroup);
    const unsigned band=goods->amount<=200?0:goods->amount<=400?8:16;
    const uint32_t image=group->first+band+unsigned(direction);
    if(image>>14!=group->first>>14||(image&0x3fff)>=archives.at(code>>9)->sprites.size())
        return refuse(OriginalFigureImageStatus::InvalidInput);
    out.base=group->first;out.image=image;out.direction=unsigned(direction);return out;
}
} // namespace game_lab
