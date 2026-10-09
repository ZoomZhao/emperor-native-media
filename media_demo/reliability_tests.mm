#import <Foundation/Foundation.h>
#include "controller.hpp"
#include <algorithm>
#include <mach/mach.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
using namespace media_demo;
std::string fixtures;
std::ofstream timeline;size_t assertions=0,checkedSamples=0,calls=0,zeroChecks=0;double maxLatency=0;uint64_t maxPhase=0;
void require(bool value,const std::string& message){++assertions;if(!value)throw std::runtime_error(message);}
void save(NSDictionary* object,const std::string& path){NSData* data=[NSJSONSerialization dataWithJSONObject:object options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
    require(data&&[data writeToFile:[NSString stringWithUTF8String:path.c_str()] atomically:YES],"save evidence");}
uint64_t footprint(){task_vm_info_data_t data{};mach_msg_type_number_t count=TASK_VM_INFO_COUNT;
    require(task_info(mach_task_self(),TASK_VM_INFO,reinterpret_cast<task_info_t>(&data),&count)==KERN_SUCCESS,"own physical footprint read");return data.phys_footprint;}
void check(const Snapshot& s,const std::string& name){++calls;NSDictionary* row=@{@"name":[NSString stringWithUTF8String:name.c_str()],@"state":[NSString stringWithUTF8String:stateName(s.state)],
    @"frame":@(s.frame),@"PCM":@(s.mediaPCM),@"epoch":@(s.epoch),@"host_ns":@(s.clock.observedNS),@"resources_zero":@(s.resourcesZero()),
    @"queue":@(s.queuedVideo),@"video_bytes":@(s.videoBytes),@"PCM_sources":@(s.pcmSources),@"PCM_bytes":@(s.pcmBytes),@"native_PCM_bytes":@(s.nativePCMBytes),
    @"message":[NSString stringWithUTF8String:s.message.c_str()]};NSData* json=[NSJSONSerialization dataWithJSONObject:row options:NSJSONWritingSortedKeys error:nil];
    timeline.write(static_cast<const char*>(json.bytes),json.length);timeline<<'\n';timeline.flush();
    require(s.queuedVideo<=4&&s.videoBytes<=62640&&s.pcmSources<=2&&s.pcmBytes<=16384&&s.readers<=1&&s.nativePCMBytes<=65536,"unchanged ownership budgets");
    require(s.gpuTickets==0&&s.gpuBytes==0,"readback completion resources");
    if(s.state==State::Playing||s.state==State::Paused){require(s.frame==s.mediaPCM/1600,"actual PTS equals PCM due frame");
        const auto phase=s.mediaPCM-s.frame*1600;require(phase<1600,"unchanged sub-frame phase bound");maxPhase=std::max(maxPhase,phase);}}
void zero(const Snapshot& s){require(s.resourcesZero(),"all owned media resources zero");require(s.stats.runtimeOpens==s.stats.runtimeReleases,"including failed open attempts released");++zeroChecks;}
Snapshot action(Controller& c,Action a,const std::string& label,uint64_t frame=0){auto s=c.perform(a,frame);check(s,label);return s;}
void pcm(uint64_t first,const std::vector<int16_t>& bytes){for(size_t i=0;i<bytes.size();++i){const auto sample=(first+i/2)%384000;
    require(bytes[i]==int16_t(int((sample*37+(i%2)*103)&0x7fff)-16384),"real decoded self-authored PCM content");}checkedSamples+=bytes.size();}
}
int main(int argc,const char* argv[]){@autoreleasepool{
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(50));std::cerr<<"hard deadline\n";std::_Exit(124);}).detach();
    if(argc!=4)return 2;const std::string mode=argv[1],out=argv[2];fixtures=argv[3];std::filesystem::create_directories(out);timeline.open(out+"/timeline.jsonl");
    const auto start=HostClock::nowNS();try{
        const auto generated=std::filesystem::path(out).parent_path().parent_path().parent_path()/".generated/reliability-fixtures";
        require(std::filesystem::canonical(fixtures)==std::filesystem::canonical(generated),"only this checkout generated fixtures");
        if(mode=="long"){
            Controller c(fixtures+"/self_authored_32s.mov",pcm,960);auto s=action(c,Action::Play,"long-play");require(s.state==State::Playing&&s.mediaPCM==0,"long fixture actual open");
            const auto origin=s.clock.anchorNS;bool stalled=false;uint64_t low=UINT64_MAX,high=0,early=0,late=0;unsigned index=0;
            while(s.state==State::Playing){const double elapsed=double(HostClock::nowNS()-origin)/1e9;
                if(elapsed>=12&&!stalled){std::this_thread::sleep_for(std::chrono::seconds(2));stalled=true;}
                else std::this_thread::sleep_for(std::chrono::milliseconds(19+(index++%4)*14));
                const auto before=HostClock::nowNS();s=action(c,Action::Tick,"long-tick");const double latency=double(HostClock::nowNS()-before)/1e6;
                require(s.state!=State::Failed,"actual long-media failure: "+s.message);maxLatency=std::max(maxLatency,latency);require(latency<=100,"unchanged <=100ms delayed tick readback");
                require(HostClock::nowNS()-origin<33000000000LL,"long playback deadline");
                if(s.state==State::Playing){const auto expected=s.clock.anchorSamples+uint64_t(s.clock.observedNS-s.clock.anchorNS)*48000/1000000000;
                    require(s.pcmFrames==expected,"long timeline must follow actual system monotonic clock");}
                if(elapsed>3){const auto bytes=footprint();low=std::min(low,bytes);high=std::max(high,bytes);if(!early)early=bytes;late=bytes;}
            }
            const double seconds=double(HostClock::nowNS()-origin)/1e9;require(stalled&&s.state==State::Ended&&s.mediaPCM==1536000&&s.frame==959,"32-second EOF exact");
            require(seconds>=32&&seconds<=32.15,"duration+0.15s threshold unchanged");require(checkedSamples==3072000,"all long PCM samples consumed once");
            require(s.stats.maxCatchupFrames>=96000,"actual 2-second delayed recovery");zero(s);
            save(@{@"passed":@YES,@"seconds":@(seconds),@"duration_seconds":@32,@"end_limit_seconds":@32.15,@"PCM_samples_checked":@(checkedSamples),
                @"max_AV_phase_samples":@(maxPhase),@"phase_limit_samples":@1600,@"max_tick_ms":@(maxLatency),@"tick_limit_ms":@100,
                @"max_catchup_frames":@(s.stats.maxCatchupFrames),@"max_video_bytes":@(s.stats.maxVideoBytes),@"max_PCM_sources":@(s.stats.maxPCMSources),
                @"max_PCM_bytes":@(s.stats.maxPCMBytes),@"max_readers":@(s.stats.maxReaders),@"max_GPU_buffer_bytes":@(s.stats.maxGPUBytes),
                @"footprint_low":@(low),@"footprint_high":@(high),@"footprint_early":@(early),@"footprint_late":@(late),@"RSS_observation_only":@YES,
                @"resources_zero_at_EOF":@YES,@"calls":@(calls),@"process_seconds":@(double(HostClock::nowNS()-start)/1e9),@"real_sound":@NO},out+"/summary.json");
        }else if(mode=="metadata"){
            Controller c(fixtures+"/valid8.mov",pcm);
            auto token=std::make_shared<stream_lab::OpenControl>();token->holdMetadataCompletionForTest=true;
            auto began=HostClock::nowNS();auto s=c.perform(Action::Play,0,token);check(s,"metadata-timeout");
            const double timeoutMS=double(HostClock::nowNS()-began)/1e6;
            require(s.state==State::Failed&&s.message.find("metadata load timeout (2000 ms)")!=std::string::npos,"visible bounded metadata timeout");
            require(timeoutMS>=2000&&timeoutMS<=2200&&s.stats.metadataTimeouts==1,"whole metadata load <=2200ms including cleanup");zero(s);
            s=action(c,Action::Stop,"timeout-stop");zero(s);
            token=std::make_shared<stream_lab::OpenControl>();token->holdMetadataCompletionForTest=true;
            std::thread cancellation([token]{std::this_thread::sleep_for(std::chrono::milliseconds(100));token->cancelled.store(true,std::memory_order_release);});
            began=HostClock::nowNS();s=c.perform(Action::Play,0,token);cancellation.join();check(s,"metadata-cancelled");
            const double cancelMS=double(HostClock::nowNS()-began)/1e6;
            require(s.state==State::Failed&&s.message.find("metadata load cancelled")!=std::string::npos,"cancelled open reports actual cancellation path");
            require(cancelMS>=100&&cancelMS<=300&&s.stats.metadataCancellations==1&&s.stats.metadataTimeouts==1,"cancellation observed <=200ms after request");zero(s);
            s=action(c,Action::Play,"metadata-retry-normal");require(s.state==State::Playing,"fresh token retries actual valid asset after timeout/cancel");
            s=action(c,Action::Stop,"metadata-final-stop");zero(s);c.close();zero(c.snapshot());
            save(@{@"passed":@YES,@"timeout_limit_ms":@2000,@"timeout_total_ms":@(timeoutMS),@"cancel_request_at_ms":@100,@"cancel_total_ms":@(cancelMS),
                @"metadata_timeouts":@(s.stats.metadataTimeouts),@"metadata_cancellations":@(s.stats.metadataCancellations),@"resources_zero_checks":@(zeroChecks),
                @"fresh_token_retry_actual_media":@YES,@"controlled_completion_gate":@YES,@"external_path_fixed":@NO,
                @"process_seconds":@(double(HostClock::nowNS()-start)/1e9),@"real_sound":@NO},out+"/summary.json");
        }else if(mode=="faults"){
            NSMutableArray* cases=[NSMutableArray array];
            for(const auto* name:{"missing","empty","invalid","truncated","corrupt","latecorrupt"}){
                const auto path=fixtures+"/"+name+".mov";Controller c(path,pcm);auto s=action(c,Action::Play,std::string(name)+"-play");
                if(std::string(name)=="latecorrupt"&&s.state==State::Playing)s=action(c,Action::Seek,"late-payload-seek",210);
                require(s.state==State::Failed&&s.clearImage&&!s.message.empty(),std::string(name)+": actual malformed media must fail visibly");zero(s);
                [cases addObject:@{@"case":[NSString stringWithUTF8String:name],@"message":[NSString stringWithUTF8String:s.message.c_str()],@"resources_zero":@YES}];
                s=action(c,Action::Stop,std::string(name)+"-stop");require(s.state==State::Ready,"stop after failure");zero(s);c.close();zero(c.snapshot());}
            const auto repair=fixtures+"/repair.mov";std::filesystem::copy_file(fixtures+"/truncated.mov",repair,std::filesystem::copy_options::overwrite_existing);
            Controller c(repair,pcm);auto s=action(c,Action::Play,"repair-bad");require(s.state==State::Failed,"actual file initially broken");zero(s);
            std::filesystem::copy_file(fixtures+"/valid8.mov",repair,std::filesystem::copy_options::overwrite_existing);
            s=action(c,Action::Play,"repair-valid");require(s.state==State::Playing&&s.frame==0,"same path recovers after actual file repair");
            std::this_thread::sleep_for(std::chrono::milliseconds(60));s=action(c,Action::Tick,"repair-tick");require(s.state==State::Playing&&s.mediaPCM>=2880,"repaired reader advances");
            s=action(c,Action::Stop,"repair-stop");zero(s);
            // Reopen the real Reader/GPU/PCM owners repeatedly, alternating active and paused close.
            for(unsigned i=0;i<8;++i){Controller cycle(fixtures+"/valid8.mov",pcm);s=action(cycle,Action::Play,"repeat-open");
                require(s.state==State::Playing,"reopened reader works");if(i%2)s=action(cycle,Action::Pause,"repeat-pause");
                cycle.close();check(cycle.snapshot(),"repeat-close");zero(cycle.snapshot());}
            save(@{@"passed":@YES,@"actual_bad_media_cases":cases,@"repaired_same_path_reopened":@YES,@"continuous_reopen_cycles":@8,@"resource_zero_checks":@(zeroChecks),
                @"calls":@(calls),@"PCM_samples_checked":@(checkedSamples),@"process_seconds":@(double(HostClock::nowNS()-start)/1e9),@"real_sound":@NO},out+"/summary.json");
        }else throw std::runtime_error("unknown mode");
        std::cout<<"PASS reliability "<<mode<<'\n';return 0;
    }catch(const std::exception& error){save(@{@"passed":@NO,@"error":[NSString stringWithUTF8String:error.what()],@"calls":@(calls),@"assertions":@(assertions)},out+"/summary.json");
        std::cerr<<error.what()<<'\n';return 1;}
}}
