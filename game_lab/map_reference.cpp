#include "map_reference.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace game_lab {
namespace {
// A stored record reached through its mirror chain, with the addressed (post-hop)
// fields the composite rule needs. sg3SingleCellGeometry repeats the same hop; this
// exposes the resolved pixel size, which that helper deliberately does not.
struct SG3SurfaceRecord {
    size_t index;int32_t width,height;uint16_t type,tileSize;uint32_t length,uncompressedLength;
};
std::optional<SG3SurfaceRecord> resolveSG3SurfaceRecord(const SG3Archive& a,size_t index) {
    if(index>=a.numImages||index>=a.sprites.size())return std::nullopt;
    while(a.sprites[index].mirrorOffset) {
        const int64_t delta=a.sprites[index].mirrorOffset;
        if(delta>=0||int64_t(index)+delta<0)return std::nullopt;
        index=size_t(int64_t(index)+delta);
    }
    const auto& s=a.sprites[index];
    return SG3SurfaceRecord{index,s.width,s.height,s.type,s.tileSize,s.length,s.uncompressedLength};
}
// A composite (multi-tile) surface record packs cols*cols single cells: cols columns
// of `(width+2)/80`, width exactly cols*80-2, height at least cols*40, an uncompressed
// size of cols*cols*3200, and tileSize in {0,cols}. The declaration is the geometry,
// not a guessed grid. Returns the block side for a confirmed composite, else nullopt.
std::optional<uint32_t> sg3CompositeCols(const SG3SurfaceRecord& r) {
    if(r.type!=30||r.width<78)return std::nullopt;
    const uint32_t cols=uint32_t((uint32_t(r.width)+2)/80);
    if(cols<2||r.width!=int32_t(cols*80-2)||r.height<int32_t(cols*40))return std::nullopt;
    if(uint64_t(r.uncompressedLength)!=uint64_t(cols)*cols*3200)return std::nullopt;
    if(r.tileSize&&r.tileSize!=cols)return std::nullopt;
    return cols;
}
}
// FUN_00475B60 @0x00475B60 assigns a 41-entry pointer table; entries 0..24 and
// 38..40 are literal names (see docs/MAP_SG3_PIPELINE.md). Only the version-213
// archives the stored grid references are listed here. The name alone is the
// mapping evidence: several version-213 archives share a 201-based type-30 run,
// so geometry cannot distinguish them.
const char* mapImageGridArchiveName(uint32_t tileset) {
    switch(tileset) {
    case 2: return "China_General.sg3";
    case 3: return "China_Terrain.sg3";
    case 5: return "China_Mon_Tumulus.sg3";
    case 6: return "China_Mon_Grand_Canal.sg3";
    case 8: return "China_Mon_Earthen_Greatwall_1.sg3";
    case 16: return "China_Elevation.sg3";
    default: return nullptr;
    }
}
MapReferenceResult renderMapReferenceViewport(const OriginalMapTerrain& terrain,const OriginalMapRawLayer& raw,
        const SG3Archive& archive,const std::string& internal555,int64_t base,const OriginalMapCamera& camera) {
    MapReferenceResult result;const auto activity=analyzeOriginalMap(terrain);size_t pixels=0;
    if(!raw.valid()||!activity.cropValid()||base < -int64_t(UINT32_MAX)||base>int64_t(UINT32_MAX)||
       !std::isfinite(camera.centerX)||!std::isfinite(camera.centerY)||camera.centerX<0||camera.centerY<0||
       camera.centerX>=activity.cropSide||camera.centerY>=activity.cropSide||camera.scale<1||camera.scale>4||
       camera.width>2048||camera.height>2048||!imagePixelCount(camera.width,camera.height,pixels)||pixels>2048u*2048u||
       !archive.numImages||archive.numImages!=archive.sprites.size())return result;
    try {
        result.image.width=camera.width;result.image.height=camera.height;result.image.pixelsBGRA.assign(pixels,0);
        std::unordered_map<size_t,DecodedImage> cache;const int scale=int(camera.scale);
        // Authored flat diagnostic painter order. This is not an original-game contract.
        for(uint32_t depth=0;depth<2*activity.cropSide-1;++depth)for(uint32_t x=0;x<activity.cropSide;++x) {
            if(depth<x||depth-x>=activity.cropSide)continue;const uint32_t y=depth-x;
            const size_t cell=size_t(y+activity.cropOrigin)*228+x+activity.cropOrigin;
            if(terrain.flags[cell]&0x80000u)continue;++result.stats.activeCells;
            const int64_t candidate=int64_t(raw.values[cell])-base;
            if(candidate<=0||uint64_t(candidate)>archive.numImages){++result.stats.outsideArchive;continue;}
            ++result.stats.validIDs;const size_t index=size_t(candidate-1);const auto geometry=sg3SingleCellGeometry(archive,index);
            if(!geometry||geometry->height>4096){++result.stats.unknownGeometry;continue;}
            ++result.stats.geometrySupported;
            const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
            const double px=(dx-dy)*40*scale+double(camera.width)/2,py=(dx+dy)*20*scale+double(camera.height)/2;
            const int left=int(std::floor(px+.5))-39*scale,top=int(std::floor(py+.5))-(int(geometry->height)-20)*scale;
            if(left>=int(camera.width)||top>=int(camera.height)||left+78*scale<=0||top+int(geometry->height)*scale<=0){++result.stats.culled;continue;}
            ++result.stats.visibleCells;
            auto found=cache.find(index);
            if(found==cache.end()) {
                auto decoded=decodeSG3Sprite(archive,index,internal555);
                if(!decoded.image.valid()){++result.stats.decodeFailures;continue;}
                const size_t bytes=decoded.image.pixelsBGRA.size()*4;
                if(bytes>64u*1024u*1024u-result.stats.cacheBytes){result.image={};return result;}
                result.stats.cacheBytes+=bytes;found=cache.emplace(index,std::move(decoded.image)).first;++result.stats.uniqueDecoded;
            }
            const auto& image=found->second;++result.stats.drawnCells;
            for(uint32_t row=0;row<image.height;++row)for(uint32_t col=0;col<image.width;++col) {
                const uint32_t source=image.pixelsBGRA[size_t(row)*image.width+col];const uint32_t alpha=source>>24;if(!alpha)continue;
                for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                    const int xx=left+int(col)*scale+sx,yy=top+int(row)*scale+sy;
                    if(xx<0||yy<0||xx>=int(camera.width)||yy>=int(camera.height))continue;
                    auto& target=result.image.pixelsBGRA[size_t(yy)*camera.width+size_t(xx)];
                    if(alpha==255){target=source;continue;}
                    uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u}) {
                        const uint32_t src=(source>>shift)&255,dst=(target>>shift)&255;
                        mixed|=(src+(dst*(255-alpha)+127)/255)<<shift;
                    }target=mixed;
                }
            }
        }
        return result;
    }catch(const std::bad_alloc&){return {};}
}
MapGroundResult renderMapGroundViewport(const OriginalMapTerrain& terrain,const OriginalTerrainState& state,
        const SG3Archive& archive,const std::string& internal555,const OriginalMapCamera& camera) {
    MapGroundResult result;const auto activity=analyzeOriginalMap(terrain);size_t pixels=0;
    if(!terrain.valid()||!state.valid()||terrain.flags.size()!=state.terrain.size()||
       !activity.cropValid()||
       !std::isfinite(camera.centerX)||!std::isfinite(camera.centerY)||camera.centerX<0||camera.centerY<0||
       camera.centerX>=activity.cropSide||camera.centerY>=activity.cropSide||camera.scale<1||camera.scale>4||
       camera.width>2048||camera.height>2048||!imagePixelCount(camera.width,camera.height,pixels)||pixels>2048u*2048u||
       !archive.numImages||archive.numImages!=archive.sprites.size())return result;
    try {
        result.image.width=camera.width;result.image.height=camera.height;result.image.pixelsBGRA.assign(pixels,0);
        std::unordered_map<size_t,DecodedImage> cache;const int scale=int(camera.scale);
        std::map<std::string,size_t> branches;
        // Same authored flat diagnostic painter order as renderMapReferenceViewport.
        for(uint32_t depth=0;depth<2*activity.cropSide-1;++depth)for(uint32_t x=0;x<activity.cropSide;++x) {
            if(depth<x||depth-x>=activity.cropSide)continue;const uint32_t y=depth-x;
            const size_t cell=size_t(y+activity.cropOrigin)*OriginalMapTerrain::storageSide+x+activity.cropOrigin;
            if(terrain.flags[cell]&0x80000u)continue;++result.stats.activeCells;
            // The engine chain reads the runtime terrain flags, the per-cell random
            // byte, the feature bits and the fertility rank; all four come straight
            // from the parsed MAP bytes.
            const auto selection=selectGroundTileCode(groundCodeRequest(state,cell));
            ++branches[selection.branch];
            // The chain may decline to write 0x00FE9880 for a cell. That is not
            // "no ground": the grid is loaded from the `.map` payload, so the cell
            // keeps its stored art. It is counted apart from unknownCells.
            if(selection.chainNoWrite){++result.stats.chainNoWriteCells;continue;}
            if(!selection.known){++result.stats.unknownCells;continue;}
            ++result.stats.knownCells;
            if(selection.block)++result.stats.blockCells;
            size_t index=0;
            // unresolvedCodes keeps no-slot and no-supported-geometry cells visible;
            // MapGroundStats deliberately exposes no second geometry-failure counter.
            // The setter's variant offsets the resolved record in id space; code
            // stays the pure base code.
            if(!resolveGroundTileCode(archive,selection.code,selection.variant,index)){++result.stats.unresolvedCodes;continue;}
            const auto geometry=sg3SingleCellGeometry(archive,index);
            if(!geometry||geometry->height>4096){++result.stats.unresolvedCodes;continue;}
            ++result.stats.geometrySupported;
            const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
            const double px=(dx-dy)*40*scale+double(camera.width)/2,py=(dx+dy)*20*scale+double(camera.height)/2;
            const int left=int(std::floor(px+.5))-39*scale,top=int(std::floor(py+.5))-(int(geometry->height)-20)*scale;
            if(left>=int(camera.width)||top>=int(camera.height)||left+78*scale<=0||top+int(geometry->height)*scale<=0){++result.stats.culled;continue;}
            ++result.stats.visibleCells;
            auto found=cache.find(index);
            if(found==cache.end()) {
                auto decoded=decodeSG3Sprite(archive,index,internal555);
                if(!decoded.image.valid()){++result.stats.decodeFailures;continue;}
                const size_t bytes=decoded.image.pixelsBGRA.size()*4;
                if(bytes>64u*1024u*1024u-result.stats.cacheBytes){result.image={};return result;}
                result.stats.cacheBytes+=bytes;found=cache.emplace(index,std::move(decoded.image)).first;++result.stats.uniqueDecoded;
            }
            const auto& image=found->second;++result.stats.drawnCells;
            for(uint32_t row=0;row<image.height;++row)for(uint32_t col=0;col<image.width;++col) {
                const uint32_t source=image.pixelsBGRA[size_t(row)*image.width+col];const uint32_t alpha=source>>24;if(!alpha)continue;
                for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                    const int xx=left+int(col)*scale+sx,yy=top+int(row)*scale+sy;
                    if(xx<0||yy<0||xx>=int(camera.width)||yy>=int(camera.height))continue;
                    auto& target=result.image.pixelsBGRA[size_t(yy)*camera.width+size_t(xx)];
                    if(alpha==255){target=source;continue;}
                    uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u}) {
                        const uint32_t src=(source>>shift)&255,dst=(target>>shift)&255;
                        mixed|=(src+(dst*(255-alpha)+127)/255)<<shift;
                    }target=mixed;
                }
            }
        }
        result.stats.branchCells.assign(branches.begin(),branches.end());
        return result;
    }catch(const std::bad_alloc&){return {};}
}
MapImageGridResult renderMapImageGridViewport(const OriginalMapTerrain& terrain,
        const OriginalTerrainState& state,const std::vector<MapImageGridTileset>& tilesets,
        const OriginalMapCamera& camera) {
    MapImageGridResult result;const auto activity=analyzeOriginalMap(terrain);size_t pixels=0;
    if(!terrain.valid()||!state.valid()||terrain.flags.size()!=state.terrain.size()||
       !activity.cropValid()||
       !std::isfinite(camera.centerX)||!std::isfinite(camera.centerY)||camera.centerX<0||camera.centerY<0||
       camera.centerX>=activity.cropSide||camera.centerY>=activity.cropSide||camera.scale<1||camera.scale>4||
       camera.width>2048||camera.height>2048||!imagePixelCount(camera.width,camera.height,pixels)||pixels>2048u*2048u)
        return result;
    for(const auto& tileset:tilesets)
        if(!tileset.archive||tileset.archive->numImages!=tileset.archive->sprites.size())
            return result;
    try {
        result.image.width=camera.width;result.image.height=camera.height;result.image.pixelsBGRA.assign(pixels,0);
        // The cache key carries the tileset because two archives share the 0-based
        // index space and a bare index would collide across them.
        std::map<std::pair<uint32_t,size_t>,DecodedImage> cache;const int scale=int(camera.scale);
        std::set<std::pair<uint32_t,size_t>> failedDecode;
        std::map<uint32_t,size_t> cellsByTileset,drawnByTileset;
        // Camera-free composite pre-pass. The per-cell offset is the engine's own
        // draw state: 0x00FDCD70 bits 0..2 are the column and bits 3..5 the row of
        // the cell inside a square ground group (verified 5,727/5,727 confirmed
        // blocks, 0 exceptions). The group side is the low nibble of the per-cell
        // sub-tile byte 0x00F9D620 plus one (verified on the same 5,727 blocks and
        // on all 219 other multi-cell components; it is constant across a block, so
        // it is NOT an s*s sub-tile index). Bit 5 of that byte is randomByte
        // 0x00F1E780 bit 0 — `or byte [cell+0x00F9D620],0x20` at 0x004B39CB inside
        // FUN_004B3970 @0x004B3970 is gated on `randomByte & 1`, and the payload
        // agrees on 2,107,821/2,107,836 active cells. Bits 4 and 6 are runtime
        // flags set at 0x004B8AB0 / 0x00424A60 and never stored in a city .map.
        // Exe SHA-256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
        // Placement is anchor = cell - (column,row), not a component bounding box,
        // so an overwrite that breaks the component shape no longer loses the block.
        // plan is per storage cell: -1 normal, -2 covered by a block, or the index
        // of the block anchored at that cell.
        struct CompositeAnchor {
            const MapImageGridTileset* tileset; size_t index;
            uint32_t cols,width,height,cells;
        };
        struct ComponentInfo {
            uint32_t tileset,recordId;int32_t width,height;uint32_t cols,bboxWidth,bboxHeight;
            bool fullRect,divisible;std::vector<uint32_t> members;
        };
        struct CompositeGroup {
            size_t anchor;uint32_t cols;uint32_t tileset,index,width,height;
            uint32_t cells=0;std::vector<uint8_t> seen;bool conflict=false,overlap=false;
        };
        const uint32_t cropSide=activity.cropSide,cropOrigin=activity.cropOrigin;
        const size_t gridCells=terrain.flags.size();
        std::vector<int32_t> plan(gridCells,-1);std::vector<CompositeAnchor> anchors;
        {
            std::vector<uint8_t> visited(gridCells,0),bboxConfirmed(gridCells,0),reasonAt(gridCells,0);
            std::vector<uint32_t> colsAt(gridCells,0),recordAt(gridCells,0);
            std::vector<const MapImageGridTileset*> tilesetAt(gridCells,nullptr);
            std::vector<ComponentInfo> components;std::vector<uint32_t> stack,members;
            for(uint32_t cy=0;cy<cropSide;++cy)for(uint32_t cx=0;cx<cropSide;++cx) {
                const size_t start=size_t(cy+cropOrigin)*OriginalMapTerrain::storageSide+cx+cropOrigin;
                if((terrain.flags[start]&0x80000u)||!state.image[start]||visited[start])continue;
                const uint32_t value=state.image[start];
                stack.assign(1,(cy<<16)|cx);visited[start]=1;members.clear();
                uint32_t minX=cx,maxX=cx,minY=cy,maxY=cy;
                while(!stack.empty()) {
                    const uint32_t packed=stack.back();stack.pop_back();
                    const uint32_t mx=packed&0xFFFFu,my=packed>>16;members.push_back(packed);
                    minX=std::min(minX,mx);maxX=std::max(maxX,mx);minY=std::min(minY,my);maxY=std::max(maxY,my);
                    for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx) {
                        if((dx==0)==(dy==0))continue; // 4-connected only
                        const int nx=int(mx)+dx,ny=int(my)+dy;
                        if(nx<0||ny<0||nx>=int(cropSide)||ny>=int(cropSide))continue;
                        const size_t next=size_t(size_t(ny+cropOrigin)*OriginalMapTerrain::storageSide+size_t(nx+cropOrigin));
                        if(visited[next]||state.image[next]!=value||(terrain.flags[next]&0x80000u))continue;
                        visited[next]=1;stack.push_back((uint32_t(ny)<<16)|uint32_t(nx));
                    }
                }
                const uint32_t tilesetId=value>>14;
                const MapImageGridTileset* selected=nullptr;
                for(const auto& tileset:tilesets)if(tileset.tileset==tilesetId){selected=&tileset;break;}
                if(!selected)continue; // per-cell path counts archiveUnavailable
                const uint32_t recordId=(value&0x3FFFu)+selected->firstRecordId;
                if(recordId<selected->firstRecordId||recordId<1||size_t(recordId)>selected->archive->numImages)continue;
                const size_t index=size_t(recordId-1);
                if(sg3SingleCellGeometry(*selected->archive,index))continue; // single-cell record: per-cell draw
                const auto record=resolveSG3SurfaceRecord(*selected->archive,index);
                const uint32_t bboxWidth=maxX-minX+1,bboxHeight=maxY-minY+1;
                const auto blockSide=record?sg3CompositeCols(*record):std::optional<uint32_t>();
                if(!record||!blockSide) { // non-type-30 / broken mirror: no structural block rule
                    result.stats.compositeForeignCells+=members.size();
                    result.stats.compositeMismatches.push_back(MapImageGridCompositeMismatch{
                        tilesetId,recordId,record?record->width:0,record?record->height:0,0,0,
                        bboxWidth,bboxHeight,uint32_t(members.size()),false,false,
                        "record does not declare an s*s block"});
                    continue;
                }
                const uint32_t cols=*blockSide;
                const bool full=members.size()==size_t(bboxWidth)*bboxHeight;
                const bool divisible=bboxWidth%cols==0&&bboxHeight%cols==0;
                ComponentInfo info{tilesetId,recordId,record->width,record->height,cols,
                    bboxWidth,bboxHeight,full,divisible,{}};
                info.members.reserve(members.size());
                for(uint32_t packed:members) {
                    const uint32_t mx=packed&0xFFFFu,my=packed>>16;
                    const size_t member=size_t(size_t(my+cropOrigin)*OriginalMapTerrain::storageSide+size_t(mx+cropOrigin));
                    info.members.push_back(member);
                    colsAt[member]=cols;recordAt[member]=uint32_t(index);
                    tilesetAt[member]=selected;bboxConfirmed[member]=full&&divisible?1:0;
                }
                components.push_back(std::move(info));
            }
            // Per-cell offset pass: group every composite cell by its implied anchor.
            std::vector<CompositeGroup> groups;std::unordered_map<size_t,size_t> byAnchor;
            for(size_t cell=0;cell<gridCells;++cell) {
                const uint32_t cols=colsAt[cell];if(!cols)continue;
                const uint32_t side=(uint32_t(state.subTile[cell]&0x0Fu)+1u);
                if(side!=cols){reasonAt[cell]=1;continue;}
                const uint8_t draw=state.draw[cell];
                const uint32_t column=draw&7u,row=(uint32_t(draw)>>3)&7u;
                if(column>=cols||row>=cols){reasonAt[cell]=2;continue;}
                const int cx=int(cell%OriginalMapTerrain::storageSide)-int(cropOrigin);
                const int cy=int(cell/OriginalMapTerrain::storageSide)-int(cropOrigin);
                const int ax=cx-int(column),ay=cy-int(row);
                if(ax<0||ay<0||ax+int(cols)>int(cropSide)||ay+int(cols)>int(cropSide)){reasonAt[cell]=3;continue;}
                const size_t anchor=size_t(ay+int(cropOrigin))*OriginalMapTerrain::storageSide+
                    size_t(ax+int(cropOrigin));
                auto inserted=byAnchor.emplace(anchor,groups.size());
                if(inserted.second) {
                    CompositeGroup group;group.anchor=anchor;group.cols=cols;
                    group.tileset=tilesetAt[cell]->tileset;group.index=recordAt[cell];
                    const auto resolved=resolveSG3SurfaceRecord(*tilesetAt[cell]->archive,recordAt[cell]);
                    group.width=uint32_t(resolved->width);group.height=uint32_t(resolved->height);
                    group.seen.assign(size_t(cols)*cols,0);groups.push_back(std::move(group));
                }
                CompositeGroup& group=groups[inserted.first->second];
                if(group.conflict||group.cols!=cols||group.tileset!=tilesetAt[cell]->tileset||group.index!=recordAt[cell]) {
                    group.conflict=true;
                    continue; // Incoming dimensions must never index the first group's storage.
                }
                const size_t offset=size_t(row)*cols+column;
                if(offset>=group.seen.size()){group.conflict=true;continue;}
                if(group.seen[offset])group.conflict=true;
                group.seen[offset]=1;++group.cells;
            }
            // A group is accepted only when every surviving offset is unique and the
            // anchor cell itself still carries the record (offset 0,0). Two accepted
            // blocks may not share a footprint cell: a repeated offset would otherwise
            // draw the same block twice at two anchors. Anything else is counted,
            // never silently placed.
            std::vector<int32_t> anchorOfGroup(groups.size(),-1),footprintOwner(gridCells,-1);
            std::vector<uint8_t> overlap(groups.size(),0);
            for(size_t g=0;g<groups.size();++g) {
                const auto& group=groups[g];
                if(group.conflict||!group.seen[0])continue;
                for(size_t o=0;o<group.seen.size();++o) {
                    const uint32_t column=uint32_t(o)%group.cols,row=uint32_t(o)/group.cols;
                    const int cx=int(group.anchor%OriginalMapTerrain::storageSide)-int(cropOrigin);
                    const int cy=int(group.anchor/OriginalMapTerrain::storageSide)-int(cropOrigin);
                    const size_t cell=size_t(size_t(cy+int(row)+cropOrigin)*OriginalMapTerrain::storageSide+
                        size_t(cx+int(column)+cropOrigin));
                    if(footprintOwner[cell]>=0) {
                        overlap[g]=1;overlap[size_t(footprintOwner[cell])]=1;
                    } else footprintOwner[cell]=int32_t(g);
                }
            }
            for(size_t g=0;g<groups.size();++g)if(overlap[g])groups[g].overlap=true;
            for(size_t g=0;g<groups.size();++g) {
                const auto& group=groups[g];
                if(group.conflict||group.overlap||!group.seen[0])continue;
                anchorOfGroup[g]=int32_t(anchors.size());
                anchors.push_back(CompositeAnchor{tilesetAt[group.anchor],group.index,
                    group.cols,group.width,group.height,group.cells});
                ++result.stats.compositeBlocks;
                if(bboxConfirmed[group.anchor])result.stats.compositeConfirmedCells+=group.cells;
                else result.stats.compositeRecoveredCells+=group.cells;
                if(group.cells<group.cols*group.cols){++result.stats.compositePartialBlocks;
                    result.stats.compositePartialBlockCells+=group.cells;}
            }
            for(size_t cell=0;cell<gridCells;++cell) {
                const uint32_t cols=colsAt[cell];if(!cols)continue;
                if(reasonAt[cell])continue;
                const int cx=int(cell%OriginalMapTerrain::storageSide)-int(cropOrigin);
                const int cy=int(cell/OriginalMapTerrain::storageSide)-int(cropOrigin);
                const uint8_t draw=state.draw[cell];
                const uint32_t column=draw&7u,row=(uint32_t(draw)>>3)&7u;
                const size_t anchor=size_t(cy-int(row)+int(cropOrigin))*OriginalMapTerrain::storageSide+
                    size_t(cx-int(column)+int(cropOrigin));
                const auto found=byAnchor.find(anchor);
                if(found==byAnchor.end()){reasonAt[cell]=4;continue;}
                const auto& group=groups[found->second];
                if(group.conflict){reasonAt[cell]=4;continue;}
                if(group.overlap){reasonAt[cell]=6;continue;}
                if(!group.seen[0]){reasonAt[cell]=5;continue;}
                plan[cell]=-2;
            }
            for(size_t g=0;g<groups.size();++g)
                if(anchorOfGroup[g]>=0)plan[groups[g].anchor]=anchorOfGroup[g];
            static const char* const reasons[]={"","subTile group side != record cols",
                "draw offset outside the record block","block anchor outside the active crop",
                "conflicting draw offsets for one block anchor",
                "block anchor cell carries another record","overlapping block footprints"};
            for(const auto& component:components) {
                size_t unresolved=0;uint32_t reason=0,groupSide=0;
                for(size_t member:component.members)if(reasonAt[member]) {
                    ++unresolved;if(!reason)reason=reasonAt[member];
                    if(!groupSide)groupSide=uint32_t(state.subTile[member]&0x0Fu)+1u;
                }
                if(!unresolved)continue;
                result.stats.compositeMismatchedCells+=unresolved;
                result.stats.compositeMismatches.push_back(MapImageGridCompositeMismatch{
                    component.tileset,component.recordId,component.width,component.height,component.cols,
                    groupSide,component.bboxWidth,component.bboxHeight,uint32_t(unresolved),
                    component.fullRect,component.divisible,reasons[reason]});
            }
        }
        // Same authored flat diagnostic painter order as the other viewports.
        for(uint32_t depth=0;depth<2*activity.cropSide-1;++depth)for(uint32_t x=0;x<activity.cropSide;++x) {
            if(depth<x||depth-x>=activity.cropSide)continue;const uint32_t y=depth-x;
            const size_t cell=size_t(y+activity.cropOrigin)*OriginalMapTerrain::storageSide+x+activity.cropOrigin;
            if(terrain.flags[cell]&0x80000u)continue;++result.stats.activeCells;
            // Read the stored surface-image id verbatim. 0 means the original left
            // the cell without a ground code; it is counted, never drawn.
            const uint32_t stored=state.image[cell];
            if(!stored){++result.stats.zeroCells;continue;}
            ++result.stats.nonzeroCells;
            const uint32_t tilesetId=stored>>14,low14=stored&0x3FFFu;
            ++cellsByTileset[tilesetId];
            // A confirmed composite draws once per block at its own anchor cell; the
            // rest of the component is covered and skipped. decode is camera-free up
            // to the cull test, exactly like the single-cell path.
            const int32_t planEntry=plan[cell];
            if(planEntry==-2)continue;
            if(planEntry>=0) {
                const auto& anchor=anchors[size_t(planEntry)];
                // cells is the number of surviving cells this block accounts for; for a
                // full block it is cols*cols, for a partially overwritten block the
                // overwritten cells are accounted by their own records.
                const size_t blockCells=anchor.cells;
                const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
                const double px=(dx-dy)*40*scale+double(camera.width)/2,py=(dx+dy)*20*scale+double(camera.height)/2;
                const int left=int(std::floor(px+.5))-int((anchor.cols-1)*40+39)*scale;
                const int top=int(std::floor(py+.5))-(int(anchor.height)-int(anchor.cols)*40+20)*scale;
                const auto key=std::make_pair(anchor.tileset->tileset,anchor.index);
                if(failedDecode.count(key)){result.stats.decodeFailures+=blockCells;continue;}
                auto found=cache.find(key);
                if(found==cache.end()) {
                    auto decoded=decodeSG3Sprite(*anchor.tileset->archive,anchor.index,anchor.tileset->internal555);
                    if(!decoded.image.valid()){failedDecode.insert(key);result.stats.decodeFailures+=blockCells;continue;}
                    const size_t bytes=decoded.image.pixelsBGRA.size()*4;
                    if(bytes>64u*1024u*1024u-result.stats.cacheBytes){result.image={};return result;}
                    result.stats.cacheBytes+=bytes;found=cache.emplace(key,std::move(decoded.image)).first;++result.stats.uniqueDecoded;
                }
                result.stats.decodedCells+=blockCells;
                if(left>=int(camera.width)||top>=int(camera.height)||left+int(anchor.width)*scale<=0||top+int(anchor.height)*scale<=0){result.stats.culled+=blockCells;continue;}
                result.stats.visibleCells+=blockCells;
                const auto& image=found->second;result.stats.drawnCells+=blockCells;++result.stats.drawnBlocks;
                drawnByTileset[anchor.tileset->tileset]+=blockCells;
                for(uint32_t row=0;row<image.height;++row)for(uint32_t col=0;col<image.width;++col) {
                    const uint32_t source=image.pixelsBGRA[size_t(row)*image.width+col];const uint32_t alpha=source>>24;if(!alpha)continue;
                    for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                        const int xx=left+int(col)*scale+sx,yy=top+int(row)*scale+sy;
                        if(xx<0||yy<0||xx>=int(camera.width)||yy>=int(camera.height))continue;
                        auto& target=result.image.pixelsBGRA[size_t(yy)*camera.width+size_t(xx)];
                        if(alpha==255){target=source;continue;}
                        uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u}) {
                            const uint32_t src=(source>>shift)&255,dst=(target>>shift)&255;
                            mixed|=(src+(dst*(255-alpha)+127)/255)<<shift;
                        }target=mixed;
                    }
                }
                continue;
            }
            const MapImageGridTileset* selected=nullptr;
            for(const auto& tileset:tilesets)if(tileset.tileset==tilesetId){selected=&tileset;break;}
            if(!selected){++result.stats.archiveUnavailable;continue;}
            const uint32_t recordId=low14+selected->firstRecordId;
            if(recordId<selected->firstRecordId||recordId<1||size_t(recordId)>selected->archive->numImages) {
                ++result.stats.unresolvedIndices;continue;
            }
            const size_t index=size_t(recordId-1);
            const auto geometry=sg3SingleCellGeometry(*selected->archive,index);
            if(!geometry||geometry->height>4096){++result.stats.geometryUnsupported;continue;}
            const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
            const double px=(dx-dy)*40*scale+double(camera.width)/2,py=(dx+dy)*20*scale+double(camera.height)/2;
            const int left=int(std::floor(px+.5))-39*scale,top=int(std::floor(py+.5))-(int(geometry->height)-20)*scale;
            const auto key=std::make_pair(tilesetId,index);
            if(failedDecode.count(key)){++result.stats.decodeFailures;continue;}
            auto found=cache.find(key);
            if(found==cache.end()) {
                auto decoded=decodeSG3Sprite(*selected->archive,index,selected->internal555);
                if(!decoded.image.valid()){failedDecode.insert(key);++result.stats.decodeFailures;continue;}
                const size_t bytes=decoded.image.pixelsBGRA.size()*4;
                if(bytes>64u*1024u*1024u-result.stats.cacheBytes){result.image={};return result;}
                result.stats.cacheBytes+=bytes;found=cache.emplace(key,std::move(decoded.image)).first;++result.stats.uniqueDecoded;
            }
            ++result.stats.decodedCells;
            if(left>=int(camera.width)||top>=int(camera.height)||left+78*scale<=0||top+int(geometry->height)*scale<=0){++result.stats.culled;continue;}
            ++result.stats.visibleCells;
            const auto& image=found->second;++result.stats.drawnCells;++drawnByTileset[tilesetId];
            for(uint32_t row=0;row<image.height;++row)for(uint32_t col=0;col<image.width;++col) {
                const uint32_t source=image.pixelsBGRA[size_t(row)*image.width+col];const uint32_t alpha=source>>24;if(!alpha)continue;
                for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx) {
                    const int xx=left+int(col)*scale+sx,yy=top+int(row)*scale+sy;
                    if(xx<0||yy<0||xx>=int(camera.width)||yy>=int(camera.height))continue;
                    auto& target=result.image.pixelsBGRA[size_t(yy)*camera.width+size_t(xx)];
                    if(alpha==255){target=source;continue;}
                    uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u}) {
                        const uint32_t src=(source>>shift)&255,dst=(target>>shift)&255;
                        mixed|=(src+(dst*(255-alpha)+127)/255)<<shift;
                    }target=mixed;
                }
            }
        }
        result.stats.tilesetCells.assign(cellsByTileset.begin(),cellsByTileset.end());
        result.stats.tilesetDrawn.assign(drawnByTileset.begin(),drawnByTileset.end());
        return result;
    }catch(const std::bad_alloc&){return {};}
}
ChainScoreStats scoreGroundChainAgainstImageGrid(const OriginalMapTerrain& terrain,
        const OriginalTerrainState& state,const SG3Archive& tileset3,uint32_t firstRecordId,
        const SG3Archive* tileset16) {
    ChainScoreStats result;
    const auto activity=analyzeOriginalMap(terrain);
    if(!terrain.valid()||!state.valid()||terrain.flags.size()!=state.terrain.size()||!activity.cropValid())
        return result;
    std::map<std::string,ChainScoreBranch> branches;
    auto branch=[&](const std::string& name) -> ChainScoreBranch& {
        auto found=branches.find(name);
        if(found==branches.end())found=branches.emplace(name,ChainScoreBranch{name,0,0,0,0}).first;
        return found->second;
    };
    const auto selections=selectGroundMap(state,terrain.activeSide);
    if(selections.size()!=terrain.flags.size())return result;
    for(size_t cell=0;cell<terrain.flags.size();++cell) {
        if(terrain.flags[cell]&0x80000u)continue;++result.activeCells;
        const uint32_t stored=state.image[cell];
        const auto& selection=selections[cell].selection;
        auto& stats=branch(std::string(selection.branch));
        ++stats.cells;
        if(!stored){++result.storedZero;++stats.storedZero;}
        // Convert the stored grid into the chain's archive-index space. The same
        // empirical base the grid render uses: stored index = low14 + (firstRecordId-1).
        const uint32_t storedTileset=stored>>14,storedLow=stored&0x3FFFu;
        const size_t storedIndex=size_t(storedLow)+size_t(firstRecordId)-1;
        if(selection.chainNoWrite) {
            ++result.chainNoWrite;
            if(!stored){++result.chainNoWriteVsStoredZero;++result.knownMatched;}
            else ++result.chainNoWriteVsStoredNonzero;
            continue;
        }
        if(!selection.known){++result.chainUnknown;if(!stored)++result.storedZeroUnpredicted;continue;}
        ++result.chainKnown;
        size_t index=0;
        const uint32_t predictedTileset=selection.code>>9;
        const auto* archive=predictedTileset==3u?&tileset3:predictedTileset==16u?tileset16:nullptr;
        if(!archive||!resolveGroundTileCode(*archive,predictedTileset,selection.code,selection.variant,index)) {
            ++result.unresolved;if(!stored)++result.storedZeroUnpredicted;continue;
        }
        ++result.emitted;++stats.emitted;
        if(stored&&storedTileset==predictedTileset&&index==storedIndex) {
            ++result.matched;++result.knownMatched;++stats.matched;
        } else {
            ++result.mismatched;
            if(!stored) {++result.storedZeroUnpredicted;++stats.storedZeroMismatch;}
            else if(storedTileset!=predictedTileset) {
                ++stats.storedNotTileset3;
                ++stats.storedTilesetOther[storedTileset<16u?size_t(storedTileset):size_t(15)];
            } else {
                // Same tileset, different archive index: bucket the signed delta so a
                // constant offset (a wrong base) separates from a scattered one (a
                // wrong variant). Bucket i holds delta i-8; larger magnitudes go to
                // the doubling buckets 9-16, 17-32, ... 513-1024, >1024.
                const int64_t delta=int64_t(storedIndex)-int64_t(index);
                if(delta>=-8&&delta<=8)++stats.indexDelta[size_t(delta+8)];
                else {
                    int64_t m=delta<0?-delta:delta;size_t bucket=0;
                    while(bucket<7&&m>16) {m=(m+1)/2;++bucket;}
                    ++stats.indexDeltaMagnitude[bucket];
                }
            }
        }
    }
    for(auto& entry:branches)result.branches.push_back(entry.second);
    return result;
}
}

