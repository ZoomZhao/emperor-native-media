#import <Foundation/Foundation.h>
#include "controller.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mach/mach.h>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace {
using namespace media_demo;
constexpr uint64_t growthGuard=16*1024*1024;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
uint64_t footprint(){task_vm_info_data_t data{};mach_msg_type_number_t count=TASK_VM_INFO_COUNT;
    require(task_info(mach_task_self(),TASK_VM_INFO,reinterpret_cast<task_info_t>(&data),&count)==KERN_SUCCESS,"own physical footprint read");return data.phys_footprint;}
void save(NSDictionary* value,const std::filesystem::path& path){NSData* bytes=[NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
    require(bytes&&[bytes writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES],"write replay summary");}
void bounds(const Snapshot& s){require(s.state!=State::Failed,"native playback failed");
    require(s.queuedVideo<=4&&s.videoBytes<=62640&&s.pcmSources<=2&&s.pcmBytes<=16384&&s.readers<=1&&s.nativePCMBytes<=65536,"owned queue budgets");
    require(s.gpuTickets==0&&s.gpuBytes==0,"readback completion released");
    if(s.state==State::Playing)require(s.frame==s.mediaPCM/1600,"PTS and PCM phase");}
void zero(const Snapshot& s){require(s.resourcesZero(),"controlled media resources zero");
    require(s.stats.runtimeOpens==s.stats.runtimeReleases,"every opened session released");}
}

int main(int argc,const char* argv[]){@autoreleasepool{
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(50));std::_Exit(124);}).detach();
    if(argc!=4)return 2;
    const std::filesystem::path root=argv[1],out=argv[2];
    const auto began=HostClock::nowNS();NSMutableArray* records=[NSMutableArray array];size_t samples=0;double maxTickMS=0;
    try{
        const auto cycleCount=std::stoul(argv[3]);require(cycleCount>=2&&cycleCount<=4,"2..4 bounded cycles");
        require(std::filesystem::canonical(out.parent_path())==std::filesystem::canonical(root/".local-results"),"own replay result directory");
        const auto fixture=std::filesystem::canonical(root/".generated/self_authored_8s.mov");
        std::ofstream timeline(out/"timeline.jsonl");require(bool(timeline),"open timeline");
        Controller controller(fixture.string(),[&](uint64_t first,const std::vector<int16_t>& pcm){
            require(!pcm.empty()&&pcm.size()%2==0,"PCM stereo format");
            for(size_t i=0;i<pcm.size();++i)require(pcm[i]==int16_t(int(((first+i/2)*37+(i%2)*103)&0x7fff)-16384),"self-authored PCM content");
            samples+=pcm.size();});
        uint64_t baseline=0,peak=0;int64_t growth=0;
        for(size_t cycle=1;cycle<=cycleCount;++cycle){@autoreleasepool{
            const size_t samplesBefore=samples;
            auto s=controller.perform(Action::Play);bounds(s);require(s.state==State::Playing&&s.mediaPCM==0,"fresh cycle begins at zero");
            const auto origin=s.clock.anchorNS;uint64_t cyclePeak=footprint();
            while(s.state==State::Playing){
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                const auto tick=HostClock::nowNS();s=controller.perform(Action::Tick);
                const double ms=double(HostClock::nowNS()-tick)/1e6;maxTickMS=std::max(maxTickMS,ms);
                require(ms<=100,"unchanged 100ms tick limit");bounds(s);
                require(HostClock::nowNS()-origin<=8150000000LL,"8.15s replay deadline");
                cyclePeak=std::max(cyclePeak,footprint());
                timeline<<"{\"cycle\":"<<cycle<<",\"pid\":"<<getpid()<<",\"PCM\":"<<s.mediaPCM<<",\"frame\":"<<s.frame<<",\"resources_zero\":"<<(s.resourcesZero()?"true":"false")<<"}\n";
            }
            const double seconds=double(HostClock::nowNS()-origin)/1e9;
            require(s.state==State::Ended&&s.mediaPCM==384000&&s.frame==239&&seconds>=8&&seconds<=8.15,"complete 8s cycle EOF");
            require(samples-samplesBefore==768000,"every PCM sample consumed once per cycle");zero(s);
            s=controller.perform(Action::Stop);require(s.state==State::Ready&&s.mediaPCM==0,"stop resets for replay");zero(s);
            const auto late=footprint();if(!baseline)baseline=late;growth=int64_t(late)-int64_t(baseline);peak=std::max(peak,cyclePeak);
            require(growth<=int64_t(growthGuard),"same-process post-release memory growth guard");
            [records addObject:@{@"cycle":@(cycle),@"pid":@(getpid()),@"EOF_seconds":@(seconds),@"resources_zero":@YES,
                @"footprint_peak":@(cyclePeak),@"footprint_after_stop":@(late),@"growth_from_first_release":@(growth),
                @"runtime_opens":@(s.stats.runtimeOpens),@"runtime_releases":@(s.stats.runtimeReleases)}];
        }}
        controller.close();const auto final=controller.snapshot();zero(final);
        require(final.stats.runtimeOpens==cycleCount,"one media session per full cycle");timeline.flush();require(bool(timeline),"complete timeline");
        save(@{@"passed":@YES,@"same_process":@YES,@"pid":@(getpid()),@"controller_instances":@1,@"cycles_completed":@(cycleCount),
            @"playback_seconds":@(cycleCount*8),@"process_seconds":@(double(HostClock::nowNS()-began)/1e9),@"records":records,
            @"PCM_samples_checked":@(samples),@"max_tick_ms":@(maxTickMS),@"footprint_peak":@(peak),@"growth_bytes":@(growth),
            @"growth_guard_bytes":@(growthGuard),@"memory_observation_only":@YES,@"hours_verified":@NO,@"real_sound":@NO,
            @"runtime_opens":@(final.stats.runtimeOpens),@"runtime_releases":@(final.stats.runtimeReleases),@"all_resources_zero":@YES},out/"summary.json");
        std::cout<<"PASS bounded same-process replay\n";return 0;
    }catch(const std::exception& error){save(@{@"passed":@NO,@"same_process":@YES,@"pid":@(getpid()),@"cycles_completed":@(records.count),
        @"error":[NSString stringWithUTF8String:error.what()],@"records":records},out/"summary.json");std::cerr<<error.what()<<'\n';return 1;}
}}
