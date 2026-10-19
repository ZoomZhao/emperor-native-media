#pragma once
#include "original_map.hpp"
#include "original_ground.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
namespace game_lab {
struct MapReferenceStats {
    size_t activeCells=0,validIDs=0,geometrySupported=0,outsideArchive=0,unknownGeometry=0,culled=0,
           visibleCells=0,drawnCells=0,decodeFailures=0,uniqueDecoded=0,cacheBytes=0;
};
struct MapReferenceResult {
    DecodedImage image;
    MapReferenceStats stats;
    bool experimental=true;
};
// Explicit caller archive/base/camera; no engine selector, original draw-order rule,
// captured coordinates, per-cell overrides, or pixels from an original screenshot.
MapReferenceResult renderMapReferenceViewport(const OriginalMapTerrain&,const OriginalMapRawLayer&,
    const SG3Archive&,const std::string& internal555,int64_t idBase,const OriginalMapCamera&);

struct MapGroundStats {
    // chainNoWriteCells: the chain ran but no setter wrote 0x00FE9880. That is
    // *not* "no ground": the grid is loaded from the `.map` payload (logical
    // offset 1535), so the cell keeps its stored art. See GroundSelection.
    size_t activeCells=0,knownCells=0,unknownCells=0,chainNoWriteCells=0,blockCells=0,unresolvedCodes=0,
           geometrySupported=0,culled=0,visibleCells=0,drawnCells=0,decodeFailures=0,
           uniqueDecoded=0,cacheBytes=0;
    std::vector<std::pair<std::string,size_t>> branchCells;
};
struct MapGroundResult { DecodedImage image; MapGroundStats stats; bool experimental=true; };
// Engine selector: the ground code per cell comes from selectGroundTileCode, not
// from a caller-supplied raw byte/base. The per-cell terrain flags, random byte,
// feature bits and fertility all come from the MAP-decoded loader state
// (OriginalTerrainState, FUN_0052E7C0 read order).
MapGroundResult renderMapGroundViewport(const OriginalMapTerrain&, const OriginalTerrainState&,
    const SG3Archive&, const std::string& internal555, const OriginalMapCamera&);

// ---- Stored surface-image grid (0x00FE9880) --------------------------------
// One tileset namespace the caller supplies for the stored grid. A stored cell
// word v has tileset = v>>14 and low14 = v&0x3FFF; the archive record is
// recordId = low14 + firstRecordId (0-based sprites index recordId-1). For a
// version-213 archive the loader registers headerValue-200 as the node index and
// publishes headerValue-201 as the entry, so the additive base is 201: with
// firstRecordId 201 every distinct referenced low14 in the 167-city corpus
// resolves to an existing record, and a 0 or 200 base does not. It is a stated
// hypothesis with a way to be wrong: every miss (unresolved/unsupported/
// undecodable) is counted, never hidden.
struct MapImageGridTileset {
    uint32_t tileset = 0;
    const SG3Archive* archive = nullptr;
    std::string internal555;      // companion .555 path for decodeSG3Sprite
    uint32_t firstRecordId = 201; // 1-based archive id that low14 counts from
};
// Tileset index -> DATA archive leaf name, from the original executable's static
// name table (FUN_00475B60 @0x00475B60). Used when a caller wants the engine's
// own archive for a stored tileset; nullptr for an index the corpus never uses or
// that does not name a ground archive. Evidence and the full 41-slot table are in
// docs/MAP_SG3_PIPELINE.md.
const char* mapImageGridArchiveName(uint32_t tileset);
// One named composite failure: a same-word component of a multi-tile (s*s) type-30
// record whose cells the per-cell offset rule could not place. Kept apart from
// geometry_unsupported only to name the reason; the cells still count
// geometry_unsupported. See docs/MAP_SG3_PIPELINE.md.
struct MapImageGridCompositeMismatch {
    uint32_t tileset=0, recordId=0;   // stored tileset (v>>14) and recordId = low14+firstRecordId
    int32_t width=0, height=0;        // resolved record pixel size (mirror target)
    uint32_t cols=0;                  // composite block side s = (width+2)/80
    uint32_t groupSide=0;             // subTile low nibble + 1 (0x00F9D620)
    uint32_t bboxWidth=0, bboxHeight=0, cells=0; // component bbox (cells) and unresolved cell count
    bool fullRect=false, divisible=false;        // full rectangle / bbox a multiple of s
    const char* reason="";                       // why the offset rule declined the cells
};
struct MapImageGridStats {
    size_t activeCells=0, zeroCells=0, nonzeroCells=0;
    // Decoding validates the complete referenced pixel payload before culling.
    // Every nonzero cell is decoded OR exactly one failure category. Culled is
    // a subset of successfully decoded cells,not a substitute for validation.
    size_t decodedCells=0, drawnCells=0, culled=0, visibleCells=0;
    size_t unresolvedIndices=0, geometryUnsupported=0, archiveUnavailable=0, decodeFailures=0;
    size_t uniqueDecoded=0, cacheBytes=0;
    // Composite (multi-tile) surface art. A cell whose resolved record declares an
    // s*s block is placed from its own per-cell offset, not from the component
    // bounding box: draw-state 0x00FDCD70 bits 0..2 are the column and bits 3..5 the
    // row inside the block, and subTile 0x00F9D620 low nibble + 1 is the group side.
    // anchor = cell - offset, so a block is placed even when overwrites break the
    // component shape. compositeBlocks counts accepted blocks; confirmed/recovered
    // split them by whether the old full-rectangle component rule also accepted the
    // cell; partial blocks name the anchoring case. Unresolved multi-tile cells stay
    // geometry_unsupported and are named in compositeMismatches. drawnBlocks is the
    // number of composite blits that were visible (camera-dependent).
    size_t compositeBlocks=0, compositeConfirmedCells=0, compositeRecoveredCells=0,
           compositePartialBlocks=0, compositePartialBlockCells=0,
           compositeMismatchedCells=0, compositeForeignCells=0, drawnBlocks=0;
    std::vector<std::pair<uint32_t,size_t>> tilesetCells; // stored tileset -> active nonzero
    std::vector<std::pair<uint32_t,size_t>> tilesetDrawn; // stored tileset -> drawn cells
    std::vector<MapImageGridCompositeMismatch> compositeMismatches;
};
struct MapImageGridResult { DecodedImage image; MapImageGridStats stats; bool experimental=true; };
// Render the stored surface-image grid verbatim: the per-cell id is read from the
// MAP-decoded 0x00FE9880 grid, never re-derived from the dispatch chain. Tilesets
// without a supplied archive are counted (archiveUnavailable), not guessed. A
// multi-tile record is placed once per block from the cell's own draw-state offset
// (0x00FDCD70) when the subTile group side (0x00F9D620) equals the declared side.
MapImageGridResult renderMapImageGridViewport(const OriginalMapTerrain&, const OriginalTerrainState&,
    const std::vector<MapImageGridTileset>&, const OriginalMapCamera&);

// ---- Chain score against the stored grid -----------------------------------
// Per-branch tally of the reconstructed chain against the stored 0x00FE9880 grid,
// plus a classification of the emitted-but-mismatched cells so a residual is a
// named cause and not a bare count. Every mismatch falls in exactly one class:
// the stored word is 0 while the chain emitted (storedZeroMismatch); the stored
// word names a tileset other than 3 (storedTilesetOther[stored>>14], with index 15
// holding "15 or more" - the corpus uses 2, 5, 6, 8 and 16); or both name
// tileset 3 and the resolved archive indexes differ. For the last class the signed
// stored-minus-predicted delta is bucketed: indexDelta[i] holds delta i-8 for
// -8..+8, and indexDeltaMagnitude holds |delta|>8 in the doubling buckets 9-16,
// 17-32, 33-64, 65-128, 129-256, 257-512, 513-1024, >1024, so a wrong variant
// (small delta) separates from a wrong code (large, quantised delta).
struct ChainScoreBranch {
    std::string branch; size_t cells=0,emitted=0,matched=0,storedZero=0;
    size_t storedNotTileset3=0,storedZeroMismatch=0;
    std::array<size_t,17> indexDelta{};
    std::array<size_t,8> indexDeltaMagnitude{};
    std::array<size_t,16> storedTilesetOther{};
};
struct ChainScoreStats {
    size_t activeCells=0, storedZero=0, chainUnknown=0, chainNoWrite=0, chainKnown=0;
    // emitted: the chain produced a determinate ground code that resolved to a
    // tileset-3 archive index (the candidate prediction). matched: that prediction
    // equals the stored grid (same tileset 3 and same archive index). mismatched:
    // emitted but the stored grid differs. knownMatched is the total agreement:
    // matched plus the cells where the chain declines to write and the grid
    // stores 0.
    size_t emitted=0, matched=0, mismatched=0, unresolved=0, knownMatched=0;
    size_t chainNoWriteVsStoredZero=0, chainNoWriteVsStoredNonzero=0, storedZeroUnpredicted=0;
    std::vector<ChainScoreBranch> branches;
};
ChainScoreStats scoreGroundChainAgainstImageGrid(const OriginalMapTerrain&, const OriginalTerrainState&,
    const SG3Archive& tileset3, uint32_t firstRecordId=201);

enum class SG3DiagnosticComponent { Full, Top };
struct SG3DiagnosticLayer {
    uint32_t logicalID=0;
    SG3DiagnosticComponent component=SG3DiagnosticComponent::Full;
    double centerX=0,centerY=0;
};
struct SG3DiagnosticScene {
    uint32_t width=0,height=0,scale=1;
    std::vector<SG3DiagnosticLayer> layers;
};
// Caller-declared integer active-cell to physical-pixel affine registration.
// canvasOrigin is the physical-pixel top-left of the diagnostic canvas. The
// registration is an explicit hypothesis, never recovered from SAV fields.
struct SG3DiagnosticRegistration {
    std::array<std::array<int64_t,3>,2> matrix{};
    std::array<int64_t,2> canvasOrigin{};
};
// Validated crop/on-map cell only; no resource ID selection or elevation rule.
std::optional<OriginalMapPoint> projectSG3DiagnosticCell(const OriginalMapTerrain&,
    const SG3DiagnosticRegistration&,int64_t activeX,int64_t activeY);
struct SG3DiagnosticResult {
    DecodedImage image;
    // Final natural-alpha producer: layer index+1, or0 for transparent/unknown.
    std::vector<uint16_t> producer;
    std::vector<size_t> visiblePerLayer;
    size_t coveredPixels=0,decodedLayers=0;
};
// Explicit hypotheses only. No MAP/SAV selector, screenshot/background input,
// colour correction, inferred visibility mask or original draw-order contract.
// Bounded internal, nonmirrored, binary-alpha type30 single-cell components.
DecodedImage decodeSG3DiagnosticComponent(const SG3Archive&,size_t logicalIndex,
    const std::string& internal555,SG3DiagnosticComponent);
SG3DiagnosticResult renderSG3DiagnosticScene(const SG3Archive&,const std::string& internal555,
    const SG3DiagnosticScene&);
// Path-only preflight; fresh filesystem-equivalent leaves require the exclusive
// reservation below. Resolve actual parent
// directories first (including symlinks and dot segments), then compare targets
// and existing-file identities. Returns normalized fresh paths; never writes.
std::vector<std::string> freshReferenceOutputs(const std::string& dataRoot,
    const std::vector<std::string>& requested);
// Reserve every destination with openat(O_CREAT|O_EXCL|O_NOFOLLOW) before any
// payload writer runs. Kernel filesystem identity handles case/Unicode aliases.
// Writers use held file descriptors only; failure removes only owned fresh files.
class ExclusiveReferenceOutputs {
public:
    ExclusiveReferenceOutputs(const std::string& dataRoot,const std::vector<std::string>& requested);
    ~ExclusiveReferenceOutputs();
    ExclusiveReferenceOutputs(const ExclusiveReferenceOutputs&)=delete;
    ExclusiveReferenceOutputs& operator=(const ExclusiveReferenceOutputs&)=delete;
    void write(size_t index,const uint8_t* bytes,size_t count);
    void commit();
private:
    struct File { std::string leaf; int parent=-1,fd=-1; uint64_t device=0,inode=0; };
    std::vector<File> files_;
    bool committed_=false;
    void cleanup() noexcept;
};
}
