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
using namespace std::chrono;using game_lab::TerminationStatus;using game_lab::TerminationResult;
static size_t checks=0;static double maxOwnerMs=0;
static void need(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
static void waitFor(const std::function<bool()>& p,const char* why,bool main=true,double seconds=4){
 auto deadline=steady_clock::now()+duration<double>(seconds);
 while(!p()&&steady_clock::now()<deadline){if(main)CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.005,false);std::this_thread::sleep_for(milliseconds(1));}
 need(p(),why);
}
static double fast(const std::function<void()>& f){auto start=steady_clock::now();f();auto ms=duration<double,std::milli>(steady_clock::now()-start).count();maxOwnerMs=std::max(maxOwnerMs,ms);return ms;}
class NoAudio:public game_lab::AudioFactory{public:size_t calls=0;std::unique_ptr<game_lab::AudioPlayback> create(const std::vector<uint8_t>&)override{++calls;throw std::runtime_error("device audio requested");}};
static bool idle(const std::shared_ptr<const game_lab::MovieSessionStats>& p){return p->workerFinished&&p->active==0&&p->pending==0&&p->delivery==0;}
static void prime(game_lab::MovieSession& s){bool got=false;s.openAndPlay("Movies/own.mov",[&](const auto& d){got=d.snapshot.state==media_demo::State::Playing&&d.snapshot.readers==1&&!d.snapshot.bgra.empty();});waitFor([&]{return got;},"real Controller/Reader did not prime");need(s.hasActiveController()&&!s.resourcesZero(),"no native resources before termination");}
static void completed(const TerminationResult& r,const std::shared_ptr<const game_lab::MovieSessionStats>& p){
 need(r.status==TerminationStatus::Completed&&r.permitsExit()&&r.closeObserved,"success did not permit exit");
 need(r.resourcesZero&&r.workerIdle&&idle(p)&&p->resourcesZero&&!p->hasController,"success before worker/reader/PCM/GPU quiescence");need(p->terminationTimers==0,"success left timer active");}