namespace game_lab {
DecodedImage decodeSG3DiagnosticComponent(const SG3Archive& archive,size_t index,
        const std::string& internal,SG3DiagnosticComponent component) {
    if(archive.numImages!=archive.sprites.size() || !sg3SingleCellGeometry(archive,index))return {};
    const auto& record=archive.sprites[index];
    if(record.mirrorOffset || record.external || record.alphaLength)return {};
    auto full=decodeSG3Sprite(archive,index,internal).image;
    if(!full.valid())return {};
    if(component==SG3DiagnosticComponent::Full)return full;
    if(component!=SG3DiagnosticComponent::Top)return {};
    // Reuse the native public RLE reader at the original stream boundary.
    // No subtraction of base alpha: a natural top may overlap its own base.
    if(record.length==record.uncompressedLength) {
        DecodedImage empty;empty.width=78;empty.height=uint32_t(record.height);
        empty.pixelsBGRA.assign(size_t(empty.width)*empty.height,0);return empty;
    }
    if(uint64_t(record.offset)+record.uncompressedLength>UINT32_MAX)return {};
    auto topArchive=archive;auto& top=topArchive.sprites[index];
    top.type=256;top.offset+=top.uncompressedLength;top.length-=top.uncompressedLength;
    top.uncompressedLength=0;
    return decodeSG3Sprite(topArchive,index,internal).image;
}
std::optional<OriginalMapPoint> projectSG3DiagnosticCell(const OriginalMapTerrain& terrain,
        const SG3DiagnosticRegistration& registration,int64_t x,int64_t y) {
    const auto crop=analyzeOriginalMap(terrain);
    if(!crop.cropValid() || x<0 || y<0 || x>=crop.cropSide || y>=crop.cropSide)return std::nullopt;
    const auto cell=size_t(y+crop.cropOrigin)*OriginalMapTerrain::storageSide+size_t(x+crop.cropOrigin);
    if(terrain.flags[cell]&0x80000u)return std::nullopt;
    const auto& m=registration.matrix;
    for(size_t row=0;row<2;++row) {
        for(size_t col=0;col<2;++col)if(m[row][col]<-4096 || m[row][col]>4096)return std::nullopt;
        if(m[row][2]<-1000000 || m[row][2]>1000000 ||
           registration.canvasOrigin[row]<-1000000 || registration.canvasOrigin[row]>1000000)return std::nullopt;
    }
    // Operand bounds above make all products and sums safe in int64_t.
    if(m[0][0]*m[1][1]-m[0][1]*m[1][0]==0)return std::nullopt;
    const auto px=m[0][0]*x+m[0][1]*y+m[0][2]-registration.canvasOrigin[0];
    const auto py=m[1][0]*x+m[1][1]*y+m[1][2]-registration.canvasOrigin[1];
    if(px<-1000000 || px>1000000 || py<-1000000 || py>1000000)return std::nullopt;
    return OriginalMapPoint{double(px),double(py)};
}
SG3DiagnosticResult renderSG3DiagnosticScene(const SG3Archive& archive,const std::string& internal,
        const SG3DiagnosticScene& scene) {
    size_t count=0;
    if(scene.width>2048 || scene.height>2048 || !imagePixelCount(scene.width,scene.height,count) ||
       scene.scale<1 || scene.scale>4 || scene.layers.empty() || scene.layers.size()>128)return {};
    try {
        SG3DiagnosticResult out;out.image.width=scene.width;out.image.height=scene.height;
        out.image.pixelsBGRA.assign(count,0);out.producer.assign(count,0);
        out.visiblePerLayer.assign(scene.layers.size(),0);
        for(size_t layer=0;layer<scene.layers.size();++layer) {
            const auto& spec=scene.layers[layer];
            if(!spec.logicalID || spec.logicalID>archive.numImages)return {};
            auto source=decodeSG3DiagnosticComponent(archive,spec.logicalID-1,internal,spec.component);
            if(!source.valid())return {};
            for(auto pixel:source.pixelsBGRA)if((pixel>>24)!=0 && (pixel>>24)!=255)return {};
            SG3CanvasPlacement placement;placement.width=scene.width;placement.height=scene.height;
            placement.scale=scene.scale;placement.centerX=spec.centerX;placement.centerY=spec.centerY;
            auto canvas=renderSG3SingleCellCanvas(source,placement);
            if(!canvas.valid())return {};
            ++out.decodedLayers;
            for(size_t p=0;p<count;++p)if(canvas.pixelsBGRA[p]>>24) {
                out.image.pixelsBGRA[p]=canvas.pixelsBGRA[p];out.producer[p]=uint16_t(layer+1);
            }
        }
        for(auto producer:out.producer)if(producer){++out.coveredPixels;++out.visiblePerLayer[producer-1];}
        return out;
    }catch(const std::bad_alloc&){return {};}
}
}

