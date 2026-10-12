#pragma once
#include "city19_postgoals_archive.hpp"

namespace emperor::file_io::research {
struct ReligionEmbedded {
    size_t start=0,end=0;
    uint16_t version=0;
    std::vector<PostGoalField> fields;
};
struct ReligionChild {
    size_t start=0,end=0;
    uint16_t version=0;
    std::vector<ReligionEmbedded> embedded;
};
struct ReligionFollowingWalk {
    size_t start=0,end=0;
    PostGoalSection religion;
    std::vector<ReligionChild> children;
    PostGoalSection commerce;
    bool commerce_qualified=false;
    std::string stop_reason;
};
// A byte/metadata qualifier only. No original pointer, factory, callback, legacy
// numeric adjustment, or city update code is executed or reproduced here.
inline ReligionFollowingWalk walk_city19_religion_following(
    GoalBytes bytes,const PostGoalWalk &post,bool include_commerce=true) {
    if (!bytes.data || bytes.size>65536 || post.sections.size()!=2 ||
        post.sections[0].name==nullptr || post.sections[1].name==nullptr ||
        std::string(post.sections[0].name)!="cFlood" || std::string(post.sections[1].name)!="cFestival" ||
        post.end!=post.sections.back().end || post.end>bytes.size)
        throw std::runtime_error("religion preceding segment/budget outside modern scope");
    GoalCursor cursor(bytes,post.end);ReligionFollowingWalk result;result.start=cursor.position();
    auto &section=result.religion;section.name="cReligion";section.reader="0x5a7b90";
    section.start=cursor.position();section.version=cursor.u16();
    if (section.version!=1) throw std::runtime_error("Religion section version outside qualified source1 subset");
    // Constructor5a7940 creates four distinct child pointers through5a7010.
    // Reader5a7bc6 services each +8 on the same S, before its own raw20 block.
    constexpr const char *scalar_calls[]={"0x51008d","0x510098","0x5100a3","0x5100ae","0x5100b9"};
    for (size_t child_index=0;child_index<4;++child_index) {
        ReligionChild child;child.start=cursor.position();child.version=cursor.u16();
        if (child.version!=3) throw std::runtime_error("Worship child version outside qualified source3 subset");
        // Source3 skips the version<=2 post-read numeric adjustment. Three
        // embedded receivers begin at child+28 with stride18, reader510010.
        for (size_t i=0;i<3;++i) {
            ReligionEmbedded entry;entry.start=cursor.position();entry.version=cursor.u16();
            if (entry.version!=2) throw std::runtime_error("Worship embedded version outside qualified source2 subset");
            for (uint32_t f=0;f<5;++f) {
                const size_t offset=cursor.position();cursor.take(4);
                entry.fields.push_back({offset,4,0x4+4*f,scalar_calls[f]});
            }
            entry.end=cursor.position();child.embedded.push_back(std::move(entry));
        }
        child.end=cursor.position();result.children.push_back(std::move(child));
    }
    // Complete selected native+c/+10 getters return self+48 and literal20.
    // This raw memory representation remains opaque, including any pointer bytes.
    const size_t raw=cursor.position();cursor.take(20);section.fields.push_back({raw,20,0x48,"0x5a7c08"});
    section.end=cursor.position();result.end=section.end;
    result.stop_reason="fourth_consumer_before_section";
    if (!include_commerce) return result;
    auto &commerce=result.commerce;commerce.name="cCommerce";commerce.reader="0x40cf40";
    commerce.start=cursor.position();commerce.version=cursor.u16();
    if (commerce.version!=1) throw std::runtime_error("Commerce section version outside modern source1 subset");
    // 40cf40 calls no-byte40cf80, reads88f180, sets1 in readmode, RET4: no fields.
    commerce.end=cursor.position();result.end=commerce.end;result.commerce_qualified=true;
    result.stop_reason="mainpool_before_section_and_objects";
    return result;
}
}
