// SPDX-License-Identifier: MIT
// Fresh native cities built only through normal commands over read-only sources.
#include "native_house_runtime_source.hpp"
#include "native_session_feedback.hpp"
#include "native_session_presentation.hpp"
#include "native_session_io.hpp"
#include "native_minimap.hpp"
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const std::string& e){++checks;if(!v)throw std::runtime_error(e);}
struct Plan {int x=-1,y=-1;std::vector<int> path;};
Plan choose(const NativeSessionSource& src,const NativeSessionState& initial){
 const auto& seed=src.seed;const int n=int(seed.side);const auto b=nativeToStrict(seed,initial,src.runtime->strictTemplate,nativeStrictPersisted(initial));need(bool(b.city),b.refusal);
 const auto ep=*b.city->buildingContext->roadEndpoints;const auto base=*b.city->figures.cropBase;
 const auto distances=originalBuildingRoadDistances(b.city->figures.passability->cells,base+ep[0]+228*ep[1]);need(bool(distances),"initial entry distances");
 const auto& networks=b.city->figures.roadNetworks;const auto entryNetwork=networks.at(base+ep[0]+228*ep[1]);need(entryNetwork!=0,"source entry road network known");
 Plan best;size_t score=SIZE_MAX;
 for(int y=1;y+7<n;++y)for(int x=1;x+12<n;++x){
  bool flat=true;for(int dy=-1;dy<=7&&flat;++dy)for(int dx=-1;dx<=12;++dx)if(!seed.buildable[size_t(y+dy)*n+x+dx]){flat=false;break;}
  if(!flat)continue;
  std::vector<uint8_t> reserved(size_t(n)*n);for(const auto& p:{std::pair<int,int>{0,1},{0,4},{3,4},{6,4},{3,1},{9,4}})
   for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx)reserved[size_t(y+p.second+dy)*n+x+p.first+dx]=1;
  const int start=y*n+x;std::deque<int> q{start};std::vector<int> prior(size_t(n)*n,-1);prior[start]=start;int end=-1;
  while(!q.empty()){const int at=q.front();q.pop_front();if(seed.sourceRoad[size_t(at)]&&networks.at(base+at%n+228*(at/n))==entryNetwork){end=at;break;}
   for(const auto& d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}){const int xx=at%n+d.first,yy=at/n+d.second;if(xx<0||yy<0||xx>=n||yy>=n)continue;const int next=yy*n+xx;
    if(prior[size_t(next)]>=0||reserved[size_t(next)]||(!seed.buildable[size_t(next)]&&!seed.sourceRoad[size_t(next)]))continue;prior[size_t(next)]=at;q.push_back(next);}}
  if(end<0)continue;std::vector<int> path;for(int at=end;;at=prior[size_t(at)]){path.push_back(at);if(at==start)break;}
  if(path.size()<4||path.size()>=score)continue;
  if((path.size()+70)*seed.roadCost+4*seed.houseCost+seed.wellCost+seed.inspectorCost>uint64_t(seed.initialFunds))continue;
  score=path.size();best={x,y,std::move(path)};
 }
 need(best.x>=0,"qualified fresh neighborhood with a real entry connection");return best;
}
void run(const ResourceRoot& root,const std::string& relative,const std::filesystem::path& output,bool planOnly){
 std::string e;auto src=loadNativeSessionSource(root,relative,e);need(bool(src),e);need(prepareNativeHouseRuntimeSource(root,*src,e),e);
 auto initial=startNativeHouseRuntime(src->seed,*src->runtime,e,NativeWellReturnPolicy::ObservedHomeRoad,true,true,true,true,true,true);need(bool(initial),e);auto s=*initial;
 need(nativeLatestRulesVersion==6&&nativePopulation(s)==0&&s.objects.empty(),"normal fresh city starts empty; default stays6");
 const auto& seed=src->seed;const auto& tpl=src->runtime->strictTemplate;const auto unchanged=encodeNativeSession(seed,s);
 const auto inspect=[&](const NativeSessionState& state){const auto before=encodeNativeSession(seed,state);auto q=queryNativeCityFeedback(seed,state,&tpl);need(q.error.empty(),q.error);need(q.entrance.required&&q.entrance.error.empty(),q.entrance.error);for(int k=0;k<3;++k){(void)queryNativeCityFeedback(seed,state,&tpl);for(const auto& o:state.objects)(void)nativeObjectDescription(seed,state,o.id,q);}need(encodeNativeSession(seed,state)==before,"entry queries preserve clock/RNG/queues/inventory/version");return q;};
 auto q=inspect(s);need(bool(q.entrance.entry)&&bool(q.entrance.exit),"source endpoints available");
 auto old=startNativeSession(seed,6);need(!queryNativeCityFeedback(seed,old,&tpl).entrance.required,"legacy adjacency policy unchanged");
 need(queryNativeCityFeedback(seed,s).entrance.error=="entrance_source_not_available","missing template explicit, no invented entrance");
 auto bad=tpl;bad.sourceRevision^=1;need(!queryNativeEntrance(seed,s,&bad).error.empty(),"changed source rejected");bad=tpl;bad.emptyCity.buildingContext->roadEndpoints->at(0)=-1;
 need(queryNativeEntrance(seed,s,&bad).error=="entrance_outside_native_view","out-of-view endpoint not clamped or guessed");need(queryNativeEntrance(seed,s,&tpl).error.empty(),"bad template copy did not mutate source");
 need(encodeNativeSession(seed,s)==unchanged,"read-only preparation preserves fresh city");
 const auto layout=choose(*src,s);const int n=int(seed.side),x=layout.x,y=layout.y;std::vector<std::tuple<std::string,int,int>> commands;
 const auto command=[&](NativeSessionState& state,NativeSessionAction a,int xx=0,int yy=0,uint64_t id=0){const auto r=transactNativeHouseRuntime(seed,state,{a,state.revision,xx,yy,0,0,id},*src->runtime);need(r.committed,"normal command: "+r.error);};
 const auto build=[&](const std::string& name,NativeSessionAction a,int xx,int yy){command(s,a,xx,yy);commands.emplace_back(name,xx,yy);};
 const auto road=[&](int xx,int yy){if(!nativeRoads(seed,s)[size_t(yy)*n+xx])build("road",NativeSessionAction::Road,xx,yy);};
 road(x,y);build("house",NativeSessionAction::House,x,y+1);const auto house=s.objects.back().id;q=inspect(s);
 need(q.find(house)->connected&&q.find(house)->entryRoadConnected==false,"local road is not continuous source road connectivity");
 need(nativeObjectDescription(seed,s,house,q).find("仅邻路")!=std::string::npos,"player sees precise missing entry link");
 for(int cell:layout.path)road(cell%n,cell/n);
 const size_t connectionEnd=commands.size();q=inspect(s);need(q.find(house)->entryReachable==true&&q.find(house)->entryRoadConnected==true,"normal road construction connects actual runtime entrance and road component");
 for(int dx=0;dx<12;++dx)for(int dy:{0,3,6})road(x+dx,y+dy);
 for(int dx:{2,5,8,11})for(int dy=1;dy<6;++dy)road(x+dx,y+dy);
 for(const auto& p:{std::pair<int,int>{0,4},{3,4},{6,4}})build("house",NativeSessionAction::House,x+p.first,y+p.second);
 build("well",NativeSessionAction::Well,x+3,y+1);build("inspector",NativeSessionAction::Inspector,x+9,y+4);q=inspect(s);
 need(q.entrance.houses.size()==4&&std::all_of(q.entrance.houses.begin(),q.entrance.houses.end(),[](const auto& i){return i.second;}),"all four real houses have entry reachability");
 auto disconnected=s;for(size_t k=2;k<connectionEnd;++k){const auto& [name,xx,yy]=commands[k];if(name!="road")continue;const auto id=nativeObjectAt(disconnected,xx,yy);if(id)command(disconnected,NativeSessionAction::Demolish,0,0,*id);}
 const auto cut=inspect(disconnected);need(cut.find(house)->connected&&cut.find(house)->entryRoadConnected==false,"road removal refreshes whole-network reason despite adjacent local roads");
 const auto overview=makeNativeTerrainOverview(src->save.terrain);need(bool(overview),"source overview");const auto mid=overview->project(x+5.5,y+3);
 std::filesystem::create_directories(output);std::ofstream plan(output/"plan.json");plan<<"{\"source\":"<<std::quoted(relative)<<",\"side\":"<<n<<",\"entry\":["<<q.entrance.entry->first<<","<<q.entrance.entry->second<<"],\"layout\":["<<x<<","<<y<<"],\"view\":["<<mid.x<<","<<mid.y<<"],\"connection_end\":"<<connectionEnd<<",\"commands\":[";
 for(size_t k=0;k<commands.size();++k){const auto& [name,xx,yy]=commands[k];if(k)plan<<',';plan<<"{\"action\":"<<std::quoted(name)<<",\"x\":"<<xx<<",\"y\":"<<yy<<'}';}plan<<"]}\n";plan.close();
 std::cout<<"{\"fresh_normal_commands\":"<<commands.size()<<",\"entry_connected_houses\":4,\"x\":"<<x<<",\"y\":"<<y<<",\"checks\":"<<checks<<"}"<<std::endl;
 if(planOnly)return;
 command(s,NativeSessionAction::Resume);bool arrived=false,watered=false,patrolled=false;uint64_t firstArrival=0,firstWater=0,firstPatrol=0;
 for(unsigned ticks=0;ticks<4000;++ticks){command(s,NativeSessionAction::Tick);
  if(!arrived&&nativePopulation(s)>0){arrived=true;firstArrival=s.ticks;}
  if(!watered&&std::any_of(s.objects.begin(),s.objects.end(),[](const auto& o){return o.house&&o.house->houseWater.value_or(0)>0;})){watered=true;firstWater=s.ticks;}
  if(!patrolled&&s.inspection->services>0){patrolled=true;firstPatrol=s.ticks;}
  if(s.ticks%500==0)std::cout<<"{\"natural_tick\":"<<s.ticks<<",\"population\":"<<nativePopulation(s)<<",\"water\":"<<watered<<",\"patrol\":"<<patrolled<<"}"<<std::endl;
  if(arrived&&watered&&patrolled)break;
 }
 need(arrived&&watered&&patrolled,"fresh normal-command city must receive actual residents/water/patrol");command(s,NativeSessionAction::Pause);q=inspect(s);
 const auto bytes=encodeNativeSession(seed,s);const auto path=(output/"fresh-played.encity").string();need(writeNativeSessionNew(path,root.configuredDirectory(),bytes,e),e);const auto read=readNativeSessionFile(path,root.configuredDirectory(),e);need(read&&*read==bytes,"normal played file exact read");const auto loaded=decodeNativeSession(seed,*read,e);need(bool(loaded),e);need(validateNativeHouseRuntimeReady(seed,*loaded,*src->runtime).empty(),"saved fresh city ready");need(encodeNativeSession(seed,*loaded)==bytes,"resume roundtrip unchanged");
 std::ofstream report(output/"result.json");report<<"{\"passed\":true,\"checks\":"<<checks<<",\"ticks\":"<<s.ticks<<",\"first_arrival\":"<<firstArrival<<",\"first_water\":"<<firstWater<<",\"first_patrol\":"<<firstPatrol<<",\"population\":"<<nativePopulation(s)<<",\"services\":"<<s.inspection->services<<",\"fixture_loaded\":false,\"original_game_run\":false}\n";std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"ticks\":"<<s.ticks<<"}"<<std::endl;
}
}
int main(int argc,char**argv){try{const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}need(argc==3||argc==4,"source and unique output required");run(ResourceRoot::fromAbsolute(env),argv[1],argv[2],argc==4&&std::string(argv[3])=="--plan-only");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