namespace game_lab {
std::vector<std::string> freshReferenceOutputs(const std::string& dataRoot,
        const std::vector<std::string>& requested) {
    namespace fs=std::filesystem;
    if(!fs::path(dataRoot).is_absolute())throw std::runtime_error("absolute data root required");
    const auto root=fs::canonical(dataRoot);
    std::vector<fs::path> paths;std::vector<std::string> targets;
    for(const auto& name:requested) {
        fs::path path(name);
        if(name.find('\0')!=std::string::npos || !path.is_absolute() || path.filename().empty() ||
           path.filename()=="." || path.filename()=="..")throw std::runtime_error("fresh absolute output file required");
        // Canonicalize the parent before lexical operations: symlink/.. follows
        // filesystem traversal, not merely string cancellation.
        const auto parent=fs::canonical(path.parent_path());
        if(!fs::is_directory(parent))throw std::runtime_error("output parent must be a directory");
        // Existing directory identities also protect case/normalization aliases
        // of the data-root path; lexical prefix spelling is not the boundary.
        for(auto ancestor=parent;;ancestor=ancestor.parent_path()) {
            if(fs::equivalent(ancestor,root))throw std::runtime_error("output inside data root refused");
            if(ancestor==ancestor.parent_path())break;
        }
        const auto target=(parent/path.filename()).string();
        for(size_t i=0;i<targets.size();++i) {
            std::error_code error;
            const bool sameExistingFile=fs::equivalent(path,paths[i],error);
            if(target==targets[i] || (!error && sameExistingFile))
                throw std::runtime_error("distinct PNG/producer output targets required");
        }
        paths.push_back(path);targets.push_back(target);
    }
    // Check every destination before allowing either writer to create a file.
    // symlink_status also rejects dangling final symlinks.
    for(const auto& path:paths)
        if(fs::symlink_status(path).type()!=fs::file_type::not_found)
            throw std::runtime_error("fresh absolute output path required");
    return targets;
}
}

