#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#include "emperor_assets.hpp"
#include "asset_bytes.hpp"
#include <new>
#include <zlib.h>

namespace game_lab {
namespace {
template<class T> struct CFScoped {
    T value;
    explicit CFScoped(T v) : value(v) {}
    ~CFScoped() { if (value) CFRelease(value); }
    CFScoped(const CFScoped&) = delete;
    CFScoped& operator=(const CFScoped&) = delete;
};
bool imageDimension(CFDictionaryRef properties, CFStringRef key, uint32_t& output) {
    const auto value = CFDictionaryGetValue(properties, key);
    int64_t dimension = 0;
    if (!value || CFGetTypeID(value) != CFNumberGetTypeID() ||
        !CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberSInt64Type, &dimension) ||
        dimension <= 0 || dimension > kMaxImageDimension) return false;
    output = static_cast<uint32_t>(dimension);
    return true;
}
}

DecodedImage decodeJPEG(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 4 || bytes.size() > kMaxAssetBytes || bytes[0] != 0xff || bytes[1] != 0xd8 ||
        bytes[bytes.size() - 2] != 0xff || bytes.back() != 0xd9)
        return {};
    try {
        CFScoped<CFDataRef> data(CFDataCreate(kCFAllocatorDefault, bytes.data(), bytes.size()));
        if (!data.value) return {};
        CFScoped<CGImageSourceRef> source(CGImageSourceCreateWithData(data.value, nullptr));
        if (!source.value) return {};
        CFScoped<CFDictionaryRef> properties(CGImageSourceCopyPropertiesAtIndex(source.value, 0, nullptr));
        uint32_t width = 0, height = 0;
        size_t count = 0;
        if (!properties.value || !imageDimension(properties.value, kCGImagePropertyPixelWidth, width) ||
            !imageDimension(properties.value, kCGImagePropertyPixelHeight, height) ||
            !imagePixelCount(width, height, count)) return {};
        CFScoped<CGImageRef> image(CGImageSourceCreateImageAtIndex(source.value, 0, nullptr));
        if (!image.value || CGImageGetWidth(image.value) != width || CGImageGetHeight(image.value) != height)
            return {};
        if (CGImageSourceGetStatus(source.value) != kCGImageStatusComplete ||
            CGImageSourceGetStatusAtIndex(source.value, 0) != kCGImageStatusComplete) return {};
        CFScoped<CGColorSpaceRef> colorSpace(CGColorSpaceCreateDeviceRGB());
        if (!colorSpace.value) return {};
        DecodedImage result;
        result.pixelsBGRA.resize(count);
        CFScoped<CGContextRef> context(CGBitmapContextCreate(result.pixelsBGRA.data(), width, height,
            8, static_cast<size_t>(width) * 4, colorSpace.value,
            kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst));
        if (!context.value) return {};
        CGContextDrawImage(context.value, CGRectMake(0, 0, width, height), image.value);
        result.width = width;
        result.height = height;
        return result;
    } catch (const std::bad_alloc&) {
        return {};
    }
}

DecodedImage loadJPEG(const std::string& path) {
    std::vector<uint8_t> bytes;
    if (!detail::readFile(path, kMaxAssetBytes, bytes)) return {};
    return decodeJPEG(bytes);
}

Raw555Image loadRaw555(const std::string& path, uint32_t width, uint32_t height) {
    size_t count = 0;
    if (!imagePixelCount(width, height, count)) return {};
    std::vector<uint8_t> bytes;
    if (!detail::readFile(path, count * 2, bytes)) return {};
    return decodeRaw555(bytes, width, height);
}

