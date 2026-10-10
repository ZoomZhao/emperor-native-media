#pragma once
#include "asset_limits.hpp"
#include <vector>

namespace game_lab {
enum class PresentationResult { InvalidPixels, ConversionFailure, Presented };
class ImagePresenter {
public:
    virtual ~ImagePresenter() = default;
    virtual void clear() = 0;
    virtual bool present(uint32_t width, uint32_t height,
                         const std::vector<uint32_t>& pixelsBGRA) = 0;
};
// One contract for the App's image views and device-free failure injection.
inline PresentationResult presentImage(ImagePresenter& presenter, uint32_t width, uint32_t height,
                                       const std::vector<uint32_t>& pixelsBGRA) {
    presenter.clear();
    size_t count = 0;
    if (!imagePixelCount(width, height, count) || pixelsBGRA.size() != count)
        return PresentationResult::InvalidPixels;
    if (!presenter.present(width, height, pixelsBGRA)) {
        presenter.clear();
        return PresentationResult::ConversionFailure;
    }
    return PresentationResult::Presented;
}
} // namespace game_lab
