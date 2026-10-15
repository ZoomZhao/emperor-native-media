#pragma once
// Authored streaming PCM sink. Audio Queue Services are the output device;
// FFmpeg is only an existing external decoder, never linked or bundled.
#include "../stream_lab/bink_process.hpp"
#include <AudioToolbox/AudioToolbox.h>
#include <atomic>
#include <mutex>
#include <memory>
#include <cmath>
#include <cstring>

namespace game_lab {
// Owner-context gain changes are immediate, including while paused. The mutex
// protects the native queue lifetime; it is never acquired by its callback.
class MovieAudioGain {
public:
    void set(double value){std::lock_guard<std::mutex> guard(lock_);
        gain_=std::isfinite(value)?std::clamp(value,0.0,1.0):0;
        if(queue_)check(AudioQueueSetParameter(queue_,kAudioQueueParam_Volume,gain_),"gain");}
    double value() const {std::lock_guard<std::mutex> guard(lock_);return gain_;}
    void attach(AudioQueueRef queue){std::lock_guard<std::mutex> guard(lock_);
        check(AudioQueueSetParameter(queue,kAudioQueueParam_Volume,gain_),"initial gain");queue_=queue;}
    void detach(){std::lock_guard<std::mutex> guard(lock_);queue_=nullptr;}
    static void check(OSStatus status,const char* action){if(status!=noErr)
        throw std::runtime_error(std::string("native Bink audio ")+action+" failed ("+std::to_string(status)+")");}
private:
    mutable std::mutex lock_;double gain_=0;AudioQueueRef queue_=nullptr;
};
class BinkAudio {
public:
    static constexpr size_t bufferCount=6,packetFrames=4096;
    explicit BinkAudio(std::shared_ptr<MovieAudioGain> gain):gain_(std::move(gain)){}
    ~BinkAudio(){close();}
    void open(const std::string& path,uint64_t base,const std::function<bool()>& cancelled){
        close();info_=stream_lab::readBinkInfo(path);base_=base;decoded_=submitted_=lastClock_=0;
        returned_=0;closing_=false;eof_=draining_=drained_=started_=false;resuming_=false;observations_=0;maxBackstep_=0;
        decoder_.openAudio(path,base);
        AudioStreamBasicDescription format{};format.mSampleRate=info_.audioRate;
        format.mFormatID=kAudioFormatLinearPCM;format.mFormatFlags=kLinearPCMFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;
        format.mBytesPerPacket=format.mBytesPerFrame=2*info_.audioChannels;
        format.mFramesPerPacket=1;format.mChannelsPerFrame=info_.audioChannels;format.mBitsPerChannel=16;
        try {
            check(AudioQueueNewOutput(&format,callback,this,nullptr,nullptr,0,&queue_),"create");
            gain_->attach(queue_);check(AudioQueueCreateTimeline(queue_,&timeline_),"timeline");
            for(auto& slot:slots_)check(AudioQueueAllocateBuffer(queue_,packetFrames*format.mBytesPerFrame,&slot.buffer),"allocate");
            fill(cancelled);
            if(!submitted_)throw std::runtime_error("Bink audio has no requested samples");
        }catch(...){close();throw;}
    }
    void play(){if(!queue_||drained_)return;resuming_=started_;
        check(AudioQueueStart(queue_,nullptr),"start/resume");draining_=false;
        paused_=false;started_=true;startWait_=Clock::now();}
    void pause(){if(!queue_||paused_)return;check(AudioQueuePause(queue_),"pause");
        lastClock_=readClock();paused_=true;}
    void service(const std::function<bool()>& cancelled){
        if(!queue_||paused_)return;
        fill(cancelled);
        if(eof_&&started_&&!draining_){check(AudioQueueStop(queue_,false),"drain");draining_=true;}
        if(draining_){UInt32 running=1,size=sizeof(running);
            check(AudioQueueGetProperty(queue_,kAudioQueueProperty_IsRunning,&running,&size),"drain status");
            if(!running){drained_=true;lastClock_=submitted_;}}
    }
    uint64_t currentFrames(){if(!queue_||paused_||drained_)return lastClock_;lastClock_=readClock();return lastClock_;}
    void close() noexcept {
        closing_=true;gain_->detach();
        if(queue_){AudioQueueStop(queue_,true);AudioQueueDispose(queue_,true);queue_=nullptr;timeline_=nullptr;}
        for(auto& slot:slots_){slot.buffer=nullptr;slot.busy=false;slot.frames=0;}
        decoder_.close();std::vector<int16_t>().swap(packet_);paused_=true;
    }
    bool active() const{return queue_!=nullptr;}
    bool finished() const{return drained_;}
    bool eof() const{return eof_;}
    size_t readers() const{return decoder_.active()?1:0;}
    size_t nativeBytes() const{return queue_?bufferCount*packetFrames*info_.audioChannels*2:0;}
    size_t pendingBuffers() const{size_t count=0;for(const auto& slot:slots_)count+=slot.busy.load();return count;}
    uint64_t decoded() const{return decoded_;}
    uint64_t submitted() const{return submitted_;}
    uint64_t returned() const{return returned_;}
    uint64_t observations() const{return observations_;}
    uint64_t maxResumeBackstep() const{return maxBackstep_;}
    uint64_t base() const{return base_;}
private:
    using Clock=std::chrono::steady_clock;
    struct Slot {AudioQueueBufferRef buffer=nullptr;std::atomic<bool> busy{false};std::atomic<uint32_t> frames{0};};
    std::shared_ptr<MovieAudioGain> gain_;stream_lab::BinkProcess decoder_;stream_lab::BinkInfo info_{};
    AudioQueueRef queue_=nullptr;AudioQueueTimelineRef timeline_=nullptr;
    std::array<Slot,bufferCount> slots_{};std::vector<int16_t> packet_;
    std::atomic<uint64_t> returned_{0};std::atomic<bool> closing_{false};
    uint64_t base_=0,decoded_=0,submitted_=0,lastClock_=0,observations_=0,maxBackstep_=0;
    bool paused_=true,eof_=false,draining_=false,drained_=false,started_=false,resuming_=false;
    Clock::time_point startWait_{};
    static void check(OSStatus value,const char* action){MovieAudioGain::check(value,action);}
    static void callback(void* context,AudioQueueRef,AudioQueueBufferRef buffer){
        auto& self=*static_cast<BinkAudio*>(context);
        for(auto& slot:self.slots_)if(slot.buffer==buffer){
            if(!self.closing_)self.returned_.fetch_add(slot.frames.load());
            slot.busy.store(false,std::memory_order_release);return;}
    }
    void fill(const std::function<bool()>& cancelled){
        for(auto& slot:slots_){
            if(eof_||slot.busy.load(std::memory_order_acquire))continue;
            const auto count=decoder_.nextAudio(packet_,cancelled);
            if(!count){eof_=true;break;}
            decoded_+=count;
            const uint64_t bound=uint64_t(std::ceil(info_.duration()*info_.audioRate))+2*info_.audioRate;
            if(base_+decoded_>bound)throw std::runtime_error("Bink PCM exceeds bounded video duration + 2 seconds");
            std::memcpy(slot.buffer->mAudioData,packet_.data(),packet_.size()*2);
            slot.buffer->mAudioDataByteSize=UInt32(packet_.size()*2);slot.frames=uint32_t(count);slot.busy=true;
            const auto result=AudioQueueEnqueueBuffer(queue_,slot.buffer,0,nullptr);
            if(result!=noErr){slot.busy=false;check(result,"enqueue");}
            submitted_+=count;
        }
    }
    uint64_t readClock(){
        AudioTimeStamp time{};Boolean discontinuity=false;
        const auto status=AudioQueueGetCurrentTime(queue_,timeline_,&time,&discontinuity);
        if(discontinuity)throw std::runtime_error("native Bink audio timeline discontinuity; playback stopped");
        if(status!=noErr||!(time.mFlags&kAudioTimeStampSampleTimeValid)){
            if(started_&&Clock::now()-startWait_>std::chrono::seconds(2))check(status?status:-1,"clock");
            return lastClock_;
        }
        if(!std::isfinite(time.mSampleTime)||time.mSampleTime<0)return lastClock_;
        ++observations_;
        const auto value=uint64_t(time.mSampleTime);
        if(value+1<lastClock_){
            // AudioQueue's estimate can step back when its paused hardware
            // timeline restarts (also reproduced with authored silent PCM).
            // Hold the existing image/clock until the unshifted native clock
            // catches up. Do not add an offset or silently accept a device jump.
            const auto step=lastClock_-value;
            if(!resuming_||step>info_.audioRate/10||Clock::now()-startWait_>std::chrono::milliseconds(250))
                throw std::runtime_error("native Bink audio clock moved backwards: "+std::to_string(lastClock_)+" -> "+std::to_string(value));
            maxBackstep_=std::max(maxBackstep_,step);
        }else resuming_=false;
        // Never present time beyond the PCM actually submitted, even at drain.
        return std::min(submitted_,std::max(lastClock_,value));
    }
};
} // namespace game_lab
