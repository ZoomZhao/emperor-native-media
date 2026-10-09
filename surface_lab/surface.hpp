#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace surface_lab {
enum class Format { RGB565LE = 1, BGRA8 = 2 };
enum class Status {
    Ok, InvalidHandle, InvalidDesc, InvalidRect, Capacity,
    Lost, AlreadyReady, InvalidSnapshot, OutOfMemory
};
const char* statusName(Status value);
struct Desc {
    int32_t width, height;
    size_t stride;
    Format format;
};
struct Rect { int32_t x, y, width, height; };
using Handle = uint64_t;
struct Snapshot { Desc desc{}; bool lost = false; std::vector<uint8_t> bytes; };

// CPU-only. No exposed memory addresses, device objects, callbacks, or threads.
class Store {
public:
    explicit Store(size_t slots = 32, size_t byteLimit = 64 * 1024 * 1024);
    Status create(Desc desc, Handle& result);
    Status destroy(Handle handle);
    Status replaceBytes(Handle handle, const std::vector<uint8_t>& bytes);
    Status readBytes(Handle handle, std::vector<uint8_t>& result) const;
    Status snapshot(Handle handle, Snapshot& result) const;
    Status importSnapshot(Handle handle, const Snapshot& value);
    Status lose(Handle handle);
    Status restore(Handle handle);
    Status copy(Handle dst, Handle src, Rect source, Rect destination);
    Status nearest(Handle dst, Handle src, Rect source, Rect destination);
    size_t liveCount() const;
    size_t activeBytes() const;
    size_t savedBytes() const;
private:
    struct Slot {
        uint32_t generation = 1;
        bool live = false, retired = false, lost = false;
        Desc desc{};
        std::vector<uint8_t> pixels, saved;
    };
    std::vector<Slot> slots_;
    size_t byteLimit_;
    Slot* find(Handle handle);
    const Slot* find(Handle handle) const;
    Status transfer(Handle dst, Handle src, Rect source, Rect destination, bool scale);
};
} // namespace surface_lab
