#include "map_viewport.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <array>
#include <iostream>
#include <limits>
using namespace game_lab;
namespace {
int checks=0;
void require(bool ok,const char* reason){++checks;if(!ok){std::cerr<<reason<<'\n';std::exit(1);}}
void point(const MapViewport& v,double x,double y,double w,double h,int ex,int ey){int tx=-99,ty=-99;require(v.pointToTile(x,y,w,h,tx,ty)&&tx==ex&&ty==ey,"analytic click coordinate");}
void rejected(const MapViewport& v,double x,double y,double w,double h){int tx=71,ty=93;require(!v.pointToTile(x,y,w,h,tx,ty)&&tx==71&&ty==93,"invalid point preserves outputs");}
void equal(double a,double b){require(std::abs(a-b)<1e-9,"analytic marker rectangle");}
}
int main(){
    MapViewport v;require(v.zoom()==1&&v.left()==0&&v.top()==0&&v.span()==128,"full map defaults");
    point(v,0,0,640,320,0,127);point(v,639.999,319.999,640,320,127,0);point(v,320,160,640,320,64,64);
    require(v.zoomBy(2,128,64),"anchored zoom");equal(v.left(),64);equal(v.top(),32);equal(v.span(),64);
    point(v,15,312.5,640,320,65,33);MapTileRect r;require(v.tileRect(65,33,640,320,r),"visible marker");
    equal(r.x,10);equal(r.y,310);equal(r.width,10);equal(r.height,5);
    require(v.pan(-20,10),"analytic pan");equal(v.left(),44);equal(v.top(),42);point(v,320,160,640,320,76,74);
    v.reset();require(v.zoomBy(4,0,0),"initial playable zoom");equal(v.left(),0);equal(v.top(),0);equal(v.span(),32);
    require(v.zoomBy(2,8.5,8.5),"selected tile anchor preserves relative position");equal(v.left(),4.25);equal(v.top(),4.25);
    point(v,.265625*532,(1-.265625)*346,532,346,8,8);
    require(v.pan(1000,1000),"pan clamp");equal(v.left(),112);equal(v.top(),112);point(v,0,0,532,346,112,127);point(v,531.999,345.999,532,346,127,112);
    require(v.pan(-1000,-1000),"opposite clamp");equal(v.left(),0);equal(v.top(),0);
    const auto inf=std::numeric_limits<double>::infinity(),nan=std::numeric_limits<double>::quiet_NaN();
    for(auto bad:{inf,-inf,nan}) {
        rejected(v,bad,1,640,320);rejected(v,1,bad,640,320);rejected(v,1,1,bad,320);rejected(v,1,1,640,bad);
        const auto old=v.summary();require(!v.pan(bad,1)&&!v.pan(1,bad)&&v.summary()==old,"bad pan preserves camera");
        require(!v.zoomBy(bad,1,1)&&!v.zoomBy(2,bad,1)&&!v.zoomBy(2,1,bad)&&v.summary()==old,"bad zoom preserves camera");
    }
    for(auto size:{-1.,0.}){rejected(v,1,1,size,320);rejected(v,1,1,640,size);}
    rejected(v,-.001,1,640,320);rejected(v,640,1,640,320);rejected(v,1,-.001,640,320);rejected(v,1,320,640,320);
    require(!v.zoomBy(0,1,1)&&!v.zoomBy(-1,1,1)&&!v.zoomBy(std::numeric_limits<double>::max(),1,1),"zero negative overflowing zoom rejects");
    r={71,93,13,17};require(!v.tileRect(128,0,640,320,r)&&r.x==71&&r.y==93,"invalid tile preserves rectangle");
    require(!v.tileRect(127,127,640,320,r)&&r.width==13&&r.height==17,"off-view marker skipped without output mutation");
    // Independently choose tile centers in map space, then project with direct arithmetic.
    // Check every visible center for several real window shapes and authored cameras.
    size_t centers=0;self_fixture::numbers("own-viewport-shapes",{532,346,640,410,1031,711,128,8});
    for(double zoom:{1.,2.,4.,8.})for(auto origin:std::array<std::pair<double,double>,4>{{{0,0},{17.25,31.5},{64,64},{128,128}}}) {
        MapViewport camera;require(camera.zoomBy(zoom,0,0)&&camera.pan(origin.first,origin.second),"authored camera setup");
        const double left=std::min(origin.first,128-128/zoom),top=std::min(origin.second,128-128/zoom),span=128/zoom;
        equal(camera.left(),left);equal(camera.top(),top);
        for(auto shape:std::array<std::pair<double,double>,3>{{{532,346},{640,410},{1031,711}}}) {
            const auto [w,h]=shape;
            for(int y=0;y<128;++y)for(int x=0;x<128;++x) {
                const double px=(x+.5-left)*w/span,py=(top+span-y-.5)*h/span;
                if(px<=0||px>=w||py<=0||py>=h)continue;
                point(camera,px,py,w,h,x,y);MapTileRect tile;require(camera.tileRect(x,y,w,h,tile),"same visible tile has marker");
                equal(tile.x+tile.width/2,px);equal(tile.y+tile.height/2,py);++centers;
            }
        }
    }
    require(centers>10000,"actual extensive center checks");
    v.reset();require(v.zoom()==1&&v.left()==0&&v.top()==0&&v.span()==128,"full-map reset after zoom pan");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"visible_centers\":"<<centers<<",\"window_shapes\":3,\"devices\":false}\n";
}
