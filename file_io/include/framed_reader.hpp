#ifndef EMPEROR_FILE_IO_FRAMED_READER_HPP
#define EMPEROR_FILE_IO_FRAMED_READER_HPP

#include "strict_zlib.h"
#include <cstdint>
#include <istream>
#include <vector>

namespace emperor::file_io {

struct FrameProfile {
    uint32_t container_marker;
    uint32_t compressed_marker;
    uint32_t raw_marker;
    size_t max_encoded_block;
    size_t max_decoded_block;
    uint64_t max_input_bytes;
    uint64_t max_output_bytes;
    uint64_t max_blocks;
};

enum class ReadStatus {
    Frame, End, InvalidProfile, UnsupportedHeader, UnknownMarker,
    Truncated, IoError, InputLimit, OutputLimit, BlockLimit,
    CodecError, NoMemory
};
enum class StartPosition { ContainerHeader, RequiredFrame };

struct FrameResult {
    ReadStatus status = ReadStatus::InvalidProfile;
    bool compressed = false;
    uint32_t opaque_word = 0; // Preserved metadata, not an output length.
    uint64_t frame_start = 0;
    uint64_t input_offset = 0;
    uint64_t blocks_completed = 0;
    uint64_t total_output = 0;
    size_t output_available = 0;
    emfr_decode_result codec{};
};

// Strict framing policy supplied explicitly by caller; no original profile here.
// Native EOF is recognized only at a complete frame boundary. An empty frame is
// a Frame event, not End. Failures are sticky and never modify caller output.
// The source and its backing storage must outlive this reader and remain valid
// across every next() call. Backing storage must not alias any output vector
// passed to next(): a successful output swap can free that vector old storage.
class FramedReader {
public:
    FramedReader(std::istream &source, FrameProfile profile,
                 StartPosition start = StartPosition::ContainerHeader);
    FrameResult next(std::vector<uint8_t> &output);
private:
    std::istream &source_;
    FrameProfile profile_;
    uint64_t input_offset_ = 0, total_output_ = 0, blocks_ = 0, frame_start_ = 0;
    bool opened_ = false, ended_ = false, failed_ = false;
    bool require_first_frame_ = false;
    FrameResult failure_result_{};
    FrameResult state(ReadStatus status) const;
    ReadStatus exact(uint8_t *destination, size_t count, bool boundary_eof);
    FrameResult fail(ReadStatus status);
};

const char *read_status_name(ReadStatus status);
}
#endif
