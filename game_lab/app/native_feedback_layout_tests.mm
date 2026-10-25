// SPDX-License-Identifier: MIT
#import <AppKit/AppKit.h>
#include "native_game_layout.hpp"
#include "../native_session_presentation.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
static bool overlaps(game_lab::NativeGameRect a,game_lab::NativeGameRect b){
 return a.x<b.x+b.width&&b.x<a.x+a.width&&a.y<b.y+b.height&&b.y<a.y+a.height;
}
int main(){@autoreleasepool{try{
 size_t details=0,summaries=0,categorySummaries=0,layoutCases=0,overlapChecks=0,houseEvolutionCases=0;
 double maxDetail=0,maxTooltip=0,maxSummary=0,maxCategorySummary=0,maxHouseEvolutionDetail=0;
 game_lab::NativeSessionSeed seed;seed.houseCapacity=100;seed.laborPercent=60;seed.hunterWorkers=2;seed.wellWorkers=1;
 const auto measure=[&](const std::string& text,bool icon){
  NSString* t=[NSString stringWithUTF8String:text.c_str()];NSMutableAttributedString* a=[[NSMutableAttributedString alloc]initWithString:t attributes:@{NSFontAttributeName:[NSFont systemFontOfSize:12]}];
  if(icon){NSMutableParagraphStyle*p=[NSMutableParagraphStyle new];p.firstLineHeadIndent=20;const auto n=[t rangeOfString:@"\n"].location;[a addAttribute:NSParagraphStyleAttributeName value:p range:NSMakeRange(0,n==NSNotFound?t.length:n)];}
  return [a boundingRectWithSize:NSMakeSize(154,1000) options:NSStringDrawingUsesLineFragmentOrigin|NSStringDrawingUsesFontLeading].size.height;
 };
 for(unsigned version=1;version<=6;++version)for(auto kind:{game_lab::NativeObjectKind::Road,game_lab::NativeObjectKind::House,game_lab::NativeObjectKind::Well,game_lab::NativeObjectKind::Hunter})for(unsigned mask=0;mask<16;++mask){
  game_lab::NativeSessionState s;s.rulesVersion=version;s.paused=mask&1;s.huntingEnabled=mask&2;
  game_lab::NativeCityFeedback q;q.rulesVersion=version;q.untilMeal=2;q.untilTax=4;q.foodStock=100;q.food.cycle.demand=399900;q.food.cycle.produced=100;q.tax.cycle.eligible=399900;q.tax.cycle.income=399900;
  game_lab::NativeObjectFeedback f;f.id=3999;f.kind=kind;f.residents=99;f.connected=mask&4;f.watered=mask&8;f.foodShortage=true;q.objects.push_back(f);
  const double dh=measure(game_lab::nativeObjectDescription(seed,s,3999,q),kind!=game_lab::NativeObjectKind::Road),th=measure(game_lab::nativeCityFeedbackSummary(s,q),false);
  if(dh>166)throw std::runtime_error("detail exceeds sidebar bounds v"+std::to_string(version)+" kind"+std::to_string(unsigned(kind))+" mask"+std::to_string(mask)+" detail"+std::to_string(dh));maxDetail=std::max(maxDetail,dh);maxTooltip=std::max(maxTooltip,th);++details;
 }
 game_lab::NativeHouseEvolutionView hv;hv.id=3999;hv.desirability=-99;hv.desirabilityWithBonus=99;
 hv.currentThresholdDown=-99;hv.currentThresholdUp=100;hv.currentMet=false;hv.nextMet=false;
 hv.directionIfWater=-1;hv.directionIfNoWater=-1;hv.currentMetIfWater=false;hv.currentMetIfNoWater=false;
 hv.nextMetIfWater=false;hv.nextMetIfNoWater=false;hv.nativeWatered=false;
 hv.notNeeded={"house_water_phase28","house_desirability_bonus_60","house_runtime_tail"};
 hv.missing={"house_water_phase28","house_desirability_bonus_60","house_runtime_col2","house_runtime_col4","house_runtime_col5","house_runtime_col6","house_runtime_col7","house_runtime_col8","house_runtime_col9","house_runtime_col10","house_runtime_col11","house_runtime_col12","house_runtime_col13","house_runtime_col21","house_runtime_col22","house_runtime_col23","house_model_row1","house_evolution_519120_518ef0"};
 const auto ht=game_lab::nativeHouseEvolutionText(hv);
 if(std::count(ht.summary.begin(),ht.summary.end(),'\n')>1)throw std::runtime_error("house evolution summary exceeds two lines");
 if(ht.summary.find("house_")!=std::string::npos||ht.detail.find("house_")!=std::string::npos)throw std::runtime_error("house evolution text leaks internal key");
 game_lab::NativeSessionState hs;hs.rulesVersion=6;hs.paused=false;hs.huntingEnabled=true;
 game_lab::NativeCityFeedback hq;hq.rulesVersion=6;hq.untilMeal=2;hq.untilTax=4;hq.foodStock=0;hq.food.cycle.demand=399900;hq.food.cycle.produced=0;hq.food.laborOverride=true;
 game_lab::NativeObjectFeedback hf;hf.id=3999;hf.kind=game_lab::NativeObjectKind::House;hf.residents=99;hf.connected=false;hf.watered=false;hf.foodShortage=true;hq.objects.push_back(hf);
 const std::string combined=game_lab::nativeObjectDescription(seed,hs,3999,hq)+"\n"+ht.summary;
 const double houseDetail=measure(combined,true);
 if(houseDetail>166)throw std::runtime_error("house evolution detail exceeds sidebar bounds height"+std::to_string(houseDetail));
 if(ht.detail.find("原版房屋供水状态未接通")==std::string::npos)throw std::runtime_error("full tooltip omits a named missing reason");
 if(ht.detail.find("原版判定·若有水：下降")==std::string::npos||ht.detail.find("原版判定·若无水：下降")==std::string::npos)
  throw std::runtime_error("house tooltip omits a strict water branch");
 if(ht.detail.find("原生供水覆盖：无（自写规则，非原版）")==std::string::npos)
  throw std::runtime_error("house tooltip mislabels native water coverage");
 maxHouseEvolutionDetail=std::max(maxHouseEvolutionDetail,houseDetail);maxDetail=std::max(maxDetail,houseDetail);
 maxTooltip=std::max(maxTooltip,measure(ht.detail,false));
 game_lab::NativeHouseEvolutionView ev;ev.emptyEarlyReturn=true;const auto et=game_lab::nativeHouseEvolutionText(ev);
 if(std::count(et.summary.begin(),et.summary.end(),'\n')>1||et.detail.empty())throw std::runtime_error("empty house evolution text is not bounded");
 ++houseEvolutionCases;
 for(unsigned version=1;version<=6;++version)for(unsigned category=0;category<4;++category)for(unsigned mask=0;mask<8;++mask){
  game_lab::NativeSessionState s;s.rulesVersion=version;s.paused=mask&1;s.huntingEnabled=mask&2;s.ticks=399900;
  game_lab::NativeCityFeedback q;q.rulesVersion=version;q.untilMeal=2;q.untilTax=4;q.foodStock=100;q.food.cycle.demand=399900;q.food.cycle.produced=100;q.tax.cycle.eligible=399900;q.tax.cycle.income=399900;q.labor.available=399900;q.labor.demand=3999;
  for(auto kind:{game_lab::NativeObjectKind::Road,game_lab::NativeObjectKind::House,game_lab::NativeObjectKind::Well,game_lab::NativeObjectKind::Hunter}){
   game_lab::NativeObjectFeedback f;f.id=3999;f.kind=kind;f.residents=99;f.connected=mask&4;f.watered=mask&2;f.foodShortage=mask&1;f.workers=99;f.requiredWorkers=99;q.objects.push_back(f);
  }
  const auto text=game_lab::nativeCategorySummary(seed,s,category,q);if(text.empty())throw std::runtime_error("empty category summary");
  const double h=measure(text,false);if(h>80)throw std::runtime_error("category summary exceeds sidebar bounds v"+std::to_string(version)+" category"+std::to_string(category)+" height"+std::to_string(h));
  maxCategorySummary=std::max(maxCategorySummary,h);maxSummary=std::max(maxSummary,h);++categorySummaries;++summaries;
 }
 for(unsigned version=1;version<=3;++version){
  if(!game_lab::nativeToolAvailable(3,version)&&game_lab::nativeToolUnavailableReason(3,version).empty())throw std::runtime_error("water tool lacks disabled reason");
  if(!game_lab::nativeToolAvailable(4,version)&&game_lab::nativeToolUnavailableReason(4,version).empty())throw std::runtime_error("hunter tool lacks disabled reason");
  if(!game_lab::nativeToolAvailable(5,version)&&game_lab::nativeToolUnavailableReason(5,version).empty())throw std::runtime_error("demolition tool lacks disabled reason");
  if(!game_lab::nativeCategoryAvailable(1,version)&&game_lab::nativeCategoryUnavailableReason(1,version).empty())throw std::runtime_error("food category lacks disabled reason");
  if(!game_lab::nativeCategoryAvailable(2,version)&&game_lab::nativeCategoryUnavailableReason(2,version).empty())throw std::runtime_error("water category lacks disabled reason");
 }
 for(unsigned category=4;category<11;++category)if(game_lab::nativeCategoryUnavailableReason(category,6).empty())throw std::runtime_error("unimplemented category lacks reason");
 const auto checkLayout=[&](double width,double height){
  const auto l=game_lab::nativeGameLayout(width,height);
  std::vector<std::pair<std::string,game_lab::NativeGameRect>> rects;
  const auto add=[&](const char* name,game_lab::NativeGameRect r){if(r.width<=0||r.height<=0)throw std::runtime_error(std::string("empty layout rect ")+name);rects.push_back({name,r});};
  add("new",l.fileNew());add("quit",l.fileQuit());add("resources",l.resources());add("state",l.compactState());
  add("details",l.details());add("summary",l.summary());add("house-conditions",l.houseConditions());add("demolish-selected",l.detailDemolish());add("notice",l.notice());
  for(unsigned slot=0;slot<4;++slot)add("tool-slot",l.tool(slot));
  const auto cancel=l.toolCancel(),slot2=l.tool(2);if(cancel.x!=slot2.x||cancel.y!=slot2.y||cancel.width!=slot2.width||cancel.height!=slot2.height)throw std::runtime_error("cancel cell must own a visible palette slot");
  add("pause",l.pause());add("resume",l.resume());for(unsigned i=0;i<3;++i)add("speed",l.speed(i));
  add("water",l.waterToggle());add("hunting",l.huntingToggle());add("confirm",l.demolitionConfirm());
  add("zoom-out",l.zoomOut());add("zoom-in",l.zoomIn());add("save",l.save());add("load",l.load());add("home",l.home());add("minimap",l.minimap());
  for(unsigned row=0;row<11;++row)add("category",l.category(row));
  for(size_t i=0;i<rects.size();++i){
   const auto& a=rects[i].second;
   if(a.x<0||a.x+a.width>width||a.y<0||a.y+a.height>height)throw std::runtime_error("layout rect outside window: "+rects[i].first);
   for(size_t j=i+1;j<rects.size();++j){++overlapChecks;if(overlaps(a,rects[j].second))throw std::runtime_error("right-panel overlap: "+rects[i].first+" / "+rects[j].first);}
  }
  ++layoutCases;
 };
 for(double width:{1024.,1120.,1280.,1440.,2048.})for(double height:{768.,800.,900.,1200.,2048.})checkLayout(width,height);
 std::cout<<"{\"passed\":true,\"authored_detail_cases\":"<<details<<",\"authored_summary_cases\":"<<summaries<<",\"authored_category_cases\":"<<categorySummaries<<",\"house_evolution_cases\":"<<houseEvolutionCases<<",\"layout_cases\":"<<layoutCases<<",\"overlap_checks\":"<<overlapChecks<<",\"max_detail_height\":"<<maxDetail<<",\"max_house_evolution_detail_height\":"<<maxHouseEvolutionDetail<<",\"max_summary_height\":"<<maxSummary<<",\"max_category_summary_height\":"<<maxCategorySummary<<",\"max_tooltip_height\":"<<maxTooltip<<",\"font_points\":12,\"window_created\":false}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}}
