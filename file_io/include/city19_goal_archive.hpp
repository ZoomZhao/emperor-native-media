#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace emperor::file_io::research {
// Non-owning immutable input. Results contain offsets/metadata, never source pointers or field values.
struct GoalBytes { const uint8_t *data; size_t size; };
struct GoalLimits {
    size_t max_prefix=16384;
    size_t max_items=128;
    size_t max_cache=256;
    size_t checkpoint_items=0;
    bool enable_homage=false;
    bool enable_trading=false;
    bool enable_yearly=false;
    bool enable_population=false;
};
enum class GoalCacheKind { Null, Class, Object };
enum class GoalClassType { Housing, Homage, Trading, Yearly, Population };
struct GoalCacheEntry {
    GoalCacheKind kind;
    uint32_t class_id=0;
    uint16_t schema=0;
    size_t registered_at=0;
    GoalClassType class_type=GoalClassType::Housing;
};
struct GoalPayload {
    size_t member=0, ordinal=0, tag_offset=0, payload_offset=0, payload_bytes=0;
    uint32_t object_id=0, class_id=0;
    bool object_reference=false;
};
struct GoalMember { size_t index=0, offset=0; uint32_t count=0; };
struct GoalPendingClass {
    bool present=false;
    size_t tag_offset=0, name_offset=0, name_bytes=0, member=0, ordinal=0;
    uint16_t schema=0;
    uint32_t class_id_if_resolved=0, object_id_if_factory_returns=0;
};
struct GoalWalk {
    size_t end=3355, known_items=0, completed_members=0;
    size_t following_member=0, following_ordinal=0;
    std::string stop_reason;
    std::vector<GoalCacheEntry> cache;
    std::vector<GoalPayload> items;
    std::vector<GoalMember> members;
    GoalPendingClass pending;
};
class GoalCursor {
    GoalBytes bytes_;
    size_t offset_;
public:
    GoalCursor(GoalBytes bytes,size_t offset):bytes_(bytes),offset_(offset) {
        if (offset>bytes.size) throw std::runtime_error("goal start beyond bounded prefix");
    }
    size_t position() const { return offset_; }
    const uint8_t *take(size_t n) {
        if (n>bytes_.size-offset_) throw std::runtime_error("goal field/tag/name truncated in bounded prefix");
        const uint8_t *p=bytes_.data+offset_; offset_+=n; return p;
    }
    uint16_t u16() { const auto p=take(2); return uint16_t(p[0])|uint16_t(p[1])<<8; }
    uint32_t u32() { const auto p=take(4); return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24; }
};
struct GoalTag { uint16_t word; uint32_t value; bool declaration; size_t offset; };
inline GoalTag goal_tag(GoalCursor &cursor) {
    const size_t offset=cursor.position(); const auto word=cursor.u16();
    const auto value=word==0x7fff?cursor.u32():uint32_t(word&0x7fff)|(uint32_t(word&0x8000)<<16);
    return {word,value,word==0xffff,offset};
}
inline void goal_base_payload(GoalCursor &cursor) {
    // Source55feb0: own version16, then four bytes atself+4. Modern version1 subset.
    if (cursor.u16()!=1) throw std::runtime_error("base goal version outside modern scope1");
    cursor.take(4);
}
inline void goal_three_dword_payload(GoalCursor &cursor,const char *version_error) {
    goal_base_payload(cursor);
    if (cursor.u16()!=1) throw std::runtime_error(version_error);
    cursor.take(12);
}
inline void goal_housing_payload(GoalCursor &cursor) {
    goal_three_dword_payload(cursor,"Housing goal version outside modern scope1");
}
inline void goal_yearly_payload(GoalCursor &cursor) {
    goal_three_dword_payload(cursor,"Yearly production goal version outside modern source1 subset");
}
inline void goal_homage_payload(GoalCursor &cursor) {
    goal_base_payload(cursor);
    if (cursor.u16()!=3) throw std::runtime_error("Homage goal version outside qualified source3 subset");
    cursor.take(4); // Source 560aca: self+c precedes self+8.
    cursor.take(4); // Source 560ae0: self+8.
}
inline void goal_one_dword_payload(GoalCursor &cursor,const char *version_error) {
    goal_base_payload(cursor);
    if (cursor.u16()!=1) throw std::runtime_error(version_error);
    cursor.take(4);
}
inline void goal_trading_payload(GoalCursor &cursor) {
    // Independent source560bd6: self+8, no nested count/container.
    goal_one_dword_payload(cursor,"Trading goal version outside modern source1 subset");
}
inline void goal_population_payload(GoalCursor &cursor) {
    // Independent source5602c6/record12a5a40: self+8, own record initialized1.
    goal_one_dword_payload(cursor,"Population goal version outside modern source1 subset");
}
// Conditional source model only. No original registry lookup, pointers, factory, callbacks or rules.
// Errors never publish a partially mutated model: all state belongs to this invocation.
inline GoalWalk walk_city19_housing_goals(GoalBytes bytes,GoalLimits limits={}) {
    if (!bytes.data || bytes.size>limits.max_prefix || limits.max_prefix>16384 ||
        !limits.max_items || limits.max_items>128 || !limits.max_cache || limits.max_cache>256 ||
        limits.checkpoint_items>limits.max_items) throw std::runtime_error("modern goal parser budget/argument");
    GoalCursor cursor(bytes,3355); GoalWalk result;
    result.cache.push_back({GoalCacheKind::Null,0,0,3355});
    auto append=[&](GoalCacheEntry entry) {
        if (result.cache.size()>=limits.max_cache) throw std::runtime_error("modern goal cache budget exhausted");
        const uint32_t id=static_cast<uint32_t>(result.cache.size()); result.cache.push_back(entry); return id;
    };
    for (size_t member=0;member<10;++member) {
        const size_t header=cursor.position();
        if (cursor.u16()!=1) throw std::runtime_error("member version outside modern scope1");
        const uint32_t count=cursor.u32();
        if (count>0x7fffffff) throw std::runtime_error("negative goal count outside modern scope");
        result.members.push_back({member,header,count});
        for (uint32_t ordinal=0;ordinal<count;++ordinal) {
            if (result.known_items>=limits.max_items) throw std::runtime_error("modern goal item budget exhausted");
            const auto tag=goal_tag(cursor); uint32_t class_id=0, object_id=0;
            bool object_reference=false;
            if (tag.declaration) {
                const uint16_t schema=cursor.u16(), length=cursor.u16();
                if (length>=64) throw std::runtime_error("source class name length must be below64");
                if (!length) throw std::runtime_error("empty class name outside modern scope");
                const size_t name_offset=cursor.position();const auto name=cursor.take(length);
                const auto letter=[](uint8_t c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_';};
                if (!letter(name[0])||!std::all_of(name,name+length,[&](uint8_t c){return letter(c)||(c>='0'&&c<='9');}))
                    throw std::runtime_error("class name outside modern ASCII identifier scope");
                constexpr char housing[]="cHousingGoal";
                constexpr char homage[]="cHomageGoal";
                constexpr char trading[]="cTradingPartnersGoal";
                constexpr char yearly[]="cYearlyProductionGoal";
                constexpr char population[]="cPopulationGoal";
                const bool is_housing=length==sizeof(housing)-1&&!std::memcmp(name,housing,length);
                const bool is_homage=limits.enable_homage&&length==sizeof(homage)-1&&!std::memcmp(name,homage,length);
                const bool is_trading=limits.enable_trading&&length==sizeof(trading)-1&&!std::memcmp(name,trading,length);
                const bool is_yearly=limits.enable_yearly&&length==sizeof(yearly)-1&&!std::memcmp(name,yearly,length);
                const bool is_population=limits.enable_population&&length==sizeof(population)-1&&!std::memcmp(name,population,length);
                if (!is_housing && !is_homage && !is_trading && !is_yearly && !is_population) {
                    result.pending={true,tag.offset,name_offset,length,member,ordinal,schema,
                        static_cast<uint32_t>(result.cache.size()),static_cast<uint32_t>(result.cache.size()+1)};
                    result.end=cursor.position();result.following_member=member;result.following_ordinal=ordinal;
                    result.stop_reason="unknown_class_declaration_before_lookup_and_payload";return result;
                }
                if (schema!=0) throw std::runtime_error(is_housing?"source mapped Housing schema mismatch":is_homage?"source mapped Homage schema mismatch":is_trading?"source mapped Trading schema mismatch":is_yearly?"source mapped Yearly schema mismatch":"source mapped Population schema mismatch");
                // Class is registered before expected-type check in the selected normal source.
                class_id=append({GoalCacheKind::Class,0,0,cursor.position(),is_housing?GoalClassType::Housing:is_homage?GoalClassType::Homage:is_trading?GoalClassType::Trading:is_yearly?GoalClassType::Yearly:GoalClassType::Population});
            } else if (tag.value&0x80000000) {
                class_id=tag.value&0x7fffffff;
                if (!class_id) throw std::runtime_error("source class reference zero invalid");
                if (class_id>=result.cache.size()) throw std::runtime_error("unqualified class reference");
                if (result.cache[class_id].kind!=GoalCacheKind::Class) throw std::runtime_error("modern cache kind mismatch: class expected");
            } else {
                object_id=tag.value;
                if (!object_id) throw std::runtime_error("generic null allowed; positive-member nonnull consumer unsupported");
                if (object_id>=result.cache.size()) throw std::runtime_error("unqualified object reference");
                if (result.cache[object_id].kind!=GoalCacheKind::Object) throw std::runtime_error("modern cache kind mismatch: object expected");
                class_id=result.cache[object_id].class_id;object_reference=true;
            }
            const size_t payload_offset=cursor.position();
            if (!object_reference) {
                // A new instance receives its metadata ID before its payload, never an original object.
                const auto class_type=result.cache[class_id].class_type;
                object_id=append({GoalCacheKind::Object,class_id,result.cache[class_id].schema,cursor.position(),class_type});
                if (class_type==GoalClassType::Housing) goal_housing_payload(cursor);
                else if (class_type==GoalClassType::Homage) goal_homage_payload(cursor);
                else if (class_type==GoalClassType::Trading) goal_trading_payload(cursor);
                else if (class_type==GoalClassType::Yearly) goal_yearly_payload(cursor);
                else goal_population_payload(cursor);
            }
            result.items.push_back({member,ordinal,tag.offset,payload_offset,cursor.position()-payload_offset,
                object_id,class_id,object_reference});
            ++result.known_items;result.end=cursor.position();
            const bool complete=ordinal+1==count;
            result.following_member=complete?member+1:member;
            result.following_ordinal=complete?0:ordinal+1;
            if (limits.checkpoint_items && result.known_items==limits.checkpoint_items) {
                result.completed_members=member+size_t(complete);
                result.stop_reason="known_item_checkpoint";return result;
            }
        }
        result.completed_members=member+1;result.end=cursor.position();
    }
    result.following_member=10;result.following_ordinal=0;
    result.stop_reason="ten_members_complete_next_owner_unqualified";return result;
}
}
