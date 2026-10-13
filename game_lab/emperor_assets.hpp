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
    int32_t mirrorOffset = 0;
    uint32_t alphaOffset = 0, alphaLength = 0;
    uint8_t external = 0, tileSize = 0;
};

struct SG3Bitmap { std::string filename; };

struct SG3Archive {
    uint32_t fileSize = 0;
    uint32_t version = 0;
    uint32_t numFiles = 0;
    uint32_t numImages = 0;
    uint32_t capacity = 0;
    std::vector<SG3Bitmap> bitmaps;
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

enum class TextLoadState { Loaded, ResourceFailure, DecodeFailure, NoText };
struct TextLoadResult {
    TextLoadState state = TextLoadState::ResourceFailure;
    std::vector<std::string> lines;
};
inline std::string formatTextLoadResult(const TextLoadResult& result) {
    switch (result.state) {
        case TextLoadState::Loaded:
            return "文本库 (共 " + std::to_string(result.lines.size()) + " 条非空文本行 · GB18030 转码)";
        case TextLoadState::ResourceFailure:
            return "文本载入失败：资源不可读取或超出大小限制";
        case TextLoadState::DecodeFailure:
            return "文本载入失败：GB18030 解码或转码失败";
        case TextLoadState::NoText:
            return "文本资源为空或没有非空文本行";
    }
    return "文本载入失败";
}
TextLoadResult loadEmperorTextResult(const std::string& path);

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

enum class SG3DecodeStatus { Decoded, Unsupported, InvalidRecord, MissingDependency, InvalidPayload, EmptyRecord };
struct SG3DecodeResult {
    SG3DecodeStatus status = SG3DecodeStatus::InvalidRecord;
    DecodedImage image;
    bool mirrored = false;
    bool alphaApplied = false;
};
// Logical index 0 corresponds to SG image ID1; dummy ID0 is excluded.
// Companion resolution is confined to the SG directory (including 555/).
SG3DecodeResult decodeSG3Sprite(const SG3Archive&, size_t index, const std::string& companion555Path);
DecodedImage extractSprite(const SG3Sprite& sprite, const std::string& companion555Path);
} // namespace game_lab
