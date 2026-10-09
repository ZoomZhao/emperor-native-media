#include "surface.hpp"
#include "oracle.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace surface_lab;
namespace {
constexpr uint64_t pixelSeed = 0x5eed565b6a8a0001ULL;
constexpr uint64_t lifeSeed = 0x1a11ce005eed0002ULL;
struct Random {
    uint64_t state;
    uint64_t next() { state ^= state << 13; state ^= state >> 7; state ^= state << 17; return state; }
    int below(int limit) { return int(next() % uint64_t(limit)); }
};
struct Totals {
    size_t assertions = 0, bytesCompared = 0, copyCases = 0, nearestCases = 0;
    size_t negativeChecks = 0, randomPixelCases = 0, randomLifeEvents = 0, groupsPassed = 0;
    size_t exhaustiveValues = 0, exhaustiveRGB555Values = 0, lifecycleCycles = 0;
    uint64_t pixelRngFinal = 0, lifeRngFinal = 0;
} totals;
std::string evidenceDirectory;
void require(bool condition, const std::string& detail) {
    ++totals.assertions;
    if (!condition) throw std::runtime_error(detail);
}
#define CHECK(value) require((value), std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #value)
void status(Status actual, Status expected) {
    if (expected != Status::Ok) ++totals.negativeChecks;
    require(actual == expected, std::string("status expected ") + statusName(expected) + ", got " + statusName(actual));
}
size_t bpp(Format format) { return (format == Format::RGB565LE || format == Format::RGB555LE) ? 2 : 4; }
Desc description(int width, int height, Format format, int padding = 0) {
    return {width, height, size_t(width) * bpp(format) + size_t(padding), format};
}
bool sameDescription(Desc a, Desc b) {
    return a.width == b.width && a.height == b.height && a.stride == b.stride && a.format == b.format;
}
bool sameSnapshot(const Snapshot& a, const Snapshot& b) {
    return sameDescription(a.desc, b.desc) && a.lost == b.lost && a.bytes == b.bytes;
}
std::vector<uint8_t> pattern(Desc desc, unsigned salt) {
    std::vector<uint8_t> result(desc.stride * size_t(desc.height), uint8_t(0xa0 + salt % 16));
    for (int y = 0; y < desc.height; ++y) for (int x = 0; x < desc.width; ++x) {
        scalar_oracle::pack(result, desc, x, y, {
            (unsigned(x) * 37 + unsigned(y) * 11 + salt) % 256,
            (unsigned(x) * 7 + unsigned(y) * 53 + salt * 3) % 256,
            (unsigned(x) * 23 + unsigned(y) * 5 + salt * 7) % 256,
            (unsigned(x) * 19 + unsigned(y) * 31 + salt * 13) % 256});
    }
    return result;
}
Handle create(Store& store, Desc desc, const std::vector<uint8_t>& initial) {
    Handle result = 0;
    status(store.create(desc, result), Status::Ok);
    CHECK(result != 0);
    status(store.replaceBytes(result, initial), Status::Ok);
    return result;
}
std::vector<uint8_t> read(Store& store, Handle handle) {
    std::vector<uint8_t> result;
    status(store.readBytes(handle, result), Status::Ok);
    return result;
}
Snapshot snapshot(Store& store, Handle handle) {
    Snapshot result;
    status(store.snapshot(handle, result), Status::Ok);
    return result;
}
void writeFile(const std::string& name, const std::vector<uint8_t>& value) {
    std::ofstream stream(evidenceDirectory + "/" + name, std::ios::binary);
    CHECK(bool(stream));
    stream.write(reinterpret_cast<const char*>(value.data()), std::streamsize(value.size()));
    CHECK(bool(stream));
}
std::string descText(Desc d) {
    std::ostringstream out;
    const char* fmt = (d.format == Format::RGB565LE ? "RGB565LE" : (d.format == Format::RGB555LE ? "RGB555LE" : "BGRA8"));
    out << "{\"width\":" << d.width << ",\"height\":" << d.height
        << ",\"stride\":" << d.stride << ",\"format\":\""
        << fmt << "\"}";
    return out.str();
}
std::string rectText(Rect r) {
    return "[" + std::to_string(r.x) + "," + std::to_string(r.y) + "," +
        std::to_string(r.width) + "," + std::to_string(r.height) + "]";
}
void bytesEqual(const std::vector<uint8_t>& actual, const std::vector<uint8_t>& expected,
    const std::string& context = "") {
    CHECK(actual.size() == expected.size());
    totals.bytesCompared += expected.size();
    const auto differing = std::mismatch(actual.begin(), actual.end(), expected.begin());
    if (differing.first != actual.end()) {
        writeFile("failure_actual.bin", actual); writeFile("failure_expected.bin", expected);
        std::ofstream meta(evidenceDirectory + "/failure_context.txt");
        meta << context << "\nfirst_byte=" << (differing.first - actual.begin()) << "\n";
        throw std::runtime_error("pixel mismatch at byte " + std::to_string(differing.first - actual.begin()) + ": " + context);
    }
    CHECK(true);
}
void transferCase(Store& store, Handle dst, Handle src, Desc sd, Desc dd,
    const std::vector<uint8_t>& beforeSource, const std::vector<uint8_t>& beforeDestination,
    Rect sr, Rect dr, bool scale, const std::string& label) {
    const auto expected = scalar_oracle::expected(beforeSource, sd, beforeDestination, dd, sr, dr);
    status(scale ? store.nearest(dst, src, sr, dr) : store.copy(dst, src, sr, dr), Status::Ok);
    if (scale) ++totals.nearestCases; else ++totals.copyCases;
    const auto actual = read(store, dst);
    const std::string context = label + " source=" + descText(sd) + " destination=" + descText(dd) +
        " source_rect=" + rectText(sr) + " destination_rect=" + rectText(dr);
    if (actual != expected) writeFile("failure_source.bin", beforeSource);
    bytesEqual(actual, expected, context);
}

void exhaustiveFormats() {
    Store store;
    const Desc words = description(256, 256, Format::RGB565LE, 3);
    const Desc colors = description(256, 256, Format::BGRA8, 5);
    std::vector<uint8_t> raw(words.stride * 256, 0xa5);
    for (unsigned value = 0; value < 65536; ++value) {
        const size_t offset = size_t(value / 256) * words.stride + size_t(value % 256) * 2;
        raw[offset] = uint8_t(value % 256); raw[offset + 1] = uint8_t(value / 256);
    }
    const auto originalDst = pattern(colors, 9);
    const Handle a = create(store, words, raw), b = create(store, colors, originalDst);
    const Rect full{0, 0, 256, 256};
    transferCase(store, b, a, words, colors, raw, originalDst, full, full, false, "all 65536 RGB565 words");
    totals.exhaustiveValues = 65536;
    const auto decoded = read(store, b);
    for (const auto& anchor : std::vector<std::pair<unsigned, scalar_oracle::Color>>{
        {0x0000, {0, 0, 0, 255}}, {0xf800, {0, 0, 255, 255}},
        {0x07e0, {0, 255, 0, 255}}, {0x001f, {255, 0, 0, 255}}, {0xffff, {255, 255, 255, 255}}}) {
        const size_t at = size_t(anchor.first / 256) * colors.stride + size_t(anchor.first % 256) * 4;
        for (size_t channel = 0; channel < 4; ++channel) CHECK(decoded[at + channel] == anchor.second[channel]);
    }
    const auto beforeBack = pattern(words, 19);
    status(store.replaceBytes(a, beforeBack), Status::Ok);
    transferCase(store, a, b, colors, words, decoded, beforeBack, full, full, false, "RGB565 roundtrip");
    const auto roundtrip = read(store, a);
    for (int y = 0; y < 256; ++y) for (int x = 0; x < 512; ++x)
        CHECK(roundtrip[size_t(y) * words.stride + size_t(x)] == raw[size_t(y) * words.stride + size_t(x)]);

    const Desc channels = description(256, 4, Format::BGRA8, 7);
    const Desc quantized = description(256, 4, Format::RGB565LE, 1);
    std::vector<uint8_t> channelBytes(channels.stride * 4, 0xb7);
    for (int row = 0; row < 4; ++row) for (int value = 0; value < 256; ++value) {
        scalar_oracle::Color c{0, 0, 0, 0}; c[size_t(row)] = unsigned(value);
        scalar_oracle::pack(channelBytes, channels, value, row, c);
    }
    const auto quantBefore = pattern(quantized, 21);
    const Handle c = create(store, channels, channelBytes), d = create(store, quantized, quantBefore);
    transferCase(store, d, c, channels, quantized, channelBytes, quantBefore,
        {0, 0, 256, 4}, {0, 0, 256, 4}, false, "BGRA channel quantization including alpha ignored");
    // Exhaustive 32768 RGB555LE words to BGRA8 and roundtrip
    const Desc words555 = description(256, 128, Format::RGB555LE, 3);
    const Desc colors555 = description(256, 128, Format::BGRA8, 5);
    std::vector<uint8_t> raw555(words555.stride * 128, 0x5a);
    for (unsigned value = 0; value < 32768; ++value) {
        const size_t offset = size_t(value / 256) * words555.stride + size_t(value % 256) * 2;
        raw555[offset] = uint8_t(value % 256); raw555[offset + 1] = uint8_t(value / 256);
    }
    const auto origDst555 = pattern(colors555, 13);
    const Handle h555_src = create(store, words555, raw555), h555_dst = create(store, colors555, origDst555);
    const Rect full555{0, 0, 256, 128};
    transferCase(store, h555_dst, h555_src, words555, colors555, raw555, origDst555, full555, full555, false, "all 32768 RGB555 words");
    totals.exhaustiveRGB555Values = 32768;
    const auto decoded555 = read(store, h555_dst);
    for (const auto& anchor : std::vector<std::pair<unsigned, scalar_oracle::Color>>{
        {0x0000, {0, 0, 0, 255}}, {0x7c00, {0, 0, 255, 255}},
        {0x03e0, {0, 255, 0, 255}}, {0x001f, {255, 0, 0, 255}}, {0x7fff, {255, 255, 255, 255}}}) {
        const size_t at = size_t(anchor.first / 256) * colors555.stride + size_t(anchor.first % 256) * 4;
        for (size_t channel = 0; channel < 4; ++channel) CHECK(decoded555[at + channel] == anchor.second[channel]);
    }
    const auto beforeBack555 = pattern(words555, 29);
    status(store.replaceBytes(h555_src, beforeBack555), Status::Ok);
    transferCase(store, h555_src, h555_dst, colors555, words555, decoded555, beforeBack555, full555, full555, false, "RGB555 roundtrip");
    const auto roundtrip555 = read(store, h555_src);
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 512; ++x)
        CHECK(roundtrip555[size_t(y) * words555.stride + size_t(x)] == raw555[size_t(y) * words555.stride + size_t(x)]);

