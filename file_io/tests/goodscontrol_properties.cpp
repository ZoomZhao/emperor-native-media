#include "city19_goodscontrol_archive.hpp"
#include <iostream>
#include <functional>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void word(Bytes&b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void dword(Bytes&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
AfterPoolWalk prior(){AfterPoolWalk p;p.figures_complete=true;p.count=0;p.end=9;p.cache.push_back({GoalCacheKind::Null,0,0,AfterPoolDomain::EarlierGoal,true});return p;}
Bytes list(uint32_t n,uint16_t version=1){Bytes b(9,255);word(b,version);dword(b,n);return b;}
void record(Bytes&b,uint16_t version=1){word(b,version);b.insert(b.end(),268,255);}
GoodsWalk walk(const Bytes&b,GoodsLimits l={}){auto p=prior();return walk_city19_goodscontrol({b.data(),b.size()},p,l);}
bool rejects(const Bytes&b,const char*why,GoodsLimits l={}){try{(void)walk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("15 sparse explicit requests268 not sizeof272",[]{size_t sum=0;for(const auto&f:goods1_fields)sum+=f.bytes;return std::size(goods1_fields)==15&&sum==268&&goods1_fields[5].receiver_offset==14&&goods1_fields[5].bytes==36&&goods1_fields[11].bytes==72&&goods1_fields[14].receiver_offset==236;});
    check("record word1 plus268 consumes270 opaque bytes",[]{auto b=list(1);record(b);auto before=b;auto r=walk(b);return r.complete&&r.records.size()==1&&r.records[0].version==1&&r.records[0].end-r.records[0].start==270&&r.end==285&&r.cache_next==1&&b==before;});
    check("zero count consumes only6 and preserves tail",[]{auto b=list(0);b.insert(b.end(),28,255);auto r=walk(b);return r.complete&&r.records.empty()&&r.end==15&&b.size()-r.end==28;});
    check("200 self records exact without class IDs or padding",[]{auto b=list(200);for(int i=0;i<200;++i)record(b);auto r=walk(b);return r.complete&&r.records.size()==200&&r.records.back().ordinal==199&&r.end==15+200*270&&r.cache_next==1;});
    check("201 accepted proves constructor default200 not upperbound",[]{auto b=list(201);for(int i=0;i<201;++i)record(b);auto r=walk(b);return r.complete&&r.records.size()==201&&r.end==b.size();});
    check("all270 proper record prefixes reject truncated",[]{auto b=list(1);record(b);for(size_t n=0;n<270;++n){Bytes cut(b.begin(),b.begin()+15+n);if(!rejects(cut,"truncated"))return false;}return walk(b).complete;});
    check("all6 list header prefixes reject truncated",[]{auto b=list(0);for(size_t n=0;n<6;++n){Bytes cut(b.begin(),b.begin()+9+n);if(!rejects(cut,"truncated"))return false;}return true;});
    check("source1 list subset rejects0,2,ffff",[]{for(uint16_t v:{0,2,65535})if(!rejects(list(0,v),"list outside qualified source1"))return false;return true;});
    check("source1 record subset rejects0,2,ffff",[]{for(uint16_t v:{0,2,65535}){auto b=list(1);record(b,v);if(!rejects(b,"record outside qualified source1"))return false;}return true;});
    check("signed negatives and modern count8193 reject",[]{return rejects(list(0xffffffff),"negative GoodsControl")&&rejects(list(0x80000000),"negative GoodsControl")&&rejects(list(8193),"exceeds modern budget");});
    check("item checkpoint before missing next record",[]{auto b=list(2);record(b);GoodsLimits l;l.max_items=1;auto r=walk(b,l);return !r.complete&&r.records.size()==1&&r.count==2&&r.end==b.size();});
    check("next Empire tail remains structurally unread",[]{auto b=list(1);record(b);size_t end=b.size();word(b,3);dword(b,8192);b.insert(b.end(),13,255);auto r=walk(b);return r.end==end&&b.size()-r.end==19&&r.stop_reason=="GoodsControl_complete_before_unqualified_Empire";});
    check("incomplete previous Figure list refuses",[]{auto b=list(0);auto p=prior();p.figures_complete=false;try{(void)walk_city19_goodscontrol({b.data(),b.size()},p);return false;}catch(const std::runtime_error&){return true;}});
    check("pending extension or class refuses context",[]{auto b=list(0);for(int i=0;i<2;++i){auto p=prior();if(i==0)p.extension.present=true;else p.pending.present=true;try{(void)walk_city19_goodscontrol({b.data(),b.size()},p);return false;}catch(const std::runtime_error&){}}return true;});
    check("same prefix budget exact4915584,one extra rejects",[]{auto b=list(0);b.resize(4915584,255);bool exact=walk(b).end==15;b.push_back(255);return exact&&rejects(b,"context/modern budget");});
    check("zero or over hard modern limits refuse",[]{auto b=list(0);for(int i=0;i<5;++i){GoodsLimits l;if(i==0)l.max_count=0;if(i==1)l.max_count=8193;if(i==2)l.max_items=0;if(i==3)l.max_items=8193;if(i==4)l.max_prefix=4915585;if(!rejects(b,"context/modern budget",l))return false;}return true;});
    size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
