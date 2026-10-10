#pragma once
#include "asset_limits.hpp"
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <vector>

namespace game_lab::detail {
inline uint16_t u16(const uint8_t* p) {
    return static_cast<uint16_t>(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
}
inline uint32_t u32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
           (uint32_t(p[3]) << 24);
}
inline int16_t i16(const uint8_t* p) {
    const int32_t n = u16(p);
    return static_cast<int16_t>(n >= 32768 ? n - 65536 : n);
}

inline bool fileSize(std::ifstream& file, size_t limit, size_t& size) {
    file.seekg(0, std::ios::end);
    const auto end = file.tellg();
    if (end < std::streampos(0)) return false;
    const auto n = static_cast<uint64_t>(static_cast<std::streamoff>(end));
    if (n > limit || n > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    size = static_cast<size_t>(n);
    file.seekg(0, std::ios::beg);
    return bool(file);
}

inline bool readRange(const std::string& path, size_t offset, size_t length,
                      std::vector<uint8_t>& output, size_t fileLimit = kMaxAssetBytes) {
    output.clear();
    std::ifstream file(path, std::ios::binary);
    size_t size = 0;
    if (!file || !fileSize(file, fileLimit, size) || offset > size || length > size - offset)
        return false;
    try {
        std::vector<uint8_t> bytes(length);
        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!file) return false;
        if (length) {
            file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(length));
            if (!file || file.gcount() != static_cast<std::streamsize>(length)) return false;
        }
        output = std::move(bytes);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

inline bool readFile(const std::string& path, size_t limit, std::vector<uint8_t>& output) {
    output.clear();
    std::ifstream file(path, std::ios::binary);
    size_t size = 0;
    if (!file || !fileSize(file, limit, size)) return false;
    try {
        std::vector<uint8_t> bytes(size);
        if (size) {
            file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
            if (!file || file.gcount() != static_cast<std::streamsize>(size)) return false;
        }
        output = std::move(bytes);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}
} // namespace game_lab::detail