    for (Handle h : {a, b, c, d, h555_src, h555_dst}) status(store.destroy(h), Status::Ok);
    CHECK(store.liveCount() == 0 && store.activeBytes() == 0 && store.savedBytes() == 0);
}

void clippingAndPadding() {
    const std::vector<std::pair<Rect, Rect>> cases{
        {{0,0,13,7}, {2,1,13,7}}, {{0,0,13,7}, {-3,-2,13,7}},
        {{-3,-2,13,7}, {1,1,13,7}}, {{7,3,13,7}, {0,0,13,7}},
        {{0,0,13,7}, {16,8,13,7}}, {{0,0,13,7}, {17,9,13,7}},
        {{50,50,13,7}, {0,0,13,7}}, {{0,0,1,1}, {16,8,1,1}},
        {{-1,0,1,1}, {3,2,1,1}}, {{0,0,13,7}, {-13,0,13,7}}};
    for (Format sf : {Format::RGB565LE, Format::BGRA8, Format::RGB555LE}) for (Format df : {Format::RGB565LE, Format::BGRA8, Format::RGB555LE}) {
        const Desc sd = description(13, 7, sf, 5), dd = description(17, 9, df, 7);
        const auto input = pattern(sd, 3), output = pattern(dd, 17);
        for (const auto& c : cases) {
            Store store;
            const Handle a = create(store, sd, input), b = create(store, dd, output);
            transferCase(store, b, a, sd, dd, input, output, c.first, c.second, false, "clip + odd stride");
            bytesEqual(read(store, a), input, "source/padding preserved");
        }
    }
}

