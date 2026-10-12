#include "city19_figure_variants_archive.hpp"
#include <iostream>
#include <functional>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void word(Bytes&b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void dword(Bytes&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
void decl(Bytes&b,const char*name,uint16_t schema=0){word(b,65535);word(b,schema);word(b,uint16_t(std::strlen(name)));b.insert(b.end(),name,name+std::strlen(name));}
void figure(Bytes&b,uint8_t presence=0,uint8_t opaque=255){word(b,7);b.insert(b.end(),395,opaque);b.push_back(presence);}
void pheasant(Bytes&b,uint16_t own=2,uint8_t presence=0){word(b,own);figure(b,presence);}
PoolWalk prior(){PoolWalk p;p.complete=true;p.end=5;p.count=1;p.items.resize(1);p.cache={{GoalCacheKind::Null,0,0,PoolDomain::EarlierGoal},{GoalCacheKind::Class,0,0,PoolDomain::Building},{GoalCacheKind::Object,1,0,PoolDomain::Building}};return p;}
Bytes list(uint32_t n){Bytes b(5,127);word(b,1);dword(b,n);return b;}
Bytes one(uint32_t n=1,uint16_t own=2,uint8_t presence=0){auto b=list(n);decl(b,"cPheasantFig");pheasant(b,own,presence);return b;}
AfterPoolWalk walk(const Bytes&b,bool enabled=true,AfterPoolLimits l={}){return walk_city19_figure_variants({b.data(),b.size()},prior(),l,enabled);}
bool rejects(const Bytes&b,const char*why,AfterPoolLimits l={}){try{(void)walk(b,true,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){
    std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("Pheasant own2 precedes Figure7,400 wire bytes",[]{auto b=one();auto r=walk(b);return r.figures_complete&&r.figures.size()==1&&r.figures[0].payload_bytes==400&&r.end==5+6+18+400&&r.cache_start==3&&r.cache.size()==5&&r.figures[0].class_id==3&&r.figures[0].object_id==4;});
    check("disabled scope stops before own version without cache append",[]{auto b=one();auto r=walk(b,false);return r.pending.present&&r.end==29&&r.cache.size()==3&&r.figures.empty();});
    check("default API remains Figure-only",[]{auto b=one();auto p=prior();auto r=walk_city19_figure_variants({b.data(),b.size()},p);return r.pending.present&&r.end==29&&r.cache.size()==3;});
    check("old Figure-only subtree identical with feature disabled",[]{auto b=list(1);decl(b,"Figure");figure(b);auto p=prior();auto old=walk_city19_after_mainpool({b.data(),b.size()},p);auto r=walk(b,false);return r.end==old.end&&r.cache.size()==old.cache.size()&&r.figures.size()==1&&r.figures[0].payload_bytes==398&&r.figures[0].payload_offset==old.figures[0].payload_offset;});
    check("mixed Figure then Pheasant retains distinct class layouts",[]{auto b=list(2);decl(b,"Figure");figure(b);decl(b,"cPheasantFig");pheasant(b);auto r=walk(b);return r.figures.size()==2&&r.figures[0].payload_bytes==398&&r.figures[1].payload_bytes==400&&r.figures[1].class_id==5&&r.cache.size()==7;});
    check("Pheasant class reference keeps prefix2",[]{auto b=one(2);word(b,0x8003);pheasant(b);auto r=walk(b);return r.figures.size()==2&&r.figures[1].payload_bytes==400&&r.figures[1].class_id==3&&r.figures[1].object_id==5&&r.cache.size()==6;});
    check("Pheasant object reference consumes only tag",[]{auto b=one(2);word(b,4);b.insert(b.end(),19,255);auto r=walk(b);return r.figures.size()==2&&r.figures[1].object_reference&&r.figures[1].payload_bytes==0&&r.cache.size()==5&&b.size()-r.end==19;});
    check("extended class and object tags preserve layout and IDs",[]{auto b=one(3);word(b,0x7fff);dword(b,0x80000003);pheasant(b);word(b,0x7fff);dword(b,5);auto r=walk(b);return r.figures.size()==3&&r.figures[1].payload_bytes==400&&r.figures[2].object_reference&&r.figures[2].payload_bytes==0&&r.cache.size()==6;});
    check("duplicate declarations receive separate class IDs",[]{auto b=one(2);decl(b,"cPheasantFig");pheasant(b);auto r=walk(b);return r.figures.size()==2&&r.figures[1].class_id==5&&r.figures[1].object_id==6&&r.cache.size()==7;});
    check("parent-first serialization order rejects",[]{auto b=list(1);decl(b,"cPheasantFig");figure(b);word(b,2);return rejects(b,"cPheasantFig own section");});
    check("wrong concrete layout cannot cross cached class",[]{auto a=one(2);word(a,0x8003);figure(a);auto b=list(2);decl(b,"Figure");figure(b);word(b,0x8003);pheasant(b);return rejects(a,"cPheasantFig own section")&&rejects(b,"Figure field section");});
    check("own version source2 subset excludes0,1,3,ffff",[]{for(uint16_t v:{0,1,3,65535})if(!rejects(one(1,v),"cPheasantFig own section"))return false;return true;});
    check("schema0 distinct from own2 and parent7",[]{auto b=list(1);decl(b,"cPheasantFig",1);pheasant(b);return rejects(b,"schema mismatch");});
    check("all400 proper payload prefixes reject truncated",[]{auto b=one();for(size_t n=0;n<400;++n){Bytes cut(b.begin(),b.begin()+29+n);if(!rejects(cut,"truncated"))return false;}return walk(b).figures_complete;});
    check("nonzero optional presence stops before unknown extension",[]{for(uint8_t v:{1,2,255}){auto b=one(1,2,v);b.insert(b.end(),21,255);auto r=walk(b);if(!r.extension.present||r.end!=429||r.figures.size()!=0||r.cache[4].payload_qualified||r.cache.size()!=5)return false;}return true;});
    check("opaque activity bits do not filter records",[]{auto a=one();auto b=a;b[33]=0;auto r=walk(a);auto q=walk(b);return r.figures.size()==1&&q.figures.size()==1&&r.end==q.end;});
    check("unknown sibling stops at declaration",[]{auto b=list(1);decl(b,"Other");size_t end=b.size();b.insert(b.end(),20,255);auto r=walk(b);return r.pending.present&&r.end==end&&r.cache.size()==3&&r.figures.empty();});
    check("checkpoint before next missing tag",[]{auto b=one(2);AfterPoolLimits l;l.max_items=1;auto r=walk(b,true,l);return !r.figures_complete&&r.figures.size()==1&&r.end==b.size();});
    check("cache exact5 accepts while4 rejects object append",[]{auto b=one();AfterPoolLimits l;l.max_cache=5;bool exact=walk(b,true,l).cache.size()==5;l.max_cache=4;return exact&&rejects(b,"cache budget exhausted",l);});
    check("Building class and object remain incompatible",[]{auto a=list(1);word(a,0x8001);auto b=list(1);word(b,2);return rejects(a,"earlier Goal/Building class")&&rejects(b,"incompatible or incomplete");});
    check("modern kind gate rejects class-as-object/object-as-class",[]{auto a=one(2);word(a,3);auto b=one(2);word(b,0x8004);return rejects(a,"object expected")&&rejects(b,"class expected");});
    check("zero and signed count extremes preserve modern policy",[]{auto zero=list(0);auto r=walk(zero);return r.figures_complete&&r.end==11&&r.cache.size()==3&&rejects(list(0xffffffff),"negative Figure count")&&rejects(list(16385),"Figure count exceeds modern budget");});
    check("2000 synthetic Pheasant records complete without runtime",[]{auto b=one(2000);for(int i=0;i<1999;++i){word(b,0x8003);pheasant(b);}auto r=walk(b);return r.figures_complete&&r.figures.size()==2000&&r.figures.back().object_id==2003&&r.cache.size()==2004&&r.end==b.size();});
    size_t failures=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failures;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-failures<<",\"failed\":"<<failures<<"}\n";return failures?1:0;
}
