#pragma once
#include "../video_lab/stream.hpp"
#include <memory>
#include <atomic>
#include <string>
#include <vector>

namespace stream_lab {
// Synthetic fixture only. No output device, window, run loop, or game dependency.
constexpr uint32_t width=64,height=48,fps=30,rate=48000,channels=2;
constexpr uint64_t frameCount=240;
constexpr size_t frameStride=261,pcmPacketFrames=2048,maxNativePCMBytes=65536;
struct IOUsage {size_t readers=0,nativePCMPackets=0,nativePCMBytes=0;};
struct IOStats {size_t readerOpens=0,readerCancels=0,videoPackets=0,audioPackets=0;
    size_t maxNativePCMBytes=0,maxVideoBufferBytes=0,markers=0,metadataTimeouts=0,metadataCancellations=0;};
// One token per open attempt. Other threads may only set cancelled; all native
// asset cancellation and cleanup remain on the Reader's owner thread.
struct OpenControl {
    std::atomic<bool> cancelled{false};
    // Private self-authored regression gate. AVFoundation still loads the real
    // asset, but the owner deliberately waits for cancellation/deadline.
    bool holdMetadataCompletionForTest=false;
};
struct PCM {uint64_t firstFrame=0;std::vector<int16_t> samples;};
struct WriterStats {size_t videoFrames=0,audioFrames=0,poolThreshold=4,maxVideoBufferBytes=0,maxPCMBytes=0;};
WriterStats writeSyntheticMovie(const std::string& path);

// One AVAssetReader at a time; one native PCM sample may be retained between pulls.
// Returned frames/PCM are caller-owned. Owner thread only; destructor cancels reading.
class MovieReader {
public:
    // Explicit expected format: fixed frame rate, rate divisible by fps, stereo S16LE.
    // Formats are verified against the asset and every packet; frame storage <=1 MiB.
    explicit MovieReader(video_lab::StreamMetadata metadata={width,height,frameStride,fps,rate,frameCount});~MovieReader();
    MovieReader(const MovieReader&)=delete;MovieReader& operator=(const MovieReader&)=delete;
    // All metadata shares one 2-second deadline. Timeout/cancellation clears
    // the native session before throwing; callback state never refers to this.
    void open(const std::string& path,uint64_t firstVideoFrame=0,std::shared_ptr<OpenControl> control={});
    void close();
    bool nextFrame(video_lab::Frame& output);
    bool nextPCM(PCM& output,size_t maxFrames=pcmPacketFrames);
    video_lab::StreamMetadata metadata() const;
    IOUsage usage() const;IOStats stats() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace stream_lab
