#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#include "game_lab/movie_session.hpp"
#include "fake_audio_hooks.hpp"
#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
using namespace std::chrono;
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void waitFor(const std::function<bool()>& p){auto end=steady_clock::now()+seconds(4);while(!p()&&steady_clock::now()<end){CFRunLoopRunInMode(kCFRunLoopDefaultMode,.002,false);std::this_thread::sleep_for(milliseconds(1));}need(p(),"bounded wait expired");}
static void configure(const std::filesystem::path& work,const char* mode,unsigned id){
 need(setenv("REVIEW_CHILD_MODE",mode,1)==0,"owned decoder mode");
 auto gate=work/("gate-"+std::to_string(id));need(!std::filesystem::exists(gate),"fresh gate required");
 need(setenv("REVIEW_GATE",gate.c_str(),1)==0,"owned gate environment");
}
static void pixels(const game_lab::MovieDelivery& d){if(!d.pixelsBGRA.empty())need(d.pixelsBGRA.size()==1&&d.pixelsBGRA[0]==(0xff000000u|uint32_t(d.snapshot.frame)),"BGRA frame bytes no longer match their original PTS");}
static void drain(game_lab::MovieSession& session,uint64_t videoBase){
 game_lab::MovieDelivery d;
 for(unsigned i=0;i<40;++i){reviewConsumeOne();d=session.performSync(media_demo::Action::Tick);need(d.snapshot.state!=media_demo::State::Failed,"continued decoder failed");pixels(d);if(d.snapshot.state==media_demo::State::Ended)break;}
 need(d.snapshot.state==media_demo::State::Ended&&d.snapshot.resourcesZero(),"continued stream did not drain/close");
 need(d.snapshot.audioDecodedFrames==44100&&d.snapshot.audioSubmittedFrames==44100&&d.snapshot.audioReturnedFrames==44100,"PCM sample counts changed across interruption");
 need(d.snapshot.stats.decodedVideoFrames-videoBase==30,"BGRA frame count changed across interruption");
 need(reviewLiveQueues()==0&&reviewLastPCMValid()&&reviewLastPCMFrames()==44100,"queued PCM prefixes/channels are corrupt, missing or duplicated");
}
int main(int argc,const char* argv[]){@autoreleasepool{try{
 need(argc==2,"fixture directory required");std::filesystem::path work(argv[1]);
 unsigned failures=0;for(unsigned trial=0;trial<5;++trial){
 configure(work,"hold",trial);reviewResetCounters();
 game_lab::MovieSession session(game_lab::ResourceRoot::fromAbsolute((work/"root").string()));
 try {
 need(session.openMovie("own.bik"),"own synthetic header open failed");
 auto play=session.performSync(media_demo::Action::Play);need(play.snapshot.state==media_demo::State::Playing,"initial play failed");
 need(play.snapshot.nativeAudio&&reviewLiveQueues()==1,"fake queue not used");
 auto idle=session.performSync(media_demo::Action::Pause);need(idle.snapshot.state==media_demo::State::Paused,"idle pause failed");
 auto resumed=session.performSync(media_demo::Action::Play);need(resumed.snapshot.state==media_demo::State::Playing,"idle resume failed");
 need(reviewConsumeOne(),"cannot return one synthetic queue buffer");
 bool oldTickDelivered=false;session.request(media_demo::Action::Tick,0,[&](const auto&){oldTickDelivered=true;});
 auto stats=session.stats();waitFor([&]{return stats->active==1;});std::this_thread::sleep_for(milliseconds(75));
 bool paused=false;game_lab::MovieDelivery result;
 session.request(media_demo::Action::Pause,0,[&](const auto& d){result=d;paused=true;});waitFor([&]{return paused;});
 if(result.snapshot.state!=media_demo::State::Paused)++failures;
 need(result.snapshot.state==media_demo::State::Paused&&!oldTickDelivered&&reviewLiveQueues()==1&&!result.snapshot.resourcesZero(),"independent blocked-read Pause reproduction regressed");
 session.close();need(session.resourcesZero()&&reviewLiveQueues()==0,"close leaked fake queue");
 }catch(...){session.close();throw;}
 }
 unsigned lifecycle=0,identity=10;
 for(const char* mode:{"pcm","video"})for(unsigned operation=0;operation<9;++operation){
 configure(work,mode,identity++);reviewResetCounters();
 game_lab::MovieSession session(game_lab::ResourceRoot::fromAbsolute((work/"root").string()));
 try {
 need(session.openMovie("own.bik"),"continuation header open");auto first=session.performSync(media_demo::Action::Play);pixels(first);
 need(reviewConsumeOne(),"return one buffer before partial read");bool oldDelivered=false;
 session.request(media_demo::Action::Tick,0,[&](const auto&){oldDelivered=true;});auto stats=session.stats();waitFor([&]{return stats->active==1;});std::this_thread::sleep_for(milliseconds(75));
 if(operation<3||operation>=6){bool ready=false;game_lab::MovieDelivery paused;
  session.request(media_demo::Action::Pause,0,[&](const auto& d){paused=d;ready=true;});waitFor([&]{return ready;});
  need(paused.snapshot.state==media_demo::State::Paused&&paused.snapshot.positionSeconds>0&&reviewCreatedQueues()==1,"partial read Pause lost position/queue");pixels(paused);
  need(!reviewConsumeOne(),"Pause only changed a label while queue kept running");auto frozen=session.performSync(media_demo::Action::Tick);
  need(frozen.snapshot.positionSeconds==paused.snapshot.positionSeconds&&frozen.snapshot.frame==paused.snapshot.frame,"paused continuation changed position/frame");
 }
 bool secondStale=false;
 if(operation>=6){
  bool resumed=false;
  session.request(media_demo::Action::Play,0,[&](const auto&){if(std::string(mode)=="video")secondStale=true;resumed=true;});
  if(std::string(mode)=="pcm"){
   waitFor([&]{return resumed;});
   session.request(media_demo::Action::Tick,0,[&](const auto&){secondStale=true;});
  }
  waitFor([&]{return stats->active==1;});std::this_thread::sleep_for(milliseconds(75));
  if(operation==8){bool paused=false;game_lab::MovieDelivery d;
   session.request(media_demo::Action::Pause,0,[&](const auto& value){d=value;paused=true;});waitFor([&]{return paused;});
   need(d.snapshot.state==media_demo::State::Paused&&!secondStale&&reviewCreatedQueues()==1,"second Pause discarded continuation or reopened queue");
  }
 }
 game_lab::MovieDelivery playing;
 if(operation==0||operation==8){std::ofstream gate(std::getenv("REVIEW_GATE"));gate<<"continue";gate.close();playing=session.performSync(media_demo::Action::Play);need(reviewCreatedQueues()==1,"Resume reopened the queue");}
 else if(operation==1||operation==3||operation==6){bool done=false;game_lab::MovieDelivery stopped;session.request(media_demo::Action::Stop,0,[&](const auto& d){stopped=d;done=true;});waitFor([&]{return done;});need(stopped.snapshot.state==media_demo::State::Ready&&stopped.snapshot.resourcesZero(),"Stop did not clear partial reads/queue");
  need(setenv("REVIEW_CHILD_MODE","flow",1)==0,"replay mode");playing=session.performSync(media_demo::Action::Play);}
 else if(operation==2||operation==4||operation==7){bool closed=false;session.closeAsync([&](bool zero,const std::string& error){need(zero&&error.empty(),"Close failed");closed=true;});waitFor([&]{return closed;});need(session.resourcesZero()&&reviewLiveQueues()==0,"Close retained pending packet/queue");
  need(setenv("REVIEW_CHILD_MODE","flow",1)==0,"replay mode");need(session.openMovie("own.bik"),"reopen after Close");playing=session.performSync(media_demo::Action::Play);}
 else {need(setenv("REVIEW_CHILD_MODE","flow",1)==0,"replacement mode");bool ready=false;session.openAndPlay("own.bik",[&](const auto& d){playing=d;ready=true;});waitFor([&]{return ready;});}
 need(playing.snapshot.state==media_demo::State::Playing&&!oldDelivered&&!secondStale,"continuation or replacement state/stale delivery");pixels(playing);
 if(operation&&operation!=8)need(playing.snapshot.frame==0&&playing.snapshot.audioClockFrames==0,"Replay/replacement retained old byte position");
 const auto base=playing.snapshot.stats.decodedVideoFrames-(playing.snapshot.frame+1);
 drain(session,base);
 need(reviewMaximumLiveQueues()==1&&reviewCreatedQueues()==reviewDisposedQueues(),"queue replacement leaked or overlapped native ownership");
 // Ended -> Replay must also start from a clean stream and close normally.
 need(setenv("REVIEW_CHILD_MODE","flow",1)==0,"ended replay mode");auto replay=session.performSync(media_demo::Action::Play);pixels(replay);need(replay.snapshot.frame==0&&replay.snapshot.audioClockFrames==0,"Ended Replay did not reset origins");
 session.performSync(media_demo::Action::Stop);session.close();need(session.resourcesZero()&&reviewLiveQueues()==0&&reviewCreatedQueues()==reviewDisposedQueues(),"Replay/Stop/Close queue lifetime mismatch");++lifecycle;
 static const char* names[]={"Pause-Resume-EOF","Pause-Stop-Replay","Pause-Close-Replay","blocked-Stop-Replay","blocked-Close-Replay","blocked-replacement","Pause-blockedResume-Stop-Replay","Pause-blockedResume-Close-Replay","Pause-blockedResume-Pause-Resume"};
 std::cout<<"{\"partial_stream\":\""<<mode<<"\",\"operation\":\""<<names[operation]<<"\",\"passed\":true}\n";
 }catch(...){session.close();throw;}
 }
 std::cout<<"{\"blocked_pause_passed\":"<<5-failures<<",\"blocked_pause_trials\":5,\"partial_stream_lifecycle_passed\":"<<lifecycle<<",\"partial_stream_lifecycle_cases\":18,\"PCM_and_BGRA_continuity\":true,\"UI\":false,\"audio_devices\":false,\"original_data\":false}\n";
 return failures||lifecycle!=18?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}}
