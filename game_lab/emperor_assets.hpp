#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../surface_lab/surface.hpp"

namespace game_lab {

// 1. JPEG Image Decoded via ImageIO
struct DecodedImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> pixelsBGRA; // 32-bit BGRA8
    bool valid() const { return width > 0 && height > 0 && !pixelsBGRA.empty(); }
};

// 2. Raw 555 Fullscreen / Texture Image (16-bit RGB555LE)
struct Raw555Image {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint16_t> pixels555;
    std::vector<uint32_t> pixelsBGRA; // converted to BGRA8 for rendering
    bool valid() const { return width > 0 && height > 0 && !pixels555.empty(); }
};

// 3. Impressions SG3 Sprite Record
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

// SG3 Archive containing multiple sprites
struct SG3Archive {
    uint32_t fileSize = 0;
    uint32_t version = 0;
    uint32_t numFiles = 0;
    uint32_t numImages = 0;
    std::vector<SG3Sprite> sprites;
    std::vector<uint8_t> rawData;
};

// 4. RIFF WAV Sound File (Linear PCM)
struct DecodedWav {
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    std::vector<int16_t> samples; // 16-bit PCM samples
    bool valid() const { return channels > 0 && sampleRate > 0 && !samples.empty(); }
};

// 5. City Map (128x128 Tile Grid)
struct CityMap {
    uint32_t width = 128;
    uint32_t height = 128;
    std::vector<uint16_t> tiles; // 16-bit tile IDs
    bool valid() const { return width > 0 && height > 0 && !tiles.empty(); }
};

// Asset Loader Utilities
DecodedImage loadJPEG(const std::string& path);
Raw555Image loadRaw555(const std::string& path, uint32_t width, uint32_t height);
SG3Archive loadSG3(const std::string& path);
DecodedWav loadWAV(const std::string& path);
CityMap loadCityMap(const std::string& path);
std::vector<std::string> loadEmperorText(const std::string& path);

// Sprite Extractor (extracts a single sprite from companion .555 file into BGRA8)
DecodedImage extractSprite(const SG3Sprite& sprite, const std::string& companion555Path);

} // namespace game_lab
