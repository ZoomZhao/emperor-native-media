#include "reference.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace scalar_reference {
using namespace video_lab;
namespace {
bool shape(const Clip& c){
    if(c.width<1||c.width>64||c.height<1||c.height>64)return false;
    if(c.stride<c.width*4u||c.stride-c.width*4u>64)return false;
    if(c.ticksPerSecond<1||c.ticksPerSecond>1000000||c.duration<1||c.duration>1000000000000ULL)return false;
    if(c.frames.size()<1||c.frames.size()>256||c.frames.size()!=c.declaredFrames)return false;
    std::vector<uint64_t> pts;
    for(const auto& f:c.frames){if(f.bgra.size()/c.height!=c.stride||f.bgra.size()%c.height!=0||f.pts>=c.duration)return false;pts.push_back(f.pts);}
    return pts.front()==0&&std::is_sorted(pts.begin(),pts.end())&&std::adjacent_find(pts.begin(),pts.end())==pts.end();
}
}
Reference::Reference(uint32_t rate){if(rate<8000||rate>192000)throw std::invalid_argument("reference rate");s_.audioRate=rate;}
Status Reference::load(const Clip& c){if(s_.loaded&&s_.mode!=Mode::Stopped)return Status::InvalidState;
    if(!shape(c))return Status::InvalidMetadata;if(s_.loaded&&s_.epoch==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    const auto rate=s_.audioRate;const auto epoch=s_.loaded?s_.epoch+1:s_.epoch;
    s_=State{};s_.loaded=true;s_.audioRate=rate;s_.epoch=epoch;s_.clip=c;ready_.assign(c.frames.size(),false);return Status::Ok;}
int Reference::desired(uint64_t ticks) const{int chosen=0;for(size_t i=0;i<s_.clip.frames.size();++i)if(s_.clip.frames[i].pts<=ticks)chosen=int(i);return chosen;}
Status Reference::enqueue(int64_t index,uint64_t epoch){if(!s_.loaded)return Status::NoClip;if(epoch!=s_.epoch)return Status::StaleEpoch;
    if(index<0||uint64_t(index)>=ready_.size())return Status::InvalidFrame;
    if(index!=s_.nextFeed)return Status::OutOfOrder;
    if(std::count(ready_.begin(),ready_.end(),true)==4)return Status::Backpressure;
    ready_[size_t(index)]=true;++s_.nextFeed;return Status::Ok;}
Status Reference::validClock(Clock c) const{if(c.epoch!=s_.epoch)return Status::StaleEpoch;
    if(c.sampleRate!=s_.audioRate)return Status::InvalidClockRate;
    if(s_.clockBound&&c.samples<s_.lastSamples)return Status::ClockRegression;return Status::Ok;}
Status Reference::start(Clock c){if(!s_.loaded)return Status::NoClip;if(s_.mode!=Mode::Stopped)return Status::InvalidState;
    const auto status=validClock(c);if(status!=Status::Ok)return status;
    s_.clockBound=true;s_.lastSamples=c.samples;s_.anchorSamples=c.samples;s_.anchorTicks=s_.mediaTicks;s_.mode=Mode::Running;return Status::Ok;}
Decision Reference::result(Kind kind) const{Decision out;out.kind=kind;out.epoch=s_.epoch;out.mediaTicks=s_.mediaTicks;out.index=s_.shown;
    if(out.index!=-1){const auto& f=s_.clip.frames[size_t(out.index)];out.pts=f.pts;out.bgra=f.bgra;}return out;}
Status Reference::advance(Clock c,Decision& out){if(!s_.loaded)return Status::NoClip;
    if(s_.mode!=Mode::Running&&s_.mode!=Mode::Paused)return Status::InvalidState;
    const auto status=validClock(c);if(status!=Status::Ok)return status;
    const uint64_t delta=s_.clockBound?c.samples-s_.anchorSamples:0;
    if(s_.mode==Mode::Running&&delta>std::numeric_limits<uint64_t>::max()/s_.clip.ticksPerSecond)return Status::ClockOverflow;
    if(!s_.clockBound){s_.clockBound=true;s_.anchorSamples=c.samples;s_.anchorTicks=s_.mediaTicks;}
    s_.lastSamples=c.samples;++s_.counters.advances;
    if(s_.mode==Mode::Paused){out=result(Kind::Paused);return Status::Ok;}
    const uint64_t whole=delta/s_.audioRate,part=delta%s_.audioRate;
    const uint64_t ticks=whole*s_.clip.ticksPerSecond+part*s_.clip.ticksPerSecond/s_.audioRate;
    s_.mediaTicks=s_.anchorTicks+std::min(ticks,s_.clip.duration-s_.anchorTicks);
    if(s_.mediaTicks>=s_.clip.duration){for(bool bit:ready_)if(bit)++s_.counters.dropped;
        ready_.assign(ready_.size(),false);s_.nextFeed=int(ready_.size());s_.mode=Mode::Ended;
        if(!s_.eofSent){++s_.counters.eofs;s_.eofSent=true;}out=result(Kind::Eof);return Status::Ok;}
    const int chosen=desired(s_.mediaTicks);
    for(int index=0;index<chosen;++index)if(ready_[size_t(index)]){ready_[size_t(index)]=false;++s_.counters.dropped;}
    if(s_.nextFeed<chosen){s_.counters.skippedUnfed+=uint64_t(chosen-s_.nextFeed);s_.nextFeed=chosen;}
    if(ready_[size_t(chosen)]){ready_[size_t(chosen)]=false;s_.shown=chosen;++s_.counters.presented;out=result(Kind::Presented);}
    else if(s_.shown==chosen){++s_.counters.repeated;out=result(Kind::Repeat);}
    else if(s_.shown!=-1){++s_.counters.held;out=result(Kind::Hold);}
    else out=result(Kind::None);return Status::Ok;}
Status Reference::pause(){if(!s_.loaded)return Status::NoClip;if(s_.mode!=Mode::Running)return Status::InvalidState;s_.mode=Mode::Paused;return Status::Ok;}
Status Reference::resume(){if(!s_.loaded)return Status::NoClip;if(s_.mode!=Mode::Paused)return Status::InvalidState;
    s_.anchorSamples=s_.lastSamples;s_.anchorTicks=s_.mediaTicks;s_.mode=Mode::Running;return Status::Ok;}
Status Reference::seek(int64_t ticks){if(!s_.loaded)return Status::NoClip;
    if(ticks<0||uint64_t(ticks)>=s_.clip.duration)return Status::InvalidSeek;
    if(s_.epoch==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    s_.epoch+=1;s_.mediaTicks=uint64_t(ticks);s_.anchorTicks=s_.mediaTicks;s_.lastSamples=0;s_.anchorSamples=0;s_.clockBound=false;
    s_.eofSent=false;s_.shown=-1;s_.nextFeed=desired(s_.mediaTicks);ready_.assign(ready_.size(),false);
    if(s_.mode==Mode::Ended)s_.mode=Mode::Stopped;return Status::Ok;}
Status Reference::stop(){if(!s_.loaded)return Status::NoClip;if(s_.epoch==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    s_.epoch+=1;s_.mode=Mode::Stopped;s_.shown=-1;s_.nextFeed=0;s_.clockBound=false;s_.eofSent=false;
    s_.mediaTicks=0;s_.anchorTicks=0;s_.anchorSamples=0;s_.lastSamples=0;ready_.assign(ready_.size(),false);return Status::Ok;}
State Reference::state() const{auto out=s_;out.queue.clear();for(size_t i=0;i<ready_.size();++i)if(ready_[i])out.queue.push_back(int(i));return out;}
}
