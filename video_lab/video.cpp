#include "video.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace video_lab {
namespace {
bool metadata(const Clip& c) {
    if(!c.width||!c.height||c.width>64||c.height>64||c.stride<size_t(c.width)*4||c.stride>size_t(c.width)*4+64||
        !c.ticksPerSecond||c.ticksPerSecond>1000000||!c.duration||c.duration>1000000000000ULL||
        !c.declaredFrames||c.declaredFrames>256||c.frames.size()!=c.declaredFrames)return false;
    uint64_t previous=0;
    for(size_t i=0;i<c.frames.size();++i){const auto& f=c.frames[i];
        if(f.bgra.size()!=c.stride*c.height||f.pts>=c.duration||(i==0&&f.pts!=0)||(i>0&&f.pts<=previous))return false;
        previous=f.pts;}
    return true;
}
}
const char* statusName(Status s){switch(s){case Status::Ok:return "OK";case Status::NoClip:return "NO_CLIP";
    case Status::InvalidMetadata:return "INVALID_METADATA";case Status::InvalidState:return "INVALID_STATE";
    case Status::StaleEpoch:return "STALE_EPOCH";case Status::InvalidFrame:return "INVALID_FRAME";
    case Status::Backpressure:return "BACKPRESSURE";case Status::OutOfOrder:return "OUT_OF_ORDER";
    case Status::InvalidClockRate:return "INVALID_CLOCK_RATE";case Status::ClockRegression:return "CLOCK_REGRESSION";
    case Status::ClockOverflow:return "CLOCK_OVERFLOW";case Status::InvalidSeek:return "INVALID_SEEK";
    case Status::EpochExhausted:return "EPOCH_EXHAUSTED";}return "UNKNOWN";}
