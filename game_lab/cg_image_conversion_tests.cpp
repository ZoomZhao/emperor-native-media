#include "cg_image_conversion.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
size_t assertions = 0;
void check(bool value, const char* message) {
    ++assertions;
    if (!value) throw std::runtime_error(message);
}
struct Image {
    CGImageRef value;
    explicit Image(CGImageRef image) : value(image) {}
    ~Image() { if (value) CGImageRelease(value); }
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
};
struct Data {
    CFDataRef value;
    explicit Data(CFDataRef data) : value(data) {}
    ~Data() { if (value) CFRelease(value); }
};
std::vector<uint8_t> bytes(const std::vector<uint32_t>& pixels) {
    std::vector<uint8_t> result(pixels.size() * sizeof(uint32_t));
    if (!pixels.empty()) std::memcpy(result.data(), pixels.data(), result.size());
    return result;
}
void pixelsEqual(CGImageRef image, uint32_t width, uint32_t height,
                 const std::vector<uint32_t>& expected) {
    check(image != nullptr, "valid image created");
    check(CGImageGetWidth(image) == width && CGImageGetHeight(image) == height,
          "non-square dimensions preserved");
    check(CGImageGetBitsPerComponent(image) == 8 && CGImageGetBitsPerPixel(image) == 32,
          "8-bit BGRA channels");
    check(CGImageGetBytesPerRow(image) == size_t(width) * 4, "tight row stride preserved");
    check(CGImageGetAlphaInfo(image) == kCGImageAlphaPremultipliedFirst,
          "premultiplied-alpha contract preserved");
    check((CGImageGetBitmapInfo(image) & kCGBitmapByteOrderMask) == kCGBitmapByteOrder32Little,
          "little-endian BGRA byte order preserved");
    Data actual(CGDataProviderCopyData(CGImageGetDataProvider(image)));
    const auto want = bytes(expected);
    check(actual.value != nullptr && CFDataGetLength(actual.value) == CFIndex(want.size()),
          "exact provider data size");
    check(std::memcmp(CFDataGetBytePtr(actual.value), want.data(), want.size()) == 0,
          "provider bytes equal independent input including row order and alpha");
}
std::vector<uint32_t> ramp() {
    std::vector<uint32_t> result;
    for (uint32_t alpha = 0; alpha < 256; ++alpha) {
        // Explicit premultiplied input; no straight-alpha conversion is requested.
        result.push_back((alpha << 24) | ((alpha * 197 / 255) << 16) |
                         ((alpha * 103 / 255) << 8) | (alpha * 29 / 255));
    }
    return result;
}
void rejectedInputs() {
    const std::vector<uint32_t> sentinel{0xff123456};
    const auto original = sentinel;
    for (const auto& shape : {std::pair<uint32_t,uint32_t>{0,1}, {1,0}, {4097,1},
                            {1,4097}, {std::numeric_limits<uint32_t>::max(),2}}) {
        Image image(game_lab::createCGImageFromPremultipliedBGRA(shape.first,shape.second,sentinel));
        check(image.value == nullptr, "invalid dimensions reject before native allocation");
    }
    for (const auto& input : {std::vector<uint32_t>{}, std::vector<uint32_t>{1,2}}) {
        Image image(game_lab::createCGImageFromPremultipliedBGRA(1,1,input));
        check(image.value == nullptr, "short or long input rejected");
    }
    check(sentinel == original, "rejected conversion leaves input unchanged");
}
void layoutAndAlpha() {
    const std::vector<uint32_t> corners{
        0xff0000ff, 0xff00ff00, 0xffff0000, 0xff123456, 0xff654321, 0xffffffff};
    self_fixture::bytes("cg-nonsquare-corners",bytes(corners));
    Image image(game_lab::createCGImageFromPremultipliedBGRA(2,3,corners));
    pixelsEqual(image.value,2,3,corners);
    const auto input = ramp();
    self_fixture::bytes("cg-premultiplied-alpha-ramp",bytes(input));
    Image alpha(game_lab::createCGImageFromPremultipliedBGRA(32,8,input));
    pixelsEqual(alpha.value,32,8,input);
    check(input == ramp(), "successful conversion leaves all input channels unchanged");
}
void inputLifetime() {
    auto input = ramp();
    const auto expected = input;
    Image image(game_lab::createCGImageFromPremultipliedBGRA(16,16,input));
    std::fill(input.begin(),input.end(),0xffabcdef);
    pixelsEqual(image.value,16,16,expected);
    std::vector<uint32_t>().swap(input);
    std::vector<uint32_t> reuse(256,0xfffedcba);
    pixelsEqual(image.value,16,16,expected);
    check(reuse.front() == 0xfffedcba, "caller reuse remains independent");
}
void repeatedReplacement() {
    for (uint32_t round = 0; round < 256; ++round) {
        const uint32_t width = round % 7 + 1, height = round % 5 + 1;
        auto input = std::vector<uint32_t>(width * height,0xff000000 | round * 0x010101);
        const auto expected = input;
        Image previous(game_lab::createCGImageFromPremultipliedBGRA(width,height,input));
        std::vector<uint32_t>().swap(input);
        const std::vector<uint32_t> nextPixels{0xff123456,0xff654321};
        Image next(game_lab::createCGImageFromPremultipliedBGRA(1,2,nextPixels));
        pixelsEqual(next.value,1,2,nextPixels);
        pixelsEqual(previous.value,width,height,expected);
        // Both native images release at the end of this iteration.
    }
}
}
int main() {
    try {
        rejectedInputs(); layoutAndAlpha(); inputLifetime(); repeatedReplacement();
        std::cout << "{\"passed\":true,\"assertions\":" << assertions
                  << ",\"replacement_release_cycles\":256,\"alpha_values\":256,"
                     "\"scope\":\"shared App CoreGraphics image conversion; offscreen only\","
                     "\"UI_used\":false,\"audio_devices_used\":false}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
