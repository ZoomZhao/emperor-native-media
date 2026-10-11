#include "framed_reader.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <streambuf>
#include <string>
#include <vector>
extern "C" uint64_t emfr_test_codec_calls(void);

namespace {
constexpr uint64_t harness_byte_limit = 64 * 1024 * 1024;
constexpr uint64_t harness_block_limit = 100000;

uint64_t number(const char *text) {
    const std::string value(text);
    if (value.empty() || value.front() == '-' || value.front() == '+')
        throw std::invalid_argument("unsigned integer required");
    size_t end = 0;
    const uint64_t result = std::stoull(value, &end, 0);
    if (end != value.size()) throw std::invalid_argument("invalid integer");
    return result;
}
uint32_t word(const char *text) {
    const uint64_t value = number(text);
    if (value > std::numeric_limits<uint32_t>::max()) throw std::out_of_range("marker");
    return static_cast<uint32_t>(value);
}
size_t budget(const char *text) {
    const uint64_t value = number(text);
    if (value > harness_byte_limit) throw std::out_of_range("harness byte budget");
    return static_cast<size_t>(value);
}
void hex(const uint8_t *bytes, size_t size) {
    constexpr char digits[] = "0123456789abcdef";
    std::cout << '"';
    for (size_t i = 0; i < size; ++i)
        std::cout << digits[bytes[i] >> 4] << digits[bytes[i] & 15];
    std::cout << '"';
}
void codec_json(const emfr_decode_result &r) {
    std::cout << "{\"status\":\"" << emfr_status_name(r.status)
              << "\",\"codec_status\":" << r.codec_status
              << ",\"input_consumed\":" << r.input_consumed
              << ",\"output_produced\":" << r.output_produced
              << ",\"output_available\":" << r.output_available
              << ",\"trailing_input\":" << r.trailing_input << '}';
}
std::vector<uint8_t> load(const char *path) {
    std::ifstream source(path, std::ios::binary | std::ios::ate);
    if (!source) throw std::runtime_error("fixture open failed");
    const auto length = source.tellg();
    if (length < 0 || static_cast<uint64_t>(length) > harness_byte_limit)
        throw std::out_of_range("fixture length");
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    source.seekg(0);
    if (!bytes.empty()) source.read(reinterpret_cast<char *>(bytes.data()), length);
    if (!source) throw std::runtime_error("fixture read failed");
    return bytes;
}
// Transport fragmentation is implemented through streambuf's get area, whose
// standard xsgetn retries underflow until the requested count or true EOF.
class ChunkedBuffer final : public std::streambuf {
public:
    explicit ChunkedBuffer(std::vector<uint8_t> &bytes) : bytes_(bytes) {
        if (const char *value = std::getenv("EMFR_TEST_CHUNK_SIZE")) {
            chunk_ = budget(value);
            if (!chunk_) throw std::invalid_argument("chunk size must be positive");
        }
        if (const char *value = std::getenv("EMFR_TEST_FAIL_AFTER")) fail_after_ = number(value);
    }
protected:
    int_type underflow() override {
        if (next_ >= fail_after_) throw std::runtime_error("synthetic transport failure");
        if (next_ == bytes_.size()) return traits_type::eof();
        const size_t count = static_cast<size_t>(std::min<uint64_t>(
            std::min(bytes_.size() - next_, chunk_), fail_after_ - next_));
        char *begin = reinterpret_cast<char *>(bytes_.data() + next_);
        setg(begin, begin, begin + count);
        next_ += count;
        return traits_type::to_int_type(*begin);
    }
private:
    std::vector<uint8_t> &bytes_;
    size_t next_ = 0, chunk_ = static_cast<size_t>(harness_byte_limit);
    uint64_t fail_after_ = std::numeric_limits<uint64_t>::max();
};
bool same_codec(const emfr_decode_result &a, const emfr_decode_result &b) {
    return a.status == b.status && a.codec_status == b.codec_status &&
           a.input_consumed == b.input_consumed && a.output_produced == b.output_produced &&
           a.output_available == b.output_available && a.trailing_input == b.trailing_input;
}
bool same_frame(const emperor::file_io::FrameResult &a, const emperor::file_io::FrameResult &b) {
    return a.status == b.status && a.compressed == b.compressed &&
           a.opaque_word == b.opaque_word && a.frame_start == b.frame_start &&
           a.input_offset == b.input_offset && a.blocks_completed == b.blocks_completed &&
           a.total_output == b.total_output && a.output_available == b.output_available &&
           same_codec(a.codec, b.codec);
}
int api() {
    uint8_t token = 0xa5;
    const emfr_decode_result cases[] = {
        emfr_decode_zlib(nullptr, 1, &token, 1, {1, 1}),
        emfr_decode_zlib(&token, 1, nullptr, 1, {1, 1}),
        emfr_decode_zlib(&token, std::numeric_limits<size_t>::max(), &token, 1,
                         {std::numeric_limits<size_t>::max(), 1}),
        emfr_decode_zlib(&token, 1, &token, std::numeric_limits<unsigned>::max(),
                         {1, std::numeric_limits<unsigned>::max()}),
        emfr_decode_zlib(nullptr, 0, nullptr, 0, {0, 0})
    };
    std::cout << "{\"mode\":\"api\",\"token_unchanged\":" << (token == 0xa5)
              << ",\"results\":[";
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (i) std::cout << ',';
        codec_json(cases[i]);
    }
    std::cout << "]}\n";
    return 0;
}
int codec(int argc, char **argv) {
    if (argc != 6) throw std::invalid_argument("codec PATH CAPACITY MAX_ENCODED MAX_DECODED");
    const auto bytes = load(argv[2]);
    const size_t capacity = budget(argv[3]);
    const emfr_decode_limits limits{budget(argv[4]), budget(argv[5])};
    constexpr size_t guard = 32;
    std::vector<uint8_t> destination(capacity + 2 * guard, 0xa5);
    const auto r = emfr_decode_zlib(bytes.data(), bytes.size(), destination.data() + guard,
                                   capacity, limits);
    const bool guards = std::all_of(destination.begin(), destination.begin() + guard,
                                   [](uint8_t c) { return c == 0xa5; }) &&
                        std::all_of(destination.end() - guard, destination.end(),
                                   [](uint8_t c) { return c == 0xa5; });
    const bool unchanged = std::all_of(destination.begin(), destination.end(),
                                      [](uint8_t c) { return c == 0xa5; });
    std::cout << "{\"mode\":\"codec\",\"result\":";
    codec_json(r);
    std::cout << ",\"guards_ok\":" << guards << ",\"destination_unchanged\":" << unchanged
              << ",\"output_hex\":";
    if (r.status == EMFR_OK && r.output_available <= capacity)
        hex(destination.data() + guard, r.output_available);
    else hex(nullptr, 0);
    std::cout << "}\n";
    return 0;
}
int frames(int argc, char **argv, bool headerless) {
    using namespace emperor::file_io;
    if (argc != 11) throw std::invalid_argument(
        "frames PATH HEADER COMPRESSED RAW MAX_ENCODED MAX_DECODED MAX_INPUT MAX_OUTPUT MAX_BLOCKS");
    const FrameProfile profile{word(argv[3]), word(argv[4]), word(argv[5]), budget(argv[6]),
                               budget(argv[7]), budget(argv[8]), budget(argv[9]), number(argv[10])};
    if (profile.max_blocks > harness_block_limit) throw std::out_of_range("harness block budget");
    auto bytes = load(argv[2]);
    ChunkedBuffer buffer(bytes);
    std::istream source(&buffer);
    if (std::getenv("EMFR_TEST_STREAM_EXCEPTIONS"))
        source.exceptions(std::ios::badbit | std::ios::failbit);
    FramedReader reader(source, profile, headerless ? StartPosition::RequiredFrame : StartPosition::ContainerHeader);
    bool first = true;
    std::cout << "{\"mode\":\"" << (headerless ? "blocks" : "frames") << "\",\"events\":[";
    for (uint64_t i = 0; i <= profile.max_blocks + 1; ++i) {
        const std::vector<uint8_t> sentinel{0xa5, 0x5a, 0xc3, 0x3c};
        auto output = sentinel;
        const auto r = reader.next(output);
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"status\":\"" << read_status_name(r.status)
                  << "\",\"compressed\":" << r.compressed
                  << ",\"opaque_word\":" << r.opaque_word
                  << ",\"frame_start\":" << r.frame_start
                  << ",\"input_offset\":" << r.input_offset
                  << ",\"blocks_completed\":" << r.blocks_completed
                  << ",\"total_output\":" << r.total_output
                  << ",\"output_available\":" << r.output_available
                  << ",\"destination_unchanged\":" << (output == sentinel)
                  << ",\"codec_calls\":" << emfr_test_codec_calls()
                  << ",\"codec\":";
        codec_json(r.codec);
        std::cout << ",\"output_hex\":";
        if (r.status == ReadStatus::Frame) hex(output.data(), output.size());
        else hex(nullptr, 0);
        std::cout << '}';
        if (r.status != ReadStatus::Frame) {
            output = sentinel;
            const auto repeated = reader.next(output);
            std::cout << "],\"sticky_terminal\":" << (same_frame(r, repeated) && output == sentinel)
                      << "}\n";
            return 0;
        }
    }
    throw std::runtime_error("harness event budget exceeded");
}
}

int main(int argc, char **argv) {
    std::cout << std::boolalpha;
    try {
        if (argc == 2 && std::string(argv[1]) == "--version") {
            std::cout << "{\"zlib_runtime_version\":" << std::quoted(emfr_zlib_runtime_version()) << "}\n";
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "codec") return codec(argc, argv);
        if (argc == 2 && std::string(argv[1]) == "api") return api();
        if (argc > 1 && std::string(argv[1]) == "frames") return frames(argc, argv, false);
        if (argc > 1 && std::string(argv[1]) == "blocks") return frames(argc, argv, true);
        throw std::invalid_argument("expected codec, frames, blocks or --version");
    } catch (const std::exception &e) {
        std::cerr << "fixture driver: " << e.what() << '\n';
        return 2;
    }
}
