#pragma once
#include <cstddef>
#include <cstdint>

namespace game_lab {
constexpr size_t kMaxAssetBytes = 128u * 1024u * 1024u;
constexpr size_t kMaxWavBytes = 64u * 1024u * 1024u;
constexpr size_t kMaxTextBytes = 16u * 1024u * 1024u;
constexpr uint32_t kMaxImageDimension = 4096;
constexpr size_t kMaxImagePixels = 4096u * 4096u;

inline bool imagePixelCount(uint32_t width, uint32_t height, size_t& count) {
    count = 0;
    if (!width || !height || width > kMaxImageDimension || height > kMaxImageDimension)
        return false;
    if (static_cast<size_t>(width) > kMaxImagePixels / height) return false;
    count = static_cast<size_t>(width) * height;
    return true;
}
} // namespace game_lab
