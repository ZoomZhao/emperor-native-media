#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#include "emperor_assets.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <zlib.h>

namespace game_lab {

DecodedImage loadJPEG(const std::string& path) {
    DecodedImage result;
    @autoreleasepool {
        NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
        NSURL* url = [NSURL fileURLWithPath:nsPath];
        CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)url, nullptr);
        if (!source) return result;
        CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
        CFRelease(source);
        if (!image) return result;

        const size_t width = CGImageGetWidth(image);
        const size_t height = CGImageGetHeight(image);
        result.width = static_cast<uint32_t>(width);
        result.height = static_cast<uint32_t>(height);
        result.pixelsBGRA.resize(width * height);

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            result.pixelsBGRA.data(),
            width, height, 8, width * 4,
            colorSpace,
            kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst
        );
        CGColorSpaceRelease(colorSpace);

        if (context) {
            CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
            CGContextRelease(context);
        }
        CGImageRelease(image);
    }
    return result;
}

Raw555Image loadRaw555(const std::string& path, uint32_t width, uint32_t height) {
    Raw555Image result;
    std::ifstream file(path, std::ios::binary);
    if (!file) return result;

    const size_t expectedBytes = width * height * 2;
    result.pixels555.resize(width * height);
    file.read(reinterpret_cast<char*>(result.pixels555.data()), expectedBytes);
    if (file.gcount() < static_cast<std::streamsize>(expectedBytes)) {
        return result;
    }

    result.width = width;
    result.height = height;
    result.pixelsBGRA.resize(width * height);

    surface_lab::Store store(4, expectedBytes * 4);
    surface_lab::Handle srcHandle = 0, dstHandle = 0;
    surface_lab::Desc srcDesc{
        static_cast<int32_t>(width),
        static_cast<int32_t>(height),
        width * 2,
        surface_lab::Format::RGB555LE
    };
    surface_lab::Desc dstDesc{
        static_cast<int32_t>(width),
        static_cast<int32_t>(height),
        width * 4,
        surface_lab::Format::BGRA8
    };

    if (store.create(srcDesc, srcHandle) != surface_lab::Status::Ok ||
        store.create(dstDesc, dstHandle) != surface_lab::Status::Ok) {
        return result;
    }

    const uint8_t* raw555Bytes = reinterpret_cast<const uint8_t*>(result.pixels555.data());
    std::vector<uint8_t> srcBytes(raw555Bytes, raw555Bytes + expectedBytes);
    store.replaceBytes(srcHandle, srcBytes);

    surface_lab::Rect rect{0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)};
    if (store.copy(dstHandle, srcHandle, rect, rect) != surface_lab::Status::Ok) {
        return result;
    }

    std::vector<uint8_t> dstBytes;
    if (store.readBytes(dstHandle, dstBytes) == surface_lab::Status::Ok &&
        dstBytes.size() == width * height * 4) {
        std::memcpy(result.pixelsBGRA.data(), dstBytes.data(), dstBytes.size());
    }

    return result;
}

SG3Archive loadSG3(const std::string& path) {
    SG3Archive archive;
    std::ifstream file(path, std::ios::binary);
    if (!file) return archive;

    file.seekg(0, std::ios::end);
    const size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 40680) return archive;

    archive.fileSize = static_cast<uint32_t>(fileSize);
    uint32_t header[5];
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    archive.version = header[1];
    archive.numFiles = header[2];
    archive.numImages = header[4];

    if (archive.version != 213 || archive.numImages == 0) return archive;

    archive.sprites.resize(archive.numImages);
    file.seekg(40680, std::ios::beg);

    for (uint32_t i = 0; i < archive.numImages; ++i) {
        uint8_t rec[64];
        file.read(reinterpret_cast<char*>(rec), 64);
        if (!file) break;

        SG3Sprite s;
        std::memcpy(&s.offset, rec + 0, 4);
        std::memcpy(&s.length, rec + 4, 4);
        std::memcpy(&s.uncompressedLength, rec + 8, 4);
        std::memcpy(&s.width, rec + 12, 2);
        std::memcpy(&s.height, rec + 14, 2);
        std::memcpy(&s.originX, rec + 16, 2);
        std::memcpy(&s.originY, rec + 18, 2);
        std::memcpy(&s.type, rec + 20, 2);
        std::memcpy(&s.flags, rec + 22, 2);
        s.bitmapId = rec[24];

        archive.sprites[i] = s;
    }

    return archive;
}

