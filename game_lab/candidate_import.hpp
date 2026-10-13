#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace game_lab {
constexpr uint32_t kCandidateContainerMarker = 4275878570u;
constexpr uint32_t kCandidateCompressedMarker = 2882400255u;
constexpr uint32_t kCandidateRawMarker = 2863386095u;
constexpr size_t kCandidateInputLimit = 1024 * 1024;
constexpr size_t kCandidateDecodedLimit = 256 * 1024;
constexpr size_t kCandidateBlockLimit = 36144;
constexpr size_t kCandidateFrameLimit = 64;
constexpr size_t kCandidateParseWindow = 16384;
constexpr size_t kCandidateGoalStart = 3355;

enum class CandidateState {
    Idle, Selecting, Cancelled, ReadFailure, BudgetExceeded, Unsupported,
    DecodeFailure, DecodedOnly, PartialParsed, ParseFailure, ResourceFailure
};
struct CandidateResult {
    CandidateState state = CandidateState::Idle;
    bool file_size_known = false, file_read = false, decoding_complete = false;
    uint64_t file_size = 0, read_bytes = 0, input_consumed = 0;
    uint64_t validated_frames = 0, decoded_bytes = 0, encoded_tail_bytes = 0;
    size_t parsed_start = 0, parsed_end = 0, parsed_bytes = 0;
    size_t unchecked_prefix = 0, remaining_suffix = 0, unparsed_decoded = 0;
    size_t known_items = 0, completed_members = 0;
    bool unsupported_next_class = false;
    std::string detail;
};
// Owns summary metadata only. It cannot receive or mutate a city, execute a
// game, write a file, or publish the private input/decoded payload.
class CandidateImportController {
public:
    void beginSelection();
    void cancelSelection();
    void inspectFile(std::string absolutePath);
    void retry();
    void clear();
    const CandidateResult& result() const { return result_; }
    const std::string& selectedPath() const { return selectedPath_; }
private:
    std::string selectedPath_;
    CandidateResult result_;
};
std::string formatCandidateImport(const CandidateImportController& controller);
} // namespace game_lab
