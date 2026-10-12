#pragma once
#include "city19_goal_archive.hpp"
#include <utility>

namespace emperor::file_io::research {
struct PostGoalField {
    size_t offset=0, bytes=0;
    uint32_t receiver_offset=0;
    const char *source_call=nullptr;
};
struct PostGoalSection {
    const char *name=nullptr;
    const char *reader=nullptr;
    size_t start=0, end=0;
    uint16_t version=0;
    uint16_t embedded_version=0;
    size_t embedded_version_offset=0;
    std::vector<PostGoalField> fields;
};
struct PostGoalWalk {
    size_t start=0, end=0;
    std::vector<PostGoalSection> sections;
    std::string stop_reason;
};
// Modern immutable structural qualifier; original instances/callbacks are never created.
// The old Goal component keeps its 16384-byte input ceiling; this later section has
// a separate modern 65536-byte ceiling to cover source's fixed 0xcb10-byte request.
inline PostGoalWalk walk_city19_postgoals(GoalBytes bytes,const GoalWalk &goals,bool include_festival=false) {
    if (!bytes.data || bytes.size>65536 || goals.completed_members!=10 ||
        goals.following_member!=10 || goals.members.size()!=10 || goals.pending.present ||
        goals.stop_reason!="ten_members_complete_next_owner_unqualified" ||
        goals.end>bytes.size) throw std::runtime_error("post-goal prefix/baseline outside modern scope");
    GoalCursor cursor(bytes,goals.end);PostGoalWalk result;
    result.start=cursor.position();
    PostGoalSection flood;flood.name="cFlood";flood.reader="0x4edda0";flood.start=cursor.position();
    // 40cf80 is the previously qualified RET4/no-byte base method. The source
    // signed-version1 branch is fully decoded under a caller-declared function seed.
    flood.version=cursor.u16();
    if (flood.version!=1) throw std::runtime_error("Flood section version outside qualified source1 subset");
    const auto field=[&](size_t n,uint32_t receiver,const char *call) {
        const size_t offset=cursor.position();cursor.take(n);flood.fields.push_back({offset,n,receiver,call});
    };
    field(4,0x4,"0x4ede36");field(1,0x8,"0x4ede43");
    field(4,0xc,"0x4ede50");field(4,0x10,"0x4ede5d");
    field(4,0x14,"0x4ede6a");field(0xcb10,0x18,"0x4ede7a");
    flood.end=cursor.position();result.sections.push_back(std::move(flood));
    result.end=cursor.position();
    result.stop_reason="next_postgoal_consumer_before_body";
    if (!include_festival) return result;
    PostGoalSection festival;festival.name="cFestival";festival.reader="0x48ed30";
    festival.start=cursor.position();festival.version=cursor.u16();
    if (festival.version!=4) throw std::runtime_error("Festival section version outside qualified source4 subset");
    // Source48ed61 invokes member+self4's slot8=4f8ca0 on the same archive.
    festival.embedded_version_offset=cursor.position();festival.embedded_version=cursor.u16();
    if (festival.embedded_version!=1) throw std::runtime_error("Festival embedded member version outside qualified source1 subset");
    const auto festival_field=[&](size_t n,uint32_t receiver,const char *call) {
        const size_t offset=cursor.position();cursor.take(n);festival.fields.push_back({offset,n,receiver,call});
    };
    // Child-relative4/8/c/10 become parent-relative8/c/10/14. 4c95e0->42dc20
    // is the fully qualified 4-byte scalar reader; its return is S, not a field value.
    festival_field(4,0x8,"0x4f8d0f");festival_field(4,0xc,"0x4f8d1a");
    festival_field(4,0x10,"0x4f8d25");festival_field(4,0x14,"0x4f8d30");
    // Literal48f128 table maps own version4 to48ee8e. These are request widths
    // and exact order; source memory padding and version1/2/3 layouts are excluded.
    constexpr const char *eight_calls[]={"0x48ee96","0x48eea3","0x48eeb0","0x48eebd",
        "0x48eeca","0x48eed7","0x48eee4","0x48eef1"};
    for (uint32_t i=0;i<8;++i) festival_field(4,0x28+4*i,eight_calls[i]);
    festival_field(4,0x18,"0x48eefe");festival_field(4,0x1c,"0x48ef0b");
    festival_field(1,0x20,"0x48ef18");festival_field(1,0x21,"0x48ef25");
    festival_field(4,0x24,"0x48ef32");festival_field(1,0x50,"0x48ef3f");
    festival_field(4,0x54,"0x48ef4c");festival_field(4,0x58,"0x48ef59");
    festival_field(4,0x48,"0x48ef63");festival_field(4,0x4c,"0x48f110");
    festival.end=cursor.position();result.sections.push_back(std::move(festival));
    result.end=cursor.position();
    return result;
}
}