int main(int argc,const char* argv[]){@autoreleasepool{try{
 need(argc==2,"fresh output argument");std::filesystem::path out(argv[1]);need(out.is_absolute()&&!std::filesystem::exists(out),"output must be fresh");auto root=out/"root";std::filesystem::create_directories(root/"Movies");stream_lab::writeSyntheticMovie((root/"Movies/own.mov").string());
 NoAudio audio;game_lab::AppMedia app(game_lab::ResourceRoot::fromAbsolute(root.string()),audio);
 // One pending result, one original deadline/callback, and a retained live Controller.
 auto s=app.createMovieSession();auto p=s->stats();prime(*s);s->setTestDelayMs(500);size_t staleFrames=0;
 s->openAndPlay("Movies/own.mov",[&](const auto&){++staleFrames;});waitFor([&]{return p->delayEntered.load();},"500ms worker hold missing",false);
 size_t replies=0,duplicates=0;TerminationResult result;const auto before=p->workerEnqueues.load();
 need(fast([&]{need(s->requestTermination(1500,[&](const auto& r){result=r;++replies;})==TerminationStatus::Pending,"request did not start");})<100,"termination blocked owner on decoding");
 for(int i=0;i<64;++i)need(s->requestTermination(60000,[&](const auto&){++duplicates;})==TerminationStatus::Pending,"duplicate request altered pending result");
 need(p->workerEnqueues==before+1&&p->terminationAttempts==1&&p->terminationTimers==1&&p->maxTerminationTimers==1,"duplicate exits accumulated work/callbacks/timers");
 need(!p->resourcesZero,"close request prematurely reported zero");
 s->openAndPlay("Movies/own.mov",[&](const auto&){++staleFrames;});s->request(media_demo::Action::Stop,0,[&](const auto&){++staleFrames;});need(p->workerEnqueues==before+1,"media commands replaced pending termination");
 waitFor([&]{return replies==1;},"success reply missing");completed(result,p);need(duplicates==0&&staleFrames==0&&p->terminationReplies==1,"duplicate/stale callback survived exit");
 need(s->requestTermination(1,[&](const auto&){++duplicates;})==TerminationStatus::Completed,"completed exit was repeated");
 const auto settled=p->workerEnqueues.load();s.reset();need(p->workerEnqueues==settled&&idle(p),"completed front destructor re-enqueued work");
 // Cancel a real AVFoundation metadata hold through the same termination request.
 s=app.createMovieSession();p=s->stats();s->holdMetadataForTest(true);size_t late=0;s->openAndPlay("Movies/own.mov",[&](const auto&){++late;});waitFor([&]{return p->active==1&&p->hasController;},"native metadata hold not active",false);std::this_thread::sleep_for(milliseconds(30));
 replies=0;need(fast([&]{s->requestTermination(1500,[&](const auto& r){result=r;++replies;});})<100,"metadata cancellation blocked exit caller");waitFor([&]{return replies==1;},"metadata cancel exit missing");completed(result,p);need(late==0,"cancelled metadata load delivered");s.reset();
 // A completed native frame queued on owner must not appear after exit.
 s=app.createMovieSession();p=s->stats();late=0;s->openAndPlay("Movies/own.mov",[&](const auto&){++late;});waitFor([&]{return p->workerFinished&&p->delivery==1;},"queued owner frame not observed",false);
 replies=0;s->requestTermination(1500,[&](const auto& r){result=r;++replies;});waitFor([&]{return replies==1;},"queued-frame exit missing");completed(result,p);need(late==0,"late frame survived exit");s.reset();
 // The real close failure reason reaches the shared App formatter; retry closes.
 s=app.createMovieSession();p=s->stats();prime(*s);s->failNextCloseForTest();replies=0;s->requestTermination(1500,[&](const auto& r){result=r;++replies;});waitFor([&]{return replies==1;},"failure reply missing");
 need(result.status==TerminationStatus::Failed&&!result.permitsExit()&&result.reason=="self-authored close fault"&&!result.resourcesZero&&p->hasController,"close failure hidden or permitted exit");need(game_lab::formatTerminationResult(result).find(result.reason)!=std::string::npos&&p->terminationTimers==0,"shared error display/timer incorrect");
 s->requestTermination(1500,[&](const auto& r){result=r;++replies;});waitFor([&]{return replies==2;},"failure retry missing");completed(result,p);need(p->terminationAttempts==2&&p->maxTerminationTimers==1,"retry created multiple timers");s.reset();
 // Timeout preserves live-resource facts and original deadline despite repeats.
 s=app.createMovieSession();p=s->stats();prime(*s);s->setTestDelayMs(500);s->openAndPlay("Movies/own.mov",[&](const auto&){++late;});waitFor([&]{return p->delayEntered.load();},"timeout worker gate missing",false);
 replies=0;duplicates=0;auto start=steady_clock::now();s->requestTermination(40,[&](const auto& r){result=r;++replies;});std::this_thread::sleep_for(milliseconds(25));
 for(int i=0;i<64;++i)s->requestTermination(60000,[&](const auto&){++duplicates;});
 waitFor([&]{return replies==1;},"timeout result missing");const auto timeoutMs=duration<double,std::milli>(steady_clock::now()-start).count();need(timeoutMs>=40&&timeoutMs<180,"duplicate requests reset deadline");
 need(result.status==TerminationStatus::TimedOut&&!result.permitsExit()&&!result.resourcesZero&&!result.workerIdle&&!result.closeObserved&&p->hasController,"timeout faked resource cleanup");need(p->terminationTimers==0&&duplicates==0&&p->terminationAttempts==1,"timeout leaked timer or duplicate reply");
 waitFor([&]{return idle(p)&&p->resourcesZero&&!p->hasController;},"late native cleanup not completed");need(s->terminationResult().status==TerminationStatus::TimedOut&&replies==1,"late close changed timeout outcome");
 s->requestTermination(1500,[&](const auto& r){result=r;++replies;});waitFor([&]{return replies==2;},"timeout retry missing");completed(result,p);need(p->terminationAttempts==2&&p->maxActive==1&&p->maxPending==1&&p->maxDelivery==1,"retry bypassed worker bounds");s.reset();
 // Owner delays consumption past the deadline; a previously queued completion
 // must not report an on-time success merely because it precedes the timer event.
 s=app.createMovieSession();p=s->stats();replies=0;s->requestTermination(20,[&](const auto& r){result=r;++replies;});waitFor([&]{return p->workerFinished&&p->resourcesZero&&p->delivery==1;},"close did not queue before deadline",false);std::this_thread::sleep_for(milliseconds(80));
 waitFor([&]{return replies==1;},"delayed owner result missing");need(result.status==TerminationStatus::TimedOut&&!result.permitsExit()&&result.resourcesZero&&result.workerIdle,"late owner consumption ignored deadline");s->requestTermination(1500,[&](const auto& r){result=r;++replies;});waitFor([&]{return replies==2;},"delayed owner retry missing");completed(result,p);s.reset();
 // Pending destruction cancels the single timer and suppresses all owner calls,
 // while the existing worker still releases actual live native resources.
 s=app.createMovieSession();p=s->stats();prime(*s);s->setTestDelayMs(500);size_t dead=0;s->openAndPlay("Movies/own.mov",[&](const auto&){++dead;});waitFor([&]{return p->delayEntered.load();},"destructor hold missing",false);
 s->requestTermination(1500,[&](const auto&){++dead;});need(fast([&]{s.reset();})<100,"pending exit destructor blocked owner");need(p->terminationTimers==0,"destructor left owner timer alive");waitFor([&]{return idle(p)&&p->resourcesZero&&!p->hasController;},"destroyed pending exit did not release resources");need(dead==0,"destroyed owner received result");need(audio.calls==0,"exit opened device audio");
 std::ofstream(out/"termination-observations.json")<<"{\"max_owner_call_ms\":"<<maxOwnerMs<<",\"original_timeout_ms\":40,\"observed_timeout_ms\":"<<timeoutMs<<",\"duplicate_requests\":64,\"max_worker\":"<<p->maxActive<<",\"max_pending\":"<<p->maxPending<<",\"max_delivery\":"<<p->maxDelivery<<",\"max_timers\":"<<p->maxTerminationTimers<<",\"actual_resources_zero\":true}\n";
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"UI\":false,\"audio_devices\":false,\"actual_termination_consumer\":true}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}}
