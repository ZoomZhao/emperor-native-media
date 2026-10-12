#include "city19_empire_inline_archive.hpp"
#include <iostream>
#include <functional>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void word(Bytes&b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void dword(Bytes&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
GoodsWalk prior(){GoodsWalk p;p.complete=true;p.end=7;p.count=0;p.cache_next=6020;return p;}
Bytes list(uint16_t version,uint32_t n){Bytes b(7,255);word(b,version);dword(b,n);return b;}
void record(Bytes&b,uint16_t version=2){word(b,version);b.insert(b.end(),38,255);}
EmpireInlineWalk walk(const Bytes&b,EmpireInlineLimits l={}){auto p=prior();return walk_city19_empire_inline({b.data(),b.size()},p,l);}
bool rejects(const Bytes&b,const char*why,EmpireInlineLimits l={}){try{(void)walk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("21 requests38 with shared tail4,not destination size",[]{size_t n=0;for(const auto&f:empire2_fields)n+=f.bytes;return std::size(empire2_fields)==21&&n==38&&empire2_fields[18].receiver_offset==32&&empire2_fields[18].bytes==4&&empire2_fields[20].receiver_offset==40&&empire2_fields[20].bytes==4;});
    check("source3 prefix one explicit object2 consumes40",[]{auto b=list(3,1);record(b);auto old=b;auto r=walk(b);return r.inline_complete&&r.records.size()==1&&r.records[0].version==2&&r.records[0].end-r.records[0].start==40&&r.end==53&&r.cache_next==6020&&r.following_grids_required&&r.trailing_parent4_required&&b==old;});
    check("source1 stops after inline,without grids or parent4",[]{auto b=list(1,1);record(b);auto r=walk(b);return r.end==53&&r.inline_complete&&!r.following_grids_required&&!r.trailing_parent4_required;});
    check("source2 follows grids but has no parent4",[]{auto b=list(2,1);record(b);auto r=walk(b);return r.end==53&&r.inline_complete&&r.following_grids_required&&!r.trailing_parent4_required;});
    check("all three zero counts consume6 only",[]{for(uint16_t v:{1,2,3}){auto b=list(v,0);b.insert(b.end(),13,255);auto r=walk(b);if(r.end!=13||!r.inline_complete||!r.records.empty())return false;}return true;});
    check("200 explicit records exact without archive IDs",[]{auto b=list(3,200);for(int i=0;i<200;++i)record(b);auto r=walk(b);return r.inline_complete&&r.records.size()==200&&r.records.back().ordinal==199&&r.end==13+8000&&r.cache_next==6020;});
    check("201 records no inferred default200 upperbound",[]{auto b=list(3,201);for(int i=0;i<201;++i)record(b);auto r=walk(b);return r.inline_complete&&r.records.size()==201&&r.end==b.size();});
    check("all40 proper object prefixes reject truncated",[]{auto b=list(3,1);record(b);for(size_t n=0;n<40;++n){Bytes cut(b.begin(),b.begin()+13+n);if(!rejects(cut,"truncated"))return false;}return walk(b).inline_complete;});
    check("all6 parent header prefixes reject truncated",[]{auto b=list(3,0);for(size_t n=0;n<6;++n){Bytes cut(b.begin(),b.begin()+7+n);if(!rejects(cut,"truncated"))return false;}return true;});
    check("unsupported parent0,4,ffff outside static selected branches",[]{for(uint16_t v:{0,4,65535})if(!rejects(list(v,0),"parent outside qualified"))return false;return true;});
    check("legacy object1 raw getter remains outside explicit2 subset",[]{for(uint16_t v:{0,1,3,65535}){auto b=list(3,1);record(b,v);if(!rejects(b,"explicit source2 subset"))return false;}return true;});
    check("negative count and modern8193 refuse",[]{return rejects(list(3,0xffffffff),"negative Empire")&&rejects(list(3,0x80000000),"negative Empire")&&rejects(list(3,8193),"exceeds modern budget");});
    check("checkpoint before missing next object",[]{auto b=list(3,2);record(b);EmpireInlineLimits l;l.max_items=1;auto r=walk(b,l);return r.records.size()==1&&!r.inline_complete&&r.end==b.size();});
    check("unqualified first grid header and payload stay unread",[]{auto b=list(3,1);record(b);size_t end=b.size();word(b,15);b.insert(b.end(),15360,255);auto r=walk(b);return r.end==end&&r.following_grids_required&&b.size()-r.end==15362;});
    check("incomplete previousGoods refuses",[]{auto b=list(3,0);auto p=prior();p.complete=false;try{(void)walk_city19_empire_inline({b.data(),b.size()},p);return false;}catch(const std::runtime_error&){return true;}});
    check("mismatched preceding count refuses",[]{auto b=list(3,0);auto p=prior();p.count=1;try{(void)walk_city19_empire_inline({b.data(),b.size()},p);return false;}catch(const std::runtime_error&){return true;}});
    check("zero or overhard modern limits refuse",[]{auto b=list(3,0);for(int i=0;i<5;++i){EmpireInlineLimits l;if(i==0)l.max_count=0;if(i==1)l.max_count=8193;if(i==2)l.max_items=0;if(i==3)l.max_items=8193;if(i==4)l.max_prefix=4915585;if(!rejects(b,"context/modern budget",l))return false;}return true;});
    check("header-only checkpoint does not inspect unsupported child",[]{auto b=list(3,1);record(b,65535);EmpireInlineLimits l;l.header_only=true;auto r=walk(b,l);return r.end==13&&r.count==1&&r.records.empty()&&!r.inline_complete;});
    size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
