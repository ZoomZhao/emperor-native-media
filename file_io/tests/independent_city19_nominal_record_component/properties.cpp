#include "city19_next_owner6_nominal_record.hpp"
#include "golden_requests.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <sys/mman.h>
#include <type_traits>
#include <unistd.h>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
constexpr size_t prefix_wire=1461,record_wire=1479,cap=4096;
constexpr auto selected=NextOwner6EmbeddedContract::nominal_52c480;
template<class B,class=void>struct implicit_contract_call:std::false_type{};
template<class B>struct implicit_contract_call<B,std::void_t<decltype(walk_city19_next_owner6_nominal_record(std::declval<B>()))>>:std::true_type{};
static_assert(!implicit_contract_call<GoalBytes>::value,"nominal source contract must be explicit");
static_assert(!NextOwner6NominalRecord::verifies_original_instance,"synthetic model must not claim original binding");
Bytes record(size_t start=0,unsigned seed=0) {
    Bytes b(start+record_wire);
    for(size_t i=0;i<b.size();++i)b[i]=uint8_t((i*37u+seed*53u)%256u);
    b[start]=6;b[start+1]=0;b[start+prefix_wire]=1;b[start+prefix_wire+1]=0;return b;
}
NextOwner6NominalRecord walk(const Bytes&b,size_t start=0) {
    return walk_city19_next_owner6_nominal_record({b.data(),b.size()},selected,start);
}
bool rejects(GoalBytes b,size_t start,const char* why,NextOwner6EmbeddedContract contract=selected) {
    try{(void)walk_city19_next_owner6_nominal_record(b,contract,start);return false;}
    catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}
}
bool mapped(const NextOwner6NominalRecord&r,size_t start) {
    if(r.verifies_original_instance||r.start!=start||r.parent_version!=6||r.embedded_version!=1||r.prefix_end!=start+prefix_wire||r.embedded_begin!=r.prefix_end||r.end!=start+record_wire||r.parent_fields.size()!=66)return false;
    for(size_t i=0;i<66;++i) {
        const auto &f=r.parent_fields[i];const auto &g=golden_requests[i];
        if(f.offset!=start+g.offset||f.bytes!=g.bytes||f.source_call!=g.call)return false;
    }
    const auto &x=r.embedded_fields[0],&y=r.embedded_fields[1];
    return x.offset==start+1463&&x.bytes==4&&x.source_call==0x52c4cf&&y.offset==start+1467&&y.bytes==12&&y.source_call==0x52c4dc;
}
int main(){
    std::vector<std::pair<std::string,bool>> checks;
    auto check=[&](const char*id,std::function<bool()>fn){bool ok=false;try{ok=fn();}catch(const std::exception&){ok=false;}checks.push_back({id,ok});};
    check("explicit_contract_not_instance",[]{
        auto b=record();const auto r=walk(b);const auto p=walk_city19_next_owner6_prefix({b.data(),b.size()});
        return !implicit_contract_call<GoalBytes>::value&&!r.verifies_original_instance&&p.pending_embedded_reader&&p.embedded_bytes_consumed==0&&p.pending_source_call==0x52d5f6;
    });
    check("composition_66_plus2",[]{
        for(size_t start:{0u,1u,17u,255u,2617u}){
            auto b=record(start);const auto copy=b;const auto p=walk_city19_next_owner6_prefix({b.data(),b.size()},start);const auto r=walk(b,start);
            if(!mapped(r,start)||!p.pending_embedded_reader||p.end!=r.embedded_begin||p.fields.size()!=r.parent_fields.size()||b!=copy)return false;
            for(size_t i=0;i<p.fields.size();++i)if(p.fields[i].offset!=r.parent_fields[i].offset||p.fields[i].bytes!=r.parent_fields[i].bytes||p.fields[i].source_call!=r.parent_fields[i].source_call)return false;
        }
        return true;
    });
    check("translation_opaque_payload",[]{
        for(size_t start:{0u,2u,257u,2617u})for(unsigned seed:{0u,1u,17u,255u}){
            auto b=record(start,seed);const auto copy=b;if(!mapped(walk(b,start),start)||b!=copy)return false;
        }
        return true;
    });
    check("latest_full_record_boundary",[]{
        auto exact=record(cap-record_wire);auto cut=record(cap-record_wire+1);cut.resize(cap);auto extra=record(cap-record_wire+1);
        auto p=walk_city19_next_owner6_prefix({cut.data(),cut.size()},cap-record_wire+1);
        return mapped(walk(exact,cap-record_wire),cap-record_wire)&&p.end==cap-17&&p.pending_embedded_reader&&
            rejects({cut.data(),cut.size()},cap-record_wire+1,"truncated")&&rejects({extra.data(),extra.size()},cap-record_wire+1,"bounded view");
    });
    check("partial_embedded_cuts_do_not_complete",[]{
        constexpr size_t start=257;auto b=record(start);const auto copy=b;
        for(size_t n=0;n<18;++n){
            GoalBytes view{b.data(),start+prefix_wire+n};auto p=walk_city19_next_owner6_prefix(view,start);
            if(p.end!=start+prefix_wire||!p.pending_embedded_reader||p.embedded_bytes_consumed||!rejects(view,start,"truncated"))return false;
        }
        return mapped(walk(b,start),start)&&b==copy;
    });
    check("unknown_contract_rejects_before_decode",[]{
        for(int value:{1,2,-1,std::numeric_limits<int>::max()}) {
            if(!rejects({nullptr,0},std::numeric_limits<size_t>::max(),"unsupported modern embedded contract",static_cast<NextOwner6EmbeddedContract>(value)))return false;
        }
        return true;
    });
    check("little_endian_selected_versions_only",[]{
        for(uint16_t v:{0,2,0x0100,0x0101,0xffff}){
            auto b=record(17);b[17+prefix_wire]=uint8_t(v);b[18+prefix_wire]=uint8_t(v>>8);
            if(!rejects({b.data(),b.size()},17,"selected own1 subset"))return false;
        }
        for(uint16_t v:{0,3,4,5,7,0x0600,0xffff}){
            auto b=record(17);b[17]=uint8_t(v);b[18]=uint8_t(v>>8);
            if(!rejects({b.data(),b.size()},17,"selected source6 subset"))return false;
        }
        return mapped(walk(record(17),17),17);
    });
    check("failed_child_call_does_not_leak_state",[]{
        for(unsigned i=0;i<32;++i){
            auto good=record(i,i);const auto copy=good;auto bad=good;bad.resize(i+record_wire-1);
            if(!rejects({bad.data(),bad.size()},i,"truncated"))return false;
            bad=good;bad[i+prefix_wire]=2;
            if(!rejects({bad.data(),bad.size()},i,"selected own1 subset"))return false;
            if(!mapped(walk(good,i),i)||good!=copy)return false;
        }
        return true;
    });
    check("readonly_record_at_protected_page",[]{
        const long page=sysconf(_SC_PAGESIZE);if(page<=0)return false;
        const size_t ps=size_t(page),readable=((record_wire+ps-1)/ps)*ps,total=readable+ps;
        void* mem=mmap(nullptr,total,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);if(mem==MAP_FAILED)return false;
        auto* data=static_cast<uint8_t*>(mem);const size_t start=readable-record_wire;const auto source=record(start);
        std::memcpy(data,source.data(),readable);bool ok=mprotect(data,readable,PROT_READ)==0&&mprotect(data+readable,ps,PROT_NONE)==0;
        if(ok){const auto r=walk_city19_next_owner6_nominal_record({data+start,record_wire},selected);ok=mapped(r,0)&&std::memcmp(data,source.data(),readable)==0;}
        munmap(mem,total);return ok;
    });
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"id\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
