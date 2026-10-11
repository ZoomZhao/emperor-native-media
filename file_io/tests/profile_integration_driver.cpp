#include "owned_reader.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <stdexcept>

using namespace emperor::file_io;
namespace {
uint64_t number(const char *s) {
    const std::string value(s); size_t used = 0;
    if (value.empty() || value[0] == '-' || value[0] == '+') throw std::invalid_argument("unsigned argument");
    const uint64_t result = std::stoull(value, &used, 0);
    if (used != value.size()) throw std::invalid_argument("integer argument");
    return result;
}
uint32_t word(const char *s) {
    const uint64_t result = number(s);
    if (result > std::numeric_limits<uint32_t>::max()) throw std::out_of_range("marker");
    return static_cast<uint32_t>(result);
}
void hex(const std::vector<uint8_t> &output) {
    const char digits[] = "0123456789abcdef"; std::cout << '"';
    for (auto c : output) std::cout << digits[c >> 4] << digits[c & 15];
    std::cout << '"';
}
}
int main(int argc, char **argv) {
    try {
        if (argc != 11) throw std::invalid_argument("PATH HEADER COMP RAW ENCODED DECODED TOTAL_INPUT TOTAL_OUTPUT BLOCKS copy|move");
        FrameProfile p{word(argv[2]), word(argv[3]), word(argv[4]), static_cast<size_t>(number(argv[5])),
                       static_cast<size_t>(number(argv[6])), number(argv[7]), number(argv[8]), number(argv[9])};
        if (p.max_input_bytes > 16 * 1024 * 1024 || p.max_output_bytes > 16 * 1024 * 1024 ||
            p.max_blocks > 1024 || p.max_encoded_block > 36144 || p.max_decoded_block > 36144)
            throw std::out_of_range("synthetic harness budget");
        std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("fixture open");
        const auto size = file.tellg();
        if (size < 0 || static_cast<uint64_t>(size) > 16 * 1024 * 1024) throw std::out_of_range("fixture bytes");
        std::vector<uint8_t> input(static_cast<size_t>(size));
        file.seekg(0);
        if (!input.empty()) file.read(reinterpret_cast<char *>(input.data()), size);
        if (!file) throw std::runtime_error("fixture read");
        std::unique_ptr<OwnedFramedReader> reader;
        if (std::string(argv[10]) == "copy") reader = std::make_unique<OwnedFramedReader>(input, p);
        else if (std::string(argv[10]) == "move") reader = std::make_unique<OwnedFramedReader>(std::move(input), p);
        else throw std::invalid_argument("copy|move");
        std::cout << std::boolalpha << "{\"events\":["; bool comma = false;
        for (uint64_t n = 0; n < p.max_blocks + 2; ++n) {
            const auto previous = input; const auto result = reader->next(input);
            if (comma) std::cout << ','; comma = true;
            std::cout << "{\"status\":\"" << read_status_name(result.status)
                      << "\",\"input_offset\":" << result.input_offset << ",\"frame_start\":" << result.frame_start
                      << ",\"output_available\":" << result.output_available << ",\"blocks\":" << result.blocks_completed
                      << ",\"total_output\":" << result.total_output << ",\"unchanged\":" << (input == previous)
                      << ",\"opaque\":" << result.opaque_word << ",\"codec_status\":\"" << emfr_status_name(result.codec.status)
                      << "\",\"codec_consumed\":" << result.codec.input_consumed
                      << ",\"codec_tail\":" << result.codec.trailing_input << ",\"output_hex\":";
            if (result.status == ReadStatus::Frame) hex(input); else std::cout << "\"\"";
            std::cout << '}';
            if (result.status != ReadStatus::Frame) {
                const auto keep = input; const auto repeated = reader->next(input);
                std::cout << "],\"sticky\":" << (repeated.status == result.status && repeated.input_offset == result.input_offset &&
                    repeated.blocks_completed == result.blocks_completed && repeated.total_output == result.total_output && input == keep) << "}\n";
                return 0;
            }
        }
        throw std::runtime_error("harness event budget");
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 2; }
}