void overlappingCopies() {
    const std::vector<std::pair<Rect, Rect>> cases{
        {{0,0,11,7}, {2,0,11,7}}, {{2,0,11,7}, {0,0,11,7}},
        {{0,0,13,6}, {0,1,13,6}}, {{0,1,13,6}, {0,0,13,6}},
        {{0,0,13,7}, {0,0,13,7}}, {{-2,-1,13,7}, {1,1,13,7}}};
    for (Format f : {Format::RGB565LE, Format::BGRA8, Format::RGB555LE}) for (const auto& c : cases) {
        Store store; const Desc d = description(13, 7, f, 7); const auto original = pattern(d, 29);
        const Handle h = create(store, d, original);
        transferCase(store, h, h, d, d, original, original, c.first, c.second, false, "overlap source snapshot");
    }
}

void nearestScaling() {
    const std::vector<std::pair<Rect, Rect>> cases{
        {{0,0,17,9}, {0,0,13,7}}, {{0,0,3,2}, {0,0,5,4}},
        {{0,0,3,2}, {-1,-1,5,4}}, {{0,0,8,6}, {0,0,16,12}},
        {{0,0,17,9}, {-4,-3,23,17}}, {{-2,-1,17,9}, {1,2,12,8}},
        {{12,6,9,8}, {0,0,7,6}}, {{0,0,17,9}, {40,40,5,4}}};
    for (Format sf : {Format::RGB565LE, Format::BGRA8, Format::RGB555LE}) for (Format df : {Format::RGB565LE, Format::BGRA8, Format::RGB555LE}) {
        const Desc sd = description(17, 9, sf, 3), dd = description(13, 11, df, 5);
        const auto input = pattern(sd, 7), output = pattern(dd, 11);
        for (const auto& c : cases) {
            Store store; const Handle a = create(store, sd, input), b = create(store, dd, output);
            transferCase(store, b, a, sd, dd, input, output, c.first, c.second, true, "nearest full mapping then clip");
        }
        Store same; const Handle a = create(same, sd, input);
        transferCase(same, a, a, sd, sd, input, input, {0,0,7,5}, {2,1,13,7}, true, "nearest alias snapshot");
    }
    // Hand-written mapping anchor: 3 -> 5 must be [0,0,1,1,2].
    Store store; const Desc sd = description(3, 1, Format::BGRA8), dd = description(5, 1, Format::BGRA8);
    const std::vector<uint8_t> source{10,0,0,255, 20,0,0,255, 30,0,0,255};
    const std::vector<uint8_t> expected{10,0,0,255, 10,0,0,255, 20,0,0,255, 20,0,0,255, 30,0,0,255};
    const Handle a = create(store, sd, source), b = create(store, dd, std::vector<uint8_t>(20, 0));
    status(store.nearest(b, a, {0,0,3,1}, {0,0,5,1}), Status::Ok); ++totals.nearestCases;
    bytesEqual(read(store, b), expected, "manual 3-to-5 anchor");
}

