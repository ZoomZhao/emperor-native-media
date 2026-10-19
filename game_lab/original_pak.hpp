#pragma once
#include "original_map.hpp"
#include "original_save.hpp"
#include "original_terrain.hpp"
#include "resource_root.hpp"

namespace game_lab {
enum class OriginalPakStatus { Loaded,ReadFailure,InvalidContainer,UnsupportedContainer,BudgetExceeded };
struct OriginalPakPart {
    size_t index=0;
    bool cityCandidate=false;
    OriginalMapResult map;
    std::optional<OriginalTerrainState> surfaceState;
};
struct OriginalPakResult {
    OriginalPakStatus status=OriginalPakStatus::ReadFailure;
    size_t inputBytes=0,decodedBytes=0,frames=0;
    std::vector<OriginalPakPart> parts;
};
OriginalPakResult decodeOriginalPak(const std::vector<uint8_t>&);
OriginalPakResult loadOriginalPak(const std::string& absolutePath);
std::string describeOriginalPak(const OriginalPakResult&);

enum class OriginalMapSourceKind { MapFile,PakSummary,PakCity,SaveFile };
struct OriginalMapSource {
    OriginalMapSourceKind kind=OriginalMapSourceKind::MapFile;
    std::string relativePath,label;
    size_t partIndex=0;
};
struct OriginalMapCatalog {
    std::vector<OriginalMapSource> sources;
    size_t looseMaps=0,saveFiles=0,pakFiles=0,completePaks=0,pakParts=0,pakDecodedBytes=0,cityCandidates=0,cityMaps=0;
    bool complete=false;
    std::string error;
};
struct OriginalMapSelection {
    OriginalMapResult map;
    std::optional<OriginalTerrainState> surfaceState;
    std::optional<OriginalSaveCellLayers> saveLayers;
    std::optional<OriginalBuildingProfile> saveBuildings;
    std::string sourceInfo;
    bool packageSummary=false;
};
// Root-resolved, bounded source identities only. No asset export, mission names,
// implicit root fallback or SG3/gameplay selector is introduced.
OriginalMapCatalog enumerateOriginalMapSources(const ResourceRoot&);
OriginalMapSelection loadOriginalMapSource(const ResourceRoot&,const OriginalMapSource&);
std::vector<size_t> originalMapSourceMatches(const std::vector<OriginalMapSource>&,const std::string& query);
}
