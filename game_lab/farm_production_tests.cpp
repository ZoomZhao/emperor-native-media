#include "game_lab/city_state_io.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using game_lab::CitySimulation;
using game_lab::BuildingType;
static std::vector<std::string> failures;
static unsigned checks=0;
static void check(bool yes,const std::string& label){++checks;if(!yes)failures.push_back(label);}
static void set32(std::vector<uint8_t>& b,size_t at,int value){for(unsigned j=0;j<4;++j)b.at(at+j)=uint32_t(value)>>(8*j);}
static void write(const std::filesystem::path& p,const std::vector<uint8_t>& b){std::ofstream o(p,std::ios::binary);o.write(reinterpret_cast<const char*>(b.data()),b.size());if(!o)throw std::runtime_error("fixture write failed");}
static CitySimulation seeded(int people,int month){
 CitySimulation c;std::string e;
 for(unsigned x:{10U,12U,14U,16U})check(c.build(x,10,BuildingType::House,e),"own houses accepted");
 check(c.build(20,20,BuildingType::Farm,e),"own farm accepted");
 auto b=c.saveSnapshot();set32(b,12,people);set32(b,20,1000);set32(b,28,month);set32(b,32,0);
 check(c.loadSnapshot(b),"own bounded population fixture accepted");return c;
}
static void invariant(const CitySimulation& c){
 const auto&s=c.stats();
 check(s.population>=0&&s.population<=s.housingCapacity,"population bounded by houses");
 check(s.foodSupply>=0,"food nonnegative");
 check(s.availableWorkers==s.population/2,"current population determines workforce");
 check(s.neededWorkers==s.totalFarms*8+s.totalWells*2+s.totalTemples*4+s.totalMarkets*4+s.totalWatchtowers*2,"all buildings included in demand");
 check(s.month>=1&&s.month<=12&&s.speed>=0&&s.speed<=3&&s.favor>=0&&s.favor<=100&&c.monthAccumulator()>=0&&c.monthAccumulator()<10,"scalar ranges valid");
}
int main(int argc,char**argv){try{
 if(argc!=2)return 2;std::filesystem::path root=argv[1];std::filesystem::create_directories(root);
 // Independently chosen economic checkpoints: 0%, 25%, 50%, 100% staff.
 // Base potential=5, autumn bonus=30; truncate each pooled component separately.
 struct Case{int population,month,harvest;};
 unsigned n=0;
 for(auto item:std::vector<Case>{{0,1,0},{1,1,0},{4,1,1},{8,1,2},{16,1,5},{0,7,0},{1,7,0},{4,7,8},{8,7,17},{16,7,35},{16,10,5},{16,12,5}}){
  auto c=seeded(item.population,item.month);auto initial=c.saveSnapshot();
  write(root/("input-"+std::to_string(n)+".emp"),initial);
  c.advanceMonth();
  check(c.stats().foodSupply==1000+item.harvest-(item.population+9)/10,"monthly stock matches separately chosen expected harvest "+std::to_string(n));
  invariant(c);write(root/("output-"+std::to_string(n++)+".emp"),c.saveSnapshot());
 }
 auto shared=seeded(16,1);std::string e;
 check(shared.build(30,30,BuildingType::Well,e),"remote service adds worker demand");shared.advanceMonth();
 check(shared.stats().foodSupply==1002,"80% staffing produces 4 before 2 consumed");
 CitySimulation rounding;
 for(unsigned x=0;x<12;++x)check(rounding.build(x,0,BuildingType::House,e),"own capacity for precision case");
 for(unsigned x=20;x<23;++x){check(rounding.build(x,20,BuildingType::Farm,e),"three-farm precision fixture");check(rounding.build(x,30,BuildingType::Well,e),"six extra jobs precision fixture");}
 auto precision=rounding.saveSnapshot();set32(precision,12,42);set32(precision,20,1000);set32(precision,28,7);
 check(rounding.loadSnapshot(precision),"21 of 30 jobs fixture accepted");write(root/"precision-input.emp",precision);
 check(rounding.inspectTile(20,20).find("预计8月全城新粮: 73 担")!=std::string::npos,"UI prediction reports pooled exact autumn yield for next month");
 rounding.advanceMonth();
 // 15*21/30 ->10; 90*21/30 ->63; five consumed. Binary floating point must not lose one.
 check(rounding.stats().foodSupply==1068,"70% staffing preserves exact integer autumn quotient 73 before five consumed");
 write(root/"precision-output.emp",rounding.saveSnapshot());invariant(rounding);
 auto preview=seeded(8,7);auto untouched=preview.saveSnapshot();
 check(preview.inspectTile(20,20).find("预计8月全城新粮: 17 担")!=std::string::npos,"preview enters autumn before the month advances");
 check(preview.saveSnapshot()==untouched,"inspection cannot mutate state");
 preview.advanceMonth();check(preview.stats().foodSupply==1016,"announced autumn yield actually enters global stock before one consumed");
 auto afterAutumn=seeded(16,10);
 check(afterAutumn.inspectTile(20,20).find("预计11月全城新粮: 5 担")!=std::string::npos,"preview leaves autumn correctly");
 auto newYear=seeded(16,12);
 check(newYear.inspectTile(20,20).find("预计1月全城新粮: 5 担")!=std::string::npos,"preview wraps December to January");
 CitySimulation percent;
 for(unsigned x=0;x<15;++x)check(percent.build(x,0,BuildingType::House,e),"own housing for exact percent");
 for(unsigned x=20;x<26;++x)check(percent.build(x,20,BuildingType::Farm,e),"six farms demand 48");
 check(percent.build(30,30,BuildingType::Well,e),"well extends demand to 50");
 auto percentInput=percent.saveSnapshot();set32(percentInput,12,58);
 check(percent.loadSnapshot(percentInput),"29 of 50 jobs fixture accepted");
 check(percent.stats().employmentPercent==58,"29 of 50 workers display exact 58 percent");
 write(root/"percent-input.emp",percentInput);
 auto running=seeded(0,6);running.setSpeed(3);
 for(unsigned step=0;step<90;++step){
  if(step==8)check(running.build(10,11,BuildingType::Road,e),"continuation adds a road");
  if(step==19)check(running.build(11,12,BuildingType::Well,e),"continuation adds service demand");
  if(step==37)check(running.demolish(11,12,e),"continuation removes service demand");
  if(step==44)running.setSpeed(0);if(step==47)running.setSpeed(2);
  running.tick();invariant(running);
  const auto snap=running.saveSnapshot();const auto path=root/("checkpoint-"+std::to_string(step)+".emp");
  check(game_lab::saveCityState(running,path,e),"production save succeeds");
  CitySimulation resumed;check(game_lab::loadCityState(resumed,path,e),"production load succeeds");
  check(resumed.saveSnapshot()==snap,"complete state immediately restored");
  auto uninterrupted=running;
  for(unsigned j=0;j<12;++j){uninterrupted.tick();resumed.tick();invariant(resumed);check(uninterrupted.saveSnapshot()==resumed.saveSnapshot(),"identical future across nonzero phase/season/worker checkpoints");}
 }
 auto paused=running;paused.setSpeed(0);const auto before=paused.saveSnapshot();for(unsigned j=0;j<25;++j)paused.tick();
 check(paused.saveSnapshot()==before,"paused production and time unchanged");
 auto bad=before;bad.push_back(1);const auto events=paused.events();
 check(!paused.loadSnapshot(bad)&&paused.saveSnapshot()==before&&paused.events()==events,"malformed save immutable after production");
 std::cout<<"{\"passed\":"<<(failures.empty()?"true":"false")<<",\"checks\":"<<checks<<",\"failures\":[";
 for(unsigned i=0;i<failures.size();++i){if(i)std::cout<<',';std::cout<<'"'<<failures[i]<<'"';}std::cout<<"]}\n";
 return failures.empty()?0:1;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}}
