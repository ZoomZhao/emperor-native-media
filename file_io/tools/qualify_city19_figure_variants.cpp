#include "owned_reader.hpp"
#include "city19_figure_variants_archive.hpp"
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
        if (argc !=13 && argc !=14 && argc !=15 && argc !=16) throw std::invalid_argument(
            "PATH EXPECTED_BYTES MAX_INPUT MAX_OUTPUT MAX_BLOCKS HEADER COMPRESSED RAW MAX_ENCODED MAX_DECODED EXPECTED_INPUT_SHA EXPECTED_DECODED_SHA [MAX_POOL_ITEMS [MAX_FIGURE_ITEMS [ENABLE_PHEASANT]]]");
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
        const uint64_t pheasant_mode=argc==16?number(argv[15]):1;
        if(pheasant_mode>1)throw std::invalid_argument("ENABLE_PHEASANT must be0 or1");
        const auto after_walk=research::walk_city19_figure_variants({prefix.data(),prefix.size()},pool,after_limits,pheasant_mode!=0);
        std::cout<<"{\"input_sha256\":"<<std::quoted(input_sha)<<",\"decoded_sha256\":"<<std::quoted(decoded_sha)
            <<",\"input_bytes\":"<<input.size()<<",\"decoded_bytes\":"<<total<<",\"validated_blocks\":"<<blocks
            <<",\"parsed_end\":"<<after_walk.end<<",\"retained_prefix_bytes\":"<<prefix.size()
            <<",\"known_items\":"<<result.known_items<<",\"completed_members\":"<<result.completed_members
            <<",\"following_member\":"<<result.following_member<<",\"following_ordinal\":"<<result.following_ordinal
            <<",\"stop_reason\":"<<std::quoted(after_walk.stop_reason)<<",\"conditional_cache_next_id\":"<<after_walk.cache.size()
            <<",\"cache_runtime_state_proved\":false,\"registry_current_vptr_proved\":false"
            <<",\"original_factory_or_schema_callback_executed\":false,\"field_values_emitted\":false"
            <<",\"Building_activity_status_interpreted\":false,\"Figure_activity_status_interpreted\":false,\"counts_are_stream_records_only\":true"
            <<",\"mainpool_indices_interpreted\":false,\"modern_cache_kind_gate_not_original_explicit_guard\":true"
            <<",\"remaining_uninterpreted_decoded_bytes\":"<<(total-after_walk.end)<<",\"member_headers\":[";
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
        std::cout<<",\"goal_parsed_end\":"<<result.end<<",\"postgoal_sections\":[";
        for (size_t i=0;i<post.sections.size();++i) {
            const auto &section=post.sections[i];if(i)std::cout<<',';
            std::cout<<"{\"name\":"<<std::quoted(section.name)<<",\"reader\":"<<std::quoted(section.reader)
                <<",\"start\":"<<section.start<<",\"end\":"<<section.end<<",\"version\":"<<section.version
                <<",\"payload_sha256\":"<<std::quoted(digest(prefix.data()+section.start,section.end-section.start))
                <<",\"embedded_version\":"<<section.embedded_version<<",\"embedded_version_offset\":"<<section.embedded_version_offset
                <<",\"fields\":[";
            for (size_t j=0;j<section.fields.size();++j) {
                const auto &f=section.fields[j];if(j)std::cout<<',';
                std::cout<<"{\"offset\":"<<f.offset<<",\"bytes\":"<<f.bytes<<",\"receiver_offset\":"<<f.receiver_offset
                    <<",\"source_call\":"<<std::quoted(f.source_call)<<",\"sha256\":"<<std::quoted(digest(prefix.data()+f.offset,f.bytes))<<'}';
            }
            std::cout<<"]}";
        }
        std::cout<<R"(],"postgoal_parsed_end":)"<<post.end<<R"(,"religion":{"start":)"<<following.religion.start
            <<R"(,"end":)"<<following.religion.end<<R"(,"version":)"<<following.religion.version
            <<R"(,"sha256":)"<<std::quoted(digest(prefix.data()+following.religion.start,following.religion.end-following.religion.start))<<R"(,"children":[)";
        for (size_t i=0;i<following.children.size();++i) {
            const auto &child=following.children[i];if(i)std::cout<<',';
            std::cout<<R"({"start":)"<<child.start<<R"(,"end":)"<<child.end<<R"(,"version":)"<<child.version<<R"(,"embedded":[)";
            for (size_t j=0;j<child.embedded.size();++j) {
                const auto &entry=child.embedded[j];if(j)std::cout<<',';
                std::cout<<R"({"start":)"<<entry.start<<R"(,"end":)"<<entry.end<<R"(,"version":)"<<entry.version
                    <<R"(,"payload_sha256":)"<<std::quoted(digest(prefix.data()+entry.start+2,entry.end-entry.start-2))<<'}';
            }
            std::cout<<"]}";
        }
        const auto &raw=following.religion.fields.back();
        std::cout<<R"(],"raw20":{"offset":)"<<raw.offset<<R"(,"bytes":)"<<raw.bytes<<R"(,"sha256":)"<<std::quoted(digest(prefix.data()+raw.offset,raw.bytes))
            <<R"(}},"commerce":)";
        if (following.commerce_qualified) {
            const auto &section=following.commerce;
            std::cout<<R"({"start":)"<<section.start<<R"(,"end":)"<<section.end<<R"(,"version":)"<<section.version<<R"(,"field_bytes":0})";
        } else std::cout<<"null";
        std::cout<<R"(,"prepool_parsed_end":)"<<following.end<<R"(,"pool":{"start":)"<<pool.start<<R"(,"end":)"<<pool.end
            <<R"(,"version":)"<<pool.version<<R"(,"count":)"<<pool.count<<R"(,"processed":)"<<pool.items.size()
            <<R"(,"declared_items_complete":)"<<(pool.complete?"true":"false")<<R"(,"cache_start":)"<<pool.cache_start
            <<R"(,"cache_next":)"<<pool.cache.size()<<R"(,"stop_reason":)"<<std::quoted(pool.stop_reason)<<R"(,"items":[)";
        for (size_t i=0;i<pool.items.size();++i) {
            const auto &item=pool.items[i];if(i)std::cout<<',';
            std::cout<<R"({"ordinal":)"<<item.ordinal<<R"(,"tag_offset":)"<<item.tag_offset<<R"(,"class_model_id":)"<<item.class_id
                <<R"(,"object_model_id":)"<<item.object_id<<R"(,"object_reference":)"<<(item.object_reference?"true":"false")
                <<R"(,"payload_offset":)"<<item.payload_offset<<R"(,"payload_bytes":)"<<item.payload_bytes
                <<R"(,"payload_sha256":)"<<std::quoted(digest(prefix.data()+item.payload_offset,item.payload_bytes))<<'}';
        }
        std::cout<<R"(],"pending_class":)";
        if (pool.pending.present) {
            const auto &pending=pool.pending;
            std::cout<<R"({"ordinal":)"<<pending.ordinal<<R"(,"tag_offset":)"<<pending.tag_offset<<R"(,"schema":)"<<pending.schema
                <<R"(,"name_bytes":)"<<pending.name_bytes<<R"(,"name_sha256":)"<<std::quoted(digest(prefix.data()+pending.name_offset,pending.name_bytes))
                <<R"(,"class_id_if_resolved":)"<<pending.class_id_if_resolved<<R"(,"object_id_if_factory_returns":)"<<pending.object_id_if_factory_returns<<'}';
        } else std::cout<<"null";
        std::cout<<R"(},"afterpool":{"start":)"<<after_walk.start<<R"(,"end":)"<<after_walk.end<<R"(,"version":)"<<after_walk.version
            <<R"(,"count":)"<<after_walk.count<<R"(,"processed":)"<<after_walk.figures.size()<<R"(,"figures_complete":)"<<(after_walk.figures_complete?"true":"false")
            <<R"(,"cache_start":)"<<after_walk.cache_start<<R"(,"cache_next":)"<<after_walk.cache.size()<<R"(,"stop_reason":)"<<std::quoted(after_walk.stop_reason)<<R"(,"items":[)";
        for(size_t i=0;i<after_walk.figures.size();++i){const auto &p=after_walk.figures[i];if(i)std::cout<<',';
            std::cout<<R"({"ordinal":)"<<p.ordinal<<R"(,"tag_offset":)"<<p.tag_offset<<R"(,"class_model_id":)"<<p.class_id
                <<R"(,"object_model_id":)"<<p.object_id<<R"(,"object_reference":)"<<(p.object_reference?"true":"false")
                <<R"(,"payload_offset":)"<<p.payload_offset<<R"(,"payload_bytes":)"<<p.payload_bytes
                <<R"(,"payload_sha256":)"<<std::quoted(digest(prefix.data()+p.payload_offset,p.payload_bytes))<<'}';}
        std::cout<<R"(],"pending_class":)";
        if(after_walk.pending.present){const auto &p=after_walk.pending;
            std::cout<<R"({"ordinal":)"<<p.ordinal<<R"(,"tag_offset":)"<<p.tag_offset<<R"(,"schema":)"<<p.schema
                <<R"(,"name_bytes":)"<<p.name_bytes<<R"(,"name_sha256":)"<<std::quoted(digest(prefix.data()+p.name_offset,p.name_bytes))
                <<R"(,"class_id_if_resolved":)"<<p.class_id_if_resolved<<R"(,"object_id_if_factory_returns":)"<<p.object_id_if_factory_returns<<'}';
        }else std::cout<<"null";
        std::cout<<R"(,"pending_extension":)";
        if(after_walk.extension.present){const auto &p=after_walk.extension;
            std::cout<<R"({"ordinal":)"<<p.ordinal<<R"(,"payload_offset":)"<<p.payload_offset<<R"(,"end":)"<<p.end
                <<R"(,"object_model_id":)"<<p.object_id<<R"(,"full_object_payload_qualified":false})";
        }else std::cout<<"null";
        std::cout<<R"(},"read_only":true,"raw_or_decoded_bytes_saved":false})"<<'\n';
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 2; }
}
