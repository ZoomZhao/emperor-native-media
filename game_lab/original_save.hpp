#pragma once
#include "original_map.hpp"
#include "original_building.hpp"
#include "original_terrain.hpp"
namespace game_lab {
enum class OriginalSaveStatus { LoadedSnapshot,ReadFailure,ChangedDuringRead,InvalidContainer,UnsupportedRole,UnsupportedAnchors,InvalidGrid,BudgetExceeded };
struct OriginalSavedGround {
    OriginalTerrainState state; // SAV19 restored inputs; image is cleared, never persisted
    std::vector<uint8_t> material; // signed 0xF11C70 bytes, T+467856, read0x53137C
    std::vector<uint16_t> owner; // 0xFC3750, T-103968, read0x531316
    uint32_t activeSide=0,cropStorageBase=0,savedView=0; //0x115F720,T+1291324,0x531419
    std::optional<uint8_t> waterMode; // saved singleton+8, serializer0x4EDDA0/read0x4EDE43
    bool valid() const {return state.valid()&&material.size()==state.cells&&owner.size()==state.cells&&
        activeSide>=1&&activeSide<=228&&cropStorageBase==((228-activeSide)/2)*229&&savedView==0&&waterMode&&*waterMode==0;}
};
struct OriginalSaveCellLayers {
    static constexpr size_t cells=228u*228u;
    std::vector<uint32_t> followingU32Unknown,mapperObjects;
    std::vector<uint8_t> mapperRandom,mapperStone,mapperFertile;
    std::optional<OriginalSavedGround> ground;
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
