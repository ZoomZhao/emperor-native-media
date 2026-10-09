#import <Foundation/Foundation.h>
#include "movie_io.hpp"
#include "presenter.hpp"
#include "../audio_lab/audio.hpp"
#include "../surface_lab/oracle.hpp"
#include <mach/mach.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace stream_lab;
std::string stage="setup",directory;uint64_t assertions=0,pcmValues=0,renderBytes=0,colorValues=0,colorError=0;
uint64_t presented=0,eofs=0,pauses=0,seeks=0,staleVideo=0,epochRejects=0,blockedFrames=0;
unsigned maxColorError=0;size_t peakVideoFrames=0,peakVideoBytes=0,peakReaders=0,peakNativePCMBytes=0;
constexpr uint64_t seed=0x57ea6a2026100801ULL;uint64_t randomState=seed;
uint64_t random(){randomState^=randomState<<13;randomState^=randomState>>7;randomState^=randomState<<17;return randomState;}
void check(bool yes,const char* what){++assertions;if(!yes)throw std::runtime_error(stage+": "+what);}
#define CHECK(x) check(bool(x),#x)
int16_t pcmOracle(uint64_t frame,size_t channel){return int16_t(int((frame*37+channel*103)%32768)-16384);}
uint64_t physicalMemory(){task_vm_info_data_t info{};mach_msg_type_number_t count=TASK_VM_INFO_COUNT;
    CHECK(task_info(mach_task_self(),TASK_VM_INFO,reinterpret_cast<task_info_t>(&info),&count)==KERN_SUCCESS);
    return info.phys_footprint;}