void rejectedInputs() {
    Store store(2, 128); const Desc good = description(4, 3, Format::BGRA8, 4);
    const auto original = pattern(good, 1); const Handle h = create(store, good, original);
    const auto before = snapshot(store, h);
    for (Desc d : std::vector<Desc>{{0,3,16,Format::BGRA8}, {-1,3,16,Format::BGRA8},
        {4,0,16,Format::BGRA8}, {5000,1,20000,Format::BGRA8}, {4,3,15,Format::BGRA8},
        {4,3,16,static_cast<Format>(99)}, {4,3,std::numeric_limits<size_t>::max(),Format::BGRA8}}) {
        Handle out = 123; status(store.create(d, out), Status::InvalidDesc); CHECK(out == 0);
        CHECK(sameSnapshot(before, snapshot(store, h)));
    }
    Handle out = 123;
    status(store.create(description(20, 20, Format::BGRA8), out), Status::Capacity); CHECK(out == 0);
    const Handle second = create(store, good, original);
    status(store.create(description(1,1,Format::BGRA8), out), Status::Capacity); CHECK(out == 0);
    status(store.replaceBytes(h, {1,2,3}), Status::InvalidSnapshot);
    for (const auto& rect : std::vector<std::pair<Rect, Rect>>{
        {{0,0,0,1},{0,0,0,1}}, {{0,0,-1,1},{0,0,-1,1}}, {{0,0,2,2},{0,0,3,2}}})
        status(store.copy(h, second, rect.first, rect.second), Status::InvalidRect);
    status(store.nearest(h, second, {0,0,1,0}, {0,0,1,1}), Status::InvalidRect);
    for (Handle invalid : {Handle(0), Handle(1), Handle(0x0000000100000064ULL), h + (1ULL << 32)}) {
        status(store.destroy(invalid), Status::InvalidHandle);
        status(store.copy(h, invalid, {0,0,1,1}, {0,0,1,1}), Status::InvalidHandle);
        std::vector<uint8_t> sentinel{3,5,7};
        status(store.readBytes(invalid, sentinel), Status::InvalidHandle); CHECK(sentinel == std::vector<uint8_t>({3,5,7}));
    }
    const int32_t lo = std::numeric_limits<int32_t>::min(), hi = std::numeric_limits<int32_t>::max();
    status(store.nearest(h, second, {0,0,hi,hi}, {lo,lo,hi,hi}), Status::Ok);
    status(store.nearest(h, second, {hi,hi,hi,hi}, {0,0,hi,hi}), Status::Ok);
    CHECK(sameSnapshot(before, snapshot(store, h)));
    auto bad = before; bad.bytes.pop_back(); status(store.importSnapshot(h, bad), Status::InvalidSnapshot);
    bad = before; ++bad.desc.width; status(store.importSnapshot(h, bad), Status::InvalidSnapshot);
    bad = before; bad.desc.format = static_cast<Format>(99); status(store.importSnapshot(h, bad), Status::InvalidSnapshot);
    CHECK(sameSnapshot(before, snapshot(store, h)));
    status(store.destroy(second), Status::Ok); status(store.destroy(h), Status::Ok);
    CHECK(store.liveCount() == 0 && store.activeBytes() == 0 && store.savedBytes() == 0);
    // A visible pixel with near-INT32_MAX coordinate deltas exercises the
    // int64 multiplication path, without an impractically large image/oracle.
    Store extreme;
    const Desc tiny = description(1,1,Format::BGRA8);
    const std::vector<uint8_t> source{3,4,5,6}, destination{8,7,6,5};
    const Handle a = create(extreme,tiny,source), b = create(extreme,tiny,destination);
    const Rect huge{lo + 2,lo + 2,hi,hi};
    status(extreme.nearest(b,a,huge,huge),Status::Ok); ++totals.nearestCases;
    bytesEqual(read(extreme,b),source,"INT32 extreme mapping; manual expected pixel");
}

