#include "city19_next_owner6_prefix.hpp"
#include <functional>
#include <iostream>
#include <limits>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
Bytes record(){Bytes b{6,0};b.insert(b.end(),1459,255);return b;}
NextOwner6Prefix walk(const Bytes&b,size_t start=0,size_t limit=4096){return walk_city19_next_owner6_prefix({b.data(),b.size()},start,limit);}
bool rejects(const Bytes&b,const char *why,size_t start=0,size_t limit=4096){
    try{(void)walk(b,start,limit);return false;}
    catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}
}
int main(){
    std::vector<std::pair<std::string,bool>> checks;
    auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("66 requests consume1459 plus word2",[]{auto b=record();auto r=walk(b);return r.version==6&&r.fields.size()==66&&r.start==0&&r.end==1461;});
    check("all ranges contiguous and final request retained",[]{auto b=record();auto r=walk(b);size_t end=2;for(const auto&f:r.fields){if(f.offset!=end)return false;end+=f.bytes;}return end==1461&&r.fields.back().bytes==1&&r.fields.back().source_call==0x52d5e4;});
    check("complete prefix leaves embedded virtual0 pending",[]{auto b=record();auto r=walk(b);return r.pending_embedded_reader&&r.embedded_bytes_consumed==0&&r.pending_source_call==0x52d5f6;});
    check("all1461 insufficient prefix cuts reject",[]{auto b=record();for(size_t n=0;n<1461;++n){Bytes cut(b.begin(),b.begin()+n);if(!rejects(cut,n?"truncated":"bounded view"))return false;}return true;});
    check("nonzero start preserves offset accounting",[]{Bytes b(17,42);auto r=record();b.insert(b.end(),r.begin(),r.end());auto out=walk(b,17);return out.start==17&&out.end==1478&&out.fields.front().offset==19&&b[0]==42;});
    check("unbound embedded and following bytes stay unread",[]{auto b=record();b.insert(b.end(),128,42);auto r=walk(b);return r.end==1461&&b.size()-r.end==128&&r.pending_embedded_reader;});
    check("input remains opaque and immutable",[]{auto b=record();for(size_t i=2;i<b.size();++i)b[i]=uint8_t(i*37);auto before=b;auto r=walk(b);return b==before&&r.end==1461;});
    check("other versions rejected by modern subset only",[]{for(uint16_t v:{0,1,2,3,4,5,7,65535}){auto b=record();b[0]=uint8_t(v);b[1]=uint8_t(v>>8);if(!rejects(b,"selected source6 subset"))return false;}return true;});
    check("4096 view accepted and one extra rejects",[]{auto b=record();b.resize(4096,42);auto r=walk(b);b.push_back(42);return r.end==1461&&rejects(b,"bounded view");});
    check("oversized budget and exhausted short budget reject",[]{auto b=record();return rejects(b,"bounded view",0,4097)&&rejects(b,"bounded view",0,1460);});
    check("end and overflowing starts never wrap",[]{auto b=record();return rejects(b,"truncated",b.size())&&rejects(b,"bounded view",b.size()+1)&&rejects(b,"bounded view",std::numeric_limits<size_t>::max());});
    check("null nonempty view rejects before decoding",[]{try{(void)walk_city19_next_owner6_prefix({nullptr,1461});return false;}catch(const std::runtime_error&e){return std::string(e.what()).find("bounded view")!=std::string::npos;}});
    size_t fail=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
