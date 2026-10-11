#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
namespace game_lab {
struct MapTileRect {double x=0,y=0,width=0,height=0;};
// View state only: top-left map coordinates, independent of window size or city data.
class MapViewport {
public:
    static constexpr int dimension=128;
    double zoom() const{return zoom_;}
    double left() const{return left_;}
    double top() const{return top_;}
    double span() const{return dimension/zoom_;}
    void reset(){zoom_=1;left_=top_=0;}
    bool pan(double x,double y) {
        if(!std::isfinite(x)||!std::isfinite(y))return false;
        const double nextX=left_+x,nextY=top_+y;
        if(!std::isfinite(nextX)||!std::isfinite(nextY))return false;
        left_=std::clamp(nextX,0.0,dimension-span());top_=std::clamp(nextY,0.0,dimension-span());return true;
    }
    bool zoomBy(double factor,double anchorX,double anchorY) {
        if(!std::isfinite(factor)||factor<=0||!std::isfinite(anchorX)||!std::isfinite(anchorY))return false;
        const double product=zoom_*factor;if(!std::isfinite(product)||product<=0)return false;
        const double nextZoom=std::clamp(product,1.0,8.0),nextSpan=dimension/nextZoom;
        const double fractionX=std::clamp((anchorX-left_)/span(),0.0,1.0);
        const double fractionY=std::clamp((anchorY-top_)/span(),0.0,1.0);
        const double nextX=anchorX-fractionX*nextSpan,nextY=anchorY-fractionY*nextSpan;
        if(!std::isfinite(nextX)||!std::isfinite(nextY))return false;
        zoom_=nextZoom;left_=std::clamp(nextX,0.0,dimension-nextSpan);top_=std::clamp(nextY,0.0,dimension-nextSpan);return true;
    }
    bool pointToTile(double x,double y,double width,double height,int& outX,int& outY) const {
        if(!std::isfinite(x)||!std::isfinite(y)||!validSize(width,height)||x<0||x>=width||y<0||y>=height)return false;
        const double right=left_+span(),bottom=top_+span();
        const double mx=std::min(left_+(x/width)*span(),std::nextafter(right,left_));
        const double my=std::min(top_+((height-y)/height)*span(),std::nextafter(bottom,top_));
        const int tx=std::clamp(int(std::floor(mx)),0,dimension-1),ty=std::clamp(int(std::floor(my)),0,dimension-1);
        outX=tx;outY=ty;return true;
    }
    bool tileRect(unsigned x,unsigned y,double width,double height,MapTileRect& out) const {
        if(x>=dimension||y>=dimension||!validSize(width,height))return false;
        MapTileRect result{(double(x)-left_)/span()*width,(top_+span()-double(y)-1)/span()*height,width/span(),height/span()};
        if(result.x+result.width<=0||result.y+result.height<=0||result.x>=width||result.y>=height)return false;
        out=result;return true;
    }
    std::string summary() const {
        char text[160];std::snprintf(text,sizeof(text),"%.0fx 左上(%.3f,%.3f) 范围%.3f格",zoom_,left_,top_,span());return text;
    }
private:
    static bool validSize(double width,double height){return std::isfinite(width)&&std::isfinite(height)&&width>0&&height>0;}
    double zoom_=1,left_=0,top_=0;
};
} // namespace game_lab
