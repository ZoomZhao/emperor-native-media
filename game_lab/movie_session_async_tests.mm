#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#include "game_lab/app_media.hpp"
#include "game_lab/movie_session.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace std::chrono;
using media_demo::Action;using media_demo::State;
static size_t checks=0;static double maxOwnerMs=0;
static void need(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
static void waitFor(const std::function<bool()>& predicate,const char* why,bool runMain=true,double seconds=6){
 auto deadline=steady_clock::now()+duration<double>(seconds);
 while(!predicate()&&steady_clock::now()<deadline){if(runMain)CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.005,false);std::this_thread::sleep_for(milliseconds(1));}
 need(predicate(),why);
}
static double immediate(const std::function<void()>& action){auto start=steady_clock::now();action();const auto elapsed=duration<double,std::milli>(steady_clock::now()-start).count();maxOwnerMs=std::max(maxOwnerMs,elapsed);return elapsed;}
class NoAudio:public game_lab::AudioFactory {public:size_t calls=0;std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>&)override{++calls;throw std::runtime_error("unexpected device audio");}};
class View:public game_lab::ImagePresenter {public:size_t presented=0,cleared=0;bool fail=false;uint32_t width=0,height=0;std::vector<uint32_t> pixels;
 void clear()override{++cleared;pixels.clear();}bool present(uint32_t w,uint32_t h,const std::vector<uint32_t>& p)override{
  need(p.size()==size_t(w)*h,"packed pixels wrong");if(fail)return false;++presented;width=w;height=h;pixels=p;return true;}};