SG3Archive loadSG3(const std::string& path) {
    constexpr size_t tableOffset = 40680, recordSize = 64;
    std::vector<uint8_t> bytes;
    if (!detail::readFile(path, kMaxAssetBytes, bytes) || bytes.size() < tableOffset) return {};
    const uint32_t version = detail::u32(bytes.data() + 4);
    const uint32_t count = detail::u32(bytes.data() + 16);
    if (version != 213 || !count || count > (bytes.size() - tableOffset) / recordSize) return {};
    try {
        SG3Archive archive;
        archive.sprites.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t* rec = bytes.data() + tableOffset + static_cast<size_t>(i) * recordSize;
            SG3Sprite s;
            s.offset = detail::u32(rec);
            s.length = detail::u32(rec + 4);
            s.uncompressedLength = detail::u32(rec + 8);
            s.width = detail::i16(rec + 12); s.height = detail::i16(rec + 14);
            s.originX = detail::i16(rec + 16); s.originY = detail::i16(rec + 18);
            s.type = detail::u16(rec + 20); s.flags = detail::u16(rec + 22);
            s.bitmapId = rec[24];
            archive.sprites.push_back(s);
        }
        archive.fileSize = static_cast<uint32_t>(bytes.size());
        archive.version = version;
        archive.numFiles = detail::u32(bytes.data() + 8);
        archive.numImages = count;
        return archive;
    } catch (const std::bad_alloc&) { return {}; }
}

DecodedWav loadWAV(const std::string& path) {
    std::vector<uint8_t> bytes;
    if (!detail::readFile(path, kMaxWavBytes, bytes)) return {};
    return decodeWAV(bytes);
}

CityMap loadCityMap(const std::string& path) {
    std::vector<uint8_t> bytes;
    if (!detail::readFile(path, kMaxAssetBytes, bytes) || bytes.size() < 32) return {};
    try {
        // Bounds repair of the existing preview payload; no simulation logic.
        constexpr size_t tileCount = 128u * 128u, byteCount = tileCount * 2;
        std::vector<uint8_t> payload(byteCount);
        uLongf length = byteCount;
        if (uncompress(payload.data(), &length, bytes.data() + 16, bytes.size() - 16) != Z_OK ||
            length != byteCount) return {};
        CityMap map;
        map.tiles.resize(tileCount);
        for (size_t i = 0; i < tileCount; ++i) map.tiles[i] = detail::u16(payload.data() + i * 2);
        map.width = 128; map.height = 128;
        return map;
    } catch (const std::bad_alloc&) { return {}; }
}

std::vector<std::string> loadEmperorText(const std::string& path) {
    std::vector<uint8_t> bytes;
    if (!detail::readFile(path, kMaxTextBytes, bytes) || bytes.empty()) return {};
    try {
        std::vector<std::string> lines;
        @autoreleasepool {
            CFStringRef text = CFStringCreateWithBytes(kCFAllocatorDefault, bytes.data(), bytes.size(),
                                                      kCFStringEncodingGB_18030_2000, false);
            if (!text) return {};
            NSString* string = (__bridge_transfer NSString*)text;
            for (NSString* line in [string componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]]) {
                NSString* trimmed = [line stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                if (trimmed.length && trimmed.UTF8String) lines.emplace_back(trimmed.UTF8String);
            }
        }
        return lines;
    } catch (const std::bad_alloc&) { return {}; }
}

DecodedImage extractSprite(const SG3Sprite& sprite, const std::string& companion555Path) {
    if (sprite.type != 16 && sprite.type != 24 && sprite.type != 32) return {};
    const uint32_t width = sprite.type, height = sprite.type;
    const size_t expected = static_cast<size_t>(width) * height * 2;
    if (sprite.length != expected || sprite.flags != 0 ||
        (sprite.uncompressedLength != 0 && sprite.uncompressedLength != expected)) return {};
    std::vector<uint8_t> bytes;
    if (!detail::readRange(companion555Path, sprite.offset, expected, bytes)) return {};
    auto raw = decodeRaw555(bytes, width, height);
    if (!raw.valid()) return {};
    return {raw.width, raw.height, std::move(raw.pixelsBGRA)};
}
} // namespace game_lab
