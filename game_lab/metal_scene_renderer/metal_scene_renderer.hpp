// SPDX-License-Identifier: MIT
// Persistent offscreen Metal backend for game_lab::SceneDrawList.
//
// Consumes the immutable draw-list snapshot and the App's SceneFrameTicket and
// produces one completed, immutable BGRA8Unorm (non-sRGB) city-sized texture
// per accepted request. It never decides which request is current, never calls
// AppKit, never presents, and never reads pixels back on the normal path.
#pragma once
#include "../scene_draw_list.hpp"
#include "../scene_frame_gate.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

namespace game_lab::metal_scene {

enum class SubmitStatus { Accepted, Busy, Unsupported, Closed, Failed };
struct SubmitResult {
    SubmitStatus status=SubmitStatus::Failed;
    std::string reason; // empty only for Accepted
};
enum class CompletionStatus { Ready, Failed, Cancelled };
enum class BlendPath { None, FixedFunction, Programmable };
enum class SelfTestState { Pending, Passed, Failed };

const char* submitStatusName(SubmitStatus);
const char* completionStatusName(CompletionStatus);
const char* blendPathName(BlendPath);

struct Capabilities {
    bool available=false;            // device + pipelines + a self-tested exact blend path
    SelfTestState selfTest=SelfTestState::Pending;
    std::string device;              // MTLDevice.name
    uint64_t registryId=0;
    BlendPath blend=BlendPath::None; // path used for every accepted request
    bool fixedExact=false,programmableExact=false; // self-test outcomes
    std::string failure;             // named reason when !available
    std::string fixedDetail,programmableDetail;    // per-path self-test detail
    std::string pixelFormat="BGRA8Unorm";          // output and intermediate format, never sRGB
    uint64_t backendGeneration=0;    // process-unique per Renderer instance
    uint64_t deviceGeneration=0;     // device instance used by this backend (== backendGeneration; a reset is a new Renderer)
    uint64_t resourceGeneration=0;   // advanced by invalidateResources()
    size_t cacheBudgetBytes=0,cacheEntryLimit=0,maxInFlight=2;
    size_t requestPinBudgetBytes=0;  // per accepted request, actual allocatedSize
    size_t selfTestTransientBytes=0; // actual bytes the one-time startup self-test held (released after it)
    uint32_t maxTextureDimension=0;
};

struct FrameDiagnostics {
    uint64_t backendGeneration=0,deviceGeneration=0,resourceGeneration=0;
    BlendPath blend=BlendPath::None;
    size_t items=0,drawnItems=0,culledItems=0,drawCalls=0;
    size_t resources=0,resourcesDrawn=0,cacheHits=0,uploads=0,uncachedUploads=0;
    size_t uploadBytes=0,backgroundBytes=0;        // logical bytes copied (w*h*4)
    size_t uploadAllocatedBytes=0,uncachedBytes=0; // actual allocatedSize of new sprite textures / of those not admitted
    size_t pinnedTextures=0,pinnedBytes=0; // held by this request's command buffer; pinnedBytes = actual allocatedSize
    double encodeMs=0,gpuMs=0,acceptToCompleteMs=0;
};

// Immutable completed output. Owns its texture outright: not a scratch, ring
// or cache texture, never written after completion. Valid for as long as the
// shared_ptr is retained, independent of cache eviction, input release,
// invalidateResources(), shutdown() or Renderer destruction.
class Frame {
public:
    struct Impl;
    explicit Frame(std::unique_ptr<Impl>);
    ~Frame();
    Frame(const Frame&)=delete;Frame& operator=(const Frame&)=delete;
    uint32_t width() const;
    uint32_t height() const;
    const char* pixelFormatName() const;   // "BGRA8Unorm"
    size_t allocatedBytes() const;
    const FrameDiagnostics& diagnostics() const;
#ifdef __OBJC__
    id<MTLTexture> texture() const;        // private storage, RenderTarget|ShaderRead
#endif
private:
    std::unique_ptr<Impl> impl_;
};

struct Completion {
    CompletionStatus status=CompletionStatus::Failed;
    SceneFrameTicket ticket;                // the submitted ticket, unchanged
    std::shared_ptr<const Frame> frame;     // non-null only for Ready
    std::string reason;                     // named reason for Failed/Cancelled
};
// Invoked exactly once per Accepted submit, on the backend's private serial
// callback queue (never the main thread, never under a backend lock). The App
// integration layer captures only a weak owner and hops to the main thread.
using CompletionHandler=std::function<void(Completion)>;

// Byte accounting uses actual MTLResource.allocatedSize unless named "logical".
// Counter lifetimes:
//  * accepted/busy/unsupported/closed/refused, ready/failed/cancelled: monotonic per Renderer.
//  * uploads/hits/uncachedUploads/evictions/invalidatedEntries/staleReplacements: monotonic.
//  * uploadBytes (logical copied, sprites+background), uploadAllocatedBytes, uncachedBytes: monotonic.
//  * retainedBytes/retainedEntries: current resident cache, <= cacheBudgetBytes / cacheEntryLimit
//    (retainedLogicalBytes: same entries at w*h*4; peakRetainedBytes: max retainedBytes).
//  * inFlight: accepted requests whose GPU work (or skip) has not finished; at most 2.
//    Drops when the command buffer completes, even if the callback was a shutdown Cancelled.
//  * pinnedTextures/pinnedBytes: resources currently held by in-flight command buffers
//    (sprites incl. cached, uncached and evicted ones, background, intermediate, output,
//    instance buffer); pinnedBytes <= maxInFlight * requestPinBudgetBytes. May overlap retainedBytes.
//  * liveFrames/liveFrameBytes: Frame objects still owned by anybody (caller-controlled).
struct Counters {
    uint64_t accepted=0,busy=0,unsupported=0,closed=0,refused=0;
    uint64_t ready=0,failed=0,cancelled=0;
    uint64_t uploads=0,hits=0,uncachedUploads=0,evictions=0,invalidatedEntries=0,staleReplacements=0;
    uint64_t uploadBytes=0,uploadAllocatedBytes=0,uncachedBytes=0;
    size_t retainedBytes=0,retainedLogicalBytes=0,retainedEntries=0,peakRetainedBytes=0;
    size_t inFlight=0,pinnedTextures=0,pinnedBytes=0,peakPinnedBytes=0;
    size_t liveFrames=0,liveFrameBytes=0;
};

// Memory the backend controls (actual allocatedSize):
//  * resident sprite cache  <= cacheBudgetBytes and cacheEntryLimit entries;
//  * each accepted request pins <= requestPinBudgetBytes (its sprites, cached or
//    not, background, intermediate layer, output, instance buffer); over budget ->
//    that request completes Failed("request_pin_budget_exceeded"), nothing drawn;
//  * at most 2 accepted requests in flight (Busy otherwise).
// Pins may overlap the cache, so backend-held GPU memory <= cacheBudgetBytes +
// 2*requestPinBudgetBytes, plus a one-time startup self-test (selfTestTransientBytes).
// Completed Frames are owned by the caller (liveFrameBytes) and are never
// reclaimed, overwritten or reused by the backend to meet any budget.
struct Options {
    size_t cacheBudgetBytes=64u*1024u*1024u;  // resident cache cap; a sprite larger than this goes uncached
    size_t cacheEntryLimit=2048;
    size_t requestPinBudgetBytes=256u*1024u*1024u; // per-request pinned cap
    // Test-only: use this path (if it self-tested exact) instead of preferring fixed.
    BlendPath forceBlendForTesting=BlendPath::None;
};

// Test-only fault bits; injectFaultForTesting ORs them into the next accepted request.
// The Nil* faults substitute nil for that nullable encoder factory and exercise the
// production nil-check branch.
enum class TestFault : uint32_t { None=0, FailBeforeCommit=1, FailAtCompletion=2, DelayEncodeMs50=4,
    NilSpriteEncoder=8, NilBackgroundBlit=16, NilCompositeEncoder=32 };

class Renderer {
public:
    explicit Renderer(Options={});
    ~Renderer();                            // == shutdown(); never waits for the GPU
    Renderer(const Renderer&)=delete;Renderer& operator=(const Renderer&)=delete;
    bool available() const;
    Capabilities capabilities() const;
    // Returns immediately. Cheap checks run on the caller; full list validation,
    // uploads and encoding run on a private serial queue. Accepted -> exactly one
    // completion; any other status -> none. Busy also covers the startup self-test.
    SubmitResult submit(std::shared_ptr<const SceneDrawList>,SceneFrameTicket,CompletionHandler);
    // Advances resourceGeneration: no cached texture is reused afterwards. Frames
    // and in-flight requests keep their own textures.
    void invalidateResources();
    // Rejects new submissions (Closed) and delivers Cancelled for every accepted
    // request not yet delivered. Non-blocking; GPU work finishes in the background.
    void shutdown();
    Counters counters() const;

    // ---- test-only (implemented without GPU waits in the normal path) ----
    bool waitForSelfTestForTesting(double seconds) const;
    bool waitIdleForTesting(double seconds) const; // inFlight==0 and callback queue drained
    void injectFaultForTesting(TestFault);         // applies to the next accepted request
private:
    struct Core;std::shared_ptr<Core> core_;
};

// Test-only synchronous readback (blit + wait). Lives in a separate translation
// unit; the normal submit/Ready path never reads pixels.
bool readFrameForTesting(const Frame&,DecodedImage& out,std::string& error);

} // namespace game_lab::metal_scene
