#pragma once
#include "audio.hpp"

namespace scalar_reference {
class Reference {
public:
    explicit Reference(audio_lab::Format format);
    audio_lab::Status createSource(audio_lab::Format format,const std::vector<int16_t>& samples,audio_lab::Handle& result);
    audio_lab::Status releaseSource(audio_lab::Handle handle);
    audio_lab::Status enqueue(audio_lab::Handle handle,int64_t totalPlays,uint64_t& ticket);
    audio_lab::Status consume(size_t frames,audio_lab::Pull& result);
    audio_lab::Status start(); audio_lab::Status pause(); audio_lab::Status resume();
    audio_lab::Status stop(); audio_lab::Status flush(); audio_lab::Status simulateLoss();
    audio_lab::Status simulateRebuild(uint64_t device,audio_lab::Format format,bool available=true);
    std::vector<audio_lab::Event> pollEvents();
    audio_lab::State state() const;
private:
    struct Tape { audio_lab::Handle handle;uint64_t ticket,loops,offset=0; };
    audio_lab::Format format_;
    audio_lab::Playback mode_=audio_lab::Playback::Stopped;
    bool ready_=true;
    uint64_t device_=1,ticket_=1;
    audio_lab::Counters counters_;
    std::array<audio_lab::SourceInfo,16> clips_;
    std::array<bool,16> retired_{};
    std::vector<Tape> tapes_;
    std::vector<audio_lab::Event> messages_;
    int index(audio_lab::Handle handle) const;
    size_t references(audio_lab::Handle handle) const;
};
} // namespace scalar_reference
