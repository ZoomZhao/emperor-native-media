#pragma once
#include "original_map.hpp"
#include "original_building.hpp"
namespace game_lab {
enum class OriginalSaveStatus { LoadedSnapshot,ReadFailure,ChangedDuringRead,InvalidContainer,UnsupportedRole,UnsupportedAnchors,InvalidGrid,BudgetExceeded };
struct OriginalSaveCellLayers {
    static constexpr size_t cells=228u*228u;
    std::vector<uint32_t> followingU32Unknown,mapperObjects;
    std::vector<uint8_t> mapperRandom,mapperStone,mapperFertile;
    bool valid() const{return followingU32Unknown.size()==cells&&mapperObjects.size()==cells&&mapperRandom.size()==cells&&mapperStone.size()==cells&&mapperFertile.size()==cells;}
};
struct OriginalSaveResult {
    uint64_t snapshotRevision=0;
    OriginalSaveStatus status=OriginalSaveStatus::ReadFailure;
    OriginalMapTerrain terrain;
    OriginalSaveCellLayers layers;
    OriginalBuildingProfile buildings;
    size_t inputBytes=0,decodedBytes=0,frames=0,sizeOffset=0,terrainOffset=0;
    bool readStable=false;
};
// Conservative SAV13 data profile plus the bounded published building table.
// Image binding is a separate scene step; no walker, clock or city rules mutate.
OriginalSaveResult decodeOriginalSave(const std::vector<uint8_t>&);
OriginalSaveResult loadOriginalSave(const std::string& absolutePath);
std::string describeOriginalSave(const OriginalSaveResult&);
// Active-square zero-based coordinates; off-map cells retain their raw values.
struct OriginalSaveCell {
    uint32_t storageX=0,storageY=0;
    bool onMap=false;
    std::array<uint32_t,6> values{}; // terrain, following, objects, random, fertile, stone
};
std::optional<OriginalSaveCell> originalSaveCell(const OriginalMapTerrain&,
    const OriginalSaveCellLayers&,int64_t x,int64_t y);
std::string describeOriginalSaveCell(const OriginalSaveCell&);
// field0=unknown following u32;1=objects;2=random;3=fertile;4=stone.
std::vector<uint32_t> originalSaveFieldValues(const OriginalSaveCellLayers&,unsigned field);
std::string originalSaveFieldLabel(unsigned field);
}
