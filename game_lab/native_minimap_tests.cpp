#include "native_minimap.hpp"
#include "native_session_source.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
using namespace game_lab;
static size_t checks=0;static void need(bool b){++checks;if(!b)throw std::runtime_error("overview invariant");}
static void near(double a,double b){need(std::abs(a-b)<1e-9);}
static OriginalMapTerrain terrain(unsigned s){OriginalMapTerrain t;t.activeSide=s;t.flags.assign(228*228,0x80000);const unsigned o=(228-s)/2;
 for(unsigned y=0;y<s;++y)for(unsigned x=0;x<s;++x)if(std::abs(int(x)-int(s/2))+std::abs(int(y)-int(s/2))<int(s/2))t.flags[(y+o)*228+x+o]=0x80;return t;}
static void verify(const OriginalMapTerrain&t){const auto copy=t.flags;auto p=makeNativeTerrainOverview(t);need(bool(p));need(p->bgra.size()==222*222);need(p->activeCells==analyzeOriginalMap(t).activeCells);
 for(unsigned y=0;y<p->side;++y)for(unsigned x=0;x<p->side;++x)if(p->onMap(x,y)){auto uv=p->project(x,y);auto back=p->inverse(uv.x,uv.y);near(back.x,x);near(back.y,y);need(uv.x>0&&uv.x<1&&uv.y>0&&uv.y<1);}
 for(double u:{0.,.001,.25,.5,.75,.999,1.})for(double v:{0.,.001,.25,.5,.75,.999,1.}){auto q=nativeOverviewPick(*p,u,v);need(bool(q));need(q->x>=0&&q->y>=0&&q->x<p->side&&q->y<p->side);need(p->onMap(int(std::floor(q->x+.5)),int(std::floor(q->y+.5))));}
 for(unsigned scale:{1,2,3,4})for(unsigned w:{798,894,1440}){OriginalMapCamera c;c.centerX=c.centerY=p->side/2;c.scale=scale;c.width=w;c.height=720;auto r=nativeOverviewViewport(*p,c);need(r.width>0&&r.height>0&&r.x>=0&&r.y>=0&&r.x+r.width<=1&&r.y+r.height<=1);if(r.x>0&&r.x+r.width<1)near(r.width*p->spanU,w/(40.*scale));if(r.y>0&&r.y+r.height<1)near(r.height*p->spanV,720/(20.*scale));
 const auto uv=p->project(c.centerX,c.centerY);near(r.x+r.width/2,uv.x);near(r.y+r.height/2,uv.y);auto zoom=nativeSessionZoom(c,p->side,1,c.width/2.,c.height/2.);auto rz=nativeOverviewViewport(*p,zoom);need(rz.width<=r.width&&rz.height<=r.height);
 auto pan=nativeSessionPan(c,p->side,80,40);auto rp=nativeOverviewViewport(*p,pan);need(rp.x<r.x||r.x==0);need(rp.y<r.y||r.y==0);}
 for(double invalid:{-1.,1.001,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){need(!nativeOverviewPick(*p,invalid,.5));need(!nativeOverviewPick(*p,.5,invalid));}
 need(copy==t.flags);const auto hash=p->bgra;for(unsigned i=0;i<100;++i){OriginalMapCamera c;c.centerX=c.centerY=p->side/2;nativeOverviewViewport(*p,c);}need(p->bgra==hash);
}
int main(int argc,char**argv){if(argc>1){const char*d=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!d){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}if(argc!=3)return 2;auto root=ResourceRoot::fromAbsolute(d);std::string e;auto s=loadNativeSessionSource(root,argv[2],e);need(bool(s));verify(s->save.terrain);auto p=makeNativeTerrainOverview(s->save.terrain);std::cout<<"{\"real_sources\":1,\"passed\":true,\"active_cells\":"<<p->activeCells<<",\"checks\":"<<checks<<",\"GUI\":false}\n";return 0;}
 for(unsigned s:{84,112,140,170,226})verify(terrain(s));auto t=terrain(112);need(!makeNativeTerrainOverview(t,15)&&!makeNativeTerrainOverview(t,513));need(makeNativeTerrainOverview(t,16)&&makeNativeTerrainOverview(t,512));auto o=makeNativeTerrainOverview(t);t.flags[0]=0;need(!makeNativeTerrainOverview(t));t=terrain(112);t.flags.clear();need(!makeNativeTerrainOverview(t));t=terrain(112);t.activeSide=113;need(!makeNativeTerrainOverview(t));t=terrain(112);std::fill(t.flags.begin(),t.flags.end(),0x80000);need(!makeNativeTerrainOverview(t));need(!makeNativeTerrainOverview({}));
 need(nativeOverviewColor(0x80004)==0&&nativeOverviewColor(4)!=nativeOverviewColor(1)&&nativeOverviewColor(0x40)!=nativeOverviewColor(0));OriginalMapCamera c;c.centerX=-1;need(nativeOverviewViewport(*o,c).width==0);
 std::cout<<"{\"passed\":true,\"authored_checks\":"<<checks<<",\"map_sizes\":5,\"GUI\":false}\n";
}
