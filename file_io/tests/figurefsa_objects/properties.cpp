#include "city19_figurefsa_objects.hpp"
#include <functional>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
constexpr auto contract=FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10;
static_assert(!FigureFsaObjectWalk::verifies_original_instance);
static_assert(!FigureFsaObjectWalk::accepts_arbitrary_dynamic_classes);
struct Fixture {
    Bytes b;
    size_t start=0,base_end=0,end=0;
    std::vector<size_t> versions,counts,declarations;
    std::vector<std::pair<size_t,size_t>> strings;
    std::vector<uint32_t> ids;
    struct Payload {size_t start,end;FigureFsaWireType type;uint32_t id;};
    std::vector<Payload> payloads;
    uint32_t next_id=1;
    uint32_t match_class=0;
    bool preloaded_classes=false,extended_classes=false;
    void u8(uint8_t x){b.push_back(x);}
    void u16(uint16_t x){u8(uint8_t(x));u8(uint8_t(x>>8));}
    void u32(uint32_t x){for(unsigned i=0;i<4;++i)u8(uint8_t(x>>(8*i)));}
    void version(uint16_t x){versions.push_back(b.size());u16(x);}
    void count(int32_t x){counts.push_back(b.size());u32(uint32_t(x));}
    void str(std::initializer_list<uint8_t>x){u32(uint32_t(x.size()));auto p=b.size();b.insert(b.end(),x);strings.push_back({p,x.size()});}
    void tag(uint32_t value,bool extended=false){
        if(extended){u16(0x7fff);u32(value);}
        else u16(uint16_t(value&0x7fff)|((value&0x80000000)?0x8000:0));
    }
    void ref(uint32_t id,bool extended=false){ids.push_back(id);tag(id,extended);}
    uint32_t cls(const char*name){
        if(preloaded_classes){const char*names[]={"FSAState","FSATransition","FSAActionContext","FSAAction","FSAContext","FSAMatch"};for(uint32_t i=0;i<6;++i)if(!std::strcmp(name,names[i])){tag(0x80000000|(i+1),extended_classes);return i+1;}throw std::runtime_error("fixture class");}
        declarations.push_back(b.size());u16(0xffff);u16(0);u16(uint16_t(std::strlen(name)));b.insert(b.end(),name,name+std::strlen(name));return next_id++;
    }
    uint32_t object(FigureFsaWireType type,const std::function<void()>&write){
        const auto id=next_id++;ids.push_back(id);const auto i=payloads.size();
        payloads.push_back({b.size(),0,type,id});write();payloads[i].end=b.size();return id;
    }
    void state_empty(bool negative=false){
        version(1);str({0x53,0,0xfe});count(negative?-1:2);
        if(!negative){ref(0);ref(0,true);}
        count(0);ref(0);count(negative?-2147483647:1);if(!negative)ref(0);
    }
};
Fixture states(uint16_t base=2,uint16_t own=5,size_t start=0,bool negative=false){
    Fixture f;f.start=start;f.b.resize(start,0xa5);f.version(base);f.str({0x4e,0,0xff});f.u8(255);
    if(base==1){f.count(0);f.count(0);f.ref(0);f.count(0);f.count(0);f.count(0);f.count(0);f.count(0);}
    const auto c=f.cls("FSAState");const auto first=f.object(FigureFsaWireType::State,[&]{f.state_empty(negative);});
    f.ref(first,true);f.tag(0x80000000|c,true);f.object(FigureFsaWireType::State,[&]{f.state_empty(negative);});
    f.count(negative?-1:2);if(!negative){f.ref(first);f.ref(0);}
    f.base_end=f.b.size();f.version(own);
    if(own!=2){f.u8(255);f.str({0x43});if(own!=4){f.count(negative?-9:1);if(!negative)f.str({0,0xff});}}
    for(unsigned i=0;i<3;++i){f.version(1);for(unsigned j=0;j<16;++j)f.u8(uint8_t(i*19+j));f.u8(uint8_t(i*127));}
    f.end=f.b.size();return f;
}
void context_null(Fixture&f){f.version(1);f.ref(0);f.ref(0,true);}
void context_full(Fixture&f){
    f.version(1);f.cls("FSAAction");f.object(FigureFsaWireType::Action,[&]{f.version(1);f.str({0x41,0,0xfe});});
    f.cls("FSAContext");f.object(FigureFsaWireType::Context,[&]{f.version(1);f.u32(0xfedcba98);});
}
void transition(Fixture&f,uint32_t parent,uint32_t context_id,bool rich=false){
    f.version(1);f.count(2);
    if(rich){if(!f.match_class)f.match_class=f.cls("FSAMatch");else f.tag(0x80000000|f.match_class,true);const auto id=f.object(FigureFsaWireType::Match,[&]{f.version(1);f.str({0x4d,0,0xff});f.u32(0x78563412);});f.ref(id,true);}
    else {f.ref(0);f.ref(0,true);}
    f.u8(255);
    for(unsigned i=0;i<20;++i)f.u8(uint8_t(0xd1+i));
    f.ref(parent,true);f.count(1);f.ref(context_id);
}
Fixture graph(uint16_t base=2,uint16_t own=5,size_t start=0,bool rich=false,bool preloaded=false,bool extended=false){
    Fixture f;f.start=start;f.preloaded_classes=preloaded;f.extended_classes=extended;f.next_id=preloaded?7:1;f.b.resize(start,0xa5);f.version(base);f.str({0x47});f.u8(255);
    if(base==1){f.count(0);f.count(0);f.ref(0);f.count(0);f.count(0);f.count(0);f.count(0);f.count(0);}
    const auto state_class=f.cls("FSAState");const auto state_id=f.next_id;
    f.object(FigureFsaWireType::State,[&]{
        f.version(1);f.str({0x53,0,0xff});f.count(1);
        const auto context_class=f.cls("FSAActionContext");
        const auto context_id=f.object(FigureFsaWireType::ActionContext,[&]{if(rich)context_full(f);else context_null(f);});
        f.count(1);f.ref(context_id);
        const auto transition_class=f.cls("FSATransition");
        const auto transition_id=f.object(FigureFsaWireType::Transition,[&]{transition(f,state_id,context_id,rich);});
        f.count(3);f.ref(transition_id);f.ref(0);
        f.tag(0x80000000|transition_class);
        f.object(FigureFsaWireType::Transition,[&]{transition(f,state_id,context_id,rich);});
        (void)context_class;
    });
    f.ref(state_id);f.tag(0x80000000|state_class);
    f.object(FigureFsaWireType::State,[&]{f.state_empty();});
    f.count(2);f.ref(state_id,true);f.ref(0);f.base_end=f.b.size();f.version(own);
    if(own!=2){f.u8(0);if(own!=4)f.count(0);}
    for(unsigned i=0;i<3;++i){f.version(1);for(unsigned j=0;j<16;++j)f.u8(uint8_t(i+j));f.u8(255);}
    f.end=f.b.size();return f;
}
FigureFsaObjectWalk walk(const Fixture&f,FigureFsaObjectLimits l={},const std::vector<FigureFsaWireCacheEntry>&c={{}}){
    return walk_city19_figurefsa_exact_objects({f.b.data(),f.b.size()},contract,c,f.start,l);
}
Fixture cached(uint16_t base,uint16_t own){
    Fixture f;f.version(base);f.str({0x43});f.u8(255);
    if(base==1){f.count(1);f.ref(4);f.count(0);f.ref(4);f.count(0);f.count(1);f.ref(0);f.count(1);f.ref(6);f.count(0);f.count(1);f.ref(2);}
    f.ref(2);f.ref(0);f.ref(2,true);f.count(1);f.ref(2);f.base_end=f.b.size();f.version(own);
    if(own!=2){f.u8(255);f.str({0x4e,0});if(own!=4){f.count(1);f.str({0xff});}}
    for(unsigned i=0;i<3;++i){f.version(1);for(unsigned j=0;j<16;++j)f.u8(uint8_t(j));f.u8(255);}
    f.end=f.b.size();return f;
}
bool reject(const Fixture&f,const char*part="",FigureFsaObjectLimits l={},const std::vector<FigureFsaWireCacheEntry>&c={{}}){
    try{(void)walk(f,l,c);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(part)!=std::string::npos;}
}
void put16(Bytes&b,size_t p,uint16_t x){b.at(p)=uint8_t(x);b.at(p+1)=uint8_t(x>>8);}
void put32(Bytes&b,size_t p,uint32_t x){for(unsigned i=0;i<4;++i)b.at(p+i)=uint8_t(x>>(8*i));}
bool golden(const Fixture&f,const FigureFsaObjectWalk&r){
    if(r.start!=f.start||r.base_end!=f.base_end||r.end!=f.end||r.references.size()!=f.ids.size()||
       r.strings.size()!=f.strings.size()||r.counts.size()!=f.counts.size()||r.payloads.size()!=f.payloads.size())return false;
    for(size_t i=0;i<f.ids.size();++i)if(r.references[i].object_id!=f.ids[i])return false;
    for(size_t i=0;i<f.strings.size();++i)if(r.strings[i].offset!=f.strings[i].first||r.strings[i].bytes!=f.strings[i].second)return false;
    for(size_t i=0;i<f.counts.size();++i)if(r.counts[i].offset!=f.counts[i])return false;
    for(size_t i=0;i<f.payloads.size();++i)if(r.payloads[i].offset!=f.payloads[i].start||r.payloads[i].end!=f.payloads[i].end||r.payloads[i].family!=f.payloads[i].type||r.payloads[i].object_id!=f.payloads[i].id)return false;
    return true;
}

