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
            if (prefix.size()<3522) prefix.insert(prefix.end(),block.begin(),block.begin()+std::min(block.size(),size_t(3522)-prefix.size()));
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
        if (!positive_count || empty_members!=0 || parsed_end!=3361)
            throw std::runtime_error("qualification requires first positive member at3361");
        const size_t member_header_end=parsed_end;
        if (prefix.size()<parsed_end+2) throw std::runtime_error("object tag truncated");
        const uint16_t tag_word=u16(parsed_end);
        parsed_end+=2;
        uint32_t tag_value=uint32_t(tag_word&0x7fff) | (uint32_t(tag_word&0x8000)<<16);
        if (tag_word==0x7fff) {
            if (prefix.size()<parsed_end+4) throw std::runtime_error("extended object tag truncated");
            tag_value=u32(parsed_end); parsed_end+=4;
        }
        const bool new_class=tag_word==0xffff;
        uint16_t declared_schema=0, name_length=0;
        std::string name_sha;
        if (new_class) {
            if (prefix.size()<parsed_end+4) throw std::runtime_error("class schema/name length truncated");
            declared_schema=u16(parsed_end); name_length=u16(parsed_end+2); parsed_end+=4;
            if (name_length>=64) throw std::runtime_error("source class name length must be below64");
            if (!name_length) throw std::runtime_error("empty class name outside qualification scope");
            if (prefix.size()<parsed_end+name_length) throw std::runtime_error("class name bytes truncated");
            const auto letter=[](uint8_t c){return (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c=='_';};
            if (!letter(prefix[parsed_end]) || !std::all_of(prefix.begin()+parsed_end,prefix.begin()+parsed_end+name_length,
                [&letter](uint8_t c){return letter(c) || (c>='0'&&c<='9');}))
                throw std::runtime_error("class name outside modern ASCII identifier scope");
            name_sha=digest(prefix.data()+parsed_end,name_length); parsed_end+=name_length;
        } else if (tag_value&0x80000000) {
            if (!(tag_value&0x7fffffff)) throw std::runtime_error("source class reference index zero invalid");
            throw std::runtime_error("class reference has no qualified cache entry; no declaration fallback");
        } else if (tag_value) {
            throw std::runtime_error("object reference has no qualified cache entry; no payload fallback");
        }
        if (!new_class || declared_schema!=0 || name_length!=12 ||
            name_sha!="c4291be23ebdfef80b50eeb34ec27a9f30d63fa29e8014a7447d482e9c9ae6c7" || parsed_end!=3379)
            throw std::runtime_error("qualification requires source-matched cHousingGoal/schema0 declaration");
        const size_t declaration_end=parsed_end;
        if (prefix.size()<3399) throw std::runtime_error("housing source payload20 truncated");
        if (u16(3379)!=1 || u16(3385)!=1)
            throw std::runtime_error("base/housing section versions outside modern scope1; no source version gate inferred");
        parsed_end=3399;
        const bool first_member_complete=u32(3357)==1;
        const size_t first_payload_end=parsed_end;
        size_t next_member_index=0, skipped_empty_members=0;
        uint32_t next_member_count=u32(3357);
        bool has_next_item=true;
        if (first_member_complete) {
            next_member_index=1;
            while (next_member_index<10) {
                if (prefix.size()<parsed_end+6) throw std::runtime_error("next member header truncated");
                if (u16(parsed_end)!=1) throw std::runtime_error("next member version outside modern scope1");
                next_member_count=u32(parsed_end+2);
                if (next_member_count>INT32_MAX) throw std::runtime_error("negative next member count outside qualification scope");
                parsed_end+=6;
                if (next_member_count>0) break;
                ++next_member_index; ++skipped_empty_members;
            }
            has_next_item=next_member_index<10;
        }
        const size_t next_tag_offset=parsed_end;
        uint16_t next_word=0, next_schema=0, next_name_length=0;
        uint32_t next_tag_value=0, reference_index=0, conditional_cache_next=3;
        std::string next_kind="no_item_all_members_empty", next_name_sha, next_payload_sha;
        bool next_payload_qualified=false, reused_object=false, pending_declaration=false;
        size_t next_payload_bytes=0;
        if (has_next_item) {
            if (prefix.size()<parsed_end+2) throw std::runtime_error("next object tag truncated");
            next_word=u16(parsed_end); parsed_end+=2;
            next_tag_value=uint32_t(next_word&0x7fff) | (uint32_t(next_word&0x8000)<<16);
            if (next_word==0x7fff) {
                if (prefix.size()<parsed_end+4) throw std::runtime_error("next extended object tag truncated");
                next_tag_value=u32(parsed_end); parsed_end+=4;
            }
            if (next_word==0xffff) {
                next_kind="new_class_declaration"; pending_declaration=true;
                if (prefix.size()<parsed_end+4) throw std::runtime_error("next class schema/name length truncated");
                next_schema=u16(parsed_end); next_name_length=u16(parsed_end+2); parsed_end+=4;
                if (next_name_length>=64) throw std::runtime_error("source next class name length must be below64");
                if (!next_name_length) throw std::runtime_error("empty next class name outside qualification scope");
                if (prefix.size()<parsed_end+next_name_length) throw std::runtime_error("next class name bytes truncated");
                const auto letter=[](uint8_t c){return (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c=='_';};
                if (!letter(prefix[parsed_end]) || !std::all_of(prefix.begin()+parsed_end,prefix.begin()+parsed_end+next_name_length,
                    [&letter](uint8_t c){return letter(c) || (c>='0'&&c<='9');}))
                    throw std::runtime_error("next class name outside modern ASCII identifier scope");
                next_name_sha=digest(prefix.data()+parsed_end,next_name_length); parsed_end+=next_name_length;
            } else if (next_tag_value&0x80000000) {
                next_kind="class_reference_new_instance"; reference_index=next_tag_value&0x7fffffff;
                if (!reference_index) throw std::runtime_error("source next class reference zero invalid");
                if (reference_index==2) throw std::runtime_error("modern cache kind mismatch: class expected, object modeled");
                if (reference_index!=1) throw std::runtime_error("next class reference has no qualified cache entry");
                if (prefix.size()<parsed_end+20) throw std::runtime_error("class reference requires new Housing payload20");
                if (u16(parsed_end)!=1 || u16(parsed_end+6)!=1)
                    throw std::runtime_error("referenced Housing section versions outside modern scope1");
                next_payload_sha=digest(prefix.data()+parsed_end,20); parsed_end+=20;
                next_payload_qualified=true;next_payload_bytes=20;conditional_cache_next=4;
            } else {
                next_kind="object_reference_no_payload"; reference_index=next_tag_value;
                if (!reference_index) throw std::runtime_error("generic null allowed; positive member consumer requires nonnull object");
                if (reference_index==1) throw std::runtime_error("modern cache kind mismatch: object expected, class modeled");
                if (reference_index!=2) throw std::runtime_error("next object reference has no qualified cache entry");
                reused_object=true;
            }
        }
        const bool next_item_complete=reused_object||next_payload_qualified;
        const uint32_t served_ordinal=first_member_complete?0:1;
        const bool next_member_complete=next_item_complete && next_member_count==served_ordinal+1;
        const size_t following_member=next_member_index+size_t(next_member_complete);
        const std::string following_kind=pending_declaration?"pending_class_payload":!has_next_item||following_member==10?
            "next_root_consumer":next_member_complete?"next_member_header":"next_object_tag_same_member";
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
          <<",\"last_member_count_sha256\":"<<std::quoted(digest(prefix.data()+member_header_end-4,4))
          <<",\"member_group_complete_without_objects\":"<<(!positive_count?"true":"false")
          <<",\"parsed_end\":"<<parsed_end<<",\"retained_prefix_bytes\":"<<prefix.size()
          <<",\"member_header_frontier\":"<<member_header_end
          <<",\"archive_tag_word\":"<<tag_word<<",\"archive_tag_internal\":"<<tag_value
          <<",\"archive_tag_kind\":"<<std::quoted(new_class?"new_class":"null_object_reference")
          <<",\"declared_schema\":"<<(new_class?std::to_string(declared_schema):"null")
          <<",\"declared_class_name_bytes\":"<<(new_class?std::to_string(name_length):"null")
          <<",\"declared_class_name_sha256\":"<<(new_class?"\""+name_sha+"\"":"null")
          <<",\"next_member_count_relation\":"<<std::quoted(next_member_count==0?"zero":next_member_count==1?"exactly_one":"greater_than_one")
          <<",\"next_item_complete_under_source_model\":"<<(next_item_complete?"true":"false")
          <<",\"next_member_complete_after_qualified_item\":"<<(next_member_complete?"true":"false")
          <<",\"following_consumer_offset\":"<<parsed_end<<",\"following_member_index\":"<<following_member
          <<",\"following_consumer_kind\":"<<std::quoted(following_kind)
          <<",\"first_payload_frontier\":"<<first_payload_end
          <<",\"next_member_index\":"<<next_member_index<<",\"skipped_empty_members\":"<<skipped_empty_members
          <<",\"next_item_present\":"<<(has_next_item?"true":"false")<<",\"next_tag_offset\":"<<(has_next_item?std::to_string(next_tag_offset):"null")
          <<",\"next_tag_word\":"<<(has_next_item?std::to_string(next_word):"null")<<",\"next_tag_kind\":"<<std::quoted(next_kind)
          <<",\"next_reference_index\":"<<(reused_object||next_payload_qualified?std::to_string(reference_index):"null")
          <<",\"next_declared_schema\":"<<(pending_declaration?std::to_string(next_schema):"null")
          <<",\"next_declared_name_bytes\":"<<(pending_declaration?std::to_string(next_name_length):"null")
          <<",\"next_declared_name_sha256\":"<<(pending_declaration?"\""+next_name_sha+"\"":"null")
          <<",\"next_pending_class_model_id\":"<<(pending_declaration?"3":"null")<<",\"next_pending_object_model_id\":"<<(pending_declaration?"4":"null")
          <<",\"next_class_lookup_or_payload_qualified\":"<<(next_payload_qualified?"true":"false")
          <<",\"next_object_reference_reuses_existing_metadata\":"<<(reused_object?"true":"false")
          <<",\"next_payload_bytes_consumed\":"<<next_payload_bytes
          <<",\"next_payload_sha256\":"<<(next_payload_qualified?"\""+next_payload_sha+"\"":"null")
          <<",\"conditional_cache_next_after_item\":"<<conditional_cache_next
          <<",\"archive_cache_index_interpreted_under_model\":"<<(reused_object||next_payload_qualified?"true":"false")
          <<",\"modern_cache_kind_check_added\":true,\"original_explicit_cache_kind_check_inferred\":false"
          <<",\"declaration_end\":"<<declaration_end
          <<",\"object_payload_offset\":3379,\"object_payload_bytes\":20,\"object_payload_end\":3399"
          <<",\"base_section_version\":1,\"base_section_version_offset\":3379,\"housing_section_version\":1,\"housing_section_version_offset\":3385"
          <<",\"object_embedded_version_bytes\":4,\"object_fixed_field_requests\":4,\"object_field_data_bytes\":16"
          <<",\"object_payload_sha256\":"<<std::quoted(digest(prefix.data()+3379,20))
          <<",\"base_field32_sha256\":"<<std::quoted(digest(prefix.data()+3381,4))
          <<",\"housing_fields12_sha256\":"<<std::quoted(digest(prefix.data()+3387,12))
          <<",\"first_member_count_relation\":"<<std::quoted(first_member_complete?"exactly_one":"greater_than_one")
          <<",\"first_member_complete_after_first_object\":"<<(first_member_complete?"true":"false")
          <<",\"next_consumer_offset\":3399,\"next_consumer_kind\":"<<std::quoted(first_member_complete?"next_member_header":"next_object_tag_same_member")
          <<",\"source_class_descriptor_mapped\":true,\"source_cache_class_id\":1,\"source_cache_object_id\":2,\"source_cache_next_id\":3"
          <<",\"cache_ids_are_conditional_model\":true,\"object_payload_fields_qualified\":true"
          <<",\"class_name_emitted\":false,\"actual_descriptor_resolved\":false"
          <<",\"expected_descriptor_used_as_actual\":false,\"original_class_factory_or_schema_reader_executed\":false,\"modern_fixed_payload_parser_executed\":true"
          <<",\"cache_runtime_state_proved\":false"
          <<",\"halt_reason\":"<<std::quoted(pending_declaration?"next-class-declaration-only-payload-unqualified":has_next_item?"next-reference-qualified-following-consumer-unqualified":"all-ten-members-complete-next-owner-unqualified")
          <<",\"object_field_values_emitted\":false,\"raw_member_count_value_emitted\":false,\"member_count_classification_emitted\":true,\"original_version_gate_inferred_from_modern_scope\":false"
          <<",\"remaining_uninterpreted_decoded_bytes\":"<<(total-parsed_end)
          <<",\"read_only\":true,\"input_metadata_unchanged\":true,\"decoded_bytes_saved\":false"
          <<",\"mainpool_indices_interpreted\":false,\"objects_deserialized\":false,\"actual_original_entry_proved\":false}\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 2; }
}
