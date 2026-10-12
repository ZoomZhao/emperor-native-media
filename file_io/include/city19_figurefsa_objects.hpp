#pragma once
#include "city19_figurefsa_archive.hpp"

namespace emperor::file_io::research {
enum class FigureFsaWireType { State, Transition, ActionContext, Action, Context, Match, OpaquePrefix };
struct FigureFsaWireCacheEntry {
    FigureFsaCacheKind kind=FigureFsaCacheKind::Null;
    FigureFsaWireType family=FigureFsaWireType::State;
    uint32_t class_id=0;
    uint16_t schema=0;
};
struct FigureFsaWireReference {
    size_t offset=0, bytes=0;
    uint32_t object_id=0, source_call=0;
    FigureFsaWireType expected=FigureFsaWireType::State;
    bool null_reference=false, extended=false;
};
enum class FigureFsaObjectContract { exact_fsa_classes_root7b5bc8_reader4dad10 };
struct FigureFsaObjectLimits : FigureFsaLimits {
    size_t max_objects=128, max_depth=16, max_strings=1024;
};
struct FigureFsaClassDeclaration {
    size_t tag_offset=0, name_offset=0, name_bytes=0;
    uint32_t class_id=0;
    uint16_t schema=0;
    FigureFsaWireType family=FigureFsaWireType::State;
};
struct FigureFsaObjectPayload {
    size_t offset=0, end=0;
    uint32_t object_id=0, class_id=0;
    FigureFsaWireType family=FigureFsaWireType::State;
    uint16_t version=0;
    std::vector<FigureFsaRange> fields;
};
struct FigureFsaObjectWalk {
    static constexpr bool verifies_original_instance=false;
    static constexpr bool accepts_new_object_payloads=true;
    static constexpr bool accepts_arbitrary_dynamic_classes=false;
    size_t start=0,base_end=0,end=0;
    uint16_t base_version=0,version=0;
    bool has_current_name=false;
    std::vector<FigureFsaRange> flags;
    std::vector<FigureFsaString> strings;
    std::vector<FigureFsaCount> counts;
    std::array<FigureFsaEmbedded,3> embedded{};
    std::vector<FigureFsaWireCacheEntry> cache;
    std::vector<FigureFsaWireReference> references;
    std::vector<FigureFsaClassDeclaration> declarations;
    std::vector<FigureFsaObjectPayload> payloads;
};
struct FigureFsaPrefixReadFailure : std::runtime_error {
    size_t attempted_offset;
    bool unknown_class=false;
    size_t tag_offset=0,name_offset=0,name_bytes=0;
    uint16_t schema=0;
    FigureFsaPrefixReadFailure(size_t offset,const std::string&message)
        :std::runtime_error(message),attempted_offset(offset) {}
};
struct FigureFsaUnknownClassFailure : std::runtime_error {
    size_t tag_offset,name_offset,name_bytes;
    uint16_t schema;
    FigureFsaUnknownClassFailure(size_t tag,size_t name,size_t bytes,uint16_t version)
        :std::runtime_error("FigureFSA unsupported concrete class declaration"),
         tag_offset(tag),name_offset(name),name_bytes(bytes),schema(version) {}
};

namespace detail {
// A local metadata graph, not original object construction or pointer validation.
class FigureFsaObjectReader {
    GoalCursor cursor_;
    FigureFsaObjectLimits limits_;
    FigureFsaObjectWalk result_;
    size_t prefix_entries_=0;
    static bool family_valid(FigureFsaWireType f) {
        return f==FigureFsaWireType::State || f==FigureFsaWireType::Transition ||
               f==FigureFsaWireType::ActionContext || f==FigureFsaWireType::Action ||
               f==FigureFsaWireType::Context || f==FigureFsaWireType::Match;
    }
    uint32_t append(FigureFsaWireCacheEntry entry) {
        if(result_.cache.size()>=limits_.max_cache)
            throw std::runtime_error("FigureFSA object cache budget");
        if(prefix_entries_&&result_.cache.size()-prefix_entries_>=1024)
            throw std::runtime_error("FigureFSA appended cache budget");
        const auto id=static_cast<uint32_t>(result_.cache.size());
        result_.cache.push_back(entry);return id;
    }
    void string(uint32_t at) {
        if(result_.strings.size()>=limits_.max_strings)
            throw std::runtime_error("FigureFSA object string count budget");
        const auto lo=cursor_.position();const auto n=cursor_.u32();
        if(n>limits_.max_string)throw std::runtime_error("FigureFSA object string byte budget");
        const auto off=cursor_.position();cursor_.take(n);
        result_.strings.push_back({lo,off,n,at});
    }
    uint8_t flag(uint32_t at) {
        const auto off=cursor_.position();const auto value=*cursor_.take(1);
        result_.flags.push_back({off,1,at});return value;
    }
    size_t count(uint32_t at) {
        const auto off=cursor_.position();const auto w=cursor_.u32();
        const int64_t n=w<=0x7fffffff?int64_t(w):int64_t(w)-0x100000000LL;
        const auto iterations=n>0?size_t(n):0;
        if(iterations>limits_.max_items)throw std::runtime_error("FigureFSA object list budget");
        result_.counts.push_back({off,n,iterations,at});return iterations;
    }
    void refs(FigureFsaWireType family,uint32_t count_at,uint32_t ref_at,size_t depth) {
        const auto n=count(count_at);
        for(size_t i=0;i<n;++i)reference(family,ref_at,depth);
    }
    uint32_t declaration(const GoalTag &tag) {
        const auto schema=cursor_.u16();const auto n=cursor_.u16();
        if(!n||n>=64)throw std::runtime_error("FigureFSA class name length outside source bound");
        const auto off=cursor_.position();const auto name=cursor_.take(n);
        const auto equal=[&](const char *s){return std::strlen(s)==n&&!std::memcmp(name,s,n);};
        FigureFsaWireType family;
        if(equal("FSAState"))family=FigureFsaWireType::State;
        else if(equal("FSATransition"))family=FigureFsaWireType::Transition;
        else if(equal("FSAActionContext"))family=FigureFsaWireType::ActionContext;
        else if(equal("FSAAction"))family=FigureFsaWireType::Action;
        else if(equal("FSAContext"))family=FigureFsaWireType::Context;
        else if(equal("FSAMatch"))family=FigureFsaWireType::Match;
        else throw FigureFsaUnknownClassFailure(tag.offset,off,n,schema);
        if(schema!=0)throw std::runtime_error("FigureFSA exact class schema mismatch");
        const auto id=append({FigureFsaCacheKind::Class,family,0,schema});
        result_.declarations.push_back({tag.offset,off,n,id,schema,family});return id;
    }
    uint32_t reference(FigureFsaWireType expected,uint32_t at,size_t depth) {
        if(result_.references.size()>=limits_.max_refs)
            throw std::runtime_error("FigureFSA object reference budget");
        const auto tag=goal_tag(cursor_);const auto tag_bytes=cursor_.position()-tag.offset;
        uint32_t class_id=0,object_id=0;
        if(tag.declaration)class_id=declaration(tag);
        else if(tag.value&0x80000000) {
            class_id=tag.value&0x7fffffff;
            if(!class_id||class_id>=result_.cache.size()||
               result_.cache[class_id].kind!=FigureFsaCacheKind::Class)
                throw std::runtime_error("FigureFSA class reference kind/range");
            if(result_.cache[class_id].family==FigureFsaWireType::OpaquePrefix)
                throw std::runtime_error("FigureFSA reference to unsupported opaque prefix class");
        } else {
            object_id=tag.value;
            if(object_id>=result_.cache.size())
                throw std::runtime_error("FigureFSA object reference range");
            if(object_id&&result_.cache[object_id].family==FigureFsaWireType::OpaquePrefix)
                throw std::runtime_error("FigureFSA reference to unsupported opaque prefix object");
            if(object_id&&(result_.cache[object_id].kind!=FigureFsaCacheKind::Object||
                          result_.cache[object_id].family!=expected))
                throw std::runtime_error("FigureFSA object reference kind/family");
            result_.references.push_back({tag.offset,tag_bytes,object_id,at,expected,
                                          object_id==0,tag.word==0x7fff});
            return object_id;
        }
        const auto type=result_.cache[class_id].family;
        const auto schema=result_.cache[class_id].schema;
        if(type!=expected)throw std::runtime_error("FigureFSA new object expected family mismatch");
        if(schema!=0)throw std::runtime_error("FigureFSA new object exact schema mismatch");
        if(depth>=limits_.max_depth||result_.payloads.size()>=limits_.max_objects)
            throw std::runtime_error("FigureFSA object depth/count budget");
        object_id=append({FigureFsaCacheKind::Object,type,class_id,schema});
        result_.references.push_back({tag.offset,tag_bytes,object_id,at,expected,false,tag.word==0x7fff});
        // Register before payload. Back references to this ID do not recurse.
        const auto index=result_.payloads.size();
        result_.payloads.push_back({cursor_.position(),0,object_id,class_id,type,0,{}});
        payload(index,depth+1);
        result_.payloads[index].end=cursor_.position();return object_id;
    }
    void payload(size_t index,size_t depth);
    void outer();
public:
    FigureFsaObjectReader(GoalBytes bytes,const std::vector<FigureFsaWireCacheEntry>&cache,
                         size_t start,FigureFsaObjectLimits limits,size_t prefix_entries=0)
        :cursor_(bytes,start),limits_(limits),prefix_entries_(prefix_entries) {
        if(!bytes.data||!limits.max_bytes||limits.max_bytes>65536||bytes.size>limits.max_bytes||
           limits.max_string>8192||limits.max_items>256||limits.max_refs>1024||
           !limits.max_cache||limits.max_cache>(prefix_entries?8192u:1024u)||
           (prefix_entries&&(prefix_entries<1||prefix_entries>8192||prefix_entries>cache.size()))||
           !limits.max_objects||limits.max_objects>128||
           !limits.max_depth||limits.max_depth>16||!limits.max_strings||limits.max_strings>1024||
           cache.empty()||cache.size()>limits.max_cache||cache[0].kind!=FigureFsaCacheKind::Null)
            throw std::runtime_error("FigureFSA object view/cache/budget argument");
        for(size_t i=1;i<cache.size();++i) {
            const auto&e=cache[i];
            const bool opaque=i<prefix_entries&&e.family==FigureFsaWireType::OpaquePrefix;
            if((!opaque&&(!family_valid(e.family)||e.schema!=0))||
               (e.kind!=FigureFsaCacheKind::Class&&e.kind!=FigureFsaCacheKind::Object))
                throw std::runtime_error("FigureFSA exact initial cache metadata");
            if(e.kind==FigureFsaCacheKind::Object&&(!e.class_id||e.class_id>=cache.size()||
               cache[e.class_id].kind!=FigureFsaCacheKind::Class||cache[e.class_id].family!=e.family))
                throw std::runtime_error("FigureFSA initial object/class relation");
        }
        result_.cache=cache;result_.start=start;
    }
    FigureFsaObjectWalk read() {outer();result_.end=cursor_.position();return std::move(result_);}
    size_t position() const {return cursor_.position();}
};
inline void FigureFsaObjectReader::payload(size_t index,size_t depth) {
    const auto type=result_.payloads[index].family;
    const auto version=cursor_.u16();result_.payloads[index].version=version;
    if(type==FigureFsaWireType::State) {
        // Selected4f5630 -> root40cf80 (zero bytes), own version1.
        if(version!=1)throw std::runtime_error("FSAState own outside selected1 subset");
        string(0x4f5785);
        refs(FigureFsaWireType::ActionContext,0x4f5791,0x4f57a9,depth);
        refs(FigureFsaWireType::ActionContext,0x4f57d2,0x4f57ea,depth);
        reference(FigureFsaWireType::Transition,0x4f5811,depth);
        refs(FigureFsaWireType::Transition,0x4f581d,0x4f5832,depth);
    } else if(type==FigureFsaWireType::ActionContext) {
        if(version!=1)throw std::runtime_error("FSAActionContext own outside selected1 subset");
        reference(FigureFsaWireType::Action,0x4f7b36,depth);
        reference(FigureFsaWireType::Context,0x4f7b40,depth);
    } else if(type==FigureFsaWireType::Transition) {
        if(version!=1)throw std::runtime_error("FSATransition own outside selected1 subset");
        refs(FigureFsaWireType::Match,0x4f69cf,0x4f69e7,depth);
        flag(0x4f6a0e); // 503d50 normalizes a serialized byte only in host memory.
        for(const auto &field:std::array<std::pair<size_t,uint32_t>,3>{{
                {4,0x4f6a19},{8,0x4f6a24},{8,0x4f6a2f}}}) {
            const auto off=cursor_.position();cursor_.take(field.first);
            result_.payloads[index].fields.push_back({off,field.first,field.second});
        }
        reference(FigureFsaWireType::State,0x4f6a39,depth);
        refs(FigureFsaWireType::ActionContext,0x4f6a45,0x4f6a5d,depth);
    } else if(type==FigureFsaWireType::Action) {
        if(version!=1)throw std::runtime_error("FSAAction own outside selected1 subset");
        string(0x4f78ec);
    } else if(type==FigureFsaWireType::Context) {
        if(version!=1)throw std::runtime_error("FSAContext own outside selected1 subset");
        const auto off=cursor_.position();cursor_.take(4);
        result_.payloads[index].fields.push_back({off,4,0x4f75fe});
    } else if(type==FigureFsaWireType::Match) {
        if(version!=1)throw std::runtime_error("FSAMatch own outside selected1 subset");
        string(0x4f7d57);
        const auto off=cursor_.position();cursor_.take(4);
        result_.payloads[index].fields.push_back({off,4,0x4f7d62});
    } else {
        throw std::runtime_error("FigureFSA unsupported concrete payload type");
    }
}
inline void FigureFsaObjectReader::outer() {
    auto&r=result_;
    r.base_version=cursor_.u16();
    if(r.base_version!=1&&r.base_version!=2)
        throw std::runtime_error("FigureFSA object base outside source1/2 subset");
    string(r.base_version==2?0x4f4604:0x4f4698);
    flag(r.base_version==2?0x4f4621:0x4f46b5);
    if(r.base_version==1) {
        refs(FigureFsaWireType::Transition,0x4f46c1,0x4f46d6,0);
        refs(FigureFsaWireType::Transition,0x4f46eb,0x4f4700,0);
        reference(FigureFsaWireType::Transition,0x4f4713,0);
        refs(FigureFsaWireType::Transition,0x4f471f,0x4f4734,0);
        refs(FigureFsaWireType::Transition,0x4f4749,0x4f475e,0);
        refs(FigureFsaWireType::ActionContext,0x4f4773,0x4f4788,0);
        refs(FigureFsaWireType::ActionContext,0x4f479d,0x4f47b2,0);
        refs(FigureFsaWireType::State,0x4f47c7,0x4f47dc,0);
    }
    const auto direct=r.base_version==2?
        std::array<uint32_t,3>{{0x4f462b,0x4f4635,0x4f463f}}:
        std::array<uint32_t,3>{{0x4f47ef,0x4f47f9,0x4f4803}};
    for(auto at:direct)reference(FigureFsaWireType::State,at,0);
    refs(FigureFsaWireType::State,r.base_version==2?0x4f464b:0x4f480f,
         r.base_version==2?0x4f4660:0x4f4824,0);
    r.base_end=cursor_.position();r.version=cursor_.u16();
    if(r.version<2||r.version>5)
        throw std::runtime_error("FigureFSA object own outside source2..5 subset");
    if(r.version!=2) {
        r.has_current_name=flag(r.version==5?0x4daeef:r.version==4?0x4dafaf:0x4db013)!=0;
        if(r.has_current_name)string(r.version==5?0x4daf02:r.version==4?0x4dafc2:0x4db026);
        if(r.version!=4) {
            const auto n=count(r.version==5?0x4daf28:0x4db04c);
            for(size_t i=0;i<n;++i)string(r.version==5?0x4daf3d:0x4db061);
        }
    }
    for(size_t i=0;i<3;++i) {
        auto&e=r.embedded[i];e.offset=cursor_.position();e.version=cursor_.u16();
        if(e.version!=1)throw std::runtime_error("FigureFSA object embedded outside own1 subset");
        constexpr std::array<uint32_t,4> calls{{0x4f8d0f,0x4f8d1a,0x4f8d25,0x4f8d30}};
        for(size_t k=0;k<4;++k) {
            const auto off=cursor_.position();cursor_.take(4);e.fields[k]={off,4,calls[k]};
        }
        const auto off=cursor_.position();cursor_.take(1);
        e.flag={off,1,r.version==2?0x4db0e5u:r.version==4?0x4daffcu:r.version==3?0x4db0bcu:0x4daf98u};
        e.end=cursor_.position();
    }
}
} // namespace detail

inline FigureFsaObjectWalk walk_city19_figurefsa_exact_objects(
    GoalBytes bytes,FigureFsaObjectContract contract,
    const std::vector<FigureFsaWireCacheEntry>&cache={{}},size_t start=0,
    FigureFsaObjectLimits limits={}) {
    if(contract!=FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10)
        throw std::runtime_error("FigureFSA object unsupported explicit contract");
    return detail::FigureFsaObjectReader(bytes,cache,start,limits).read();
}
// Explicit archive continuation. Prefix entries preserve global IDs and kinds;
// their unmodeled types are never accepted as one of the six exact FSA classes.
inline FigureFsaObjectWalk walk_city19_figurefsa_with_archive_prefix(
    GoalBytes bytes,FigureFsaObjectContract contract,
    const std::vector<FigureFsaWireCacheEntry>&cache,size_t prefix_entries,
    size_t start=0,FigureFsaObjectLimits limits={}) {
    if(contract!=FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10||!prefix_entries)
        throw std::runtime_error("FigureFSA prefix unsupported explicit contract");
    detail::FigureFsaObjectReader reader(bytes,cache,start,limits,prefix_entries);
    try {return reader.read();}
    catch(const FigureFsaUnknownClassFailure&e) {
        FigureFsaPrefixReadFailure failure(reader.position(),e.what());
        failure.unknown_class=true;failure.tag_offset=e.tag_offset;
        failure.name_offset=e.name_offset;failure.name_bytes=e.name_bytes;failure.schema=e.schema;
        throw failure;
    }
    catch(const std::runtime_error&e) {throw FigureFsaPrefixReadFailure(reader.position(),e.what());}
}
} // namespace emperor::file_io::research
