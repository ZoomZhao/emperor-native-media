#pragma once
#include "original_building_view.hpp"
#include "map_reference.hpp"
#include "original_save.hpp"
#include <set>

namespace game_lab {
struct OriginalBuildingPlacement {
    OriginalBuildingFootprint footprint;
    OriginalBuildingImageBinding binding;
    uint32_t drawX=0,drawY=0;
};
struct OriginalBuildingScene {
    OriginalTerrainState surface;
    std::vector<OriginalBuildingPlacement> placed;
    std::vector<OriginalBuildingFootprint> withheldFootprints;
    std::vector<OriginalBuildingMarker> markers;
    size_t liveRecords=0,bindableRecords=0,placedCells=0,rejectedRecords=0;
    bool accepted=false;
    bool groundRebuilt=false;
    size_t groundCells=0,groundUnknown=0,groundNoWrite=0,groundUnresolved=0,unboundBuildingCells=0;
    std::vector<GroundMapCell> groundSelections;
    std::vector<uint8_t> drawWritten,subTileWritten,buildingOccupancy;
};
// Owned exe SHA256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
// 0x4B72B0: view codes0,2,4,6 mark (0,h-1),(0,0),(w-1,0),(w-1,h-1).
// The existing flat projection is view0 (0x4699D0), so only that view is rendered.
inline std::optional<std::pair<uint32_t,uint32_t>> originalBuildingDrawCorner(
    const OriginalBuildingFootprint& f,unsigned view=0){
    const uint32_t w=originalFootprintWidth(f),h=originalFootprintHeight(f);
    if(!w||!h||w>6||h>6||view>6||(view&1))return std::nullopt;
    return std::make_pair(uint32_t(f.x)+(view>=4?w-1:0),uint32_t(f.y)+(view==0||view==6?h-1:0));
}
// Compose a base-image working grid, also used as the pre-ground SAV seed below.
// SAV's followingU32Unknown is not an image field. No MAP is borrowed.
inline OriginalBuildingScene composeOriginalBuildingScene(const OriginalMapTerrain& terrain,
    const OriginalBuildingProfile& profile,const SG3Archive& archive){
    OriginalBuildingScene out;
    if(!terrain.valid()||!analyzeOriginalMap(terrain).cropValid()||profile.status!=OriginalBuildingStatus::Complete||
        profile.footprints.size()>4000||profile.liveSizedRecords>4000||profile.unplacedMarkers.size()>4000)return out;
    const size_t n=OriginalTerrainState::cells;
    auto& s=out.surface;s.terrain=terrain.flags;s.image.assign(n,0);s.draw.assign(n,0);s.subTile.assign(n,0);
    s.feature.assign(n,0);s.randomByte.assign(n,0);s.gF05160.assign(n,0);s.gEAC3F0.assign(n,0);
    s.roadDistance.assign(n,0);s.fertility.assign(n,0);s.gBEBF3A.assign(n,0);s.gBF8A4A.assign(n,0);
    out.liveRecords=profile.liveSizedRecords;out.markers=profile.unplacedMarkers;
    const size_t origin=(228-terrain.activeSide)/2;
    std::vector<unsigned> owners(n,0);
    std::vector<OriginalBuildingPlacement> candidates;
    std::set<size_t> slots;
    for(const auto& f:profile.footprints){
        const auto binding=bindOriginalBuildingBaseImage(f,"DATA/China_General.sg3",archive);
        if(!binding){out.withheldFootprints.push_back(f);continue;}
        ++out.bindableRecords;
        const auto corner=originalBuildingDrawCorner(f);
        if(!validOriginalBuildingFootprint(terrain,f)||!corner||f.slot>=4000||!slots.insert(f.slot).second){++out.rejectedRecords;out.withheldFootprints.push_back(f);continue;}
        candidates.push_back({f,*binding,corner->first,corner->second});
        for(size_t y=0;y<f.size;++y)for(size_t x=0;x<f.size;++x)++owners[(origin+f.y+y)*228+origin+f.x+x];
    }
    for(const auto& p:candidates){const auto& f=p.footprint;bool overlap=false;
        for(size_t y=0;y<f.size;++y)for(size_t x=0;x<f.size;++x)overlap|=owners[(origin+f.y+y)*228+origin+f.x+x]>1;
        // Rebuild order/lifecycle overrides for overlapping records are unverified.
        if(overlap||p.binding.physicalId<201||p.binding.physicalId-201>0x3fff){++out.rejectedRecords;out.withheldFootprints.push_back(f);continue;}
        const uint32_t word=(p.binding.tileset<<14)|(p.binding.physicalId-201);
        for(size_t y=0;y<f.size;++y)for(size_t x=0;x<f.size;++x){const size_t cell=(origin+f.y+y)*228+origin+f.x+x;
            // Writer0x4B7423..0x4B743F stamps the same surface word; the small
            // table0x81FF18 supplies column | row<<3 and one corner gets0x40.
            s.image[cell]=word;s.subTile[cell]=uint8_t(f.size-1);s.draw[cell]=uint8_t(x+8*y);
            if(f.x+x==p.drawX&&f.y+y==p.drawY)s.draw[cell]|=0x40;
        }
        out.placedCells+=size_t(f.size)*f.size;out.placed.push_back(p);
    }
    out.accepted=true;return out;
}
// SAV19 clears image at0x534CB9, runs building+100 dispatch0x4BC220 at
// 0x534E36, then ground0x53D630 at0x534E40, post-object redraw0x41F400
// at0x534E5B. Unknown building art remains occupancy, never a fake SG word.
inline OriginalBuildingScene composeOriginalSavedGroundScene(const OriginalMapTerrain& terrain,
    const OriginalBuildingProfile& profile,const OriginalSavedGround& saved,
    const SG3Archive& general,const SG3Archive& ground,const SG3Archive& elevation){
    auto out=composeOriginalBuildingScene(terrain,profile,general);
    if(!out.accepted||!saved.valid()||saved.activeSide!=terrain.activeSide||saved.state.terrain!=terrain.flags){out.accepted=false;return out;}
    auto working=saved.state;working.image=out.surface.image;
    const size_t n=OriginalTerrainState::cells;
    out.buildingOccupancy.assign(n,0);
    // Terrain bit8 is the dispatch gate at0x53ECBA; even an unbound multipart
    // writer cannot turn it into ordinary ground. Market/warehouse/station/wall
    // footprints write before ground (cited individually in docs/SAV.md).
    // Entertainment has no ordinary square contract: its parent market already
    // covers those cells. Do not manufacture a square from its common size.
    for(size_t cell=0;cell<n;++cell)if(!(terrain.flags[cell]&0x80000u)&&(terrain.flags[cell]&8u)) {
        out.buildingOccupancy[cell]=working.image[cell]?1:2;
        if(!working.image[cell]){working.image[cell]=UINT32_MAX;++out.unboundBuildingCells;}
    }
    // Independently authored bound stamps, not saved metadata, seed the rebuild.
    for(size_t cell=0;cell<n;++cell)if(out.surface.image[cell]) {
        working.draw[cell]=out.surface.draw[cell];working.subTile[cell]=uint8_t((working.subTile[cell]&0xE0u)|out.surface.subTile[cell]);
    }
    auto rebuilt=selectGroundMapSeeded(working,terrain.activeSide,saved.material,saved.owner);
    if(rebuilt.cells.size()!=n||!rebuilt.surface.valid()){out.accepted=false;return out;}
    out.surface=std::move(rebuilt.surface);out.surface.terrain=terrain.flags;
    out.drawWritten=std::move(rebuilt.drawWritten);out.subTileWritten=std::move(rebuilt.subTileWritten);
    out.groundSelections=std::move(rebuilt.cells);
    for(size_t cell=0;cell<n;++cell) {
        if(out.buildingOccupancy[cell]) {
            out.surface.image[cell]=working.image[cell]==UINT32_MAX?0:working.image[cell];
            if(out.buildingOccupancy[cell]==1){out.drawWritten[cell]=0xFF;out.subTileWritten[cell]=0x0F;}
            continue;
        }
        out.surface.image[cell]=0;
        if(terrain.flags[cell]&0x80000u)continue;
        const auto& selection=out.groundSelections[cell].selection;
        if(!selection.known){if(selection.chainNoWrite)++out.groundNoWrite;else ++out.groundUnknown;continue;}
        const uint32_t tileset=selection.code>>9;const auto* archive=tileset==3?&ground:tileset==16?&elevation:tileset==2?&general:nullptr;
        const size_t slot=selection.code&0x1ffu;
        const size_t id=archive&&slot<archive->imageIndex.size()?size_t(archive->imageIndex[slot])+selection.variant:0;
        if(!archive||slot>=archive->imageIndex.size()||!archive->imageIndex[slot]||id<201||id>archive->sprites.size()||id-201>0x3fff){++out.groundUnresolved;continue;}
        out.surface.image[cell]=(tileset<<14)|uint32_t(id-201);++out.groundCells;
    }
    out.groundRebuilt=true;return out;
}
struct OriginalBuildingSceneView {
    DecodedImage image;
    MapImageGridStats bases;
    size_t liveRecords=0,placedRecords=0,withheldRecords=0,withheldOutlines=0,unshapedMarkers=0;
    size_t groundCells=0,groundUnknown=0,groundNoWrite=0,groundUnresolved=0,unboundBuildingCells=0;
};
inline OriginalBuildingSceneView renderOriginalBuildingScene(const OriginalMapTerrain& terrain,
    const OriginalBuildingScene& scene,const SG3Archive& archive,const std::string& companion,
    const OriginalMapCamera& camera,int bit=-1,const std::vector<MapImageGridTileset>& groundTilesets={}){
    OriginalBuildingSceneView out;if(!scene.accepted||scene.surface.terrain!=terrain.flags)return out;
    auto background=renderOriginalBuildingFootprints(terrain,scene.withheldFootprints,camera,bit,scene.markers);
    if(!background.image.valid())return out;
    auto tilesets=groundTilesets;tilesets.push_back({2,&archive,companion,201});
    auto bases=renderMapImageGridViewport(terrain,scene.surface,tilesets,camera,MapImageGridOrder::View0DrawCorner);
    if(!bases.image.valid())return out;
    // Existing image-grid compositor supplies the painter order and premultiplied
    // base layer. Numerical ground/withheld annotations are the diagnostic floor.
    for(size_t i=0;i<bases.image.pixelsBGRA.size();++i){const uint32_t src=bases.image.pixelsBGRA[i],a=src>>24;if(!a)continue;
        auto& dst=background.image.pixelsBGRA[i];uint32_t mixed=0;
        for(unsigned shift:{0u,8u,16u,24u})mixed|=(((src>>shift)&255)+(((dst>>shift)&255)*(255-a)+127)/255)<<shift;
        dst=mixed;
    }
    out.image=std::move(background.image);out.bases=std::move(bases.stats);out.liveRecords=scene.liveRecords;
    out.placedRecords=scene.placed.size();out.withheldRecords=scene.liveRecords>=scene.placed.size()?scene.liveRecords-scene.placed.size():0;
    out.withheldOutlines=background.valid;out.unshapedMarkers=scene.markers.size();
    out.groundCells=scene.groundCells;out.groundUnknown=scene.groundUnknown;out.groundNoWrite=scene.groundNoWrite;
    out.groundUnresolved=scene.groundUnresolved;out.unboundBuildingCells=scene.unboundBuildingCells;return out;
}
}
