#pragma once
#include "video.hpp"
namespace scalar_reference {
class Reference {
public:
    explicit Reference(uint32_t rate=22050);
    video_lab::Status load(const video_lab::Clip& clip);
    video_lab::Status enqueue(int64_t index,uint64_t epoch);
    video_lab::Status start(video_lab::Clock clock);
    video_lab::Status advance(video_lab::Clock clock,video_lab::Decision& output);
    video_lab::Status pause();video_lab::Status resume();video_lab::Status seek(int64_t ticks);video_lab::Status stop();
    video_lab::State state() const;
private:
    video_lab::State s_;
    std::vector<bool> ready_;
    int desired(uint64_t ticks) const;
    video_lab::Status validClock(video_lab::Clock clock) const;
    video_lab::Decision result(video_lab::Kind kind) const;
};
}