int main(){
    std::vector<std::pair<std::string,bool>> checks;
    auto check=[&](const char*name,std::function<bool()>fn){bool ok=false;try{ok=fn();}catch(const std::exception&){ok=false;}checks.push_back({name,ok});};
    check("state_declaration_cached_class_all_outer_versions",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto f=states(b,v);auto r=walk(f);if(!golden(f,r)||r.cache.size()!=4||r.declarations.size()!=1||r.payloads.size()!=2)return false;}return true;});
    check("cached_branch_agrees_with_preserved_component",[]{
        const std::vector<FigureFsaWireCacheEntry> nc{{},{FigureFsaCacheKind::Class,FigureFsaWireType::State,0,0},{FigureFsaCacheKind::Object,FigureFsaWireType::State,1,0},{FigureFsaCacheKind::Class,FigureFsaWireType::Transition,0,0},{FigureFsaCacheKind::Object,FigureFsaWireType::Transition,3,0},{FigureFsaCacheKind::Class,FigureFsaWireType::ActionContext,0,0},{FigureFsaCacheKind::Object,FigureFsaWireType::ActionContext,5,0}};
        const std::vector<FigureFsaCacheEntry> oc{{},{FigureFsaCacheKind::Class,FigureFsaFamily::State,0,0},{FigureFsaCacheKind::Object,FigureFsaFamily::State,1,0},{FigureFsaCacheKind::Class,FigureFsaFamily::Transition,0,0},{FigureFsaCacheKind::Object,FigureFsaFamily::Transition,3,0},{FigureFsaCacheKind::Class,FigureFsaFamily::ActionContext,0,0},{FigureFsaCacheKind::Object,FigureFsaFamily::ActionContext,5,0}};
        for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto f=cached(b,v);auto r=walk(f,{},nc);auto old=walk_city19_figurefsa_cached_record({f.b.data(),f.b.size()},FigureFsaContract::cached_refs_root7b5bc8_reader4dad10,oc);if(!golden(f,r)||r.end!=old.end||r.base_end!=old.base_end||r.payloads.size()||r.cache.size()!=nc.size()||r.strings.size()!=old.strings.size()||r.references.size()!=old.references.size()||r.counts.size()!=old.counts.size())return false;for(size_t i=0;i<r.references.size();++i)if(r.references[i].object_id!=old.references[i].object_id||r.references[i].offset!=old.references[i].offset||r.references[i].bytes!=old.references[i].bytes)return false;}return true;});
    check("three_primary_classes_new_payloads_and_back_reference_cycle",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto f=graph(b,v);auto r=walk(f);if(!golden(f,r)||r.declarations.size()!=3||r.payloads.size()!=5||r.cache.size()!=9)return false;bool seen=false;for(const auto&p:r.payloads)if(p.family==FigureFsaWireType::Transition){seen=true;if(p.fields.size()!=3||p.fields[0].bytes!=4||p.fields[1].bytes!=8||p.fields[2].bytes!=8)return false;}if(!seen)return false;}return true;});
    check("six_concrete_classes_declared_and_cached_new_instances",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto f=graph(b,v,0,true);auto r=walk(f);if(!golden(f,r)||r.declarations.size()!=6||r.payloads.size()!=9||r.cache.size()!=16)return false;bool types[6]={};for(const auto&p:r.payloads){types[unsigned(p.family)]=true;if(p.version!=1)return false;if(p.family==FigureFsaWireType::Context&&p.end-p.offset!=6)return false;if(p.family==FigureFsaWireType::Match&&(p.end-p.offset!=13||p.fields.size()!=1||p.fields[0].bytes!=4))return false;if(p.family==FigureFsaWireType::Action&&p.end-p.offset!=9)return false;}for(bool t:types)if(!t)return false;}return true;});
    check("all_six_preloaded_classes_support_small_and_extended_tags",[]{std::vector<FigureFsaWireCacheEntry> c{{}};for(auto t:{FigureFsaWireType::State,FigureFsaWireType::Transition,FigureFsaWireType::ActionContext,FigureFsaWireType::Action,FigureFsaWireType::Context,FigureFsaWireType::Match})c.push_back({FigureFsaCacheKind::Class,t,0,0});for(bool extended:{false,true}){auto f=graph(1,5,0,true,true,extended);auto r=walk(f,{},c);if(!golden(f,r)||!r.declarations.empty()||r.payloads.size()!=9||r.cache.size()!=16||c.size()!=7)return false;}return true;});
    check("every_six_class_graph_truncation_rejected",[]{auto f=graph(1,5,3,true);for(size_t n=f.start;n<f.end;++n){auto cut=f;cut.b.resize(n);if(!reject(cut))return false;}return true;});
    check("all_six_own_version_subsets_reject_unsupported",[]{auto f=graph(2,5,0,true);for(const auto&p:f.payloads)for(uint16_t v:{0,2,0x0100,65535}){auto bad=f;put16(bad.b,p.start,v);if(!reject(bad,"own outside"))return false;}return true;});
    check("every_recursive_graph_truncation_rejected",[]{auto f=graph(1,5,3);for(size_t n=f.start;n<f.end;++n){auto cut=f;cut.b.resize(n);if(!reject(cut))return false;}return true;});
    check("depth_budget_checks_new_payload_only_not_back_reference",[]{auto f=graph();FigureFsaObjectLimits l;l.max_depth=2;if(!golden(f,walk(f,l)))return false;l.max_depth=1;return reject(f,"depth/count",l);});
    check("every_state_fixture_truncation_rejected",[]{for(uint16_t b:{1,2})for(uint16_t v:{2,3,4,5}){auto f=states(b,v,3);for(size_t n=f.start;n<f.end;++n){auto cut=f;cut.b.resize(n);if(!reject(cut))return false;}}return true;});
    check("signed_counts_preserved_and_nonpositive_skip",[]{auto f=states(1,5,0,true);auto r=walk(f);bool seen=false;for(auto c:r.counts)if(c.declared<0){seen=true;if(c.iterations)return false;}return seen&&golden(f,r);});
    check("class_schema_separate_from_object_own_version",[]{auto f=states();auto r=walk(f);if(r.declarations[0].schema!=0||r.payloads[0].version!=1)return false;auto bad=f;put16(bad.b,f.declarations[0]+2,1);return reject(bad,"schema mismatch");});
    check("unknown_class_and_name_length_rejected",[]{auto f=states();auto bad=f;bad.b[f.declarations[0]+6]='X';if(!reject(bad,"unsupported concrete"))return false;for(uint16_t n:{0,64,65535}){bad=f;put16(bad.b,f.declarations[0]+4,n);if(!reject(bad,"name length"))return false;}return true;});
    check("unsupported_own_version_and_endian_rejected",[]{auto f=states();for(uint16_t v:{0,2,0x0100,65535}){auto bad=f;put16(bad.b,f.payloads[0].start,v);if(!reject(bad,"FSAState own"))return false;}return true;});
    check("object_cache_and_string_budgets",[]{auto f=states();FigureFsaObjectLimits l;l.max_objects=1;if(!reject(f,"count budget",l))return false;l={};l.max_cache=2;if(!reject(f,"cache budget",l))return false;l={};l.max_strings=1;if(!reject(f,"string count",l))return false;auto bad=f;put32(bad.b,f.payloads[0].start+2,8193);return reject(bad,"string byte");});
    check("class_and_object_tags_kind_zero_range_and_family",[]{auto f=states();const auto ref=f.payloads[0].end;for(uint32_t id:{1u,1024u}){auto bad=f;put32(bad.b,ref+2,id);if(!reject(bad,"object reference"))return false;}const auto cls=f.payloads[1].start-6;for(uint32_t id:{0u,2u,1024u}){auto bad=f;put32(bad.b,cls+2,0x80000000|id);if(!reject(bad,"class reference"))return false;}auto bad=f;const char wrong[]="FSAMatch";std::memcpy(bad.b.data()+f.declarations[0]+6,wrong,8);return reject(bad,"expected family");});
    check("duplicate_class_declarations_keep_separate_ids",[]{auto f=states();const auto p=f.payloads[1].start-6;Bytes declaration;declaration={0xff,0xff,0,0,8,0,'F','S','A','S','t','a','t','e'};f.b.erase(f.b.begin()+p,f.b.begin()+p+6);f.b.insert(f.b.begin()+p,declaration.begin(),declaration.end());auto r=walk(f);return r.declarations.size()==2&&r.payloads.size()==2&&r.cache.size()==5&&r.payloads[0].class_id==1&&r.payloads[1].class_id==3&&r.payloads[1].object_id==4;});
    check("positive_list_and_input_view_budgets",[]{auto f=states();auto bad=f;put32(bad.b,f.counts[0],257);if(!reject(bad,"list budget"))return false;FigureFsaObjectLimits l;l.max_bytes=f.b.size()-1;if(!reject(f,"argument",l))return false;l={};l.max_refs=1;return reject(f,"reference budget",l);});
    check("translated_prefix_and_unconsumed_trailing_bytes",[]{for(size_t p:{1u,19u,257u,4096u}){auto f=states(1,3,p);f.b.insert(f.b.end(),23,0xff);if(!golden(f,walk(f)))return false;}return true;});
    check("failures_are_readonly_and_later_invocation_clean",[]{auto f=states();auto before=f.b;std::vector<FigureFsaWireCacheEntry> c{{}};for(size_t n=0;n<f.end;n+=7){auto bad=f;bad.b.resize(n);if(!reject(bad,"",{},c)||c.size()!=1||c[0].kind!=FigureFsaCacheKind::Null||f.b!=before||!golden(f,walk(f)))return false;}return true;});
    check("explicit_contract_argument_and_initial_cache_validation",[]{auto f=states();try{(void)walk_city19_figurefsa_exact_objects({nullptr,0},static_cast<FigureFsaObjectContract>(99));return false;}catch(const std::runtime_error&e){if(std::string(e.what()).find("explicit contract")==std::string::npos)return false;}std::vector<FigureFsaWireCacheEntry> c{{},{FigureFsaCacheKind::Class,FigureFsaWireType::State,0,3}};return reject(f,"initial cache",{},c);});
    check("readonly_exact_record_guard_page",[]{auto f=states(1,5);auto n=f.b.size();long p=sysconf(_SC_PAGESIZE);if(p<=0||n>size_t(p))return false;auto ps=size_t(p);void*mem=mmap(nullptr,ps*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);if(mem==MAP_FAILED)return false;auto*end=static_cast<uint8_t*>(mem)+ps;auto*start=end-n;std::memcpy(start,f.b.data(),n);bool ok=mprotect(mem,ps,PROT_READ)==0&&mprotect(end,ps,PROT_NONE)==0;if(ok){auto r=walk_city19_figurefsa_exact_objects({start,n},contract);ok=golden(f,r)&&!std::memcmp(start,f.b.data(),n);}munmap(mem,ps*2);return ok;});
    size_t failed=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"id\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