void lifecycle() {
    Store store(1, 128); const Desc d = description(4,3,Format::BGRA8,4); const auto original = pattern(d,31);
    const Handle h = create(store,d,original); status(store.restore(h),Status::AlreadyReady);
    const auto ready = snapshot(store,h); status(store.lose(h),Status::Ok);
    CHECK(store.liveCount()==1 && store.activeBytes()==0 && store.savedBytes()==60);
    const auto lost = snapshot(store,h); CHECK(lost.lost && lost.bytes==original);
    auto brokenLost = lost; brokenLost.bytes.pop_back();
    status(store.importSnapshot(h,brokenLost),Status::InvalidSnapshot);
    CHECK(sameSnapshot(lost,snapshot(store,h)));
    std::vector<uint8_t> sentinel{9,8}; status(store.readBytes(h,sentinel),Status::Lost); CHECK(sentinel==std::vector<uint8_t>({9,8}));
    status(store.replaceBytes(h,original),Status::Lost);
    status(store.copy(h,h,{0,0,4,3},{0,0,4,3}),Status::Lost);
    status(store.nearest(h,h,{0,0,4,3},{0,0,4,3}),Status::Lost);
    status(store.lose(h),Status::Lost); CHECK(sameSnapshot(lost,snapshot(store,h)));
    status(store.restore(h),Status::Ok); CHECK(sameSnapshot(ready,snapshot(store,h)));
    Store clone(1,128); Handle cloned=0; status(clone.create(d,cloned),Status::Ok);
    status(clone.importSnapshot(cloned,lost),Status::Ok); CHECK(sameSnapshot(lost,snapshot(clone,cloned)));
    status(clone.restore(cloned),Status::Ok); bytesEqual(read(clone,cloned),original,"new store semantic restore");
    status(store.lose(h),Status::Ok);
    const auto cloneBefore = snapshot(clone,cloned);
    // Handles are store-local, so use two separate objects in one test store
    // to check a lost source and a lost destination independently.
    Store two(2,256); auto callerOwned=original;
    const Handle input=create(two,d,callerOwned), output=create(two,d,pattern(d,7));
    callerOwned.assign(callerOwned.size(),0); bytesEqual(read(two,input),original,"caller overwrite cannot change owned pixels");
    const auto outputBefore=snapshot(two,output);status(two.lose(input),Status::Ok);
    status(two.copy(output,input,{0,0,4,3},{0,0,4,3}),Status::Lost);
    CHECK(sameSnapshot(outputBefore,snapshot(two,output)));
    status(two.restore(input),Status::Ok);status(two.lose(output),Status::Ok);
    const auto outputLost=snapshot(two,output);
    status(two.nearest(output,input,{0,0,4,3},{0,0,4,3}),Status::Lost);
    CHECK(sameSnapshot(outputLost,snapshot(two,output)));
    CHECK(sameSnapshot(cloneBefore,snapshot(clone,cloned)));
    status(store.destroy(h),Status::Ok); status(store.restore(h),Status::InvalidHandle);
    Handle previous=h;
    for(int round=0;round<256;++round) {
        const Handle next=create(store,d,original); CHECK(next!=previous);
        status(store.replaceBytes(previous,original),Status::InvalidHandle);
        status(store.lose(next),Status::Ok); status(store.restore(next),Status::Ok);
        bytesEqual(read(store,next),original,"256 allocation/loss/restore/release cycles");
        status(store.destroy(next),Status::Ok); previous=next;
        CHECK(store.liveCount()==0 && store.activeBytes()==0 && store.savedBytes()==0);
        ++totals.lifecycleCycles;
    }
    status(clone.lose(cloned),Status::Ok); status(clone.destroy(cloned),Status::Ok);
    CHECK(clone.liveCount()==0 && clone.activeBytes()==0 && clone.savedBytes()==0);
}

