#include "surface.hpp"
#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace surface_lab {
namespace {
size_t pixelSize(Format format) {
    switch (format) {
        case Format::RGB565LE: return 2;
        case Format::BGRA8: return 4;
    }
    return 0;
}
bool validDesc(Desc desc, size_t& bytes) {
    const size_t bpp = pixelSize(desc.format);
    if (!bpp || desc.width <= 0 || desc.height <= 0 ||
        desc.width > 4096 || desc.height > 4096) return false;
    if (desc.stride < size_t(desc.width) * bpp ||
        desc.stride > std::numeric_limits<size_t>::max() / size_t(desc.height)) return false;
    bytes = desc.stride * size_t(desc.height);
    return true;
}
bool sameDesc(Desc a, Desc b) {
    return a.width == b.width && a.height == b.height &&
        a.stride == b.stride && a.format == b.format;
}
struct Color { uint8_t b, g, r, a; };
Color decode(const uint8_t* p, Format format) {
    if (format == Format::BGRA8) return {p[0], p[1], p[2], p[3]};
    const uint16_t word = uint16_t(uint16_t(p[0]) | uint16_t(uint16_t(p[1]) << 8));
    const uint8_t b = uint8_t(word & 31), g = uint8_t((word >> 5) & 63);
    const uint8_t r = uint8_t((word >> 11) & 31);
    return {uint8_t((b << 3) | (b >> 2)), uint8_t((g << 2) | (g >> 4)),
            uint8_t((r << 3) | (r >> 2)), 255};
}
void encode(uint8_t* p, Format format, Color c) {
    if (format == Format::BGRA8) {
        p[0] = c.b; p[1] = c.g; p[2] = c.r; p[3] = c.a;
        return;
    }
    const uint16_t word = uint16_t((uint16_t(c.r >> 3) << 11) |
        (uint16_t(c.g >> 2) << 5) | uint16_t(c.b >> 3));
    p[0] = uint8_t(word & 255); p[1] = uint8_t(word >> 8);
}
void freeVector(std::vector<uint8_t>& bytes) { std::vector<uint8_t>().swap(bytes); }
}

