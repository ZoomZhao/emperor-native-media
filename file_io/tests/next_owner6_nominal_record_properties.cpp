#include "city19_next_owner6_nominal_record.hpp"
#include <functional>
#include <iostream>
#include <limits>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
constexpr auto contract=NextOwner6EmbeddedContract::nominal_52c480;
Bytes record(){Bytes b{6,0};b.insert(b.end(),1459,255);b.push_back(1);b.push_back(0);b.insert(b.end(),16,255);return b;}
NextOwner6NominalRecord walk(const Bytes&b,size_t start=0,size_t limit=4096){return walk_city19_next_owner6_nominal_record({b.data(),b.size()},contract,start,limit);}
bool rejects(const Bytes&b,const char *why,size_t start=0,size_t limit=4096){try{(void)walk(b,start,limit);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){
    std::vector<std::pair<std::string,bool>> checks;
    auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("selected parent6 embedded1 record consumes1479",[]{auto b=record();auto r=walk(b);return r.start==0&&r.end==1479&&r.parent_version==6&&r.embedded_version==1&&r.parent_fields.size()==66&&!r.verifies_original_instance;});
    check("embedded word plus raw4 and12 are separate ranges",[]{auto b=record();auto r=walk(b);return r.prefix_end==1461&&r.embedded_begin==1461&&r.embedded_fields[0].offset==1463&&r.embedded_fields[0].bytes==4&&r.embedded_fields[0].source_call==0x52c4cf&&r.embedded_fields[1].offset==1467&&r.embedded_fields[1].bytes==12&&r.embedded_fields[1].source_call==0x52c4dc;});
    check("prior prefix API still stops at1461 and remains pending",[]{auto b=record();auto r=walk_city19_next_owner6_prefix({b.data(),b.size()});return r.end==1461&&r.pending_embedded_reader&&r.embedded_bytes_consumed==0;});
    check("all1479 insufficient selected record cuts fail",[]{auto b=record();for(size_t n=0;n<1479;++n){Bytes cut(b.begin(),b.begin()+n);if(!rejects(cut,n?"truncated":"bounded view"))return false;}return true;});
    check("prefix success cannot silently complete missing embedded record",[]{auto b=record();b.resize(1461);auto p=walk_city19_next_owner6_prefix({b.data(),b.size()});return p.end==1461&&rejects(b,"truncated");});
    check("other embedded versions refused only by modern subset",[]{for(uint16_t v:{0,2,65535}){auto b=record();b[1461]=uint8_t(v);b[1462]=uint8_t(v>>8);if(!rejects(b,"selected own1 subset"))return false;}return true;});
    check("other parent versions refused only by modern subset",[]{for(uint16_t v:{0,3,4,5,7,65535}){auto b=record();b[0]=uint8_t(v);b[1]=uint8_t(v>>8);if(!rejects(b,"selected source6 subset"))return false;}return true;});
    check("unsupported source contract never guesses embedded reader",[]{auto b=record();try{(void)walk_city19_next_owner6_nominal_record({b.data(),b.size()},static_cast<NextOwner6EmbeddedContract>(1));return false;}catch(const std::runtime_error&e){return std::string(e.what()).find("unsupported modern embedded contract")!=std::string::npos;}});
    check("nonzero start keeps parent and child offsets exact",[]{Bytes b(13,42);auto r=record();b.insert(b.end(),r.begin(),r.end());auto out=walk(b,13);return out.start==13&&out.prefix_end==1474&&out.embedded_begin==1474&&out.embedded_fields[0].offset==1476&&out.end==1492;});
    check("following bytes remain unread after selected record",[]{auto b=record();b.insert(b.end(),128,42);auto r=walk(b);return r.end==1479&&b.size()-r.end==128;});
    check("payload remains opaque and input immutable",[]{auto b=record();for(size_t i=2;i<b.size();++i)if(i!=1461&&i!=1462)b[i]=uint8_t(i*37);auto before=b;auto r=walk(b);return b==before&&r.end==1479;});
    check("view4096 accepted and one extra rejected",[]{auto b=record();b.resize(4096,42);auto r=walk(b);b.push_back(42);return r.end==1479&&rejects(b,"bounded view");});
    check("budget and integer extreme starts refuse without wrapping",[]{auto b=record();return rejects(b,"bounded view",0,4097)&&rejects(b,"bounded view",0,1478)&&rejects(b,"truncated",b.size())&&rejects(b,"bounded view",std::numeric_limits<size_t>::max());});
    check("null nonempty input refuses before decoding",[]{try{(void)walk_city19_next_owner6_nominal_record({nullptr,1479},contract);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find("bounded view")!=std::string::npos;}});
    size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
