#pragma once
#include "city19_goal_archive.hpp"
#include "city19_later7_layout.hpp"
namespace emperor::file_io::research {
struct Later7FieldRange {size_t offset=0,bytes=0;uint32_t source_call=0;};
struct Later7Walk {size_t start=0,end=0;uint16_t version=0;std::vector<Later7FieldRange> fields;};
// Standalone source7 byte-request model. It does not reach this consumer in a real save,
// invoke original state writes, or bind preceding virtual/object consumers.
inline Later7Walk walk_city19_later7_requests(GoalBytes bytes,size_t start=0,size_t max_prefix=16384){
    if(!bytes.data||bytes.size>max_prefix||max_prefix>16384||start>bytes.size)
        throw std::runtime_error("later7 modern bounded view");
    GoalCursor cursor(bytes,start);Later7Walk r;r.start=start;r.version=cursor.u16();
    if(r.version!=7)throw std::runtime_error("later7 outside selected source7 subset,not original version rejection");
    for(const auto &request:later7_requests){
        const auto offset=cursor.position();cursor.take(request.bytes);r.fields.push_back({offset,request.bytes,request.source_call});
    }
    r.end=cursor.position();return r;
}
}
