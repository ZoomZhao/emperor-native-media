#include "city19_later7_requests.hpp"
#include "golden_requests.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <sys/mman.h>
#include <unistd.h>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
constexpr size_t wire=12107, cap=16384;
Bytes record(size_t start=0,size_t tail=0,unsigned seed=1) {
    Bytes b(start+wire+tail);
    for(size_t i=0;i<b.size();++i)b[i]=uint8_t((i*37u+seed*53u)%256u);
    b[start]=7;b[start+1]=0;return b;
}
Later7Walk walk(const Bytes&b,size_t start=0,size_t budget=cap) {
    return walk_city19_later7_requests({b.data(),b.size()},start,budget);
}
bool rejects(GoalBytes b,size_t start,size_t budget,const char* reason) {
    try {(void)walk_city19_later7_requests(b,start,budget);return false;}
    catch(const std::runtime_error&e){return std::string(e.what()).find(reason)!=std::string::npos;}
}
bool same(const Later7Walk&a,const Later7Walk&b,size_t shift=0) {
    if(a.start+shift!=b.start||a.end+shift!=b.end||a.version!=b.version||a.fields.size()!=b.fields.size())return false;
    for(size_t i=0;i<a.fields.size();++i) {
        const auto &x=a.fields[i],&y=b.fields[i];
        if(x.offset+shift!=y.offset||x.bytes!=y.bytes||x.source_call!=y.source_call)return false;
    }
    return true;
}
int main() {
    std::vector<std::pair<std::string,bool>> checks;
    auto check=[&](const char*id,std::function<bool()>fn) {
        bool ok=false;try {ok=fn();}catch(const std::exception&){ok=false;}checks.push_back({id,ok});
    };
    check("golden245",[]{
        const auto r=walk(record());
        if(r.version!=7||r.fields.size()!=245||r.start||r.end!=wire)return false;
        for(size_t i=0;i<245;++i) {
            const auto &f=r.fields[i];const auto &g=golden_requests[i];
            if(f.source_call!=g.call||f.bytes!=g.bytes||f.offset!=g.offset)return false;
        }
        return r.fields.back().source_call==0x59a107&&r.fields.back().offset==12103;
    });
    check("translation",[]{
        const auto reference=walk(record());
        for(size_t start:{0u,1u,2u,17u,255u,4096u,4277u})for(unsigned seed:{0u,1u,17u,255u}) {
            auto b=record(start,0,seed);const auto copy=b;
            if(!same(reference,walk(b,start),start)||b!=copy)return false;
        }
        return true;
    });
    check("late_boundary",[]{
        auto exact=record(cap-wire);auto cut=record(cap-wire+1);cut.resize(cap);
        auto full=record(cap-wire+1);
        return walk(exact,cap-wire).end==cap&&
            rejects({cut.data(),cut.size()},cap-wire+1,cap,"truncated")&&
            rejects({full.data(),full.size()},cap-wire+1,cap,"bounded view");
    });
    check("whole_view_budget",[]{
        auto exact=record();auto tail=record(0,1);
        return walk(exact,0,wire).end==wire&&walk(tail).end==wire&&
            rejects({tail.data(),tail.size()},0,wire,"bounded view")&&
            rejects({exact.data(),exact.size()},0,cap+1,"bounded view");
    });
    check("empty_word",[]{
        uint8_t one=7;auto b=record(257);
        return rejects({&one,0},0,0,"truncated")&&rejects({&one,1},0,1,"truncated")&&
            rejects({b.data(),b.size()},b.size(),cap,"truncated")&&
            rejects({b.data(),b.size()},b.size()-1,cap,"truncated");
    });
    check("extreme_guard",[]{
        uint8_t one=7;const size_t huge=std::numeric_limits<size_t>::max();
        return rejects({nullptr,0},0,cap,"bounded view")&&
            rejects({nullptr,huge},huge,huge,"bounded view")&&
            rejects({&one,huge},0,cap,"bounded view")&&
            rejects({&one,1},huge,cap,"bounded view")&&
            rejects({&one,1},0,huge,"bounded view");
    });
    check("field_cuts",[]{
        constexpr size_t start=257;auto full=record(start);const auto original=full;
        for(const auto &g:golden_requests)for(size_t cut:{g.offset-1,g.offset,g.offset+g.bytes-1}) {
            const size_t n=start+cut;
            if(n>=full.size()||!rejects({full.data(),n},start,cap,"truncated"))return false;
        }
        return walk(full,start).end==start+wire&&full==original;
    });
    check("version_encoding",[]{
        for(uint16_t v:{0,1,6,8,0x0700,0x0107,0xffff}) {
            auto b=record(4096);b[4096]=uint8_t(v);b[4097]=uint8_t(v>>8);
            if(!rejects({b.data(),b.size()},4096,cap,"selected source7 subset"))return false;
        }
        return walk(record(4096),4096).version==7;
    });
    check("failure_recovery",[]{
        const auto reference=walk(record());
        for(unsigned i=0;i<32;++i) {
            auto good=record(i,0,i);const auto copy=good;auto bad=good;bad.resize(i+12106);
            if(!rejects({bad.data(),bad.size()},i,cap,"truncated"))return false;
            bad=good;bad[i]=8;
            if(!rejects({bad.data(),bad.size()},i,cap,"selected source7 subset"))return false;
            if(!same(reference,walk(good,i),i)||good!=copy)return false;
        }
        return true;
    });
    check("readonly_guard",[]{
        const long page=sysconf(_SC_PAGESIZE);if(page<=0)return false;
        const size_t ps=size_t(page), readable=((wire+ps-1)/ps)*ps, total=readable+ps;
        // Own synthetic bytes only; a following page traps reads/writes beyond this record.
        void* mem=mmap(nullptr,total,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
        if(mem==MAP_FAILED)return false;
        auto *data=static_cast<uint8_t*>(mem);const size_t start=readable-wire;
        auto source=record(start);std::memcpy(data,source.data(),readable);
        bool ok=mprotect(data,readable,PROT_READ)==0&&mprotect(data+readable,ps,PROT_NONE)==0;
        if(ok) {
            const auto r=walk_city19_later7_requests({data,readable},start,cap);
            ok=r.end==readable&&r.fields.size()==245&&std::memcmp(data,source.data(),readable)==0;
        }
        munmap(mem,total);return ok;
    });
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i) {
        if(i)std::cout<<',';if(!checks[i].second)++failed;
        std::cout<<"{\"id\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';
    }
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
