#include "framed_reader.hpp"
#include <algorithm>
#include <climits>
#include <limits>
#include <new>

namespace emperor::file_io {
namespace {
uint32_t le32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
}

FramedReader::FramedReader(std::istream &source, FrameProfile profile, StartPosition start)
    : source_(source), profile_(profile), opened_(start == StartPosition::RequiredFrame),
      require_first_frame_(start == StartPosition::RequiredFrame) {
    if (profile.container_marker == profile.compressed_marker ||
        profile.container_marker == profile.raw_marker ||
        profile.compressed_marker == profile.raw_marker ||
        profile.max_encoded_block > UINT_MAX ||
        profile.max_decoded_block >= UINT_MAX ||
        profile.max_encoded_block > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max())) {
        failed_ = true;
    }
}

FrameResult FramedReader::state(ReadStatus status) const {
    FrameResult r;
    r.status = status; r.input_offset = input_offset_;
    r.frame_start = frame_start_;
    r.blocks_completed = blocks_; r.total_output = total_output_;
    return r;
}
FrameResult FramedReader::fail(ReadStatus status) {
    failed_ = true;
    failure_result_ = state(status);
    return failure_result_;
}
ReadStatus FramedReader::exact(uint8_t *destination, size_t count, bool boundary_eof) {
    if (count > profile_.max_input_bytes - input_offset_) {
        if (boundary_eof) {
            try {
                if (source_.peek() == std::char_traits<char>::eof()) {
                    if (source_.bad() || !source_.eof()) return ReadStatus::IoError;
                    return ReadStatus::End;
                }
            } catch (...) {
                if (source_.eof() && !source_.bad()) return ReadStatus::End;
                return ReadStatus::IoError;
            }
        }
        return ReadStatus::InputLimit;
    }
    if (!count) return ReadStatus::Frame;
    bool threw = false;
    try { source_.read(reinterpret_cast<char *>(destination), static_cast<std::streamsize>(count)); }
    catch (const std::ios_base::failure &) { threw = true; }
    catch (...) { return ReadStatus::IoError; }
    const std::streamsize got = source_.gcount();
    if (got < 0 || static_cast<uint64_t>(got) > count) return ReadStatus::IoError;
    input_offset_ += static_cast<uint64_t>(got);
    if (static_cast<size_t>(got) == count && !source_.bad()) return ReadStatus::Frame;
    if (source_.bad() || (threw && !source_.eof())) return ReadStatus::IoError;
    if (source_.eof()) return boundary_eof && got == 0 ? ReadStatus::End : ReadStatus::Truncated;
    return ReadStatus::IoError;
}

FrameResult FramedReader::next(std::vector<uint8_t> &output) {
    if (failed_) return failure_result_;
    if (ended_) return state(ReadStatus::End);
    try {
        uint8_t word[4];
        if (!opened_) {
            const auto status = exact(word, 4, false);
            if (status != ReadStatus::Frame) return fail(status);
            if (le32(word) != profile_.container_marker) return fail(ReadStatus::UnsupportedHeader);
            opened_ = true;
        }
        const uint64_t start = input_offset_;
        frame_start_ = start;
        auto status = exact(word, 4, !require_first_frame_ || blocks_ != 0);
        if (status == ReadStatus::End) { ended_ = true; return state(status); }
        if (status != ReadStatus::Frame) return fail(status);
        if (blocks_ >= profile_.max_blocks) return fail(ReadStatus::BlockLimit);
        const uint32_t marker = le32(word);
        const bool compressed = marker == profile_.compressed_marker;
        if (!compressed && marker != profile_.raw_marker) return fail(ReadStatus::UnknownMarker);
        status = exact(word, 4, false);
        if (status != ReadStatus::Frame) return fail(status);
        const size_t encoded_size = le32(word);
        if (encoded_size > profile_.max_encoded_block) return fail(ReadStatus::InputLimit);
        uint32_t opaque = 0;
        if (compressed) {
            status = exact(word, 4, false);
            if (status != ReadStatus::Frame) return fail(status);
            opaque = le32(word);
        }
        // Reject declared payload outside input budget before allocation or read.
        if (encoded_size > profile_.max_input_bytes - input_offset_) return fail(ReadStatus::InputLimit);
        const uint64_t remaining_output = profile_.max_output_bytes - total_output_;
        const size_t output_limit = static_cast<size_t>(std::min<uint64_t>(profile_.max_decoded_block, remaining_output));
        if (!compressed && encoded_size > output_limit) return fail(ReadStatus::OutputLimit);
        std::vector<uint8_t> encoded(encoded_size);
        status = exact(encoded.data(), encoded.size(), false);
        if (status != ReadStatus::Frame) return fail(status);
        std::vector<uint8_t> decoded;
        emfr_decode_result codec{};
        if (compressed) {
            decoded.resize(output_limit);
            codec = emfr_decode_zlib(encoded.data(), encoded.size(), decoded.data(), decoded.size(),
                                     {profile_.max_encoded_block, output_limit});
            if (codec.status != EMFR_OK) {
                const auto mapped = codec.status == EMFR_OUTPUT_LIMIT ? ReadStatus::OutputLimit :
                                    codec.status == EMFR_NO_MEMORY ? ReadStatus::NoMemory : ReadStatus::CodecError;
                FrameResult failure = fail(mapped);
                failure.frame_start = start; failure.compressed = true;
                failure.opaque_word = opaque; failure.codec = codec;
                failure_result_ = failure;
                return failure;
            }
            decoded.resize(codec.output_available);
        } else decoded.swap(encoded);
        total_output_ += decoded.size();
        ++blocks_;
        FrameResult result = state(ReadStatus::Frame);
        result.frame_start = start; result.compressed = compressed;
        result.opaque_word = opaque; result.output_available = decoded.size(); result.codec = codec;
        output.swap(decoded);
        return result;
    } catch (const std::bad_alloc &) { return fail(ReadStatus::NoMemory); }
    catch (...) { return fail(ReadStatus::IoError); }
}

const char *read_status_name(ReadStatus status) {
    switch (status) {
    case ReadStatus::Frame: return "frame";
    case ReadStatus::End: return "end";
    case ReadStatus::InvalidProfile: return "invalid_profile";
    case ReadStatus::UnsupportedHeader: return "unsupported_header";
    case ReadStatus::UnknownMarker: return "unknown_marker";
    case ReadStatus::Truncated: return "truncated";
    case ReadStatus::IoError: return "io_error";
    case ReadStatus::InputLimit: return "input_limit";
    case ReadStatus::OutputLimit: return "output_limit";
    case ReadStatus::BlockLimit: return "block_limit";
    case ReadStatus::CodecError: return "codec_error";
    case ReadStatus::NoMemory: return "no_memory";
    }
    return "unknown_status";
}
}