void randomizedPixels() {
    Random rng{pixelSeed};
    for(int index=0;index<1024;++index) {
        const bool alias=rng.below(5)==0, scale=rng.below(2)==0;
        const Format sf=rng.below(2)==0?Format::RGB565LE:Format::BGRA8;
        const int sw=1+rng.below(33), sh=1+rng.below(25), sp=rng.below(8);
        const Desc sd=description(sw,sh,sf,sp);
        Desc dd=sd;
        if(!alias){const int dw=1+rng.below(33),dh=1+rng.below(25);
            const Format df=rng.below(2)==0?Format::RGB565LE:Format::BGRA8;
            const int dp=rng.below(8);dd=description(dw,dh,df,dp);}
        std::vector<uint8_t> source(sd.stride*size_t(sd.height)), destination(dd.stride*size_t(dd.height));
        for(auto& byte:source) byte=uint8_t(rng.next());
        if(alias) destination=source; else for(auto& byte:destination) byte=uint8_t(rng.next());
        Rect sr{rng.below(45)-8,rng.below(37)-8,1+rng.below(40),1+rng.below(32)};
        Rect dr{rng.below(45)-8,rng.below(37)-8,sr.width,sr.height};
        if(scale){dr.width=1+rng.below(45);dr.height=1+rng.below(35);}
        Store store;const Handle a=create(store,sd,source), b=alias?a:create(store,dd,destination);
        transferCase(store,b,a,sd,dd,source,destination,sr,dr,scale,"pixel seed 0x5eed565b6a8a0001 case "+std::to_string(index));
        if(!alias) bytesEqual(read(store,a),source,"random source immutable");
        ++totals.randomPixelCases;
    }
    totals.pixelRngFinal=rng.state;
}