Player::Player(uint32_t rate){if(rate<8000||rate>192000)throw std::invalid_argument("audio rate outside 8000..192000");s_.audioRate=rate;}
Status Player::load(const Clip& clip){
    if(s_.loaded&&s_.mode!=Mode::Stopped)return Status::InvalidState;
    if(!metadata(clip))return Status::InvalidMetadata;
    if(s_.loaded&&s_.epoch==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    State staging;staging.loaded=true;staging.clip=clip;staging.audioRate=s_.audioRate;
    staging.epoch=s_.loaded?s_.epoch+1:s_.epoch;s_=std::move(staging);return Status::Ok;
}
int Player::wanted(uint64_t ticks) const {
    const auto it=std::upper_bound(s_.clip.frames.begin(),s_.clip.frames.end(),ticks,
        [](uint64_t t,const Frame& f){return t<f.pts;});
    return int(it-s_.clip.frames.begin())-1;
}
Status Player::enqueue(int64_t index,uint64_t epoch){
    if(!s_.loaded)return Status::NoClip;if(epoch!=s_.epoch)return Status::StaleEpoch;
    if(index<0||index>=int64_t(s_.clip.frames.size()))return Status::InvalidFrame;
    if(index!=s_.nextFeed)return Status::OutOfOrder;
    if(s_.queue.size()==4)return Status::Backpressure;
    s_.queue.push_back(int(index));++s_.nextFeed;return Status::Ok;
}
Status Player::clockStatus(Clock clock) const {
    if(clock.epoch!=s_.epoch)return Status::StaleEpoch;
    if(clock.sampleRate!=s_.audioRate)return Status::InvalidClockRate;
    if(s_.clockBound&&clock.samples<s_.lastSamples)return Status::ClockRegression;
    return Status::Ok;
}
Status Player::start(Clock clock){
    if(!s_.loaded)return Status::NoClip;if(s_.mode!=Mode::Stopped)return Status::InvalidState;
    const Status status=clockStatus(clock);if(status!=Status::Ok)return status;
    s_.clockBound=true;s_.lastSamples=clock.samples;s_.anchorSamples=clock.samples;s_.anchorTicks=s_.mediaTicks;
    s_.mode=Mode::Running;return Status::Ok;
}
Decision Player::decision(Kind kind) const {
    Decision out;out.kind=kind;out.epoch=s_.epoch;out.mediaTicks=s_.mediaTicks;out.index=s_.shown;
    if(s_.shown>=0){const auto& f=s_.clip.frames[size_t(s_.shown)];out.pts=f.pts;out.bgra=f.bgra;}return out;
}
Status Player::advance(Clock clock,Decision& output){
    if(!s_.loaded)return Status::NoClip;
    if(s_.mode!=Mode::Running&&s_.mode!=Mode::Paused)return Status::InvalidState;
    const Status status=clockStatus(clock);if(status!=Status::Ok)return status;
    const uint64_t delta=s_.clockBound?clock.samples-s_.anchorSamples:0;
    if(s_.mode==Mode::Running&&delta>std::numeric_limits<uint64_t>::max()/s_.clip.ticksPerSecond)return Status::ClockOverflow;
    if(!s_.clockBound){s_.anchorSamples=clock.samples;s_.anchorTicks=s_.mediaTicks;s_.clockBound=true;}
    s_.lastSamples=clock.samples;++s_.counters.advances;
    if(s_.mode==Mode::Paused){output=decision(Kind::Paused);return Status::Ok;}
    const uint64_t elapsed=delta*s_.clip.ticksPerSecond/s_.audioRate;
    s_.mediaTicks=s_.anchorTicks+std::min(elapsed,s_.clip.duration-s_.anchorTicks);
    if(s_.mediaTicks==s_.clip.duration){
        s_.counters.dropped+=s_.queue.size();s_.queue.clear();s_.nextFeed=int(s_.clip.frames.size());
        s_.mode=Mode::Ended;if(!s_.eofSent){++s_.counters.eofs;s_.eofSent=true;}output=decision(Kind::Eof);return Status::Ok;
    }
    const int desired=wanted(s_.mediaTicks);
    while(!s_.queue.empty()&&s_.queue.front()<desired){s_.queue.erase(s_.queue.begin());++s_.counters.dropped;}
    if(s_.nextFeed<desired){s_.counters.skippedUnfed+=uint64_t(desired-s_.nextFeed);s_.nextFeed=desired;}
    Kind kind=Kind::None;
    if(!s_.queue.empty()&&s_.queue.front()==desired){s_.shown=desired;s_.queue.erase(s_.queue.begin());++s_.counters.presented;kind=Kind::Presented;}
    else if(s_.shown==desired){++s_.counters.repeated;kind=Kind::Repeat;}
    else if(s_.shown>=0){++s_.counters.held;kind=Kind::Hold;}
    output=decision(kind);return Status::Ok;
}
Status Player::pause(){if(!s_.loaded)return Status::NoClip;if(s_.mode!=Mode::Running)return Status::InvalidState;s_.mode=Mode::Paused;return Status::Ok;}
Status Player::resume(){if(!s_.loaded)return Status::NoClip;if(s_.mode!=Mode::Paused)return Status::InvalidState;
    s_.anchorSamples=s_.lastSamples;s_.anchorTicks=s_.mediaTicks;s_.mode=Mode::Running;return Status::Ok;}
Status Player::seek(int64_t ticks){
    if(!s_.loaded)return Status::NoClip;if(ticks<0||uint64_t(ticks)>=s_.clip.duration)return Status::InvalidSeek;
    if(s_.epoch==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    ++s_.epoch;s_.mediaTicks=uint64_t(ticks);s_.anchorTicks=s_.mediaTicks;s_.anchorSamples=0;s_.lastSamples=0;
    s_.clockBound=false;s_.eofSent=false;s_.queue.clear();s_.shown=-1;s_.nextFeed=wanted(s_.mediaTicks);
    if(s_.mode==Mode::Ended)s_.mode=Mode::Stopped;return Status::Ok;
}
Status Player::stop(){if(!s_.loaded)return Status::NoClip;if(s_.epoch==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    ++s_.epoch;s_.mode=Mode::Stopped;s_.clockBound=false;s_.mediaTicks=0;s_.anchorTicks=0;s_.anchorSamples=0;s_.lastSamples=0;
    s_.queue.clear();s_.shown=-1;s_.nextFeed=0;s_.eofSent=false;return Status::Ok;}
} // namespace video_lab