void verifyFrame(const video_lab::Frame& frame){CHECK(frame.pts<frameCount);CHECK(frame.bgra.size()==frameStride*height);
    uint64_t signature=0;
    // Independent RGB expectation uses arithmetic digits rather than producer bit shifts.
    for(size_t band=0;band<8;++band){const bool one=(frame.pts/(uint64_t(1)<<band))%2;
        const std::array<int,3> rgb=one?std::array<int,3>{40,160,160}:std::array<int,3>{160,80,40};
        for(size_t y=2;y<height-2;++y)for(size_t x=band*8+2;x<band*8+6;++x){const auto* p=frame.bgra.data()+y*frameStride+x*4;
            for(size_t c=0;c<3;++c){const unsigned error=unsigned(std::abs(int(p[c])-rgb[2-c]));
                ++colorValues;colorError+=error;maxColorError=std::max(maxColorError,error);CHECK(error<=16);}}
        const auto* center=frame.bgra.data()+24*frameStride+(band*8+4)*4;
        if(center[0]>100)signature+=uint64_t(1)<<band;
    }
    CHECK(signature==frame.pts);
    for(size_t y=0;y<height;++y){for(size_t x=0;x<width;++x)CHECK(frame.bgra[y*frameStride+x*4+3]==255);
        for(size_t p=width*4;p<frameStride;++p)CHECK(frame.bgra[y*frameStride+p]==0xad);}
}
void streamContract(){stage="stream contract";using namespace video_lab;
    bool threw=false;try{StreamingPlayer invalid(0);}catch(const std::invalid_argument&){threw=true;}CHECK(threw);
    threw=false;try{MovieReader invalid({width,height,frameStride,fps,rate+1,frameCount});}catch(const std::runtime_error&){threw=true;}CHECK(threw);
    MovieReader tiny({2,2,13,30,48000,2});CHECK(tiny.metadata().width==2&&tiny.metadata().stride==13);
    StreamingPlayer v;StreamDecision d;CHECK(v.advance({1,0,rate},d)==Status::NoClip);
    auto metadata=StreamMetadata{width,height,frameStride,fps,rate,frameCount};auto invalid=metadata;invalid.stride=255;
    CHECK(v.open(invalid)==Status::InvalidMetadata);CHECK(v.open(metadata)==Status::Ok);
    CHECK(v.push(0,{})==Status::StaleEpoch);CHECK(v.push(1,{1,{}})==Status::OutOfOrder);
    CHECK(v.push(1,{0,{}})==Status::InvalidFrame);CHECK(v.start({1,0,rate+1})==Status::InvalidClockRate);
    for(uint64_t i=0;i<4;++i)CHECK(v.push(1,{i,std::vector<uint8_t>(frameStride*height,uint8_t(i))})==Status::Ok);
    CHECK(v.push(1,{4,std::vector<uint8_t>(frameStride*height)})==Status::Backpressure);CHECK(v.usage().nextFeed==4);
    CHECK(v.start({1,0,rate})==Status::Ok);CHECK(v.open(metadata)==Status::InvalidState);
    CHECK(v.advance({1,0,rate},d)==Status::Ok&&d.kind==Kind::Presented&&d.frame->pts==0);
    CHECK(v.advance({1,0,rate},d)==Status::Ok&&d.kind==Kind::Repeat);
    CHECK(v.advance({1,3*1600,rate},d)==Status::Ok&&d.kind==Kind::Presented&&d.frame->pts==3);
    CHECK(v.advance({1,4*1600,rate},d)==Status::Ok&&d.kind==Kind::Hold&&d.frame->pts==3);
    CHECK(v.advance({1,0,rate},d)==Status::ClockRegression);
    CHECK(v.advance({1,UINT64_MAX,rate},d)==Status::ClockOverflow);
    CHECK(v.pause()==Status::Ok);CHECK(v.advance({1,5*1600,rate},d)==Status::Ok&&d.kind==Kind::Paused&&d.mediaTick==4);
    CHECK(v.resume()==Status::Ok);CHECK(v.seek(200)==Status::Ok);CHECK(v.usage().ownedFrameBytes==0);
    CHECK(v.advance({1,5*1600,rate},d)==Status::StaleEpoch);CHECK(v.advance({2,0,rate},d)==Status::Ok&&d.mediaTick==200);
    CHECK(v.push(2,{200,std::vector<uint8_t>(frameStride*height)})==Status::Ok);
    CHECK(v.advance({2,0,rate},d)==Status::Ok&&d.kind==Kind::Presented);
    CHECK(v.advance({2,40*1600,rate},d)==Status::Ok&&d.kind==Kind::Eof);
    CHECK(v.advance({2,40*1600,rate},d)==Status::InvalidState);CHECK(v.seek(frameCount)==Status::InvalidSeek);
    CHECK(v.stop()==Status::Ok&&v.usage().heldFrames==0&&v.usage().queuedFrames==0);
}
struct AudioStats {size_t maxSources=0,maxQueue=0,maxSamples=0,maxEvents=0,completed=0,cancelled=0,staleEvents=0,rebuilds=0;};
class AudioPipe {
    struct Entry {audio_lab::Handle handle;uint64_t ticket;size_t frames,remaining;};
    std::unique_ptr<audio_lab::Player> player;std::vector<Entry> entries;uint64_t epoch=0,cursor=0,readCursor=0;AudioStats metrics;
    void observe(){const auto usage=player->usage();CHECK(usage.liveSources<=2&&usage.queueItems<=2&&usage.pcmSamples<=8192&&usage.pendingEvents<=2);
        CHECK(usage.liveSources==entries.size());metrics.maxSources=std::max(metrics.maxSources,usage.liveSources);
        metrics.maxQueue=std::max(metrics.maxQueue,usage.queueItems);metrics.maxSamples=std::max(metrics.maxSamples,usage.pcmSamples);
        metrics.maxEvents=std::max(metrics.maxEvents,usage.pendingEvents);}
    void completions(){observe();for(const auto& event:player->pollEvents()){
        const auto entry=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.ticket==event.ticket&&e.handle==event.handle;});
        CHECK(entry!=entries.end());CHECK(event.type==audio_lab::EventType::Completed&&event.deliveredFrames==entry->frames&&entry->remaining==0);
        CHECK(player->releaseSource(entry->handle)==audio_lab::Status::Ok);entries.erase(entry);++metrics.completed;}observe();}
