// SPDX-License-Identifier: MIT
#include "city_playback.hpp"
#include "../original_world_checkpoint_test_fixture.hpp"
#include <cstdlib>
#include <sys/wait.h>
using namespace game_lab;using namespace game_lab::world_test;
int main(int argc,char** argv){try{
 const char* configured=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!configured||!*configured){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\",\"scope\":\"real world disk restore\"}\n";return 0;}
 need(argc>=3,"output directory and explicit source or--child required");const auto root=ResourceRoot::fromAbsolute(configured);const auto output=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(output);std::string error;
 if(std::string(argv[2])=="--child"){
  need(argc==4,"child has only an explicit checkpoint file");const auto bytes=readOriginalWorldCheckpointFile(argv[3],configured,error);need(bool(bytes),error.c_str());const auto saved=decodeOriginalWorldCheckpoint(*bytes,error);need(bool(saved),error.c_str());
  AppMapSurface surface(root);AppCitySessionState session;auto player=AppCityPlayback::fromWorldSnapshot(surface,*saved,0,&session,error);need(bool(player),error.c_str());const auto captured=player->worldSnapshot(error);need(bool(captured),error.c_str());need(originalWorldPersistentEqual(*saved,*captured,error),"fresh process full persistent world equals disk value");
  need(player->status().state==AppCityPlaybackState::Paused&&player->status().experiment==saved->policy.experimental,"child resumes paused withsame provenance");
  const bool stepped=player->step(0);const auto after=player->worldSnapshot(error);need(bool(after),error.c_str());const auto encoded=encodeOriginalWorldCheckpoint(*after,error);need(bool(encoded),error.c_str());
  std::ofstream file(output/"child-next.bin",std::ios::binary);file.write(encoded->data(),std::streamsize(encoded->size()));need(bool(file),"child writes own next-state comparison artifact");
  std::cout<<"{\"child\":true,\"step\":"<<(stepped?"true":"false")<<",\"checks\":"<<checks<<"}\n";return 0;
 }
 if(std::string(argv[2])=="--base"||std::string(argv[2])=="--retired"){
  unsigned sources=0,actors=0;const bool retired=std::string(argv[2])=="--retired";
  for(int n=3;n<argc;++n){
   const auto bound=bindOriginalWorldSource(root,argv[n],error);need(bool(bound),error.c_str());auto s=fixture();s.source=bound->binding;
   for(const auto& actor:bound->save.layers.figures->slots)s.city.figures.figures.at(actor.slot)=actor;s.city.figures.freeSlots.clear();s.city.figures.highestLiveSlot=0;
   for(const auto& [slot,f]:s.city.figures.figures){if(f.alive)s.city.figures.highestLiveSlot=slot;else if(slot)s.city.figures.freeSlots.push_back(slot);}
   emperor::file_io::research::GoalLimits limits;limits.enable_homage=limits.enable_trading=limits.enable_yearly=limits.enable_population=true;
   const auto goals=emperor::file_io::research::walk_city19_housing_goals({bound->payload.data(),std::min(size_t(16384),bound->payload.size())},limits);
   const auto pool=evidenceFigurePrefix(bound->payload,bound->save.buildings.tableEnd,bound->save.terrainOffset,goals.cache.size()+4000+bound->save.buildings.declaredClasses);
   need(pool.complete&&pool.completed==2000,"real source has no unknown variable-payload shift");
   for(const auto& f:pool.records)if(f.field(22).value_or(0)&&(f.field(18)==24||f.field(18)==25)){s.marketBases.emplace(f.slot,OriginalMarketKnownBase{bound->save.snapshotRevision,f});++actors;}
   const auto ids=OriginalKernelIdentities::loaded(s.city.figures.figures,error);need(bool(ids),error.c_str());s.ledger=ids->persistent();s.continuation.initialFigures=s.city.figures;
   if(retired){need(s.marketBases.empty(),"retired endpoint has no live24/25");need(!s.city.figures.figures.at(1936).alive&&s.city.figures.figures.at(1936).follower==1998,"real inactive follower residue retained");}
   const auto path=output/("base-"+std::to_string(sources)+".emworld");const auto written=writeOriginalWorldCheckpointNew(path.string(),configured,s);need(written.state!=OriginalWorldPublication::NotPublished,written.error.c_str());
   const auto bytes=readOriginalWorldCheckpointFile(path.string(),configured,error);need(bool(bytes),error.c_str());const auto restored=decodeOriginalWorldCheckpoint(*bytes,error);need(bool(restored),error.c_str());need(originalWorldPersistentEqual(s,*restored,error),"all represented raw/typed slot fields survive actual disk");
   if(!retired){const auto pass=originalFigureOrderedNextPass(restored->city.figures,{});need(!pass.complete&&!pass.context,"raw preservation never grants business qualification");}
   ++sources;
  }
  std::cout<<"{\"passed\":true,\"sources\":"<<sources<<",\"live_market_bases\":"<<actors<<",\"retired_endpoint\":"<<(retired?"true":"false")<<",\"scope\":\"complete slot/base values in authored world; not full App admission\",\"checks\":"<<checks<<"}\n";return 0;
 }
 unsigned sources=0,crossProcess=0,failureOracles=0,boundaries=0;
 for(int n=2;n<argc;++n){
  const auto bound=bindOriginalWorldSource(root,argv[n],error);need(bool(bound),error.c_str());AppCityPlaybackPreferences prefs;need(readAppCityPlaybackPreferences(root,prefs,error),error.c_str());
  AppMapSurface surface(root);AppCitySessionState session;session.crtRandom=17;
  AppCityPlayback player(surface,bound->save,bound->payload,prefs,17,0,true,&session,false,&*bound);need(player.prepared(),surface.cityPlaybackError().c_str());
  // N0 has a legitimate prepared-input/context lag; preserve both roles.
  const auto initial=player.worldSnapshot(error);need(bool(initial),error.c_str());const auto initialBytes=encodeOriginalWorldCheckpoint(*initial,error);need(bool(initialBytes),error.c_str());
  auto loaded=AppCityPlayback::fromWorldSnapshot(surface,*initial,0,&session,error);need(bool(loaded),error.c_str());const auto initialAgain=loaded->worldSnapshot(error);need(bool(initialAgain)&&originalWorldPersistentEqual(*initial,*initialAgain,error),"N0 source binding andinput lag roundtrip withoutAfterLoad");
  need(loaded->step(0),loaded->status().refusal.c_str());const auto first=loaded->worldSnapshot(error);need(bool(first),error.c_str());need(first->continuation.updates==1,"first supported labelled step once");
  const auto path=output/("source-"+std::to_string(sources)+".emworld");const auto written=writeOriginalWorldCheckpointNew(path.string(),configured,*first);need(written.state!=OriginalWorldPublication::NotPublished,written.error.c_str());
  const auto disk=readOriginalWorldCheckpointFile(path.string(),configured,error);need(bool(disk),error.c_str());const auto decoded=decodeOriginalWorldCheckpoint(*disk,error);need(bool(decoded),error.c_str());need(originalWorldPersistentEqual(*first,*decoded,error),"actual file full canonical equality");
  auto restored=AppCityPlayback::fromWorldSnapshot(surface,*decoded,0,&session,error);need(bool(restored),error.c_str());
  const auto fifo=output/("app-fifo-"+std::to_string(sources)+".emworld");need(mkfifo(fifo.c_str(),0600)==0,"own FIFO input forlive oldworld test");
  const auto oldWorld=restored->worldSnapshot(error);need(bool(oldWorld),error.c_str());OriginalWorldSaveReceipt receipt;need(receipt.acknowledge(written,first->commitRevision,surface.worldRevision()),"save receipt currentbeforeFIFO");const auto oldReceipt=receipt.savedRevision;
  const auto started=std::chrono::steady_clock::now();const auto rejected=readOriginalWorldCheckpointFile(fifo.string(),configured,error);const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
  need(!rejected&&error=="world_io_read_bounds"&&elapsed<1000,"actualoldworld FIFO load promptlyrefuses");const auto afterFIFO=restored->worldSnapshot(error);need(bool(afterFIFO)&&originalWorldPersistentEqual(*oldWorld,*afterFIFO,error)&&receipt.savedRevision==oldReceipt,"FIFO rejection preserves all actualoldworld fields andsaved receipt");
  const auto before=restored->worldSnapshot(error);need(bool(before),error.c_str());
  for(const char* stage:{"world_before_publish","before_pixels","before_publish"}){
   const auto handle=surface.cityFigureHandle(bound->save.layers.figures->live.empty()?0:bound->save.layers.figures->live.front().slot);
   surface.setCityPublicationProbe([&](const char* point){if(std::string(point)==stage)throw std::runtime_error("authored late failure");});
   if(std::string(stage)=="world_before_publish")need(!surface.restoreWorldSnapshot(*decoded,error),"late restore failure refuses");else need(!restored->step(0),"late update failure refuses");
   const auto unchanged=surface.worldSnapshot(error);need(bool(unchanged)&&originalWorldPersistentEqual(*before,*unchanged,error),"every persistent field unchanged after failed candidate");
   need(!handle||surface.cityFigureHandleCurrent(*handle),"failed candidate retains old actor handle");surface.setCityPublicationProbe({});restored=AppCityPlayback::fromWorldSnapshot(surface,*decoded,0,&session,error);need(bool(restored),error.c_str());++failureOracles;
  }
  // Separate self-authored process reads the disk file, never original UI/game.
  const auto childOutput=output/("child-"+std::to_string(sources));std::filesystem::create_directory(childOutput);
  const auto pid=fork();need(pid>=0,"spawn own test subprocess");if(pid==0){execl(argv[0],argv[0],childOutput.c_str(),"--child",path.c_str(),static_cast<char*>(nullptr));_exit(42);}int status=0;need(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"fresh process disk restore passes");
  need(restored->step(0),restored->status().refusal.c_str());const auto next=restored->worldSnapshot(error);need(bool(next),error.c_str());const auto bytes=encodeOriginalWorldCheckpoint(*next,error);need(bool(bytes),error.c_str());
  std::vector<uint8_t> childBytes;need(detail::readFile((childOutput/"child-next.bin").string(),originalWorldFileLimit,childBytes),"child next-state file");need(*bytes==std::string(childBytes.begin(),childBytes.end()),"fresh process same logical next input gives identical whole canonical world");
  ++crossProcess;
  // Restoring a declared-premise world must replace receiver provenance.
  AppCityPlayback strict(surface,bound->save,bound->payload,prefs,17,0,false,&session,false,&*bound,true);need(strict.prepared(),"strict source prepares");strict.play();need(!strict.step(0)&&strict.status().refusal=="event_ui_5eff20_not_closed","strict unknown event gate remains refused");
  auto exp=AppCityPlayback::fromWorldSnapshot(surface,*first,0,&session,error);need(bool(exp)&&exp->status().experiment,"restore retains declared-premise label");auto forged=*first;forged.policy.experimental=false;need(!surface.restoreWorldSnapshot(forged,error)&&error=="world_snapshot_policy_promotion","savefile cannot launderzero gates asordinary");
  if(sources==0){
   // Full represented-world comparison across actual supported phase edges.
   // A before/after pair is obtained by executing the same next logical input
   // on the uninterrupted and restored owners, never by shifting the calendar.
   AppMapSurface traceSurface(root);AppCitySessionState traceSession;
   auto trace=AppCityPlayback::fromWorldSnapshot(traceSurface,*first,0,&traceSession,error);need(bool(trace),error.c_str());
   const std::set<unsigned> selected{0,15,16,17,30,31,34,35,38,39,42,43,44,45,49,50};
   for(unsigned i=0;i<51;++i){const unsigned phase=(unsigned(first->city.phase)+i)%51;
    if(selected.count(phase)){
     const auto captured=trace->worldSnapshot(error);need(bool(captured)&&captured->city.phase==phase,"actual phase matches trace");
     const auto target=output/("boundary-"+std::to_string(phase)+".emworld");const auto published=writeOriginalWorldCheckpointNew(target.string(),configured,*captured);need(published.state!=OriginalWorldPublication::NotPublished,published.error.c_str());
     const auto wire=readOriginalWorldCheckpointFile(target.string(),configured,error);need(bool(wire),error.c_str());const auto state=decodeOriginalWorldCheckpoint(*wire,error);need(bool(state),error.c_str());
     AppMapSurface other(root);AppCitySessionState otherSession;auto mirror=AppCityPlayback::fromWorldSnapshot(other,*state,0,&otherSession,error);need(bool(mirror),error.c_str());
     const bool a=trace->step(0),b=mirror->step(0);need(a==b&&trace->status().refusal==mirror->status().refusal,"phase-edge next result/refusal equal");
     const auto x=trace->worldSnapshot(error),y=mirror->worldSnapshot(error);need(bool(x)&&bool(y)&&originalWorldPersistentEqual(*x,*y,error),"phase-edge entire persistent world equals uninterrupted continuation");++boundaries;
    }else need(trace->step(0),trace->status().refusal.c_str());
   }
   // Host zero/wrap and paused host gaps must not become offline catch-up.
   auto near=*first;near.continuation.admission=UINT32_MAX-2;near.continuation.session.admissionClock=near.continuation.admission;
   auto clock=AppCityPlayback::fromWorldSnapshot(traceSurface,near,0,&traceSession,error);need(bool(clock),error.c_str());
   need(!clock->tick(999999),"paused host elapsed cannot advance");clock->play();need(!clock->tick(999999),"first resumed tick reanchors");
   need(clock->tick(999999+clock->status().intervalMs),"logical admission crossesuint32 wrap once");const auto after=clock->worldSnapshot(error);need(bool(after)&&after->continuation.updates==near.continuation.updates+1,"wrap performsone update only");
   const auto revision=traceSurface.worldRevision();OriginalMapCamera camera;camera.width=128;camera.height=96;camera.centerX=camera.centerY=1;
   const auto view=clock->render(camera);need(view.image.valid()&&traceSurface.worldRevision()>revision,"draw-only camera change invalidates earlier revision");
   const auto carried=traceSession;AppCityPlayback fresh(traceSurface,bound->save,bound->payload,prefs,123,traceSession.admissionClock,false,&traceSession,false,&*bound);
   need(fresh.prepared()&&traceSession.crtRandom==carried.crtRandom&&traceSession.randomHistory.cursor==carried.randomHistory.cursor,"SAV load afterdiskrestore carriesprocess session");
  }
  ++sources;std::cout<<"{\"source_completed\":"<<sources<<",\"failure_oracles\":"<<failureOracles<<",\"cross_process\":"<<crossProcess<<"}\n"<<std::flush;
 }
 std::cout<<"{\"passed\":true,\"real_sources\":"<<sources<<",\"fullworld_failure_oracles\":"<<failureOracles<<",\"fresh_process_disk_cases\":"<<crossProcess<<",\"phase_edge_disk_cases\":"<<boundaries<<",\"checks\":"<<checks<<",\"original_game_started\":false}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
