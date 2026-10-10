#pragma once
#include "heap_native.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <new>
#include <utility>
#include <vector>

namespace runtime_core {
// Logical offsets in a private vector. No executable mapping, files or host-pointer rebasing.
class Memory {
public:
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;
    Memory(Memory&&) = delete;
    Memory& operator=(Memory&&) = delete;
    Memory(uint32_t base, size_t capacity) : base_(base) {
        if (!capacity || capacity > 32u * 1024u * 1024u ||
            uint64_t(base) + capacity > uint64_t(UINT32_MAX))
            throw std::invalid_argument("invalid private memory extent");
        bytes_.resize(capacity, 0);
    }
    bool map(uint32_t begin, uint32_t end) {
        if (uint64_t(begin) != uint64_t(base_) + mapped_ || end < begin ||
            uint64_t(end) > uint64_t(base_) + bytes_.size()) return false;
        mapped_ = static_cast<size_t>(uint64_t(end) - base_);
        return true;
    }
    bool check(uint32_t address, uint64_t length, unsigned permissions) const {
        if (!permissions || (permissions & ~3u) || address < base_) return false;
        const uint64_t offset = uint64_t(address) - base_;
        return offset <= mapped_ && length <= mapped_ - offset;
    }
    bool fill(uint32_t address, uint32_t length, uint8_t value) {
        if (!check(address, length, 2)) return false;
        if (length) { std::memset(bytes_.data() + (address - base_), value, length); ++writes_; }
        return true;
    }
    bool copy(uint32_t destination, uint32_t source, uint32_t length) {
        if (!check(destination, length, 2) || !check(source, length, 1)) return false;
        if (length) {
            std::memmove(bytes_.data() + (destination - base_), bytes_.data() + (source - base_), length);
            ++writes_;
        }
        return true;
    }
    bool read(uint32_t address, uint32_t length, std::vector<uint8_t>& output) const {
        if (!check(address, length, 1)) return false;
        try {
            const auto first = bytes_.begin() + (address - base_);
            std::vector<uint8_t> snapshot(first, first + length);
            output = std::move(snapshot);
            return true;
        } catch (const std::bad_alloc&) {
            return false;
        }
    }
    uint64_t writes() const { return writes_; }
    size_t mappedBytes() const { return mapped_; }
    S77Memory heapAdapter() {
        return {this,
            [](void* p, uint32_t a, uint32_t n, unsigned permission) -> int {
                return static_cast<Memory*>(p)->check(a, n, permission);
            },
            [](void* p, uint32_t begin, uint32_t end) -> int {
                return static_cast<Memory*>(p)->map(begin, end);
            },
            [](void* p, uint32_t a, uint32_t n, unsigned char value) -> int {
                return static_cast<Memory*>(p)->fill(a, n, value);
            },
            [](void* p, uint32_t dst, uint32_t src, uint32_t n) -> int {
                return static_cast<Memory*>(p)->copy(dst, src, n);
            }};
    }
private:
    uint32_t base_;
    size_t mapped_ = 0;
    uint64_t writes_ = 0;
    std::vector<uint8_t> bytes_;
};
} // namespace runtime_core
