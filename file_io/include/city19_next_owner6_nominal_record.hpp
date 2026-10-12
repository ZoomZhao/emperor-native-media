#pragma once
#include "city19_next_owner6_prefix.hpp"
#include <utility>

namespace emperor::file_io::research {
// A selected source contract, not a pointer obtained from a save or live process.
enum class NextOwner6EmbeddedContract { nominal_52c480 };
struct NextOwner6NominalRecord {
    static constexpr bool verifies_original_instance=false;
    size_t start=0,prefix_end=0,embedded_begin=0,end=0;
    uint16_t parent_version=0,embedded_version=0;
    std::vector<NextOwner6Range> parent_fields;
    std::array<NextOwner6Range,2> embedded_fields{};
};

// Standalone selected own6 + embedded own1 wire model. Binding the original
// +5c4 receiver to this contract requires construction, root preservation,
// valid lifetime/storage, normal callees and the documented alias conditions.
// This API does not assert those conditions for an original instance.
inline NextOwner6NominalRecord walk_city19_next_owner6_nominal_record(
    GoalBytes bytes,NextOwner6EmbeddedContract contract,size_t start=0,size_t max_prefix=4096){
    if(contract!=NextOwner6EmbeddedContract::nominal_52c480)
        throw std::runtime_error("next owner6 unsupported modern embedded contract");
    auto prefix=walk_city19_next_owner6_prefix(bytes,start,max_prefix);
    NextOwner6NominalRecord r;
    r.start=start;r.prefix_end=prefix.end;r.embedded_begin=prefix.end;
    r.parent_version=prefix.version;r.parent_fields=std::move(prefix.fields);
    GoalCursor cursor(bytes,prefix.end);
    r.embedded_version=cursor.u16();
    if(r.embedded_version!=1)
        throw std::runtime_error("next owner6 embedded outside selected own1 subset, not original version rejection");
    const std::array<std::pair<size_t,uint32_t>,2> requests{{{4,0x52c4cf},{12,0x52c4dc}}};
    for(size_t i=0;i<requests.size();++i){
        auto offset=cursor.position();cursor.take(requests[i].first);
        r.embedded_fields[i]={offset,requests[i].first,requests[i].second};
    }
    r.end=cursor.position();return r;
}
}
