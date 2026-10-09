#pragma once
#include "video.hpp"
#include <deque>
#include <optional>
namespace video_lab {
// Fixed-frame-rate streaming timeline; it owns at most queueLimit packets plus one held frame.
struct StreamMetadata {uint32_t width=0,height=0;size_t stride=0;uint32_t fps=0,audioRate=0;uint64_t frames=0;};
struct StreamUsage {size_t queuedFrames=0,heldFrames=0,ownedFrameBytes=0;uint64_t epoch=1,mediaTick=0,nextFeed=0;Mode mode=Mode::Stopped;};
// Borrowed frame: valid only until the next advance, seek, stop, open, or destruction.
// Clock.samples counts audio frames, never channel samples. Owner thread only.
struct StreamDecision {Kind kind=Kind::None;uint64_t epoch=0,mediaTick=0;const Frame* frame=nullptr;};
class StreamingPlayer {
public:
    explicit StreamingPlayer(size_t queueLimit=4,size_t maxFrameBytes=1048576);
    Status open(StreamMetadata metadata);Status push(uint64_t epoch,Frame frame);
    Status start(Clock clock);Status advance(Clock clock,StreamDecision& output);
    Status pause();Status resume();Status seek(uint64_t frameIndex);Status stop();
    StreamUsage usage() const;StreamMetadata metadata() const{return meta_;}
private:
    StreamMetadata meta_{};bool loaded_=false,bound_=false;Mode mode_=Mode::Stopped;
    uint64_t epoch_=1,tick_=0,next_=0,lastClock_=0,anchorClock_=0,anchorTick_=0;
    size_t queueLimit_,maxFrameBytes_;std::deque<Frame> queue_;std::optional<Frame> held_;
    Status clockStatus(Clock clock) const;StreamDecision decision(Kind kind) const;Status reset(uint64_t target,bool stop);
};
} // namespace video_lab
