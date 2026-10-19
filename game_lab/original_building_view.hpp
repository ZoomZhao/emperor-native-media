#pragma once
#include "original_building.hpp"
#include <cmath>
namespace game_lab {
struct OriginalBuildingView {DecodedImage image;size_t valid=0,rejected=0,visible=0,outlinePixels=0;};
// Flat saved-coordinate footprint over the SAME SAV's numerical terrain. Amber
// is an annotation,not original artwork. No MAP substitute or guessed sprite.
inline OriginalBuildingView renderOriginalBuildingFootprints(const OriginalMapTerrain& terrain,
    const std::vector<OriginalBuildingFootprint>& footprints,const OriginalMapCamera& camera,int bit=-1){
    OriginalBuildingView result;if(footprints.size()>4000)return result;
    result.image=renderOriginalMapSpatialFlags(terrain,camera,bit);if(!result.image.valid())return result;
    auto line=[&](int x0,int y0,int x1,int y1){
        bool visible=false;const int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1;int error=dx+dy;
        while(true){if(x0>=0&&y0>=0&&x0<int(camera.width)&&y0<int(camera.height)){auto& pixel=result.image.pixelsBGRA[size_t(y0)*camera.width+size_t(x0)];if(pixel!=0xffffb000u){pixel=0xffffb000u;++result.outlinePixels;}visible=true;}if(x0==x1&&y0==y1)break;const int e2=2*error;if(e2>=dy){error+=dy;x0+=sx;}if(e2<=dx){error+=dx;y0+=sy;}}
        return visible;
    };
    for(const auto& f:footprints){
        if(!validOriginalBuildingFootprint(terrain,f)){++result.rejected;continue;}
        const auto origin=projectOriginalMapCell(terrain,camera,f.x,f.y);if(!origin){++result.rejected;continue;}++result.valid;
        // Grid-square boundaries under the existing40/20 centre basis.
        const double scale=camera.scale,n=f.size;
        const double px[]={origin->x,origin->x+40*n*scale,origin->x,origin->x-40*n*scale};
        const double py[]={origin->y-20*scale,origin->y+20*(n-1)*scale,origin->y+20*(2*n-1)*scale,origin->y+20*(n-1)*scale};
        bool visible=false;for(size_t edge=0;edge<4;++edge){const size_t next=(edge+1)%4;visible|=line(int(std::floor(px[edge]+.5)),int(std::floor(py[edge]+.5)),int(std::floor(px[next]+.5)),int(std::floor(py[next]+.5)));}result.visible+=visible;
    }return result;
}
}
