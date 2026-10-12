#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include "emperor_assets.hpp"
#include "audio_policy.hpp"
#include "resource_root.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
size_t checks = 0;
void require(bool value, const char* description) {
    ++checks;
    if (!value) throw std::runtime_error(description);
}
void put16(std::vector<uint8_t>& bytes, uint16_t n) {
    bytes.push_back(n & 0xff); bytes.push_back(n >> 8);
}
void put32(std::vector<uint8_t>& bytes, uint32_t n) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes.push_back((n >> shift) & 0xff);
}
void set16(std::vector<uint8_t>& bytes, size_t offset, uint16_t n) {
    bytes.at(offset) = n & 0xff; bytes.at(offset + 1) = n >> 8;
}
void set32(std::vector<uint8_t>& bytes, size_t offset, uint32_t n) {
    for (unsigned i = 0; i < 4; ++i) bytes.at(offset + i) = (n >> (i * 8)) & 0xff;
}
void chunk(std::vector<uint8_t>& bytes, const char* id, const std::vector<uint8_t>& payload) {
    bytes.insert(bytes.end(), id, id + 4); put32(bytes, static_cast<uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    if (payload.size() & 1) bytes.push_back(0);
}
std::vector<uint8_t> format(uint16_t channels = 1) {
    std::vector<uint8_t> f;
    put16(f, 1); put16(f, channels); put32(f, 22050); put32(f, 22050u * channels * 2u);
    put16(f, channels * 2); put16(f, 16);
    return f;
}
std::vector<uint8_t> wave(const std::vector<uint8_t>& f, const std::vector<uint8_t>& data,
                          bool junk = false, bool dataFirst = false) {
    std::vector<uint8_t> bytes{'R','I','F','F',0,0,0,0,'W','A','V','E'};
    if (junk) chunk(bytes, "JUNK", {0x7f});
    if (dataFirst) chunk(bytes, "data", data);
    chunk(bytes, "fmt ", f);
    if (!dataFirst) chunk(bytes, "data", data);
    set32(bytes, 4, static_cast<uint32_t>(bytes.size() - 8));
    return bytes;
}
void rejectWav(const std::vector<uint8_t>& bytes, const char* description) {
    const auto result = game_lab::decodeWAV(bytes);
    require(!result.valid() && result.samples.empty() && !result.channels && !result.sampleRate,
            description);
}
void testWav() {
    const std::vector<uint8_t> pcm{0,0x80,0xff,0xff,0,0,0xff,0x7f};
    const auto reference = wave(format(), pcm);
    const auto valid = game_lab::decodeWAV(reference);
    require(valid.valid() && valid.channels == 1 && valid.sampleRate == 22050, "valid mono PCM16");
    require(valid.samples == std::vector<int16_t>({-32768,-1,0,32767}), "independent signed LE sample oracle");
    require(game_lab::decodeWAV(wave(format(2), pcm)).valid(), "complete stereo frames");
    require(game_lab::decodeWAV(wave(format(), pcm, true)).samples == valid.samples, "odd unknown chunk padding");
    require(game_lab::decodeWAV(wave(format(), pcm, false, true)).samples == valid.samples, "data before format");
    rejectWav({}, "empty input");
    rejectWav(wave(format(), {0,0x80,0}), "odd data cannot write beyond sample storage");
    rejectWav(wave(format(2), {0,0}), "partial stereo frame");
    rejectWav(wave(format(), {}), "empty data");
    for (size_t length = 0; length < reference.size(); ++length)
        rejectWav(std::vector<uint8_t>(reference.begin(), reference.begin() + length), "all truncation points");
    auto changed = reference; changed.push_back(0); rejectWav(changed, "trailing bytes outside RIFF extent");
    changed = reference; set32(changed, 4, 0xffffffff); rejectWav(changed, "RIFF length overflow");
    changed = reference; set32(changed, 40, 0xffffffff); rejectWav(changed, "data length overflow");
    changed = reference; set32(changed, 16, 0xffffffff); rejectWav(changed, "format length overflow");
    changed = reference; set32(changed, 16, 4); rejectWav(changed, "format chunk shorter than fixed fields");
    for (const uint16_t audioFormat : {uint16_t(3), uint16_t(0xfffe)}) {
        auto f = format(); set16(f, 0, audioFormat); rejectWav(wave(f, pcm), "non-PCM formats rejected");
    }
    for (const uint16_t channels : {uint16_t(0), uint16_t(9)})
        rejectWav(wave(format(channels), pcm), "channel limits");
    auto f = format(); set16(f, 14, 8); rejectWav(wave(f, pcm), "unsupported bit depth");
    f = format(); set16(f, 12, 1); rejectWav(wave(f, pcm), "bad block alignment");
    f = format(); set32(f, 8, 1); rejectWav(wave(f, pcm), "bad byte rate");
    f = format(); set32(f, 4, 0); rejectWav(wave(f, pcm), "zero sample rate");
    f = format(); set32(f, 4, 192001); rejectWav(wave(f, pcm), "sample rate upper limit");
    changed = reference; chunk(changed, "fmt ", format()); set32(changed, 4, changed.size() - 8);
    rejectWav(changed, "duplicate format");
    changed = reference; chunk(changed, "data", pcm); set32(changed, 4, changed.size() - 8);
    rejectWav(changed, "duplicate data");
    changed = reference; changed.insert(changed.end(), {'J','U','N','K',1,0,0,0,0x7f});
    set32(changed, 4, changed.size() - 8); rejectWav(changed, "missing odd chunk padding");
}

void testPixels() {
    const std::vector<uint8_t> raw{0,0x7c,0xe0,0x03,0x1f,0,0xff,0x7f};
    const auto decoded = game_lab::decodeRaw555(raw, 2, 2);
    require(decoded.valid(), "RGB555 converts exact frame");
    require(decoded.pixelsBGRA == std::vector<uint32_t>({0xffff0000,0xff00ff00,0xff0000ff,0xffffffff}),
            "independent red green blue white anchors");
    require(decoded.pixels555 == std::vector<uint16_t>({0x7c00,0x03e0,0x001f,0x7fff}), "LE source words");
    const auto topBit = game_lab::decodeRaw555({0,0xfc}, 1, 1);
    require(topBit.valid() && topBit.pixelsBGRA[0] == 0xffff0000, "unused high bit");
    require(!game_lab::decodeRaw555({0}, 1, 1).valid(), "partial pixel rejected");
    require(!game_lab::decodeRaw555({0,0,0}, 1, 1).valid(), "extra pixel data rejected");
    require(!game_lab::decodeRaw555({}, 0, 1).valid(), "zero dimension rejected");
    require(!game_lab::decodeRaw555({}, 4097, 1).valid(), "dimension cap before allocation");
    require(!game_lab::decodeRaw555({}, 0xffffffff, 0xffffffff).valid(), "multiplication cannot wrap");
    game_lab::DecodedImage partial{2,2,{0}};
    require(!partial.valid(), "incomplete BGRA vector cannot be valid");
    auto missing = decoded; missing.pixelsBGRA.clear();
    require(!missing.valid(), "failed conversion cannot leave valid raw result");
    require(!game_lab::decodeJPEG({}).valid(), "empty JPEG");
    require(!game_lab::decodeJPEG({0xff,0xd8,0xff,0}).valid(), "broken JPEG");

    // Build a fresh JPEG from a solid self-authored 2x2 bitmap, never a game image.
    uint32_t pixels[] = {0xff336699,0xff336699,0xff336699,0xff336699};
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    require(space != nullptr, "fixture color space");
    CGContextRef context = CGBitmapContextCreate(pixels, 2, 2, 8, 8, space,
        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(space);
    require(context != nullptr, "fixture bitmap context");
    CGImageRef image = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    require(image != nullptr, "fixture CGImage");
    NSMutableData* jpeg = [NSMutableData data];
    CGImageDestinationRef destination = CGImageDestinationCreateWithData((__bridge CFMutableDataRef)jpeg,
        CFSTR("public.jpeg"), 1, nullptr);
    require(destination != nullptr, "fixture JPEG encoder");
    CGImageDestinationAddImage(destination, image, nullptr);
    const bool encoded = CGImageDestinationFinalize(destination);
    CFRelease(destination); CGImageRelease(image);
    require(encoded, "fixture JPEG bytes");
    const uint8_t* begin = static_cast<const uint8_t*>(jpeg.bytes);
    const auto result = game_lab::decodeJPEG(std::vector<uint8_t>(begin, begin + jpeg.length));
    require(result.valid() && result.width == 2 && result.height == 2, "self JPEG exact dimensions");
    for (const auto pixel : result.pixelsBGRA) {
        require((pixel >> 24) == 255, "JPEG opaque BGRA alpha");
        for (unsigned shift : {0u,8u,16u})
            require(std::abs(int((pixel >> shift) & 255) - int((0xff336699u >> shift) & 255)) <= 12,
                    "self JPEG channel order within lossy tolerance");
    }
}

struct Temporary {
    std::filesystem::path base;
    Temporary() {
        std::string name = (std::filesystem::temp_directory_path() / "emperor-self-boundary.XXXXXX").string();
        std::vector<char> buffer(name.begin(), name.end()); buffer.push_back(0);
        const char* created = mkdtemp(buffer.data());
        if (!created) throw std::runtime_error("self fixture directory unavailable");
        base = created;
    }
    ~Temporary() { std::error_code error; std::filesystem::remove_all(base, error); }
};
void write(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    require(bool(file), "write self-authored fixture");
}
void testFilesAndRoot() {
    Temporary temp;
    const auto directory = temp.base / "resources";
    std::filesystem::create_directories(directory / "nested");
    const auto pcm = wave(format(), {0,0x80,0xff,0x7f});
    write(directory / "nested/self.wav", pcm);
    write(temp.base / "outside.wav", pcm);
    const auto root = game_lab::ResourceRoot::fromAbsolute(directory.string());
    require(root.configured(), "explicit synthetic absolute root");
    const auto resolved = root.resolve("nested/self.wav");
    require(!resolved.empty() && game_lab::loadWAV(resolved).valid(), "self file loads through configured root");
    require(!game_lab::ResourceRoot::fromAbsolute("relative").configured(), "root never guesses cwd");
    require(!game_lab::ResourceRoot::fromAbsolute("").configured(), "missing root rejected");
    require(root.resolve("../outside.wav").empty(), "parent traversal rejected");
    require(root.resolve("nested/../nested/self.wav").empty(), "embedded parent traversal rejected");
    require(root.resolve((temp.base / "outside.wav").string()).empty(), "absolute child rejected");
    require(root.resolve("missing.wav").empty(), "missing file has no fallback");
    require(root.resolve("nested").empty(), "directory is not asset file");
    require(root.resolve(std::string("nested/self.wav\0tail", 20)).empty(), "NUL path rejected");
    std::filesystem::create_symlink(temp.base / "outside.wav", directory / "escape.wav");
    require(root.resolve("escape.wav").empty(), "symlink outside configured root rejected");
    write(directory / "raw.555", {0,0x7c});
    require(game_lab::loadRaw555(root.resolve("raw.555"), 1, 1).valid(), "self raw file");
    require(!game_lab::loadRaw555(root.resolve("raw.555"), 2, 1).valid(), "short raw file fails");
    std::vector<uint8_t> sg3(40680 + 64);
    set32(sg3, 4, 213); set32(sg3, 8, 1); set32(sg3, 16, 1);
    write(directory / "self.sg3", sg3);
    require(game_lab::loadSG3(root.resolve("self.sg3")).sprites.size() == 1, "self SG3 complete table");
    set32(sg3, 16, 0xffffffff); write(directory / "self.sg3", sg3);
    const auto truncated = game_lab::loadSG3(root.resolve("self.sg3"));
    require(truncated.sprites.empty() && !truncated.numImages, "unbounded SG3 count rejected before allocation");
    game_lab::SG3Sprite sprite; sprite.type = 16; sprite.length = 512;
    write(directory / "sprite.555", std::vector<uint8_t>(512, 0));
    require(game_lab::extractSprite(sprite, root.resolve("sprite.555")).valid(), "exact self square sprite");
    sprite.offset = 1;
    require(!game_lab::extractSprite(sprite, root.resolve("sprite.555")).valid(), "short range never padded");
    sprite.offset = 0; sprite.type = 78;
    require(!game_lab::extractSprite(sprite, root.resolve("sprite.555")).valid(), "unknown layout never guessed");
    sprite.type = 16; sprite.flags = 1;
    require(!game_lab::extractSprite(sprite, root.resolve("sprite.555")).valid(), "unsupported flags rejected");
}
void testAudioPolicy() {
    game_lab::AudioPolicy policy;
    require(policy.muted() && policy.gain() == 0 && !policy.permitsPlayback(), "launch silence for music and SFX");
    policy.setVolume(1);
    require(!policy.permitsPlayback(), "volume alone cannot unmute");
    policy.setMuted(false);
    require(policy.gain() == 1 && policy.permitsPlayback(), "explicit unmute permits explicit playback");
    policy.setVolume(-1); require(!policy.permitsPlayback(), "negative volume clamps silent");
    policy.setVolume(2); require(policy.gain() == 1, "high volume clamps");
    policy.setVolume(std::numeric_limits<double>::quiet_NaN());
    require(policy.gain() == 0, "nonfinite volume silent");
    policy.setVolume(0.4); policy.setMuted(true);
    require(policy.gain() == 0 && !policy.permitsPlayback(), "one mute policy stops both gains");
}
}

int main() {
    @autoreleasepool {
        try {
            testWav(); testPixels(); testFilesAndRoot(); testAudioPolicy();
            std::printf("{\"passed\":true,\"assertions\":%zu,\"fixtures\":\"self-authored\",\"game_data_read\":false,\"ui\":false,\"audio_devices\":false}\n", checks);
            return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "boundary failure: %s\n", error.what());
            return 1;
        }
    }
}
