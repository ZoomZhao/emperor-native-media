#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include "emperor_assets.hpp"
#include "audio_policy.hpp"
#include "resource_root.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <cmath>
#include <cstring>
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

game_lab::DecodedWav tracedWAV(const std::vector<uint8_t>& bytes) {
    self_fixture::bytes("wav",bytes);return game_lab::decodeWAV(bytes);
}
game_lab::Raw555Image tracedRaw555(const std::vector<uint8_t>& bytes,uint32_t width,uint32_t height) {
    self_fixture::numbers("rgb555-dimensions-u64le",{width,height});
    self_fixture::bytes("rgb555",bytes);return game_lab::decodeRaw555(bytes,width,height);
}
game_lab::DecodedImage tracedJPEG(const std::vector<uint8_t>& bytes) {
    self_fixture::bytes("jpeg",bytes);return game_lab::decodeJPEG(bytes);
}

game_lab::ResourceRoot tracedRoot(const std::string& path) {
    self_fixture::bytes("resource-root-utf8",std::vector<uint8_t>(path.begin(),path.end()));
    return game_lab::ResourceRoot::fromAbsolute(path);
}
std::string tracedResolve(const game_lab::ResourceRoot& root,const std::string& path) {
    self_fixture::bytes("resource-path-utf8",std::vector<uint8_t>(path.begin(),path.end()));
    return root.resolve(path);
}
void tracedVolume(game_lab::AudioPolicy& policy,double value) {
    uint64_t bits=0;static_assert(sizeof(bits)==sizeof(value));std::memcpy(&bits,&value,sizeof(bits));
    self_fixture::numbers("policy-volume-f64bits-u64le",{bits});policy.setVolume(value);
}
void tracedMuted(game_lab::AudioPolicy& policy,bool value) {
    self_fixture::numbers("policy-muted-u64le",{value?1u:0u});policy.setMuted(value);
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
    const auto result = tracedWAV(bytes);
    require(!result.valid() && result.samples.empty() && !result.channels && !result.sampleRate,
            description);
}
void testWav() {
    const std::vector<uint8_t> pcm{0,0x80,0xff,0xff,0,0,0xff,0x7f};
    const auto reference = wave(format(), pcm);
    const auto valid = tracedWAV(reference);
    require(valid.valid() && valid.channels == 1 && valid.sampleRate == 22050, "valid mono PCM16");
    require(valid.samples == std::vector<int16_t>({-32768,-1,0,32767}), "independent signed LE sample oracle");
    require(tracedWAV(wave(format(2), pcm)).valid(), "complete stereo frames");
    require(tracedWAV(wave(format(), pcm, true)).samples == valid.samples, "odd unknown chunk padding");
    require(tracedWAV(wave(format(), pcm, false, true)).samples == valid.samples, "data before format");
    rejectWav({}, "empty input");
    rejectWav(wave(format(), {0,0x80,0}), "odd data cannot write beyond sample storage");
    rejectWav(wave(format(2), {0,0}), "partial stereo frame");
    rejectWav(wave(format(), {}), "empty data");
    for (size_t length = 0; length < reference.size(); ++length)
        rejectWav(std::vector<uint8_t>(reference.begin(), reference.begin() + length), "all truncation points");
    auto extension = format(); extension.push_back(0);
    rejectWav(wave(extension, pcm), "partial WAVEFORMATEX length field");
    extension = format(); put16(extension, 4);
    rejectWav(wave(extension, pcm), "declared format extension body is missing");
    extension = format(); put16(extension, 0);
    require(tracedWAV(wave(extension, pcm)).valid(), "zero-length PCM format extension");
    extension = format(); put16(extension, 1); extension.push_back(0x5a);
    require(tracedWAV(wave(extension, pcm)).valid(), "complete odd format extension includes pad");
    for(uint16_t channels: {uint16_t(1),uint16_t(8)})for(uint32_t rate: {8000u,192000u}){
        auto f=format(channels);set32(f,4,rate);set32(f,8,rate*channels*2);
        std::vector<uint8_t> frame;
        std::vector<int16_t> expected;
        for(uint16_t channel=0;channel<channels;++channel){put16(frame,0x8000);expected.push_back(-32768);}
        const auto decoded=tracedWAV(wave(f,frame));
        require(decoded.valid()&&decoded.channels==channels&&decoded.sampleRate==rate&&decoded.samples==expected,
                "PCM format endpoint matrix with independent full-frame oracle");
    }
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
    const auto decoded = tracedRaw555(raw, 2, 2);
    require(decoded.valid(), "RGB555 converts exact frame");
    require(decoded.pixelsBGRA == std::vector<uint32_t>({0xffff0000,0xff00ff00,0xff0000ff,0xffffffff}),
            "independent red green blue white anchors");
    require(decoded.pixels555 == std::vector<uint16_t>({0x7c00,0x03e0,0x001f,0x7fff}), "LE source words");
    const auto topBit = tracedRaw555({0,0xfc}, 1, 1);
    require(topBit.valid() && topBit.pixelsBGRA[0] == 0xffff0000, "unused high bit");
    require(!tracedRaw555({0}, 1, 1).valid(), "partial pixel rejected");
    require(!tracedRaw555({0,0,0}, 1, 1).valid(), "extra pixel data rejected");
    require(!tracedRaw555({}, 0, 1).valid(), "zero dimension rejected");
    require(!tracedRaw555({}, 4097, 1).valid(), "dimension cap before allocation");
    require(!tracedRaw555({}, 0xffffffff, 0xffffffff).valid(), "multiplication cannot wrap");
    game_lab::DecodedImage partial{2,2,{0}};
    require(!partial.valid(), "incomplete BGRA vector cannot be valid");
    auto missing = decoded; missing.pixelsBGRA.clear();
    require(!missing.valid(), "failed conversion cannot leave valid raw result");
    std::vector<uint8_t> domain(65536u * 2u);
    for(uint32_t word=0;word<65536;++word){domain[word*2]=word&255;domain[word*2+1]=word>>8;}
    const auto full=tracedRaw555(domain,256,256);
    require(full.valid(), "full RGB555LE word domain converts");
    constexpr uint8_t expansion[32]={0,8,16,24,33,41,49,57,66,74,82,90,99,107,115,123,
        132,140,148,156,165,173,181,189,198,206,214,222,231,239,247,255};
    for(uint32_t word=0;word<65536;++word){
        const uint32_t expected=0xff000000u|(uint32_t(expansion[(word>>10)&31])<<16)|
            (uint32_t(expansion[(word>>5)&31])<<8)|expansion[word&31];
        require(full.pixelsBGRA[word]==expected, "independent lookup oracle for every word including high bit");
    }
    require(!tracedJPEG({}).valid(), "empty JPEG");
    require(!tracedJPEG({0xff,0xd8,0xff,0}).valid(), "broken JPEG");

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
    const auto result = tracedJPEG(std::vector<uint8_t>(begin, begin + jpeg.length));
    require(result.valid() && result.width == 2 && result.height == 2, "self JPEG exact dimensions");
    std::vector<uint8_t> incomplete(begin,begin+jpeg.length);
    incomplete.pop_back();
    require(!tracedJPEG(incomplete).valid(), "truncated JPEG container cannot be valid");
    std::vector<uint8_t> oversized(begin,begin+jpeg.length);
    bool found=false;
    for(size_t offset=0;offset+8<oversized.size();++offset){
        if(oversized[offset]==0xff&&(oversized[offset+1]==0xc0||oversized[offset+1]==0xc2)){
            oversized[offset+5]=0x10;oversized[offset+6]=1;found=true;break;
        }
    }
    require(found,"self JPEG contains a recognized size record");
    require(!tracedJPEG(oversized).valid(),"JPEG metadata dimension cap before bitmap allocation");
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
    self_fixture::bytes("synthetic-file",data);
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
    const auto root = tracedRoot(directory.string());
    require(root.configured(), "explicit synthetic absolute root");
    const auto resolved = tracedResolve(root,"nested/self.wav");
    require(!resolved.empty() && game_lab::loadWAV(resolved).valid(), "self file loads through configured root");
    require(!tracedRoot("relative").configured(), "root never guesses cwd");
    require(!tracedRoot("").configured(), "missing root rejected");
    require(tracedResolve(root,"../outside.wav").empty(), "parent traversal rejected");
    require(tracedResolve(root,"nested/../nested/self.wav").empty(), "embedded parent traversal rejected");
    require(tracedResolve(root,(temp.base / "outside.wav").string()).empty(), "absolute child rejected");
    require(tracedResolve(root,"missing.wav").empty(), "missing file has no fallback");
    require(tracedResolve(root,"nested").empty(), "directory is not asset file");
    require(tracedResolve(root,std::string("nested/self.wav\0tail", 20)).empty(), "NUL path rejected");
    std::filesystem::create_symlink(temp.base / "outside.wav", directory / "escape.wav");
    require(tracedResolve(root,"escape.wav").empty(), "symlink outside configured root rejected");
    std::filesystem::create_symlink(directory / "nested/self.wav",directory / "inside.wav");
    require(tracedResolve(root,"inside.wav")==resolved,"symlink inside explicit root is accepted");
    const auto prefixSibling=temp.base/"resources_extra";
    std::filesystem::create_directories(prefixSibling);write(prefixSibling/"self.wav",pcm);
    std::filesystem::create_symlink(prefixSibling/"self.wav",directory/"prefix.wav");
    require(tracedResolve(root,"prefix.wav").empty(),"lexical prefix sibling is outside component root");
    require(!tracedRoot((directory/"missing").string()).configured(),"missing absolute root rejected");
    require(!tracedRoot(resolved).configured(),"file cannot be configured as root");
    write(directory / "raw.555", {0,0x7c});
    require(game_lab::loadRaw555(tracedResolve(root,"raw.555"), 1, 1).valid(), "self raw file");
    require(!game_lab::loadRaw555(tracedResolve(root,"raw.555"), 2, 1).valid(), "short raw file fails");

}
void testAudioPolicy() {
    game_lab::AudioPolicy policy;
    require(policy.muted() && policy.gain() == 0 && !policy.permitsPlayback(), "launch silence for music and SFX");
    tracedVolume(policy,1);
    require(!policy.permitsPlayback(), "volume alone cannot unmute");
    tracedMuted(policy,false);
    require(policy.gain() == 1 && policy.permitsPlayback(), "explicit unmute permits explicit playback");
    tracedVolume(policy,-1); require(!policy.permitsPlayback(), "negative volume clamps silent");
    tracedVolume(policy,2); require(policy.gain() == 1, "high volume clamps");
    tracedVolume(policy,std::numeric_limits<double>::quiet_NaN());
    require(policy.gain() == 0, "nonfinite volume silent");
    tracedVolume(policy,std::numeric_limits<double>::infinity());
    require(policy.gain()==0&&!policy.permitsPlayback(),"positive infinity remains silent");
    tracedVolume(policy,-std::numeric_limits<double>::infinity());
    require(policy.gain()==0&&!policy.permitsPlayback(),"negative infinity remains silent");
    tracedVolume(policy,0.4); tracedMuted(policy,true);
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
