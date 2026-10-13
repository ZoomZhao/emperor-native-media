#include "candidate_import.hpp"
#include "../file_io/include/owned_reader.hpp"
#include "../file_io/include/city19_goal_archive.hpp"
#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <new>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace game_lab {
namespace {
struct ReadOnlyFile {
    int fd;
    explicit ReadOnlyFile(const char* path)
        : fd(::open(path,O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW)) {}
    ~ReadOnlyFile() { if (fd >= 0) ::close(fd); }
};
bool sameFile(const struct stat& a, const struct stat& b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_size == b.st_size &&
           a.st_mtimespec.tv_sec == b.st_mtimespec.tv_sec &&
           a.st_mtimespec.tv_nsec == b.st_mtimespec.tv_nsec;
}
std::string quotedFilename(const std::string& name) {
    constexpr char hex[] = "0123456789abcdef";
    std::string out = "\"";
    const auto escapedByte = [&](uint8_t byte) {
        out += "\\x"; out += hex[byte >> 4]; out += hex[byte & 15];
    };
    const auto escapedCodepoint = [&](uint32_t codepoint) {
        out += "\\u";
        for (int shift = 12; shift >= 0; shift -= 4) out += hex[(codepoint >> shift) & 15];
    };
    for (size_t i = 0; i < name.size();) {
        const auto first = static_cast<uint8_t>(name[i]);
        if (first < 128) {
            if (first == '\n') out += "\\n";
            else if (first == '\r') out += "\\r";
            else if (first == '\t') out += "\\t";
            else if (first == '\\' || first == '"') { out += '\\'; out += char(first); }
            else if (first < 32 || first == 127) escapedCodepoint(first);
            else out += char(first);
            ++i; continue;
        }
        const size_t length = first >= 0xc2 && first <= 0xdf ? 2 :
            first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
        uint32_t codepoint = length ? first & ((1u << (7 - length)) - 1) : 0;
        bool valid = length && length <= name.size() - i;
        for (size_t n = 1; valid && n < length; ++n) {
            const auto next = static_cast<uint8_t>(name[i + n]);
            if ((next & 0xc0) != 0x80) valid = false;
            else codepoint = (codepoint << 6) | (next & 63);
        }
        const uint32_t minimum = length == 2 ? 128 : length == 3 ? 2048 : 65536;
        valid = valid && codepoint >= minimum && codepoint <= 0x10ffff &&
            !(codepoint >= 0xd800 && codepoint <= 0xdfff);
        if (!valid) { escapedByte(first); ++i; continue; }
        // Unicode paragraph/C1 and directional controls can also affect the
        // structure of a plain-text App report. Ordinary Unicode names survive.
        if ((codepoint >= 0x80 && codepoint <= 0x9f) || codepoint == 0x2028 || codepoint == 0x2029 ||
            codepoint == 0x061c || codepoint == 0x200e || codepoint == 0x200f ||
            (codepoint >= 0x202a && codepoint <= 0x202e) ||
            (codepoint >= 0x2066 && codepoint <= 0x2069)) escapedCodepoint(codepoint);
        else out.append(name,i,length);
        i += length;
    }
    out += '"'; return out;
}
}
void CandidateImportController::clear() { selectedPath_.clear(); result_ = {}; }
void CandidateImportController::beginSelection() { clear(); result_.state = CandidateState::Selecting; }
void CandidateImportController::cancelSelection() { clear(); result_.state = CandidateState::Cancelled; }
void CandidateImportController::retry() { inspectFile(selectedPath_); }
void CandidateImportController::inspectFile(std::string path) {
    result_ = {};
    selectedPath_ = std::move(path);
    try {
        if (selectedPath_.empty() || selectedPath_.find('\0') != std::string::npos ||
            !std::filesystem::path(selectedPath_).is_absolute()) {
            selectedPath_.clear(); result_.state = CandidateState::ReadFailure; return;
        }
        ReadOnlyFile file(selectedPath_.c_str());
        struct stat before{}, after{};
        if (file.fd < 0 || ::fstat(file.fd,&before) || !S_ISREG(before.st_mode) || before.st_size < 0) {
            result_.state = CandidateState::ReadFailure; return;
        }
        result_.file_size_known = true;
        result_.file_size = static_cast<uint64_t>(before.st_size);
        if (result_.file_size > kCandidateInputLimit) {
            result_.state = CandidateState::BudgetExceeded; result_.detail = "file_input_limit"; return;
        }
        std::vector<uint8_t> input(static_cast<size_t>(before.st_size));
        while (result_.read_bytes < input.size()) {
            const ssize_t n = ::read(file.fd,input.data() + result_.read_bytes,
                                     input.size() - result_.read_bytes);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) { result_.state = CandidateState::ReadFailure; return; }
            result_.read_bytes += static_cast<uint64_t>(n);
        }
        uint8_t extra = 0;
        ssize_t tail;
        do { tail = ::read(file.fd,&extra,1); } while (tail < 0 && errno == EINTR);
        if (tail != 0 || ::fstat(file.fd,&after) || !sameFile(before,after)) {
            result_.state = CandidateState::ReadFailure; return;
        }
        result_.file_read = true;
        const emperor::file_io::FrameProfile profile{
            kCandidateContainerMarker,kCandidateCompressedMarker,kCandidateRawMarker,
            kCandidateBlockLimit,kCandidateBlockLimit,kCandidateInputLimit,
            kCandidateDecodedLimit,kCandidateFrameLimit};
        emperor::file_io::OwnedFramedReader reader(std::move(input),profile);
        std::vector<uint8_t> decoded, chunk;
        decoded.reserve(kCandidateDecodedLimit);
        for (size_t event = 0; event < kCandidateFrameLimit + 2; ++event) {
            const auto frame = reader.next(chunk);
            result_.input_consumed = frame.input_offset;
            result_.validated_frames = frame.blocks_completed;
            using emperor::file_io::ReadStatus;
            if (frame.status == ReadStatus::Frame) {
                decoded.insert(decoded.end(),chunk.begin(),chunk.end());
                result_.encoded_tail_bytes += frame.codec.trailing_input;
                continue;
            }
            if (frame.status == ReadStatus::End) { result_.decoding_complete = true; break; }
            result_.detail = emperor::file_io::read_status_name(frame.status);
            if (frame.status == ReadStatus::InputLimit || frame.status == ReadStatus::OutputLimit ||
                frame.status == ReadStatus::BlockLimit) result_.state = CandidateState::BudgetExceeded;
            else if (frame.status == ReadStatus::UnsupportedHeader || frame.status == ReadStatus::UnknownMarker)
                result_.state = CandidateState::Unsupported;
            else if (frame.status == ReadStatus::NoMemory)
                result_.state = CandidateState::ResourceFailure;
            else result_.state = CandidateState::DecodeFailure;
            // Earlier valid blocks stay private after any later failure.
            return;
        }
        if (!result_.decoding_complete) { result_.state = CandidateState::DecodeFailure; return; }
        result_.decoded_bytes = decoded.size();
        result_.unparsed_decoded = decoded.size();
        result_.remaining_suffix = decoded.size();
        if (decoded.size() < kCandidateGoalStart) {
            result_.state = CandidateState::DecodedOnly;
            result_.detail = "before_existing_goal_entry"; return;
        }
        try {
            using namespace emperor::file_io::research;
            GoalLimits limits;
            limits.checkpoint_items = 1; // Reuse one existing checkpoint; no parser extension.
            const auto walk = walk_city19_housing_goals(
                {decoded.data(),std::min(decoded.size(),kCandidateParseWindow)},limits);
            result_.parsed_start = kCandidateGoalStart;
            result_.parsed_end = walk.end;
            result_.parsed_bytes = walk.end - kCandidateGoalStart;
            result_.unchecked_prefix = kCandidateGoalStart;
            result_.remaining_suffix = decoded.size() - walk.end;
            result_.unparsed_decoded = decoded.size() - result_.parsed_bytes;
            result_.known_items = walk.known_items;
            result_.completed_members = walk.completed_members;
            result_.unsupported_next_class = walk.pending.present;
            result_.detail = walk.stop_reason;
            result_.state = CandidateState::PartialParsed;
        } catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& error) {
            result_.state = CandidateState::ParseFailure; result_.detail = error.what();
        }
    } catch (const std::bad_alloc&) { result_.state = CandidateState::ResourceFailure; }
    catch (const std::exception&) { result_.state = CandidateState::ReadFailure; }
}
std::string formatCandidateImport(const CandidateImportController& controller) {
    const auto& r = controller.result();
    std::ostringstream out;
    switch (r.state) {
        case CandidateState::Idle: out << "未选择候选文件"; break;
        case CandidateState::Selecting: out << "等待文件选择；已清空旧检查结果"; break;
        case CandidateState::Cancelled: out << "已取消选择；没有保留旧检查结果"; break;
        case CandidateState::ReadFailure: out << "文件读取失败（路径无效、非普通文件或读取不完整）"; break;
        case CandidateState::BudgetExceeded: out << "预算超限；没有发布部分解码内容"; break;
        case CandidateState::Unsupported: out << "容器头或块标识不受当前配置支持"; break;
        case CandidateState::DecodeFailure: out << "解码失败或数据截断；没有发布部分解码内容"; break;
        case CandidateState::DecodedOnly: out << "帧序列解码完成；数据不足以进入既有局部检查点"; break;
        case CandidateState::PartialParsed:
            out << "帧序列解码完成；仅核对了局部结构";
            if (r.unsupported_next_class) out << "，后续类型不受支持";
            break;
        case CandidateState::ParseFailure: out << "帧序列解码完成；局部结构不支持或被截断"; break;
        case CandidateState::ResourceFailure: out << "资源分配失败；检查未完成"; break;
    }
    if (!controller.selectedPath().empty())
        out << "\n文件：" << quotedFilename(std::filesystem::path(controller.selectedPath()).filename().string());
    if (r.file_size_known) out << "\n文件大小：" << r.file_size << " 字节";
    out << "\n文件读取：" << (r.file_read ? "完整" : "未完成") << "，已读 " << r.read_bytes << " 字节";
    if (r.file_read) out << "\n帧消费：" << r.input_consumed << " 字节，文件剩余 "
        << r.file_size - r.input_consumed << " 字节；已验证 " << r.validated_frames << " 块";
    out << "\n解码完成：" << (r.decoding_complete ? "是" : "否") << "；可检查解码数据 " << r.decoded_bytes << " 字节";
    if (r.decoding_complete) {
        out << "\n局部核对范围：[" << r.parsed_start << ", " << r.parsed_end << ")，共 " << r.parsed_bytes << " 字节";
        out << "\n未核对前缀 " << r.unchecked_prefix << " 字节；范围后剩余 " << r.remaining_suffix << " 字节";
        out << "\n合计未解析解码数据：" << r.unparsed_decoded << " 字节；已知对象 " << r.known_items;
        out << "\n压缩帧内未解码尾部：" << r.encoded_tail_bytes << " 字节";
    }
    out << "\n预算：输入1 MiB，解码256 KiB，64块，单块36144字节，局部窗口16 KiB";
    out << "\n仅按既有静态候选配置检查，不证明文件原版身份或完整存档兼容。";
    out << "\n未载入城市；未写入或修改候选文件。";
    return out.str();
}
} // namespace game_lab
