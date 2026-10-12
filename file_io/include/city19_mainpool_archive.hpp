#pragma once
#include "city19_religion_archive.hpp"
#include "city19_building5_layout.hpp"

namespace emperor::file_io::research {
enum class PoolDomain { EarlierGoal, Building };
struct PoolCacheEntry {GoalCacheKind kind;uint32_t class_id=0;uint16_t schema=0;PoolDomain domain=PoolDomain::EarlierGoal;};
struct PoolItem {size_t ordinal=0,tag_offset=0,payload_offset=0,payload_bytes=0;uint32_t class_id=0,object_id=0;bool object_reference=false;};
struct PoolPending {bool present=false;size_t tag_offset=0,name_offset=0,name_bytes=0,ordinal=0;uint16_t schema=0;uint32_t class_id_if_resolved=0,object_id_if_factory_returns=0;};
struct PoolLimits {size_t max_prefix=1048576,max_count=8192,max_items=4096,max_cache=8192;};
struct PoolWalk {size_t start=0,end=0;uint16_t version=0;uint32_t count=0;size_t cache_start=0;std::vector<PoolCacheEntry> cache;std::vector<PoolItem> items;PoolPending pending;bool complete=false;std::string stop_reason;};
inline PoolWalk walk_city19_mainpool(GoalBytes bytes,const GoalWalk &goals,const ReligionFollowingWalk &before,PoolLimits limits={}) {
    if (!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>1048576||!limits.max_count||limits.max_count>8192||
        !limits.max_items||limits.max_items>4096||!limits.max_cache||limits.max_cache>8192||!before.commerce_qualified||
        before.end>bytes.size||goals.completed_members!=10||goals.cache.empty()) throw std::runtime_error("mainpool preceding context/budget outside modern scope");
    GoalCursor cursor(bytes,before.end);PoolWalk result;result.start=cursor.position();
    // Same archive S, no new cache constructor. Earlier goal classes deriveGoalBase,
    // not expectedBuilding817890, and cannot be treated as pool object classes.
    for (const auto &e:goals.cache) result.cache.push_back({e.kind,e.class_id,e.schema,PoolDomain::EarlierGoal});
    result.cache_start=result.cache.size();
    if (result.cache.size()>limits.max_cache) throw std::runtime_error("mainpool carried cache exceeds modern budget");
    const auto append=[&](PoolCacheEntry e) {
        if (result.cache.size()>=limits.max_cache) throw std::runtime_error("mainpool modern cache budget exhausted");
        auto id=static_cast<uint32_t>(result.cache.size());result.cache.push_back(e);return id;
    };
    result.version=cursor.u16();
    if (result.version!=1) throw std::runtime_error("pool section version outside qualified source1 subset");
    result.count=cursor.u32();
    if (result.count>0x7fffffff) throw std::runtime_error("negative pool count outside modern scope");
    if (result.count>limits.max_count) throw std::runtime_error("pool count exceeds modern budget, not original upper bound");
    result.end=cursor.position();
    for (uint32_t ordinal=0;ordinal<result.count;++ordinal) {
        if (result.items.size()>=limits.max_items) {
            result.stop_reason="modern_item_budget_checkpoint";return result;
        }
        const auto tag=goal_tag(cursor);uint32_t class_id=0,object_id=0;bool object_reference=false;
        if (tag.declaration) {
            const uint16_t schema=cursor.u16(),length=cursor.u16();
            if (!length||length>=64) throw std::runtime_error("pool class name length outside modern source subset");
            const auto offset=cursor.position();const auto name=cursor.take(length);
            const auto letter=[](uint8_t c){return(c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';};
            if (!letter(name[0])||!std::all_of(name,name+length,[&](uint8_t c){return letter(c)||(c>='0'&&c<='9');}))
                throw std::runtime_error("pool class name outside modern ASCII identifier subset");
            constexpr char building[]="Building";
            if (length!=sizeof(building)-1||std::memcmp(name,building,length)) {
                result.pending={true,tag.offset,offset,length,ordinal,schema,
                    static_cast<uint32_t>(result.cache.size()),static_cast<uint32_t>(result.cache.size()+1)};
                result.end=cursor.position();result.stop_reason="unknown_pool_class_before_lookup_and_payload";return result;
            }
            if (schema!=0) throw std::runtime_error("source mapped Building schema mismatch");
            class_id=append({GoalCacheKind::Class,0,0,PoolDomain::Building});
        } else if (tag.value&0x80000000) {
            class_id=tag.value&0x7fffffff;
            if (!class_id||class_id>=result.cache.size()) throw std::runtime_error("unqualified pool class reference");
            if (result.cache[class_id].kind!=GoalCacheKind::Class) throw std::runtime_error("modern pool cache kind mismatch class expected");
            if (result.cache[class_id].domain!=PoolDomain::Building) throw std::runtime_error("expected Building incompatible with earlier goal class");
        } else {
            object_id=tag.value;
            if (!object_id) throw std::runtime_error("generic null allowed; pool nonnull consumer unsupported");
            if (object_id>=result.cache.size()) throw std::runtime_error("unqualified pool object reference");
            if (result.cache[object_id].kind!=GoalCacheKind::Object) throw std::runtime_error("modern pool cache kind mismatch object expected");
            if (result.cache[object_id].domain!=PoolDomain::Building) throw std::runtime_error("expected Building incompatible with earlier goal object");
            class_id=result.cache[object_id].class_id;object_reference=true;
        }
        const size_t payload=cursor.position();
        if (!object_reference) {
            object_id=append({GoalCacheKind::Object,class_id,0,PoolDomain::Building});
            if (cursor.u16()!=5) throw std::runtime_error("Building field section outside qualified source5 subset");
            for (const auto &field:building5_fields) cursor.take(field.bytes);
        }
        result.items.push_back({ordinal,tag.offset,payload,cursor.position()-payload,class_id,object_id,object_reference});
        result.end=cursor.position();
    }
    // Source fills storage to4000 with new defaults for small counts; no stream
    // bytes are consumed by that fill. We create no original or default objects.
    result.complete=true;result.stop_reason="declared_pool_items_complete_next_consumer_unqualified";
    return result;
}
}
