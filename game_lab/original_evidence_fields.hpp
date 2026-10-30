// SPDX-License-Identifier: MIT
#pragma once
#include "original_evidence_figures.hpp"
#include "original_evidence_route.hpp"

namespace game_lab {
// Observational SAV19 fields already read by originalBuildingSavedUpdateContext
// and original_scene_timeline. Anchors come from decodeOriginalSave; no building
// (real or synthetic), update step, RNG call or guessed calendar is required.
struct EvidenceClock {
    uint32_t phase=0,cycle=0,monthPart=0,month=0;
    bool rangesValid()const{return phase<=50&&monthPart<=15&&month<=11;}
};
inline std::optional<EvidenceClock> evidenceSavedClock(const std::vector<uint8_t>& data,size_t terrain,size_t side){
    constexpr size_t end=original_save19::c82ef4+4;
    if(data.size()>8u*1024u*1024u||data.size()<2||detail::u16(data.data())!=19||
        terrain>data.size()||end>data.size()-terrain||side<176||side>data.size())return std::nullopt;
    return EvidenceClock{detail::u32(data.data()+terrain+original_save19::c82ef8),
        detail::u32(data.data()+terrain+original_save19::c82ef4),
        detail::u32(data.data()+terrain+original_save19::c82ef0),detail::u32(data.data()+side-176)};
}
inline constexpr std::array<unsigned,15> evidenceSelectedFigureTypes{{2,19,20,21,22,23,24,25,28,39,40,54,75,76,77}};
inline bool evidenceSelectedFigure(unsigned type){
    return std::find(evidenceSelectedFigureTypes.begin(),evidenceSelectedFigureTypes.end(),type)!=evidenceSelectedFigureTypes.end();
}
// Numeric/reference observations only. These are Figure7 receiver offsets,
// never unframed file offsets or inferred cargo/quality fields.
inline constexpr const char* evidenceMarketLinkContract="figure24-25-reference-v1";
inline std::optional<std::map<std::string,int64_t>> evidenceMarketFields(const EvidenceFigure& f){
    const auto type=f.field(18);if(!type||(*type!=24&&*type!=25))return std::nullopt;
    struct Field{const char* name;unsigned offset;bool signedWord;};
    constexpr Field fields[]={{"receiver_5_u8",5,false},
        {"receiver_8_u32",8,false},
        {"receiver_12_u32",12,false},
        {"receiver_19_u8",19,false},
        {"receiver_22_u8",22,false},
        {"receiver_25_wire_u16",25,false},
        {"receiver_26_wire_u16",26,false},
        {"receiver_27_wire_u16",27,false},
        {"receiver_28_s16",28,true},
        {"receiver_30_s16",30,true},
        {"receiver_44_s16",44,true},
        {"receiver_46_s16",46,true},
        {"receiver_48_u32",48,false},
        {"receiver_52_s16",52,true},
        {"receiver_54_s16",54,true},
        {"receiver_64_u8",64,false},
        {"receiver_65_wire_u16",65,false},
        {"receiver_66_s16",66,true},
        {"receiver_68_s16",68,true},
        {"receiver_70_s16",70,true},
        {"receiver_97_u8",97,false},
        {"receiver_98_s16",98,true},
        {"receiver_100_s16",100,true},
        {"receiver_102_s16",102,true},
        {"receiver_104_s16",104,true},
        {"receiver_114_s16",114,true},
        {"receiver_128_u8",128,false},
        {"receiver_130_s16",130,true},
        {"receiver_136_u8",136,false},
        {"receiver_144_s16",144,true},
        {"receiver_148_u16",148,false},
        {"receiver_161_u8",161,false},
        {"receiver_200_u32",200,false},
        {"receiver_228_u32",228,false},
        {"receiver_276_u32",276,false},
        {"receiver_304_s16",304,true},
        {"receiver_368_u8",368,false},
        {"receiver_400_u32",400,false},
        {"receiver_404_u32",404,false}};
    std::map<std::string,int64_t> out;
    for(const auto& field:fields){const auto v=f.field(field.offset);if(!v)return std::nullopt;
        out.emplace(field.name,field.signedWord&&*v>32767?int64_t(*v)-65536:int64_t(*v));}
    return out;
}
struct EvidenceMarketBuilding {
    uint16_t type=0;uint8_t alive=0;
    std::optional<size_t> parentSlot;std::optional<int16_t> stallWord30;
};
struct EvidenceMarketLinkCheck {
    std::optional<bool> valid;
    const char* state="market_reference_unavailable";
    std::optional<size_t> buyer,companion,owner,parent,delivery;
};
// Qualify represented live references only. Unknown transient/FSA states are
// not corrupt saves and never acquire movement or gameplay admission here.
inline EvidenceMarketLinkCheck evidenceMarketLinks(const EvidenceFigure& f,const EvidenceFigurePrefix& prefix,
    const std::map<size_t,EvidenceMarketBuilding>* buildings,const OriginalFigureSavedServices* services){
    EvidenceMarketLinkCheck out;
    const auto fail=[&](const char* why){auto r=out;r.valid=false;r.state=why;return r;};
    const auto unknown=[&](const char* why){auto r=out;r.state=why;return r;};
    if(!prefix.complete||prefix.declared!=2000||prefix.completed!=2000||prefix.records.size()!=2000)
        return unknown("complete_base_pool_required");
    if(!f.slot||f.slot>=2000||prefix.records[f.slot].slot!=f.slot||!f.recordComplete||f.alias)
        return fail("market_actor_record_required");
    const auto type=f.field(18),alive=f.field(22);
    if(!type||!alive||(*type!=24&&*type!=25))return fail("market_actor_type_required");
    if(*alive!=1||f.runtimeClass!=OriginalFigureClass::Figure)return unknown("market_actor_live_Figure_required");
    const auto index=[](const EvidenceFigure& row,unsigned offset)->std::optional<size_t>{
        const auto v=row.field(offset);if(!v||*v>32767)return std::nullopt;return size_t(*v);};
    const EvidenceFigure* buyer=&f;const EvidenceFigure* companion=nullptr;
    if(*type==25){const auto leader=index(f,114);if(!leader||!*leader||*leader>=2000)return fail("companion_leader_reference");
        buyer=&prefix.records[*leader];out.buyer=*leader;out.companion=f.slot;
        if(buyer->slot!=*leader||!buyer->recordComplete||buyer->alias||buyer->field(18)!=24)return fail("companion_leader_type24");
        if(buyer->field(22)!=1||buyer->runtimeClass!=OriginalFigureClass::Figure)return unknown("buyer_live_Figure_required");
        companion=&f;
    }else out.buyer=f.slot;
    const auto follower=index(*buyer,102);if(!follower||*follower>=2000)return fail("buyer_follower_reference");
    if(*follower){const auto& c=prefix.records[*follower];out.companion=*follower;
        if(c.slot!=*follower||!c.recordComplete||c.alias||c.field(18)!=25)return fail("buyer_follower_type25");
        if(c.field(22)!=1||c.runtimeClass!=OriginalFigureClass::Figure)return unknown("companion_live_Figure_required");
        if(index(c,114)!=out.buyer||(*type==25&&*follower!=f.slot))return fail("leader_follower_not_reciprocal");
        companion=&c;
    }else if(companion)return fail("leader_follower_not_reciprocal");
    for(const auto& r:prefix.records)if(r.field(18)==25&&r.field(22).value_or(0)&&index(r,114)==out.buyer)
        if(!companion||r.slot!=companion->slot)return fail("reverse_companion_not_reciprocal");
    const auto owner=index(*buyer,98),delivery=index(*buyer,104);
    if(!owner||!*owner||*owner>=4000||!delivery||!*delivery||*delivery>=4000)return fail("buyer_building_references");
    out.owner=*owner;out.delivery=*delivery;
    if(companion&&index(*companion,98)!=owner)return fail("pair_owner_mismatch");
    if(!buildings)return unknown("market_building_records_required");
    const auto stall=buildings->find(*owner),source=buildings->find(*delivery);
    if(stall==buildings->end()||(stall->second.type!=66&&stall->second.type!=67))return fail("buyer_owner_stall66_or67");
    if(stall->second.alive!=1)return unknown("owner_stall_not_live1");
    const auto parent=stall->second.parentSlot;
    if(!parent||!*parent||*parent>=4000)return fail("stall_parent_reference");out.parent=*parent;
    const auto market=buildings->find(*parent);
    if(market==buildings->end()||market->second.type!=59)return fail("stall_parent_type59");
    if(market->second.alive!=1)return unknown("parent_market_not_live1");
    if(!stall->second.stallWord30)return unknown("stall_word30_required");
    if(*stall->second.stallWord30!=int(*out.buyer))return fail("stall_word30_buyer_mismatch");
    if(source==buildings->end()||(source->second.type!=53&&source->second.type!=54&&source->second.type!=56&&source->second.type!=58))
        return fail("buyer_delivery_storage_type");
    if(source->second.alive!=1)return unknown("delivery_storage_not_live1");
    const auto word=[](uint32_t n){return int16_t(n>32767?int(n)-65536:int(n));};
    for(const EvidenceFigure* r:{buyer,companion}){if(!r)continue;
        const auto route=r->field(66),cursor=r->field(68),length=r->field(70),action=r->field(64);
        if(!route||!cursor||!length||!action)return unknown("market_route_fields_required");
        OriginalFigure q;q.slot=r->slot;q.type=uint8_t(*r->field(18));q.action=uint8_t(*action);
        q.route=word(*route);q.routeCursor=word(*cursor);q.routeLength=word(*length);
        const auto check=evidenceRouteCheck(q,services);
        if(check.referenceValid==false)return fail("market_route_reference_invalid");
        if(check.referenceValid!=true)return unknown("market_route_reference_unknown");
    }
    out.valid=true;out.state=companion?"reciprocal_pair":"no_companion_reference";return out;
}

inline bool evidenceHuntFigure(unsigned type){return type==40||type==54||(type>=75&&type<=77);}
// Named observations mirror originalFigurePool's existing Figure7 receiver
// reads. A known base record never establishes variable-FSA/business semantics.
inline std::optional<std::map<std::string,int64_t>> evidenceHuntFields(const EvidenceFigure& f){
    const auto type=f.field(18);if(!type||!evidenceHuntFigure(*type))return std::nullopt;
    struct Field{const char* name;unsigned offset;bool signedWord;};
    constexpr Field fields[]={{"counter_s16",62,true},{"destination_x_s16",44,true},{"destination_y_s16",46,true},
        {"home_x_s16",52,true},{"home_y_s16",54,true},{"hidden_wire_u8",111,false},
        {"previous_action_wire_u8",24,false},{"free_movement_wire_u8",20,false},{"movement_mode_wire_u8",128,false},
        {"linked_building_wire_u32",328,false},{"flags120_wire_u32",288,false},{"damage_wire_u32",168,false},
        {"projectile_wire_u8",297,false},{"projectile_hits_wire_u8",160,false}};
    std::map<std::string,int64_t> out;
    for(const auto& field:fields){const auto v=f.field(field.offset);if(!v)return std::nullopt;
        out.emplace(field.name,field.signedWord&&*v>32767?int64_t(*v)-65536:int64_t(*v));}
    return out;
}
// Common5 wire101 is both the producer secondary stock and the Hunter return
// accumulator. The existing readers must agree, including signed values; this
// is not a resource conversion, legal-capacity or gameplay-admission test.
inline std::optional<bool> evidenceHunterProductionCrosscheck(const OriginalFigureBuildingLink& b,
    const OriginalBuildingProductionInputs* p){
    if(b.type!=33||!p)return std::nullopt;
    return b.hunterReturnGoods==p->secondaryStock&&b.industryInput==p->primaryStock&&
        b.workers==p->workers&&b.serviceCount==p->operational&&
        b.storageLaborDisabled==p->laborDisabled&&b.destinationEnabled==p->enabled&&
        b.delegatedCount&&*b.delegatedCount==p->industry[0x5c-4];
}
} // namespace game_lab
