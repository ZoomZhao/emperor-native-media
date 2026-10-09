#include "oracle.hpp"
#include <algorithm>

namespace scalar_oracle {
using namespace surface_lab;
Color unpack(const std::vector<uint8_t>& bytes, Desc desc, int x, int y) {
    const size_t step = desc.format == Format::RGB565LE ? 2 : 4;
    const size_t p = size_t(y) * desc.stride + size_t(x) * step;
    if (step == 4) return {bytes.at(p), bytes.at(p + 1), bytes.at(p + 2), bytes.at(p + 3)};
    const unsigned word = bytes.at(p) + 256u * bytes.at(p + 1);
    const unsigned blue = word % 32, green = (word / 32) % 64, red = word / 2048;
    // Arithmetic decomposition, independent of the component's bit operations.
    return {blue * 8 + blue / 4, green * 4 + green / 16, red * 8 + red / 4, 255};
}
void pack(std::vector<uint8_t>& bytes, Desc desc, int x, int y, Color c) {
    const size_t step = desc.format == Format::RGB565LE ? 2 : 4;
    const size_t p = size_t(y) * desc.stride + size_t(x) * step;
    if (step == 4) {
        for (size_t n = 0; n < 4; ++n) bytes.at(p + n) = uint8_t(c[n]);
    } else {
        const unsigned word = (c[2] / 8) * 2048 + (c[1] / 4) * 32 + c[0] / 8;
        bytes.at(p) = uint8_t(word % 256); bytes.at(p + 1) = uint8_t(word / 256);
    }
}
std::vector<uint8_t> expected(const std::vector<uint8_t>& input, Desc sd,
    const std::vector<uint8_t>& original, Desc dd, Rect sr, Rect dr) {
    auto out = original;
    // Source-cell footprint oracle. The component instead scans destination
    // pixels and divides their coordinates. Each source cell owns [ceil(kD/S),
    // ceil((k+1)D/S)), preserving the original mapping after destination clipping.
    const auto ceilRatio = [](int64_t a, int64_t b) { return (a + b - 1) / b; };
    for (int64_t ky = 0; ky < sr.height; ++ky) {
        const int64_t sy = int64_t(sr.y) + ky;
        if (sy < 0 || sy >= sd.height) continue;
        const int64_t top = int64_t(dr.y) + ceilRatio(ky * dr.height, sr.height);
        const int64_t bottom = int64_t(dr.y) + ceilRatio((ky + 1) * dr.height, sr.height);
        for (int64_t kx = 0; kx < sr.width; ++kx) {
            const int64_t sx = int64_t(sr.x) + kx;
            if (sx < 0 || sx >= sd.width) continue;
            const int64_t left = int64_t(dr.x) + ceilRatio(kx * dr.width, sr.width);
            const int64_t right = int64_t(dr.x) + ceilRatio((kx + 1) * dr.width, sr.width);
            const auto c = unpack(input, sd, int(sx), int(sy));
            for (int64_t y = std::max<int64_t>(0, top); y < std::min<int64_t>(dd.height, bottom); ++y)
                for (int64_t x = std::max<int64_t>(0, left); x < std::min<int64_t>(dd.width, right); ++x)
                    pack(out, dd, int(x), int(y), c);
        }
    }
    return out;
}
} // namespace scalar_oracle
