#pragma once
#include "city19_empire_grids_archive.hpp"
#include "city19_following_fixed4_layout.hpp"
namespace emperor::file_io::research {
struct FollowingFixedRecord {size_t ordinal=0,start=0,end=0;uint16_t version=0;bool full_record_qualified=false;};
struct FollowingFixedLimits {size_t max_prefix=4915584,max_records=12;bool inspect_next_header=true;};
struct FollowingFixedWalk {
    size_t start=0,end=0,cache_next=0,pending_ordinal=0,next_header_start=0;uint16_t pending_version=0,next_version=0;
    bool complete=false,pending_version_present=false,pending_extension=false,next_header_present=false;
    std::vector<FollowingFixedRecord> records;std::string stop_reason;
};
inline FollowingFixedWalk walk_city19_following_fixed(GoalBytes bytes,const EmpireGridWalk &before,FollowingFixedLimits limits={}) {
    if(!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>4915584||!limits.max_records||limits.max_records>12||
       !before.parent_byte_requests_complete||before.end>bytes.size||before.parent_version<1||before.parent_version>3||
       !before.grids_complete||before.grids.size()!=(before.parent_version>=2?22u:0u)||
       before.parent_tail_qualified!=(before.parent_version==3))throw std::runtime_error("following fixed preceding context/modern budget");
    GoalCursor cursor(bytes,before.end);FollowingFixedWalk r;r.start=r.end=before.end;r.cache_next=before.cache_next;
    // 510e6e..510e8f: [10bfb30,10bfdd0), step38h gives12 direct511ea0 calls, no list-count header.
    for(size_t i=0;i<12;++i){
        if(i>=limits.max_records){r.stop_reason="modern_following_fixed_checkpoint_before_next_word";return r;}
        const auto start=cursor.position();const auto v=cursor.u16();r.end=cursor.position();
        if(v!=4){r.pending_version_present=true;r.pending_ordinal=i;r.pending_version=v;r.stop_reason="following_fixed_outside_source4_subset";return r;}
        for(const auto &f:following_fixed4_fields)cursor.take(f.bytes);
        const bool extension=*cursor.take(1)!=0;r.end=cursor.position();r.records.push_back({i,start,r.end,v,!extension});
        if(extension){r.pending_extension=true;r.pending_ordinal=i;r.stop_reason="following_fixed_nonzero_presence_before_unqualified_virtual8";return r;}
    }
    r.complete=true;
    if(limits.inspect_next_header){r.next_header_start=cursor.position();r.next_version=cursor.u16();r.next_header_present=true;r.end=cursor.position();r.stop_reason="following_fixed_complete_next_vector_header_only";}
    else r.stop_reason="following_fixed_complete_before564e30";
    return r;
}
}
