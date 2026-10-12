#include "owned_reader.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace emperor::file_io;
namespace {
struct ReadOnlyFd {
    int value;
    explicit ReadOnlyFd(const char *path) : value(::open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC)) {
        if (value < 0) throw std::runtime_error(std::string("readonly open failed: ") + std::strerror(errno));
    }
    ~ReadOnlyFd() { ::close(value); }
};
uint64_t number(const char *s) {
    const std::string value(s); size_t used = 0;
    if (value.empty() || value[0]=='-' || value[0]=='+') throw std::invalid_argument("unsigned argument");
    auto n = std::stoull(value, &used, 0);
    if (used != value.size()) throw std::invalid_argument("integer argument");
    return n;
}
uint32_t word(const char *s) {
    auto n = number(s);
    if (n > UINT32_MAX) throw std::out_of_range("marker argument");
    return static_cast<uint32_t>(n);
}
std::string digest(const uint8_t *data, size_t size) {
    if (size > UINT_MAX) throw std::out_of_range("digest slice");
    uint8_t empty = 0, result[CC_SHA256_DIGEST_LENGTH];
    if (!CC_SHA256(size ? data : &empty, static_cast<CC_LONG>(size), result))
        throw std::runtime_error("SHA256 failed");
    constexpr char hex[] = "0123456789abcdef";
    std::string text; text.reserve(2 * sizeof(result));
    for (auto c : result) { text += hex[c >> 4]; text += hex[c & 15]; }
    return text;
}
std::string final_digest(CC_SHA256_CTX context) {
    uint8_t result[CC_SHA256_DIGEST_LENGTH]; CC_SHA256_Final(result, &context);
    constexpr char hex[] = "0123456789abcdef";
    std::string text;
    for (auto c : result) { text += hex[c >> 4]; text += hex[c & 15]; }
    return text;
}
bool same(const struct stat &a, const struct stat &b) {
    return a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size &&
        a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec && a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec;
}
}
int main(int argc, char **argv) {
    try {
        if (argc != 13) throw std::invalid_argument(
            "PATH EXPECTED_BYTES MAX_INPUT MAX_OUTPUT MAX_BLOCKS HEADER COMPRESSED RAW MAX_ENCODED MAX_DECODED EXPECTED_INPUT_SHA EXPECTED_DECODED_SHA");
        const std::string path(argv[1]);
        if (!std::all_of(path.begin(),path.end(),[](unsigned char c){return c>=32 && c!=127;}))
            throw std::invalid_argument("path control character");
        std::unique_ptr<char, decltype(&std::free)> resolved(::realpath(path.c_str(),nullptr),&std::free);
        if (!resolved || path != resolved.get()) throw std::runtime_error("absolute canonical nonsymlink path required");
        const uint64_t expected = number(argv[2]);
        if (expected < 4) throw std::invalid_argument("qualification input must contain a complete four-byte header");
        FrameProfile profile{word(argv[6]),word(argv[7]),word(argv[8]),static_cast<size_t>(number(argv[9])),
            static_cast<size_t>(number(argv[10])),number(argv[3]),number(argv[4]),number(argv[5])};
        if (profile.max_input_bytes>262144 || profile.max_output_bytes>16777216 || profile.max_blocks>512 ||
            profile.max_encoded_block>36144 || profile.max_decoded_block>36144 || expected>profile.max_input_bytes)
            throw std::out_of_range("qualification tool hard budget");
        ReadOnlyFd fd(path.c_str()); struct stat before{},after{};
        if (::fstat(fd.value,&before) || !S_ISREG(before.st_mode) || before.st_size<0 ||
            static_cast<uint64_t>(before.st_size)!=expected) throw std::runtime_error("input identity/size mismatch");
        std::vector<uint8_t> input(static_cast<size_t>(expected)); size_t done=0;
        while (done<input.size()) {
            const ssize_t got=::read(fd.value,input.data()+done,input.size()-done);
            if (got<0 && errno==EINTR) continue;
            if (got<=0) throw std::runtime_error("input shortread/I/O failure");
            done+=static_cast<size_t>(got);
        }
        uint8_t probe=0;
        if (::pread(fd.value,&probe,1,static_cast<off_t>(expected))!=0 || ::fstat(fd.value,&after) || !same(before,after))
            throw std::runtime_error("input changed or non-EOF after boundedread");
        const std::string input_sha=digest(input.data(),input.size());
        if (input_sha != argv[11]) throw std::runtime_error("fixed complete input SHA mismatch");
        OwnedFramedReader reader(input,profile); std::vector<uint8_t> block,prefix;
        CC_SHA256_CTX cumulative{};CC_SHA256_Init(&cumulative);
        uint64_t total=0,blocks=0;
        for (;;) {
            auto result=reader.next(block);
            if (result.status==ReadStatus::End) {
                if (result.input_offset!=input.size()) throw std::runtime_error("file tail remains");
                break;
            }
            if (result.status!=ReadStatus::Frame || result.codec.trailing_input ||
                (result.compressed && result.codec.codec_status!=1)) throw std::runtime_error("unqualified block");
            if (!block.empty()) CC_SHA256_Update(&cumulative,block.data(),static_cast<CC_LONG>(block.size()));
            if (prefix.size()<3415) prefix.insert(prefix.end(),block.begin(),block.begin()+std::min(block.size(),size_t(3415)-prefix.size()));
            total+=block.size();++blocks;
        }
        const auto decoded_sha=final_digest(cumulative);
        if (decoded_sha!=argv[12]) throw std::runtime_error("decoded SHA mismatch");
        if (::fstat(fd.value,&after) || !same(before,after)) throw std::runtime_error("input changed");
        if (prefix.size()<4) throw std::runtime_error("root/section versions truncated");
        const auto u16=[&prefix](size_t p){return uint16_t(prefix[p]) | uint16_t(prefix[p+1])<<8;};
        if (u16(0)!=19 || u16(2)!=6) throw std::runtime_error("sourcecity19/section6 mismatch; no fallback");
        if (prefix.size()<95) throw std::runtime_error("fixed prefix through component2 truncated");
        if (u16(45)!=2) throw std::runtime_error("source componentversion2 mismatch; no fallback");
        if (prefix.size()<97) throw std::runtime_error("helper version truncated");
        if (u16(95)!=3) throw std::runtime_error("qualification helperversion3 mismatch; no fallback");
        if (prefix.size()<99) throw std::runtime_error("fixed owner version truncated");
        if (u16(97)!=4) throw std::runtime_error("qualification ownerversion4 mismatch; no fallback");
        if (prefix.size()<3355) throw std::runtime_error("four fixed owner payload fields truncated");
        const auto u32=[&prefix](size_t p) {
            return uint32_t(prefix[p]) | uint32_t(prefix[p+1])<<8 |
                uint32_t(prefix[p+2])<<16 | uint32_t(prefix[p+3])<<24;
        };
        size_t parsed_end=3355, empty_members=0;
        bool positive_count=false;
        while (empty_members<10) {
            if (prefix.size()<parsed_end+6) throw std::runtime_error("member version/count truncated");
            if (u16(parsed_end)!=1) throw std::runtime_error("qualification memberversion1 mismatch; original has no local version gate");
            const uint32_t count_bits=u32(parsed_end+2);
            if (count_bits>INT32_MAX) throw std::runtime_error("negative member count outside qualification scope");
            parsed_end+=6;
            if (count_bits>0) { positive_count=true; break; }
            ++empty_members;
        }
        std::cout<<"{\"input_sha256\":"<<std::quoted(input_sha)<<",\"decoded_sha256\":"<<std::quoted(decoded_sha)
          <<",\"input_bytes\":"<<input.size()<<",\"decoded_bytes\":"<<total<<",\"validated_blocks\":"<<blocks
          <<",\"city_version\":19,\"first_section_version\":6,\"section_begin\":2,\"section_version_width\":2"
          <<",\"section_payload_bytes\":41,\"section_end\":45,\"fixed_field_requests\":8"
          <<",\"section_payload_sha256\":"<<std::quoted(digest(prefix.data()+4,41))
          <<",\"opaque16_offset\":17,\"opaque16_bytes\":16,\"opaque16_sha256\":"<<std::quoted(digest(prefix.data()+17,16))
          <<",\"component_version\":2,\"component_version_offset\":45,\"component_begin\":45"
          <<",\"component_payload_offset\":47,\"component_payload_bytes\":48,\"component_end\":95"
          <<",\"component_fixed_field_requests\":5,\"component_array_dwords\":8"
          <<",\"component_payload_sha256\":"<<std::quoted(digest(prefix.data()+47,48))
          <<",\"component_array32_offset\":59,\"component_array32_sha256\":"<<std::quoted(digest(prefix.data()+59,32))
          <<",\"live_original_receiver_type_proved\":false"
          <<",\"helper_version\":3,\"helper_version_offset\":95,\"fixed_owner_version\":4,\"fixed_owner_version_offset\":97"
          <<",\"fixed_owner_payload_offset\":99,\"fixed_owner_payload_bytes\":3256,\"fixed_owner_payload_end\":3355"
          <<",\"fixed_owner_payload_sha256\":"<<std::quoted(digest(prefix.data()+99,3256))
          <<",\"fixed_owner_requests\":4,\"member_group_begin\":3355,\"member_header_bytes\":6,\"member_version\":1"
          <<",\"completed_empty_members\":"<<empty_members
          <<",\"positive_member_count_observed\":"<<(positive_count?"true":"false")
          <<",\"positive_member_index\":"<<(positive_count?std::to_string(empty_members):"null")
          <<",\"last_member_count_sha256\":"<<std::quoted(digest(prefix.data()+parsed_end-4,4))
          <<",\"member_group_complete_without_objects\":"<<(!positive_count?"true":"false")
          <<",\"parsed_end\":"<<parsed_end<<",\"retained_prefix_bytes\":"<<prefix.size()
          <<",\"halt_reason\":"<<std::quoted(positive_count?"positive-member-object-payload-unqualified":"all-ten-empty-members-complete-next-owner-unqualified")
          <<",\"member_count_or_object_values_emitted\":false,\"original_version_gate_inferred_from_modern_scope\":false"
          <<",\"remaining_uninterpreted_decoded_bytes\":"<<(total-parsed_end)
          <<",\"read_only\":true,\"input_metadata_unchanged\":true,\"decoded_bytes_saved\":false"
          <<",\"cache_or_mainpool_indices_interpreted\":false,\"objects_deserialized\":false,\"actual_original_entry_proved\":false}\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 2; }
}
