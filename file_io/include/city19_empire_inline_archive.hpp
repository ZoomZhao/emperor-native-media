#pragma once
#include "city19_goodscontrol_archive.hpp"
#include "city19_empire2_layout.hpp"
namespace emperor::file_io::research {
struct EmpireInlineRecord{size_t ordinal=0,start=0,end=0;uint16_t version=0;};
struct EmpireInlineLimits{size_t max_prefix=4915584,max_count=8192,max_items=8192;bool header_only=false;};
struct EmpireInlineWalk{size_t start=0,end=0,cache_next=0;uint16_t version=0;uint32_t count=0;bool inline_complete=false,following_grids_required=false,trailing_parent4_required=false;std::vector<EmpireInlineRecord> records;std::string stop_reason;};
inline EmpireInlineWalk walk_city19_empire_inline(GoalBytes bytes,const GoodsWalk &before,EmpireInlineLimits limits={}){
    if(!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>4915584||!limits.max_count||limits.max_count>8192||
       !limits.max_items||limits.max_items>8192||!before.complete||before.records.size()!=before.count||before.end>bytes.size)
        throw std::runtime_error("Empire preceding context/modern budget");
    GoalCursor cursor(bytes,before.end);EmpireInlineWalk r;r.start=before.end;r.cache_next=before.cache_next;
    r.version=cursor.u16();
    if(r.version<1||r.version>3)throw std::runtime_error("Empire parent outside qualified source1/2/3 subset");
    //All three selected branches request4 into local count, then signed TEST/JLE.
    r.count=cursor.u32();if(r.count>0x7fffffff)throw std::runtime_error("negative Empire inline count outside modern scope");
    if(r.count>limits.max_count)throw std::runtime_error("Empire inline count exceeds modern budget, not source bound");
    r.end=cursor.position();r.following_grids_required=r.version>=2;r.trailing_parent4_required=r.version==3;
    if(limits.header_only){r.stop_reason="Empire_header_only_before_inline_root_dependency";return r;}
    for(uint32_t ordinal=0;ordinal<r.count;++ordinal){
        if(r.records.size()>=limits.max_items){r.stop_reason="modern_Empire_inline_item_checkpoint";return r;}
        const auto start=cursor.position();const auto version=cursor.u16();
        if(version!=2)throw std::runtime_error("EmpireObject outside qualified explicit source2 subset");
        for(const auto &f:empire2_fields)cursor.take(f.bytes);
        r.records.push_back({ordinal,start,cursor.position(),version});r.end=cursor.position();
    }
    r.inline_complete=true;
    r.stop_reason=r.following_grids_required?"Empire_inline_complete_before_first43e370_grid":"Empire_source1_byte_requests_complete_postload_not_executed";
    return r;
}
}