namespace game_lab {
ExclusiveReferenceOutputs::ExclusiveReferenceOutputs(const std::string& root,
        const std::vector<std::string>& requested) {
    const auto paths=freshReferenceOutputs(root,requested);
    files_.reserve(paths.size());
    try {
        for(const auto& name:paths) {
            const std::filesystem::path path(name);
            files_.emplace_back();auto& file=files_.back();file.leaf=path.filename().string();
            file.parent=::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);
            if(file.parent<0)throw std::runtime_error("output parent open failed");
            file.fd=::openat(file.parent,file.leaf.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
            if(file.fd<0)throw std::runtime_error("exclusive fresh output reservation refused;filesystem alias or existing target");
            struct stat identity{};
            if(::fstat(file.fd,&identity)!=0 || !S_ISREG(identity.st_mode))throw std::runtime_error("reserved output identity failed");
            file.device=uint64_t(identity.st_dev);file.inode=uint64_t(identity.st_ino);
        }
    }catch(...){cleanup();throw;}
}
ExclusiveReferenceOutputs::~ExclusiveReferenceOutputs(){cleanup();}
void ExclusiveReferenceOutputs::cleanup() noexcept {
    for(auto& file:files_) {
        if(!committed_ && file.parent>=0 && file.fd>=0) {
            struct stat current{};
            if(::fstatat(file.parent,file.leaf.c_str(),&current,AT_SYMLINK_NOFOLLOW)==0 &&
               uint64_t(current.st_dev)==file.device && uint64_t(current.st_ino)==file.inode)
                (void)::unlinkat(file.parent,file.leaf.c_str(),0);
        }
        if(file.fd>=0){(void)::close(file.fd);file.fd=-1;}
        if(file.parent>=0){(void)::close(file.parent);file.parent=-1;}
    }
}
void ExclusiveReferenceOutputs::write(size_t index,const uint8_t* bytes,size_t count) {
    if(committed_ || index>=files_.size() || (count && !bytes))throw std::runtime_error("invalid reserved output write");
    size_t offset=0;
    while(offset<count) {
        const auto amount=std::min(count-offset,size_t(65536));
        const auto written=::write(files_[index].fd,bytes+offset,amount);
        if(written<0 && errno==EINTR)continue;
        if(written<=0)throw std::runtime_error("reserved output write failed");
        offset+=size_t(written);
    }
}
void ExclusiveReferenceOutputs::commit() {
    if(committed_)throw std::runtime_error("output set already committed");
    for(const auto& file:files_) {
        struct stat current{};
        if(::fstatat(file.parent,file.leaf.c_str(),&current,AT_SYMLINK_NOFOLLOW)!=0 ||
           uint64_t(current.st_dev)!=file.device || uint64_t(current.st_ino)!=file.inode)
            throw std::runtime_error("reserved output name identity changed");
        int result;do{result=::fsync(file.fd);}while(result!=0 && errno==EINTR);
        if(result!=0)throw std::runtime_error("reserved output sync failed");
    }
    committed_=true;cleanup();
}
}
