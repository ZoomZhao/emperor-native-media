#pragma once
#include "resource_root.hpp"
#include "image_presentation.hpp"
#include "../media_demo/controller.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstring>
#include <atomic>
#include <dispatch/dispatch.h>

namespace game_lab {

enum class MovieErrorKind { None, PathSecurity, ResourceUnavailable, MetadataTimeout, DecodeFailure, PresenterFailure, CloseFailure };
const char* movieErrorKindName(MovieErrorKind);
struct MovieSessionStats {
    std::atomic<size_t> workerEnqueues{0}, mainSchedules{0};
    std::atomic<size_t> terminationTimers{0}, maxTerminationTimers{0}, terminationAttempts{0}, terminationReplies{0};
    std::atomic<size_t> active{0}, pending{0}, delivery{0}, maxActive{0}, maxPending{0}, maxDelivery{0};
    std::atomic<bool> resourcesZero{true}, hasController{false}, delayEntered{false}, workerFinished{false};
};
class MovieAudioGain;
struct MovieSessionState;
struct MovieTermination;
enum class TerminationStatus { NotRequested, Pending, Completed, Failed, TimedOut };
struct TerminationResult {
    TerminationStatus status = TerminationStatus::NotRequested;
    bool resourcesZero = false, workerIdle = false, closeObserved = false;
    std::string reason;
    bool permitsExit() const { return status == TerminationStatus::Completed && resourcesZero && workerIdle && closeObserved; }
};
std::string formatTerminationResult(const TerminationResult&);

struct MovieDelivery {
    uint64_t revision = 0;
    media_demo::Snapshot snapshot;
    std::vector<uint32_t> pixelsBGRA;
    MovieErrorKind errorKind = MovieErrorKind::None;
    std::string errorMessage, resolvedPath;
    uint32_t width = 32, height = 24;
    bool poster = false;
    bool accepted = false;
    bool presented = false;
    PresentationResult presentation = PresentationResult::InvalidPixels;
};

std::string formatMovieStatus(const MovieDelivery&);

// Common delivery processor shared between App and headless integration tests.
inline bool processMovieDelivery(uint64_t activeRevision,
                                 uint64_t deliveryRevision,
                                 const media_demo::Snapshot& snapshot,
                                 ImagePresenter* presenter,
                                 MovieDelivery& out) {
    out.revision = deliveryRevision;
    out.snapshot = snapshot;
    out.pixelsBGRA.clear();
    out.presented = false;
    out.presentation = PresentationResult::InvalidPixels;

    if (deliveryRevision != activeRevision) {
        out.accepted = false;
        return false;
    }

    out.accepted = true;
    if (snapshot.clearImage && presenter) {
        presenter->clear();
    }

    size_t pixelCount=0;
    if (imagePixelCount(snapshot.imageWidth,snapshot.imageHeight,pixelCount) &&
        snapshot.imageStride>=size_t(snapshot.imageWidth)*4 &&
        snapshot.imageStride<=8u*1024u*1024u/snapshot.imageHeight &&
        snapshot.bgra.size()==snapshot.imageStride*snapshot.imageHeight) {
        out.width=snapshot.imageWidth;out.height=snapshot.imageHeight;
        out.pixelsBGRA.resize(pixelCount);
        for (size_t y=0;y<out.height;++y) {
            const uint8_t* src=snapshot.bgra.data()+y*snapshot.imageStride;
            uint8_t* dst=reinterpret_cast<uint8_t*>(out.pixelsBGRA.data())+y*size_t(out.width)*4;
            std::memcpy(dst,src,size_t(out.width)*4);
        }
        if (presenter) {
            out.presentation = presentImage(*presenter,out.width,out.height,out.pixelsBGRA);
            out.presented = (out.presentation == PresentationResult::Presented);
        }
    }
    return true;
}

// All public calls and deliveries use the main/owner context. Only Controller work
// runs on the private worker. An async presenter must remain alive until delivery
// or session destruction; the real App owns it for the delegate lifetime.
class MovieSession {
public:
    explicit MovieSession(ResourceRoot root);
    ~MovieSession();

    MovieSession(const MovieSession&) = delete;
    MovieSession& operator=(const MovieSession&) = delete;

    // Owner calls return immediately; superseded callbacks are dropped. One
    // active command, one latest pending command and one latest main delivery.
    void openAndPlay(const std::string&, std::function<void(const MovieDelivery&)>, ImagePresenter* = nullptr);
    void requestPoster(const std::string&, std::function<void(const MovieDelivery&)>, ImagePresenter* = nullptr);
    void closeAsync(std::function<void(bool, const std::string&)> = nullptr);
    // Owner context only. Pending requests keep the original callback/deadline;
    // Completed returns immediately without another callback. Failed/TimedOut
    // may be retried. Only one owner timer; no additional worker is created.
    TerminationStatus requestTermination(uint32_t timeoutMs, std::function<void(const TerminationResult&)>);
    TerminationResult terminationResult() const;
    std::shared_ptr<const MovieSessionStats> stats() const;
    void setTestDelayMs(uint32_t);
    void holdMetadataForTest(bool);
    void failNextCloseForTest();
    // Synchronous compatibility helpers are for headless callers only.
    bool openMovie(const std::string& relativePath);
    void request(media_demo::Action action, uint64_t seekFrame,
                 std::function<void(const MovieDelivery&)> callback,
                 ImagePresenter* presenter = nullptr);
    MovieDelivery performSync(media_demo::Action action, uint64_t seekFrame = 0,
                              ImagePresenter* presenter = nullptr);
    void close();
    void setAudioGain(double gain); // owner context; default is muted

    uint64_t revision() const { return revision_; }
    std::string currentRelativePath() const { return relativePath_; }
    std::string resolvedPath() const { return resolvedPath_; }
    bool hasActiveController() const;
    bool resourcesZero() const;
    media_demo::Snapshot lastSnapshot() const { return lastSnapshot_; }
    size_t staleDeliveries() const { return staleDeliveries_; }

private:
    // Main-context-only token survives queued blocks after this object is gone.
    std::shared_ptr<bool> deliveryAlive_ = std::make_shared<bool>(true);
    ResourceRoot root_;
    std::string relativePath_;
    std::string resolvedPath_;
    uint64_t revision_ = 1;
    size_t staleDeliveries_ = 0;
    std::shared_ptr<stream_lab::OpenControl> loadControl_;
    std::shared_ptr<MovieSessionState> state_;
    media_demo::Snapshot lastSnapshot_;
    std::unique_ptr<MovieTermination> termination_;
    bool terminationLocksCommands() const;
    void closeAsyncImpl(std::function<void(bool, const std::string&)>);
    void checkTermination(uint64_t);
    void finishTermination(TerminationStatus, const std::string&);
    void cancelTerminationTimer();
    void invalidate();
    void loadAsync(const std::string&, bool, std::function<void(const MovieDelivery&)>, ImagePresenter*);
    void accept(MovieDelivery&, ImagePresenter*);
};

} // namespace game_lab