int main(int argc,const char* argv[]){@autoreleasepool{try{
 need(argc==2,"fresh output argument");std::filesystem::path work(argv[1]);need(work.is_absolute()&&!std::filesystem::exists(work),"fresh owned output");
 auto root=work/"root";std::filesystem::create_directories(root/"Movies");
 stream_lab::writeSyntheticMovie((root/"Movies/own.mov").string());std::ofstream(root/"Movies/bad.mov")<<"self-authored malformed MOV";
 NoAudio audio;game_lab::AppMedia app(game_lab::ResourceRoot::fromAbsolute(root.string()),audio);View view;
 auto session=app.createMovieSession();auto probe=session->stats();size_t old=0,current=0;game_lab::MovieDelivery delivered;
 session->setTestDelayMs(500);
 need(immediate([&]{session->openAndPlay("Movies/own.mov",[&](const auto&){++old;},&view);})<100,"open blocked owner");
 waitFor([&]{return probe->delayEntered.load();},"worker not entered controlled delay",false);
 auto burstStart=steady_clock::now();
 for(int i=0;i<64;++i) need(immediate([&]{session->openAndPlay("Movies/own.mov",[&](const auto& d){++current;delivered=d;},&view);})<100,"replacement blocked owner");
 need(probe->active==1&&probe->pending<=1,"burst backlog not bounded");
 waitFor([&]{return current==1;},"latest replacement not delivered");
 need(duration<double>(steady_clock::now()-burstStart).count()<3,"overwritten work still accumulated");
 need(old==0&&current==1&&delivered.accepted&&delivered.snapshot.state==State::Playing,"latest-only load wrong");
 need(delivered.presented&&view.width==32&&view.height==24&&delivered.snapshot.readers==1,"existing native Controller not consumed");
 need(probe->maxActive==1&&probe->maxPending==1&&probe->maxDelivery==1,"declared queue bound wrong");
 session->setTestDelayMs(0);bool done=false;
 session->request(Action::Stop,0,[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"stop delivery missing");need(delivered.snapshot.resourcesZero()&&session->resourcesZero(),"stop cached release wrong");
 // Hold a real native metadata open, then cancel it using the production close.
 session->holdMetadataForTest(true);done=false;session->openAndPlay("Movies/own.mov",[&](const auto&){done=true;},&view);
 waitFor([&]{return probe->active==1;},"metadata job not active",false);std::this_thread::sleep_for(milliseconds(30));
 bool closed=false;std::string closeError;
 need(immediate([&]{session->closeAsync([&](bool zero,const std::string& e){closed=zero;closeError=e;});})<100,"metadata close blocked owner");
 waitFor([&]{return closed;},"metadata cancellation failed close");need(!done&&closeError.empty()&&probe->resourcesZero&&!probe->hasController,"late load survived close");
 session->holdMetadataForTest(false);
 // Completed worker delivery remains queued, close invalidates it before main consumption.
 session->openAndPlay("Movies/own.mov",[&](const auto&){++old;},&view);waitFor([&]{return probe->delivery==1&&probe->active==0;},"queued result not held",false);
 closed=false;session->closeAsync([&](bool zero,const std::string&){closed=zero;});waitFor([&]{return closed;},"queued close incomplete");need(old==0,"late main delivery accessed old presenter");
 size_t firstClose=0,lastClose=0;session->closeAsync([&](bool,const std::string&){++firstClose;});session->closeAsync([&](bool zero,const std::string& e){need(zero&&e.empty(),"repeat close not idempotent");++lastClose;});
 waitFor([&]{return lastClose==1;},"repeat close missing");need(firstClose==0&&!probe->hasController,"old close callback survived generation");
 // Error detail is consumed through the same status function used by App.
 done=false;session->openAndPlay("Movies/bad.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"bad decode no delivery");
 need(delivered.snapshot.state==State::Failed&&!delivered.errorMessage.empty()&&probe->resourcesZero,"bad replacement lacks reason/cleanup");
 need(game_lab::formatMovieStatus(delivered).find(delivered.errorMessage)!=std::string::npos&&view.pixels.empty(),"App formatter erased Reader reason");
 done=false;session->openAndPlay("../outside.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"path denial missing");need(delivered.errorKind==game_lab::MovieErrorKind::PathSecurity,"path reason lost");
 done=false;session->openAndPlay("Movies/missing.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"missing path delivery missing");need(delivered.errorKind==game_lab::MovieErrorKind::ResourceUnavailable,"missing file reason lost");
 done=false;session->openAndPlay("Movies/own.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"good replacement did not recover");need(delivered.snapshot.state==State::Playing&&delivered.presented,"good recovery failed");
 session->failNextCloseForTest();done=false;session->closeAsync([&](bool zero,const std::string& e){need(!zero&&e=="self-authored close fault","close fault hidden or marked zero");done=true;});waitFor([&]{return done;},"close fault not returned");need(!probe->resourcesZero&&probe->hasController,"close error faked cleanup");
 closed=false;session->closeAsync([&](bool zero,const std::string& e){closed=zero&&e.empty();});waitFor([&]{return closed;},"close retry failed");
 session->setTestDelayMs(500);done=false;need(immediate([&]{session->requestPoster("Movies/own.mov",[&](const auto& d){delivered=d;done=true;},&view);})<100,"poster decoded on owner");
 waitFor([&]{return done;},"actual async poster missing");need(delivered.poster&&delivered.presented&&view.width==64&&view.height==48&&probe->resourcesZero&&!probe->hasController,"poster did not use fixed existing Reader contract");
 session->setTestDelayMs(0);done=false;session->requestPoster("Movies/bad.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"poster error missing");need(!delivered.errorMessage.empty()&&delivered.snapshot.state==State::Failed&&view.pixels.empty(),"poster swallowed native reason");
 view.fail=true;done=false;session->requestPoster("Movies/own.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"presenter fault missing");need(delivered.errorKind==game_lab::MovieErrorKind::PresenterFailure&&!game_lab::formatMovieStatus(delivered).empty(),"presenter reason not shared");view.fail=false;
 session->holdMetadataForTest(true);done=false;session->requestPoster("Movies/own.mov",[&](const auto& d){delivered=d;done=true;},&view);waitFor([&]{return done;},"metadata deadline not delivered",true,4);need(delivered.errorKind==game_lab::MovieErrorKind::MetadataTimeout&&probe->resourcesZero,"metadata timeout reason wrong");session->holdMetadataForTest(false);
 // Close and Stop bursts while a controlled 500ms replacement is active.
 session->setTestDelayMs(500);done=false;session->openAndPlay("Movies/own.mov",[&](const auto&){done=true;},&view);
 waitFor([&]{return probe->delayEntered.load();},"close delay gate not entered",false);
 for(int i=0;i<32;++i)session->request(Action::Stop,0,[&](const auto&){done=true;},&view);
 closed=false;need(immediate([&]{session->closeAsync([&](bool zero,const std::string& e){closed=zero&&e.empty();});})<100,"500ms worker close blocked owner");
 need(!probe->resourcesZero,"requested close reported premature cleanup");waitFor([&]{return closed;},"blocked close failed cleanup");need(!done&&probe->maxPending==1,"Stop burst delivered obsolete callbacks or queued work");
 session->setTestDelayMs(0);
 // Front destruction returns while a 500ms worker is active. Shared probe must
 // prove subsequent worker cleanup; merely suppressing a callback is insufficient.
 auto disposable=app.createMovieSession();auto death=disposable->stats();bool primed=false;
 disposable->openAndPlay("Movies/own.mov",[&](const auto& d){primed=d.snapshot.state==State::Playing;},&view);waitFor([&]{return primed;},"live destructor setup failed");need(death->hasController&&!death->resourcesZero,"destructor setup lacks real native resources");
 disposable->setTestDelayMs(500);size_t deadCalls=0;
 disposable->openAndPlay("Movies/own.mov",[&](const auto&){++deadCalls;},&view);waitFor([&]{return death->delayEntered.load();},"destructor worker gate missing",false);
 need(immediate([&]{disposable.reset();})<100,"front destructor blocked owner");
 waitFor([&]{return death->workerFinished&&death->resourcesZero&&!death->hasController;},"worker did not confirm destruction cleanup");need(deadCalls==0,"destroyed front delivered callback");
 closed=false;session->closeAsync([&](bool zero,const std::string& e){closed=zero&&e.empty();});waitFor([&]{return closed;},"final cleanup not confirmed");session.reset();waitFor([&]{return probe->workerFinished&&probe->resourcesZero;},"final destructor cleanup missing");
 need(audio.calls==0,"device audio factory used");std::ofstream(work/"async-observations.json")<<"{\"max_owner_call_ms\":"<<maxOwnerMs<<",\"burst_replace_count\":64,\"burst_stop_count\":32,\"max_worker\":"<<probe->maxActive<<",\"max_pending\":"<<probe->maxPending<<",\"max_delivery\":"<<probe->maxDelivery<<",\"final_resources_zero\":true,\"destroyed_live_resources_zero\":true}\n";std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"synthetic_MOV\":true,\"UI\":false,\"audio_devices\":false}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}}
