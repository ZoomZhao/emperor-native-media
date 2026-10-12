#pragma once
#include "city19_figure_variants_archive.hpp"
#include "city19_goods1_layout.hpp"
namespace emperor::file_io::research {
struct GoodsRecord {size_t ordinal=0,start=0,end=0;uint16_t version=0;};
struct GoodsLimits {size_t max_prefix=4915584,max_count=8192,max_items=8192;};
struct GoodsWalk {size_t start=0,end=0,cache_next=0;uint16_t version=0;uint32_t count=0;bool complete=false;std::vector<GoodsRecord> records;std::string stop_reason;};
inline GoodsWalk walk_city19_goodscontrol(GoalBytes bytes,const AfterPoolWalk &before,GoodsLimits limits={}){
    if(!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>4915584||!limits.max_count||limits.max_count>8192||
       !limits.max_items||limits.max_items>8192||!before.figures_complete||before.pending.present||before.extension.present||
       before.figures.size()!=before.count||before.cache.empty()||before.end>bytes.size)
        throw std::runtime_error("GoodsControl preceding context/modern budget");
    GoalCursor cursor(bytes,before.end);GoodsWalk r;r.start=before.end;r.cache_next=before.cache.size();
    r.version=cursor.u16();if(r.version!=1)throw std::runtime_error("GoodsControl list outside qualified source1 subset");
    //5067a5 explicitly requests4 into stack count; signed TEST/JLE, returncount ignored.
    r.count=cursor.u32();if(r.count>0x7fffffff)throw std::runtime_error("negative GoodsControl count outside modern scope");
    if(r.count>limits.max_count)throw std::runtime_error("GoodsControl count exceeds modern budget, not source bound");
    r.end=cursor.position();
    for(uint32_t ordinal=0;ordinal<r.count;++ordinal){
        if(r.records.size()>=limits.max_items){r.stop_reason="modern_GoodsControl_item_checkpoint";return r;}
        const size_t start=cursor.position();const uint16_t version=cursor.u16();
        if(version!=1)throw std::runtime_error("GoodsControl record outside qualified source1 subset");
        for(const auto &f:goods1_fields)cursor.take(f.bytes);
        r.records.push_back({ordinal,start,cursor.position(),version});r.end=cursor.position();
    }
    // Plain preconstructed inline objects, no class/object tags or archive-ID append.
    r.complete=true;r.stop_reason="GoodsControl_complete_before_unqualified_Empire";return r;
}
}