void randomizedLifecycle() {
    struct Model { Handle handle; bool live,lost; Desc desc; std::vector<uint8_t> bytes; };
    Store store(8,4096);Random rng{lifeSeed};std::vector<Model> history;
    for(int event=0;event<2048;++event) {
        const int operation=history.empty()?0:rng.below(6);
        size_t live=0;for(const auto& m:history)if(m.live)++live;
        if(operation==0) {
            const int width=1+rng.below(5),height=1+rng.below(5);
            const Format format=rng.below(2)==0?Format::RGB565LE:Format::BGRA8;
            const int padding=rng.below(5);const Desc d=description(width,height,format,padding);
            Handle out=0;status(store.create(d,out),live==8?Status::Capacity:Status::Ok);
            if(live<8){auto bytes=pattern(d,unsigned(event));status(store.replaceBytes(out,bytes),Status::Ok);
                history.push_back({out,true,false,d,std::move(bytes)});}else CHECK(out==0);
        } else {
            Model& m=history[size_t(rng.below(int(history.size())))];
            const Status invalid=m.live?Status::Ok:Status::InvalidHandle;
            if(operation==1){status(store.destroy(m.handle),invalid);if(m.live){m.live=false;m.lost=false;}}
            else if(operation==2){status(store.lose(m.handle),!m.live?invalid:(m.lost?Status::Lost:Status::Ok));if(m.live)m.lost=true;}
            else if(operation==3){status(store.restore(m.handle),!m.live?invalid:(m.lost?Status::Ok:Status::AlreadyReady));if(m.live)m.lost=false;}
            else if(operation==4){auto bytes=pattern(m.desc,unsigned(event));
                status(store.replaceBytes(m.handle,bytes),!m.live?invalid:(m.lost?Status::Lost:Status::Ok));if(m.live&&!m.lost)m.bytes=bytes;}
            else {Snapshot out{};status(store.snapshot(m.handle,out),invalid);
                if(m.live)CHECK(out.lost==m.lost&&sameDescription(out.desc,m.desc)&&out.bytes==m.bytes);}
        }
        size_t active=0,saved=0;live=0;
        for(const auto& m:history)if(m.live){++live;if(m.lost)saved+=m.bytes.size();else active+=m.bytes.size();}
        CHECK(store.liveCount()==live&&store.activeBytes()==active&&store.savedBytes()==saved);
        ++totals.randomLifeEvents;
    }
    for(auto& m:history)if(m.live)status(store.destroy(m.handle),Status::Ok);
    CHECK(store.liveCount()==0&&store.activeBytes()==0&&store.savedBytes()==0);
    totals.lifeRngFinal=rng.state;
}

