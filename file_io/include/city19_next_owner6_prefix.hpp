#pragma once
#include "city19_goal_archive.hpp"
#include "city19_next_owner6_layout.hpp"

namespace emperor::file_io::research {
struct NextOwner6Range {size_t offset=0,bytes=0;uint32_t source_call=0;};
struct NextOwner6Prefix {
    size_t start=0,end=0;
    uint16_t version=0;
    std::vector<NextOwner6Range> fields;
    bool pending_embedded_reader=true;
    size_t embedded_bytes_consumed=0;
    uint32_t pending_source_call=0x52d5f6;
};

// Standalone own6 prefix only. The embedded virtual0 consumer is deliberately
// pending. No original receiver reset, state write, or callee is invoked.
inline NextOwner6Prefix walk_city19_next_owner6_prefix(GoalBytes bytes,size_t start=0,size_t max_prefix=4096){
    if(!bytes.data||bytes.size>max_prefix||max_prefix>4096||start>bytes.size)
        throw std::runtime_error("next owner6 modern bounded view");
    GoalCursor cursor(bytes,start);
    NextOwner6Prefix r;r.start=start;r.version=cursor.u16();
    if(r.version!=6)
        throw std::runtime_error("next owner6 outside selected source6 subset, not original version rejection");
    for(const auto &request:next_owner6_requests){
        auto offset=cursor.position();cursor.take(request.bytes);
        r.fields.push_back({offset,request.bytes,request.source_call});
    }
    r.end=cursor.position();return r;
}
}
