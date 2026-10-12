#include "city19_empire_inline_archive.hpp"
#include <iostream>
#include <functional>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void u16(Bytes&b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void u32(Bytes&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
void declaration(Bytes&b,const char*name,uint16_t schema=0){u16(b,65535);u16(b,schema);u16(b,uint16_t(std::strlen(name)));b.insert(b.end(),name,name+std::strlen(name));}
void building(Bytes&b,bool active=true){u16(b,5);b.push_back(active?1:0);b.insert(b.end(),179,255);}
Bytes prefix(){
    Bytes b(3355);u16(b,1);u32(b,1);declaration(b,"cHousingGoal");u16(b,1);u32(b,11);u16(b,1);for(int i=0;i<3;++i)u32(b,12+i);
    for(int i=0;i<9;++i){u16(b,1);u32(b,0);}
    u16(b,1);u32(b,11);b.push_back(12);for(int i=0;i<3;++i)u32(b,13+i);b.insert(b.end(),0xcb10,7);
    u16(b,4);u16(b,1);for(int i=0;i<12;++i)u32(b,21+i);u32(b,41);u32(b,42);b.push_back(43);b.push_back(44);u32(b,45);b.push_back(46);for(int i=0;i<4;++i)u32(b,47+i);
    u16(b,1);for(int c=0;c<4;++c){u16(b,3);for(int m=0;m<3;++m){u16(b,2);for(int f=0;f<5;++f)u32(b,100+c*15+m*5+f);}}
    b.insert(b.end(),20,255);u16(b,1);return b;
}
struct Context{GoalWalk goals;ReligionFollowingWalk before;};
Context context(const Bytes&b){auto g=walk_city19_housing_goals({b.data(),std::min(b.size(),size_t(16384))});auto p=walk_city19_postgoals({b.data(),std::min(b.size(),size_t(65536))},g,true);return {g,walk_city19_religion_following({b.data(),std::min(b.size(),size_t(65536))},p,true)};}

