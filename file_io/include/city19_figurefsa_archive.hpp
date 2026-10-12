#pragma once
#include "city19_goal_archive.hpp"
#include <array>
#include <limits>

namespace emperor::file_io::research {
// This selected path does not run an original factory or infer a runtime type.
enum class FigureFsaContract { cached_refs_root7b5bc8_reader4dad10 };
enum class FigureFsaFamily { State, Transition, ActionContext };
enum class FigureFsaCacheKind { Null, Class, Object };
struct FigureFsaCacheEntry {
    FigureFsaCacheKind kind=FigureFsaCacheKind::Null;
    FigureFsaFamily family=FigureFsaFamily::State;
    uint32_t class_id=0;
    uint16_t schema=0; // Class schema is not an object's own version.
};
struct FigureFsaLimits {
    size_t max_bytes=65536,max_string=8192,max_items=256,max_refs=1024,max_cache=1024;
};
struct FigureFsaRange { size_t offset=0,bytes=0;uint32_t source_call=0; };
struct FigureFsaString {
    size_t length_offset=0,offset=0,bytes=0;uint32_t source_call=0;
};
struct FigureFsaReference {
    size_t offset=0,bytes=0;uint32_t object_id=0,source_call=0;
    FigureFsaFamily expected=FigureFsaFamily::State;
    bool null_reference=false,extended=false;
};
struct FigureFsaCount {
    size_t offset=0;int64_t declared=0;size_t iterations=0;uint32_t source_call=0;
};
struct FigureFsaEmbedded {
    size_t offset=0,end=0;uint16_t version=0;
    std::array<FigureFsaRange,4> fields{};
    FigureFsaRange flag{};
};
struct FigureFsaRecord {
    static constexpr bool verifies_original_instance=false;
    static constexpr bool accepts_new_object_payloads=false;
    size_t start=0,base_end=0,end=0;
    uint16_t base_version=0,version=0;
    bool has_current_name=false;
    std::vector<FigureFsaRange> flags;
    std::vector<FigureFsaString> strings;
    std::vector<FigureFsaReference> references;
    std::vector<FigureFsaCount> counts;
    std::array<FigureFsaEmbedded,3> embedded{};
};

inline FigureFsaRecord walk_city19_figurefsa_cached_record(
    GoalBytes bytes,FigureFsaContract contract,const std::vector<FigureFsaCacheEntry>&cache,
    size_t start=0,FigureFsaLimits limits={}) {
    if(contract!=FigureFsaContract::cached_refs_root7b5bc8_reader4dad10)
        throw std::runtime_error("FigureFSA unsupported explicit contract");
    if(!bytes.data||!limits.max_bytes||limits.max_bytes>65536||bytes.size>limits.max_bytes||
       start>bytes.size||limits.max_string>8192||limits.max_items>256||
       limits.max_refs>1024||!limits.max_cache||limits.max_cache>1024||
       cache.empty()||cache.size()>limits.max_cache||cache[0].kind!=FigureFsaCacheKind::Null)
        throw std::runtime_error("FigureFSA view/cache/budget argument");
    const auto family_valid=[](FigureFsaFamily f){return f==FigureFsaFamily::State||
        f==FigureFsaFamily::Transition||f==FigureFsaFamily::ActionContext;};
    for(size_t i=1;i<cache.size();++i) {
        const auto&e=cache[i];
        if(!family_valid(e.family)||(e.kind!=FigureFsaCacheKind::Class&&e.kind!=FigureFsaCacheKind::Object))
            throw std::runtime_error("FigureFSA invalid cache metadata");
        if(e.kind==FigureFsaCacheKind::Object&&(e.class_id==0||e.class_id>=cache.size()||
           cache[e.class_id].kind!=FigureFsaCacheKind::Class||cache[e.class_id].family!=e.family))
            throw std::runtime_error("FigureFSA object/class cache relation");
    }
    GoalCursor cursor(bytes,start);FigureFsaRecord r;r.start=start;
    const auto flag=[&](uint32_t at){const auto off=cursor.position();const auto v=*cursor.take(1);
        r.flags.push_back({off,1,at});return v;};
    const auto string=[&](uint32_t at){
        const auto length_offset=cursor.position();const uint32_t n=cursor.u32();
        if(n>limits.max_string)throw std::runtime_error("FigureFSA string budget");
        const auto off=cursor.position();cursor.take(n);r.strings.push_back({length_offset,off,n,at});
    };
    const auto count=[&](uint32_t at){
        const auto off=cursor.position();const uint32_t word=cursor.u32();
        const int64_t n=word<=0x7fffffff?int64_t(word):int64_t(word)-0x100000000LL;
        const size_t iterations=n>0?size_t(n):0;
        if(iterations>limits.max_items)throw std::runtime_error("FigureFSA count budget");
        r.counts.push_back({off,n,iterations,at});return iterations;
    };
    const auto reference=[&](FigureFsaFamily expected,uint32_t at){
        if(r.references.size()>=limits.max_refs)throw std::runtime_error("FigureFSA reference budget");
        const auto tag=goal_tag(cursor);
        if(tag.declaration||(tag.value&0x80000000))
            throw std::runtime_error("FigureFSA new class/object payload outside cached-reference contract");
        if(tag.value>=cache.size())throw std::runtime_error("FigureFSA object reference not in cache");
        if(tag.value&&(cache[tag.value].kind!=FigureFsaCacheKind::Object||cache[tag.value].family!=expected))
            throw std::runtime_error("FigureFSA cache kind/expected family mismatch");
        r.references.push_back({tag.offset,cursor.position()-tag.offset,tag.value,at,expected,
            tag.value==0,tag.word==0x7fff});
    };
    const auto refs=[&](FigureFsaFamily family,uint32_t count_at,uint32_t ref_at){
        const auto n=count(count_at);for(size_t i=0;i<n;++i)reference(family,ref_at);
    };
    r.base_version=cursor.u16(); // 40cf80 is RET4: no preceding wire bytes.
    if(r.base_version!=1&&r.base_version!=2)
        throw std::runtime_error("FigureFSA base outside selected source1/2 subset");
    string(r.base_version==2?0x4f4604:0x4f4698);
    flag(r.base_version==2?0x4f4621:0x4f46b5);
    if(r.base_version==1) {
        refs(FigureFsaFamily::Transition,0x4f46c1,0x4f46d6);
        refs(FigureFsaFamily::Transition,0x4f46eb,0x4f4700);
        reference(FigureFsaFamily::Transition,0x4f4713);
        refs(FigureFsaFamily::Transition,0x4f471f,0x4f4734);
        refs(FigureFsaFamily::Transition,0x4f4749,0x4f475e);
        refs(FigureFsaFamily::ActionContext,0x4f4773,0x4f4788);
        refs(FigureFsaFamily::ActionContext,0x4f479d,0x4f47b2);
        refs(FigureFsaFamily::State,0x4f47c7,0x4f47dc);
    }
    const std::array<uint32_t,3> direct_state=r.base_version==2?
        std::array<uint32_t,3>{{0x4f462b,0x4f4635,0x4f463f}}:
        std::array<uint32_t,3>{{0x4f47ef,0x4f47f9,0x4f4803}};
    for(auto at:direct_state)reference(FigureFsaFamily::State,at);
    refs(FigureFsaFamily::State,r.base_version==2?0x4f464b:0x4f480f,
        r.base_version==2?0x4f4660:0x4f4824);
    r.base_end=cursor.position();r.version=cursor.u16();
    if(r.version<2||r.version>5)
        throw std::runtime_error("FigureFSA own outside selected source2..5 subset");
    if(r.version!=2) {
        const uint32_t at=r.version==5?0x4daeef:r.version==4?0x4dafaf:0x4db013;
        r.has_current_name=flag(at)!=0;
        if(r.has_current_name)string(r.version==5?0x4daf02:r.version==4?0x4dafc2:0x4db026);
        if(r.version!=4) {
            const auto n=count(r.version==5?0x4daf28:0x4db04c);
            for(size_t i=0;i<n;++i)string(r.version==5?0x4daf3d:0x4db061);
        }
    }
    for(size_t i=0;i<3;++i) {
        auto&e=r.embedded[i];e.offset=cursor.position();e.version=cursor.u16();
        if(e.version!=1)throw std::runtime_error("FigureFSA embedded outside selected own1 subset");
        constexpr std::array<uint32_t,4> calls{{0x4f8d0f,0x4f8d1a,0x4f8d25,0x4f8d30}};
        for(size_t k=0;k<4;++k){const auto off=cursor.position();cursor.take(4);e.fields[k]={off,4,calls[k]};}
        const auto off=cursor.position();cursor.take(1);
        e.flag={off,1,r.version==2?0x4db0e5u:r.version==4?0x4daffcu:r.version==3?0x4db0bcu:0x4daf98u};
        e.end=cursor.position();
    }
    r.end=cursor.position();return r;
}
}
