#include "owned_reader.hpp"
#include "city19_figurefsa_objects.hpp"
#include "city19_following_vectors_archive.hpp"
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
        if (argc !=13 && argc !=14 && argc !=15 && argc !=16 && argc !=17 && argc !=18 && argc !=19 && argc !=20 && argc !=21) throw std::invalid_argument(
            "PATH EXPECTED_BYTES MAX_INPUT MAX_OUTPUT MAX_BLOCKS HEADER COMPRESSED RAW MAX_ENCODED MAX_DECODED EXPECTED_INPUT_SHA EXPECTED_DECODED_SHA [MAX_POOL_ITEMS [MAX_FIGURE_ITEMS [ENABLE_PHEASANT [MAX_GOODS_ITEMS [MAX_EMPIRE_ITEMS [MAX_GRIDS [MAX_FIXED [MAX_VECTORS]]]]]]]]");
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
            if (prefix.size()<4915584) prefix.insert(prefix.end(),block.begin(),block.begin()+std::min(block.size(),size_t(4915584)-prefix.size()));
            total+=block.size();++blocks;
        }
        const auto decoded_sha=final_digest(cumulative);
        if (decoded_sha!=argv[12]) throw std::runtime_error("decoded SHA mismatch");
        if (::fstat(fd.value,&after) || !same(before,after)) throw std::runtime_error("input changed");
        if (prefix.size()<3355) throw std::runtime_error("fixed city context truncated");
        const auto u16=[&prefix](size_t p){return uint16_t(prefix[p])|uint16_t(prefix[p+1])<<8;};
        if (u16(0)!=19||u16(2)!=6||u16(45)!=2||u16(95)!=3||u16(97)!=4)
            throw std::runtime_error("source city/context versions outside modern qualification scope");
        research::GoalLimits limits; limits.enable_homage=true; limits.enable_trading=true; limits.enable_yearly=true; limits.enable_population=true;
        const auto result=research::walk_city19_housing_goals({prefix.data(),std::min(prefix.size(),size_t(16384))},limits);
        const auto post=research::walk_city19_postgoals({prefix.data(),std::min(prefix.size(),size_t(65536))},result,true);
        const auto following=research::walk_city19_religion_following({prefix.data(),std::min(prefix.size(),size_t(65536))},post,true);
        research::PoolLimits pool_limits;
        if (argc>=14) pool_limits.max_items=static_cast<size_t>(number(argv[13]));
        const auto pool=research::walk_city19_mainpool({prefix.data(),std::min(prefix.size(),size_t(1048576))},result,following,pool_limits);
        research::AfterPoolLimits after_limits;
        if(argc>=15)after_limits.max_items=static_cast<size_t>(number(argv[14]));
        const uint64_t pheasant_mode=argc>=16?number(argv[15]):1;
        if(pheasant_mode>1)throw std::invalid_argument("ENABLE_PHEASANT must be0 or1");
        const auto after_walk=research::walk_city19_figure_variants({prefix.data(),prefix.size()},pool,after_limits,pheasant_mode!=0);
        research::GoodsLimits goods_limits;
        if(argc>=17)goods_limits.max_items=static_cast<size_t>(number(argv[16]));
        const auto goods_walk=research::walk_city19_goodscontrol({prefix.data(),prefix.size()},after_walk,goods_limits);
        research::EmpireInlineLimits empire_limits;
        if(argc>=18)empire_limits.max_items=static_cast<size_t>(number(argv[17]));
        const auto empire_walk=research::walk_city19_empire_inline({prefix.data(),prefix.size()},goods_walk,empire_limits);
        research::EmpireGridLimits grid_limits;
        if(argc>=19)grid_limits.max_grids=static_cast<size_t>(number(argv[18]));
        const auto grid_walk=research::walk_city19_empire_grids({prefix.data(),prefix.size()},empire_walk,grid_limits);
        research::FollowingFixedLimits fixed_limits;fixed_limits.max_records=1;fixed_limits.inspect_next_header=false;
        const auto fixed=research::walk_city19_following_fixed({prefix.data(),prefix.size()},grid_walk,fixed_limits);
        constexpr size_t start=1991913;
        if(fixed.end!=start||!fixed.pending_extension||fixed.cache_next!=6020||after_walk.cache.size()!=6020)
            throw std::runtime_error("conditional preceding frontier/cache6020 mismatch");
        std::vector<research::FigureFsaWireCacheEntry> cache;
        for(const auto&e:after_walk.cache) {
            const auto k=e.kind==research::GoalCacheKind::Null?research::FigureFsaCacheKind::Null:
                e.kind==research::GoalCacheKind::Class?research::FigureFsaCacheKind::Class:research::FigureFsaCacheKind::Object;
            cache.push_back({k,research::FigureFsaWireType::OpaquePrefix,e.class_id,e.schema});
        }
        research::FigureFsaObjectLimits fsa_limits;fsa_limits.max_cache=8192;
        const auto window=std::min(size_t(65536),prefix.size()-start);
        bool complete=false;size_t frontier=start,attempted=start,final_cache=6020,objects=0,classes=0,refs=0;
        std::string stop;bool unknown=false;size_t class_tag=0,name_offset=0,name_bytes=0;uint16_t class_schema=0;std::string name_hash;
        try {
            const auto fsa=research::walk_city19_figurefsa_with_archive_prefix(
                {prefix.data()+start,window},research::FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10,
                cache,6020,0,fsa_limits);
            complete=true;frontier=start+fsa.end;attempted=frontier;final_cache=fsa.cache.size();
            objects=fsa.payloads.size();classes=fsa.declarations.size();refs=fsa.references.size();
            stop="selected_first_FigureFSA_record_complete_before_next_caller_consumer";
        } catch(const research::FigureFsaPrefixReadFailure&e) {attempted=start+e.attempted_offset;stop=e.what();if(e.unknown_class){unknown=true;class_tag=start+e.tag_offset;name_offset=start+e.name_offset;name_bytes=e.name_bytes;class_schema=e.schema;name_hash=digest(prefix.data()+name_offset,name_bytes);}}
        if(::fstat(fd.value,&after)||!same(before,after))throw std::runtime_error("input changed after conditional experiment");
        std::cout<<"{\"experiment\":\"city19_first_holder30_root7b5bc8_six_exact_classes_cache6020\","
            <<"\"input_sha256\":"<<std::quoted(input_sha)<<",\"decoded_sha256\":"<<std::quoted(decoded_sha)
            <<",\"validated_blocks\":"<<blocks<<",\"decoded_bytes\":"<<total
            <<",\"candidate_root\":\"7b5bc8\",\"candidate_reader\":\"4dad10\",\"conditional_frontier\":"<<frontier
            <<",\"unconditional_frontier\":1991913,\"attempted_stop_offset\":"<<attempted
            <<",\"new_conditional_bytes\":"<<frontier-start<<",\"new_unconditional_bytes\":0,\"record_complete_under_assumptions\":"<<(complete?"true":"false")
            <<",\"initial_cache_entries\":6020,\"final_conditional_cache_entries\":"<<final_cache
            <<",\"new_objects\":"<<objects<<",\"new_classes\":"<<classes<<",\"reference_count\":"<<refs
            <<",\"stop_reason\":"<<std::quoted(stop)
            <<",\"unknown_class_declaration\":"<<(unknown?"true":"false")<<",\"unknown_class_tag_offset\":"<<class_tag<<",\"unknown_class_name_offset\":"<<name_offset<<",\"unknown_class_name_bytes\":"<<name_bytes<<",\"unknown_class_schema\":"<<class_schema<<",\"unknown_class_name_sha256\":"<<std::quoted(name_hash)
            <<",\"actual_holder_identity_proved\":false,\"actual_registry_or_vptr_proved\":false,\"raw_or_decoded_bytes_saved\":false,\"original_or_derived_instructions_executed\":false,\"input_unchanged\":true}\n";
        return 0;
    } catch(const std::exception&e) {std::cerr<<"conditional experiment rejected: "<<e.what()<<'\n';return 1;}
}
