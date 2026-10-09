#include "stream.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace video_lab {
StreamingPlayer::StreamingPlayer(size_t limit,size_t bytes):queueLimit_(limit),maxFrameBytes_(bytes){
    if(!limit||limit>16||!bytes)throw std::invalid_argument("stream limits");}
Status StreamingPlayer::open(StreamMetadata m){if(loaded_&&mode_!=Mode::Stopped&&mode_!=Mode::Ended)return Status::InvalidState;
    if(!m.width||!m.height||m.width>4096||m.height>4096||m.stride<size_t(m.width)*4||m.stride>size_t(m.width)*4+64||
        m.stride>maxFrameBytes_/m.height||!m.fps||m.fps>240||m.audioRate<8000||m.audioRate>192000||!m.frames||m.frames>1000000)return Status::InvalidMetadata;
    if(loaded_&&epoch_==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    if(loaded_)++epoch_;meta_=m;loaded_=true;queue_.clear();held_.reset();mode_=Mode::Stopped;tick_=next_=0;bound_=false;lastClock_=anchorClock_=anchorTick_=0;return Status::Ok;}
Status StreamingPlayer::push(uint64_t epoch,Frame frame){if(!loaded_)return Status::NoClip;if(epoch!=epoch_)return Status::StaleEpoch;
    if(mode_==Mode::Ended)return Status::InvalidState;if(frame.pts!=next_||frame.pts>=meta_.frames)return Status::OutOfOrder;
    if(frame.bgra.size()!=meta_.stride*meta_.height)return Status::InvalidFrame;
    if(queue_.size()==queueLimit_)return Status::Backpressure;queue_.push_back(std::move(frame));++next_;return Status::Ok;}
Status StreamingPlayer::clockStatus(Clock c) const{if(c.epoch!=epoch_)return Status::StaleEpoch;
    if(c.sampleRate!=meta_.audioRate)return Status::InvalidClockRate;if(bound_&&c.samples<lastClock_)return Status::ClockRegression;return Status::Ok;}
Status StreamingPlayer::start(Clock c){if(!loaded_)return Status::NoClip;if(mode_!=Mode::Stopped)return Status::InvalidState;
    const auto status=clockStatus(c);if(status!=Status::Ok)return status;bound_=true;lastClock_=anchorClock_=c.samples;anchorTick_=tick_;mode_=Mode::Running;return Status::Ok;}
StreamDecision StreamingPlayer::decision(Kind kind) const{return {kind,epoch_,tick_,held_?&*held_:nullptr};}
Status StreamingPlayer::advance(Clock c,StreamDecision& out){if(!loaded_)return Status::NoClip;
    if(mode_!=Mode::Running&&mode_!=Mode::Paused)return Status::InvalidState;const auto status=clockStatus(c);if(status!=Status::Ok)return status;
    const uint64_t delta=bound_?c.samples-anchorClock_:0;if(mode_==Mode::Running&&delta>std::numeric_limits<uint64_t>::max()/meta_.fps)return Status::ClockOverflow;
    if(!bound_){bound_=true;anchorClock_=c.samples;anchorTick_=tick_;}lastClock_=c.samples;
    if(mode_==Mode::Paused){out=decision(Kind::Paused);return Status::Ok;}
    tick_=anchorTick_+std::min(delta*meta_.fps/meta_.audioRate,meta_.frames-anchorTick_);
    if(tick_==meta_.frames){queue_.clear();next_=meta_.frames;mode_=Mode::Ended;out=decision(Kind::Eof);return Status::Ok;}
    while(!queue_.empty()&&queue_.front().pts<tick_)queue_.pop_front();if(next_<tick_)next_=tick_;
    Kind kind=Kind::None;if(!queue_.empty()&&queue_.front().pts==tick_){held_=std::move(queue_.front());queue_.pop_front();kind=Kind::Presented;}
    else if(held_&&held_->pts==tick_)kind=Kind::Repeat;else if(held_)kind=Kind::Hold;out=decision(kind);return Status::Ok;}
Status StreamingPlayer::pause(){if(!loaded_)return Status::NoClip;if(mode_!=Mode::Running)return Status::InvalidState;mode_=Mode::Paused;return Status::Ok;}
Status StreamingPlayer::resume(){if(!loaded_)return Status::NoClip;if(mode_!=Mode::Paused)return Status::InvalidState;
    anchorClock_=lastClock_;anchorTick_=tick_;mode_=Mode::Running;return Status::Ok;}
Status StreamingPlayer::reset(uint64_t target,bool stop){if(epoch_==std::numeric_limits<uint64_t>::max())return Status::EpochExhausted;
    ++epoch_;tick_=next_=target;queue_.clear();held_.reset();bound_=false;lastClock_=anchorClock_=0;anchorTick_=target;
    if(stop||mode_==Mode::Ended)mode_=Mode::Stopped;return Status::Ok;}
Status StreamingPlayer::seek(uint64_t target){if(!loaded_)return Status::NoClip;if(target>=meta_.frames)return Status::InvalidSeek;return reset(target,false);}
Status StreamingPlayer::stop(){if(!loaded_)return Status::NoClip;return reset(0,true);}
StreamUsage StreamingPlayer::usage() const{StreamUsage out{queue_.size(),held_?size_t(1):size_t(0),0,epoch_,tick_,next_,mode_};
    for(const auto& frame:queue_)out.ownedFrameBytes+=frame.bgra.size();if(held_)out.ownedFrameBytes+=held_->bgra.size();return out;}
} // namespace video_lab
