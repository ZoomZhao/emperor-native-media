#pragma once
#include "asset_limits.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <vector>

namespace game_lab {
// Tight rows of little-endian BGRA8 with premultiplied alpha.
// Returns a caller-owned CGImage (+1), or nullptr on invalid input/factory failure.
inline CGImageRef createCGImageFromPremultipliedBGRA(
    uint32_t width, uint32_t height, const std::vector<uint32_t>& pixels) {
    size_t count = 0;
    if (!game_lab::imagePixelCount(width, height, count) || pixels.size() != count) return nullptr;
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    if (!space) return nullptr;
    CGContextRef context = CGBitmapContextCreate(const_cast<uint32_t*>(pixels.data()), width, height,
        8, static_cast<size_t>(width) * 4, space,
        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(space);
    if (!context) return nullptr;
    CGImageRef image = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    if (!image) return nullptr;
    return image;
}
} // namespace game_lab
