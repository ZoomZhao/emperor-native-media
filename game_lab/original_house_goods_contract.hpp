// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace game_lab {
// Explicit project-format contract, not new independent original-game evidence.
// originalBuildingUpdateInputs copies framed House1 tail[4..64] verbatim.
// originalFigureBuildingLinks reads goods[n] at framed tail[17+2*n]; the
// MarketHouse consumer treats goods[0] as food. Thus its PAYLOAD offset is 13,
// not 14. Keep the framing offset separate from an in-memory runtime address.
inline constexpr size_t originalHousePayloadBytes=61;
inline constexpr size_t originalHousePayloadWireOffset=4;
inline constexpr size_t originalHouseGoodsWireOffset=17;
inline constexpr size_t originalHouseGoodsCount=12;
inline constexpr size_t originalHouseGoodsPayloadOffset=
    originalHouseGoodsWireOffset-originalHousePayloadWireOffset;
using OriginalHousePayload=std::array<uint8_t,originalHousePayloadBytes>;
using OriginalHouseGoods=std::array<int16_t,originalHouseGoodsCount>;

inline std::optional<size_t> originalHouseGoodsOffset(size_t index){
    if(index>=originalHouseGoodsCount)return std::nullopt;
    return originalHouseGoodsPayloadOffset+2*index;
}
inline std::optional<int16_t> originalHouseGoodsWord(const uint8_t* payload,size_t bytes,size_t index){
    const auto offset=originalHouseGoodsOffset(index);
    if(!payload||bytes!=originalHousePayloadBytes||!offset)return std::nullopt;
    const uint16_t bits=uint16_t(payload[*offset])|uint16_t(uint16_t(payload[*offset+1])<<8);
    return int16_t(bits<=0x7fff?int32_t(bits):int32_t(bits)-0x10000);
}
inline bool originalHouseSetGoodsWord(uint8_t* payload,size_t bytes,size_t index,int32_t value){
    const auto offset=originalHouseGoodsOffset(index);
    if(!payload||bytes!=originalHousePayloadBytes||!offset||value< -32768||value>32767)return false;
    const uint16_t bits=uint16_t(value);payload[*offset]=uint8_t(bits);payload[*offset+1]=uint8_t(bits>>8);return true;
}
inline std::optional<OriginalHouseGoods> originalHouseGoodsFromPayload(const uint8_t* payload,size_t bytes){
    if(!payload||bytes!=originalHousePayloadBytes)return std::nullopt;
    OriginalHouseGoods goods{};
    for(size_t n=0;n<goods.size();++n)goods[n]=*originalHouseGoodsWord(payload,bytes,n);
    return goods;
}
inline bool originalHouseGoodsToPayload(uint8_t* payload,size_t bytes,const OriginalHouseGoods& goods){
    if(!payload||bytes!=originalHousePayloadBytes)return false;
    for(size_t n=0;n<goods.size();++n)originalHouseSetGoodsWord(payload,bytes,n,goods[n]);
    return true;
}
inline std::optional<OriginalHouseGoods> originalHouseGoodsFromPayload(const OriginalHousePayload& payload){
    return originalHouseGoodsFromPayload(payload.data(),payload.size());
}
inline bool originalHouseGoodsToPayload(OriginalHousePayload& payload,const OriginalHouseGoods& goods){
    return originalHouseGoodsToPayload(payload.data(),payload.size(),goods);
}
// This bounded prefix is only the already-supported outer1/data1 House1
// framing. It neither parses a Common5 record nor accepts another schema.
inline std::optional<OriginalHouseGoods> originalHouseGoodsFromWire(const uint8_t* wire,size_t bytes){
    if(!wire||bytes<originalHousePayloadWireOffset+originalHousePayloadBytes||
       wire[0]!=1||wire[1]!=0||wire[2]!=1||wire[3]!=0)return std::nullopt;
    return originalHouseGoodsFromPayload(wire+originalHousePayloadWireOffset,originalHousePayloadBytes);
}
} // namespace game_lab
