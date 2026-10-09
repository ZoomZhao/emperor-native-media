#pragma once
#include "movie_io.hpp"
#include "../metal_lab/backend.hpp"
#include <functional>
#include <memory>

namespace stream_lab {
struct RenderResult {uint64_t epoch=0,id=0,pts=0;std::vector<uint8_t> bytes;};
struct RenderStats {size_t submissions=0,applied=0,stale=0,callbacks=0,rebuilds=0,releasedJobs=0;
    size_t maxTickets=0,maxMailbox=0,maxGPUBufferEnvelopeBytes=0,maxGPUBufferEnvelopeCount=0;};
struct RenderUsage {size_t tickets=0,mailbox=0,surfaces=0,surfaceBytes=0,gpuBufferEnvelopeBytes=0;};
// Two asynchronous offscreen transfers. All methods and visitor calls run on the owner thread.
// Native callbacks publish only epoch/id metadata to a two-entry mailbox.
// seek/rebuild changes the epoch before destroying old surfaces. Old completions are fenced,
// dropped, then their tickets are released; a committed Metal command itself is not cancelled.
class EpochPresenter {
public:
    explicit EpochPresenter(uint64_t epoch,
        video_lab::StreamMetadata source={width,height,frameStride,fps,rate,frameCount},
        surface_lab::Desc destination={32,24,133,surface_lab::Format::BGRA8},
        surface_lab::Rect destinationRect={-1,-1,34,26});~EpochPresenter();
    EpochPresenter(const EpochPresenter&)=delete;EpochPresenter& operator=(const EpochPresenter&)=delete;
    // Reject stale epoch, duplicate pending id, or a third pending ticket without submitting.
    bool submit(uint64_t epoch,uint64_t id,const video_lab::Frame& frame);
    void drain(const std::function<void(const RenderResult&)>& visitor);
    void rebuild(uint64_t newEpoch);
    void close();
    RenderUsage usage() const;RenderStats stats() const;std::string deviceName() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace stream_lab
