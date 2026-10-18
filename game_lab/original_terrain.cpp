#include "original_terrain.hpp"
#include "original_map.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <array>
#include <new>
namespace game_lab {
namespace {
constexpr size_t maxInput=4u*1024u*1024u,maxDecoded=8u*1024u*1024u;
constexpr std::array<uint8_t,8> citySignature{{5,0,0xfe,0xca,0,0,2,0}};
// The read-order offsets must agree with the layer offsets the container parser
// already validated on the local corpus, so a drift in either place fails the build.
static_assert(OriginalTerrainState::drawOffset==OriginalMapCellLayers::edgesOffset,
    "draw (0x00FDCD70) is the container edges layer");
static_assert(OriginalTerrainState::terrainOffset==261455,
    "terrain offset is fixed by the loader read order");
static_assert(OriginalTerrainState::featureOffset==OriginalMapCellLayers::objectsOffset,
    "feature (0x00F37DA0) is the container objects layer");
static_assert(OriginalTerrainState::randomByteOffset==OriginalMapCellLayers::mapperRandomOffset,
    "randomByte (0x00F1E780) is the container mapperRandom layer");
static_assert(OriginalTerrainState::fertilityOffset==OriginalMapCellLayers::mapperFertileOffset,
    "fertility (0x00E9F8E0) is the container mapperFertile layer");
}
bool OriginalTerrainState::valid() const {
    return image.size()==cells&&draw.size()==cells&&terrain.size()==cells&&
        feature.size()==cells&&subTile.size()==cells&&randomByte.size()==cells&&
        gF05160.size()==cells&&gEAC3F0.size()==cells&&roadDistance.size()==cells&&
        fertility.size()==cells&&gBEBF3A.size()==cells&&gBF8A4A.size()==cells;
}
std::optional<OriginalTerrainState> parseOriginalTerrainState(const std::vector<uint8_t>& payload) {
    if(payload.size()<OriginalTerrainState::flatEnd||payload.size()>maxDecoded||
       !std::equal(citySignature.begin(),citySignature.end(),payload.begin()))return std::nullopt;
    try {
        OriginalTerrainState state;
        auto wordsAt=[&](size_t offset,std::vector<uint32_t>& target) {
            target.resize(OriginalTerrainState::cells);
            for(size_t i=0;i<OriginalTerrainState::cells;++i)
                target[i]=detail::u32(payload.data()+offset+4*i);
        };
        auto bytesAt=[&](size_t offset,std::vector<uint8_t>& target) {
            target.assign(payload.begin()+offset,payload.begin()+offset+OriginalTerrainState::cells);
        };
        wordsAt(OriginalTerrainState::imageOffset,state.image);
        bytesAt(OriginalTerrainState::drawOffset,state.draw);
        wordsAt(OriginalTerrainState::terrainOffset,state.terrain);
        wordsAt(OriginalTerrainState::featureOffset,state.feature);
        bytesAt(OriginalTerrainState::subTileOffset,state.subTile);
        bytesAt(OriginalTerrainState::randomByteOffset,state.randomByte);
        bytesAt(OriginalTerrainState::gF05160Offset,state.gF05160);
        state.g101D0AC=detail::u32(payload.data()+OriginalTerrainState::g101D0ACOffset);
        state.g101D0B0=detail::u32(payload.data()+OriginalTerrainState::g101D0B0Offset);
        bytesAt(OriginalTerrainState::gEAC3F0Offset,state.gEAC3F0);
        bytesAt(OriginalTerrainState::roadDistanceOffset,state.roadDistance);
        bytesAt(OriginalTerrainState::fertilityOffset,state.fertility);
        std::copy(payload.begin()+OriginalTerrainState::blob24Offset,
            payload.begin()+OriginalTerrainState::blob24Offset+OriginalTerrainState::blob24Size,
            state.blob24.begin());
        bytesAt(OriginalTerrainState::gBEBF3AOffset,state.gBEBF3A);
        bytesAt(OriginalTerrainState::gBF8A4AOffset,state.gBF8A4A);
        state.g10C6F18=detail::u32(payload.data()+OriginalTerrainState::g10C6F18Offset);
        std::copy(payload.begin()+OriginalTerrainState::g10C6DB0Offset,
            payload.begin()+OriginalTerrainState::g10C6DB0Offset+OriginalTerrainState::g10C6DB0Size,
            state.g10C6DB0.begin());
        return state;
    }catch(const std::bad_alloc&){return std::nullopt;}
}
std::optional<OriginalTerrainState> loadOriginalTerrainState(const std::string& path) {
    std::vector<uint8_t> input;
    if(!detail::readFile(path,maxInput,input))return std::nullopt;
    const auto container=decodeOriginalMapContainer(input);
    if(!container.complete)return std::nullopt;
    return parseOriginalTerrainState(container.payload);
}
} // namespace game_lab
