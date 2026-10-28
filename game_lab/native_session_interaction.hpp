// SPDX-License-Identifier: MIT
#pragma once
#include "native_session.hpp"
#include "original_map.hpp"
#include <cmath>
namespace game_lab {
// Image pixels and Cocoa logical points are distinct. Fit and picking use the
// same centred rectangle; backing pixels (Retina) never enter this transform.
struct NativeCanvasRect {double x=0,y=0,width=0,height=0;};
inline NativeCanvasRect nativeCanvasFit(double width,double height,double imageWidth,double imageHeight) {
    if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0||imageWidth<=0||imageHeight<=0||!std::isfinite(imageWidth)||!std::isfinite(imageHeight))return {};
    const double ratio=std::min(width/imageWidth,height/imageHeight);
    return {(width-imageWidth*ratio)/2,(height-imageHeight*ratio)/2,imageWidth*ratio,imageHeight*ratio};
}
inline std::optional<OriginalMapPoint> nativeCanvasImagePoint(NativeCanvasRect fit,double imageWidth,double imageHeight,double x,double y) {
    if(!std::isfinite(x)||!std::isfinite(y)||fit.width<=0||fit.height<=0||x<fit.x||y<fit.y||x>=fit.x+fit.width||y>=fit.y+fit.height)return std::nullopt;
    return OriginalMapPoint{(x-fit.x)*imageWidth/fit.width,(y-fit.y)*imageHeight/fit.height};
}
inline bool nativeCameraValid(const OriginalMapCamera& c,uint32_t side) {
    return side>0&&side<=228&&c.width>0&&c.width<=2048&&c.height>0&&c.height<=2048&&c.scale>=1&&c.scale<=4&&
        std::isfinite(c.centerX)&&std::isfinite(c.centerY)&&c.centerX>=0&&c.centerY>=0&&c.centerX<side&&c.centerY<side;
}
inline OriginalMapPoint nativeSessionProject(const OriginalMapCamera& c,double x,double y) {
    return {double(c.width)/2+(x-y-c.centerX+c.centerY)*40*c.scale,double(c.height)/2+(x+y-c.centerX-c.centerY)*20*c.scale};
}
// Half-open diamond boundaries: a tie belongs to the increasing coordinate.
inline std::optional<std::pair<int,int>> nativeSessionTile(const OriginalMapCamera& c,uint32_t side,double px,double py) {
    if(!nativeCameraValid(c,side)||!std::isfinite(px)||!std::isfinite(py)||px<0||py<0||px>=c.width||py>=c.height)return std::nullopt;
    const double a=(px-double(c.width)/2)/(40*c.scale),b=(py-double(c.height)/2)/(20*c.scale);
    const double x=std::floor(c.centerX+(a+b)/2+.5),y=std::floor(c.centerY+(b-a)/2+.5);
    if(x<0||y<0||x>=side||y>=side)return std::nullopt;return std::pair<int,int>{int(x),int(y)};
}
// Lightweight legacy preview. Resource-backed v8 App callers route through
// previewNativeSessionCommand in native_house_runtime.hpp instead.
inline NativeSessionResult nativeSessionPlacementPreview(const NativeSessionSeed& seed,const NativeSessionState& state,NativeSessionAction action,int64_t x,int64_t y) {
    if(action!=NativeSessionAction::Road&&action!=NativeSessionAction::House&&action!=NativeSessionAction::Well&&action!=NativeSessionAction::Hunter&&action!=NativeSessionAction::Inspector)return {false,"no_construction_tool"};
    auto copy=state;return transactNativeSession(seed,copy,{action,state.revision,x,y});
}
using NativeTile=std::pair<int,int>;
struct NativeRoadGesture {
    std::optional<NativeTile> start,end;
    void cancel(){start.reset();end.reset();}
    void begin(std::optional<NativeTile> point){cancel();if(point){start=point;end=point;}}
    void move(std::optional<NativeTile> point){if(!point)cancel();else if(start)end=point;}
    std::optional<std::pair<NativeTile,NativeTile>> finish(std::optional<NativeTile> point){
        move(point);const auto result=start&&end?std::make_optional(std::make_pair(*start,*end)):std::nullopt;cancel();return result;
    }
};
inline OriginalMapCamera nativeSessionPan(OriginalMapCamera c,uint32_t side,double dx,double dy) {
    if(!nativeCameraValid(c,side)||!std::isfinite(dx)||!std::isfinite(dy))return c;
    const double a=dx/(40*c.scale),b=dy/(20*c.scale);
    c.centerX=std::clamp(c.centerX-(a+b)/2,0.0,double(side)-.001);
    c.centerY=std::clamp(c.centerY-(b-a)/2,0.0,double(side)-.001);return c;
}
inline OriginalMapCamera nativeSessionZoom(OriginalMapCamera c,uint32_t side,int direction,double px,double py) {
    if(!nativeCameraValid(c,side)||!std::isfinite(px)||!std::isfinite(py))return c;
    const unsigned scale=unsigned(std::clamp(int(c.scale)+(direction>0?1:direction<0?-1:0),1,4));
    const double dx=px-double(c.width)/2,dy=py-double(c.height)/2;
    const double factor=1.0/c.scale-1.0/scale;
    c.centerX=std::clamp(c.centerX+(dx/40+dy/20)*factor/2,0.0,double(side)-.001);
    c.centerY=std::clamp(c.centerY+(dy/20-dx/40)*factor/2,0.0,double(side)-.001);c.scale=scale;return c;
}
} // namespace game_lab
