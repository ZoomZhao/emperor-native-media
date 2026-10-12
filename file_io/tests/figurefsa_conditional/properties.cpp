#include "city19_figurefsa_archive.hpp"
#include <functional>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
constexpr auto selected=FigureFsaContract::cached_refs_root7b5bc8_reader4dad10;
static_assert(!FigureFsaRecord::verifies_original_instance);
static_assert(!FigureFsaRecord::accepts_new_object_payloads);
struct Fixture {
    Bytes b;size_t start=0,base_end=0,end=0;
    std::vector<std::pair<size_t,size_t>> strings,refs;
    std::vector<size_t> counts,versions;
    void u8(uint8_t x){b.push_back(x);}
    void u16(uint16_t x){u8(uint8_t(x));u8(uint8_t(x>>8));}
    void u32(uint32_t x){for(unsigned i=0;i<4;++i)u8(uint8_t(x>>(8*i)));}
    void str(std::initializer_list<uint8_t> x){u32(uint32_t(x.size()));const auto p=b.size();b.insert(b.end(),x);strings.push_back({p,x.size()});}
    void ref(uint32_t id,bool extended=false){const auto p=b.size();if(extended){u16(0x7fff);u32(id);}else u16(uint16_t(id));refs.push_back({p,b.size()-p});}
    void number(int32_t x){counts.push_back(b.size());u32(uint32_t(x));}
};
std::vector<FigureFsaCacheEntry> cache() {
    return {{FigureFsaCacheKind::Null,FigureFsaFamily::State,0,0},
        {FigureFsaCacheKind::Class,FigureFsaFamily::State,0,42},
        {FigureFsaCacheKind::Object,FigureFsaFamily::State,1,42},
        {FigureFsaCacheKind::Class,FigureFsaFamily::Transition,0,17},
        {FigureFsaCacheKind::Object,FigureFsaFamily::Transition,3,17},
        {FigureFsaCacheKind::Class,FigureFsaFamily::ActionContext,0,65535},
        {FigureFsaCacheKind::Object,FigureFsaFamily::ActionContext,5,65535}};
}
Fixture fixture(uint16_t base,uint16_t own,size_t start=0,bool rich=true,bool current=true,bool negative=false) {
    Fixture f;f.b.resize(start,0xa5);f.start=start;f.versions.push_back(f.b.size());f.u16(base);
    f.str({0x42,0x00,0xfe});f.u8(255);
    if(base==1) {
        // Source4f4692 sequence, independently spelled out; not production tables.
        f.number(negative?-3:rich?1:0);if(!negative&&rich)f.ref(4);
        f.number(0);f.ref(rich?4:0);
        f.number(negative?-1:rich?2:0);if(!negative&&rich){f.ref(4,true);f.ref(0);}
        f.number(0);
        f.number(rich?1:0);if(rich)f.ref(6);
        f.number(0);
        f.number(rich?2:0);if(rich){f.ref(2);f.ref(2,true);}
    }
    f.ref(rich?2:0);f.ref(0);f.ref(rich?2:0,rich);
    f.number(negative?-2147483647:rich?2:0);if(!negative&&rich){f.ref(0);f.ref(2);}
    f.base_end=f.b.size();f.versions.push_back(f.b.size());f.u16(own);
    if(own!=2) {
        f.u8(current?255:0);if(current)f.str({0x43,0x00,0xff,0x44});
        if(own!=4){f.number(negative?-1:rich?2:0);if(!negative&&rich){f.str({});f.str({0x78,0x79,0x80});}}
    }
    for(unsigned i=0;i<3;++i){f.versions.push_back(f.b.size());f.u16(1);for(unsigned j=0;j<16;++j)f.u8(uint8_t(i*37+j));f.u8(uint8_t(i*127));}
    f.end=f.b.size();return f;
}
FigureFsaRecord walk(const Fixture&f,const std::vector<FigureFsaCacheEntry>&c=cache(),FigureFsaLimits limits={}) {
    return walk_city19_figurefsa_cached_record({f.b.data(),f.b.size()},selected,c,f.start,limits);
}
bool rejects(const Fixture&f,const char*text,const std::vector<FigureFsaCacheEntry>&c=cache(),FigureFsaLimits l={}) {
    try{(void)walk(f,c,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(text)!=std::string::npos;}
}
void put16(Bytes&b,size_t p,uint16_t x){b[p]=uint8_t(x);b[p+1]=uint8_t(x>>8);}
void put32(Bytes&b,size_t p,uint32_t x){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(x>>(8*i));}
bool mapped(const Fixture&f,const FigureFsaRecord&r) {
    if(r.start!=f.start||r.base_end!=f.base_end||r.end!=f.end||r.strings.size()!=f.strings.size()||r.references.size()!=f.refs.size()||r.counts.size()!=f.counts.size()||r.verifies_original_instance)return false;
    for(size_t i=0;i<f.strings.size();++i)if(r.strings[i].offset!=f.strings[i].first||r.strings[i].bytes!=f.strings[i].second||r.strings[i].length_offset+4!=r.strings[i].offset)return false;
    for(size_t i=0;i<f.refs.size();++i)if(r.references[i].offset!=f.refs[i].first||r.references[i].bytes!=f.refs[i].second)return false;
    for(size_t i=0;i<f.counts.size();++i)if(r.counts[i].offset!=f.counts[i])return false;
    for(size_t i=0;i<3;++i){const auto&e=r.embedded[i];if(e.version!=1||e.end-e.offset!=19||e.flag.offset+1!=e.end)return false;for(size_t j=0;j<4;++j)if(e.fields[j].offset!=e.offset+2+j*4||e.fields[j].bytes!=4)return false;}
    return true;
}
int main(){
    std::vector<std::pair<std::string,bool>> checks;
    auto check=[&](const char*id,std::function<bool()>fn){bool ok=false;try{ok=fn();}catch(const std::exception&){ok=false;}checks.push_back({id,ok});};
    check("all_base_own_branches_and_opaque_strings",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5})for(bool name:{false,true}){auto f=fixture(b,v,17,true,name);auto before=f.b;if(!mapped(f,walk(f))||f.b!=before)return false;}return true;});
    check("minimum_sizes_not_memory_strides",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto f=fixture(b,v,0,false,false);auto r=walk(f);const size_t base=b==1?50:20,own=v==2?59:v==4?60:64;if(r.base_end!=base||r.end!=base+own||!mapped(f,r))return false;}return true;});
    check("every_truncation_rejected",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto full=fixture(b,v,3);for(size_t n=3;n<full.end;++n){auto f=full;f.b.resize(n);if(!rejects(f,"truncated"))return false;}}return true;});
    check("prefix_translation_and_trailing_bytes",[]{for(size_t start:{0u,1u,257u,4096u}){auto f=fixture(1,5,start);f.b.insert(f.b.end(),19,0xff);auto before=f.b;if(!mapped(f,walk(f))||f.b!=before)return false;}return true;});
    check("version_subsets_and_little_endian",[]{for(uint16_t bad:{0,3,255,0x0200,65535}){auto f=fixture(2,5);put16(f.b,f.versions[0],bad);if(!rejects(f,"base outside"))return false;}for(uint16_t bad:{0,1,6,0x0500,65535}){auto f=fixture(2,5);put16(f.b,f.versions[1],bad);if(!rejects(f,"own outside"))return false;}for(size_t child=0;child<3;++child)for(uint16_t bad:{0,2,0x0100,65535}){auto f=fixture(2,5);put16(f.b,f.versions[child+2],bad);if(!rejects(f,"embedded outside"))return false;}return true;});
    check("signed_nonpositive_counts_skip",[]{for(uint16_t b:{1,2}){auto f=fixture(b,5,0,true,true,true);auto r=walk(f);if(!mapped(f,r))return false;bool seen=false;for(const auto&c:r.counts)if(c.declared<0){seen=true;if(c.iterations)return false;}if(!seen)return false;}return true;});
    check("positive_count_and_string_budgets",[]{auto f=fixture(2,5);auto bad=f;put32(bad.b,bad.counts[0],257);if(!rejects(bad,"count budget"))return false;bad=f;put32(bad.b,2,8193);if(!rejects(bad,"string budget"))return false;FigureFsaLimits l;l.max_refs=1;return rejects(f,"reference budget",cache(),l);});
    check("cache_null_repeated_and_extended_refs",[]{auto f=fixture(1,5);auto r=walk(f);size_t repeated=0,extended=0,nulls=0;for(const auto&e:r.references){repeated+=e.object_id==2;extended+=e.extended;nulls+=e.null_reference;}return mapped(f,r)&&repeated>1&&extended>1&&nulls>1;});
    check("cache_kind_family_range_and_metadata",[]{auto f=fixture(2,5);auto bad=f;put16(bad.b,f.refs[0].first,1);if(!rejects(bad,"cache kind"))return false;bad=f;put16(bad.b,f.refs[0].first,4);if(!rejects(bad,"expected family"))return false;bad=f;put16(bad.b,f.refs[0].first,1000);if(!rejects(bad,"not in cache"))return false;auto c=cache();c[2].class_id=4;if(!rejects(f,"object/class",c))return false;c=cache();c[0].kind=FigureFsaCacheKind::Class;return rejects(f,"argument",c);});
    check("new_object_and_class_decl_rejected_before_payload",[]{for(uint16_t tag:{0x8001,0xffff}){auto f=fixture(2,5);f.b.resize(f.refs[0].first+2);put16(f.b,f.refs[0].first,tag);if(!rejects(f,"new class/object"))return false;}auto f=fixture(2,5);put32(f.b,f.refs[2].first+2,0x80000001);return rejects(f,"new class/object");});
    check("cache_schema_not_object_version",[]{auto f=fixture(1,3);auto c=cache();for(auto&e:c)e.schema=1234;return mapped(f,walk(f,c));});
    check("failures_do_not_mutate_input_or_cache",[]{auto good=fixture(1,5);const auto data=good.b;const auto c=cache();for(unsigned i=0;i<16;++i){auto bad=good;bad.b.resize(good.end-1);if(!rejects(bad,"truncated",c)||!mapped(good,walk(good,c))||good.b!=data||c[2].class_id!=1||c[4].family!=FigureFsaFamily::Transition)return false;}return true;});
    check("explicit_contract_and_view_budget",[]{auto f=fixture(2,2);try{(void)walk_city19_figurefsa_cached_record({nullptr,0},static_cast<FigureFsaContract>(99),cache());return false;}catch(const std::runtime_error&e){if(std::string(e.what()).find("explicit contract")==std::string::npos)return false;}FigureFsaLimits l;l.max_bytes=f.b.size()-1;return rejects(f,"argument",cache(),l);});
    check("readonly_record_at_guard_page",[]{auto f=fixture(1,5);const auto n=f.b.size();const long p=sysconf(_SC_PAGESIZE);if(p<=0||n>size_t(p))return false;const auto ps=size_t(p);void*mem=mmap(nullptr,ps*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);if(mem==MAP_FAILED)return false;auto*end=static_cast<uint8_t*>(mem)+ps;auto*start=end-n;std::memcpy(start,f.b.data(),n);bool ok=mprotect(mem,ps,PROT_READ)==0&&mprotect(end,ps,PROT_NONE)==0;if(ok){auto r=walk_city19_figurefsa_cached_record({start,n},selected,cache());ok=mapped(f,r)&&std::memcmp(start,f.b.data(),n)==0;}munmap(mem,ps*2);return ok;});
    size_t failed=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"id\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
