#pragma once
#include "../surface_lab/surface.hpp"
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace metal_lab {
enum class Status { Ok, Unavailable, SurfaceRejected, UnsupportedRect, InvalidRect,
    GpuError, Timeout, ChangedDestination, AlreadyApplied };
struct Completion;
struct Job;
struct Ticket { std::shared_ptr<Job> job; std::shared_ptr<Completion> completion; };
class Backend {
public:
    Backend(); ~Backend();
    Backend(const Backend&) = delete; Backend& operator=(const Backend&) = delete;
    bool available() const; std::string reason() const; std::string deviceName() const;
    Status submit(surface_lab::Store& store, surface_lab::Handle dst, surface_lab::Handle src,
        surface_lab::Rect source, surface_lab::Rect destination, bool nearest, Ticket& ticket,
        std::function<void(bool)> completed = {});
    static Status finish(surface_lab::Store& store, Ticket& ticket, std::vector<uint8_t>* readback = nullptr);
    static Status wait(const Ticket& ticket);
    static unsigned callbackCount(const Ticket& ticket);
    static size_t retainedBufferBytes(const Ticket& ticket);
    static size_t retainedBufferCount(const Ticket& ticket);
private:
    struct Impl; std::unique_ptr<Impl> impl_;
};
} // namespace metal_lab
