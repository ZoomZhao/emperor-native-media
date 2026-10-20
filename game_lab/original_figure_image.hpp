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
};
inline OriginalFigureImageInputs originalFigureImageInputs(const OriginalFigure& f){
    return {f.type,f.action,f.direction,f.previousDirection,f.frame,f.subtype,f.attackDirection,
        f.attackFrame,f.state85,f.cartStyle,f.resource,f.stationary,f.counter,f.variant,f.amount,f.runtimeClass};
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
// Reconstruct the SELECTED saved phase, after the original updater's counter
// changes. This does not execute the updater or infer elapsed ticks.
inline OriginalFigureImageSelection figureImage(const OriginalFigureImageInputs& f,
    const std::map<uint32_t,const SG3Archive*>& archives,uint32_t terrainFlags=0,unsigned view=0,
    std::optional<uint16_t> ownerType=std::nullopt){
    OriginalFigureImageSelection out;
    const auto refuse=[&](OriginalFigureImageStatus status){out.status=status;return out;};
    if(view>6||(view&1)||f.direction>127||f.previousDirection>127)
        return refuse(OriginalFigureImageStatus::InvalidInput);
    if(f.runtimeClass==OriginalFigureClass::Pheasant&&f.type!=76)return refuse(OriginalFigureImageStatus::ExternalState);
    int direction=f.direction;unsigned frame=f.frame;uint32_t code=0;bool clamp=false,linear=false;
    if(f.runtimeClass==OriginalFigureClass::Emissary){ //vtable7ACD68+28=488E70
        out.selectorVA=0x488e70;code=0x4c17;
        if(f.action==4){code=0x4c18;frame=f.counter;linear=true;clamp=true;}
    }else switch(f.type){
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
        if(f.action==7)return refuse(OriginalFigureImageStatus::ExternalState); // local attack eligibility bVar4
        code=f.action==4?0x4ec3:f.action==5||f.action==16||f.action==13?0x4ec0:
            f.action==8?0x4ec1:f.action==10?0x4ec2:0x4ebe;
        if(f.action==8){frame=f.counter;direction=0;linear=true;}
        else if(f.action==12)frame=0;
        else if(f.action==4){frame=f.counter/2;direction=0;clamp=true;}
        else clamp=true;break;
    case 76:{
        if(f.runtimeClass!=OriginalFigureClass::Pheasant)return refuse(OriginalFigureImageStatus::ExternalState);
        out.selectorVA=0x59f310;if(direction>=8)direction=f.previousDirection;
        // Pheasant row1:0x85CF60 (type pointer0x85CF18 ->0x85CD58).
        if(f.action==4||f.action==10){code=0x4c2d;frame=std::min(int(f.counter),7);linear=true;direction=0;}
        else {const bool entry0=f.action==5||f.action==33||f.action==34;
            code=entry0||f.action==31?0x4c2c:0x4c2b;
            if(!entry0&&(terrainFlags&0x0c000100u))code=0x4c2b;clamp=true;}break;
    }
    case 20:case 30:case 34:case 35:case 36:return refuse(OriginalFigureImageStatus::OwnerState);
    case 45:case 47:return refuse(OriginalFigureImageStatus::ExternalState);
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
} // namespace game_lab