DecodedWav loadWAV(const std::string& path) {
    DecodedWav result;
    std::ifstream file(path, std::ios::binary);
    if (!file) return result;

    char riff[4];
    file.read(riff, 4);
    if (std::memcmp(riff, "RIFF", 4) != 0) return result;

    uint32_t riffSize = 0;
    file.read(reinterpret_cast<char*>(&riffSize), 4);

    char wave[4];
    file.read(wave, 4);
    if (std::memcmp(wave, "WAVE", 4) != 0) return result;

    while (file) {
        char chunkId[4];
        uint32_t chunkSize = 0;
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (!file) break;

        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat = 0;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&result.channels), 2);
            file.read(reinterpret_cast<char*>(&result.sampleRate), 4);
            uint32_t byteRate = 0;
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            uint16_t blockAlign = 0;
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&result.bitsPerSample), 2);

            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            const size_t numSamples = chunkSize / 2;
            result.samples.resize(numSamples);
            file.read(reinterpret_cast<char*>(result.samples.data()), chunkSize);
            break;
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    return result;
}

CityMap loadCityMap(const std::string& path) {
    CityMap map;
    std::ifstream file(path, std::ios::binary);
    if (!file) return map;

    file.seekg(0, std::ios::end);
    const size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 32) return map;

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    // Compressed tile payload starts at offset 16
    const size_t expectedTiles = 128 * 128;
    const size_t expectedBytes = expectedTiles * sizeof(uint16_t);
    map.tiles.resize(expectedTiles);

    uLongf destLen = static_cast<uLongf>(expectedBytes);
    const int zerr = uncompress(
        reinterpret_cast<Bytef*>(map.tiles.data()),
        &destLen,
        buffer.data() + 16,
        static_cast<uLong>(fileSize - 16)
    );

    if (zerr != Z_OK) {
        map.tiles.clear();
        map.width = 0;
        map.height = 0;
    }

    return map;
}

std::vector<std::string> loadEmperorText(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path, std::ios::binary);
    if (!file) return lines;

    file.seekg(0, std::ios::end);
    const size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    @autoreleasepool {
        CFStringRef cfStr = CFStringCreateWithBytes(
            kCFAllocatorDefault,
            buffer.data(),
            fileSize,
            kCFStringEncodingGB_18030_2000,
            false
        );
        if (!cfStr) return lines;

        NSString* nsStr = (__bridge_transfer NSString*)cfStr;
        NSArray<NSString*>* nsLines = [nsStr componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];
        for (NSString* line in nsLines) {
            NSString* trimmed = [line stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
            if (trimmed.length > 0) {
                lines.push_back(trimmed.UTF8String);
            }
        }
    }
    return lines;
}

DecodedImage extractSprite(const SG3Sprite& sprite, const std::string& companion555Path) {
    DecodedImage img;
    if (sprite.length == 0) return img;

    std::ifstream file(companion555Path, std::ios::binary);
    if (!file) return img;

    file.seekg(sprite.offset, std::ios::beg);
    std::vector<uint8_t> raw(sprite.length);
    file.read(reinterpret_cast<char*>(raw.data()), sprite.length);
    if (file.gcount() < static_cast<std::streamsize>(sprite.length)) return img;

    // Check if square tile
    uint32_t w = 0, h = 0;
    if (sprite.type == 16 || sprite.type == 24 || sprite.type == 32) {
        w = sprite.type;
        h = sprite.type;
    } else if (sprite.width > 0 && sprite.height > 0) {
        w = sprite.width;
        h = sprite.height;
    } else {
        uint32_t totalPixels = static_cast<uint32_t>(raw.size() / 2);
        w = 32;
        h = totalPixels / 32;
    }

    if (w == 0 || h == 0) return img;

    img.width = w;
    img.height = h;
    img.pixelsBGRA.resize(w * h, 0);

    const size_t rawBytes = std::min<size_t>(w * h * 2, raw.size());
    std::vector<uint8_t> srcBytes(raw.data(), raw.data() + rawBytes);
    if (srcBytes.size() < w * h * 2) {
        srcBytes.resize(w * h * 2, 0);
    }

    surface_lab::Store store(4, w * h * 8);
    surface_lab::Handle srcHandle = 0, dstHandle = 0;
    surface_lab::Desc srcDesc{
        static_cast<int32_t>(w),
        static_cast<int32_t>(h),
        w * 2,
        surface_lab::Format::RGB555LE
    };
    surface_lab::Desc dstDesc{
        static_cast<int32_t>(w),
        static_cast<int32_t>(h),
        w * 4,
        surface_lab::Format::BGRA8
    };

    if (store.create(srcDesc, srcHandle) == surface_lab::Status::Ok &&
        store.create(dstDesc, dstHandle) == surface_lab::Status::Ok) {
        store.replaceBytes(srcHandle, srcBytes);
        surface_lab::Rect rect{0, 0, static_cast<int32_t>(w), static_cast<int32_t>(h)};
        if (store.copy(dstHandle, srcHandle, rect, rect) == surface_lab::Status::Ok) {
            std::vector<uint8_t> dstBytes;
            if (store.readBytes(dstHandle, dstBytes) == surface_lab::Status::Ok &&
                dstBytes.size() == w * h * 4) {
                std::memcpy(img.pixelsBGRA.data(), dstBytes.data(), dstBytes.size());
            }
        }
    }

    return img;
}

} // namespace game_lab
