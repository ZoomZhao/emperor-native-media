#import <Foundation/Foundation.h>
#include "movie_session.hpp"

namespace game_lab {

MovieSession::MovieSession(ResourceRoot root)
    : root_(std::move(root)) {
    worker_ = dispatch_queue_create("local.emperor.movie-worker", DISPATCH_QUEUE_SERIAL);
}

MovieSession::~MovieSession() {
    *deliveryAlive_ = false;
    close();
}

bool MovieSession::hasActiveController() const {
    __block bool active = false;
    if (worker_) {
        dispatch_sync(worker_, ^{
            active = (controller_ != nullptr);
        });
    }
    return active;
}

bool MovieSession::resourcesZero() const {
    __block bool zero = true;
    if (worker_) {
        dispatch_sync(worker_, ^{
            if (controller_) {
                zero = controller_->snapshot().resourcesZero();
            }
        });
    }
    return zero;
}

bool MovieSession::openMovie(const std::string& relativePath) {
    if (loadControl_) {
        loadControl_->cancelled.store(true, std::memory_order_release);
    }
    ++revision_;
    const auto resolved = root_.resolve(relativePath);
    if (resolved.empty()) {
        if (worker_) {
            dispatch_sync(worker_, ^{
                if (controller_) {
                    try { controller_->close(); } catch (...) {}
                    controller_.reset();
                }
            });
        }
        relativePath_.clear();
        resolvedPath_.clear();
        lastSnapshot_ = media_demo::Snapshot{};
        lastSnapshot_.state = media_demo::State::Failed;
        lastSnapshot_.clearImage = true;
        lastSnapshot_.message = "相对路径未通过 ResourceRoot 校验或不存在";
        return false;
    }

    __block bool created = false;
    if (worker_) {
        dispatch_sync(worker_, ^{
            if (controller_) {
                try { controller_->close(); } catch (...) {}
                controller_.reset();
            }
            try {
                controller_ = std::make_unique<media_demo::Controller>(resolved);
                created = true;
            } catch (const std::exception& e) {
                lastSnapshot_ = media_demo::Snapshot{};
                lastSnapshot_.state = media_demo::State::Failed;
                lastSnapshot_.clearImage = true;
                lastSnapshot_.message = std::string("无法打开媒体: ") + e.what();
            }
        });
    }

    if (!created) {
        relativePath_.clear();
        resolvedPath_.clear();
        return false;
    }

    relativePath_ = relativePath;
    resolvedPath_ = resolved;
    lastSnapshot_ = media_demo::Snapshot{};
    lastSnapshot_.state = media_demo::State::Ready;
    return true;
}

void MovieSession::request(media_demo::Action action, uint64_t seekFrame,
                           std::function<void(const MovieDelivery&)> callback,
                           ImagePresenter* presenter) {
    if (action != media_demo::Action::Tick) {
        if (loadControl_) {
            loadControl_->cancelled.store(true, std::memory_order_release);
        }
        ++revision_;
    }
    const auto rev = revision_;
    const auto control = std::make_shared<stream_lab::OpenControl>();
    loadControl_ = control;

    const auto lifetime = deliveryAlive_;
    dispatch_async(worker_, ^{
        media_demo::Snapshot snap;
        try {
            if (controller_) {
                snap = controller_->perform(action, seekFrame, control);
            } else {
                snap.state = media_demo::State::Failed;
                snap.clearImage = true;
                snap.message = "未打开有效媒体文件";
            }
        } catch (const std::exception& error) {
            snap.state = media_demo::State::Failed;
            snap.clearImage = true;
            snap.message = std::string("错误：") + error.what();
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            // Worker completion does not fence its already queued main delivery.
            // Test the retained token before accessing this, presenter or callback.
            if (!*lifetime) return;
            MovieDelivery delivery;
            const bool accepted = processMovieDelivery(this->revision_, rev, snap, presenter, delivery);
            if (!accepted) {
                ++this->staleDeliveries_;
            } else {
                this->lastSnapshot_ = delivery.snapshot;
            }
            if (callback) {
                callback(delivery);
            }
        });
    });
}

MovieDelivery MovieSession::performSync(media_demo::Action action, uint64_t seekFrame,
                                       ImagePresenter* presenter) {
    if (action != media_demo::Action::Tick) {
        if (loadControl_) {
            loadControl_->cancelled.store(true, std::memory_order_release);
        }
        ++revision_;
    }
    const auto rev = revision_;
    const auto control = std::make_shared<stream_lab::OpenControl>();
    loadControl_ = control;

    __block media_demo::Snapshot snap;
    dispatch_sync(worker_, ^{
        try {
            if (controller_) {
                snap = controller_->perform(action, seekFrame, control);
            } else {
                snap.state = media_demo::State::Failed;
                snap.clearImage = true;
                snap.message = "未打开有效媒体文件";
            }
        } catch (const std::exception& error) {
            snap.state = media_demo::State::Failed;
            snap.clearImage = true;
            snap.message = std::string("错误：") + error.what();
        }
    });

    MovieDelivery delivery;
    const bool accepted = processMovieDelivery(this->revision_, rev, snap, presenter, delivery);
    if (!accepted) {
        ++this->staleDeliveries_;
    } else {
        this->lastSnapshot_ = delivery.snapshot;
    }
    return delivery;
}

void MovieSession::close() {
    if (loadControl_) {
        loadControl_->cancelled.store(true, std::memory_order_release);
    }
    ++revision_;
    if (worker_) {
        dispatch_sync(worker_, ^{
            if (controller_) {
                try {
                    controller_->close();
                } catch (...) {}
                controller_.reset();
            }
        });
    }
    relativePath_.clear();
    resolvedPath_.clear();
    lastSnapshot_ = media_demo::Snapshot{};
    lastSnapshot_.clearImage = true;
}

} // namespace game_lab
