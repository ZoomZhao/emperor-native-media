// SPDX-License-Identifier: MIT
// Native immigration strategy integration over two source-bound empty maps.
// Authored boundary cases are explicitly synthetic, never original observations.
#define main nativePopulationPreviousDryMain
#include "native_house_dry_runtime_tests.cpp"
#undef main
namespace {
NativeSessionState populationCity(Test& t){
 std::string error;auto state=startNativeHouseRuntime(t.seed(),*t.src.runtime,error,NativeWellReturnPolicy::ObservedHomeRoad,true,true,true,true);
 need(bool(state),"population constructor:"+error);auto s=*state;
 std::vector<int> path;const auto xy=choose(t.src,path);
 for(int cell:path)if(!t.seed().sourceRoad[size_t(cell)])t.action(s,NativeSessionAction::Road,cell%int(t.seed().side),cell/int(t.seed().side));
 t.action(s,NativeSessionAction::House,xy.first,xy.second);t.house=s.objects.back().id;
 need(s.populationRuntime&&nativeHouseRuntimeWireVersion(s)==5,"explicit new population identity");
 need(nativePopulation(s)==0&&planNativeFoodCycle(t.seed(),s).arrivals.empty(),"meal cycle cannot write new mode population");
 return s;
}
void replaceImmigrant(NativeSessionState& s,const OriginalFigure& f){
 std::string e;const auto encoded=encodeNativeImmigrant(f,e);need(bool(encoded),"stage immigrant codec:"+e);bool found=false;
 for(auto& bytes:s.figureRuntime->immigrantPayloads){const auto old=decodeNativeImmigrant(bytes,e);need(bool(old),"stage prior codec");if(old->slot==f.slot){bytes=*encoded;found=true;}}
 need(found,"stage existing actor only");
}
void syntheticResident(Test& t,NativeSessionState& s,uint32_t residents){
 const auto prior=t.home(s).residents;need(residents>=prior,"synthetic positive boundary only");const auto delta=residents-prior;
 auto& o=t.home(s);o.residents=residents;o.house->imageWord20=int16_t(residents);
 auto record=t.record(s);record.residents=int16_t(residents);record.free=int16_t(*nativeHouseCapacity(t.seed(),s,o)-residents);s.strictRecords[t.house]=encodeNativeStrictRecord(record);
 auto& p=*s.populationRuntime;p.admitted+=delta;p.arrived+=delta;p.populationArrived+=delta;s.observedDry->arrivedSinceMeal+=delta;
 std::string e;auto context=decodeNativeStrictContext(*s.strictContext,e);need(bool(context),"synthetic population context");
 context->population->total+=int32_t(delta);context->population->peak=std::max(context->population->peak,context->population->total);*context->nativePopulation+=delta;s.strictContext=encodeNativeStrictContext(*context);t.canonicalize(s);
}
void capacityBoundaries(Test& t,const NativeSessionState& first){
 auto s=first;unsigned n=0;std::optional<OriginalFigure> approach;
 while(n++<4000){for(const auto& bytes:s.figureRuntime->immigrantPayloads){std::string e;const auto f=decodeNativeImmigrant(bytes,e);need(bool(f),"approach codec");if(f->action==8)approach=*f;}if(approach)break;t.tick(s);}
 need(bool(approach),"natural nonempty approach exists");need(t.home(s).residents==6,"first natural group leaves one actual place");
 auto enlarged=*approach;const unsigned additional=3-enlarged.employmentAmount;enlarged.employmentAmount=3;s.populationRuntime->admitted+=additional;replaceImmigrant(s,enlarged);t.canonicalize(s);
 // Explicit authored reservation oversubscription: the native policy clips
 // before strict type11, which itself does not clip a nonempty house.
 auto full=s;syntheticResident(t,full,7);const auto fullUnplaced=full.populationRuntime->unplaced;
 t.replay(full,"synthetic-full-target-cancel");t.tick(full);
 need(full.figureRuntime->immigrantPayloads.empty()&&full.populationRuntime->unplaced==fullUnplaced+3&&t.home(full).residents==7,"full target cancellation preserves residents and offer receipt");
 need(validateNativeHouseRuntimeReady(t.seed(),full,*t.src.runtime).empty(),"full cleanup graph ready");
 const auto unplaced=s.populationRuntime->unplaced;t.replay(s,"synthetic-nonempty-clip");t.tick(s);
 need(s.populationRuntime->unplaced==unplaced+2,"native clipping records two unplaced, never phantom arrivals");
 for(n=0;!s.figureRuntime->immigrantPayloads.empty()&&n<80;++n)t.tick(s);
 need(t.home(s).residents==7&&s.populationRuntime->arrived==7&&s.populationRuntime->unplaced==unplaced+2,"clipped final person arrives once");t.replay(s,"clipped-arrival-cleaned");
}
void multipleSamePass(Test& t){
 auto s=populationCity(t);const uint64_t firstHouse=t.house;
 auto available=t.src;for(const auto& o:s.objects)for(unsigned dy=0;dy<nativeObjectSide(o.kind);++dy)for(unsigned dx=0;dx<nativeObjectSide(o.kind);++dx){const auto at=size_t(o.y+dy)*t.seed().side+o.x+dx;available.seed.buildable[at]=0;if(o.kind==NativeObjectKind::Road)available.seed.sourceRoad[at]=1;}
 std::vector<int> path;const auto xy=choose(available,path);for(int cell:path)if(!nativeRoads(t.seed(),s)[size_t(cell)])t.action(s,NativeSessionAction::Road,cell%int(t.seed().side),cell/int(t.seed().side));
 t.action(s,NativeSessionAction::House,xy.first,xy.second);const uint64_t secondHouse=s.objects.back().id;need(secondHouse!=firstHouse,"two distinct owned houses");
 t.action(s,NativeSessionAction::Resume);s.houseSchedule->phase=21;t.tick(s);t.tick(s);for(unsigned delay=0;s.figureRuntime->immigrantPayloads.empty()&&delay<8;++delay){s.houseSchedule->phase=23;t.tick(s);}need(s.figureRuntime->immigrantPayloads.size()==1,"first admitted group after saved inDelay expires");
 std::string e;auto frozen=decodeNativeImmigrant(s.figureRuntime->immigrantPayloads.front(),e);need(bool(frozen),"first pending codec");
 auto strict=t.strict(s);auto city=*strict.city;auto& w=city.figures.figures.at(frozen->slot);
 if(w.route){city.figures.services->routeOwners[size_t(w.route)]=0;w.route=0;}w.action=6;w.counter=1000;
 native_phase_detail::advanceContexts(city,23);auto project=strictToNative(t.seed(),s,city,strict.isolation,t.src.runtime->strictTemplate);need(bool(project.state),"freeze first synthetic dispatch:"+project.refusal);s=*project.state;t.tick(s);
 need(s.figureRuntime->immigrantPayloads.size()==2,"second group admitted without replacing first reservation");
 // Authored mixed-family cancellation history: the replaced inbound offer is
 // unplaced; a historical departure of that size is now an independent 13.
 // No empty-house type13 arrival is enabled or exercised.
 auto mixed=s;std::optional<OriginalFigure> migrant;for(auto it=mixed.figureRuntime->immigrantPayloads.begin();it!=mixed.figureRuntime->immigrantPayloads.end();++it){const auto f=decodeNativeImmigrant(*it,e);need(bool(f),"mixed prior11 codec");if(uint64_t(f->home)==secondHouse){migrant=*f;mixed.figureRuntime->immigrantPayloads.erase(it);break;}}
 need(bool(migrant),"second owned group for mixed fixture");migrant->type=13;migrant->action=7;migrant->counter=0;
 auto bytes=encodeNativeMigrant(*migrant,e);need(bool(bytes),"mixed13 codec:"+e);mixed.figureRuntime->migrantPayloads.push_back(*bytes);
 mixed.populationRuntime->unplaced+=migrant->employmentAmount;mixed.populationRuntime->populationArrived+=migrant->employmentAmount;mixed.populationRuntime->populationDeparted+=migrant->employmentAmount;t.canonicalize(mixed);
 const auto kept11=mixed.figureRuntime->immigrantPayloads;t.action(mixed,NativeSessionAction::Pause);
 const auto cancel=transactNativeHouseRuntime(t.seed(),mixed,{NativeSessionAction::Demolish,mixed.revision,0,0,0,0,secondHouse},*t.src.runtime);
 need(cancel.committed,"mixed11+13 target cancellation:"+cancel.error);need(mixed.figureRuntime->immigrantPayloads==kept11&&mixed.figureRuntime->migrantPayloads.size()==1,"mixed cancellation preserves unrelated inbound and departed people");
 const auto released=decodeNativeMigrant(mixed.figureRuntime->migrantPayloads.front(),e);need(released&&released->home==0&&released->action==6,"mixed target releases reservation rather than losing emigrants");t.action(mixed,NativeSessionAction::Resume);t.replay(mixed,"mixed-cancel-restored");
 strict=t.strict(s);city=*strict.city;
 for(auto& [slot,f]:city.figures.figures)if(f.alive&&f.type==11){const auto& h=city.figures.buildings->at(size_t(f.home));if(f.route)city.figures.services->routeOwners[size_t(f.route)]=0;
  f.route=0;f.action=8;f.freeMovement=1;f.x=f.priorX=f.destinationX=h.x;f.y=f.priorY=f.destinationY=h.y;f.cell=h.cell;f.subX=int16_t(20*int(h.x));f.subY=int16_t(20*int(h.y));f.freeTargetX=f.subX;f.freeTargetY=f.subY;f.freeRemainingX=f.freeRemainingY=f.freeError=0;f.next=0;
 }
 city.figures.cellHeads.assign(51984,0);for(const auto& [slot,f]:city.figures.figures)if(f.alive){const auto cell=originalFigureDrawCell(f);need(cell<51984,"staged draw cell");auto& head=city.figures.cellHeads[size_t(cell)];if(!head)head=uint16_t(slot);else{auto tail=size_t(head);while(city.figures.figures.at(tail).next)tail=city.figures.figures.at(tail).next;city.figures.figures.at(tail).next=uint16_t(slot);}}
 native_phase_detail::advanceContexts(city,19);project=strictToNative(t.seed(),s,city,strict.isolation,t.src.runtime->strictTemplate);need(bool(project.state),"two-arrival synthetic boundary:"+project.refusal);s=*project.state;
 const auto beforeFingerprint=native_bridge_detail::fingerprint(city,strict.isolation);auto changed=city;auto slot=decodeNativeImmigrant(s.figureRuntime->immigrantPayloads.front(),e)->slot;++changed.figures.figures.at(slot).counter;
 need(beforeFingerprint!=native_bridge_detail::fingerprint(changed,strict.isolation),"type11 field mutation changes lossless fingerprint");
 t.replay(s,"two-houses-same-pass");t.tick(s);need(nativePopulation(s)==12&&s.populationRuntime->populationArrived==12&&s.populationRuntime->arrived==12,"two actual ordered-pass arrivals aggregate to twelve");
 need(s.figureRuntime->immigrantPayloads.empty()&&t.record(s).migrant==0,"same-pass cleanup removes all inbound payloads");
 const auto rebuilt=t.strict(s);need(rebuilt.city->figures.services->population->total==12,"same-pass strict and native totals identical");
 t.replay(s,"two-arrivals-recovered");t.saveStage(s,"two-arrivals");t.house=firstHouse;
}
void cancelledNewOffer(Test& t){
 auto s=populationCity(t);t.action(s,NativeSessionAction::Resume);s.houseSchedule->phase=21;t.tick(s);t.tick(s);
 need(t.record(s).distance>0,"actual phase22 publishes reachable house");t.action(s,NativeSessionAction::Pause);
 std::vector<uint64_t> roads;for(const auto& o:s.objects)if(o.kind==NativeObjectKind::Road)roads.push_back(o.id);
 need(!roads.empty(),"owned access roads exist");for(uint64_t id:roads){const auto r=transactNativeHouseRuntime(t.seed(),s,{NativeSessionAction::Demolish,s.revision,0,0,0,0,id},*t.src.runtime);need(r.committed,"road removal:"+r.error);}
 need(!nativeHouseConnected(t.seed(),t.home(s),nativeRoads(t.seed(),s)),"actual native house is disconnected after owned roads removed");
 need(t.record(s).distance>0&&s.houseSchedule->phase==23,"stale capacity distance retained until next road phase");t.action(s,NativeSessionAction::Resume);
 t.replay(s,"same-pass-new-offer-unreachable");t.tick(s);
 need(s.populationRuntime->admitted>0&&s.populationRuntime->unplaced==s.populationRuntime->admitted&&s.populationRuntime->arrived==0&&nativePopulation(s)==0,"successful same-pass allocation then cancel retained in receipts");
 need(s.figureRuntime->immigrantPayloads.empty()&&t.record(s).migrant==0,"unreachable cancellation clears actual home reservation");
 need(validateNativeHouseRuntimeReady(t.seed(),s,*t.src.runtime).empty(),"new-offer cancellation graph ready");t.replay(s,"same-pass-cancelled-recovered");
}
void arrivalStaffing(Test& t){
 auto s=populationCity(t);const auto& h=t.home(s);const auto roads=nativeRoads(t.seed(),s);int wx=-1,wy=-1;
 for(int y=std::max(0,int(h.y)-4);y<=std::min(int(t.seed().side)-2,int(h.y)+5)&&wx<0;++y)for(int x=std::max(0,int(h.x)-4);x<=std::min(int(t.seed().side)-2,int(h.x)+5)&&wx<0;++x){
  NativeSessionObject probe;probe.kind=NativeObjectKind::Well;probe.x=uint32_t(x);probe.y=uint32_t(y);if(!nativeHouseConnected(t.seed(),probe,roads))continue;
  auto candidate=s;const auto okay=transactNativeSession(t.seed(),candidate,{NativeSessionAction::Well,s.revision,x,y});if(okay.committed){wx=x;wy=y;}
 }
 need(wx>=0,"connected Well footprint beside owned roads");t.action(s,NativeSessionAction::Well,wx,wy);const uint64_t well=s.objects.back().id;
 t.action(s,NativeSessionAction::Resume);for(unsigned n=0;nativePopulation(s)==0&&n<4000;++n)t.tick(s);
 need(nativePopulation(s)>0,"natural arrival with prebuilt unstaffed Well");const auto labor=planNativeLabor(t.seed(),s);const auto* job=labor.find(well);
 need(labor.error.empty()&&job&&job->workers>0,"resident arrival produces actual native available workers");
 const auto strict=t.strict(s);need(strict.city->buildings.at(well).state.inputs.workers==int(job->workers)&&strict.city->figures.buildings->at(well).workers==int(job->workers),"post-arrival Well staffing matches saved and derived projections");
 t.replay(s,"arrival-staffing-same-state");t.saveStage(s,"population-staffed-arrival");
}
void runPopulation(Test& t){
 auto s=populationCity(t);const auto initial=s;t.saveStage(s,"population-empty");
 std::cout<<"{\"labor\":{\"sentiment\":"<<s.populationRuntime->labor.sentiment<<",\"health\":"<<s.populationRuntime->labor.health<<",\"pending\":"<<s.populationRuntime->labor.statisticsPending<<",\"difficulty\":"<<s.populationRuntime->labor.difficulty<<",\"wage\":"<<s.populationRuntime->labor.wageBand<<"}}"<<std::endl;
 t.action(s,NativeSessionAction::Resume);bool first=false;std::set<int> actions;std::set<unsigned> restoredPhases;
 for(unsigned n=0;n<4000&&!first;++n){
  if(!s.figureRuntime->immigrantPayloads.empty()){std::string e;const auto w=decodeNativeImmigrant(s.figureRuntime->immigrantPayloads.front(),e);need(bool(w),"live immigrant codec:"+e);if(actions.insert(w->action).second){
   const auto scene=nativeSessionScene(t.src,s);need(scene&&scene->figures,"live type11 scene projection exists");
   need(scene->figures->live.size()==s.figureRuntime->immigrantPayloads.size()+s.figureRuntime->migrantPayloads.size()+s.figureRuntime->wellWalkerPayloads.size(),"all live families reach drawing profile");
   std::string projectionError;const auto visible=encodeNativeImmigrant(scene->figures->slots.at(w->slot),projectionError);need(visible&&*visible==s.figureRuntime->immigrantPayloads.front(),"drawing profile keeps all118 type11 fields");
   t.saveStage(s,"population-action-"+std::to_string(w->action));
  }}
  if(restoredPhases.insert(s.houseSchedule->phase).second)t.replay(s,"population-phase-"+std::to_string(s.houseSchedule->phase));
  const auto previous=nativePopulation(s);t.tick(s);
  if(nativePopulation(s)>previous){need(s.populationRuntime->populationArrived==nativePopulation(s),"arrival is sole population writer");first=true;}
 }
 need(first,"natural type11 first arrival within native test bound");
 need(actions.count(7)&&actions.count(8),"natural road route and free approach observed (zero countdown may leave action6 in dispatch pass)");
 need(t.home(s).house->type==3&&t.home(s).house->model==0,"ordinary empty becomes type3/model0");
 need(s.populationRuntime->arrived>0&&s.populationRuntime->admitted>=s.populationRuntime->arrived,"inbound accounting records actual arrivals");
 need(t.home(s).residents<=*nativeHouseCapacity(t.seed(),s,t.home(s)),"first arrival respects capacity");
 t.replay(s,"population-first-arrival");t.saveStage(s,"population-first-arrival");
 auto occupied=s;t.action(occupied,NativeSessionAction::Pause);const auto removedPeople=t.home(occupied).residents;
 const auto removal=transactNativeHouseRuntime(t.seed(),occupied,{NativeSessionAction::Demolish,occupied.revision,0,0,0,0,t.house},*t.src.runtime);
 need(removal.committed,"occupied target demolition:"+removal.error);
 need(nativePopulation(occupied)==0&&occupied.populationRuntime->populationDeparted==removedPeople&&validateNativeHouseRuntimeReady(t.seed(),occupied,*t.src.runtime).empty(),"demolition debits same native/strict population receipt");
 t.action(occupied,NativeSessionAction::Resume);t.replay(occupied,"occupied-demolition-restored");
 std::cout<<"{\"checkpoint\":\"natural-first-arrival\",\"checks\":"<<checks<<",\"steps\":"<<steps<<"}"<<std::endl;
 capacityBoundaries(t,s);
 std::cout<<"{\"checkpoint\":\"capacity-boundaries\",\"checks\":"<<checks<<"}"<<std::endl;
 const auto meal=planNativeFoodCycle(t.seed(),s);need(meal.arrivals.empty()&&meal.cycle.arrivals==0,"occupied new mode meal does not double grow");
 // Demolition during a live inbound reservation must cancel the whole owned
 // route/FIFO/cell reference while preserving historical admission receipts.
 auto cancel=initial;t.action(cancel,NativeSessionAction::Resume);
 unsigned wait=0;while(cancel.figureRuntime->immigrantPayloads.empty()&&wait++<160)t.tick(cancel);
 need(!cancel.figureRuntime->immigrantPayloads.empty(),"natural reserved target for cancellation");
 const auto admitted=cancel.populationRuntime->admitted;t.action(cancel,NativeSessionAction::Pause);
 const auto before=encodeNativeSession(t.seed(),cancel);const NativeSessionCommand command{NativeSessionAction::Demolish,cancel.revision,0,0,0,0,t.house};
 const auto preview=previewNativeHouseCommand(t.seed(),cancel,command,*t.src.runtime);
 need(preview.committed&&encodeNativeSession(t.seed(),cancel)==before,"cancel preview is side effect free:"+preview.error);
 const auto result=transactNativeHouseRuntime(t.seed(),cancel,command,*t.src.runtime);need(result.committed,"demolish inbound target:"+result.error);
 need(cancel.figureRuntime->immigrantPayloads.empty()&&cancel.populationRuntime->unplaced==admitted&&nativePopulation(cancel)==0,"cancelled group unplaced, never phantom residents");
 need(validateNativeHouseRuntimeReady(t.seed(),cancel,*t.src.runtime).empty(),"cancel graph ready");
 const auto bytes=encodeNativeSession(t.seed(),cancel);const auto again=transactNativeHouseRuntime(t.seed(),cancel,{NativeSessionAction::Demolish,cancel.revision,0,0,0,0,t.house},*t.src.runtime);
 need(!again.committed&&encodeNativeSession(t.seed(),cancel)==bytes,"repeat target removal refuses atomically");
 multipleSamePass(t);std::cout<<"{\"checkpoint\":\"mixed-and-multiple-arrivals\",\"checks\":"<<checks<<"}"<<std::endl;
 cancelledNewOffer(t);std::cout<<"{\"checkpoint\":\"cancelled-new-offer\",\"checks\":"<<checks<<"}"<<std::endl;
 arrivalStaffing(t);
 // Existing R4 keeps the original authored meal population strategy.
 std::string error;auto old=startNativeHouseRuntime(t.seed(),*t.src.runtime,error,NativeWellReturnPolicy::ObservedHomeRoad,true,true,true);
 need(old&&!old->populationRuntime&&nativeHouseRuntimeWireVersion(*old)==4,"old R4 remains wire4 without new authority");
 std::cout<<"{\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replays\":"<<replays<<",\"population\":"<<nativePopulation(s)<<",\"arrived\":"<<s.populationRuntime->arrived<<",\"admitted\":"<<s.populationRuntime->admitted<<",\"passed\":true}"<<std::endl;
}
}
int main(int argc,char** argv){try{
 const char* data=std::getenv("EMPEROR_GAMEDATA_ROOT");auto root=ResourceRoot::fromAbsolute(data?data:"");if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}std::string error;
 auto source=loadNativeSessionSource(root,argc>1?argv[1]:"Save/2026102301/历史战役自动保存_1.sav",error);need(bool(source),"source:"+error);need(prepareNativeHouseRuntimeSource(root,*source,error),"runtime:"+error);
 const std::string option=argc>2?argv[2]:"";Test t{*source,root,option.rfind("--",0)==0?"":option,0};
 if(option=="--edge")cancelledNewOffer(t);else if(option=="--staffing")arrivalStaffing(t);else if(option=="--multiple")multipleSamePass(t);else runPopulation(t);
 std::cout<<"{\"case_complete\":true,\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replays\":"<<replays<<"}"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
