#pragma once
#include "city19_after_mainpool_archive.hpp"
namespace emperor::file_io::research {
// Exact class->layout selection: cPheasant prefix2 BEFORE Figure7; default remains Figure-only.
inline AfterPoolWalk walk_city19_figure_variants(GoalBytes bytes,const PoolWalk &before,AfterPoolLimits limits={},bool enable_pheasant=false) {
    if(!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>4915584||!limits.max_count||limits.max_count>16384||
       !limits.max_items||limits.max_items>8192||!limits.max_cache||limits.max_cache>32768||!before.complete||before.pending.present||
       before.items.size()!=before.count||before.cache.empty()||before.end>bytes.size)
        throw std::runtime_error("afterpool preceding context/modern budget");
    GoalCursor cursor(bytes,before.end);AfterPoolWalk result;result.start=before.end;
    for(const auto &e:before.cache)result.cache.push_back({e.kind,e.class_id,e.schema,e.domain==PoolDomain::Building?AfterPoolDomain::Building:AfterPoolDomain::EarlierGoal,true});
    std::vector<bool> class_is_pheasant(result.cache.size(),false);
    result.cache_start=result.cache.size();
    if(result.cache.size()>limits.max_cache)throw std::runtime_error("afterpool carried cache exceeds modern budget");
    const auto append=[&](AfterPoolCacheEntry e){
        if(result.cache.size()>=limits.max_cache)throw std::runtime_error("afterpool modern cache budget exhausted");
        auto id=static_cast<uint32_t>(result.cache.size());result.cache.push_back(e);class_is_pheasant.push_back(false);return id;
    };
    result.version=cursor.u16();
    if(result.version!=1)throw std::runtime_error("Figure list section outside qualified source1 subset");
    result.count=cursor.u32();
    if(result.count>0x7fffffff)throw std::runtime_error("negative Figure count outside modern scope");
    if(result.count>limits.max_count)throw std::runtime_error("Figure count exceeds modern budget, not original bound");
    result.end=cursor.position();
    for(uint32_t ordinal=0;ordinal<result.count;++ordinal) {
        if(result.figures.size()>=limits.max_items){result.stop_reason="modern_Figure_item_budget_checkpoint";return result;}
        const auto tag=goal_tag(cursor);uint32_t class_id=0,object_id=0;bool object_reference=false;
        if(tag.declaration){
            const uint16_t schema=cursor.u16(),length=cursor.u16();
            if(!length||length>=64)throw std::runtime_error("Figure name length outside modern subset");
            const auto offset=cursor.position();const auto name=cursor.take(length);
            const auto letter=[](uint8_t ch){return(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||ch=='_';};
            if(!letter(name[0])||!std::all_of(name,name+length,[&](uint8_t ch){return letter(ch)||(ch>='0'&&ch<='9');}))
                throw std::runtime_error("Figure name outside modern ASCII subset");
            constexpr char literal[]="Figure",pheasant[]="cPheasantFig";
            const bool is_figure=length==sizeof(literal)-1&&!std::memcmp(name,literal,length);
            const bool is_pheasant=enable_pheasant&&length==sizeof(pheasant)-1&&!std::memcmp(name,pheasant,length);
            if(!is_figure&&!is_pheasant){
                result.pending={true,tag.offset,offset,length,ordinal,schema,static_cast<uint32_t>(result.cache.size()),static_cast<uint32_t>(result.cache.size()+1)};
                result.end=cursor.position();result.stop_reason="unknown_Figure_class_before_lookup_payload";return result;
            }
            if(schema!=0)throw std::runtime_error("Figure mapped static schema mismatch");
            class_id=append({GoalCacheKind::Class,0,0,AfterPoolDomain::Figure,true});
            class_is_pheasant[class_id]=is_pheasant;
        }else if(tag.value&0x80000000){
            class_id=tag.value&0x7fffffff;
            if(!class_id||class_id>=result.cache.size())throw std::runtime_error("unqualified Figure class reference");
            const auto &e=result.cache[class_id];
            if(e.kind!=GoalCacheKind::Class)throw std::runtime_error("modern Figure cache kind mismatch class expected");
            if(e.domain!=AfterPoolDomain::Figure)throw std::runtime_error("expected Figure incompatible with earlier Goal/Building class");
        }else{
            object_id=tag.value;
            if(!object_id)throw std::runtime_error("generic null allowed; Figure nonnull consumer outside modern scope");
            if(object_id>=result.cache.size())throw std::runtime_error("unqualified Figure object reference");
            const auto &e=result.cache[object_id];
            if(e.kind!=GoalCacheKind::Object)throw std::runtime_error("modern Figure cache kind mismatch object expected");
            if(e.domain!=AfterPoolDomain::Figure||!e.payload_qualified)throw std::runtime_error("expected Figure incompatible or incomplete cached object");
            class_id=e.class_id;object_reference=true;
        }
        const size_t payload=cursor.position();
        if(!object_reference){
            object_id=append({GoalCacheKind::Object,class_id,0,AfterPoolDomain::Figure,false});
            if(class_is_pheasant[class_id]&&cursor.u16()!=2)throw std::runtime_error("cPheasantFig own section outside qualified source2 subset");
            if(cursor.u16()!=7)throw std::runtime_error("Figure field section outside qualified source7 subset");
            for(const auto &field:figure7_fields)cursor.take(field.bytes);
            //503d50 canonicalizes any nonzero wire byte. This is optional-object
            //presence, not an activity interpretation. Unknown extension is not skipped.
            const bool extension_present=*cursor.take(1)!=0;
            if(extension_present){result.extension={true,ordinal,payload,cursor.position(),object_id};result.end=cursor.position();result.stop_reason="Figure_extension_before_unqualified_4d72f0_payload";return result;}
            result.cache[object_id].payload_qualified=true;
        }
        result.figures.push_back({ordinal,tag.offset,payload,cursor.position()-payload,class_id,object_id,object_reference});
        result.end=cursor.position();
    }
    result.figures_complete=true;result.stop_reason="Figure_list_complete_next_consumer_unqualified";return result;
}
}
