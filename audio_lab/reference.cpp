#include "reference.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace scalar_reference {
using namespace audio_lab;
namespace {
bool accepted(Format f){return 8000<=f.rate&&f.rate<=192000&&0<f.channels&&f.channels<9;}
bool matches(Format a,Format b){return a.channels==b.channels&&a.rate==b.rate;}
}
Reference::Reference(Format format):format_(format){if(!accepted(format))throw std::invalid_argument("reference format");}
int Reference::index(Handle handle) const {
    const uint64_t slot=handle%4294967296ULL,epoch=handle/4294967296ULL;
    if(slot<1||slot>16)return -1;
    const auto& clip=clips_[size_t(slot-1)];
    return clip.live&&clip.generation==epoch?int(slot-1):-1;
}
size_t Reference::references(Handle handle) const {
    size_t count=0;for(const auto& tape:tapes_)if(tape.handle==handle)++count;return count;
}
Status Reference::createSource(Format format,const std::vector<int16_t>& samples,Handle& out){
    out=0;if(!accepted(format))return Status::InvalidFormat;
    if(!matches(format,format_))return Status::UnsupportedFormat;
    if(samples.size()==0||samples.size()%format_.channels!=0||samples.size()/format_.channels>65536)return Status::InvalidPCM;
    for(size_t slot=0;slot<clips_.size();++slot){auto& clip=clips_[slot];
        if(clip.live||retired_[slot])continue;
        clip.samples=std::vector<int16_t>(samples.begin(),samples.end());clip.live=true;
        out=uint64_t(clip.generation)*4294967296ULL+slot+1;return Status::Ok;}
    return Status::Capacity;
}
Status Reference::releaseSource(Handle handle){
    const int slot=index(handle);if(slot<0)return Status::InvalidHandle;
    if(references(handle)>0)return Status::Busy;
    auto& clip=clips_[size_t(slot)];clip.live=false;clip.samples.clear();
    if(clip.generation<std::numeric_limits<uint32_t>::max())++clip.generation;
    else retired_[size_t(slot)]=true;
    return Status::Ok;
}
Status Reference::enqueue(Handle handle,int64_t loops,uint64_t& out){
    out=0;if(index(handle)<0)return Status::InvalidHandle;
    if(loops<0||loops>2147483647LL)return Status::InvalidLoops;
    if(tapes_.size()>=8||ticket_==std::numeric_limits<uint64_t>::max())return Status::Capacity;
    tapes_.push_back({handle,ticket_,uint64_t(loops),0});out=ticket_++;return Status::Ok;
}
Status Reference::consume(size_t frames,Pull& result){
    if(frames<1||frames>4096)return Status::InvalidFrameCount;
    Pull out;out.samples.resize(frames*format_.channels,0);
    if(!ready_)out.guard=Guard::DeviceLost;
    else if(mode_==Playback::Paused)out.guard=Guard::Paused;
    else if(mode_==Playback::Stopped)out.guard=Guard::Stopped;
    for(size_t frame=0;frame<frames;++frame){
        if(out.guard!=Guard::None){++out.blockedFrames;continue;}
        if(tapes_.empty()){++out.underrunFrames;continue;}
        Tape& tape=tapes_.front();const int slot=index(tape.handle);
        if(slot<0)throw std::logic_error("reference source not live");
        const auto& clip=clips_[size_t(slot)];const uint64_t length=clip.samples.size()/format_.channels;
        const uint64_t selected=tape.offset%length;
        for(size_t channel=0;channel<format_.channels;++channel)
            out.samples[frame*format_.channels+channel]=clip.samples[size_t(selected)*format_.channels+channel];
        ++tape.offset;++out.sourceFrames;
        if(tape.loops!=0&&tape.offset==tape.loops*length){
            messages_.push_back({EventType::Completed,tape.ticket,tape.handle,tape.offset,device_});
            tapes_.erase(tapes_.begin());}
    }
    counters_.outputFrames+=frames;counters_.sourceFrames+=out.sourceFrames;
    counters_.underrunFrames+=out.underrunFrames;counters_.blockedFrames+=out.blockedFrames;
    result=std::move(out);return Status::Ok;
}
Status Reference::start(){if(mode_!=Playback::Stopped)return Status::InvalidState;
    if(!ready_)return Status::DeviceUnavailable;mode_=Playback::Running;return Status::Ok;}
Status Reference::pause(){if(mode_!=Playback::Running)return Status::InvalidState;mode_=Playback::Paused;return Status::Ok;}
Status Reference::resume(){if(mode_!=Playback::Paused)return Status::InvalidState;
    if(!ready_)return Status::DeviceUnavailable;mode_=Playback::Running;return Status::Ok;}
Status Reference::stop(){mode_=Playback::Stopped;return Status::Ok;}
Status Reference::flush(){for(const auto& tape:tapes_)messages_.push_back({EventType::Cancelled,tape.ticket,tape.handle,tape.offset,device_});
    tapes_.clear();mode_=Playback::Stopped;return Status::Ok;}
Status Reference::simulateLoss(){if(!ready_)return Status::InvalidState;
    messages_.push_back({EventType::DeviceLost,0,0,0,device_});ready_=false;return Status::Ok;}
Status Reference::simulateRebuild(uint64_t device,Format format,bool available){
    if(ready_)return Status::InvalidState;if(device==0)return Status::InvalidDevice;
    if(!accepted(format))return Status::InvalidFormat;if(!matches(format,format_))return Status::UnsupportedFormat;
    if(!available)return Status::DeviceUnavailable;
    ready_=true;device_=device;messages_.push_back({EventType::DeviceRebuilt,0,0,0,device});return Status::Ok;}
std::vector<Event> Reference::pollEvents(){auto result=messages_;messages_.clear();return result;}
State Reference::state() const {
    State out;out.format=format_;out.playback=mode_;out.deviceReady=ready_;out.device=device_;out.nextTicket=ticket_;
    out.counters=counters_;out.sources=clips_;out.events=messages_;
    for(size_t i=0;i<clips_.size();++i){const auto& clip=clips_[i];const Handle handle=uint64_t(clip.generation)*4294967296ULL+i+1;
        out.sources[i].queueReferences=references(handle);}
    for(const auto& tape:tapes_){const auto& clip=clips_[size_t(index(tape.handle))];const uint64_t length=clip.samples.size()/format_.channels;
        out.queue.push_back({tape.handle,tape.ticket,tape.loops,tape.offset/length,tape.offset%length,tape.offset});}
    return out;
}
} // namespace scalar_reference
