#include "controller.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace media_demo {
namespace {
constexpr uint64_t pcmPerFrame=stream_lab::rate/stream_lab::fps;
void require(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
uint64_t checkedFrames(uint64_t frames){require(frames>0&&frames<=1000000,"clip frame bound");return frames;}
void videoOK(video_lab::Status status){require(status==video_lab::Status::Ok,"video API rejected action");}
void audioOK(audio_lab::Status status){require(status==audio_lab::Status::Ok,"audio queue API rejected action");}
}
const char* stateName(State state){switch(state){case State::Ready:return "待机";case State::Playing:return "播放中";
    case State::Paused:return "已暂停";case State::Ended:return "播放结束";case State::Failed:return "发生错误";}return "未知";}
Controller::Controller(std::string path,PCMObserver observer,uint64_t frames):path_(std::move(path)),observer_(std::move(observer)),frameCount_(checkedFrames(frames)),totalPCM_(frameCount_*pcmPerFrame),
    reader_({stream_lab::width,stream_lab::height,stream_lab::frameStride,stream_lab::fps,stream_lab::rate,frameCount_}),video_(4,stream_lab::frameStride*stream_lab::height){
    require(!path_.empty(),"missing bundled synthetic movie");videoOK(video_.open(reader_.metadata()));clock_.reset();observation_=clock_.read();}
Controller::~Controller(){try{close();}catch(...){}}
Snapshot Controller::snapshot() const{Snapshot out;out.state=state_;out.frame=frame_;out.pcmFrames=consumed_;out.basePCM=basePCM_;out.mediaPCM=basePCM_+consumed_;
    out.durationPCM=totalPCM_;out.positionSeconds=double(out.mediaPCM)/stream_lab::rate;out.durationSeconds=double(totalPCM_)/stream_lab::rate;out.deviceBlocked=deviceBlocked_;out.clock=observation_;out.stats=stats_;
    const auto video=video_.usage();out.epoch=video.epoch;out.queuedVideo=video.queuedFrames;out.videoBytes=video.ownedFrameBytes;
    const auto io=reader_.usage();out.readers=io.readers;out.nativePCMBytes=io.nativePCMBytes;
    const auto readerStats=reader_.stats();out.stats.metadataTimeouts=readerStats.metadataTimeouts;out.stats.metadataCancellations=readerStats.metadataCancellations;
    if(audio_){const auto a=audio_->usage();out.pcmSources=a.liveSources;out.pcmBytes=a.pcmSamples*sizeof(int16_t);out.pcmQueue=a.queueItems;out.pcmEvents=a.pendingEvents;}
    if(presenter_){const auto p=presenter_->usage();out.gpuTickets=p.tickets;out.gpuSurfaces=p.surfaces;out.gpuBytes=p.gpuBufferEnvelopeBytes;}return out;}
void Controller::peak(){const auto s=snapshot();stats_.maxVideoBytes=std::max(stats_.maxVideoBytes,s.videoBytes);
    stats_.maxPCMSources=std::max(stats_.maxPCMSources,s.pcmSources);stats_.maxPCMBytes=std::max(stats_.maxPCMBytes,s.pcmBytes);
    stats_.maxReaders=std::max(stats_.maxReaders,s.readers);stats_.maxGPUBytes=std::max(stats_.maxGPUBytes,s.gpuBytes);}
void Controller::releaseRuntime(){const bool existed=sessionActive_;
    if(presenter_){presenter_->close();require(!presenter_->usage().surfaces&&!presenter_->usage().tickets&&!presenter_->usage().mailbox,"GPU release failed");presenter_.reset();}
    reader_.close();require(!reader_.usage().readers&&!reader_.usage().nativePCMBytes,"reader release failed");
    if(audio_){audioOK(audio_->flush());const auto events=audio_->pollEvents();require(events.size()==sources_.size(),"flush terminal event count");
        for(const auto& source:sources_)audioOK(audio_->releaseSource(source.handle));sources_.clear();
        const auto usage=audio_->usage();require(!usage.liveSources&&!usage.queueItems&&!usage.pcmSamples&&!usage.pendingEvents,"PCM release failed");audio_.reset();}
    if(existed)++stats_.runtimeReleases;sessionActive_=false;
}
void Controller::close(){clock_.reset();videoOK(video_.stop());releaseRuntime();state_=State::Ready;
    frame_=basePCM_=consumed_=readPCM_=readVideo_=frozenPCM_=0;deviceBlocked_=resumeAfterDevice_=false;observation_=clock_.read();
    require(snapshot().resourcesZero(),"close resource invariant");}
void Controller::prepare(uint64_t target){require(target<frameCount_,"seek outside clip");
    clock_.reset();videoOK(video_.stop());videoOK(video_.seek(target));releaseRuntime();
    frame_=target;basePCM_=target*pcmPerFrame;consumed_=0;readPCM_=basePCM_;readVideo_=target;
    sessionActive_=true;++stats_.runtimeOpens;reader_.open(path_,target,openControl_);audio_=std::make_unique<audio_lab::Player>(audio_lab::Format{stream_lab::rate,stream_lab::channels});
    presenter_=std::make_unique<stream_lab::EpochPresenter>(video_.usage().epoch,reader_.metadata());
    audioOK(audio_->start());videoOK(video_.start({video_.usage().epoch,0,stream_lab::rate}));state_=State::Playing;
    clock_.reset(totalPCM_-basePCM_);peak();
}
Snapshot Controller::begin(uint64_t position,bool running){require(position<totalPCM_,"rebuild outside clip");prepare(position/pcmPerFrame);
    clock_.reset(totalPCM_-basePCM_,position-basePCM_);auto result=step(clock_.read());
    if(running)observation_=clock_.resume();else{audioOK(audio_->pause());state_=State::Paused;observation_=clock_.read();}
    ++stats_.clockRebuilds;auto out=snapshot();out.bgra=std::move(result.bgra);return out;
}
void Controller::fillVideo(){const uint64_t next=video_.usage().nextFeed;
    while(readVideo_<next){video_lab::Frame discard;require(reader_.nextFrame(discard)&&discard.pts==readVideo_,"catch-up video order");++readVideo_;++stats_.discardedVideo;}
    while(video_.usage().queuedFrames<4&&video_.usage().nextFeed<frameCount_){video_lab::Frame packet;
        require(reader_.nextFrame(packet)&&packet.pts==readVideo_&&packet.pts==video_.usage().nextFeed,"video frame order");
        ++readVideo_;videoOK(video_.push(video_.usage().epoch,std::move(packet)));peak();}}
size_t Controller::fillAudio(size_t needed){size_t available=0;for(const auto& source:sources_)available+=source.remaining;
    while(available<needed&&sources_.size()<2){stream_lab::PCM packet;
        require(reader_.nextPCM(packet),"early PCM EOF");require(packet.firstFrame==readPCM_,"PCM PTS continuity");
        const size_t frames=packet.samples.size()/stream_lab::channels;require(frames>0&&frames<=stream_lab::pcmPacketFrames,"PCM slice bound");
        audio_lab::Handle handle=0;uint64_t ticket=0;audioOK(audio_->createSource({stream_lab::rate,stream_lab::channels},packet.samples,handle));
        audioOK(audio_->enqueue(handle,1,ticket));sources_.push_back({handle,ticket,frames,frames});available+=frames;readPCM_+=frames;peak();}
    require(available>0,"PCM made no progress");return available;}
void Controller::collectAudio(){for(const auto& event:audio_->pollEvents()){
    const auto source=std::find_if(sources_.begin(),sources_.end(),[&](const Source& item){return item.ticket==event.ticket&&item.handle==event.handle;});
    require(source!=sources_.end()&&event.type==audio_lab::EventType::Completed&&source->remaining==0&&event.deliveredFrames==source->frames,"PCM completion invariant");
    audioOK(audio_->releaseSource(source->handle));sources_.erase(source);}}
void Controller::consumeAudio(uint64_t count){stats_.maxCatchupFrames=std::max(stats_.maxCatchupFrames,count);
    while(count){const size_t requested=size_t(std::min(count,pcmPerFrame));
        // A native packet can end in a short slice. Consume the two existing sources
        // before pulling a third, keeping the original memory bound and exact cursor.
        const size_t take=std::min(requested,fillAudio(requested));audio_lab::Pull output;audioOK(audio_->consume(take,output));
        require(output.sourceFrames==take&&!output.underrunFrames&&!output.blockedFrames,"PCM consumption invariant");
        if(observer_)observer_(basePCM_+consumed_,output.samples);
        size_t left=take;for(auto& source:sources_){const size_t amount=std::min(left,source.remaining);source.remaining-=amount;left-=amount;}
        require(left==0,"PCM source accounting");collectAudio();consumed_+=take;count-=take;stats_.checkedPCMFrames+=take;
        // Discard PCM; no native audio output object is created.
    }}
void Controller::image(video_lab::StreamDecision decision,Snapshot& output){require(decision.frame&&presenter_,"missing displayed frame");frame_=decision.frame->pts;
    const auto epoch=video_.usage().epoch,id=nextRender_++;require(presenter_->submit(epoch,id,*decision.frame),"GPU presenter rejected frame");peak();
    presenter_->drain([&](const stream_lab::RenderResult& result){require(result.epoch==epoch&&result.id==id&&result.pts==frame_&&result.bytes.size()==133*24,"GPU result invariant");output.bgra=result.bytes;});++stats_.renders;}
Snapshot Controller::step(HostClock::Reading reading){require(reading.samples>=consumed_&&reading.samples<=totalPCM_-basePCM_,"clock/sample regression");
    observation_=reading;consumeAudio(reading.samples-consumed_);
    // Drop overdue packets, refill the bounded queue, then present the exact due frame.
    const uint64_t shown=std::min(reading.samples,totalPCM_-basePCM_-1);video_lab::StreamDecision decision;
    videoOK(video_.advance({video_.usage().epoch,shown,stream_lab::rate},decision));fillVideo();
    videoOK(video_.advance({video_.usage().epoch,shown,stream_lab::rate},decision));
    require(decision.kind==video_lab::Kind::Presented||decision.kind==video_lab::Kind::Repeat,"timeline missing current frame");
    Snapshot result;image(decision,result);
    if(basePCM_+consumed_==totalPCM_){videoOK(video_.advance({video_.usage().epoch,reading.samples,stream_lab::rate},decision));
        require(decision.kind==video_lab::Kind::Eof&&sources_.empty(),"EOF queue invariant");video_lab::Frame packet;stream_lab::PCM pcm;
        require(!reader_.nextFrame(packet)&&!reader_.nextPCM(pcm),"media beyond declared EOF");
        clock_.pause();videoOK(video_.stop());releaseRuntime();state_=State::Ended;frame_=frameCount_-1;
        auto out=snapshot();out.bgra=std::move(result.bgra);out.message="EOF：播放结束，媒体资源已释放。";return out;}
    auto out=snapshot();out.bgra=std::move(result.bgra);return out;
}
Snapshot Controller::fail(const std::exception& error){std::string message=error.what();try{close();}catch(const std::exception& cleanup){message+="；清理失败：";message+=cleanup.what();}
    state_=State::Failed;auto out=snapshot();out.clearImage=true;out.message="错误："+message;return out;}
Snapshot Controller::perform(Action action,uint64_t target,std::shared_ptr<stream_lab::OpenControl> control){
    openControl_=control?control:std::make_shared<stream_lab::OpenControl>();try{switch(action){
    case Action::Play:{if(state_==State::Playing)return snapshot();if(deviceKnown_&&!device_.available){auto out=snapshot();out.message="输出设备暂不可用，保持静音等待。";return out;}
        Snapshot out;if(state_==State::Paused&&!deviceBlocked_){audioOK(audio_->resume());clock_.resume();state_=State::Playing;out=step(clock_.read());}
        else out=begin(state_==State::Ended||state_==State::Failed?0:basePCM_+consumed_,true);
        out.message="播放：系统单调时钟，保持静音。";return out;}
    case Action::Pause:{Snapshot out;if(state_==State::Playing){clock_.pause();out=step(clock_.read());
            if(state_==State::Playing){audioOK(audio_->pause());state_=State::Paused;out.state=state_;}}
        else out=snapshot();resumeAfterDevice_=false;out.message="暂停：媒体时钟已冻结。";return out;}
    case Action::Seek:{require(target<frameCount_,"seek outside clip");const bool run=state_==State::Playing;
        const bool blocked=deviceBlocked_;auto out=begin(target*pcmPerFrame,run&&!blocked);if(blocked){frozenPCM_=target*pcmPerFrame;videoOK(video_.stop());releaseRuntime();
            auto pixels=std::move(out.bgra);out=snapshot();out.bgra=std::move(pixels);}
        out.message="Seek：跳转到帧 "+std::to_string(target)+"，时钟已重建。";return out;}
    case Action::Stop:{close();auto out=snapshot();out.clearImage=true;out.message="停止：回到起点，媒体资源已释放。";return out;}
    case Action::Tick:return state_==State::Playing?step(clock_.read()):snapshot();}
    return snapshot();}catch(const std::exception& error){return fail(error);}}
Snapshot Controller::deviceChanged(DeviceEvent event,std::shared_ptr<stream_lab::OpenControl> control){
    openControl_=control?control:std::make_shared<stream_lab::OpenControl>();try{
    if(event.sequence<=lastDeviceSequence_){++stats_.staleDeviceEvents;return snapshot();}lastDeviceSequence_=event.sequence;
    if(event.origin==DeviceOrigin::Native)++stats_.nativeDeviceEvents;else ++stats_.injectedDeviceEvents;
    require(std::isfinite(event.nominalRate)&&event.nominalRate>=0,"invalid device rate event");
    if(deviceKnown_&&device_.device==event.device&&device_.available==event.available&&device_.nominalRate==event.nominalRate){++stats_.duplicateDeviceEvents;return snapshot();}
    device_=event;deviceKnown_=true;if(state_!=State::Playing&&state_!=State::Paused)return snapshot();
    bool run=state_==State::Playing;uint64_t position=basePCM_+consumed_;
    if(deviceBlocked_){run=resumeAfterDevice_;position=frozenPCM_;}
    else{clock_.pause();auto current=step(clock_.read());if(state_==State::Ended)return current;position=current.mediaPCM;}
    if(!event.available){videoOK(video_.stop());releaseRuntime();deviceBlocked_=true;resumeAfterDevice_=run;frozenPCM_=position;state_=State::Paused;
        auto out=snapshot();out.message="输出设备暂不可用：位置已冻结，保持静音。";return out;}
    deviceBlocked_=false;resumeAfterDevice_=false;auto out=begin(position,run);++stats_.deviceRecoveries;out.stats.deviceRecoveries=stats_.deviceRecoveries;
    out.message="输出设备变化：会话和时钟已重建，保持静音。";return out;
}catch(const std::exception& error){return fail(error);}}
} // namespace media_demo
