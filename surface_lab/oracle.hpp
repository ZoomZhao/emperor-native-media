#pragma once
#include "surface.hpp"
#include <array>

namespace scalar_oracle {
using Color = std::array<unsigned, 4>; // B,G,R,A
Color unpack(const std::vector<uint8_t>& bytes, surface_lab::Desc desc, int x, int y);
void pack(std::vector<uint8_t>& bytes, surface_lab::Desc desc, int x, int y, Color value);
std::vector<uint8_t> expected(const std::vector<uint8_t>& immutableSource,
    surface_lab::Desc sourceDesc, const std::vector<uint8_t>& originalDestination,
    surface_lab::Desc destinationDesc, surface_lab::Rect sourceRect, surface_lab::Rect destinationRect);
} // namespace scalar_oracle
