#include "game_lab/app/native_game_layout.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace game_lab;
static unsigned checks=0;static void need(bool b){++checks;if(!b)throw std::runtime_error("layout invariant");}
int main(){
 const auto r=nativeGameLayout(1024,768);need(r.map.x==0&&r.map.y==8&&r.map.width==798&&r.map.height==720);need(r.top.y==728&&r.top.height==40);need(r.sideUpper.x==798&&r.sideUpper.y==310);need(r.sideLower.width==226&&r.sideLower.height==310);
 auto mm=r.minimap();need(mm.x==853&&768-mm.y-mm.height==542&&mm.width==111&&mm.height==111);
 for(unsigned row=0;row<11;++row){auto b=r.category(row);need(b.x==804&&768-b.y-b.height==38+38*row&&b.width==46&&b.height==38);}
 for(unsigned slot=0;slot<6;++slot){auto b=r.tool(slot);need(b.x==856+54*(slot%3)&&768-b.y-b.height==318+54*(slot/3)&&b.width==54&&b.height==53);}
 for(double w:{1024.,1120.,1280.,1440.,2048.})for(double h:{768.,800.,900.,1200.,2048.}){
  auto l=nativeGameLayout(w,h);need(l.map.width+l.sideUpper.width==w);need(l.map.height+l.top.height+l.bottom.height==h);need(l.map.x+l.map.width==l.sideUpper.x);need(l.sideLower.y+l.sideLower.height==l.sideUpper.y);
  auto mini=l.minimap();need(mini.x>=l.sideLower.x&&mini.x+mini.width<=w&&mini.y>=0&&mini.y+mini.height<=l.sideUpper.y);
  for(unsigned row=0;row<11;++row){auto b=l.category(row);need(b.x>=l.sideUpper.x&&b.x+b.width<=w&&b.y>=l.sideUpper.y&&b.y+b.height<=h);for(double scale:{1.,2.}){need(std::floor(b.x*scale)==b.x*scale&&std::floor(b.y*scale)==b.y*scale);}}
  for(unsigned row=0;row<6;++row){auto b=l.tool(row);need(b.x>l.category(0).x+l.category(0).width&&b.x+b.width<=w&&b.y>=l.sideUpper.y);}
 }
 for(double invalid:{-1.,0.,1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){auto l=nativeGameLayout(invalid,invalid);need(l.width==1024&&l.height==768);}
 need(r.category(11).width==0&&r.tool(6).width==0);
 std::cout<<"{\"passed\":true,\"authored_layout_checks\":"<<checks<<",\"reference_regions\":4,\"sizes\":25,\"GUI\":false}\n";
}
