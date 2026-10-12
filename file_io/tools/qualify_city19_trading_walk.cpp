#include "owned_reader.hpp"
#include "city19_goal_archive.hpp"
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
        if (argc != 13 && argc != 14) throw std::invalid_argument(
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
            if (prefix.size()<16384) prefix.insert(prefix.end(),block.begin(),block.begin()+std::min(block.size(),size_t(16384)-prefix.size()));
            total+=block.size();++blocks;
        }
        const auto decoded_sha=final_digest(cumulative);
        if (decoded_sha!=argv[12]) throw std::runtime_error("decoded SHA mismatch");
        if (::fstat(fd.value,&after) || !same(before,after)) throw std::runtime_error("input changed");
        if (prefix.size()<3355) throw std::runtime_error("fixed city context truncated");
        const auto u16=[&prefix](size_t p){return uint16_t(prefix[p])|uint16_t(prefix[p+1])<<8;};
        if (u16(0)!=19||u16(2)!=6||u16(45)!=2||u16(95)!=3||u16(97)!=4)
            throw std::runtime_error("source city/context versions outside modern qualification scope");
        research::GoalLimits limits; limits.enable_homage=true; limits.enable_trading=true;
        if (argc==14) limits.checkpoint_items=static_cast<size_t>(number(argv[13]));
        const auto result=research::walk_city19_housing_goals({prefix.data(),prefix.size()},limits);
        std::cout<<"{\"input_sha256\":"<<std::quoted(input_sha)<<",\"decoded_sha256\":"<<std::quoted(decoded_sha)
            <<",\"input_bytes\":"<<input.size()<<",\"decoded_bytes\":"<<total<<",\"validated_blocks\":"<<blocks
            <<",\"parsed_end\":"<<result.end<<",\"retained_prefix_bytes\":"<<prefix.size()
            <<",\"known_items\":"<<result.known_items<<",\"completed_members\":"<<result.completed_members
            <<",\"following_member\":"<<result.following_member<<",\"following_ordinal\":"<<result.following_ordinal
            <<",\"stop_reason\":"<<std::quoted(result.stop_reason)<<",\"conditional_cache_next_id\":"<<result.cache.size()
            <<",\"cache_runtime_state_proved\":false,\"registry_current_vptr_proved\":false"
            <<",\"original_factory_or_schema_callback_executed\":false,\"field_values_emitted\":false"
            <<",\"mainpool_indices_interpreted\":false,\"modern_cache_kind_gate_not_original_explicit_guard\":true"
            <<",\"remaining_uninterpreted_decoded_bytes\":"<<(total-result.end)<<",\"member_headers\":[";
        for (size_t i=0;i<result.members.size();++i) {
            const auto &m=result.members[i];if(i)std::cout<<',';
            std::cout<<"{\"member\":"<<m.index<<",\"offset\":"<<m.offset<<",\"version\":1,\"count\":"<<m.count<<'}';
        }
        std::cout<<"],\"items\":[";
        for (size_t i=0;i<result.items.size();++i) {
            const auto &p=result.items[i];if(i)std::cout<<',';
            std::cout<<"{\"member\":"<<p.member<<",\"ordinal\":"<<p.ordinal<<",\"tag_offset\":"<<p.tag_offset
                <<",\"class_model_id\":"<<p.class_id<<",\"object_model_id\":"<<p.object_id
                <<",\"object_reference\":"<<(p.object_reference?"true":"false")
                <<",\"payload_offset\":"<<p.payload_offset<<",\"payload_bytes\":"<<p.payload_bytes
                <<",\"payload_sha256\":"<<std::quoted(digest(prefix.data()+p.payload_offset,p.payload_bytes))<<'}';
        }
        std::cout<<"],\"pending_class\":";
        if (result.pending.present) {
            const auto &p=result.pending;
            std::cout<<"{\"member\":"<<p.member<<",\"ordinal\":"<<p.ordinal<<",\"tag_offset\":"<<p.tag_offset
                <<",\"schema\":"<<p.schema<<",\"name_bytes\":"<<p.name_bytes
                <<",\"name_sha256\":"<<std::quoted(digest(prefix.data()+p.name_offset,p.name_bytes))
                <<",\"class_id_if_resolved\":"<<p.class_id_if_resolved<<",\"object_id_if_factory_returns\":"<<p.object_id_if_factory_returns
                <<",\"lookup_or_payload_qualified\":false}";
        } else std::cout<<"null";
        std::cout<<",\"read_only\":true,\"raw_or_decoded_bytes_saved\":false}\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 2; }
}
