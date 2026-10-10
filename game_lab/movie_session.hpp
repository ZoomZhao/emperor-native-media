#pragma once
#include "resource_root.hpp"
#include "image_presentation.hpp"
#include "../media_demo/controller.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstring>
#include <dispatch/dispatch.h>

namespace game_lab {

struct MovieDelivery {
    uint64_t revision = 0;
    media_demo::Snapshot snapshot;
    std::vector<uint32_t> pixelsBGRA;
    bool accepted = false;
    bool presented = false;
    PresentationResult presentation = PresentationResult::InvalidPixels;
};

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

    if (snapshot.bgra.size() == 133 * 24) {
        out.pixelsBGRA.resize(32 * 24);
        for (size_t y = 0; y < 24; ++y) {
            const uint8_t* src = snapshot.bgra.data() + y * 133;
            uint8_t* dst = reinterpret_cast<uint8_t*>(out.pixelsBGRA.data()) + y * 32 * 4;
            std::memcpy(dst, src, 32 * 4);
        }
        if (presenter) {
            out.presentation = presentImage(*presenter, 32, 24, out.pixelsBGRA);
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

    bool openMovie(const std::string& relativePath);
    void request(media_demo::Action action, uint64_t seekFrame,
                 std::function<void(const MovieDelivery&)> callback,
                 ImagePresenter* presenter = nullptr);
    MovieDelivery performSync(media_demo::Action action, uint64_t seekFrame = 0,
                              ImagePresenter* presenter = nullptr);
    void close();

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
    dispatch_queue_t worker_ = nullptr;
    std::unique_ptr<media_demo::Controller> controller_;
    media_demo::Snapshot lastSnapshot_;
};

} // namespace game_lab
