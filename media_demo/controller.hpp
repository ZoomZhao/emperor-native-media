#pragma once
#include "../stream_lab/movie_io.hpp"
#include "../stream_lab/presenter.hpp"
#include "../audio_lab/audio.hpp"
#include "host_clock.hpp"
#include "device_monitor.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace media_demo {
enum class Action {Play,Pause,Seek,Stop,Tick};
enum class State {Ready,Playing,Paused,Ended,Failed};
const char* stateName(State state);
struct ControllerStats {size_t runtimeOpens=0,runtimeReleases=0,renders=0,discardedVideo=0;
    uint64_t decodedVideoFrames=0;
    size_t maxVideoBytes=0,maxPCMSources=0,maxPCMBytes=0,maxReaders=0,maxGPUBytes=0;
    uint64_t checkedPCMFrames=0,maxCatchupFrames=0,clockRebuilds=0,nativeDeviceEvents=0,injectedDeviceEvents=0,
        deviceRecoveries=0,duplicateDeviceEvents=0,staleDeviceEvents=0,metadataTimeouts=0,metadataCancellations=0;};
struct Snapshot {
    State state=State::Ready;uint64_t epoch=1,frame=0,pcmFrames=0,mediaPCM=0,basePCM=0,durationPCM=0;
    size_t queuedVideo=0,pcmSources=0,gpuTickets=0,readers=0,nativePCMBytes=0,videoBytes=0,pcmBytes=0,
        pcmQueue=0,pcmEvents=0,gpuSurfaces=0,gpuBytes=0;bool clearImage=false,deviceBlocked=false;
    HostClock::Reading clock{};ControllerStats stats;
    std::vector<uint8_t> bgra;
    uint32_t imageWidth=32,imageHeight=24,fpsNumerator=30,fpsDenominator=1;
    size_t imageStride=133;uint64_t videoFrames=240;
    double positionSeconds=0,durationSeconds=8;
    std::string message;
    bool resourcesZero() const{return !queuedVideo&&!videoBytes&&!pcmSources&&!pcmBytes&&!pcmQueue&&!pcmEvents&&
        !gpuTickets&&!gpuSurfaces&&!gpuBytes&&!readers&&!nativePCMBytes;}
};
// Owner thread only. Audio is discarded offline; real monotonic time controls
// both PCM consumption and video PTS, independently of the GUI tick count.
class Controller {
public:
    using PCMObserver=std::function<void(uint64_t,const std::vector<int16_t>&)>;
    explicit Controller(std::string moviePath,PCMObserver observer={},uint64_t frames=stream_lab::frameCount);~Controller();
    Snapshot perform(Action action,uint64_t seekFrame=0,std::shared_ptr<stream_lab::OpenControl> control={});
    Snapshot deviceChanged(DeviceEvent event,std::shared_ptr<stream_lab::OpenControl> control={});
    Snapshot snapshot() const;void close();
private:
    struct Source {audio_lab::Handle handle;uint64_t ticket;size_t frames,remaining;};
    std::string path_;PCMObserver observer_;State state_=State::Ready;
    const uint64_t frameCount_,totalPCM_;bool sessionActive_=false;
    std::shared_ptr<stream_lab::OpenControl> openControl_;
    uint64_t frame_=0,basePCM_=0,consumed_=0,readPCM_=0,readVideo_=0,nextRender_=1,frozenPCM_=0,lastDeviceSequence_=0;
    HostClock clock_;HostClock::Reading observation_{};ControllerStats stats_;
    DeviceEvent device_{};bool deviceKnown_=false,deviceBlocked_=false,resumeAfterDevice_=false;
    stream_lab::MovieReader reader_;video_lab::StreamingPlayer video_;
    std::unique_ptr<audio_lab::Player> audio_;std::unique_ptr<stream_lab::EpochPresenter> presenter_;
    std::vector<Source> sources_;
    Snapshot begin(uint64_t absolutePCM,bool running);void prepare(uint64_t frame);void releaseRuntime();
    void fillVideo();size_t fillAudio(size_t needed);void collectAudio();void consumeAudio(uint64_t count);
    void image(video_lab::StreamDecision decision,Snapshot& output);void peak();
    Snapshot step(HostClock::Reading reading);Snapshot fail(const std::exception& error);
};
} // namespace media_demo
