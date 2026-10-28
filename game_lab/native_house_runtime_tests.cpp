// SPDX-License-Identifier: MIT
// Real-data integration; no source data is written, and missing root is SKIP.
#include "native_house_runtime_source.hpp"
#include <iostream>
#include <cstdlib>
#include <deque>
#include "native_session_io.hpp"
#include <filesystem>
using namespace game_lab;
size_t checks=0;void need(bool v,const std::string& why){++checks;if(!v)throw std::runtime_error(why);}
int main(int argc,char**argv){try{
 auto root=ResourceRoot::fromAbsolute(std::getenv("EMPEROR_GAMEDATA_ROOT")?std::getenv("EMPEROR_GAMEDATA_ROOT"):"");if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
 std::string error;auto src=loadNativeSessionSource(root,argc>1?argv[1]:"Save/2026102301/历史战役自动保存_1.sav",error);if(!src||!prepareNativeHouseRuntimeSource(root,*src,error)){std::cerr<<"source:"<<error;return 1;}
 auto state=startNativeHouseRuntime(src->seed,*src->runtime,error);if(!state){std::cerr<<error;return 2;}
 const auto& seed=src->seed;
 auto badSource=*src;badSource.seed.sourceRevision^=1;need(!prepareNativeHouseRuntimeSource(root,badSource,error)&&error=="runtime_source_changed","source revision mismatch refused");
 const auto initialWire=encodeNativeSession(seed,*state);need(!initialWire.empty(),"initial encodable");
 auto initialLoaded=decodeNativeSession(seed,initialWire,error);need(initialLoaded&&encodeNativeSession(seed,*initialLoaded)==initialWire,"initial exact wire");
 auto run=[&](NativeSessionAction a,int x=0,int y=0){auto r=transactNativeHouseRuntime(seed,*state,{a,state->revision,x,y},*src->runtime);need(r.committed,"action="+std::to_string(int(a))+" at="+std::to_string(x)+","+std::to_string(y)+" phase="+std::to_string(state->houseSchedule->phase)+" "+r.error);};
 int sx=-1,sy=-1;size_t best=SIZE_MAX;std::vector<int> chosen;
 const int n=int(seed.side);const auto& random=src->save.layers.ground->state.randomByte;int origin=(228-n)/2;
 for(int y=1;y+7<n;++y)for(int x=1;x+8<n;++x){bool okay=true;for(int dy=0;dy<7&&okay;++dy)for(int dx=0;dx<8;++dx)if(!seed.buildable[size_t(y+dy)*n+x+dx]){okay=false;break;}
  if(!okay||!(random[size_t(y+1+origin)*228+x+origin]&1))continue;
  const int start=y*n+x;std::deque<int> q{start};std::vector<int> prior(size_t(n)*n,-1);prior[start]=start;int end=-1;
  while(!q.empty()){int at=q.front();q.pop_front();if(seed.sourceRoad[at]){end=at;break;}int xx=at%n,yy=at/n;for(auto d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}){int nx=xx+d.first,ny=yy+d.second;if(nx<0||ny<0||nx>=n||ny>=n)continue;int next=ny*n+nx;if(prior[next]>=0||(!seed.buildable[next]&&!seed.sourceRoad[next]))continue;prior[next]=at;q.push_back(next);}}
  if(end<0)continue;std::vector<int> path;for(int at=end;;at=prior[at]){path.push_back(at);if(at==start)break;}
  if(path.size()<best){best=path.size();sx=x;sy=y;chosen=path;}
  if(best<4)break;
 }
 if(sx<0)throw std::runtime_error("no connected buildable probe area");
 for(int at:chosen)if(!seed.sourceRoad[at])run(NativeSessionAction::Road,at%n,at/n);
 for(int dx=0;dx<8;++dx)if(!nativeRoads(seed,*state)[size_t(sy)*n+sx+dx])run(NativeSessionAction::Road,sx+dx,sy);
 for(int dx=0;dx<8;++dx)run(NativeSessionAction::Road,sx+dx,sy+3);
 for(int dy=1;dy<3;++dy)run(NativeSessionAction::Road,sx+2,sy+dy);
 for(int dx:{0,3,6})run(NativeSessionAction::House,sx+dx,sy+4);
 run(NativeSessionAction::House,sx,sy+1);run(NativeSessionAction::Well,sx+3,sy+1);run(NativeSessionAction::Hunter,sx+6,sy+1);
 std::cout<<"{\"constructed\":true,\"x\":"<<sx<<",\"y\":"<<sy<<",\"road_cells\":"<<chosen.size()<<"}\n";
 run(NativeSessionAction::Resume);
 bool watered=false,upgraded=false;size_t maxWalkers=0;
 for(int k=0;k<700;++k){
  // Exercise phase38->39 with a freshly decoded independent history. All
  // records, caches, RNG and render inputs must match the uninterrupted step.
  if(state->houseSchedule->phase==39){
   const auto wire=encodeNativeSession(seed,*state);auto reloaded=decodeNativeSession(seed,wire,error);need(bool(reloaded),"pre-evolution checkpoint decodes");
   auto r=transactNativeHouseRuntime(seed,*reloaded,{NativeSessionAction::Tick,reloaded->revision},*src->runtime);need(r.committed,"reloaded evolution step:"+r.error);
   run(NativeSessionAction::Tick);need(encodeNativeSession(seed,*state)==encodeNativeSession(seed,*reloaded),"evolution resume deterministic");
  }else run(NativeSessionAction::Tick);
  maxWalkers=std::max(maxWalkers,state->figureRuntime->wellWalkerPayloads.size());
  for(const auto& o:state->objects)if(o.house){watered|=o.house->houseWater.value_or(0)>0;upgraded|=o.house->type==4;}
 }
 need(maxWalkers>0,"real model dispatched a Well walker");need(watered,"walker wrote actual house water");need(upgraded,"actual phase39 published occupied type4");
 for(const auto& o:state->objects)if(o.house){need(nativeHouseCapacity(seed,*state,o)==std::make_optional(seed.houseCapacities.at(o.house->model)),"active grade capacity");}
 auto scene=nativeSessionScene(*src,*state);need(scene&&scene->figures&&scene->figures->live.size()==state->figureRuntime->wellWalkerPayloads.size(),"same state renders live walkers");
 // Native-only geometry regression: retain a constructed house's solid
 // footprint after a grade change, including neighbouring road probes. This
 // deliberately moves a test house; it is not original SAV/type4 evidence.
 auto adjacent=*state;
 auto moved=std::find_if(adjacent.objects.begin(),adjacent.objects.end(),[&](const auto& o){return o.house&&o.x==uint32_t(sx)&&o.y==uint32_t(sy+1);});
 need(moved!=adjacent.objects.end()&&moved->house->type==4,"odd-variant occupied house upgraded");
 const auto gradeId=moved->id;moved->x+=1;
 for(const auto& o:adjacent.objects)if(o.kind==NativeObjectKind::Road&&o.x==uint32_t(sx+2)&&(o.y==uint32_t(sy+1)||o.y==uint32_t(sy+2)))adjacent.retired.push_back({o.id,o.kind});
 std::sort(adjacent.retired.begin(),adjacent.retired.end(),[](const auto& a,const auto& b){return a.id<b.id;});
 adjacent.objects.erase(std::remove_if(adjacent.objects.begin(),adjacent.objects.end(),[&](const auto& o){return o.kind==NativeObjectKind::Road&&o.x==uint32_t(sx+2)&&(o.y==uint32_t(sy+1)||o.y==uint32_t(sy+2));}),adjacent.objects.end());
 auto adjacentBridge=nativeToStrict(seed,adjacent,src->runtime->strictTemplate,nativeStrictPersisted(adjacent));
 need(bool(adjacentBridge.city),"adjacent type4 road access retained:"+adjacentBridge.refusal);
 auto projectedAdjacent=strictToNative(seed,adjacent,*adjacentBridge.city,adjacentBridge.isolation,src->runtime->strictTemplate);
 need(bool(projectedAdjacent.state),"adjacent grade bridge remains lossless:"+projectedAdjacent.refusal);
 need(std::find(adjacentBridge.policies.begin(),adjacentBridge.policies.end(),"native_house_type4_road_footprint_retained")!=adjacentBridge.policies.end(),"road adapter explicitly tagged");
 // Synthetic dispatch boundary: inject one non-road neighbouring owner into
 // the hook's local view. Raw strict initial-roam must continue to refuse the
 // unproven type4 predicate; only the explicit native footprint view admits it.
 auto dispatch=*adjacentBridge.city;
 auto worker=decodeNativeWellWalker(adjacent.figureRuntime->wellWalkerPayloads.front(),error);need(bool(worker),"dispatch test worker decodes");
 const auto wellId=size_t(worker->owner);auto retiring=dispatch.figures.figures.at(worker->slot);retiring.alive=2;
 need(originalFigureRemoveArrivedWorker(retiring,dispatch.figures),"dispatch fixture reciprocal cleanup");
 need(originalCombinedSyncBuildings(dispatch).empty(),"dispatch fixture synchronizes owner");
 dispatch.buildings.at(wellId).state.inputs.spawnCounter=30;
 auto& context=*dispatch.buildingContext;context.buildingInputs->at(wellId).spawnCounter=30;
 const auto road=context.poolHookRoadAccess->at(wellId);const auto spawnCell=*dispatch.figures.cropBase+uint32_t(road.x)+228u*uint32_t(road.y);
 bool injected=false;for(int delta:{-228,1,228,-1}){const auto at=size_t(int64_t(spawnCell)+delta);if(at>=51984||(dispatch.figures.terrain[at]&0x448))continue;
  dispatch.figures.terrain[at]|=8;dispatch.figures.buildingOwners[at]=uint16_t(gradeId);context.terrainOwners->at(at)=uint16_t(gradeId);injected=true;break;}
 need(injected,"non-road neighbour staged without replacing a real road");
 native_phase_detail::advanceContexts(dispatch,31);const auto refs=src->runtime->archiveRefs();NativePhaseStepInputs phaseInputs;phaseInputs.archives=&refs;phaseInputs.wrap=NativePhaseWrapPolicy::WrapPreserveMonthPart;phaseInputs.retainNativeHunterEconomy=true;
 auto rawDispatch=stepNativeHousePhase(dispatch,adjacentBridge.isolation,phaseInputs);
 need(!rawDispatch.candidate,"strict type4 initial-roam predicate remains unproven");
 phaseInputs.retainNativeHouseFootprints=true;auto nativeDispatch=stepNativeHousePhase(dispatch,adjacentBridge.isolation,phaseInputs);
 need(bool(nativeDispatch.candidate),"native footprint dispatch succeeds:"+nativeDispatch.refusal);
 need(nativeDispatch.allocations==1,"one Well worker allocated at boundary");
 const auto& admitted=*nativeDispatch.candidate;need(admitted.buildings.at(gradeId).state.building.type==4&&admitted.figures.buildings->at(gradeId).type==4&&admitted.buildingContext->buildings->at(gradeId).type==4&&admitted.buildingContext->buildingLinks->at(gradeId).type==4,"canonical type4 restored in every pool");
 auto lifecycle=admitted;native_phase_detail::advanceContexts(lifecycle,40);auto phase40=stepNativeHousePhase(lifecycle,nativeDispatch.isolation,phaseInputs);
 need(bool(phase40.candidate),"occupied type4 phase40 progresses without a lifecycle skip:"+phase40.refusal);
 need(phase40.candidate->buildings.at(gradeId).state.inputs.houseInUse==1,"constructed accepting flag survives upgrade and lifecycle");
 // A bounded forced branch checks the *named unsupported destination*, not
 // guessed return behavior. The rest of the city must continue while held.
 auto held=*state;auto walker=decodeNativeWellWalker(held.figureRuntime->wellWalkerPayloads.front(),error);need(bool(walker),"live payload decodes");
 walker->action=1;walker->roamSteps=walker->roamLimit;const auto payload=encodeNativeWellWalker(*walker,error);need(bool(payload),"return-boundary payload encodes");held.figureRuntime->wellWalkerPayloads.front()=*payload;
 auto heldResult=transactNativeHouseRuntime(seed,held,{NativeSessionAction::Tick,held.revision},*src->runtime);need(heldResult.committed,"return boundary safely isolated:"+heldResult.error);need(held.heldWalkerReasons.count(walker->slot)==1,"return hold explicit");
 const auto frozen=held.figureRuntime->wellWalkerPayloads;const auto slots=held.figureRuntime->freeFigureSlots;
 for(int k=0;k<3;++k){auto r=transactNativeHouseRuntime(seed,held,{NativeSessionAction::Tick,held.revision},*src->runtime);need(r.committed,"city continues past held walker");}
 need(held.figureRuntime->wellWalkerPayloads==frozen&&held.figureRuntime->freeFigureSlots==slots,"held walker neither moves nor respawns");
 auto paused=transactNativeHouseRuntime(seed,held,{NativeSessionAction::Pause,held.revision},*src->runtime);need(paused.committed,"pause held city");
 const auto owner=uint64_t(walker->owner);NativeSessionCommand demolition{NativeSessionAction::Demolish,held.revision};demolition.objectId=owner;
 auto removed=transactNativeHouseRuntime(seed,held,demolition,*src->runtime);need(removed.committed,"normal Well removal reciprocal cleanup:"+removed.error);
 need(held.figureRuntime->wellWalkerPayloads.empty()&&held.heldWalkerReasons.empty()&&held.figureRuntime->freeFigureSlots.size()==1999,"Well cleanup removes worker and restores FIFO exactly once");
 for(auto route:held.figureRuntime->services.routeOwners)need(route!=int16_t(walker->slot),"no stale worker route owner");
 auto before=encodeNativeSession(seed,*state);auto copy=*state;auto bad=transactNativeHouseRuntime(seed,copy,{NativeSessionAction::Road,state->revision,-1,0},*src->runtime);if(bad.committed||encodeNativeSession(seed,copy)!=before)throw std::runtime_error("atomic failure");
 run(NativeSessionAction::Pause);
 if(argc>2){const std::filesystem::path output(argv[2]);need(output.is_absolute()&&std::filesystem::is_directory(output),"explicit test output directory");
  const auto bytes=encodeNativeSession(seed,*state);need(writeNativeSessionNew((output/"house-v8.encity").string(),root.configuredDirectory(),bytes,error),"unique outside-data checkpoint:"+error);
  auto read=readNativeSessionFile((output/"house-v8.encity").string(),root.configuredDirectory(),error);need(read&&*read==bytes,"actual file read exact");
  need(!writeNativeSessionNew((output/"house-v8.encity").string(),root.configuredDirectory(),bytes,error),"cannot overwrite checkpoint");
 }
 std::cout<<"{\"source\":"<<std::quoted(seed.source)<<",\"checks\":"<<checks<<",\"ticks\":"<<state->ticks<<",\"native_population\":"<<nativePopulation(*state)<<",\"max_walkers\":"<<maxWalkers<<",\"watered\":"<<(watered?"true":"false")<<",\"upgraded_type4\":"<<(upgraded?"true":"false")<<",\"held_house_paths\":"<<state->heldHouseReasons.size()<<",\"bytes\":"<<encodeNativeSession(seed,*state).size()<<",\"passed\":true}"<<std::endl;
 return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL:"<<e.what()<<std::endl;return 3;}}
