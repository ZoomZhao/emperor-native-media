#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace video_lab {
struct Frame { uint64_t pts = 0; std::vector<uint8_t> bgra; };
struct Clip {
    uint32_t width = 0, height = 0;
    size_t stride = 0;
    uint64_t ticksPerSecond = 0, duration = 0;
    uint32_t declaredFrames = 0;
    std::vector<Frame> frames;
};
struct Clock { uint64_t epoch, samples; uint32_t sampleRate; }; // samples means audio frames, not channel samples
enum class Status { Ok, NoClip, InvalidMetadata, InvalidState, StaleEpoch,
    InvalidFrame, Backpressure, OutOfOrder, InvalidClockRate, ClockRegression,
    ClockOverflow, InvalidSeek, EpochExhausted };
const char* statusName(Status status);
enum class Mode { Stopped, Running, Paused, Ended };
enum class Kind { None, Presented, Repeat, Hold, Paused, Eof };
struct Decision { Kind kind = Kind::None; uint64_t epoch = 0, mediaTicks = 0, pts = 0;
    int index = -1; std::vector<uint8_t> bgra; };
struct Counters { uint64_t advances = 0, presented = 0, repeated = 0, held = 0,
    dropped = 0, skippedUnfed = 0, eofs = 0; };
struct State {
    bool loaded = false, clockBound = false, eofSent = false;
    Clip clip;
    uint32_t audioRate = 22050;
    Mode mode = Mode::Stopped;
    uint64_t epoch = 1, mediaTicks = 0, lastSamples = 0, anchorSamples = 0, anchorTicks = 0;
    int nextFeed = 0, shown = -1;
    std::vector<int> queue;
    Counters counters;
};
class Player {
public:
    explicit Player(uint32_t simulatedAudioRate = 22050);
    Status load(const Clip& clip);
    Status enqueue(int64_t frameIndex, uint64_t epoch);
    Status start(Clock clock);
    Status advance(Clock clock, Decision& output);
    Status pause(); Status resume(); Status seek(int64_t ticks); Status stop();
    State state() const { return s_; }
private:
    State s_;
    int wanted(uint64_t ticks) const;
    Status clockStatus(Clock clock) const;
    Decision decision(Kind kind) const;
};
} // namespace video_lab
