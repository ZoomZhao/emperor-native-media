#import <Foundation/Foundation.h>
#include "game_lab/movie_session.hpp"
#include "stream_lab/bink_process.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using Clock=std::chrono::steady_clock;
void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
bool spin(const std::function<bool()>& ready,double seconds){const auto deadline=Clock::now()+std::chrono::duration<double>(seconds);while(!ready()&&Clock::now()<deadline)CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.005,false);return ready();}
void header(const std::filesystem::path& file){std::array<uint8_t,44> h{};h[0]='B';h[1]='I';h[2]='K';h[3]='i';
    for(const auto& value:std::vector<std::pair<size_t,uint32_t>>{{4,36},{8,240},{20,13},{24,7},{28,30},{32,1}})
        for(size_t j=0;j<4;++j)h[value.first+j]=uint8_t(value.second>>(8*j));
    std::ofstream o(file,std::ios::binary);o.write(reinterpret_cast<const char*>(h.data()),h.size());o.close();need(bool(o),"own header fixture");}
int main(int argc,char** argv){@autoreleasepool{if(argc!=3)return 2;const auto root=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(root);header(root/"stall.bik");
    setenv("EMPEROR_FFMPEG_PATH",argv[2],1);setenv("PROBE_MODE","hang",1);
    setenv("PROBE_SIGNAL_LOG",(root/"child-signal-mask.json").c_str(),1);
    try{
        game_lab::MovieSession s(game_lab::ResourceRoot::fromAbsolute(root.string()));bool opened=false;
        s.openAndPlay("stall.bik",[&](const auto& d){opened=d.snapshot.state==media_demo::State::Playing;});need(spin([&]{return opened;},2),"first frame opens session");
        // Let frame 1 become due; worker now blocks on the deliberately stalled
        // self child. A second valid Tick is coalesced into the pending slot.
        std::this_thread::sleep_for(std::chrono::milliseconds(45));
        s.request(media_demo::Action::Tick,0,{});need(spin([&]{return s.stats()->active.load()==1;},1),"first Tick became active");
        s.request(media_demo::Action::Tick,0,{});
        const auto started=Clock::now();bool replied=false;game_lab::TerminationResult result;
        s.requestTermination(800,[&](const auto& r){result=r;replied=true;});
        need(spin([&]{return replied;},1.5),"termination has a bounded reply");
        const auto elapsed=std::chrono::duration<double>(Clock::now()-started).count();
        std::cout<<"{\"id\":\"active_tick_cancel_survives_pending_tick\",\"permits_exit\":"<<(result.permitsExit()?"true":"false")
                 <<",\"status\":"<<int(result.status)<<",\"seconds\":"<<elapsed<<",\"resources_zero\":"<<(result.resourcesZero?"true":"false")
                 <<",\"worker_idle\":"<<(result.workerIdle?"true":"false")<<"}\n";
        // Preserve the failed result but permit bounded worker cleanup before
        // leaving the probe; no background test children survive.
        need(spin([&]{return s.stats()->workerFinished.load()&&s.resourcesZero();},6),"worker eventually retires stalled child");
        return result.permitsExit()?0:1;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}}
