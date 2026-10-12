#include "city19_mainpool_archive.hpp"
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
PoolWalk walk(const Bytes&b,PoolLimits l={}){auto c=context(b);return walk_city19_mainpool({b.data(),b.size()},c.goals,c.before,l);}
bool rejects(const Bytes&b,const char*why,PoolLimits l={}){try{(void)walk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
Bytes pool(uint32_t count){Bytes b=prefix();u16(b,1);u32(b,count);return b;}
Bytes one(uint32_t count=1){Bytes b=pool(count);declaration(b,"Building");building(b);return b;}
int main(){
    std::vector<std::pair<std::string,bool>> checks;
    const auto check=[&](const char*n,const std::function<bool()>&f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    const size_t start=55835;
    check("carried Goal cache keeps IDs while Building extends shared archive",[&]{auto b=one();auto c=context(b);auto r=walk(b);return c.before.end==start&&c.goals.cache.size()==3&&r.cache_start==3&&r.cache.size()==5&&r.items.size()==1&&r.items[0].class_id==3&&r.items[0].object_id==4&&r.cache[1].domain==PoolDomain::EarlierGoal&&r.cache[3].domain==PoolDomain::Building;});
    check("Building selected source5 consumes182 and leaves next consumer opaque",[&]{auto b=one();size_t end=b.size();b.insert(b.end(),31,255);auto before=b;auto r=walk(b);return r.complete&&r.end==start+6+14+182&&r.end==end&&r.items.size()==1&&r.items[0].payload_bytes==182&&b==before&&b.size()-r.end==31&&r.stop_reason=="declared_pool_items_complete_next_consumer_unqualified";});
    check("inactive first slot still consumes full payload and retains ordinal",[&]{auto b=pool(2);declaration(b,"Building");building(b,false);u16(b,0x8003);building(b,true);auto r=walk(b);return r.items.size()==2&&r.items[0].ordinal==0&&r.items[1].ordinal==1&&r.items[0].payload_bytes==182&&r.items[1].payload_bytes==182&&r.items[1].object_id==5&&r.end==start+6+14+182+2+182;});
    check("duplicate declarations receive separate conditional class IDs",[&]{auto b=one(2);declaration(b,"Building");building(b);auto r=walk(b);return r.items.size()==2&&r.items[0].class_id==3&&r.items[1].class_id==5&&r.items[1].object_id==6&&r.cache.size()==7&&r.end==start+6+2*(14+182);});
    check("duplicate object references consume tags only and allocate no IDs",[&]{auto b=one(3);u16(b,4);u16(b,4);auto r=walk(b);return r.items.size()==3&&r.cache.size()==5&&r.items[1].object_reference&&r.items[2].object_reference&&r.items[1].payload_bytes==0&&r.items[2].payload_bytes==0&&r.items[2].object_id==4&&r.end==start+6+14+182+4;});
    check("extended class and object tags preserve six byte widths",[&]{auto b=one(3);u16(b,0x7fff);u32(b,0x80000003);building(b);u16(b,0x7fff);u32(b,5);auto r=walk(b);return r.items.size()==3&&r.items[1].object_id==5&&r.items[1].payload_bytes==182&&r.items[2].object_reference&&r.items[2].payload_bytes==0&&r.cache.size()==6&&r.end==start+6+14+182+6+182+6;});
    check("4001 complete records prove4000 is not a pool count limit",[&]{auto b=one(4001);for(int i=0;i<4000;++i){u16(b,0x8003);building(b);}auto r=walk(b);return r.complete&&r.items.size()==4001&&r.items.back().object_id==4004&&r.cache.size()==4005&&r.end==start+6+14+182+4000*184;});
    check("zero count consumes header only and does not synthesize4000 payloads",[&]{auto b=pool(0);b.push_back(255);auto r=walk(b);return r.complete&&r.items.empty()&&r.cache.size()==3&&r.end==start+6;});
    check("batch checkpoint occurs before missing next tag",[&]{auto b=one(2);PoolLimits l;l.max_items=1;auto r=walk(b,l);return !r.complete&&r.end==b.size()&&r.items.size()==1&&r.count==2&&r.stop_reason=="modern_item_budget_checkpoint";});
    check("4096 processed references checkpoint before slot4096 tag",[&]{auto b=one(4097);for(int i=0;i<4095;++i)u16(b,4);auto r=walk(b);return !r.complete&&r.items.size()==4096&&r.end==b.size()&&r.cache.size()==5;});
    check("unknown declaration stops before opaque payload without cache append",[&]{auto b=pool(8192);declaration(b,"Granary");auto end=b.size();b.insert(b.end(),19,255);auto r=walk(b);return r.pending.present&&r.pending.class_id_if_resolved==3&&r.pending.object_id_if_factory_returns==4&&r.cache.size()==3&&r.end==end&&r.items.empty()&&r.count==8192&&!r.complete;});
    check("signed extremes and modern count8193 reject before tags",[&]{return rejects(pool(0xffffffff),"negative pool count")&&rejects(pool(0x80000000),"negative pool count")&&rejects(pool(0x7fffffff),"pool count exceeds modern budget")&&rejects(pool(8193),"pool count exceeds modern budget");});
    check("earlier Goal class and object cannot be reused as Building",[&]{auto a=pool(1);u16(a,0x8001);auto b=pool(1);u16(b,2);return rejects(a,"earlier goal class")&&rejects(b,"earlier goal object");});
    check("null and maximal extended tags do not wrap into valid IDs",[&]{auto a=pool(1);u16(a,0);auto b=pool(1);u16(b,0x7fff);u32(b,0xffffffff);auto c=pool(1);u16(c,0x7fff);u32(c,0x80000000);return rejects(a,"nonnull consumer")&&rejects(b,"unqualified pool class reference")&&rejects(c,"unqualified pool class reference");});
    check("modern kind guards reject class as object and object as class",[&]{auto a=one(2);u16(a,3);auto b=one(2);u16(b,0x8004);return rejects(a,"object expected")&&rejects(b,"class expected");});
    check("cache budget exact five succeeds and four rejects next object",[&]{auto b=one();PoolLimits l;l.max_cache=5;auto r=walk(b,l);l.max_cache=4;return r.cache.size()==5&&rejects(b,"cache budget exhausted",l);});
    check("every proper prefix of first Building record fails truncated",[&]{auto b=one();const size_t record=start+6+14;for(size_t n=0;n<182;++n){Bytes cut(b.begin(),b.begin()+record+n);if(!rejects(cut,"truncated"))return false;}return walk(b).complete;});
    check("extended tag payload cannot be partially read",[&]{auto b=one(2);u16(b,0x7fff);u32(b,4);for(size_t n=1;n<=4;++n){Bytes cut(b.begin(),b.end()-n);if(!rejects(cut,"truncated"))return false;}auto r=walk(b);return r.items.size()==2&&r.items[1].object_reference;});
    check("modern prefix1048576 accepts and1048577 rejects",[&]{auto b=one();b.resize(1048576,255);auto r=walk(b);b.push_back(255);return r.end==start+202&&rejects(b,"context/budget");});
    check("zero or over hard limits reject without widening scope",[&]{auto b=one();for(int i=0;i<7;++i){PoolLimits l;if(i==0)l.max_items=0;if(i==1)l.max_items=4097;if(i==2)l.max_count=0;if(i==3)l.max_count=8193;if(i==4)l.max_cache=0;if(i==5)l.max_cache=8193;if(i==6)l.max_prefix=1048577;if(!rejects(b,"context/budget",l))return false;}return true;});
    size_t failed=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