public:
    void reset(uint64_t nextEpoch,uint64_t first){
        CHECK(nextEpoch>epoch);std::vector<audio_lab::Event> delayed;const auto oldEpoch=epoch;
        if(player){CHECK(player->flush()==audio_lab::Status::Ok);observe();delayed=player->pollEvents();
            CHECK(delayed.size()==entries.size());for(const auto& event:delayed){const auto item=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.handle==event.handle&&e.ticket==event.ticket;});
                CHECK(item!=entries.end());CHECK(event.type==audio_lab::EventType::Cancelled&&event.deliveredFrames==item->frames-item->remaining);++metrics.cancelled;}
            for(const auto& e:entries)CHECK(player->releaseSource(e.handle)==audio_lab::Status::Ok);
            entries.clear();const auto usage=player->usage();CHECK(usage.liveSources==0&&usage.queueItems==0&&usage.pcmSamples==0&&usage.pendingEvents==0);player.reset();}
        epoch=nextEpoch;cursor=readCursor=first*rate/fps;player=std::make_unique<audio_lab::Player>(audio_lab::Format{rate,channels});++metrics.rebuilds;
        // Terminal events are application events, tagged with the session that produced them.
        // Deliver them after the new player exists; numeric handles/tickets can repeat there.
        for(const auto& event:delayed){(void)event;CHECK(oldEpoch!=epoch);++metrics.staleEvents;}
        CHECK(player->start()==audio_lab::Status::Ok);observe();
    }
    void fill(MovieReader& reader){size_t available=0;for(const auto& e:entries)available+=e.remaining;
        while(available<1600){CHECK(entries.size()<2);PCM packet;CHECK(reader.nextPCM(packet));CHECK(packet.firstFrame==readCursor);
            CHECK(!packet.samples.empty()&&packet.samples.size()<=pcmPacketFrames*channels);const size_t count=packet.samples.size()/channels;
            audio_lab::Handle handle=0;uint64_t ticket=0;CHECK(player->createSource({rate,channels},packet.samples,handle)==audio_lab::Status::Ok);
            CHECK(player->enqueue(handle,1,ticket)==audio_lab::Status::Ok);entries.push_back({handle,ticket,count,count});
            available+=count;readCursor+=count;observe();}
    }
    void consume(){audio_lab::Pull pull;CHECK(player->consume(1600,pull)==audio_lab::Status::Ok);
        CHECK(pull.sourceFrames==1600&&pull.underrunFrames==0&&pull.blockedFrames==0&&pull.guard==audio_lab::Guard::None);
        CHECK(pull.samples.size()==3200);for(size_t i=0;i<1600;++i)for(size_t c=0;c<channels;++c){CHECK(pull.samples[i*channels+c]==pcmOracle(cursor+i,c));++pcmValues;}
        size_t left=1600;for(auto& e:entries){const auto take=std::min(left,e.remaining);e.remaining-=take;left-=take;}CHECK(left==0);cursor+=1600;completions();
    }
    void pause(){CHECK(player->pause()==audio_lab::Status::Ok);audio_lab::Pull pull;CHECK(player->consume(1600,pull)==audio_lab::Status::Ok);
        CHECK(pull.sourceFrames==0&&pull.blockedFrames==1600&&pull.underrunFrames==0&&pull.guard==audio_lab::Guard::Paused);
        CHECK(std::all_of(pull.samples.begin(),pull.samples.end(),[](int16_t v){return v==0;}));blockedFrames+=1600;observe();CHECK(player->resume()==audio_lab::Status::Ok);}
    void eof(MovieReader& reader){CHECK(entries.empty());PCM packet;CHECK(!reader.nextPCM(packet));CHECK(cursor==frameCount*rate/fps);observe();}
    void close(){if(!player)return;CHECK(player->flush()==audio_lab::Status::Ok);auto events=player->pollEvents();
        CHECK(events.size()==entries.size());for(const auto& e:entries)CHECK(player->releaseSource(e.handle)==audio_lab::Status::Ok);entries.clear();
        auto usage=player->usage();CHECK(!usage.liveSources&&!usage.queueItems&&!usage.pcmSamples&&!usage.pendingEvents);player.reset();}
    AudioStats stats() const{return metrics;}
};
void saveJSON(const std::string& name,NSDictionary* object){NSError* error=nil;NSData* data=[NSJSONSerialization dataWithJSONObject:object options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:&error];
    CHECK(data!=nil&&error==nil);CHECK([data writeToFile:[NSString stringWithUTF8String:(directory+"/"+name).c_str()] atomically:YES]);}
