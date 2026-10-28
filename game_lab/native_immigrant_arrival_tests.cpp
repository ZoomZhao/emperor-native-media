// SPDX-License-Identifier: MIT
// Synthetic proof tests use the actual worker updater + ordered cleanup with
// an authored SG3 descriptor only. No original assets, files, GUI or ASan.
#include "native_immigrant_arrival.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0,groups=0;
void need(bool yes,const std::string& why){++checks;if(!yes)throw std::runtime_error(why);}
struct Fixture {OriginalFigureUpdateContext before;SG3Archive archive;};
Fixture fixture(){Fixture f;auto& c=f.before;c.terrain.assign(51984,0);c.cellHeads.assign(51984,0);c.cropBase=10000;c.activeSide=100;c.modelSpeeds[11]=6;c.modelSpeeds[13]=6;c.buildingEditMode=false;c.buildings.emplace();c.buildings->emplace(0,OriginalFigureBuildingLink{});c.houseCapacities={{0,7},{1,14}};c.services.emplace();c.services->population=OriginalFigurePopulation{};c.services->directions.assign(500000,0);c.services->routeOwners.fill(0);
 for(size_t n=0;n<2000;++n){OriginalFigure w;w.slot=n;c.figures.emplace(n,w);}for(size_t n=1999;n>0;--n)c.freeSlots.push_back(n);
 f.archive.version=214;f.archive.numImages=128;f.archive.sprites.resize(128);f.archive.imageIndex.fill(1);f.archive.bitmaps.resize(1);f.archive.bitmaps[0].filename="authored-test.bmp";f.archive.sprites[0].animationSpriteCount=12;f.archive.sprites[0].animationDirections=8;return f;}
void add(Fixture& f,size_t house,size_t slot,unsigned residents,unsigned amount,uint16_t type=3,int x=5,int y=5){auto& c=f.before;OriginalFigureBuildingLink h;h.type=type;h.alive=1;h.accepting=1;h.worker=int16_t(slot);h.x=int16_t(x);h.y=int16_t(y);h.cell=*c.cropBase+unsigned(x)+228u*unsigned(y);h.size=2;h.residents=int16_t(residents);h.freeSlots=int16_t(7-residents);h.houseModel=0;h.laborDisabled=0;c.buildings->emplace(house,h);
 OriginalFigure w;w.slot=slot;w.recordOffset=slot*388;w.type=11;w.alive=1;w.action=8;w.home=int16_t(house);w.employmentAmount=uint8_t(amount);w.progress=20;w.priorProgress=73;w.x=w.destinationX=int16_t(x);w.y=w.destinationY=int16_t(y);w.subX=w.freeTargetX=int16_t(20*x);w.subY=w.freeTargetY=int16_t(20*y);w.cell=h.cell;w.route=int16_t(slot);w.routeLength=1;w.effectKind=3;w.effectVariant=4;w.effectFlags=2;w.effectX=-1;w.effectY=2;w.effectHeight=3;w.effectVelocityX=4;w.effectVelocityY=-2;w.effectVelocityZ=7;
 w.next=c.cellHeads[w.cell];c.cellHeads[w.cell]=uint16_t(slot);c.figures.at(slot)=w;c.highestLiveSlot=std::max(c.highestLiveSlot,slot);c.freeSlots.erase(std::find(c.freeSlots.begin(),c.freeSlots.end(),slot));c.services->routeOwners[slot]=int16_t(slot);c.services->population->total+=int(residents);c.services->population->peak=c.services->population->total;}
