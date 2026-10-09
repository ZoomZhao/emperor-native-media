#include "audio.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace audio_lab {
namespace {
bool valid(Format f) { return f.rate >= 8000 && f.rate <= 192000 && f.channels >= 1 && f.channels <= 8; }
bool equal(Format a, Format b) { return a.rate == b.rate && a.channels == b.channels; }
}
const char* statusName(Status s) {
    switch(s) {
        case Status::Ok:return "OK"; case Status::InvalidFormat:return "INVALID_FORMAT";
        case Status::UnsupportedFormat:return "UNSUPPORTED_FORMAT"; case Status::InvalidPCM:return "INVALID_PCM";
        case Status::Capacity:return "CAPACITY"; case Status::InvalidHandle:return "INVALID_HANDLE";
        case Status::Busy:return "BUSY"; case Status::InvalidLoops:return "INVALID_LOOPS";
        case Status::InvalidState:return "INVALID_STATE"; case Status::InvalidDevice:return "INVALID_DEVICE";
        case Status::DeviceUnavailable:return "DEVICE_UNAVAILABLE"; case Status::InvalidFrameCount:return "INVALID_FRAME_COUNT";
    }
    return "UNKNOWN";
}
Player::Player(Format format) : format_(format) {
    if(!valid(format))throw std::invalid_argument("rate must be 8000..192000; channels must be 1..8");
}
Player::Source* Player::find(Handle handle) {
    const uint32_t index=uint32_t(handle & 0xffffffffu),generation=uint32_t(handle>>32);
    if(!index||index>sources_.size())return nullptr;
    Source& source=sources_[index-1];
    return source.live&&source.generation==generation?&source:nullptr;
}
Status Player::createSource(Format format,const std::vector<int16_t>& samples,Handle& result) {
    result=0;
    if(!valid(format))return Status::InvalidFormat;
    if(!equal(format,format_))return Status::UnsupportedFormat;
    if(samples.empty()||samples.size()%format_.channels||samples.size()/format_.channels>65536)return Status::InvalidPCM;
    for(size_t i=0;i<sources_.size();++i){Source& source=sources_[i];
        if(source.live||source.retired)continue;
        auto copy=samples; source.samples.swap(copy);source.live=true;source.references=0;
        result=(uint64_t(source.generation)<<32)|uint64_t(i+1);return Status::Ok;}
    return Status::Capacity;
}
Status Player::releaseSource(Handle handle) {
    Source* source=find(handle);if(!source)return Status::InvalidHandle;
    if(source->references)return Status::Busy;
    std::vector<int16_t>().swap(source->samples);source->live=false;
    if(source->generation==std::numeric_limits<uint32_t>::max())source->retired=true;
    else ++source->generation;
    return Status::Ok;
}
Status Player::enqueue(Handle handle,int64_t loops,uint64_t& ticket) {
    ticket=0;Source* source=find(handle);if(!source)return Status::InvalidHandle;
    if(loops<0||loops>std::numeric_limits<int32_t>::max())return Status::InvalidLoops;
    if(queue_.size()==8||nextTicket_==std::numeric_limits<uint64_t>::max())return Status::Capacity;
    const uint64_t issued=nextTicket_;
    queue_.push_back({handle,issued,uint32_t(loops),0,0,0});
    ++nextTicket_;++source->references;ticket=issued;return Status::Ok;
}
Status Player::consume(size_t frames,Pull& result) {
    if(!frames||frames>4096)return Status::InvalidFrameCount;
    Pull staging;staging.samples.assign(frames*format_.channels,0);
    if(!deviceReady_)staging.guard=Guard::DeviceLost;
    else if(playback_==Playback::Paused)staging.guard=Guard::Paused;
    else if(playback_==Playback::Stopped)staging.guard=Guard::Stopped;
    if(staging.guard!=Guard::None){
        staging.blockedFrames=frames;counters_.blockedFrames+=frames;
    } else {
        size_t written=0;
        while(written<frames&&!queue_.empty()) {
            Item& item=queue_.front();Source* source=find(item.handle);
            if(!source)throw std::logic_error("internal live source invariant violated");
            const size_t sourceFrames=source->samples.size()/format_.channels;
            const size_t take=std::min(frames-written,sourceFrames-item.cursor);
            std::copy_n(source->samples.begin()+std::ptrdiff_t(item.cursor*format_.channels),
                take*format_.channels,staging.samples.begin()+std::ptrdiff_t(written*format_.channels));
            written+=take;item.cursor+=take;item.delivered+=take;
            if(item.cursor==sourceFrames) {
                item.cursor=0;++item.passes;
                if(item.loops&&item.passes==item.loops){
                    events_.push_back({EventType::Completed,item.ticket,item.handle,item.delivered,device_});
                    --source->references;queue_.pop_front();
                }
            }
        }
        staging.sourceFrames=written;staging.underrunFrames=frames-written;
        counters_.sourceFrames+=written;counters_.underrunFrames+=frames-written;
    }
    counters_.outputFrames+=frames;result=std::move(staging);return Status::Ok;
}
Status Player::start() {
    if(playback_!=Playback::Stopped)return Status::InvalidState;
    if(!deviceReady_)return Status::DeviceUnavailable;
    playback_=Playback::Running;return Status::Ok;
}
Status Player::pause() {
    if(playback_!=Playback::Running)return Status::InvalidState;
    playback_=Playback::Paused;return Status::Ok;
}
Status Player::resume() {
    if(playback_!=Playback::Paused)return Status::InvalidState;
    if(!deviceReady_)return Status::DeviceUnavailable;
    playback_=Playback::Running;return Status::Ok;
}
Status Player::stop() { playback_=Playback::Stopped;return Status::Ok; }
Status Player::flush() {
    while(!queue_.empty()){const Item& item=queue_.front();Source* source=find(item.handle);
        if(!source)throw std::logic_error("internal flush source invariant violated");
        events_.push_back({EventType::Cancelled,item.ticket,item.handle,item.delivered,device_});
        --source->references;queue_.pop_front();}
    playback_=Playback::Stopped;return Status::Ok;
}
Status Player::simulateLoss() {
    if(!deviceReady_)return Status::InvalidState;
    events_.push_back({EventType::DeviceLost,0,0,0,device_});deviceReady_=false;return Status::Ok;
}
Status Player::simulateRebuild(uint64_t device,Format format,bool available) {
    if(deviceReady_)return Status::InvalidState;
    if(!device)return Status::InvalidDevice;
    if(!valid(format))return Status::InvalidFormat;
    if(!equal(format,format_))return Status::UnsupportedFormat;
    if(!available)return Status::DeviceUnavailable;
    events_.push_back({EventType::DeviceRebuilt,0,0,0,device});device_=device;deviceReady_=true;return Status::Ok;
}
std::vector<Event> Player::pollEvents() {std::vector<Event> out;out.swap(events_);return out;}
Usage Player::usage() const{Usage out;out.queueItems=queue_.size();out.pendingEvents=events_.size();
    for(const auto& source:sources_)if(source.live){++out.liveSources;out.pcmSamples+=source.samples.size();}return out;}
State Player::state() const {
    State out;out.format=format_;out.playback=playback_;out.deviceReady=deviceReady_;out.device=device_;
    out.nextTicket=nextTicket_;out.counters=counters_;out.events=events_;
    for(size_t i=0;i<sources_.size();++i){const Source& source=sources_[i];
        out.sources[i]={source.generation,source.live,source.references,source.samples};}
    for(const Item& item:queue_)out.queue.push_back({item.handle,item.ticket,item.loops,item.passes,item.cursor,item.delivered});
    return out;
}
} // namespace audio_lab
