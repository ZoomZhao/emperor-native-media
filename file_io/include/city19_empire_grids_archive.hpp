#pragma once
#include "city19_empire_inline_archive.hpp"
#include "city19_empire_grid15_layout.hpp"
namespace emperor::file_io::research {
struct EmpireGridChild {size_t ordinal=0,start=0,end=0;uint16_t version=0;};
struct EmpireGridRecord {size_t ordinal=0,start=0,end=0;uint16_t version=0;std::vector<EmpireGridChild> children;};
struct EmpireGridPending {bool present=false,child=false;size_t grid_ordinal=0,child_ordinal=0,start=0,end=0;uint16_t version=0;};
struct EmpireGridLimits {size_t max_prefix=4915584,max_grids=22;bool first_header_only=false,stop_before_parent_tail=false;};
struct EmpireGridWalk {
    size_t start=0,end=0,cache_next=0,parent_tail_offset=0;uint16_t parent_version=0;
    bool grids_complete=false,parent_tail_qualified=false,parent_byte_requests_complete=false;
    std::vector<EmpireGridRecord> grids;EmpireGridPending pending;std::string stop_reason;
};
inline EmpireGridWalk walk_city19_empire_grids(GoalBytes bytes,const EmpireInlineWalk &before,EmpireGridLimits limits={}) {
    if(!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>4915584||!limits.max_grids||limits.max_grids>22||
       !before.inline_complete||before.records.size()!=before.count||before.end>bytes.size||before.version<1||before.version>3||
       before.following_grids_required!=(before.version>=2)||before.trailing_parent4_required!=(before.version==3))
        throw std::runtime_error("Empire grid preceding context/modern budget");
    EmpireGridWalk r;r.start=r.end=before.end;r.cache_next=before.cache_next;r.parent_version=before.version;
    if(before.version==1){r.grids_complete=r.parent_byte_requests_complete=true;r.stop_reason="Empire_source1_no_grids_postload_unexecuted";return r;}
    GoalCursor cursor(bytes,before.end);
    // 480945: EDI=c07b80; +3c00; signed CMP c5a380/JL gives22 direct43e370 calls.
    for(size_t ordinal=0;ordinal<22;++ordinal){
        if(ordinal>=limits.max_grids){r.stop_reason="modern_Empire_grid_checkpoint_before_next_header";return r;}
        EmpireGridRecord g;g.ordinal=ordinal;g.start=cursor.position();g.version=cursor.u16();r.end=cursor.position();
        if((ordinal==0&&limits.first_header_only)||g.version!=15){
            r.pending={true,false,ordinal,0,g.start,r.end,g.version};
            r.stop_reason=limits.first_header_only?"Empire_grid_header_only":"Empire_grid_outside_source15_subset";return r;
        }
        // 43fd90/43fdb0 selects43e78e for15; exact requests before and after the child loop.
        for(const auto &f:empire_grid15_pre)cursor.take(f.bytes);
        r.end=cursor.position();
        // 43e935 count16h;43e941 calls43da30 with this grid version, then +=274h/DEC/JNZ.
        for(size_t child=0;child<22;++child){
            const auto start=cursor.position();const auto version=cursor.u16();r.end=cursor.position();
            if(version!=5){r.pending={true,true,ordinal,child,start,r.end,version};r.stop_reason="Empire_grid_child_outside_source5_subset";return r;}
            for(const auto &f:empire_grid_child5_fields)cursor.take(f.bytes);
            g.children.push_back({child,start,cursor.position(),version});r.end=cursor.position();
        }
        for(const auto &f:empire_grid15_post)cursor.take(f.bytes);
        g.end=cursor.position();r.end=g.end;r.grids.push_back(std::move(g));
    }
    r.grids_complete=true;
    if(before.version==3){
        if(limits.stop_before_parent_tail){r.stop_reason="Empire_grids_complete_before_parent3_raw4";return r;}
        r.parent_tail_offset=cursor.position();cursor.take(4);r.end=cursor.position();r.parent_tail_qualified=true;
    }
    r.parent_byte_requests_complete=true;r.stop_reason="Empire_selected_byte_requests_complete_postload_unexecuted";return r;
}
}
