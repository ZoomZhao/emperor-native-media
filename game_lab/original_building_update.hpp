#pragma once
#include "original_building.hpp"

namespace game_lab {
// All executable facts: SHA256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
// docs/SAV.md#building-update-pass records call order, read-order and limits.
enum class OriginalBuildingUpdateScope {AdmittedTicks,ProductionPasses,HouseServiceDecayPasses};
enum class OriginalBuildingUpdateBlocker {None,InvalidState,MissingContext,ExternalWrites,
    ProductionVirtuals,RoadRefresh,Employment,HousingEvolution,BuildingVirtual20,
    RiskAndFire,MonthlyCity,Availability,CityPhase,UnclosedPhase};
inline const char* originalBuildingUpdateBlockerName(OriginalBuildingUpdateBlocker b){
    switch(b){case OriginalBuildingUpdateBlocker::None:return "computed";
    case OriginalBuildingUpdateBlocker::InvalidState:return "invalid_saved_building_update_inputs";
    case OriginalBuildingUpdateBlocker::MissingContext:return "explicit_phase_terrain_disabled_context_required";
    case OriginalBuildingUpdateBlocker::ExternalWrites:return "intervening_figure_city_writes_not_supplied";
    case OriginalBuildingUpdateBlocker::ProductionVirtuals:return "production_virtuals_51e4a0_74_208";
    case OriginalBuildingUpdateBlocker::RoadRefresh:return "road_refresh_4acfc0_426df0_1391fe0";
    case OriginalBuildingUpdateBlocker::Employment:return "employment_4f19a0_51e310";
    case OriginalBuildingUpdateBlocker::HousingEvolution:return "house_evolution_519120_518ef0";
    case OriginalBuildingUpdateBlocker::BuildingVirtual20:return "building_virtual20_416d20";
    case OriginalBuildingUpdateBlocker::RiskAndFire:return "risk_fire_4af5d0_42a220";
    case OriginalBuildingUpdateBlocker::MonthlyCity:return "monthly_city_4ac650";
    case OriginalBuildingUpdateBlocker::Availability:return "availability_4ace60_virtual1a0";
    case OriginalBuildingUpdateBlocker::CityPhase:return "city_accounting_4f1560_4ac2fa";
    case OriginalBuildingUpdateBlocker::UnclosedPhase:return "scheduled_phase_callee_not_closed";}
    return "invalid_saved_building_update_inputs";
}
struct OriginalBuildingUpdateState {
    OriginalBuildingFootprint building;
    OriginalBuildingUpdateInputs inputs;
};
struct OriginalBuildingUpdateContext {
    // Explicit operation count: admitted ticks are distinct from phase20 calls.
    OriginalBuildingUpdateScope scope;
    // Required isolation premise: normal live-mode city scheduler is entered;
    // scenario/event early return is false; no intervening figure/service/city writes.
    // Retained fields and existing selectors only; this is not a full opaque
    // runtime object snapshot (unserialized scratch+92 is outside this API).
    // This is a building-only experiment, never a complete future city state.
    bool isolateBuildingWrites;
    std::optional<uint32_t> phase,monthPart,cycle,month;
    std::optional<uint32_t> terrainFlags;
    std::optional<bool> typeDisabled;
    OriginalBuildingUpdateContext(OriginalBuildingUpdateScope operation,bool isolated)
        :scope(operation),isolateBuildingWrites(isolated){}
};
// SAV19 531427/435/443 => T+1299328/+1299332/+1299336.
// Month C82EEC is5312AF => S-176. No runtime clock/CRT value is inferred.
inline std::optional<OriginalBuildingUpdateContext> originalBuildingSavedUpdateContext(
    const std::vector<uint8_t>& data,size_t terrain,size_t side,
    const OriginalBuildingFootprint& building,OriginalBuildingUpdateScope scope,bool isolated){
    // Continuation of531816 through53191A; the269 bytes are53192B.
    constexpr size_t disabled=1563037+207936+26+13+2*51984+4+2*51984+1436+4+224+3600+624+256+224+448+200;
    if(data.size()>8u*1024u*1024u||data.size()<2||detail::u16(data.data())!=19||
        terrain>data.size()||disabled+269>data.size()-terrain||side<176||side>data.size()||
        !building.imageInputs||building.imageInputs->gridIndex>=51984||building.type>=269)return std::nullopt;
    OriginalBuildingUpdateContext c(scope,isolated);
    c.phase=detail::u32(data.data()+terrain+original_save19::c82ef8);
    c.monthPart=detail::u32(data.data()+terrain+original_save19::c82ef0);
    c.cycle=detail::u32(data.data()+terrain+original_save19::c82ef4);
    c.month=detail::u32(data.data()+side-176);
    c.terrainFlags=detail::u32(data.data()+terrain+4*building.imageInputs->gridIndex);
    c.typeDisabled=data[terrain+disabled+building.type]!=0;
    return c;
}
struct OriginalBuildingUpdateResult {
    OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::InvalidState;
    std::optional<OriginalBuildingUpdateState> state;
    std::optional<OriginalBuildingUpdateContext> context;
    unsigned operations=0,productionCalls=0,virtual20Calls=0,serviceDecayCalls=0;
    size_t changedSelectorInputs=0;
    bool selectorChanged=false;
    std::optional<OriginalBuildingBaseSelection> beforeSelection,afterSelection;
};
inline int16_t originalBuildingSignedWord(uint16_t v){return int16_t(v>32767?int(v)-65536:int(v));}
inline uint16_t originalBuildingCropFlags(const OriginalBuildingUpdateInputs& i){
    return i.industry?detail::u16(i.industry->data()+0x6a-4):0;
}
inline int16_t originalBuildingCropProgress(const OriginalBuildingUpdateInputs& i){
    return i.industry?detail::i16(i.industry->data()):0;
}
inline void originalBuildingSetCropFlags(OriginalBuildingUpdateInputs& i,uint16_t flags){
    i.industry->at(0x6a-4)=uint8_t(flags);i.industry->at(0x6b-4)=uint8_t(flags>>8);
}
//0x4C1BC0: saturating LOW BYTE decay, preserve only flags C000.
inline void originalBuildingDecayCropFlags(OriginalBuildingUpdateInputs& i){
    const auto flags=originalBuildingCropFlags(i);const unsigned low=flags&255;
    originalBuildingSetCropFlags(i,uint16_t((flags&0xc000)|(low?low-1:0)));
}
inline bool originalBuildingIsHouse(uint16_t type){return type>=2&&type<=17;}
//0x517280, tail bytes from0x517410 (runtime data offsets+1 in wire).
inline void originalBuildingDecayHouseServices(OriginalBuildingUpdateInputs& i){
    for(unsigned offset:{41u,42u,43u,44u,45u,49u,50u,51u,13u,14u,15u,16u}){
        auto& v=i.house->at(offset-4);if(v)--v;
    }
}
//0x51E4A0 -> C8(-4)=4C11F0 ->4C0F60.0x4C2600 gives
// hemp125/63/0;0x4C2260 gives millet157/79/0 except final harvest
// month10(table0x7AE1AC). Addition wraps16 BEFORE signed10000 clamp.
inline OriginalBuildingUpdateBlocker originalBuildingProductionStep(
    OriginalBuildingUpdateState& s,const OriginalBuildingUpdateContext& c){
    auto& i=s.inputs;const auto type=s.building.type;
    //4C1A90 ->4C1A20 precedes all per-record output/terrain gates.
    if((type==194||type==196)&&i.alive==1&&c.monthPart&&*c.monthPart==0&&c.month&&*c.month==(type==194?8u:10u)){
        if(!i.industry)return OriginalBuildingUpdateBlocker::InvalidState;
        originalBuildingSetCropFlags(i,uint16_t(originalBuildingCropFlags(i)|0x8000));
    }
    if((i.alive!=1&&i.alive!=3)||!i.output)return OriginalBuildingUpdateBlocker::None;
    if(type!=194&&type!=196)return OriginalBuildingUpdateBlocker::ProductionVirtuals;
    if(!i.industry||!s.building.imageInputs)return OriginalBuildingUpdateBlocker::InvalidState;
    if(!c.terrainFlags||!c.typeDisabled||!c.month||*c.month>11)return OriginalBuildingUpdateBlocker::MissingContext;
    if(*c.terrainFlags&0x100)return OriginalBuildingUpdateBlocker::None;
    auto& delay=i.industry->at(0x63-4);
    if(delay){--delay;return OriginalBuildingUpdateBlocker::None;}
    const unsigned low=originalBuildingCropFlags(i)&255;
    const int growth=type==196&&*c.month==10?0:low>32?(type==194?125:157):low?(type==194?63:79):0;
    int16_t progress=originalBuildingCropProgress(i);
    if(growth){progress=originalBuildingSignedWord(uint16_t(uint32_t(uint16_t(progress))+unsigned(growth)));
        if(progress>10000)progress=10000;}
    if(*c.typeDisabled)progress=0;
    i.industry->at(0)=uint8_t(progress);i.industry->at(1)=uint8_t(uint16_t(progress)>>8);
    s.building.imageInputs->cropWord4=progress;
    return OriginalBuildingUpdateBlocker::None;
}
// The function publishes only the complete requested operation sequence for this
// record. Refusals have no prefix state. Scope distinguishes a verified subpass
// from the admitted city tick, which includes currently unported later phases.
inline OriginalBuildingUpdateResult originalBuildingStateAfterUpdates(
    const OriginalBuildingUpdateState& saved,const OriginalBuildingUpdateContext& context,unsigned count){
    OriginalBuildingUpdateResult out;
    if(count>10000||saved.building.slot>=4000||saved.inputs.identity!=saved.building.slot)return out;
    if(!context.isolateBuildingWrites){out.blocker=OriginalBuildingUpdateBlocker::ExternalWrites;return out;}
    if(!context.phase||*context.phase>50||!context.monthPart||*context.monthPart>15||!context.cycle||!context.month||*context.month>11){
        out.blocker=OriginalBuildingUpdateBlocker::MissingContext;return out;}
    auto s=saved;auto c=context;out.beforeSelection=originalBuildingBaseSelection(saved.building);
    const bool crop=s.building.type==194||s.building.type==196,house=originalBuildingIsHouse(s.building.type);
    if((crop&&!s.inputs.industry)||(house&&!s.inputs.house))return out;
    if(crop&&(!s.building.imageInputs||s.building.imageInputs->cropWord4!=originalBuildingCropProgress(s.inputs)))return out;
    for(unsigned n=0;n<count;++n){
        OriginalBuildingUpdateBlocker blocker=OriginalBuildingUpdateBlocker::None;
        if(c.scope==OriginalBuildingUpdateScope::ProductionPasses){blocker=originalBuildingProductionStep(s,c);++out.productionCalls;}
        else if(c.scope==OriginalBuildingUpdateScope::HouseServiceDecayPasses){
            if(house&&(s.inputs.alive==1||s.inputs.alive==3)){originalBuildingDecayHouseServices(s.inputs);++out.serviceDecayCalls;}}
        else {
            const unsigned phase=*c.phase;
            // Fail at the first missing write producer, not at an invented
            // per-type no-op. Phase21 rewrites road fields of every live type.
            if(phase==21)blocker=OriginalBuildingUpdateBlocker::RoadRefresh;
            else if(phase==25)blocker=OriginalBuildingUpdateBlocker::Employment;
            else if(phase==39&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution;
            else if(phase==6&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution; //5185C0 countdown
            else if(phase==44)blocker=OriginalBuildingUpdateBlocker::RiskAndFire;
            else if(phase==31&&(s.inputs.alive==1||s.inputs.alive==3)){
                if(crop){ //4C1040 ->4C12E0, harvest flag on first month part
                    if(*c.monthPart==0&&*c.month==(s.building.type==194?8u:10u))
                        originalBuildingSetCropFlags(s.inputs,uint16_t(originalBuildingCropFlags(s.inputs)|0x8000));
                    ++out.virtual20Calls;
                }else if(s.building.type==231)++out.virtual20Calls; //428F00 false
                else blocker=OriginalBuildingUpdateBlocker::BuildingVirtual20;
            }else if(phase==20){blocker=originalBuildingProductionStep(s,c);++out.productionCalls;}
            else if(phase==35&&house&&(s.inputs.alive==1||s.inputs.alive==3)){originalBuildingDecayHouseServices(s.inputs);++out.serviceDecayCalls;}
            // Other phases may dispatch further virtuals. They remain named
            // blockers; a complete state never crosses them silently.
            else if(phase==12)blocker=OriginalBuildingUpdateBlocker::Availability; //4ACE60 availability
            else if(phase==36&&house)blocker=OriginalBuildingUpdateBlocker::HousingEvolution;
            else if(phase==1)blocker=OriginalBuildingUpdateBlocker::CityPhase;
            else if(phase!=0&&phase!=2&&phase!=14&&phase!=15&&phase!=19&&phase!=26&&phase!=34&&
                phase!=35&&phase!=36&&phase!=39&&phase!=41&&phase!=42&&phase!=46&&phase!=48&&phase!=49&&phase!=50)
                blocker=OriginalBuildingUpdateBlocker::UnclosedPhase;
            if(blocker==OriginalBuildingUpdateBlocker::None){
                if(phase==48&&s.inputs.alive==1&&s.inputs.byte52)--s.inputs.byte52; //4AE8B0
                if(phase==50){
                    //4AC650 includes independent city work even on an ordinary
                    // month part. The closed flag-decay helper above is exposed
                    // separately; do not turn it into a complete wrap claim.
                    blocker=OriginalBuildingUpdateBlocker::MonthlyCity;
                }else c.phase=phase+1;
            }
        }
        if(blocker!=OriginalBuildingUpdateBlocker::None){out.blocker=blocker;return out;}
        ++out.operations;
    }
    out.afterSelection=originalBuildingBaseSelection(s.building);
    if(crop&&saved.building.imageInputs&&s.building.imageInputs&&saved.building.imageInputs->cropWord4!=s.building.imageInputs->cropWord4)++out.changedSelectorInputs;
    if(house&&saved.inputs.house&&s.inputs.house&&saved.inputs.house->at(55)!=s.inputs.house->at(55))++out.changedSelectorInputs;
    if(out.beforeSelection&&out.afterSelection)out.selectorChanged=out.beforeSelection->code!=out.afterSelection->code||out.beforeSelection->variant!=out.afterSelection->variant;
    out.blocker=OriginalBuildingUpdateBlocker::None;out.state=std::move(s);out.context=c;return out;
}
} // namespace game_lab