void addWaiting(Fixture& f,size_t slot,int x=6,int y=5,int counter=51){auto& c=f.before;
 OriginalFigure w;w.slot=slot;w.type=13;w.alive=1;w.action=6;w.counter=int16_t(counter);w.employmentAmount=2;w.progress=20;
 w.x=int16_t(x);w.y=int16_t(y);w.subX=int16_t(x*20);w.subY=int16_t(y*20);w.cell=*c.cropBase+unsigned(x)+228u*unsigned(y);
 w.next=c.cellHeads[w.cell];c.cellHeads[w.cell]=uint16_t(slot);c.figures.at(slot)=w;
 c.highestLiveSlot=std::max(c.highestLiveSlot,slot);c.freeSlots.erase(std::find(c.freeSlots.begin(),c.freeSlots.end(),slot));
}
void road(Fixture& f,size_t house){auto& h=f.before.buildings->at(house);h.roadDistance=1;f.before.workerRoadExits[house]=std::make_pair(h.x,h.y);}
OriginalFigureUpdateContext run(const Fixture& f){const std::map<uint32_t,const SG3Archive*> refs{{38,&f.archive}};const auto p=originalFigureOrderedNextPass(f.before,refs);need(p.complete&&p.context,"actual ordered worker pass completes; blocker slot="+std::to_string(p.firstBlockedSlot));return *p.context;}
auto prove(const OriginalFigureUpdateContext& before,const OriginalFigureUpdateContext& after,size_t house){std::string e="old error";auto d=proveNativeImmigrantArrival(before,after,house,e);need(bool(d),"arrival proof:"+e);need(e.empty(),"successful proof clears stale error");return *d;}
void happy(){++groups;
 {auto f=fixture();add(f,12,4,0,9,2);const auto after=run(f);const auto d=prove(f.before,after,12);need(d.houseSlot==12&&d.figureSlot==4&&d.residentsBefore==0&&d.residentsAfter==7&&d.arrived==7&&d.convertedEmpty,"empty type2 ordinary conversion with positive strict capacity clipping");need(after.buildings->at(12).type==3&&after.buildings->at(12).houseModel==0,"empty conversion chooses existing ordinary type3/model0");need(after.figures.at(4).priorProgress==20&&after.figures.at(4).effectVelocityZ==7,"actual cleanup retains only its documented record tail");}
 {auto f=fixture();const unsigned offered=9;const unsigned nativeClipped=std::min(offered,7u-2u);add(f,12,4,2,nativeClipped);const auto after=run(f);const auto d=prove(f.before,after,12);need(d.arrived==5&&d.residentsAfter==7&&!d.convertedEmpty,"synthetic native PRE-clipping is verified as direct strict nonempty addition");}
 {auto f=fixture();add(f,12,4,2,3);const auto after=run(f);const auto first=prove(f.before,after,12),second=prove(f.before,after,12);need(first.arrived==3&&second.arrived==3&&f.before.buildings->at(12).residents==2,"pure repeated proof has no publication or hidden consumed-event state");}
}
void multiple(){++groups;auto f=fixture();add(f,12,4,0,3,2);add(f,18,7,2,2,3,10,10);const auto after=run(f);const auto a=prove(f.before,after,12),b=prove(f.before,after,18);need(a.arrived==3&&b.arrived==2&&a.arrived+b.arrived==5,"several houses in same ordered pass each prove their own positive delta");need(after.services->population->total-f.before.services->population->total==5&&after.services->population->lastArrival==std::optional<uint32_t>{2},"aggregate total and shared lastArrival differ from first event");need(after.freeSlots[after.freeSlots.size()-2]==4&&after.freeSlots.back()==7,"several removals append in ascending ordered-pass slot order");}
void laterReservation(){++groups;
 {auto f=fixture();add(f,12,4,0,3,2);road(f,12);addWaiting(f,7);const auto after=run(f);const auto d=prove(f.before,after,12);
  need(d.convertedEmpty&&d.arrived==3&&d.residentsAfter==3&&after.buildings->at(12).worker==7,"lower type11 empty arrival permits later type13 reciprocal reservation");
  need(after.figures.at(4).alive==0&&after.figures.at(7).alive==1&&after.figures.at(7).action==7&&after.figures.at(7).home==12,"old actor fully removed and replacement only reserves, never arrives twice");
  need(after.services->population->total==3&&after.freeSlots.back()==4&&after.highestLiveSlot==7,"later reservation adds no residents, FIFO entry or hidden second arrival");}
 {auto f=fixture();add(f,12,4,2,2);road(f,12);addWaiting(f,7,6,5,52);const auto after=run(f);const auto d=prove(f.before,after,12);
  need(d.arrived==2&&d.residentsAfter==4&&after.figures.at(7).counter==53,"strict increment-and-compare permits later wait counters, not only the example 51");}
 {auto f=fixture();add(f,12,4,0,3,2);add(f,18,5,5,2,3,6,5);road(f,12);road(f,18);addWaiting(f,7);const auto after=run(f);
  const auto a=prove(f.before,after,12),b=prove(f.before,after,18);
  need(a.arrived==3&&b.arrived==2&&after.buildings->at(18).freeSlots==0&&after.buildings->at(12).worker==7,"closer other house filled by earlier arrival is not wrongly treated as still vacant");}
 {auto f=fixture();add(f,12,4,0,3,2);add(f,18,5,1,1,3,10,10);road(f,12);road(f,18);addWaiting(f,6,10,10);addWaiting(f,7);const auto after=run(f);
  prove(f.before,after,12);prove(f.before,after,18);
  need(after.buildings->at(18).worker==6&&after.buildings->at(12).worker==7,"two earlier arrivals and two ordered new reservations reconstruct without using end-state vacancy");}
 {auto f=fixture();add(f,12,4,0,3,2);add(f,18,9,2,1,3,6,5);road(f,12);road(f,18);addWaiting(f,7);const auto after=run(f);
  prove(f.before,after,12);prove(f.before,after,18);
  need(after.buildings->at(12).worker==7&&after.buildings->at(18).worker==0,"higher old arrival still blocks its closer house at replacement selection time");}
 {auto f=fixture();add(f,12,4,0,3,2);add(f,18,5,2,2,3,6,5);f.before.figures.at(5).type=13;road(f,12);road(f,18);addWaiting(f,7,5,5);const auto after=run(f);
  prove(f.before,after,12);need(after.buildings->at(18).residents==4,"earlier supported NONEMPTY type13 arrival may precede later ordinary reservation");}
}
void forgedReservation(){++groups;auto f=fixture();add(f,12,4,0,3,2);road(f,12);addWaiting(f,7);const auto after=run(f);
 using Change=std::function<void(OriginalFigureUpdateContext&,OriginalFigureUpdateContext&)>;
 const std::vector<Change> changes={
  [](auto&,auto& b){b.buildings->at(12).worker=4;},
  [](auto&,auto& b){b.buildings->at(12).worker=3;},
  [](auto& a,auto&){a.figures.at(7).type=11;},
  [](auto& a,auto&){a.figures.at(7).home=12;},
  [](auto& a,auto&){a.figures.at(7).owner=12;},
  [](auto& a,auto&){a.figures.at(7).action=7;},
  [](auto& a,auto&){a.figures.at(7).counter=50;},
  [](auto& a,auto&){a.figures.at(7).counter=32767;},
  [](auto& a,auto&){a.figures.at(7).updateOwner=1;},
  [](auto& a,auto&){a.figures.at(7).employmentAmount=0;},
  [](auto&,auto& b){b.figures.at(7).home=18;},
  [](auto&,auto& b){b.figures.at(7).action=8;},
  [](auto&,auto& b){++b.figures.at(7).counter;},
  [](auto&,auto& b){++b.figures.at(7).destinationX;},
  [](auto&,auto& b){++b.figures.at(7).employmentAmount;},
  [](auto&,auto& b){++b.figures.at(7).age;},
  [](auto&,auto& b){b.figures.at(7).owner=12;},
  [](auto&,auto& b){b.buildings->at(12).residents=5;b.buildings->at(12).freeSlots=2;},
  [](auto& a,auto&){a.workerRoadExits.erase(12);},
  [](auto&,auto& b){b.workerRoadExits[12]=std::make_pair(int16_t(8),int16_t(8));},
  [](auto& a,auto& b){auto h=a.buildings->at(12);h.worker=0;h.residents=2;h.freeSlots=5;h.type=3;h.x=6;h.cell=*a.cropBase+6+228*5;a.buildings->emplace(20,h);b.buildings->emplace(20,h);},
  [](auto& a,auto& b){auto h=a.buildings->at(12);h.worker=0;h.residents=2;h.freeSlots=5;h.type=3;h.x=7;h.cell=*a.cropBase+7+228*5;a.buildings->emplace(6,h);b.buildings->emplace(6,h);},
  [](auto& a,auto& b){auto h=a.buildings->at(12);h.worker=0;h.residents=2;h.freeSlots=5;h.type=3;h.x=30;h.cell=*a.cropBase+30+228*5;a.buildings->emplace(20,h);h.worker=7;b.buildings->emplace(20,h);},
  [](auto&,auto& b){b.services->routeOwners[8]=7;}
 };
 for(size_t n=0;n<changes.size();++n){auto a=f.before,b=after;changes[n](a,b);std::string e;
  const auto oldBefore=native_immigrant_arrival_detail::fields(a.figures.at(4)),newAfter=native_immigrant_arrival_detail::fields(b.figures.at(7));
  need(!proveNativeImmigrantArrival(a,b,12,e)&&!e.empty(),"forged later reservation rejected case "+std::to_string(n));
  need(native_immigrant_arrival_detail::fields(a.figures.at(4))==oldBefore&&native_immigrant_arrival_detail::fields(b.figures.at(7))==newAfter,"replacement proof is still pure");}
 {auto duplicate=f;addWaiting(duplicate,8,8,5,0);auto b=run(duplicate);b.figures.at(8).home=12;b.figures.at(8).action=7;
  std::string e;need(!proveNativeImmigrantArrival(duplicate.before,b,12,e)&&e=="immigrant_arrival_replacement_duplicate_home","two live replacement families cannot share a target");}
 {auto empty=fixture();add(empty,12,4,0,3,2);auto end=run(empty);empty.before.figures.at(4).type=13;std::string e;
  need(!native_immigrant_arrival_detail::proveArrivalCore(empty.before,end,12,e,true)&&e=="immigrant_arrival_empty_conversion_unported","selection reconstruction never enables unresolved type13 empty-house arrival");}
}
void refusalCases(){++groups;auto f=fixture();add(f,12,4,2,3);const auto after=run(f);
 using Change=std::function<void(OriginalFigureUpdateContext&,OriginalFigureUpdateContext&)>;
 const std::vector<Change> changes={
  [](auto& a,auto&){a.buildings.reset();},[](auto& a,auto&){a.buildings->at(12).worker=0;},[](auto& a,auto&){a.figures.at(4).home=13;},
  [](auto& a,auto&){a.figures.at(4).type=13;},[](auto& a,auto&){a.figures.at(4).action=7;},[](auto& a,auto&){a.figures.at(4).updateOwner=1;},
  [](auto& a,auto&){a.figures.at(4).freeRemainingX=1;},[](auto& a,auto&){++a.figures.at(4).subX;},
  [](auto& a,auto&){auto h=a.buildings->at(12);a.buildings->emplace(20,h);},
  [](auto& a,auto&){a.houseCapacities.erase(0);},[](auto& a,auto&){a.figures.at(4).employmentAmount=9;},
  [](auto&,auto& b){b.buildings->at(12).type=5;},[](auto&,auto& b){b.buildings->at(12).houseModel=1;},
  [](auto&,auto& b){b.buildings->at(12).residents=4;},[](auto&,auto& b){b.buildings->at(12).freeSlots=5;},
  [](auto&,auto& b){b.buildings->at(12).worker=4;},[](auto&,auto& b){++b.buildings->at(12).x;},
  [](auto&,auto& b){b.figures.at(4).target=7;},[](auto&,auto& b){b.figures.at(4).priorProgress=73;},
  [](auto&,auto& b){b.freeSlots.pop_back();},[](auto&,auto& b){b.freeSlots.push_back(4);},
  [](auto&,auto& b){std::swap(b.freeSlots[0],b.freeSlots[1]);},
  [](auto&,auto& b){b.cellHeads[10000+5+228*5]=4;},
  [](auto&,auto& b){b.services->routeOwners[4]=4;},[](auto& a,auto&){a.services->routeOwners[5]=4;},
  [](auto& a,auto&){a.services->routeOwners[4]=0;},
  [](auto&,auto& b){b.buildings->at(0).worker=4;},[](auto& a,auto&){a.figures.at(0).home=12;},
  [](auto&,auto& b){b.services->population.reset();},[](auto& a,auto&){a.highestLiveSlot=1999;}
 };
 for(size_t n=0;n<changes.size();++n){auto a=f.before,b=after;changes[n](a,b);std::string e;const auto beforeActor=native_immigrant_arrival_detail::fields(a.figures.at(4)),afterActor=native_immigrant_arrival_detail::fields(b.figures.at(4));const auto d=proveNativeImmigrantArrival(a,b,12,e);need(!d&&!e.empty(),"invalid/partial proof refuses case "+std::to_string(n));need(native_immigrant_arrival_detail::fields(a.figures.at(4))==beforeActor&&native_immigrant_arrival_detail::fields(b.figures.at(4))==afterActor,"proof never mutates either record");}
 std::string e;need(!proveNativeImmigrantArrival(f.before,after,0,e)&&!e.empty(),"building slot0 is never an arrival target");need(!proveNativeImmigrantArrival(f.before,after,3999,e)&&!e.empty(),"missing positive target rejected");
}
void duplicatesAndDomains(){++groups;
 {auto f=fixture();add(f,12,4,2,3);add(f,18,7,1,1,3,10,10);auto after=run(f);f.before.figures.at(7).home=12;std::string e;need(!proveNativeImmigrantArrival(f.before,after,12,e)&&e=="immigrant_arrival_duplicate_house_target","two live incoming figures cannot claim one target");}
 {auto f=fixture();add(f,12,4,2,9);const auto after=run(f);std::string e;need(after.buildings->at(12).residents==11,"strict nonempty worker does NOT clip; native preprocessing has separate ownership");need(!proveNativeImmigrantArrival(f.before,after,12,e)&&e=="immigrant_arrival_amount_not_admitted","unclipped overcapacity is never misproved as native clipped arrival");}
 {auto f=fixture();add(f,12,4,0,3,2);f.before.buildingEditMode=true;const auto after=run(f);std::string e;need(!proveNativeImmigrantArrival(f.before,after,12,e)&&e=="immigrant_arrival_empty_conversion_unported","editing branch is outside native ordinary conversion proof");}
 {auto f=fixture();add(f,12,4,0,3,4);const auto after=run(f);std::string e;need(!proveNativeImmigrantArrival(f.before,after,12,e)&&e=="immigrant_arrival_empty_conversion_unported","other empty classes are not inferred from ordinary type2");}
 {auto f=fixture();add(f,12,4,2,3);auto after=run(f);f.before.figures.at(4).employmentAmount=0;std::string e;need(!proveNativeImmigrantArrival(f.before,after,12,e),"zero offered group is not a positive arrival event");}
}
}
int main(){try{happy();multiple();laterReservation();forgedReservation();refusalCases();duplicatesAndDomains();std::cout<<"{\"groups\":"<<groups<<",\"checks\":"<<checks<<",\"passed\":true,\"synthetic_contract_only\":true}"<<std::endl;return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<std::endl;return 1;}}
