#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace audio_lab {
struct Format { uint32_t rate, channels; };
using Handle = uint64_t;
enum class Status { Ok, InvalidFormat, UnsupportedFormat, InvalidPCM, Capacity,
    InvalidHandle, Busy, InvalidLoops, InvalidState, InvalidDevice,
    DeviceUnavailable, InvalidFrameCount };
const char* statusName(Status value);
enum class Playback { Stopped, Running, Paused };
enum class Guard { None, Stopped, Paused, DeviceLost };
enum class EventType { Completed, Cancelled, DeviceLost, DeviceRebuilt };
struct Event {
    EventType type; uint64_t ticket, handle, deliveredFrames, device;
};
struct Counters {
    uint64_t outputFrames = 0, sourceFrames = 0, underrunFrames = 0, blockedFrames = 0;
};
struct Pull {
    std::vector<int16_t> samples;
    size_t sourceFrames = 0, underrunFrames = 0, blockedFrames = 0;
    Guard guard = Guard::None;
};
struct SourceInfo {
    uint32_t generation = 1; bool live = false; size_t queueReferences = 0;
    std::vector<int16_t> samples;
};
struct ItemInfo {
    uint64_t handle, ticket, loops, passes, cursorFrames, deliveredFrames;
};
struct State {
    Format format{}; Playback playback = Playback::Stopped;
    bool deviceReady = true; uint64_t device = 1, nextTicket = 1;
    Counters counters;
    std::array<SourceInfo,16> sources;
    std::vector<ItemInfo> queue;
    std::vector<Event> events;
};
struct Usage {size_t liveSources=0,queueItems=0,pcmSamples=0,pendingEvents=0;};

// An offline pull queue, not an audio device or a mixer.
class Player {
public:
    explicit Player(Format format);
    Status createSource(Format format, const std::vector<int16_t>& samples, Handle& result);
    Status releaseSource(Handle handle);
    Status enqueue(Handle handle, int64_t totalPlays, uint64_t& ticket);
    Status consume(size_t frames, Pull& result);
    Status start(); Status pause(); Status resume(); Status stop(); Status flush();
    Status simulateLoss();
    Status simulateRebuild(uint64_t device, Format format, bool available = true);
    std::vector<Event> pollEvents();
    State state() const;
    Usage usage() const;
private:
    struct Source { uint32_t generation = 1; bool live = false, retired = false;
        size_t references = 0; std::vector<int16_t> samples; };
    struct Item { Handle handle; uint64_t ticket; uint32_t loops;
        uint64_t passes = 0, delivered = 0; size_t cursor = 0; };
    Format format_;
    Playback playback_ = Playback::Stopped;
    bool deviceReady_ = true;
    uint64_t device_ = 1, nextTicket_ = 1;
    Counters counters_;
    std::array<Source,16> sources_;
    std::deque<Item> queue_;
    std::vector<Event> events_;
    Source* find(Handle handle);
};
} // namespace audio_lab
