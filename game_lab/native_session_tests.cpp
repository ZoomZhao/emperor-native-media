// SPDX-License-Identifier: MIT
#include "native_session_source.hpp"
#include "native_session_io.hpp"
#include "native_session_presentation.hpp"
#include <iostream>
#include <cstdlib>
#include <chrono>
#if defined(__APPLE__)
#include <mach/mach.h>
#endif
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
NativeSessionSeed fixture(){NativeSessionSeed s;s.source="test.sav";s.sourceRevision=1;s.modelRevision=2;s.side=8;s.initialFunds=100;s.roadCost=2;s.houseCost=15;s.houseCapacity=7;s.buildable.assign(64,1);s.sourceRoad.assign(64,0);return s;}
void apply(const NativeSessionSeed& seed,NativeSessionState& s,NativeSessionAction a,int x=0,int y=0){const auto r=transactNativeSession(seed,s,{a,s.revision,x,y});need(r.committed,r.error.c_str());}
void reject(const NativeSessionSeed& seed,NativeSessionState& s,NativeSessionCommand c,const std::string& reason){const auto before=encodeNativeSession(seed,s);const auto r=transactNativeSession(seed,s,c);need(!r.committed&&r.error==reason,"named rejection");need(encodeNativeSession(seed,s)==before,"failed transaction is atomic");}
void roads(){
 const auto seed=fixture();size_t endpointCases=0;
 for(int y=0;y<8;++y)for(int x=0;x<8;++x)for(int ty=0;ty<8;++ty)for(int tx=0;tx<8;++tx){
  auto state=startNativeSession(seed);const auto before=encodeNativeSession(seed,state);const auto plan=planNativeRoadPath(seed,state,x,y,tx,ty);
  const size_t count=size_t(std::abs(tx-x)+std::abs(ty-y)+1);need(plan.error.empty()&&plan.cells.size()==count&&plan.newCells==count,"all endpoint path sizes");
  need(plan.cost==int64_t(count)*seed.roadCost&&encodeNativeSession(seed,state)==before,"preview cost and purity");
  need(plan.cells.front().x==unsigned(x)&&plan.cells.front().y==unsigned(y)&&plan.cells.back().x==unsigned(tx)&&plan.cells.back().y==unsigned(ty),"path endpoints exact");
  std::set<std::pair<unsigned,unsigned>> unique;for(size_t n=0;n<plan.cells.size();++n){const auto& c=plan.cells[n];need(unique.emplace(c.x,c.y).second,"path no duplicate cell");
   if(n)need(std::abs(int(c.x)-int(plan.cells[n-1].x))+std::abs(int(c.y)-int(plan.cells[n-1].y))==1,"orthogonal path");
   need(n<=size_t(std::abs(tx-x))?c.y==unsigned(y):c.x==unsigned(tx),"X first deterministic bend");}
  need(transactNativeSession(seed,state,{NativeSessionAction::RoadPath,0,x,y,tx,ty}).committed,"whole path commit");
  need(state.revision==1&&state.objects.size()==count&&state.funds==100-int64_t(count)*2,"one revision whole cost");
  reject(seed,state,{NativeSessionAction::RoadPath,state.revision,x,y,tx,ty},"road_path_already_present");++endpointCases;
 }
 auto s=startNativeSession(seed);apply(seed,s,NativeSessionAction::Road,2,2);
 auto p=planNativeRoadPath(seed,s,1,2,3,2);need(p.newCells==2&&p.cells.size()==3&&p.cost==4&&p.cells[1].existing,"native road free exactly once");
 need(transactNativeSession(seed,s,{NativeSessionAction::RoadPath,s.revision,1,2,3,2}).committed&&s.funds==94&&s.objects.size()==3,"skip existing road in atomic path");
 auto original=seed;original.sourceRoad[2*8+2]=1;original.buildable[2*8+2]=0;auto os=startNativeSession(original);
 need(transactNativeSession(original,os,{NativeSessionAction::RoadPath,0,1,2,3,2}).committed&&os.funds==96&&os.objects.size()==2,"original road traversed but never copied or charged");
 auto blocked=seed;blocked.buildable[2*8+2]=0;auto bs=startNativeSession(blocked);
 reject(blocked,bs,{NativeSessionAction::RoadPath,0,0,2,5,2},"occupied_or_unsupported_terrain");
 auto hs=startNativeSession(seed);apply(seed,hs,NativeSessionAction::House,2,2);reject(seed,hs,{NativeSessionAction::RoadPath,hs.revision,0,2,5,2},"occupied_or_unsupported_terrain");
 auto exact=seed;exact.initialFunds=6;auto es=startNativeSession(exact);need(transactNativeSession(exact,es,{NativeSessionAction::RoadPath,0,0,0,2,0}).committed&&es.funds==0,"exact path budget accepted");
 auto poor=seed;poor.initialFunds=5;auto ps=startNativeSession(poor);reject(poor,ps,{NativeSessionAction::RoadPath,0,0,0,2,0},"insufficient_funds");
 for(auto cmd:{NativeSessionCommand{NativeSessionAction::RoadPath,0,-1,0,2,0},{NativeSessionAction::RoadPath,0,0,0,8,0},{NativeSessionAction::RoadPath,0,0,0,INT64_MAX,INT64_MIN}}){auto empty=startNativeSession(seed);reject(seed,empty,cmd,"outside_scene");}
 reject(seed,s,{NativeSessionAction::RoadPath,0,1,1,2,1},"stale_command");
 NativeRoadGesture g;const auto origin=std::make_optional(NativeTile{1,1});g.begin(origin);g.move(NativeTile{6,1});g.move(NativeTile{2,1});auto end=g.finish(NativeTile{2,1});need(end&&end->first==*origin&&end->second==NativeTile{2,1}&&!g.start,"backtracking replaces endpoint not accumulated trace");
 auto back=startNativeSession(seed);need(transactNativeSession(seed,back,{NativeSessionAction::RoadPath,0,end->first.first,end->first.second,end->second.first,end->second.second}).committed&&back.objects.size()==2,"backtracking charges final two cells only");
 for(int n=0;n<20;++n){g.begin(origin);g.move(NativeTile{4,4});g.cancel();need(!g.finish(NativeTile{4,4}),"cancel and late release cannot commit");g.begin(origin);g.move(std::nullopt);g.move(origin);need(!g.finish(origin),"leave and reenter cannot resurrect gesture");}
 g.begin(origin);need(g.finish(origin).has_value(),"zero length gesture remains single tile");g.begin(std::nullopt);need(!g.finish(origin),"invalid press never starts gesture");
 auto one=startNativeSession(seed);need(transactNativeSession(seed,one,{NativeSessionAction::RoadPath,0,1,1,1,1}).committed&&one.objects.size()==1,"zero length is one road");
 std::string error;const auto encoded=encodeNativeSession(seed,back);auto restored=decodeNativeSession(seed,encoded,error);need(restored&&encodeNativeSession(seed,*restored)==encoded,"bulk roads use unchanged v1 save format");
 std::cout<<"{\"road_endpoint_cases\":"<<endpointCases<<",\"passed\":true}\n";
}
void water(){
 const auto begin=checks;auto seed=fixture();seed.wellCost=25;
 auto v1=startNativeSession(seed);need(encodeNativeSession(seed,v1)=="ENCITY 1 original-data-native-v1\n\"test.sav\" 1 2\n8 100 2 15 7\n0 0 1 1 100 0\n","v1 exact previous bytes");
 reject(seed,v1,{NativeSessionAction::Well,0,4,2},"well_requires_native_v2");
 auto s=startNativeSession(seed,2);apply(seed,s,NativeSessionAction::Road,1,2);apply(seed,s,NativeSessionAction::House,2,2);apply(seed,s,NativeSessionAction::Resume);
 for(int n=0;n<4;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==0,"v2 dry road-connected house does not grow");
 apply(seed,s,NativeSessionAction::Pause);apply(seed,s,NativeSessionAction::Well,4,2);
 need(s.funds==58&&!nativeWellActive(seed,s.objects.back(),nativeRoads(seed,s)),"disconnected well costs original model but inactive");
 need(!nativeHouseWatered(seed,s.objects[1],nativeWaterCoverage(seed,s)),"inactive well supplies nothing");
 reject(seed,s,{NativeSessionAction::Well,s.revision,5,3},"occupied_or_unsupported_terrain");
 reject(seed,s,{NativeSessionAction::RoadPath,s.revision,4,0,4,5},"occupied_or_unsupported_terrain");
 reject(seed,s,{NativeSessionAction::Well,s.revision,7,7},"outside_scene");
 reject(seed,s,{NativeSessionAction::Well,s.revision,INT64_MAX,INT64_MIN},"outside_scene");
 apply(seed,s,NativeSessionAction::Road,4,1);need(s.funds==56&&nativeHouseWatered(seed,s.objects[1],nativeWaterCoverage(seed,s)),"road activates well and full house coverage");
 const auto beforePreview=encodeNativeSession(seed,s);need(nativeSessionPlacementPreview(seed,s,NativeSessionAction::Well,6,6).committed,"well preview permitted");need(encodeNativeSession(seed,s)==beforePreview,"well preview pure");
 reject(seed,s,{NativeSessionAction::Tick,s.revision},"paused");apply(seed,s,NativeSessionAction::Resume);for(int n=0;n<2;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==1&&s.funds==56,"supplied house grows without additional charge");
 std::string error;const auto bytes=encodeNativeSession(seed,s);auto restored=decodeNativeSession(seed,bytes,error);need(restored&&restored->rulesVersion==2&&encodeNativeSession(seed,*restored)==bytes,"v2 exact roundtrip");
 apply(seed,s,NativeSessionAction::Tick);apply(seed,*restored,NativeSessionAction::Tick);need(encodeNativeSession(seed,s)==encodeNativeSession(seed,*restored),"v2 next tick identical");
 auto legacy=startNativeSession(seed);apply(seed,legacy,NativeSessionAction::Road,1,2);apply(seed,legacy,NativeSessionAction::House,2,2);apply(seed,legacy,NativeSessionAction::Resume);for(int n=0;n<2;++n)apply(seed,legacy,NativeSessionAction::Tick);
 auto legacyReload=decodeNativeSession(seed,encodeNativeSession(seed,legacy),error);need(legacyReload&&legacyReload->rulesVersion==1&&nativePopulation(*legacyReload)==1,"legacy growth still ignores water");
 auto missing=seed;missing.wellCost=0;auto ms=startNativeSession(missing,2);need(validateNativeSession(missing,ms)=="missing_well_model_price","v2 missing price rejected");need(validateNativeSession(missing,startNativeSession(missing)).empty(),"v1 does not need well model");
 auto exact=seed;exact.initialFunds=25;auto es=startNativeSession(exact,2);apply(exact,es,NativeSessionAction::Well,0,0);need(es.funds==0,"exact well price accepted");auto poor=seed;poor.initialFunds=24;auto ps=startNativeSession(poor,2);reject(poor,ps,{NativeSessionAction::Well,0,0,0},"insufficient_funds");
 auto broken=s;broken.objects[2].residents=1;need(validateNativeSession(seed,broken)=="invalid_residents","well cannot contain residents");broken=s;broken.rulesVersion=7;need(validateNativeSession(seed,broken)=="unsupported_native_rules","unknown rules rejected");
 auto modify=[&](const std::string& a,const std::string& b){auto bad=bytes;const auto at=bad.find(a);need(at!=std::string::npos,"mutation target exists");bad.replace(at,a.size(),b);need(!decodeNativeSession(seed,bad,error),"incompatible water save rejected");};
 modify("ENCITY 2","ENCITY 3");modify("native-v2","native-v1");modify("7 25 4\n","7 25 5\n");modify("7 25 4\n","7 24 4\n");
 // Coverage is clipped square distance from the2x2 well footprint, unioned.
 auto big=seed;big.side=20;big.initialFunds=1000;big.buildable.assign(400,1);big.sourceRoad.assign(400,0);auto bs=startNativeSession(big,2);apply(big,bs,NativeSessionAction::Well,7,7);apply(big,bs,NativeSessionAction::Road,6,7);auto c=nativeWaterCoverage(big,bs);
 need(std::count(c.begin(),c.end(),1)==100,"well radius4 yields10x10 square");
 need(nativeHouseWatered(big,{0,NativeObjectKind::House,3,3,0},c),"boundary all four included");need(nativeHouseWatered(big,{0,NativeObjectKind::House,11,11,0},c),"upper boundary all four included");
 need(!nativeHouseWatered(big,{0,NativeObjectKind::House,12,11,0},c),"partial house not watered");need(!nativeHouseWatered(big,{0,NativeObjectKind::House,UINT32_MAX,11,0},c),"invalid house coordinate safe");
 apply(big,bs,NativeSessionAction::Well,17,7);apply(big,bs,NativeSessionAction::Road,16,7);c=nativeWaterCoverage(big,bs);need(nativeHouseWatered(big,{0,NativeObjectKind::House,12,7,0},c),"two well union may supply complete house");
 auto edge=startNativeSession(big,2);apply(big,edge,NativeSessionAction::Well,0,0);apply(big,edge,NativeSessionAction::Road,2,0);c=nativeWaterCoverage(big,edge);need(std::count(c.begin(),c.end(),1)==36,"edge clipping6x6 no wrap");
 apply(big,bs,NativeSessionAction::House,3,3);apply(big,bs,NativeSessionAction::Road,2,3);apply(big,bs,NativeSessionAction::House,11,11);apply(big,bs,NativeSessionAction::Road,10,11);apply(big,bs,NativeSessionAction::Resume);for(int n=0;n<2;++n)apply(big,bs,NativeSessionAction::Tick);need(nativePopulation(bs)==2,"same well supplies multiple houses without quota");
 // Bottom/right clipping across small, tested-original and maximum source sizes.
 for(unsigned side:{3u,4u,5u,6u,8u,112u,228u}){
  auto cornerSeed=seed;cornerSeed.side=side;cornerSeed.buildable.assign(size_t(side)*side,1);cornerSeed.sourceRoad.assign(size_t(side)*side,0);
  auto corner=startNativeSession(cornerSeed,2);apply(cornerSeed,corner,NativeSessionAction::Well,side-2,side-2);apply(cornerSeed,corner,NativeSessionAction::Road,side-3,side-2);
  const auto frozen=encodeNativeSession(cornerSeed,corner);const auto coverage=nativeWaterCoverage(cornerSeed,corner);const unsigned span=std::min(6u,side),first=side-span;
  need(coverage.size()==size_t(side)*side&&std::count(coverage.begin(),coverage.end(),1)==span*span,"bottom-right clipped square count");
  for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)need(bool(coverage[size_t(y)*side+x])==(x>=first&&y>=first),"bottom/right cell coverage no wrap or spill");
  if(side>=4)need(nativeHouseWatered(cornerSeed,{0,NativeObjectKind::House,side-4,side-4,0},coverage),"bottom-right nearby house fully covered");
  if(first)need(!nativeHouseWatered(cornerSeed,{0,NativeObjectKind::House,first-1,side-4,0},coverage),"corner partial coverage stays dry");
  need(encodeNativeSession(cornerSeed,corner)==frozen,"corner service evaluation pure");
 }
 std::cout<<"{\"water_bottom_right_sizes\":7,\"passed\":true}\n";
 // No demolition exists; nevertheless v2 state does not invent eviction.
 auto retained=legacy;retained.rulesVersion=2;need(validateNativeSession(seed,retained).empty(),"existing dry residents valid");apply(seed,retained,NativeSessionAction::Tick);apply(seed,retained,NativeSessionAction::Tick);need(nativePopulation(retained)==1,"loss of supply stops growth not eviction");
 std::cout<<"{\"water_authored_checks\":"<<checks-begin<<",\"passed\":true}\n";
}
void food(){
 const auto begin=checks;auto seed=fixture();seed.side=12;seed.initialFunds=1000;seed.wellCost=25;seed.hunterCost=32;seed.buildable.assign(144,1);seed.sourceRoad.assign(144,0);
 auto s=startNativeSession(seed,3);apply(seed,s,NativeSessionAction::Road,1,2);apply(seed,s,NativeSessionAction::House,2,2);apply(seed,s,NativeSessionAction::Well,2,4);apply(seed,s,NativeSessionAction::Road,1,4);
 apply(seed,s,NativeSessionAction::Resume);for(int n=0;n<2;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==0&&s.foodStock==0,"v3 water alone cannot admit immigrant");
 apply(seed,s,NativeSessionAction::Hunter,4,4);for(int n=0;n<2;++n)apply(seed,s,NativeSessionAction::Tick);need(s.foodProduced==0&&nativePopulation(s)==0,"disconnected hunter cannot produce");apply(seed,s,NativeSessionAction::Road,4,3);
 auto before=encodeNativeSession(seed,s);const auto plan=planNativeFoodCycle(seed,s);need(plan.error.empty()&&plan.cycle.produced==4&&plan.cycle.arrivals==1&&plan.stock==3,"pure production then arrival plan");need(encodeNativeSession(seed,s)==before,"meal planner pure");
 for(int n=0;n<2;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==1&&s.foodStock==3&&s.foodProduced==4&&s.foodConsumed==1&&s.funds==922,"first meal and exact construction ledger");
 apply(seed,s,NativeSessionAction::Pause);reject(seed,s,{NativeSessionAction::Tick,s.revision},"paused");apply(seed,s,NativeSessionAction::HuntingPause);reject(seed,s,{NativeSessionAction::HuntingPause,s.revision},"hunting_already_in_requested_state");apply(seed,s,NativeSessionAction::Resume);
 for(int n=0;n<6;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==2&&s.foodStock==0&&s.lastFood.demand==2&&s.lastFood.fed==0&&s.lastFood.arrivals==0,"food exhaustion stops arrivals without eviction");
 apply(seed,s,NativeSessionAction::HuntingResume);for(int n=0;n<2;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==3&&s.foodStock==1&&s.lastFood.fed==2&&s.lastFood.arrivals==1,"production recovery feeds existing residents before arrivals");
 std::string error;const auto bytes=encodeNativeSession(seed,s);auto restored=decodeNativeSession(seed,bytes,error);need(restored&&restored->rulesVersion==3&&encodeNativeSession(seed,*restored)==bytes,"v3 exact stock ledger and rule roundtrip");
 apply(seed,s,NativeSessionAction::Tick);apply(seed,*restored,NativeSessionAction::Tick);apply(seed,s,NativeSessionAction::Tick);apply(seed,*restored,NativeSessionAction::Tick);need(encodeNativeSession(seed,s)==encodeNativeSession(seed,*restored),"v3 next meal identical after reload");
 for(int n=0;n<160;++n){if(n%9==0)apply(seed,s,s.huntingEnabled?NativeSessionAction::HuntingPause:NativeSessionAction::HuntingResume);const auto old=s;apply(seed,s,NativeSessionAction::Tick);
  need(s.foodProduced-s.foodConsumed==s.foodStock,"lifetime food conservation");need(s.funds==old.funds,"food creates no money");need(nativePopulation(s)>=nativePopulation(old),"shortage never evicts");
  if(s.ticks%2==0)need(uint64_t(old.foodStock)+s.lastFood.produced==uint64_t(s.foodStock)+s.lastFood.fed+s.lastFood.arrivals,"per-cycle stock conservation");else need(old.foodStock==s.foodStock&&old.foodProduced==s.foodProduced&&old.foodConsumed==s.foodConsumed,"odd tick no food activity");}
 auto full=startNativeSession(seed,3);apply(seed,full,NativeSessionAction::Hunter,4,4);apply(seed,full,NativeSessionAction::Road,4,3);apply(seed,full,NativeSessionAction::Resume);for(int n=0;n<60;++n)apply(seed,full,NativeSessionAction::Tick);need(full.foodStock==100&&full.foodProduced==100&&full.lastFood.produced==0,"storage capacity stops excess production rather than overflow");
 auto edge=full;edge.foodStock=99;edge.foodConsumed=1;const auto room=planNativeFoodCycle(seed,edge);need(room.error.empty()&&room.cycle.produced==1&&room.stock==100,"single available storage unit accepted exactly");
 auto invalid=full;invalid.foodStock=101;need(!planNativeFoodCycle(seed,invalid).error.empty(),"stock overflow rejects before arithmetic");invalid=full;invalid.foodProduced=UINT64_MAX;need(encodeNativeSession(seed,invalid).empty(),"lifetime counter overflow rejected");invalid=full;invalid.foodConsumed=101;need(encodeNativeSession(seed,invalid).empty(),"overconsumption rejected");invalid=s;invalid.lastFood.fed=invalid.lastFood.demand+1;need(encodeNativeSession(seed,invalid).empty(),"invalid meal receipt rejected");
 auto maxTick=full;maxTick.ticks=1000000000;maxTick.revision=maxTick.ticks+10;maxTick.foodProduced=uint64_t(nativeFoodCapacity)*(maxTick.ticks/2);maxTick.foodConsumed=maxTick.foodProduced-maxTick.foodStock;reject(seed,maxTick,{NativeSessionAction::Tick,maxTick.revision},"tick_limit");
 auto two=startNativeSession(seed,3);apply(seed,two,NativeSessionAction::Road,1,2);apply(seed,two,NativeSessionAction::House,2,2);apply(seed,two,NativeSessionAction::Well,2,4);apply(seed,two,NativeSessionAction::Road,1,4);apply(seed,two,NativeSessionAction::House,4,1);apply(seed,two,NativeSessionAction::Road,4,3);two.ticks=2;two.foodStock=1;two.foodProduced=1;two.lastFood.produced=1;two.huntingEnabled=false;
 const auto ordered=planNativeFoodCycle(seed,two);need(ordered.error.empty()&&ordered.arrivals==std::vector<size_t>{1}&&ordered.stock==0,"one remaining ration assigned by stable house identity");
 auto missing=seed;missing.hunterCost=0;need(validateNativeSession(missing,startNativeSession(missing,3))=="missing_hunter_model_price","v3 requires verified producer cost");
 for(unsigned version:{1u,2u}){auto old=startNativeSession(seed,version);reject(seed,old,{NativeSessionAction::Hunter,0,4,4},"hunter_requires_native_v3");reject(seed,old,{NativeSessionAction::HuntingPause,0},"food_requires_native_v3");need(validateNativeSession(missing,old).empty(),"old versions need no hunter model");}
 auto v2seed=fixture();v2seed.wellCost=25;need(encodeNativeSession(v2seed,startNativeSession(v2seed,2))=="ENCITY 2 original-data-native-v2\n\"test.sav\" 1 2\n8 100 2 15 7 25 4\n0 0 1 1 100 0\n","v2 exact previous bytes");
 auto alter=[&](const std::string&a,const std::string&b){auto changed=bytes;const auto at=changed.find(a);need(at!=std::string::npos,"food mutation target exists");changed.replace(at,a.size(),b);need(!decodeNativeSession(seed,changed,error),"invalid food format refused");};alter("FOOD meat 4 100","FOOD rice 4 100");alter("FOOD meat 4 100","FOOD meat 5 100");alter("FOOD meat 4 100","FOOD meat 4 101");alter("ENCITY 3","ENCITY 4");alter("7 25 4 32\n","7 25 4 31\n");
 auto poor=seed;poor.initialFunds=31;auto ps=startNativeSession(poor,3);reject(poor,ps,{NativeSessionAction::Hunter,0,4,4},"insufficient_funds");auto exact=seed;exact.initialFunds=32;auto es=startNativeSession(exact,3);apply(exact,es,NativeSessionAction::Hunter,4,4);need(es.funds==0,"exact hunter cost accepted");reject(seed,s,{NativeSessionAction::RoadPath,s.revision,4,4,4,6},"occupied_or_unsupported_terrain");
 std::cout<<"{\"food_authored_checks\":"<<checks-begin<<",\"passed\":true}\n";
}
void demolition(){
 const auto begin=checks;auto seed=fixture();seed.initialFunds=10000;seed.wellCost=25;seed.hunterCost=32;
 auto s=startNativeSession(seed,4);
 for(auto p:{std::pair<int,int>{1,2},{1,4},{5,3}})apply(seed,s,NativeSessionAction::Road,p.first,p.second);
 apply(seed,s,NativeSessionAction::House,2,2);apply(seed,s,NativeSessionAction::Well,2,4);apply(seed,s,NativeSessionAction::Hunter,4,4);
 apply(seed,s,NativeSessionAction::Resume);for(int n=0;n<4;++n)apply(seed,s,NativeSessionAction::Tick);apply(seed,s,NativeSessionAction::Pause);
 need(nativePopulation(s)==2&&s.foodStock==5,"v4 retains v3 food before demolition");
 const auto original=encodeNativeSession(seed,s);const auto plan=planNativeDemolition(seed,s,4);
 need(plan.candidate&&plan.object.residents==2&&plan.before.wateredHouses==1&&plan.after.wateredHouses==0,"house preview shows departure and footprint");
 need(encodeNativeSession(seed,s)==original,"demolition planning cannot change state");
 for(int y=2;y<4;++y)for(int x=2;x<4;++x)need(nativeObjectAt(s,x,y)==std::optional<uint64_t>{4},"all footprint cells select stable identity");
 need(!nativeObjectAt(s,INT64_MIN,0)&&!nativeObjectAt(s,INT64_MAX,INT64_MAX)&&!nativeObjectAt(s,7,7),"invalid or empty cells no identity");
 auto cmd=[&](const NativeSessionState& v,uint64_t id){NativeSessionCommand c{NativeSessionAction::Demolish,v.revision};c.objectId=id;return c;};
 auto house=s;const auto oldFunds=house.funds;need(transactNativeSession(seed,house,cmd(house,4)).committed,"demolish populated house");
 need(house.funds==oldFunds&&nativePopulation(house)==0&&house.foodStock==5&&house.foodProduced==8&&house.foodConsumed==3&&house.departedSinceMeal==2,"no refund no food loss and historical meal preserved");
 need(house.retired.size()==1&&house.retired[0].id==4&&house.nextId==7,"identity retained without reuse");
 reject(seed,house,cmd(house,4),"native_object_not_found");auto stale=cmd(s,5);reject(seed,house,stale,"stale_command");
 apply(seed,house,NativeSessionAction::House,2,2);need(house.objects.back().id==7&&house.funds==oldFunds-seed.houseCost,"rebuild pays new price new identity");
 std::string error;const auto bytes=encodeNativeSession(seed,house);auto decoded=decodeNativeSession(seed,bytes,error);need(decoded&&encodeNativeSession(seed,*decoded)==bytes,"v4 archive and meal departure exact roundtrip");
 apply(seed,house,NativeSessionAction::Resume);apply(seed,*decoded,NativeSessionAction::Resume);for(int n=0;n<2;++n){apply(seed,house,NativeSessionAction::Tick);apply(seed,*decoded,NativeSessionAction::Tick);}
 need(encodeNativeSession(seed,house)==encodeNativeSession(seed,*decoded)&&house.departedSinceMeal==0&&nativePopulation(house)==1,"next meal clears departures and deterministic reload continuation");
 reject(seed,house,cmd(house,7),"pause_before_demolition");
 for(uint64_t id:{1ULL,2ULL,3ULL,5ULL,6ULL}){
  auto one=s;const auto p=planNativeDemolition(seed,one,id);need(p.candidate.has_value(),"every own kind deletable");
  need(transactNativeSession(seed,one,cmd(one,id)).committed&&one.funds==s.funds&&one.foodStock==s.foodStock,"every kind atomic no refund preserving stock");
  if(id==1)need(p.before.connectedHouses==1&&p.after.connectedHouses==0,"road removal disconnects occupied house without eviction");
  if(id==2||id==5)need(p.before.wells==1&&p.after.wells==0&&p.after.wateredHouses==0,"road or well loss disables water");
  if(id==3||id==6)need(p.before.hunters==1&&p.after.hunters==0,"road or hunter loss disables production");
  const auto pop=nativePopulation(one);apply(seed,one,NativeSessionAction::Resume);for(int n=0;n<4;++n)apply(seed,one,NativeSessionAction::Tick);
  if(id!=3&&id!=6)need(nativePopulation(one)==pop,"disconnection or missing water stops arrivals without eviction");
  else need(one.foodProduced==s.foodProduced,"no producer or adjacency no further production");
 }
 for(auto order:{std::vector<uint64_t>{1,2,3,4,5,6},std::vector<uint64_t>{6,5,4,3,2,1},std::vector<uint64_t>{4,2,5,3,6,1}}){
  auto empty=s;for(auto id:order){need(transactNativeSession(seed,empty,cmd(empty,id)).committed,"remove arbitrary stable-id order");need(validateNativeSession(seed,empty).empty(),"archive valid after every removal");}
  need(empty.objects.empty()&&empty.retired.size()==6&&empty.funds==s.funds&&empty.foodStock==s.foodStock,"empty city retains expenditure and food");
  auto back=decodeNativeSession(seed,encodeNativeSession(seed,empty),error);need(back&&back->objects.empty(),"empty demolished city reload");
 }
 auto rebuilt=s;for(int n=0;n<32;++n){const auto id=n?rebuilt.objects.back().id:6;need(transactNativeSession(seed,rebuilt,cmd(rebuilt,id)).committed,"repeated retire");apply(seed,rebuilt,NativeSessionAction::Hunter,4,4);need(rebuilt.objects.back().id==uint64_t(7+n)&&rebuilt.retired.size()==size_t(n+1),"never reuse identity after rebuild");need(rebuilt.funds==s.funds-int64_t(n+1)*seed.hunterCost,"rebuild cumulative original price ledger");}
 auto bad=*plan.candidate;bad.retired[0].id=5;need(!validateNativeSession(seed,bad).empty(),"duplicate retired/live id rejected");bad=*plan.candidate;bad.retired[0].kind=NativeObjectKind::Road;need(!validateNativeSession(seed,bad).empty(),"retired cost kind mismatch rejected");bad=*plan.candidate;bad.departedSinceMeal=0;need(!validateNativeSession(seed,bad).empty(),"lost historical meal departure rejected");bad=*plan.candidate;bad.funds++;need(!validateNativeSession(seed,bad).empty(),"refund outside rule rejected");
 auto limited=startNativeSession(seed,4);limited.retired.assign(3999,{});for(size_t n=0;n<limited.retired.size();++n)limited.retired[n]={n+1,NativeObjectKind::Road};limited.nextId=4000;limited.funds=seed.initialFunds-3999*seed.roadCost;
 need(validateNativeSession(seed,limited).empty(),"full lifetime archive valid");reject(seed,limited,{NativeSessionAction::Road,0,0,0},"object_limit");reject(seed,limited,{NativeSessionAction::RoadPath,0,0,0,0,0},"object_limit");
 for(unsigned v:{1u,2u,3u}){auto old=startNativeSession(seed,v);const auto oldBytes=encodeNativeSession(seed,old);reject(seed,old,cmd(old,1),"demolition_requires_native_v4");need(encodeNativeSession(seed,old)==oldBytes,"older format unchanged by unsupported operation");old.retired.push_back({1,NativeObjectKind::Road});old.nextId=2;old.funds-=seed.roadCost;need(validateNativeSession(seed,old)=="demolition_requires_native_v4","older state cannot carry retirement metadata");}
 auto protectedSeed=seed;protectedSeed.sourceRoad[0]=1;protectedSeed.buildable[0]=0;protectedSeed.buildable[1]=0;auto protectedCity=startNativeSession(protectedSeed,4);
 need(!nativeObjectAt(protectedCity,0,0)&&!nativeObjectAt(protectedCity,1,0),"original road and object never selectable");reject(protectedSeed,protectedCity,cmd(protectedCity,0),"native_object_not_found");reject(protectedSeed,protectedCity,cmd(protectedCity,UINT64_MAX),"native_object_not_found");
 for(auto text:{bytes+"extra",bytes.substr(0,bytes.find("RETIRED")),std::string("ENCITY 4 original-data-native-v3\n")})need(!decodeNativeSession(seed,text,error),"malformed retirement formats refused");
 std::cout<<"{\"demolition_authored_checks\":"<<checks-begin<<",\"passed\":true}\n";
}
void tax(){
 const auto begin=checks;auto seed=fixture();seed.initialFunds=79;seed.wellCost=25;seed.hunterCost=32;
 auto city=startNativeSession(seed,5);for(auto p:{std::pair<int,int>{1,2},{1,4},{5,3}})apply(seed,city,NativeSessionAction::Road,p.first,p.second);
 apply(seed,city,NativeSessionAction::House,2,2);apply(seed,city,NativeSessionAction::Well,2,4);apply(seed,city,NativeSessionAction::Hunter,4,4);need(city.funds==1,"income scenario has one gold left after original prices");
 reject(seed,city,{NativeSessionAction::Road,city.revision,7,7},"insufficient_funds");reject(seed,city,{NativeSessionAction::Tick,city.revision},"paused");
 auto p=planNativeTaxCycle(seed,city);need(p.error.empty()&&p.cycle.income==0&&p.cycle.emptyHouses==1,"empty houses cannot pay taxes");
 apply(seed,city,NativeSessionAction::Resume);for(int n=0;n<3;++n)apply(seed,city,NativeSessionAction::Tick);need(city.taxCycles==0&&city.taxReceived==0&&city.funds==1,"no income before full tax period");
 const auto pre=encodeNativeSession(seed,city);p=planNativeTaxCycle(seed,city);need(p.cycle.eligible==1&&p.cycle.income==1&&encodeNativeSession(seed,city)==pre,"pure tax proposal uses pre-tick population");
 std::string error;auto restored=decodeNativeSession(seed,pre,error);need(restored.has_value(),"pre-boundary checkpoint loads");apply(seed,city,NativeSessionAction::Tick);apply(seed,*restored,NativeSessionAction::Tick);need(encodeNativeSession(seed,city)==encodeNativeSession(seed,*restored),"reload crossing period same receipt once");
 need(city.funds==2&&city.taxReceived==1&&city.taxCycles==1&&city.lastTax.eligible==1&&nativePopulation(city)==2,"tax occurs before same-tick new arrival");
 apply(seed,city,NativeSessionAction::Road,7,7);need(city.funds==0,"earned tax unlocks previously unaffordable construction");
 const auto paid=encodeNativeSession(seed,city);restored=decodeNativeSession(seed,paid,error);need(restored&&encodeNativeSession(seed,*restored)==paid,"load after payment never gives another reward");
 uint64_t manual=city.taxReceived;const auto spent=seed.initialFunds+int64_t(city.taxReceived)-city.funds;
 for(unsigned n=0;n<200;++n){
  const auto oldPop=nativePopulation(city);if((city.ticks+1)%nativeTaxPeriod==0)manual+=oldPop;
  apply(seed,city,NativeSessionAction::Tick);need(city.taxReceived==manual&&city.taxCycles==city.ticks/nativeTaxPeriod,"one tax assessment per completed period");need(city.funds==seed.initialFunds+int64_t(manual)-spent,"conserved funds across repeated meals and taxes");
  if(n%13==0){const auto bytes=encodeNativeSession(seed,city);auto r=decodeNativeSession(seed,bytes,error);need(r&&encodeNativeSession(seed,*r)==bytes,"mid-cycle deterministic checkpoint");}
 }
 apply(seed,city,NativeSessionAction::Pause);const auto paused=encodeNativeSession(seed,city);for(int n=0;n<4;++n)reject(seed,city,{NativeSessionAction::Tick,city.revision},"paused");need(encodeNativeSession(seed,city)==paused,"pause cannot accumulate tax");
 NativeSessionCommand remove{NativeSessionAction::Demolish,city.revision};remove.objectId=1;need(transactNativeSession(seed,city,remove).committed,"remove sole taxable-house road");p=planNativeTaxCycle(seed,city);need(p.cycle.income==0&&p.cycle.eligible==0&&p.cycle.disconnected==nativePopulation(city),"road loss immediately removes tax eligibility");
 const auto income=city.taxReceived;apply(seed,city,NativeSessionAction::Resume);for(int n=0;n<4;++n)apply(seed,city,NativeSessionAction::Tick);need(city.taxReceived==income&&city.lastTax.income==0&&city.lastTax.disconnected>0,"disconnected residents produce explicit zero-income receipt");apply(seed,city,NativeSessionAction::Pause);
 apply(seed,city,NativeSessionAction::Road,1,2);p=planNativeTaxCycle(seed,city);need(p.cycle.income==nativePopulation(city),"road reconstruction restores tax basis");
 remove={NativeSessionAction::Demolish,city.revision};remove.objectId=4;need(transactNativeSession(seed,city,remove).committed,"house demolition removes taxpayer");need(planNativeTaxCycle(seed,city).cycle.income==0,"removed residents cannot be taxed");
 // Construct a valid near-cap ledger, then test real arithmetic rejection before meal publication.
 auto bigSeed=seed;bigSeed.initialFunds=INT32_MAX;auto big=*restored;big.funds=INT32_MAX;big.ticks=40003;big.revision=50000;big.taxCycles=big.ticks/4;big.taxReceived=uint64_t(spent);big.lastTax={1,0,0,1};need(validateNativeSession(bigSeed,big).empty(),"valid treasury exactly at cap");
 reject(bigSeed,big,{NativeSessionAction::Tick,big.revision},"tax_overflow");
 big.funds=INT32_MAX-2;big.taxReceived=uint64_t(spent)-2;need(validateNativeSession(bigSeed,big).empty(),"two gold headroom valid");apply(bigSeed,big,NativeSessionAction::Tick);need(big.funds==INT32_MAX&&big.lastTax.income==2,"exact treasury cap accepted");
 for(int kind=0;kind<5;++kind){auto bad=big;if(kind==0)bad.taxReceived=UINT64_MAX;if(kind==1)++bad.taxCycles;if(kind==2)++bad.lastTax.income;if(kind==3)bad.lastTax.eligible=UINT32_MAX;if(kind==4)++bad.funds;need(encodeNativeSession(bigSeed,bad).empty(),"tax cycle income/cap inconsistencies refuse encoding");}
 const auto good=encodeNativeSession(bigSeed,big);for(const auto& pair:{std::pair<std::string,std::string>{"TAX 4 1","TAX 3 1"},{"TAX 4 1","TAX 4 2"},{"TAX 4 1","OTHER 4 1"}}){auto bad=good;const auto at=bad.find(pair.first);need(at!=std::string::npos,"tax record present");bad.replace(at,pair.first.size(),pair.second);need(!decodeNativeSession(bigSeed,bad,error),"unknown tax rule refused");}
 for(unsigned version:{1u,2u,3u,4u}){auto old=startNativeSession(seed,version);need(planNativeTaxCycle(seed,old).error=="tax_requires_native_v5","older rules cannot assess tax");old.taxReceived=1;need(validateNativeSession(seed,old)=="tax_requires_native_v5","old versions reject hidden income");}
 // Two populated houses removed between meals preserve the prior receipt until the next meal.
 auto multiSeed=seed;multiSeed.initialFunds=10000;auto multi=startNativeSession(multiSeed,5);
 for(auto q:{std::pair<int,int>{1,2},{1,4},{5,3},{4,2}})apply(multiSeed,multi,NativeSessionAction::Road,q.first,q.second);
 apply(multiSeed,multi,NativeSessionAction::House,2,2);apply(multiSeed,multi,NativeSessionAction::Well,2,4);apply(multiSeed,multi,NativeSessionAction::Hunter,4,4);apply(multiSeed,multi,NativeSessionAction::House,4,0);
 apply(multiSeed,multi,NativeSessionAction::Resume);for(int n=0;n<4;++n)apply(multiSeed,multi,NativeSessionAction::Tick);apply(multiSeed,multi,NativeSessionAction::Pause);need(nativePopulation(multi)==4,"two inhabited houses before removal");
 for(auto id:{5ULL,8ULL}){remove={NativeSessionAction::Demolish,multi.revision};remove.objectId=id;need(transactNativeSession(multiSeed,multi,remove).committed,"consecutive populated-house demolition");need(uint64_t(multi.lastFood.demand)+multi.lastFood.arrivals==nativePopulation(multi)+multi.departedSinceMeal,"departures reconcile historical meal after each removal");}
 need(multi.departedSinceMeal==4&&nativePopulation(multi)==0,"all four residents accounted for");apply(multiSeed,multi,NativeSessionAction::Resume);apply(multiSeed,multi,NativeSessionAction::Tick);need(multi.departedSinceMeal==4,"odd tick preserves departures");apply(multiSeed,multi,NativeSessionAction::Tick);need(multi.departedSinceMeal==0&&multi.lastFood.demand==0&&multi.foodProduced-multi.foodConsumed==multi.foodStock,"next meal clears departures without food loss or duplicate consumers");
 std::cout<<"{\"tax_authored_checks\":"<<checks-begin<<",\"passed\":true}\n";
}
void checkpointHelpers(){
 const auto begin=checks;auto seed=fixture();seed.wellCost=25;seed.hunterCost=32;
 for(unsigned version=1;version<=5;++version){
  auto state=startNativeSession(seed,version);const auto original=encodeNativeSession(seed,state),content=nativeSessionContent(seed,state);
  const auto ref=nativeSessionReference(original);need(ref&&ref->source==seed.source&&ref->version==version,"checkpoint identifies original seed and exact version");
  apply(seed,state,NativeSessionAction::Resume);need(nativeSessionContent(seed,state)==content,"resume alone not unsaved content");apply(seed,state,NativeSessionAction::Pause);need(nativeSessionContent(seed,state)==content,"pause alone not unsaved content");need(encodeNativeSession(seed,state)!=original,"dirty comparison does not rewrite real revision history");
  apply(seed,state,NativeSessionAction::Road,0,0);need(nativeSessionContent(seed,state)!=content,"construction creates unsaved content");const auto built=nativeSessionContent(seed,state);apply(seed,state,NativeSessionAction::Resume);apply(seed,state,NativeSessionAction::Tick);need(nativeSessionContent(seed,state)!=built,"actual progression creates unsaved content");
  auto bad=state;++bad.funds;need(nativeSessionContent(seed,bad).empty(),"invalid city never considered saved");
 }
 for(const auto& path:{"/absolute.sav","../escape.sav","folder/../escape.sav","./scene.sav","scene.map",""}){
  std::ostringstream text;text<<"ENCITY 5 original-data-native-v5\n"<<std::quoted(path)<<" 1 2\n";need(!nativeSessionReference(text.str()),"unsafe or unsupported source reference rejects");}
 for(const auto& bytes:{std::string("ENCITY 7 original-data-native-v7\n\"test.sav\" 1 2"),std::string("ENCITY 5 original-data-native-v4\n\"test.sav\" 1 2"),std::string(512001,'x'),std::string("ENCITY\0 5",9)})need(!nativeSessionReference(bytes),"bad version/header is not a continuation candidate");
 const auto state=startNativeSession(seed,5);const auto bytes=encodeNativeSession(seed,state)+"trailing junk";need(nativeSessionReference(bytes).has_value(),"header inspection is not full acceptance");std::string error;need(!decodeNativeSession(seed,bytes,error),"full load rejects invalid body after valid reference");
 std::cout<<"{\"checkpoint_flow_checks\":"<<checks-begin<<",\"passed\":true}\n";
}
void presentation(){
 const auto start=checks;
 need(nativePresentationInterval(1)==1&&nativePresentationInterval(2)==.5&&nativePresentationInterval(4)==.25,"presentation speeds do not change tick rules");
 need(nativePresentationInterval(0)==1&&nativePresentationInterval(99)==1,"unknown presentation speed stays normal");
 for(unsigned v=1;v<=5;++v)for(unsigned tool=0;tool<=7;++tool){const bool expect=tool<=5&&(tool!=3||v>=2)&&(tool!=4||v>=3)&&(tool!=5||v>=4);need(nativeToolAvailable(tool,v)==expect,"old version palette availability");}
 for(unsigned v=1;v<=5;++v)for(unsigned category=0;category<6;++category)need(nativeCategoryAvailable(category,v)==(category<4&&(category!=1||v>=3)&&(category!=2||v>=2)),"old version category availability");
 need(nativeToolCategory(2)==0&&nativeToolCategory(4)==1&&nativeToolCategory(3)==2&&nativeToolCategory(0)==3&&nativeToolCategory(1)==3&&nativeToolCategory(5)==3,"category contract");
 auto seed=fixture();seed.initialFunds=2000;seed.wellCost=25;seed.hunterCost=32;auto city=startNativeSession(seed,5);
 apply(seed,city,NativeSessionAction::Road,0,1);apply(seed,city,NativeSessionAction::House,1,1);apply(seed,city,NativeSessionAction::Well,1,3);apply(seed,city,NativeSessionAction::Hunter,4,1);
 const auto before=encodeNativeSession(seed,city);
 for(int n=0;n<3;++n){need(nativeObjectDescription(seed,city,1).find("道路 #1")!=std::string::npos,"road identity");need(nativeObjectDescription(seed,city,2).find("邻路")!=std::string::npos,"house road service shown");need(nativeObjectDescription(seed,city,3).find("供水停用")!=std::string::npos,"disconnected well is inactive");need(nativeObjectDescription(seed,city,4).find("生产停用")!=std::string::npos,"disconnected hunter inactive");need(nativeObjectDescription(seed,city,999).find("原始地表只读")!=std::string::npos,"unknown selection not guessed");}
 need(encodeNativeSession(seed,city)==before,"inspection and speed planning never mutate state");
 apply(seed,city,NativeSessionAction::Road,0,3);need(nativeObjectDescription(seed,city,3).find("供水启用")!=std::string::npos,"well connection update");need(nativeObjectDescription(seed,city,2).find("有水")!=std::string::npos,"house water update");
 NativeSessionCommand remove{NativeSessionAction::Demolish,city.revision};remove.objectId=2;need(transactNativeSession(seed,city,remove).committed,"remove inspected object");need(nativeObjectDescription(seed,city,2).find("点击自建建筑")!=std::string::npos,"retired selection cannot describe reused identity");
 for(unsigned version=1;version<=3;++version){auto old=startNativeSession(seed,version);apply(seed,old,NativeSessionAction::House,1,1);need(nativeObjectDescription(seed,old,1).find("旧版本不支持拆除")!=std::string::npos,"legacy inspection does not promise demolition");}
 std::cout<<"{\"presentation_checks\":"<<checks-start<<",\"passed\":true}\n";
}
void authored(){
 presentation();
 roads();water();food();demolition();tax();checkpointHelpers();
 auto seed=fixture();auto s=startNativeSession(seed);need(validateNativeSession(seed,s).empty(),"fresh state");
 reject(seed,s,{NativeSessionAction::Tick,0},"paused");reject(seed,s,{NativeSessionAction::House,0,-1,0},"outside_scene");
 apply(seed,s,NativeSessionAction::House,2,2);need(s.funds==85,"house cost once");
 reject(seed,s,{NativeSessionAction::House,s.revision,3,3},"occupied_or_unsupported_terrain");
 apply(seed,s,NativeSessionAction::Resume);for(int n=0;n<4;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==0,"disconnected empty house stays empty");
 apply(seed,s,NativeSessionAction::Road,1,2);need(s.funds==83,"road price once");
 for(int n=0;n<20;++n)apply(seed,s,NativeSessionAction::Tick);need(nativePopulation(s)==7&&s.funds==83,"bounded immigration, no invented income");
 apply(seed,s,NativeSessionAction::Pause);reject(seed,s,{NativeSessionAction::Tick,s.revision},"paused");
 reject(seed,s,{NativeSessionAction::Road,s.revision-1,0,0},"stale_command");
 std::string error;const auto encoded=encodeNativeSession(seed,s);const auto restored=decodeNativeSession(seed,encoded,error);need(restored&&encodeNativeSession(seed,*restored)==encoded,"exact native save reload");
 auto continued=*restored;apply(seed,s,NativeSessionAction::Resume);apply(seed,continued,NativeSessionAction::Resume);apply(seed,s,NativeSessionAction::Tick);apply(seed,continued,NativeSessionAction::Tick);need(encodeNativeSession(seed,s)==encodeNativeSession(seed,continued),"reload next tick identical");
 for(auto bad:{encoded+"garbage",std::string("ENCITY 2 original-data-native-v1\n"),std::string(600000,'x')})need(!decodeNativeSession(seed,bad,error),"malformed/newer formats reject");
 auto other=seed;++other.sourceRevision;need(!decodeNativeSession(other,encoded,error),"changed original source rejected");other=seed;++other.modelRevision;need(!decodeNativeSession(other,encoded,error),"changed model rejected");
 auto bad=s;bad.funds++;need(encodeNativeSession(seed,bad).empty(),"unaccounted credit rejected");bad=s;bad.objects[0].id=2;need(encodeNativeSession(seed,bad).empty(),"duplicate identity rejected");
 bad=s;bad.objects[0].residents=8;need(encodeNativeSession(seed,bad).empty(),"capacity overflow rejected");bad=s;bad.revision=UINT64_MAX;need(encodeNativeSession(seed,bad).empty(),"counter overflow rejected");
 auto poor=seed;poor.initialFunds=1;auto ps=startNativeSession(poor);reject(poor,ps,{NativeSessionAction::Road,0,0,0},"insufficient_funds");
 auto blocked=seed;blocked.buildable[0]=0;auto bs=startNativeSession(blocked);reject(blocked,bs,{NativeSessionAction::House,0,0,0},"occupied_or_unsupported_terrain");
 reject(seed,s,{NativeSessionAction(99),s.revision},"unsupported_command");
 OriginalMapCamera camera;camera.centerX=4;camera.centerY=4;camera.width=640;camera.height=480;
 for(int y=0;y<8;++y)for(int x=0;x<8;++x){auto p=nativeSessionTile(camera,8,320+(x-y)*40,240+(x+y-8)*20);if(240+(x+y-8)*20>=0)need(p&&*p==std::pair<int,int>(x,y),"renderer inverse click");}
 need(!nativeSessionTile(camera,8,-1,0),"outside canvas ignored");
 // Shared geometry across pan, zoom, letterboxing, resize and Retina logical points.
 for(unsigned scale=1;scale<=4;++scale)for(const auto& center:{std::pair<double,double>{4,4},{3.25,4.75}}){
  camera.centerX=center.first;camera.centerY=center.second;camera.scale=scale;
  for(const auto& size:{std::pair<double,double>{640,480},{1000,480},{640,800},{920,680}}){
   const auto fit=nativeCanvasFit(size.first,size.second,camera.width,camera.height);
   for(int y=0;y<8;++y)for(int x=0;x<8;++x){const auto pixel=nativeSessionProject(camera,x,y);
    if(pixel.x<0||pixel.y<0||pixel.x>=camera.width||pixel.y>=camera.height)continue;
    const double vx=fit.x+pixel.x*fit.width/camera.width,vy=fit.y+pixel.y*fit.height/camera.height;
    for(double backing:{1.,2.}){const auto point=nativeCanvasImagePoint(fit,camera.width,camera.height,vx*backing/backing,vy*backing/backing);
     need(bool(point),"fitted view point within image");const auto tile=nativeSessionTile(camera,8,point->x,point->y);need(tile&&*tile==std::pair<int,int>{x,y},"resize/scale/Retina grid identity");}
   }
   if(fit.x>0)need(!nativeCanvasImagePoint(fit,camera.width,camera.height,fit.x-.01,size.second/2),"horizontal letterbox ignores click");
   if(fit.y>0)need(!nativeCanvasImagePoint(fit,camera.width,camera.height,size.first/2,fit.y-.01),"vertical letterbox ignores click");
  }
 }
 camera={4,4,640,480,1,false};
 auto point=nativeSessionProject(camera,4.49,4.49);need(nativeSessionTile(camera,8,point.x,point.y)==std::make_optional(std::pair<int,int>{4,4}),"inside diamond corner");
 point=nativeSessionProject(camera,4.5,4.5);need(nativeSessionTile(camera,8,point.x,point.y)==std::make_optional(std::pair<int,int>{5,5}),"half-open diamond boundary");
 const auto zoomed=nativeSessionZoom(camera,8,1,400,260);point=nativeSessionProject(zoomed,5.5,3.5);need(std::abs(point.x-400)<.001&&std::abs(point.y-260)<.001,"zoom projection remains defined");
 const auto anchorBefore=nativeSessionTile(camera,8,400,260),anchorAfter=nativeSessionTile(zoomed,8,400,260);need(anchorBefore==anchorAfter,"zoom anchors cursor cell");
 auto moved=nativeSessionPan(camera,8,80,40);need(moved.centerX==2&&moved.centerY==4,"pan inverses projected motion");
 need(nativeSessionPan(camera,8,1e300,1e300).centerX==0,"large pan clamps");
 camera.centerX=NAN;need(!nativeSessionTile(camera,8,320,240),"nonfinite camera rejects before integer cast");camera.centerX=4;camera.scale=0;need(!nativeSessionTile(camera,8,320,240),"zero scale rejects");camera.scale=1;
 need(!nativeSessionTile(camera,8,INFINITY,240),"nonfinite click rejects");need(nativeCanvasFit(0,480,640,480).width==0,"empty view rejects");
 const auto beforePreview=encodeNativeSession(seed,s);
 for(int n=0;n<3;++n){need(nativeSessionPlacementPreview(seed,s,NativeSessionAction::Road,0,0).committed,"legal preview");need(!nativeSessionPlacementPreview(seed,s,NativeSessionAction::House,2,2).committed,"overlap preview");need(!nativeSessionPlacementPreview(poor,ps,NativeSessionAction::Road,0,0).committed,"unaffordable preview");need(encodeNativeSession(seed,s)==beforePreview,"preview has no side effects");}
 std::cout<<"{\"authored_checks\":"<<checks<<",\"passed\":true}\n";
}
void real(int argc,char**argv){
 const auto env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
 need(argc==3,"real relative source argument");const auto root=ResourceRoot::fromAbsolute(env);std::string error;const auto source=loadNativeSessionSource(root,argv[2],error);need(bool(source),error.c_str());const auto& seed=source->seed;
 const auto choices=nativeSessionChoices(root,{argv[2],argv[2],"not-a-source.sav","../outside.sav"});need(choices.size()==1&&choices[0].source==argv[2]&&choices[0].side==seed.side,"eligible-source selector validates and deduplicates");

 auto s=startNativeSession(seed);bool found=false;unsigned x=0,y=0;
 for(unsigned yy=2;yy+3<seed.side&&!found;++yy)for(unsigned xx=2;xx+3<seed.side&&!found;++xx){bool good=true;
  for(unsigned dy=0;dy<3;++dy)for(unsigned dx=0;dx<3;++dx)good&=seed.buildable[size_t(yy+dy)*seed.side+xx+dx]!=0;
  if(good){x=xx+1;y=yy;found=true;}}
 need(found,"real source construction area");need(transactNativeSession(seed,s,{NativeSessionAction::RoadPath,s.revision,int64_t(x)-1,y,int64_t(x)-1,int64_t(y)+2}).committed,"real three-cell atomic road");apply(seed,s,NativeSessionAction::House,x,y);apply(seed,s,NativeSessionAction::Resume);
 for(unsigned n=0;n<seed.houseCapacity*2;++n)apply(seed,s,NativeSessionAction::Tick);apply(seed,s,NativeSessionAction::Pause);
 need(nativePopulation(s)==seed.houseCapacity,"real capacity reached");need(s.funds==seed.initialFunds-3*seed.roadCost-seed.houseCost,"real model debit");
 const auto projected=nativeSessionScene(*source,s);need(bool(projected),"native state projection");need(projected->buildings.footprints.size()==1&&projected->buildings.footprints[0].slot==4,"native identity preserved into shared renderer");
 const auto general=loadSG3(root.resolve("DATA/China_General.sg3")),terrain=loadSG3(root.resolve("DATA/China_Terrain.sg3")),elevation=loadSG3(root.resolve("DATA/China_Elevation.sg3"));
 const auto scene=composeOriginalSavedGroundScene(projected->terrain,projected->buildings,projected->ground,general,terrain,elevation);
 need(scene.accepted&&scene.placed.size()==1&&scene.rejectedRecords==0,"real original housing sprite binds");
 const auto bytes=encodeNativeSession(seed,s);const auto restored=decodeNativeSession(seed,bytes,error);need(restored&&encodeNativeSession(seed,*restored)==bytes,"real session serialization exact");
 // Additional v2 scenario uses only verified flat cells from this real source.
 auto w=startNativeSession(seed,2);bool area=false;unsigned wx=0,wy=0;
 for(unsigned yy=1;yy+4<seed.side&&!area;++yy)for(unsigned xx=1;xx+3<seed.side&&!area;++xx){bool good=true;for(unsigned dy=0;dy<4;++dy)for(unsigned dx=0;dx<3;++dx)good&=seed.buildable[size_t(yy+dy)*seed.side+xx+dx]!=0;if(good){wx=xx+1;wy=yy;area=true;}}
 need(area&&seed.wellCost>0,"real well model and placement area");apply(seed,w,NativeSessionAction::Road,wx-1,wy);apply(seed,w,NativeSessionAction::House,wx,wy);apply(seed,w,NativeSessionAction::Well,wx,wy+2);
 need(!nativeWellActive(seed,w.objects.back(),nativeRoads(seed,w)),"real disconnected well inactive");apply(seed,w,NativeSessionAction::Resume);for(int n=0;n<2;++n)apply(seed,w,NativeSessionAction::Tick);need(nativePopulation(w)==0,"real v2 dry house no growth");apply(seed,w,NativeSessionAction::Pause);
 need(transactNativeSession(seed,w,{NativeSessionAction::RoadPath,w.revision,wx-1,wy,wx-1,wy+2}).committed,"real road activates well");need(nativeHouseWatered(seed,w.objects[1],nativeWaterCoverage(seed,w)),"real full house water coverage");
 apply(seed,w,NativeSessionAction::Resume);for(int n=0;n<2;++n)apply(seed,w,NativeSessionAction::Tick);apply(seed,w,NativeSessionAction::Pause);need(nativePopulation(w)==1&&w.funds==seed.initialFunds-3*seed.roadCost-seed.houseCost-seed.wellCost,"real v2 immigration and prices");
 const auto wp=nativeSessionScene(*source,w);need(wp&&wp->buildings.namedWellRecords==1,"native well projection uses known original type");const auto ws=composeOriginalSavedGroundScene(wp->terrain,wp->buildings,wp->ground,general,terrain,elevation);need(ws.accepted&&ws.placed.size()==2&&ws.rejectedRecords==0,"real well and housing sprites bind2/2");
 const auto wb=encodeNativeSession(seed,w);auto wr=decodeNativeSession(seed,wb,error);need(wr&&encodeNativeSession(seed,*wr)==wb,"real v2 serialization exact");
 std::cout<<"{\"real_water_source\":"<<std::quoted(argv[2])<<",\"well_cost\":"<<seed.wellCost<<",\"well\":["<<wx<<','<<wy+2<<"],\"original_art_structure_bindings\":2,\"dry_population\":0,\"wet_population\":"<<nativePopulation(w)<<",\"funds\":"<<w.funds<<",\"roundtrip\":true}\n";
 auto foodCity=startNativeSession(seed,3);unsigned fx=0,fy=0;double best=1e30;bool foodArea=false;
 for(unsigned yy=1;yy+5<seed.side;++yy)for(unsigned xx=1;xx+5<seed.side;++xx){bool good=true;for(unsigned dy=0;dy<5;++dy)for(unsigned dx=0;dx<5;++dx)good&=seed.buildable[size_t(yy+dy)*seed.side+xx+dx]!=0;
  const double distance=std::abs(double(xx)-seed.side/2.)+std::abs(double(yy)-seed.side/2.);if(good&&distance<best){fx=xx+1;fy=yy;best=distance;foodArea=true;}}
 need(foodArea&&seed.hunterCost>0,"real producer price and flat construction site");
 need(transactNativeSession(seed,foodCity,{NativeSessionAction::RoadPath,foodCity.revision,fx-1,fy,fx-1,fy+4}).committed,"real food road vertical");need(transactNativeSession(seed,foodCity,{NativeSessionAction::RoadPath,foodCity.revision,fx-1,fy+4,fx+2,fy+4}).committed,"real food road horizontal");
 apply(seed,foodCity,NativeSessionAction::House,fx,fy);apply(seed,foodCity,NativeSessionAction::Well,fx,fy+2);apply(seed,foodCity,NativeSessionAction::Hunter,fx+2,fy+2);apply(seed,foodCity,NativeSessionAction::Resume);for(int n=0;n<4;++n)apply(seed,foodCity,NativeSessionAction::Tick);apply(seed,foodCity,NativeSessionAction::Pause);
 need(nativePopulation(foodCity)==2&&foodCity.foodProduced==8&&foodCity.foodConsumed==3&&foodCity.foodStock==5,"real v3 production consumption immigration");need(foodCity.funds==seed.initialFunds-8*seed.roadCost-seed.houseCost-seed.wellCost-seed.hunterCost,"real v3 exact construction ledger");
 const auto fp=nativeSessionScene(*source,foodCity);need(bool(fp),"real food scene projection");const auto fs=composeOriginalSavedGroundScene(fp->terrain,fp->buildings,fp->ground,general,terrain,elevation);need(fs.accepted&&fs.placed.size()==3&&fs.rejectedRecords==0,"original house well hunter bind3/3");
 const auto fb=encodeNativeSession(seed,foodCity);const auto fr=decodeNativeSession(seed,fb,error);need(fr&&encodeNativeSession(seed,*fr)==fb,"real food exact roundtrip");
 std::cout<<"{\"real_food_source\":"<<std::quoted(argv[2])<<",\"hunter_cost\":"<<seed.hunterCost<<",\"house\":["<<fx<<','<<fy<<"],\"well\":["<<fx<<','<<fy+2<<"],\"hunter\":["<<fx+2<<','<<fy+2<<"],\"original_art_bindings\":3,\"population\":"<<nativePopulation(foodCity)<<",\"food_produced\":"<<foodCity.foodProduced<<",\"food_consumed\":"<<foodCity.foodConsumed<<",\"stock\":"<<foodCity.foodStock<<",\"funds\":"<<foodCity.funds<<",\"roundtrip\":true}\n";
 auto rebuild=foodCity;rebuild.rulesVersion=4;const auto originalFunds=rebuild.funds;const auto stock=rebuild.foodStock;
 const auto originalIds=rebuild.objects;for(const auto& object:originalIds){NativeSessionCommand c{NativeSessionAction::Demolish,rebuild.revision};c.objectId=object.id;need(transactNativeSession(seed,rebuild,c).committed,"real owned deletion");}
 need(rebuild.objects.empty()&&rebuild.retired.size()==11&&rebuild.funds==originalFunds&&rebuild.foodStock==stock,"real no-refund food-preserving removal");
 const auto removed=nativeSessionScene(*source,rebuild);need(removed&&removed->terrain.flags==source->save.terrain.flags&&removed->ground.owner==source->save.layers.ground->owner,"real deletion restores exact original terrain/owner source");
 apply(seed,rebuild,NativeSessionAction::House,fx,fy);need(rebuild.objects.back().id==12&&rebuild.funds==originalFunds-seed.houseCost,"real rebuild stable new id and debit");
 const auto rebuilt=nativeSessionScene(*source,rebuild);need(bool(rebuilt),"real rebuilding projects");const auto rs=composeOriginalSavedGroundScene(rebuilt->terrain,rebuilt->buildings,rebuilt->ground,general,terrain,elevation);need(rs.accepted&&rs.placed.size()==1&&rs.rejectedRecords==0,"original art renders rebuilt id with gaps");
 const auto rb=encodeNativeSession(seed,rebuild);const auto rr=decodeNativeSession(seed,rb,error);need(rr&&encodeNativeSession(seed,*rr)==rb,"real v4 rebuilt archive roundtrip");
 std::cout<<"{\"real_demolition_source\":"<<std::quoted(argv[2])<<",\"retired\":11,\"rebuilt_id\":12,\"source_terrain_restored\":true,\"stock_preserved\":true,\"funds\":"<<rebuild.funds<<",\"roundtrip\":true}\n";
 auto taxCity=startNativeSession(seed,5);need(transactNativeSession(seed,taxCity,{NativeSessionAction::RoadPath,taxCity.revision,fx-1,fy,fx-1,fy+4}).committed,"real taxable city road");need(transactNativeSession(seed,taxCity,{NativeSessionAction::RoadPath,taxCity.revision,fx-1,fy+4,fx+2,fy+4}).committed,"real producer road");apply(seed,taxCity,NativeSessionAction::House,fx,fy);apply(seed,taxCity,NativeSessionAction::Well,fx,fy+2);apply(seed,taxCity,NativeSessionAction::Hunter,fx+2,fy+2);apply(seed,taxCity,NativeSessionAction::Resume);for(int n=0;n<7;++n)apply(seed,taxCity,NativeSessionAction::Tick);
 need(taxCity.taxCycles==1&&taxCity.taxReceived==1&&taxCity.funds==seed.initialFunds-8*seed.roadCost-seed.houseCost-seed.wellCost-seed.hunterCost+1,"real original construction costs with one native tax");const auto tb=encodeNativeSession(seed,taxCity);auto tr=decodeNativeSession(seed,tb,error);need(tr.has_value(),"real tax before period reload");apply(seed,taxCity,NativeSessionAction::Tick);apply(seed,*tr,NativeSessionAction::Tick);need(taxCity.taxCycles==2&&taxCity.taxReceived==4&&encodeNativeSession(seed,taxCity)==encodeNativeSession(seed,*tr),"real cross-period tax exactly once");apply(seed,taxCity,NativeSessionAction::Pause);
 const auto taxScene=nativeSessionScene(*source,taxCity);need(taxScene.has_value(),"real v5 projection");const auto ts=composeOriginalSavedGroundScene(taxScene->terrain,taxScene->buildings,taxScene->ground,general,terrain,elevation);need(ts.accepted&&ts.placed.size()==3&&ts.rejectedRecords==0,"real v5 original assets retained");
 OriginalBuildingFootprint office;office.type=125;office.size=2;need(!originalBuildingBaseSelection(office),"tax office has no supported original image binding");
 std::cout<<"{\"real_tax_source\":"<<std::quoted(argv[2])<<",\"cycles\":2,\"tax_received\":4,\"funds\":"<<taxCity.funds<<",\"original_art_bindings\":3,\"cross_period_roundtrip\":true}\n";
 char tmp[]="/tmp/emperor-native-session-XXXXXX";const char* dir=mkdtemp(tmp);need(dir,"test output directory");const auto target=(std::filesystem::path(dir)/"test.encity").string();
 need(writeNativeSessionNew(target,env,bytes,error),"atomic new session save");need(!writeNativeSessionNew(target,env,bytes,error),"no overwrite existing session");const auto loaded=readNativeSessionFile(target,env,error);need(loaded&&*loaded==bytes,"independent file readback exact");
 need(!writeNativeSessionNew(root.configuredDirectory()+"/blocked.encity",env,bytes,error),"original root write rejected before creating file");
 std::cout<<"{\"source\":"<<std::quoted(argv[2])<<",\"initial_funds\":"<<seed.initialFunds<<",\"road_cost\":"<<seed.roadCost<<",\"house_cost\":"<<seed.houseCost<<",\"capacity\":"<<seed.houseCapacity<<",\"funds_after\":"<<s.funds<<",\"population\":"<<nativePopulation(s)<<",\"road\":["<<x-1<<','<<y<<"],\"house\":["<<x<<','<<y<<"],\"roundtrip\":true,\"original_art_house_bindings\":1,\"atomic_road_cells\":3}\n";
}
uint64_t residentBytes(){
#if defined(__APPLE__)
 mach_task_basic_info_data_t info{};mach_msg_type_number_t count=MACH_TASK_BASIC_INFO_COUNT;
 if(task_info(mach_task_self(),MACH_TASK_BASIC_INFO,reinterpret_cast<task_info_t>(&info),&count)==KERN_SUCCESS)return info.resident_size;
#endif
 return 0;
}
void soak(int argc,char**argv){
 const auto env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
 need(argc==4,"soak source and tick count required");char* end=nullptr;const auto total=std::strtoul(argv[3],&end,10);need(end&&!*end&&total>=1000&&total<=100000&&total%1000==0,"bounded soak tick count");
 const auto root=ResourceRoot::fromAbsolute(env);std::string error;const auto source=loadNativeSessionSource(root,argv[2],error);need(bool(source),error.c_str());const auto& seed=source->seed;
 unsigned fx=0,fy=0;bool found=false;for(unsigned y=0;y+5<=seed.side&&!found;++y)for(unsigned x=1;x+4<=seed.side&&!found;++x){bool flat=true;for(unsigned dy=0;dy<5;++dy)for(int dx=-1;dx<4;++dx)flat&=seed.buildable[size_t(y+dy)*seed.side+x+dx]!=0;if(flat){fx=x;fy=y;found=true;}}
 need(found,"real 5x5 service construction area");auto city=startNativeSession(seed,5),reloaded=city;int64_t spent=0;uint64_t income=0;bool roadPresent=true;uint64_t houseId=4,roadId=1;size_t saves=0,demolitions=0;
 auto both=[&](NativeSessionAction action,int x=0,int y=0,uint64_t id=0){NativeSessionCommand command{action,city.revision,x,y};command.objectId=id;const auto a=transactNativeSession(seed,city,command),b=transactNativeSession(seed,reloaded,command);need(a.committed&&b.committed,"soak command both histories commit");need(encodeNativeSession(seed,city)==encodeNativeSession(seed,reloaded),"uninterrupted and repeatedly loaded histories exact");};
 for(const auto& cell:{std::pair<unsigned,unsigned>{fx-1,fy},{fx-1,fy+2},{fx+3,fy+1}})both(NativeSessionAction::Road,cell.first,cell.second);
 both(NativeSessionAction::House,fx,fy);both(NativeSessionAction::Well,fx,fy+2);both(NativeSessionAction::Hunter,fx+2,fy+2);spent=3*seed.roadCost+seed.houseCost+seed.wellCost+seed.hunterCost;both(NativeSessionAction::Resume);
 char temporary[]="/tmp/emperor-native-soak-XXXXXX";const char* directory=mkdtemp(temporary);need(directory,"separate soak save directory");const auto start=std::chrono::steady_clock::now();
 for(unsigned long n=1;n<=total;++n){
  if(n%4==0&&roadPresent)income+=nativePopulation(city);
  both(NativeSessionAction::Tick);need(city.taxReceived==income&&city.funds==seed.initialFunds+int64_t(income)-spent,"independent expected tax and spend ledger");need(city.foodProduced-city.foodConsumed==city.foodStock,"long-run food conservation");
  if(n%400==100)both(NativeSessionAction::HuntingPause);if(n%400==140)both(NativeSessionAction::HuntingResume);
  if(n%1000==600){both(NativeSessionAction::Pause);both(NativeSessionAction::Demolish,0,0,houseId);++demolitions;houseId=city.nextId;both(NativeSessionAction::House,fx,fy);spent+=seed.houseCost;both(NativeSessionAction::Resume);}
  if(n%1000==800){both(NativeSessionAction::Pause);both(NativeSessionAction::Demolish,0,0,roadId);++demolitions;roadPresent=false;both(NativeSessionAction::Resume);}
  if(n%1000==820){both(NativeSessionAction::Pause);roadId=city.nextId;both(NativeSessionAction::Road,fx-1,fy);spent+=seed.roadCost;roadPresent=true;both(NativeSessionAction::Resume);}
  if(n%100==0){both(NativeSessionAction::Pause);const auto bytes=encodeNativeSession(seed,city);reject(seed,city,{NativeSessionAction::Tick,city.revision},"paused");need(encodeNativeSession(seed,reloaded)==bytes,"paused state cannot progress");
   const auto path=(std::filesystem::path(directory)/(std::to_string(n)+".encity")).string();need(writeNativeSessionNew(path,env,bytes,error),"soak new-file save");need(!writeNativeSessionNew(path,env,bytes,error),"soak overwrite refused");const auto saved=readNativeSessionFile(path,env,error);need(saved&&*saved==bytes,"soak disk bytes exact");const auto restored=decodeNativeSession(seed,*saved,error);need(bool(restored),"soak decode committed city");reloaded=*restored;++saves;both(NativeSessionAction::Resume);}
  if(n%1000==0){const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();std::cout<<"{\"soak_progress\":"<<n<<",\"elapsed_ms\":"<<ms<<",\"rss_bytes\":"<<residentBytes()<<",\"live_objects\":"<<city.objects.size()<<",\"retired\":"<<city.retired.size()<<"}\n"<<std::flush;}
 }
 both(NativeSessionAction::Pause);need(city.ticks==total&&saves==total/100&&demolitions==2*total/1000,"complete soak denominator");
 std::cout<<"{\"real_soak_source\":"<<std::quoted(argv[2])<<",\"ticks\":"<<city.ticks<<",\"disk_roundtrips\":"<<saves<<",\"demolitions_rebuilt\":"<<demolitions<<",\"income\":"<<city.taxReceived<<",\"funds\":"<<city.funds<<",\"population\":"<<nativePopulation(city)<<",\"save_directory\":"<<std::quoted(directory)<<",\"passed\":true}\n";
}

}
int main(int argc,char**argv){try{if(argc==1)authored();else if(std::string(argv[1])=="--soak")soak(argc,argv);else real(argc,argv);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
