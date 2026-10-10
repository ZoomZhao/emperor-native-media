#include "emperor_assets.hpp"
#include "asset_bytes.hpp"
#include "../surface_lab/surface.hpp"
#include <cstring>
#include <new>

namespace game_lab {
Raw555Image decodeRaw555(const std::vector<uint8_t>& bytes, uint32_t width, uint32_t height) {
    size_t count = 0;
    if (!imagePixelCount(width, height, count) || bytes.size() != count * 2) return {};
    try {
        surface_lab::Store store(2, count * 6);
        surface_lab::Handle source = 0, destination = 0;
        const surface_lab::Desc src{static_cast<int32_t>(width), static_cast<int32_t>(height),
                                    static_cast<size_t>(width) * 2, surface_lab::Format::RGB555LE};
        const surface_lab::Desc dst{static_cast<int32_t>(width), static_cast<int32_t>(height),
                                    static_cast<size_t>(width) * 4, surface_lab::Format::BGRA8};
        if (store.create(src, source) != surface_lab::Status::Ok ||
            store.create(dst, destination) != surface_lab::Status::Ok ||
            store.replaceBytes(source, bytes) != surface_lab::Status::Ok) return {};
        const surface_lab::Rect rect{0, 0, src.width, src.height};
        if (store.copy(destination, source, rect, rect) != surface_lab::Status::Ok) return {};
        std::vector<uint8_t> converted;
        if (store.readBytes(destination, converted) != surface_lab::Status::Ok ||
            converted.size() != count * 4) return {};

        Raw555Image result;
        result.pixels555.resize(count);
        result.pixelsBGRA.resize(count);
        for (size_t i = 0; i < count; ++i) result.pixels555[i] = detail::u16(bytes.data() + i * 2);
        std::memcpy(result.pixelsBGRA.data(), converted.data(), converted.size());
        result.width = width;
        result.height = height;
        return result;
    } catch (const std::bad_alloc&) {
        return {};
    }
}
} // namespace game_lab
