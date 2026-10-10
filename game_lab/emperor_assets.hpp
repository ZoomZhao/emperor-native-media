#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "asset_limits.hpp"

namespace game_lab {
struct DecodedImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> pixelsBGRA;
    bool valid() const {
        size_t count = 0;
        return imagePixelCount(width, height, count) && pixelsBGRA.size() == count;
    }
};

struct Raw555Image {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint16_t> pixels555;
    std::vector<uint32_t> pixelsBGRA;
    bool valid() const {
        size_t count = 0;
        return imagePixelCount(width, height, count) && pixels555.size() == count &&
               pixelsBGRA.size() == count;
    }
};

struct SG3Sprite {
    uint32_t offset = 0;
    uint32_t length = 0;
    uint32_t uncompressedLength = 0;
    int16_t width = 0;
    int16_t height = 0;
    int16_t originX = 0;
    int16_t originY = 0;
    uint16_t type = 0;
    uint16_t flags = 0;
    uint8_t bitmapId = 0;
};

struct SG3Archive {
    uint32_t fileSize = 0;
    uint32_t version = 0;
    uint32_t numFiles = 0;
    uint32_t numImages = 0;
    std::vector<SG3Sprite> sprites;
    std::vector<uint8_t> rawData;
};

struct DecodedWav {
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    std::vector<int16_t> samples;
    bool valid() const {
        return channels >= 1 && channels <= 8 && sampleRate >= 8000 && sampleRate <= 192000 &&
               bitsPerSample == 16 && !samples.empty() && samples.size() % channels == 0;
    }
};

// Existing asset preview only; no city simulation semantics.
struct CityMap {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint16_t> tiles;
    bool valid() const { return width == 128 && height == 128 && tiles.size() == 128u * 128u; }
};

// In-memory decoders allow entirely self-authored boundary fixtures.
DecodedWav decodeWAV(const std::vector<uint8_t>& bytes);
Raw555Image decodeRaw555(const std::vector<uint8_t>& bytes, uint32_t width, uint32_t height);
DecodedImage decodeJPEG(const std::vector<uint8_t>& bytes);
DecodedImage loadJPEG(const std::string& path);
Raw555Image loadRaw555(const std::string& path, uint32_t width, uint32_t height);
SG3Archive loadSG3(const std::string& path);
DecodedWav loadWAV(const std::string& path);
CityMap loadCityMap(const std::string& path);
std::vector<std::string> loadEmperorText(const std::string& path);

// Only exact raw RGB555 square records of type 16/24/32 are accepted.
// Compressed/unknown sprite layouts are unsupported and fail closed.
DecodedImage extractSprite(const SG3Sprite& sprite, const std::string& companion555Path);
} // namespace game_lab
