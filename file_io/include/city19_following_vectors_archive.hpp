#pragma once
#include "city19_following_fixed_archive.hpp"
namespace emperor::file_io::research {
struct FollowingVectorRecord {size_t ordinal=0,start=0,end=0;uint16_t version=0;std::vector<uint32_t> counts;};
struct FollowingVectorLimits {size_t max_prefix=4915584,max_records=27,max_count=8192;bool inspect_next_header=true;};
struct FollowingVectorWalk {
    size_t start=0,end=0,cache_next=0,pending_group=0,next_header_start=0;uint16_t next_version=0;
    bool complete=false,pending_present=false,pending_objects=false,next_header_present=false;
    std::vector<FollowingVectorRecord> records;FollowingVectorRecord pending;std::string stop_reason;
};
inline FollowingVectorWalk walk_city19_following_vectors(GoalBytes bytes,const FollowingFixedWalk &before,FollowingVectorLimits limits={}) {
    if(!bytes.data||bytes.size>limits.max_prefix||limits.max_prefix>4915584||!limits.max_records||limits.max_records>27||
       !limits.max_count||limits.max_count>8192||!before.complete||before.records.size()!=12||!before.next_header_present||
       before.end>bytes.size||before.next_header_start>before.end||before.end-before.next_header_start!=2||
       before.pending_extension||before.pending_version_present)throw std::runtime_error("following vectors preceding context/modern budget");
    GoalCursor cursor(bytes,before.end);FollowingVectorWalk r;r.start=before.next_header_start;r.end=before.end;r.cache_next=before.cache_next;
    // 564e36..564e4f: [12a5dd0,12a6d00), destination step90h gives27 direct56ab00 calls.
    for(size_t i=0;i<27;++i){
        if(i>=limits.max_records){r.stop_reason="modern_following_vector_checkpoint_before_next_word";return r;}
        FollowingVectorRecord record;record.ordinal=i;record.start=i?cursor.position():before.next_header_start;
        record.version=i?cursor.u16():before.next_version;r.end=cursor.position();
        if(record.version!=1){record.end=r.end;r.pending=std::move(record);r.pending_present=true;r.stop_reason="following_vector_outside_source1_subset";return r;}
        // Original56a970 clears old vectors, with virtual destruction/erase effects still conditional.
        // Selected source1 then requests six signed raw4 counts; zero skips each77fd90 object reader.
        for(size_t group=0;group<6;++group){
            const auto count=cursor.u32();record.counts.push_back(count);r.end=cursor.position();
            if(count>0x7fffffff)throw std::runtime_error("negative vector count outside modern subset");
            if(count>limits.max_count)throw std::runtime_error("vector count exceeds modern budget,not original upper bound");
            if(count){record.end=r.end;r.pending=std::move(record);r.pending_present=r.pending_objects=true;r.pending_group=group;r.stop_reason="following_vector_nonzero_count_before_unqualified_77fd90_tag";return r;}
        }
        record.end=r.end;r.records.push_back(std::move(record));
    }
    r.complete=true;
    if(limits.inspect_next_header){r.next_header_start=cursor.position();r.next_version=cursor.u16();r.next_header_present=true;r.end=cursor.position();r.stop_reason="following_vectors_complete_next593140_header_only";}
    else r.stop_reason="following_vectors_complete_before593140";
    return r;
}
}
