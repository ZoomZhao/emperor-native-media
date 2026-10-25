// SPDX-License-Identifier: MIT
#pragma once
#include "native_session_interaction.hpp"
#include <memory>
namespace game_lab {
// Native overview colours and affine projection; not the original radar palette.
// The centred crop/off-map flag and primitive annotations are documented in MAP.md.
struct NativeTerrainOverview {
    uint32_t side=0,pixels=0;
    double minU=0,minV=0,spanU=0,spanV=0;
    std::vector<uint32_t> flags,bgra;
    size_t activeCells=0;
    bool onMap(int x,int y)const{return x>=0&&y>=0&&x<int(side)&&y<int(side)&&!(flags[size_t(y)*side+x]&0x80000u);}
    OriginalMapPoint project(double x,double y)const{return {(x-y-minU)/spanU,(x+y-minV)/spanV};}
    OriginalMapPoint inverse(double u,double v)const{u=minU+u*spanU;v=minV+v*spanV;return {(u+v)/2,(v-u)/2};}
};
inline uint32_t nativeOverviewColor(uint32_t f){
    if(f&0x80000u)return 0;
    if(f&4u)return 0xff397aa1u; // native water blue
    if(f&0x40u)return 0xffd6c090u; // roads
    if(f&8u)return 0xffac654au; // source building-related cells
    if(f&2u)return 0xff969383u; // rock/ore annotation
    if(f&1u)return 0xff41613cu; // trees/bamboo
    if(f&0x80u)return 0xff789653u;
    return 0xff9e9c66u; // remaining land/unknown combinations, no rule inference
}
inline std::shared_ptr<const NativeTerrainOverview> makeNativeTerrainOverview(const OriginalMapTerrain& t,uint32_t pixels=222){
    if(pixels<16||pixels>512)return {};
    const auto crop=analyzeOriginalMap(t);if(!crop.cropValid())return {};
    auto o=std::make_shared<NativeTerrainOverview>();o->side=crop.cropSide;o->pixels=pixels;
    o->minU=o->minV=1e9;double maxU=-1e9,maxV=-1e9;
    for(unsigned y=0;y<o->side;++y)for(unsigned x=0;x<o->side;++x){
        const auto f=t.flags[size_t(y+crop.cropOrigin)*228+x+crop.cropOrigin];o->flags.push_back(f);if(f&0x80000u)continue;
        ++o->activeCells;const double u=double(x)-y,v=double(x)+y;
        o->minU=std::min(o->minU,u-1);o->minV=std::min(o->minV,v-1);maxU=std::max(maxU,u+1);maxV=std::max(maxV,v+1);
    }
    if(!o->activeCells)return {};o->spanU=maxU-o->minU;o->spanV=maxV-o->minV;
    o->bgra.resize(size_t(pixels)*pixels);
    for(unsigned py=0;py<pixels;++py)for(unsigned px=0;px<pixels;++px){
        const auto p=o->inverse((px+.5)/pixels,(py+.5)/pixels);const int x=int(std::floor(p.x+.5)),y=int(std::floor(p.y+.5));
        o->bgra[size_t(py)*pixels+px]=o->onMap(x,y)?nativeOverviewColor(o->flags[size_t(y)*o->side+x]):0;
    }
    return o;
}
// Clicks outside the view refuse. Clicks on its transparent boundary clamp to
// the nearest real cell; ties are resolved in stable storage-row order.
inline std::optional<OriginalMapPoint> nativeOverviewPick(const NativeTerrainOverview& o,double u,double v){
    if(!std::isfinite(u)||!std::isfinite(v)||u<0||u>1||v<0||v>1||!o.activeCells)return {};
    auto p=o.inverse(u,v);p.x=std::clamp(p.x,0.,double(o.side)-.001);p.y=std::clamp(p.y,0.,double(o.side)-.001);
    if(o.onMap(int(std::floor(p.x+.5)),int(std::floor(p.y+.5))))return p;
    double best=std::numeric_limits<double>::infinity();OriginalMapPoint found;
    for(unsigned y=0;y<o.side;++y)for(unsigned x=0;x<o.side;++x)if(o.onMap(x,y)){
        const double d=(x-p.x)*(x-p.x)+(y-p.y)*(y-p.y);if(d<best){best=d;found={double(x),double(y)};}}
    return std::isfinite(best)?std::make_optional(found):std::nullopt;
}
inline NativeCanvasRect nativeOverviewViewport(const NativeTerrainOverview& o,const OriginalMapCamera& c){
    if(!nativeCameraValid(c,o.side))return {};
    const auto mid=o.project(c.centerX,c.centerY);
    const double rx=c.width/(80.*c.scale*o.spanU),ry=c.height/(40.*c.scale*o.spanV);
    const double left=std::clamp(mid.x-rx,0.,1.),right=std::clamp(mid.x+rx,0.,1.);
    const double top=std::clamp(mid.y-ry,0.,1.),bottom=std::clamp(mid.y+ry,0.,1.);
    return {left,top,std::max(0.,right-left),std::max(0.,bottom-top)};
}
} // namespace game_lab
