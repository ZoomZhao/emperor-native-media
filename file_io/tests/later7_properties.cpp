#include "city19_later7_requests.hpp"
#include <functional>
#include <iostream>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void word(Bytes&b,uint16_t v){b.push_back(uint8_t(v));b.push_back(uint8_t(v>>8));}
Bytes record(){Bytes b;word(b,7);b.insert(b.end(),12105,255);return b;}
Later7Walk walk(const Bytes&b,size_t start=0,size_t limit=16384){return walk_city19_later7_requests({b.data(),b.size()},start,limit);}
bool rejects(const Bytes&b,const char *why,size_t start=0,size_t limit=16384){try{(void)walk(b,start,limit);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,std::function<bool()>f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("245 literal rawrequests plus word consume12107",[]{auto b=record();auto original=b;auto r=walk(b);return r.version==7&&r.fields.size()==245&&r.start==0&&r.end==12107&&b==original;});
    check("request ranges are contiguous and last shared tail4 mandatory",[]{auto b=record();auto r=walk(b);size_t end=2;for(const auto&f:r.fields){if(f.offset!=end)return false;end+=f.bytes;}return end==12107&&r.fields.back().source_call==0x59a107&&r.fields.back().bytes==4&&r.fields.back().offset==12103;});
    check("all12107 proper record cuts fail before success",[]{auto b=record();for(size_t n=0;n<12107;++n){Bytes cut(b.begin(),b.begin()+n);if(!rejects(cut,n?"truncated":"bounded view"))return false;}return true;});
    check("nonzero buffer start preserves unrelated prefix",[]{Bytes b(11,42);auto r=record();b.insert(b.end(),r.begin(),r.end());auto out=walk(b,11);return out.start==11&&out.end==12118&&out.fields.front().offset==13&&b.front()==42;});
    check("unknown following bytes stay unread",[]{auto b=record();b.insert(b.end(),64,42);auto r=walk(b);return r.end==12107&&b.size()-r.end==64;});
    check("other sourceversions rejected by modern subset only",[]{for(uint16_t v:{0,1,6,8,65535}){auto b=record();b[0]=uint8_t(v);b[1]=uint8_t(v>>8);if(!rejects(b,"selected source7 subset"))return false;}return true;});
    check("exact16384 view accepted andone extra refused",[]{auto b=record();b.resize(16384,42);auto r=walk(b);b.push_back(42);return r.end==12107&&rejects(b,"bounded view");});
    check("budget16385 andoutofrange start refuse",[]{auto b=record();return rejects(b,"bounded view",0,16385)&&rejects(b,"bounded view",b.size()+1);});
    size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
