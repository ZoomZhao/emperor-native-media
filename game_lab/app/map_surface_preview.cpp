// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace game_lab {
namespace {
struct Group {uint32_t word=0,side=0;int x=0,y=0;std::vector<size_t> cells;};
const SG3Sprite* resolved(const SG3Archive& archive,size_t index){
    if(index>=archive.numImages||index>=archive.sprites.size())return nullptr;
    while(archive.sprites[index].mirrorOffset){
        const auto delta=archive.sprites[index].mirrorOffset;
        if(delta>=0||int64_t(index)+delta<0)return nullptr;
        index=size_t(int64_t(index)+delta);
    }
    return &archive.sprites[index];
}
uint32_t baseSide(const SG3Sprite& r){
    if(r.type!=30||r.width<78||r.height<40)return 0;
    const uint32_t n=(uint32_t(r.width)+2)/80;
    if(!n||r.width!=int32_t(80*n-2)||r.height<int32_t(40*n)||
        r.uncompressedLength!=uint64_t(n)*n*3200||(r.tileSize&&r.tileSize!=n))return 0;
    return n;
}
}
AppSurfacePreviewResult renderMapSurfaceCandidateViewport(const OriginalMapTerrain& terrain,
        const OriginalTerrainState& state,const std::vector<MapImageGridTileset>& tilesets,const OriginalMapCamera& camera){
    AppSurfacePreviewResult out;
    try {
    auto strict=renderMapImageGridViewport(terrain,state,tilesets,camera);
    out.image=std::move(strict.image);out.verified=std::move(strict.stats);
    if(!out.image.valid())return out;
    std::set<uint32_t> eligibleWords;
    for(const auto& mismatch:out.verified.compositeMismatches)for(const auto& t:tilesets){
        if(t.tileset!=mismatch.tileset)continue;
        if(mismatch.recordId>=t.firstRecordId&&mismatch.recordId-t.firstRecordId<=0x3fff)
            eligibleWords.insert((t.tileset<<14)|(mismatch.recordId-t.firstRecordId));
        break;
    }
    if(eligibleWords.empty())return out;
    const auto activity=analyzeOriginalMap(terrain);
    // Index all active members,including malformed offsets and accepted members.
    // Mixed sides,extra members and partial groups must fail as a whole.
    using Key=std::tuple<uint32_t,int,int>;
    std::map<Key,Group> grouped;
    for(uint32_t y=0;y<activity.cropSide;++y)for(uint32_t x=0;x<activity.cropSide;++x){
        const size_t cell=size_t(y+activity.cropOrigin)*228+x+activity.cropOrigin;
        if((terrain.flags[cell]&0x80000u)||!eligibleWords.count(state.image[cell]))continue;
        const auto word=state.image[cell];
        const int ax=int(x)-int(state.draw[cell]&7),ay=int(y)-int((state.draw[cell]>>3)&7);
        auto& group=grouped[{word,ax,ay}];group.word=word;group.x=ax;group.y=ay;
        group.side=uint32_t(state.subTile[cell]&15)+1;group.cells.push_back(cell);
    }
    std::vector<Group> candidates;
    for(auto& entry:grouped){auto& group=entry.second;
        const MapImageGridTileset* selected=nullptr;
        for(const auto& t:tilesets)if(t.tileset==(group.word>>14)){selected=&t;break;}
        if(!selected)continue;
        const uint64_t record=uint64_t(group.word&0x3fff)+selected->firstRecordId;
        if(!record||record>selected->archive->numImages)continue;
        const size_t index=size_t(record-1);const auto sprite=resolved(*selected->archive,index);
        if(!sprite)continue;
        const auto n=baseSide(*sprite);
        // These are the two geometry categories left by the strict path. Never
        // draw over a strict single-cell or matching-side composite acceptance.
        if(sg3SingleCellGeometry(*selected->archive,index))continue;
        size_t eligible=0;for(const auto cell:group.cells)if(!n||uint32_t(state.subTile[cell]&15)+1!=n)++eligible;
        if(!eligible)continue;
        const uint32_t s=group.side;
        bool complete=s<=8&&group.x>=0&&group.y>=0&&group.x+int(s)<=int(activity.cropSide)&&
            group.y+int(s)<=int(activity.cropSide)&&group.cells.size()==size_t(s)*s;
        if(complete)for(uint32_t y=0;y<s;++y)for(uint32_t x=0;x<s;++x){
            const size_t cell=size_t(group.y+int(y)+int(activity.cropOrigin))*228+size_t(group.x+int(x)+int(activity.cropOrigin));
            if((terrain.flags[cell]&0x80000u)||state.image[cell]!=group.word||
                uint32_t(state.subTile[cell]&15)+1!=s||(state.draw[cell]&63)!=x+8*y)complete=false;
        }
        if(!complete){out.candidate.rejectedGroupCells+=eligible;continue;}
        out.candidate.structuralCells+=group.cells.size();++out.candidate.structuralGroups;
        candidates.push_back(std::move(group));
    }
    // A separate foreground candidate layer with a deterministic depth order.
    // It does not claim original interleaving/occlusion with accepted terrain.
    std::sort(candidates.begin(),candidates.end(),[](const Group& a,const Group& b){
        return std::make_tuple(a.x+a.y+2*int(a.side-1),a.x,a.y,a.word)<
            std::make_tuple(b.x+b.y+2*int(b.side-1),b.x,b.y,b.word);
    });
    using Resource=std::pair<uint32_t,size_t>;
    std::map<Resource,DecodedImage> cache;std::set<Resource> failed;
    for(const auto& group:candidates){
        const MapImageGridTileset* selected=nullptr;
        for(const auto& t:tilesets)if(t.tileset==(group.word>>14)){selected=&t;break;}
        const size_t index=size_t(uint64_t(group.word&0x3fff)+selected->firstRecordId-1);
        const Resource key{selected->tileset,index};auto found=cache.find(key);
        const auto sprite=resolved(*selected->archive,index);
        if(!sprite||sprite->width<=0||sprite->height<=0||sprite->width>4096||sprite->height>4096)failed.insert(key);
        if(!failed.count(key)&&found==cache.end()){
            auto decoded=decodeSG3Sprite(*selected->archive,index,selected->internal555);
            if(decoded.status!=SG3DecodeStatus::Decoded||!decoded.image.valid())failed.insert(key);
            else{
                const size_t bytes=decoded.image.pixelsBGRA.size()*4;
                if(bytes>64u*1024u*1024u-out.candidate.cacheBytes)failed.insert(key);
                else{out.candidate.cacheBytes+=bytes;found=cache.emplace(key,std::move(decoded.image)).first;}
            }
        }
        if(failed.count(key)){out.candidate.decodeFailureCells+=group.cells.size();continue;}
        const auto& image=found->second;out.candidate.decodedCells+=group.cells.size();++out.candidate.decodedGroups;
        const double centreX=double(group.x)+(double(group.side)-1)/2,centreY=double(group.y)+(double(group.side)-1)/2;
        const double px=((centreX-camera.centerX)-(centreY-camera.centerY))*40*camera.scale+double(camera.width)/2;
        const double py=((centreX-camera.centerX)+(centreY-camera.centerY))*20*camera.scale+double(camera.height)/2;
        const int scale=int(camera.scale);const uint32_t n=baseSide(*sprite);
        // Isometric art keeps its declared base centre;regular art uses the
        // storage diamond's lower tip. Both are explicitly preview conventions.
        const int left=int(std::floor(px+.5))-int(image.width/2)*scale;
        const int top=int(std::floor(py+.5))+int(20*(n?n:group.side))*scale-int(image.height)*scale;
        if(left>=int(camera.width)||top>=int(camera.height)||left+int(image.width)*scale<=0||top+int(image.height)*scale<=0)continue;
        size_t painted=0;
        for(uint32_t y=0;y<image.height;++y)for(uint32_t x=0;x<image.width;++x){
            const uint32_t source=image.pixelsBGRA[size_t(y)*image.width+x],alpha=source>>24;if(!alpha)continue;
            for(int yy=0;yy<scale;++yy)for(int xx=0;xx<scale;++xx){
                const int dx=left+int(x)*scale+xx,dy=top+int(y)*scale+yy;
                if(dx<0||dy<0||dx>=int(camera.width)||dy>=int(camera.height))continue;
                auto& target=out.image.pixelsBGRA[size_t(dy)*camera.width+size_t(dx)];
                if(alpha==255)target=source;
                else{uint32_t mixed=0;for(unsigned shift:{0u,8u,16u,24u}){
                    const auto src=(source>>shift)&255,dst=(target>>shift)&255;
                    mixed|=(src+(dst*(255-alpha)+127)/255)<<shift;}target=mixed;}
                ++painted;
            }
        }
        out.candidate.paintedPixels+=painted;
        if(painted){out.candidate.drawnCells+=group.cells.size();++out.candidate.drawnGroups;}
    }
    return out;
    }catch(const std::bad_alloc&){return {};}
}
AppSurfacePreviewResult AppMapSurface::renderPreview(const OriginalMapCamera& camera)const{
    if(!state_)return {};
    std::vector<MapImageGridTileset> tilesets;
    for(const auto& item:archives_)tilesets.push_back({item.first,&item.second.index,item.second.companion,201});
    auto pixels=camera;pixels.outlines=false;
    return renderMapSurfaceCandidateViewport(terrain_,*state_,tilesets,pixels);
}
}
