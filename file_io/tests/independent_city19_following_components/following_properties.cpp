#include "city19_goodscontrol_archive.hpp"
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
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,const std::function<bool()>&f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
const size_t start=56037;
check("qualified whole prefix keeps mixed concrete layouts and shared IDs",[&]{auto b=mixed();auto before=b;auto p=pool_before(b);auto f=figures(b);return p.end==start&&p.cache.size()==5&&f.figures_complete&&f.figures.size()==4&&f.cache_start==5&&f.cache.size()==10&&f.figures[0].class_id==5&&f.figures[0].object_id==6&&f.figures[1].class_id==7&&f.figures[1].object_id==8&&f.figures[2].object_id==9&&f.figures[2].payload_bytes==400&&f.figures[3].object_reference&&f.figures[3].object_id==8&&f.figures[3].payload_bytes==0&&b==before;});
check("Goods inline records preserve cache after mixed class and object refs",[&]{auto b=mixed();size_t s=b.size();u16(b,1);u32(b,3);for(int i=0;i<3;++i)goods(b);auto r=walkgoods(b);return r.complete&&r.records.size()==3&&r.cache_next==10&&r.start==s&&r.end==s+6+3*270;});
check("word enum high bytes remain opaque before zero presence",[&]{auto b=flist(1);declaration(b,"Figure");figure(b);auto f=figures(b);return f.figures_complete&&!f.extension.present&&f.figures.size()==1&&f.figures[0].payload_bytes==398&&f.end==start+6+12+398;});
check("nonzero presence for both classes stops without crossing Goods",[&]{for(bool bird:{false,true})for(uint8_t v:{1,2,128,255}){auto b=flist(1);declaration(b,bird?"cPheasantFig":"Figure");if(bird)pheasant(b,v);else figure(b,v);size_t end=b.size();u16(b,1);u32(b,1);goods(b);auto f=figures(b);if(!f.extension.present||f.figures_complete||!f.figures.empty()||f.end!=end||f.cache.size()!=7||f.cache[6].payload_qualified||!goods_rejects(b,"preceding context"))return false;}return true;});
check("second incomplete object does not enter completed sequence",[&]{auto b=flist(2);declaration(b,"Figure");figure(b);declaration(b,"cPheasantFig");pheasant(b,255);auto f=figures(b);return f.extension.present&&f.figures.size()==1&&f.cache.size()==9&&!f.cache[8].payload_qualified&&f.extension.ordinal==1&&!f.figures_complete;});
check("disabled concrete scope leaves own2 and following Goods unread",[&]{auto b=mixed();u16(b,1);u32(b,0);auto f=figures(b,false);return f.pending.present&&f.pending.ordinal==1&&f.figures.size()==1&&f.end==start+6+12+398+18&&f.cache.size()==7;});
check("reversed Pheasant parent order rejects before Goods",[&]{auto b=flist(1);declaration(b,"cPheasantFig");figure(b);u16(b,2);return fig_rejects(b,"cPheasantFig own section");});
check("concrete class reference refuses ordinary Figure body",[&]{auto b=flist(2);declaration(b,"cPheasantFig");pheasant(b);u16(b,0x8005);figure(b);return fig_rejects(b,"cPheasantFig own section");});
check("unknown Figure sibling blocks Goods without cache append",[&]{auto b=flist(1);declaration(b,"OtherFig");auto end=b.size();b.insert(b.end(),15,255);auto f=figures(b);return f.pending.present&&f.end==end&&f.cache.size()==5&&goods_rejects(b,"preceding context");});
check("all398 ordinary and400 concrete payload cuts reject",[&]{for(bool bird:{false,true}){auto b=flist(1);declaration(b,bird?"cPheasantFig":"Figure");size_t s=b.size();if(bird)pheasant(b);else figure(b);size_t n=bird?400:398;for(size_t i=0;i<n;++i){Bytes cut(b.begin(),b.begin()+s+i);if(!fig_rejects(cut,"truncated"))return false;}}return true;});
check("Goods retains next Empire-looking word count and poison",[&]{auto b=mixed();u16(b,1);u32(b,1);goods(b);auto end=b.size();u16(b,3);u32(b,200);b.insert(b.end(),34,255);auto before=b;auto r=walkgoods(b);return r.complete&&r.end==end&&b.size()-r.end==40&&r.stop_reason=="GoodsControl_complete_before_unqualified_Empire"&&b==before;});
check("Goods record tags do not consume or append archive IDs",[&]{auto b=mixed();u16(b,1);u32(b,2);goods(b);goods(b);auto r=walkgoods(b);return r.complete&&r.cache_next==10&&r.records.size()==2&&r.records[1].start-r.records[0].start==270&&r.records[1].ordinal==1;});
check("201 Goods records cross constructor default200 without padding",[&]{auto b=flist(0);u16(b,1);u32(b,201);for(int i=0;i<201;++i)goods(b);auto r=walkgoods(b);return r.complete&&r.records.size()==201&&r.end==start+6+6+201*270&&r.cache_next==5;});
check("Figure2001 is a finite record count not activity upper bound",[&]{auto b=flist(2001);declaration(b,"Figure");figure(b);for(int i=0;i<2000;++i){u16(b,0x8005);figure(b);}auto f=figures(b);return f.figures_complete&&f.figures.size()==2001&&f.cache.size()==2007&&f.end==b.size();});
check("all270 Goods payload cuts reject with real complete predecessor",[&]{auto b=mixed();u16(b,1);u32(b,1);size_t s=b.size();goods(b);for(size_t i=0;i<270;++i){Bytes cut(b.begin(),b.begin()+s+i);if(!goods_rejects(cut,"truncated"))return false;}return walkgoods(b).complete;});
check("Goods checkpoint occurs before missing next word",[&]{auto b=mixed();u16(b,1);u32(b,2);goods(b);GoodsLimits l;l.max_items=1;auto r=walkgoods(b,l);return !r.complete&&r.records.size()==1&&r.count==2&&r.end==b.size()&&r.cache_next==10;});
check("Goods count signed extremes reject before record allocation",[&]{for(uint32_t n:{0xffffffffU,0x80000000U,0x7fffffffU,8193U}){auto b=mixed();u16(b,1);u32(b,n);if(!goods_rejects(b,n>0x7fffffff?"negative GoodsControl":"count exceeds modern budget"))return false;}return true;});
check("Figure cache exact complete boundary rejects one missing slot",[&]{auto b=mixed();AfterPoolLimits l;l.max_cache=10;auto f=figures(b,true,l);l.max_cache=9;bool bad=false;try{(void)figures(b,true,l);}catch(const std::runtime_error&e){bad=std::string(e.what()).find("cache budget exhausted")!=std::string::npos;}return f.cache.size()==10&&bad;});
size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
