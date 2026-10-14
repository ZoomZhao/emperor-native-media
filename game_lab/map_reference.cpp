#include "map_reference.hpp"
#include "asset_bytes.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
namespace game_lab {
MapReferenceResult renderMapReferenceViewport(const OriginalMapTerrain& terrain,const OriginalMapRawLayer& raw,
        const SG3Archive& archive,const std::string& internal555,int64_t base,const OriginalMapCamera& camera) {
    MapReferenceResult result;const auto activity=analyzeOriginalMap(terrain);size_t pixels=0;
    if(!raw.valid()||!activity.cropValid()||base < -int64_t(UINT32_MAX)||base>int64_t(UINT32_MAX)||
       !std::isfinite(camera.centerX)||!std::isfinite(camera.centerY)||camera.centerX<0||camera.centerY<0||
       camera.centerX>=activity.cropSide||camera.centerY>=activity.cropSide||camera.scale<1||camera.scale>4||
       camera.width>2048||camera.height>2048||!imagePixelCount(camera.width,camera.height,pixels)||pixels>2048u*2048u||
       archive.numImages!=archive.sprites.size())return result;
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
}