const char* statusName(Status status) {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidHandle: return "INVALID_HANDLE";
        case Status::InvalidDesc: return "INVALID_DESC";
        case Status::InvalidRect: return "INVALID_RECT";
        case Status::Capacity: return "CAPACITY";
        case Status::Lost: return "LOST";
        case Status::AlreadyReady: return "ALREADY_READY";
        case Status::InvalidSnapshot: return "INVALID_SNAPSHOT";
        case Status::OutOfMemory: return "OUT_OF_MEMORY";
    }
    return "UNKNOWN";
}
Store::Store(size_t slots, size_t byteLimit) : slots_(slots), byteLimit_(byteLimit) {}
Store::Slot* Store::find(Handle handle) {
    const uint32_t encodedSlot = uint32_t(handle & 0xffffffffu);
    const uint32_t generation = uint32_t(handle >> 32);
    if (!encodedSlot || encodedSlot > slots_.size()) return nullptr;
    Slot& slot = slots_[encodedSlot - 1];
    return slot.live && slot.generation == generation ? &slot : nullptr;
}
const Store::Slot* Store::find(Handle handle) const {
    const uint32_t encodedSlot = uint32_t(handle & 0xffffffffu);
    const uint32_t generation = uint32_t(handle >> 32);
    if (!encodedSlot || encodedSlot > slots_.size()) return nullptr;
    const Slot& slot = slots_[encodedSlot - 1];
    return slot.live && slot.generation == generation ? &slot : nullptr;
}
Status Store::create(Desc desc, Handle& result) {
    result = 0;
    size_t count = 0;
    if (!validDesc(desc, count)) return Status::InvalidDesc;
    const size_t used = activeBytes() + savedBytes();
    if (used > byteLimit_ || count > byteLimit_ - used) return Status::Capacity;
    for (size_t i = 0; i < slots_.size(); ++i) {
        Slot& slot = slots_[i];
        if (slot.live || slot.retired) continue;
        try {
            std::vector<uint8_t> staging(count, 0);
            slot.pixels.swap(staging);
        } catch (const std::bad_alloc&) { return Status::OutOfMemory; }
        slot.desc = desc; slot.live = true; slot.lost = false;
        result = (uint64_t(slot.generation) << 32) | uint64_t(i + 1);
        return Status::Ok;
    }
    return Status::Capacity;
}
Status Store::destroy(Handle handle) {
    Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    freeVector(slot->pixels); freeVector(slot->saved);
    slot->live = false; slot->lost = false;
    if (slot->generation == std::numeric_limits<uint32_t>::max()) slot->retired = true;
    else ++slot->generation;
    return Status::Ok;
}
Status Store::replaceBytes(Handle handle, const std::vector<uint8_t>& bytes) {
    Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    if (slot->lost) return Status::Lost;
    if (bytes.size() != slot->pixels.size()) return Status::InvalidSnapshot;
    // Fixed-size overwrite cannot allocate and cannot partially reject.
    std::copy(bytes.begin(), bytes.end(), slot->pixels.begin());
    return Status::Ok;
}
Status Store::readBytes(Handle handle, std::vector<uint8_t>& result) const {
    const Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    if (slot->lost) return Status::Lost;
    try { auto staging = slot->pixels; result.swap(staging); }
    catch (const std::bad_alloc&) { return Status::OutOfMemory; }
    return Status::Ok;
}
Status Store::snapshot(Handle handle, Snapshot& result) const {
    const Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    try {
        Snapshot staging{slot->desc, slot->lost, slot->lost ? slot->saved : slot->pixels};
        result = std::move(staging);
    } catch (const std::bad_alloc&) { return Status::OutOfMemory; }
    return Status::Ok;
}
Status Store::importSnapshot(Handle handle, const Snapshot& value) {
    Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    size_t count = 0;
    if (!validDesc(value.desc, count) || !sameDesc(value.desc, slot->desc) ||
        count != value.bytes.size()) return Status::InvalidSnapshot;
    try {
        auto staging = value.bytes;
        freeVector(slot->pixels); freeVector(slot->saved);
        if (value.lost) slot->saved.swap(staging); else slot->pixels.swap(staging);
        slot->lost = value.lost;
    } catch (const std::bad_alloc&) { return Status::OutOfMemory; }
    return Status::Ok;
}
Status Store::lose(Handle handle) {
    Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    if (slot->lost) return Status::Lost;
    slot->saved.swap(slot->pixels); slot->lost = true;
    return Status::Ok;
}
Status Store::restore(Handle handle) {
    Slot* slot = find(handle);
    if (!slot) return Status::InvalidHandle;
    if (!slot->lost) return Status::AlreadyReady;
    // Reallocate before committing; allocation failure retains the lost snapshot.
    try { auto staging = slot->saved; slot->pixels.swap(staging); }
    catch (const std::bad_alloc&) { return Status::OutOfMemory; }
    freeVector(slot->saved); slot->lost = false;
    return Status::Ok;
}
Status Store::copy(Handle dst, Handle src, Rect source, Rect destination) {
    return transfer(dst, src, source, destination, false);
}
Status Store::nearest(Handle dst, Handle src, Rect source, Rect destination) {
    return transfer(dst, src, source, destination, true);
}
Status Store::transfer(Handle dst, Handle src, Rect source, Rect destination, bool scale) {
    Slot* out = find(dst); const Slot* in = find(src);
    if (!out || !in) return Status::InvalidHandle;
    if (out->lost || in->lost) return Status::Lost;
    if (source.width <= 0 || source.height <= 0 || destination.width <= 0 ||
        destination.height <= 0 || (!scale && (source.width != destination.width ||
        source.height != destination.height))) return Status::InvalidRect;
    const int64_t x0 = std::max<int64_t>(0, destination.x);
    const int64_t y0 = std::max<int64_t>(0, destination.y);
    const int64_t x1 = std::min<int64_t>(out->desc.width, int64_t(destination.x) + destination.width);
    const int64_t y1 = std::min<int64_t>(out->desc.height, int64_t(destination.y) + destination.height);
    if (x0 >= x1 || y0 >= y1) return Status::Ok;
    std::vector<uint8_t> before;
    try { before = in->pixels; }
    catch (const std::bad_alloc&) { return Status::OutOfMemory; }
    const size_t inputBpp = pixelSize(in->desc.format), outputBpp = pixelSize(out->desc.format);
    for (int64_t y = y0; y < y1; ++y) {
        const int64_t sy = int64_t(source.y) + ((y - destination.y) * source.height) / destination.height;
        if (sy < 0 || sy >= in->desc.height) continue;
        for (int64_t x = x0; x < x1; ++x) {
            const int64_t sx = int64_t(source.x) + ((x - destination.x) * source.width) / destination.width;
            if (sx < 0 || sx >= in->desc.width) continue;
            const size_t inputOffset = size_t(sy) * in->desc.stride + size_t(sx) * inputBpp;
            const size_t outputOffset = size_t(y) * out->desc.stride + size_t(x) * outputBpp;
            encode(out->pixels.data() + outputOffset, out->desc.format,
                   decode(before.data() + inputOffset, in->desc.format));
        }
    }
    return Status::Ok;
}
size_t Store::liveCount() const {
    size_t total = 0; for (const Slot& slot : slots_) if (slot.live) ++total; return total;
}
size_t Store::activeBytes() const {
    size_t total = 0; for (const Slot& slot : slots_) total += slot.pixels.size(); return total;
}
size_t Store::savedBytes() const {
    size_t total = 0; for (const Slot& slot : slots_) total += slot.saved.size(); return total;
}
} // namespace surface_lab
