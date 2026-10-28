// SPDX-License-Identifier: MIT
#pragma once
#include "original_evidence_figures.hpp"

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
inline constexpr std::array<unsigned,14> evidenceSelectedFigureTypes{{2,19,20,21,22,23,24,28,39,40,54,75,76,77}};
inline bool evidenceSelectedFigure(unsigned type){
    return std::find(evidenceSelectedFigureTypes.begin(),evidenceSelectedFigureTypes.end(),type)!=evidenceSelectedFigureTypes.end();
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
