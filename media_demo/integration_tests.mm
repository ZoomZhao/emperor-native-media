#import <Foundation/Foundation.h>
#include "controller.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <deque>

namespace {
using namespace media_demo;
constexpr uint64_t total=384000,perFrame=1600;
size_t assertions=0,calls=0,samples=0,pausedChecks=0,zeroChecks=0;
uint64_t maxPhase=0;double maxLatency=0,eofSeconds=0;
std::ofstream trace;
void require(bool value,const std::string& message){++assertions;if(!value)throw std::runtime_error(message);}
void save(NSDictionary* value,const std::string& path){NSData* data=[NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
    require(data!=nil,"JSON encoding");require([data writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES],"JSON save");}
void delay(unsigned ms){std::this_thread::sleep_for(std::chrono::milliseconds(ms));}
void record(const Snapshot& s,const char* name){NSDictionary* row=@{@"step":[NSString stringWithUTF8String:name],@"state":[NSString stringWithUTF8String:stateName(s.state)],
    @"epoch":@(s.epoch),@"frame":@(s.frame),@"media_pcm":@(s.mediaPCM),@"relative_pcm":@(s.pcmFrames),@"base_pcm":@(s.basePCM),
    @"host_ns":@(s.clock.observedNS),@"anchor_ns":@(s.clock.anchorNS),@"anchor_pcm":@(s.clock.anchorSamples),@"running":@(s.clock.running),
    @"queue_frames":@(s.queuedVideo),@"video_bytes":@(s.videoBytes),@"PCM_sources":@(s.pcmSources),@"PCM_bytes":@(s.pcmBytes),
    @"readers":@(s.readers),@"native_PCM_bytes":@(s.nativePCMBytes),@"GPU_tickets":@(s.gpuTickets),@"GPU_bytes":@(s.gpuBytes),
    @"resources_zero":@(s.resourcesZero()),@"device_blocked":@(s.deviceBlocked),@"message":[NSString stringWithUTF8String:s.message.c_str()]};
    NSData* json=[NSJSONSerialization dataWithJSONObject:row options:NSJSONWritingSortedKeys error:nil];trace.write(static_cast<const char*>(json.bytes),json.length);trace<<'\n';trace.flush();
}
void check(const Snapshot& s,const char* name){record(s,name);++calls;require(s.state!=State::Failed,std::string(name)+": "+s.message);
    require(s.queuedVideo<=4&&s.videoBytes<=5*261*48,"video ownership bound");require(s.pcmSources<=2&&s.pcmBytes<=16384,"PCM ownership bound");
    require(s.readers<=1&&s.nativePCMBytes<=65536&&s.gpuTickets==0&&s.gpuBytes==0,"reader/GPU completion bound");
    if(s.state==State::Playing||s.state==State::Paused){require(s.mediaPCM<total,"active PCM bound");
        require(s.frame==s.mediaPCM/perFrame,"presented video PTS must equal PCM due frame");
        maxPhase=std::max(maxPhase,s.mediaPCM-s.frame*perFrame);require(s.mediaPCM-s.frame*perFrame<perFrame,"AV phase must be below one frame");}
    if(s.clock.running){const uint64_t elapsed=uint64_t(std::max(int64_t(0),s.clock.observedNS-s.clock.anchorNS));
        const auto expected=std::min(total-s.basePCM,s.clock.anchorSamples+std::min(elapsed,uint64_t(8000000000))*48000/1000000000);
        require(s.pcmFrames==expected,"PCM must follow real host elapsed time, not ticks");}
}
void zero(const Snapshot& s){require(s.resourcesZero(),"all controlled resources must be zero");++zeroChecks;}
void stable(const Snapshot& a,const Snapshot& b){require(a.mediaPCM==b.mediaPCM&&a.frame==b.frame&&a.epoch==b.epoch,"paused/lost position must stay exact");++pausedChecks;}
Snapshot act(Controller& controller,Action action,const char* name,uint64_t target=0){const auto before=HostClock::nowNS();auto s=controller.perform(action,target);const auto after=HostClock::nowNS();
    check(s,name);if(action==Action::Tick&&s.state==State::Playing){const double latency=double(after-before)/1000000;maxLatency=std::max(maxLatency,latency);
        require(latency<=100,"tick-to-readback latency must be <=100ms");require(s.clock.observedNS>=before&&s.clock.observedNS<=after,"clock sampled inside actual tick");}
    return s;}
}
int main(int argc,const char* argv[]){@autoreleasepool{
    std::thread([]{delay(55000);std::cerr<<"hard 55-second deadline\n";std::_Exit(124);}).detach();
    if(argc!=3)return 2;const std::string asset=argv[1],out=argv[2];std::filesystem::create_directories(out);trace.open(out+"/timeline.jsonl");
    const auto start=HostClock::nowNS();try{
        const auto fixed=std::filesystem::path(out).parent_path().parent_path().parent_path()/".generated/self_authored_8s.mov";
        require(std::filesystem::canonical(asset)==std::filesystem::canonical(fixed),"only the verified self-authored fixture is allowed");
        Controller controller(asset,[](uint64_t first,const std::vector<int16_t>& pcm){require(!pcm.empty()&&pcm.size()%2==0,"PCM observer format");
            for(size_t i=0;i<pcm.size();++i){const auto expected=int16_t(int(((first+i/2)*37+(i%2)*103)&0x7fff)-16384);
                require(pcm[i]==expected,"decoded PCM must match self-authored absolute sample");}samples+=pcm.size();});
        std::mutex mutex;std::deque<DeviceEvent> events;std::vector<DeviceEvent> delivered;
        DeviceMonitor devices([&](DeviceEvent event){std::lock_guard<std::mutex> lock(mutex);events.push_back(event);});
        auto drain=[&](const char* label){std::deque<DeviceEvent> pending;{std::lock_guard<std::mutex> lock(mutex);pending.swap(events);}
            require(!pending.empty(),"device event delivered");Snapshot s;for(const auto event:pending){delivered.push_back(event);s=controller.deviceChanged(event);check(s,label);}return s;};
        const auto initial=devices.stats();require(initial.activeListeners==3&&initial.listenerStatus==0&&initial.readStatus==0,"real CoreAudio read/three listener registrations");
        require(initial.current.device!=0&&initial.current.available,"existing output device readable without opening IO");drain("native-initial");
        auto s=act(controller,Action::Tick,"idle");zero(s);delay(120);auto t=act(controller,Action::Tick,"idle-later");stable(s,t);zero(t);
        s=act(controller,Action::Play,"play");require(s.frame==0&&s.mediaPCM==0,"play starts at zero");
        for(const auto ms:{17,80,215,430,37}){const auto previous=s;delay(ms);s=act(controller,Action::Tick,"irregular-tick");
            require(s.mediaPCM>=previous.mediaPCM+uint64_t(ms)*48,"delayed tick must catch up actual elapsed time");}
        s=act(controller,Action::Pause,"pause");require(s.state==State::Paused&&!s.clock.running,"pause freezes host clock");delay(200);t=act(controller,Action::Tick,"paused-later");stable(s,t);
        act(controller,Action::Play,"resume");delay(85);s=act(controller,Action::Tick,"resumed-tick");require(s.mediaPCM>=t.mediaPCM+85*48,"resume elapsed time");
        act(controller,Action::Pause,"pause-for-seek");s=act(controller,Action::Seek,"paused-seek",120);require(s.state==State::Paused&&s.mediaPCM==192000,"paused seek exact target");
        delay(150);t=act(controller,Action::Tick,"paused-seek-later");stable(s,t);act(controller,Action::Play,"play-after-seek");
        s=act(controller,Action::Seek,"playing-seek",30);require(s.state==State::Playing&&s.mediaPCM==48000,"playing seek rebuild exact target");delay(120);act(controller,Action::Tick,"seek-tick");
        s=act(controller,Action::Pause,"pause-before-device");const double changedRate=initial.current.nominalRate==44100?48000:44100;
        DeviceEvent ready{initial.current.device,changedRate,true,0,DeviceOrigin::Injected};devices.inject(ready);t=drain("injected-format-change-paused");
        require(t.state==State::Paused&&t.mediaPCM==s.mediaPCM&&t.epoch>s.epoch,"paused device rebuild preserves exact fractional PCM");
        delay(150);s=act(controller,Action::Tick,"rebuilt-paused-later");stable(t,s);const auto opens=s.stats.runtimeOpens;
        devices.inject(ready);t=drain("duplicate-device");require(t.stats.runtimeOpens==opens&&t.stats.duplicateDeviceEvents>=1,"duplicate event must not rebuild");
        auto stale=delivered.back();t=controller.deviceChanged(stale);check(t,"stale-device");require(t.stats.staleDeviceEvents==1,"stale device event discarded");
        DeviceEvent lost{0,0,false,0,DeviceOrigin::Injected};devices.inject(lost);s=drain("loss-while-paused");zero(s);require(s.deviceBlocked&&s.state==State::Paused,"lost device pause and release");
        delay(150);t=act(controller,Action::Tick,"lost-paused-later");stable(s,t);devices.inject(ready);t=drain("restore-paused");require(t.state==State::Paused&&t.mediaPCM==s.mediaPCM,"restore respects user pause");
        act(controller,Action::Play,"play-before-loss");delay(73);act(controller,Action::Tick,"before-loss-tick");devices.inject(lost);s=drain("loss-while-playing");zero(s);
        delay(200);t=act(controller,Action::Tick,"lost-playing-later");stable(s,t);ready.device+=100000;ready.nominalRate=96000;devices.inject(ready);t=drain("restore-playing-new-device");
        require(t.state==State::Playing&&t.mediaPCM==s.mediaPCM&&!t.deviceBlocked,"recovery resumes exact frozen sample");delay(100);act(controller,Action::Tick,"recovered-tick");
        devices.inject(lost);drain("loss-before-user-pause");act(controller,Action::Pause,"user-pause-while-lost");
        s=act(controller,Action::Seek,"seek-while-lost",75);require(s.mediaPCM==120000&&!s.bgra.empty()&&s.deviceBlocked,"lost-device seek previews exact target");zero(s);
        devices.inject(ready);t=drain("restore-after-user-pause");require(t.state==State::Paused&&t.mediaPCM==s.mediaPCM,"explicit pause cancels auto-resume after loss");
        act(controller,Action::Play,"play-after-user-pause");
        devices.inject(lost);drain("loss-before-stop");s=act(controller,Action::Stop,"stop-while-lost");zero(s);const auto stoppedOpens=s.stats.runtimeOpens;
        devices.inject(ready);s=drain("restore-after-stop");require(s.state==State::Ready&&s.stats.runtimeOpens==stoppedOpens,"stop cancels automatic recovery");zero(s);
        for(unsigned i=0;i<3;++i){act(controller,Action::Play,"resource-cycle-play");delay(40);act(controller,Action::Tick,"resource-cycle-tick");
            if(i==1)act(controller,Action::Pause,"resource-cycle-pause");act(controller,Action::Seek,"resource-cycle-seek",60+i*40);
            if(i==2){devices.inject(lost);s=drain("resource-cycle-loss");zero(s);devices.inject(ready);drain("resource-cycle-recovery");}
            s=act(controller,Action::Stop,"resource-cycle-stop");zero(s);}
        const size_t beforeFull=samples;s=act(controller,Action::Play,"full-play");const auto fullStart=s.clock.anchorNS;unsigned index=0;
        while(s.state==State::Playing){delay(index++==25?430:unsigned(19+(index%4)*14));s=act(controller,Action::Tick,"full-tick");require(HostClock::nowNS()-fullStart<9150000000LL,"full clip deadline");}
        eofSeconds=double(HostClock::nowNS()-fullStart)/1000000000;require(s.state==State::Ended&&s.mediaPCM==total&&s.frame==239,"EOF exact duration and last frame");
        require(eofSeconds>=8&&eofSeconds<=8.15,"8-second clip must complete within 8.15 real seconds");require(samples-beforeFull==total*2,"full clip all PCM samples exactly once");zero(s);
        s=act(controller,Action::Play,"replay");require(s.frame==0&&s.mediaPCM==0,"replay rebuild from zero");controller.close();s=controller.snapshot();check(s,"close-active");zero(s);
        controller.close();zero(controller.snapshot());require(s.stats.runtimeOpens==s.stats.runtimeReleases,"every runtime session released");
        devices.close();const auto final=devices.stats();require(final.activeListeners==0&&final.removeStatus==0&&final.closed,"native listeners unregister and fence");
        const auto count=delivered.size();devices.inject(ready);delay(100);{std::lock_guard<std::mutex> lock(mutex);require(events.empty(),"closed monitor cannot publish late events");}
        require(delivered.size()==count&&devices.stats().lateEventsSuppressed>=1,"late injected event suppressed");
        const auto stats=s.stats;
        save(@{@"passed":@YES,@"real_monotonic_clock":@YES,@"real_sound":@NO,@"real_device_switch_induced":@NO,@"device_recovery_injected":@YES,
            @"calls":@(calls),@"assertions":@(assertions),@"PCM_samples_checked":@(samples),@"max_AV_phase_samples":@(maxPhase),@"AV_phase_limit_samples":@1600,
            @"max_tick_readback_ms":@(maxLatency),@"tick_readback_limit_ms":@100,@"full_clip_seconds":@(eofSeconds),@"full_clip_limit_seconds":@8.15,
            @"pause_freeze_checks":@(pausedChecks),@"resource_zero_checks":@(zeroChecks),@"runtime_opens":@(stats.runtimeOpens),@"runtime_releases":@(stats.runtimeReleases),
            @"max_video_bytes":@(stats.maxVideoBytes),@"max_PCM_sources":@(stats.maxPCMSources),@"max_PCM_bytes":@(stats.maxPCMBytes),@"max_readers":@(stats.maxReaders),
            @"max_GPU_buffer_bytes":@(stats.maxGPUBytes),@"max_catchup_frames":@(stats.maxCatchupFrames),@"discarded_video_packets":@(stats.discardedVideo),
            @"clock_rebuilds":@(stats.clockRebuilds),@"device_recoveries":@(stats.deviceRecoveries),@"native_device_events":@(final.nativeEvents),@"injected_device_events":@(final.injectedEvents),
            @"native_property_listeners_registered":@(initial.activeListeners),@"listeners_after_close":@(final.activeListeners),@"process_seconds":@(double(HostClock::nowNS()-start)/1e9)},out+"/summary.json");
        std::cout<<"PASS real-clock silent media integration, injected recovery and zero-resource close\n";return 0;
    }catch(const std::exception& error){save(@{@"passed":@NO,@"error":[NSString stringWithUTF8String:error.what()],@"calls":@(calls),@"assertions":@(assertions),@"PCM_samples_checked":@(samples)},out+"/summary.json");
        std::cerr<<error.what()<<"\n";return 1;}
}}