PoolWalk pool_before(const Bytes&b){auto c=context(b);return walk_city19_mainpool({b.data(),std::min(b.size(),size_t(1048576))},c.goals,c.before);}
Bytes base(){auto b=prefix();u16(b,1);u32(b,1);declaration(b,"Building");building(b);return b;}
void figure(Bytes&b,uint8_t present=0){u16(b,7);b.insert(b.end(),395,255);b.push_back(present);}
void pheasant(Bytes&b,uint8_t present=0){u16(b,2);figure(b,present);}
void goods(Bytes&b){u16(b,1);b.insert(b.end(),268,255);}
Bytes flist(uint32_t n){auto b=base();u16(b,1);u32(b,n);return b;}
Bytes mixed(uint32_t n=4){auto b=flist(n);declaration(b,"Figure");figure(b);declaration(b,"cPheasantFig");pheasant(b);u16(b,0x8007);pheasant(b);u16(b,8);return b;}
AfterPoolWalk figures(const Bytes&b,bool enable=true,AfterPoolLimits l={}){auto p=pool_before(b);return walk_city19_figure_variants({b.data(),b.size()},p,l,enable);}
GoodsWalk walkgoods(const Bytes&b,GoodsLimits l={}){auto f=figures(b);return walk_city19_goodscontrol({b.data(),b.size()},f,l);}
bool goods_rejects(const Bytes&b,const char*why,GoodsLimits l={}){try{(void)walkgoods(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
bool fig_rejects(const Bytes&b,const char*why){try{(void)figures(b);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}

Bytes ready(uint16_t parent,uint32_t count){auto b=mixed();u16(b,1);u32(b,2);goods(b);goods(b);u16(b,parent);u32(b,count);return b;}
void empire(Bytes&b,uint8_t value=255){u16(b,2);b.insert(b.end(),38,value);}
EmpireInlineWalk walk(const Bytes&b,EmpireInlineLimits l={}){auto g=walkgoods(b);return walk_city19_empire_inline({b.data(),b.size()},g,l);}
bool rejects(const Bytes&b,const char*why,EmpireInlineLimits l={}){try{(void)walk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,const std::function<bool()>&f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
check("190 explicit40 records continue true Goods context without cache IDs",[&]{auto b=ready(3,190);size_t start=b.size()-6;for(int i=0;i<190;++i)empire(b,uint8_t(i));auto old=b;auto g=walkgoods(b);auto r=walk(b);return g.cache_next==10&&r.cache_next==10&&r.start==g.end&&r.start==start&&r.inline_complete&&r.records.size()==190&&r.records.back().ordinal==189&&r.end==start+6+190*40&&b==old;});
check("forty wire bytes keep back to back records independent of stride44",[&]{auto b=ready(3,2);size_t s=b.size();empire(b,255);empire(b,0);auto r=walk(b);return r.records.size()==2&&r.records[0].start==s&&r.records[0].end==s+40&&r.records[1].start==s+40&&r.records[1].end==s+80;});
check("source1 byte requests end without grids but postload unexecuted",[&]{auto b=ready(1,1);empire(b);auto r=walk(b);return r.inline_complete&&!r.following_grids_required&&!r.trailing_parent4_required&&r.end==b.size()&&r.stop_reason=="Empire_source1_byte_requests_complete_postload_not_executed";});
check("source2 and3 stop before grid word and differ on parent4 obligation",[&]{for(uint16_t v:{2,3}){auto b=ready(v,1);empire(b);size_t end=b.size();u16(b,65535);b.insert(b.end(),100,255);auto old=b;auto r=walk(b);if(!r.inline_complete||!r.following_grids_required||r.trailing_parent4_required!=(v==3)||r.end!=end||b.size()-r.end!=102||b!=old)return false;}return true;});
check("all forty object and six parent header cuts reject",[&]{auto b=ready(3,1);size_t s=b.size();empire(b);for(size_t i=0;i<40;++i){Bytes cut(b.begin(),b.begin()+s+i);if(!rejects(cut,"truncated"))return false;}for(size_t i=0;i<6;++i){Bytes cut(b.begin(),b.begin()+s-6+i);if(!rejects(cut,"truncated"))return false;}return true;});
check("last shared tail DWORD byte is mandatory",[&]{auto b=ready(3,1);empire(b);b.pop_back();return rejects(b,"truncated");});
check("legacy child1 raw40 alternative is not explicit child2",[&]{auto b=ready(3,1);u16(b,1);b.insert(b.end(),40,255);return rejects(b,"explicit source2 subset");});
check("header only validates count without inspecting bad child",[&]{auto b=ready(3,8192);u16(b,65535);EmpireInlineLimits l;l.header_only=true;auto r=walk(b,l);return r.count==8192&&r.records.empty()&&!r.inline_complete&&r.end==b.size()-2&&r.following_grids_required&&r.trailing_parent4_required;});
check("header only still rejects signed and modern count extremes",[&]{EmpireInlineLimits l;l.header_only=true;for(uint32_t v:{0xffffffffU,0x80000000U,0x7fffffffU,8193U})if(!rejects(ready(3,v),v>0x7fffffff?"negative Empire":"count exceeds modern budget",l))return false;return true;});
check("first missing next record checkpoint keeps count and cache",[&]{auto b=ready(3,2);empire(b);EmpireInlineLimits l;l.max_items=1;auto r=walk(b,l);return !r.inline_complete&&r.count==2&&r.records.size()==1&&r.end==b.size()&&r.cache_next==10;});
check("201 no default200 rejection or padding",[&]{auto b=ready(3,201);for(int i=0;i<201;++i)empire(b);auto r=walk(b);return r.records.size()==201&&r.inline_complete&&r.end==b.size();});
check("zero count branches leave required grid tail unread",[&]{for(uint16_t v:{1,2,3}){auto b=ready(v,0);size_t end=b.size();b.insert(b.end(),22,255);auto r=walk(b);if(!r.inline_complete||!r.records.empty()||r.end!=end||r.following_grids_required!=(v>=2)||r.trailing_parent4_required!=(v==3))return false;}return true;});
check("prefix4915584 accepts unknown tail and4915585 rejects",[&]{auto b=ready(3,1);empire(b);size_t end=b.size();b.resize(4915584,255);auto r=walk(b);b.push_back(255);return r.end==end&&rejects(b,"context/modern budget");});
check("Goods item checkpoint cannot masquerade as complete Empire context",[&]{auto b=mixed();u16(b,1);u32(b,2);goods(b);GoodsLimits gl;gl.max_items=1;auto g=walkgoods(b,gl);u16(b,3);u32(b,0);try{(void)walk_city19_empire_inline({b.data(),b.size()},g);return false;}catch(const std::runtime_error&e){return !g.complete&&std::string(e.what()).find("preceding context")!=std::string::npos;}});
size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