void persistedOracleFixture() {
    const Desc sd=description(3,2,Format::BGRA8,5),dd=description(5,4,Format::BGRA8,3);
    const auto source=pattern(sd,5),before=pattern(dd,19);
    const Rect sr{0,0,3,2},dr{-1,0,5,4};
    const auto expected=scalar_oracle::expected(source,sd,before,dd,sr,dr);
    Store store;const Handle a=create(store,sd,source),b=create(store,dd,before);
    status(store.nearest(b,a,sr,dr),Status::Ok);++totals.nearestCases;
    const auto actual=read(store,b);bytesEqual(actual,expected,"persisted oracle fixture");
    writeFile("fixture_source.bin",source);writeFile("fixture_destination_before.bin",before);
    writeFile("fixture_expected.bin",expected);writeFile("fixture_actual.bin",actual);
    std::ofstream meta(evidenceDirectory+"/fixture.json");CHECK(bool(meta));
    meta<<"{\n  \"source\":"<<descText(sd)<<",\n  \"destination\":"<<descText(dd)
        <<",\n  \"source_rect\":"<<rectText(sr)<<",\n  \"destination_rect\":"<<rectText(dr)
        <<",\n  \"operation\":\"nearest\",\n  \"expected_bytes\":"<<expected.size()
        <<",\n  \"byte_mismatches\":0,\n  \"oracle\":\"source-cell footprint / ceiling arithmetic\"\n}\n";
    CHECK(bool(meta));
}
std::string jsonEscape(const std::string& value) {
    std::string out;for(char c:value){if(c=='"'||c=='\\')out+='\\';if(c=='\n')out+="\\n";else out+=c;}return out;
}
void writeSummary(bool ok,const std::string& error,double elapsed) {
    std::ofstream out(evidenceDirectory+"/summary.json");
    if(!out)throw std::runtime_error("cannot write summary.json");
    out<<"{\n  \"passed\":"<<(ok?"true":"false")<<",\n  \"scope\":\"self-authored CPU offscreen surface component\",\n"
        <<"  \"pixel_seed_hex\":\"0x5eed565b6a8a0001\",\n  \"lifecycle_seed_hex\":\"0x1a11ce005eed0002\",\n"
        <<"  \"groups_passed\":"<<totals.groupsPassed<<",\n  \"assertions\":"<<totals.assertions
        <<",\n  \"bytes_compared\":"<<totals.bytesCompared<<",\n  \"copy_cases\":"<<totals.copyCases
        <<",\n  \"nearest_cases\":"<<totals.nearestCases<<",\n  \"negative_checks\":"<<totals.negativeChecks
        <<",\n  \"random_pixel_cases\":"<<totals.randomPixelCases<<",\n  \"random_lifecycle_events\":"<<totals.randomLifeEvents
        <<",\n  \"exhaustive_RGB565_values\":"<<totals.exhaustiveValues
        <<",\n  \"exhaustive_RGB555_values\":"<<totals.exhaustiveRGB555Values
        <<",\n  \"allocation_loss_restore_release_cycles\":"<<totals.lifecycleCycles<<",\n"
        <<"  \"pixel_rng_final_hex\":\"0x"<<std::hex<<totals.pixelRngFinal<<"\",\n"
        <<"  \"lifecycle_rng_final_hex\":\"0x"<<totals.lifeRngFinal<<"\",\n"<<std::dec
        <<"  \"elapsed_ms\":"<<elapsed<<",\n  \"error\":\""<<jsonEscape(error)<<"\",\n"
        <<"  \"GPU_tested\":false,\n  \"UI_or_input_used\":false,\n  \"audio_devices_used\":false,\n"
        <<"  \"game_resources_or_instructions_loaded\":false,\n  \"binary_translator_or_emulator_used\":false\n}\n";
    if(!out)throw std::runtime_error("cannot finish summary.json");
}
}

int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"usage: surface-tests EXISTING_EVIDENCE_DIRECTORY\n";return 2;}
    evidenceDirectory=argv[1];const auto start=std::chrono::steady_clock::now();
    const std::vector<std::pair<std::string,std::function<void()>>> groups{
        {"exhaustive formats and canonical colors",exhaustiveFormats},
        {"stride padding and rectangle clipping",clippingAndPadding},
        {"overlap copies",overlappingCopies},{"nearest scaling and manual anchor",nearestScaling},
        {"negative inputs and no mutation",rejectedInputs},{"loss restoration and handle lifecycle",lifecycle},
        {"1024 deterministic randomized pixel cases",randomizedPixels},
        {"2048 deterministic lifecycle events",randomizedLifecycle},{"persisted independent oracle fixture",persistedOracleFixture}};
    try {
        for(const auto& group:groups){const size_t before=totals.assertions;group.second();++totals.groupsPassed;
            std::cout<<"PASS "<<group.first<<" ("<<totals.assertions-before<<" assertions)\n";}
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        writeSummary(true,"",ms);
        std::cout<<"PASS all: "<<totals.groupsPassed<<" groups, "<<totals.assertions<<" assertions, "
            <<totals.bytesCompared<<" bytes compared; CPU/offscreen only\n";return 0;
    } catch(const std::exception& error) {
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::cerr<<"FAIL "<<error.what()<<"\n";
        try{writeSummary(false,error.what(),ms);}catch(const std::exception& writeError){std::cerr<<writeError.what()<<"\n";}
        return 1;
    }
}