void integration(){stage="write bounded synthetic fixture";
    CHECK(pcmOracle(0,0)==-16384&&pcmOracle(0,1)==-16281&&pcmOracle(1600,0)==10048&&pcmOracle(1600,1)==10151);
    const std::string path=directory+"/self_authored_8s.mov";const auto writer=writeSyntheticMovie(path);
    CHECK(writer.videoFrames==frameCount&&writer.audioFrames==frameCount*1600&&writer.maxPCMBytes==6400&&writer.poolThreshold==4);
    MovieReader reader;video_lab::StreamingPlayer video(4,frameStride*height);CHECK(video.open(reader.metadata())==video_lab::Status::Ok);
    EpochPresenter renderer(video.usage().epoch);AudioPipe audio;uint64_t fakeClock=0,nextId=1;const video_lab::Frame* held=nullptr;
    std::map<uint64_t,std::pair<uint64_t,std::vector<uint8_t>>> expected;std::ofstream trace(directory+"/timeline.jsonl");CHECK(bool(trace));
    size_t maxExpected=0;std::array<uint64_t,3> footprints{};
    auto observe=[&]{const auto v=video.usage();CHECK(v.queuedFrames<=4&&v.heldFrames<=1&&v.ownedFrameBytes<=(4+1)*frameStride*height);
        peakVideoFrames=std::max(peakVideoFrames,v.queuedFrames+v.heldFrames);peakVideoBytes=std::max(peakVideoBytes,v.ownedFrameBytes);
        const auto io=reader.usage();CHECK(io.readers<=1&&io.nativePCMPackets<=1&&io.nativePCMBytes<=maxNativePCMBytes);
        peakReaders=std::max(peakReaders,io.readers);peakNativePCMBytes=std::max(peakNativePCMBytes,io.nativePCMBytes);
        const auto r=renderer.usage();CHECK(r.tickets<=2&&r.mailbox<=2&&r.surfaces==3&&r.surfaceBytes==18912&&r.gpuBufferEnvelopeBytes<=69264);};
    auto drain=[&]{renderer.drain([&](const RenderResult& output){auto item=expected.find(output.id);CHECK(item!=expected.end());
        CHECK(output.epoch==video.usage().epoch&&output.pts==item->second.first&&output.bytes==item->second.second);
        renderBytes+=output.bytes.size();expected.erase(item);});CHECK(expected.empty());observe();};
    auto refill=[&]{while(video.usage().queuedFrames<4&&video.usage().nextFeed<frameCount){video_lab::Frame frame;
        CHECK(reader.nextFrame(frame));CHECK(frame.pts==video.usage().nextFeed);verifyFrame(frame);
        CHECK(video.push(video.usage().epoch,std::move(frame))==video_lab::Status::Ok);observe();}};
    auto render=[&](const video_lab::Frame& frame){if(renderer.usage().tickets==2)drain();const uint64_t id=nextId++;
        auto pixels=scalar_oracle::expected(frame.bgra,{int32_t(width),int32_t(height),frameStride,surface_lab::Format::BGRA8},
            std::vector<uint8_t>(133*24,0xc7),{32,24,133,surface_lab::Format::BGRA8},{0,0,int32_t(width),int32_t(height)},{-1,-1,34,26});
        expected.emplace(id,std::make_pair(frame.pts,std::move(pixels)));maxExpected=std::max(maxExpected,expected.size());
        CHECK(renderer.submit(video.usage().epoch,id,frame));if(presented==1)CHECK(!renderer.submit(video.usage().epoch,id,frame));CHECK(expected.size()<=2);observe();};
    auto reset=[&](uint64_t first,bool reopen){stage="seek/reopen and delayed completion";drain();const auto oldEpoch=video.usage().epoch;
        if(held){CHECK(renderer.submit(oldEpoch,nextId++,*held));CHECK(renderer.submit(oldEpoch,nextId++,*held));
            CHECK(!renderer.submit(oldEpoch,nextId++,*held));}
        if(reopen){CHECK(video.stop()==video_lab::Status::Ok);CHECK(video.open(reader.metadata())==video_lab::Status::Ok);}
        else{CHECK(video.seek(first)==video_lab::Status::Ok);++seeks;}
        held=nullptr;const auto epoch=video.usage().epoch;CHECK(epoch>oldEpoch);
        CHECK(video.push(oldEpoch,{})==video_lab::Status::StaleEpoch);++staleVideo;
        video_lab::StreamDecision unused;CHECK(video.advance({oldEpoch,fakeClock,rate},unused)==
            (video.usage().mode==video_lab::Mode::Stopped?video_lab::Status::InvalidState:video_lab::Status::StaleEpoch));
        CHECK(video.usage().queuedFrames==0&&video.usage().heldFrames==0&&video.usage().ownedFrameBytes==0);
        renderer.rebuild(epoch);reader.close();CHECK(reader.usage().readers==0&&reader.usage().nativePCMBytes==0);
        reader.open(path,first);audio.reset(epoch,first);fakeClock=0;
        if(video.usage().mode==video_lab::Mode::Stopped)CHECK(video.start({epoch,fakeClock,rate})==video_lab::Status::Ok);
        CHECK(!renderer.submit(oldEpoch,nextId++,{}));++epochRejects;observe();};
    // Initial session has no previous epoch to cancel.
    reader.open(path);audio.reset(video.usage().epoch,0);CHECK(video.start({video.usage().epoch,0,rate})==video_lab::Status::Ok);
    auto play=[&](uint64_t first,uint64_t count,bool eof){stage="bounded streaming";
        for(uint64_t offset=0;offset<count;++offset){@autoreleasepool{
            refill();audio.fill(reader);observe();video_lab::StreamDecision decision;
            CHECK(video.advance({video.usage().epoch,fakeClock,rate},decision)==video_lab::Status::Ok);
            CHECK(decision.kind==video_lab::Kind::Presented&&decision.mediaTick==first+offset&&decision.frame&&decision.frame->pts==first+offset);
            held=decision.frame;++presented;render(*held);
            if((count==frameCount&&(offset==37||offset==109||offset==201))||(count!=frameCount&&offset==3)){
                CHECK(video.pause()==video_lab::Status::Ok);audio.pause();fakeClock+=1600;
                CHECK(video.advance({video.usage().epoch,fakeClock,rate},decision)==video_lab::Status::Ok&&decision.kind==video_lab::Kind::Paused&&decision.mediaTick==first+offset);
                CHECK(video.resume()==video_lab::Status::Ok);CHECK(video.advance({video.usage().epoch,fakeClock,rate},decision)==video_lab::Status::Ok&&decision.kind==video_lab::Kind::Repeat&&decision.frame->pts==first+offset);++pauses;}
            trace<<"{\"epoch\":"<<video.usage().epoch<<",\"video_pts\":"<<first+offset<<",\"video_timescale\":30,\"source_audio_first_frame\":"<<(first+offset)*1600
                <<",\"source_audio_frames\":1600,\"fake_clock_before_consume\":"<<fakeClock<<"}\n";
            audio.consume();fakeClock+=1600;observe();
        }}
        drain();if(eof){video_lab::StreamDecision decision;CHECK(video.advance({video.usage().epoch,fakeClock,rate},decision)==video_lab::Status::Ok);
            CHECK(decision.kind==video_lab::Kind::Eof&&decision.mediaTick==frameCount&&decision.frame&&decision.frame->pts==frameCount-1);held=decision.frame;
            CHECK(video.advance({video.usage().epoch,fakeClock,rate},decision)==video_lab::Status::InvalidState);++eofs;
            video_lab::Frame frame;CHECK(!reader.nextFrame(frame));audio.eof(reader);}
    };
    for(size_t pass=0;pass<3;++pass){if(pass)reset(0,true);play(0,frameCount,true);footprints[pass]=physicalMemory();}
    for(size_t i=0;i<8;++i){const uint64_t target=random()%220;reset(target,false);play(target,12,false);}
    for(size_t i=0;i<3;++i){reset(232,false);play(232,8,true);}
    stage="shutdown with two pending jobs";CHECK(held!=nullptr);CHECK(renderer.submit(video.usage().epoch,nextId++,*held));CHECK(renderer.submit(video.usage().epoch,nextId++,*held));
    CHECK(video.stop()==video_lab::Status::Ok);held=nullptr;renderer.close();reader.close();audio.close();
    const auto ru=renderer.usage();CHECK(!ru.tickets&&!ru.mailbox&&!ru.surfaces&&!ru.surfaceBytes&&!ru.gpuBufferEnvelopeBytes);
    const auto vu=video.usage();CHECK(!vu.queuedFrames&&!vu.heldFrames&&!vu.ownedFrameBytes);CHECK(reader.usage().readers==0&&reader.usage().nativePCMBytes==0);
    const auto rs=renderer.stats();const auto is=reader.stats();const auto as=audio.stats();
    CHECK(presented==840&&eofs==6&&pauses==20&&seeks==11&&pcmValues==2688000&&renderBytes==2681280);
    CHECK(rs.applied==840&&rs.stale==28&&rs.callbacks==868&&rs.submissions==868&&rs.releasedJobs==868&&rs.rebuilds==14);
    CHECK(rs.maxTickets==2&&rs.maxMailbox==2&&rs.maxGPUBufferEnvelopeCount==10&&rs.maxGPUBufferEnvelopeBytes==69264);
    CHECK(is.readerOpens==14&&is.readerCancels==14&&maxExpected==2&&staleVideo==13&&epochRejects==13);
    CHECK(peakVideoFrames==5&&peakVideoBytes==62640);CHECK(double(colorError)/double(colorValues)<=4.0);
    // Warm framework caches once. This guard is supplemental; hard queue/allocation bounds are separate.
    CHECK(footprints[1]<=footprints[0]+64*1024*1024&&footprints[2]<=footprints[0]+64*1024*1024);
    CHECK(trace.good());trace.close();
    NSString* gpu=[NSString stringWithUTF8String:renderer.deviceName().c_str()];
    saveJSON("summary.json",@{@"passed":@YES,@"scope":@"self-authored bounded streaming; deterministic fake clock; offscreen GPU",
        @"fixture_seconds":@8,@"fixture_video_frames":@(writer.videoFrames),@"fixture_PCM_frames":@(writer.audioFrames),@"GPU_device":gpu,
        @"assertions":@(assertions),@"presented_frames":@(presented),@"PCM_samples_compared":@(pcmValues),@"GPU_bytes_compared":@(renderBytes),
        @"eofs":@(eofs),@"pause_resume_cycles":@(pauses),@"seeks":@(seeks),@"reader_opens":@(is.readerOpens),@"reader_cancels":@(is.readerCancels),
        @"decoded_video_packets":@(is.videoPackets),@"decoded_PCM_packets":@(is.audioPackets),@"GPU_submissions":@(rs.submissions),
        @"GPU_callbacks":@(rs.callbacks),@"GPU_stale_completions_discarded":@(rs.stale),@"GPU_jobs_released":@(rs.releasedJobs),
        @"GPU_rebuilds":@(rs.rebuilds),@"stale_video_packets_rejected":@(staleVideo),@"stale_GPU_submissions_rejected":@(epochRejects),
        @"PCM_completion_events":@(as.completed),@"PCM_cancel_events":@(as.cancelled),@"stale_PCM_events_discarded":@(as.staleEvents),
        @"blocked_PCM_frames_during_pause":@(blockedFrames),@"peak_video_frames":@(peakVideoFrames),@"peak_video_owned_bytes":@(peakVideoBytes),
        @"peak_PCM_sources":@(as.maxSources),@"peak_PCM_queue":@(as.maxQueue),@"peak_PCM_samples":@(as.maxSamples),@"peak_PCM_events":@(as.maxEvents),
        @"peak_readers":@(peakReaders),@"peak_native_PCM_packet_bytes":@(is.maxNativePCMBytes),@"peak_retained_native_PCM_bytes":@(peakNativePCMBytes),
        @"peak_decoded_CV_buffer_bytes":@(is.maxVideoBufferBytes),@"writer_CV_pool_threshold":@(writer.poolThreshold),@"writer_peak_CV_buffer_bytes":@(writer.maxVideoBufferBytes),
        @"peak_GPU_tickets":@(rs.maxTickets),@"peak_GPU_mailbox":@(rs.maxMailbox),@"GPU_buffer_allocation_envelope_count":@(rs.maxGPUBufferEnvelopeCount),
        @"GPU_buffer_allocation_envelope_bytes":@(rs.maxGPUBufferEnvelopeBytes),@"surface_owned_bytes":@18912,@"all_owned_session_resources_zero_after_close":@YES,
        @"physical_footprint_after_pass_bytes":@[@(footprints[0]),@(footprints[1]),@(footprints[2])],@"physical_footprint_growth_guard_bytes":@(64*1024*1024),
        @"color_interior_channel_values":@(colorValues),@"color_max_error":@(maxColorError),@"color_mean_error":@(double(colorError)/double(colorValues)),
        @"color_max_tolerance":@16,@"color_mean_tolerance":@4,@"color_comparison_excludes_two_pixel_bar_edges":@YES,
        @"seed_hex":@"0x57ea6a2026100801",@"final_rng_hex":[NSString stringWithFormat:@"0x%016llx",static_cast<unsigned long long>(randomState)],
        @"framework_internal_memory_hard_bound_proven":@NO,@"native_GPU_command_cancellation_tested":@NO,@"actual_device_loss_tested":@NO,
        @"real_audio_output_tested":@NO,@"real_AV_sync_tested":@NO,@"window_or_UI_used":@NO,@"game_code_or_assets_used":@NO});
    std::cout<<"PASS 8-second streaming fixture: 840 presentations, 2688000 PCM samples, 2681280 GPU bytes; 28 stale completions dropped; owned resources zero\n";
}
}
int main(int argc,char** argv){@autoreleasepool{
    if(argc!=2){std::cerr<<"usage: stream-tests EVIDENCE_DIR\n";return 2;}directory=argv[1];
    try{streamContract();integration();return 0;}catch(const std::exception& error){
        std::cerr<<error.what()<<"\n";NSError* ignored=nil;NSData* json=[NSJSONSerialization dataWithJSONObject:
            @{@"passed":@NO,@"stage":[NSString stringWithUTF8String:stage.c_str()],@"reason":[NSString stringWithUTF8String:error.what()],@"assertions":@(assertions)} options:NSJSONWritingPrettyPrinted error:&ignored];
        [json writeToFile:[NSString stringWithUTF8String:(directory+"/summary.json").c_str()] atomically:YES];return 1;}
}}
