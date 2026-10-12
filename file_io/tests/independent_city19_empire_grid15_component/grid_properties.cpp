#include "city19_empire_grids_archive.hpp"
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

Bytes grid_prefix(uint16_t parent=3){auto b=ready(parent,2);empire(b,255);empire(b,0);return b;}
EmpireInlineWalk inline_before(const Bytes&b){return walk(b);}
void grid(Bytes&b){u16(b,15);b.insert(b.end(),1192,255);for(int i=0;i<22;++i){u16(b,5);b.insert(b.end(),627,uint8_t(i));}b.insert(b.end(),334,255);}
Bytes complete(uint16_t parent=3,bool tail=true){auto b=grid_prefix(parent);if(parent>=2)for(int i=0;i<22;++i)grid(b);if(parent==3&&tail)u32(b,0xffffffff);return b;}
EmpireGridWalk grids(const Bytes&b,EmpireGridLimits l={}){auto prior=inline_before(b);return walk_city19_empire_grids({b.data(),b.size()},prior,l);}
bool grid_rejects(const Bytes&b,const char*why,EmpireGridLimits l={}){try{(void)grids(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,const std::function<bool()>&f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
check("complete true predecessor plus22 grids preserves cache and input",[&]{auto b=complete();auto old=b;auto prior=inline_before(b);auto r=grids(b);return prior.cache_next==10&&r.cache_next==10&&r.start==prior.end&&r.end==prior.end+338056&&r.grids.size()==22&&r.grids_complete&&r.parent_tail_qualified&&r.parent_byte_requests_complete&&b==old;});
check("all484 child offsets have629 wire width and exact nesting",[&]{auto b=complete();auto r=grids(b);if(r.grids.size()!=22)return false;for(size_t g=0;g<22;++g){const auto&q=r.grids[g];if(q.ordinal!=g||q.start!=r.start+g*15366||q.end-q.start!=15366||q.children.size()!=22)return false;for(size_t i=0;i<22;++i){const auto&c=q.children[i];if(c.ordinal!=i||c.version!=5||c.start!=q.start+1194+i*629||c.end-c.start!=629)return false;}}return true;});
check("15360 and628 destination strides cannot set wire boundaries",[&]{auto b=grid_prefix();const size_t start=b.size();grid(b);grid(b);EmpireGridLimits l;l.max_grids=2;auto r=grids(b,l);return r.grids.size()==2&&r.grids[1].start==start+15366&&r.grids[0].children.size()==22&&r.grids[0].children[1].start-r.grids[0].children[0].start==629;});
check("unknown last outer version stops after its word with21 complete",[&]{auto b=complete();auto start=inline_before(b).end;size_t target=start+21*15366;b[target]=14;b[target+1]=0;auto r=grids(b);return r.grids.size()==21&&r.pending.present&&!r.pending.child&&r.pending.grid_ordinal==21&&r.pending.version==14&&r.end==target+2&&!r.grids_complete&&!r.parent_tail_qualified;});
check("unknown last nested version stops before627 bytes and parent tail",[&]{auto b=complete();auto start=inline_before(b).end;size_t target=start+21*15366+1194+21*629;b[target]=255;b[target+1]=255;auto r=grids(b);return r.grids.size()==21&&r.pending.present&&r.pending.child&&r.pending.grid_ordinal==21&&r.pending.child_ordinal==21&&r.pending.version==65535&&r.pending.start==target&&r.end==target+2&&!r.parent_byte_requests_complete;});
check("second header checkpoint does not even peek unknown version",[&]{auto b=grid_prefix();grid(b);size_t end=b.size();u16(b,65535);EmpireGridLimits l;l.max_grids=1;auto r=grids(b,l);return r.grids.size()==1&&r.end==end&&!r.pending.present&&!r.grids_complete;});
check("known and unknown first header-only stops before all raw requests",[&]{for(uint16_t v:{15,14,65535}){auto b=grid_prefix();auto end=b.size();u16(b,v);EmpireGridLimits l;l.first_header_only=true;auto r=grids(b,l);if(!r.pending.present||r.pending.child||r.pending.version!=v||r.end!=end+2||!r.grids.empty())return false;}return true;});
check("source1 consumes no grid despite unknown future bytes",[&]{auto b=complete(1);size_t end=b.size();u16(b,65535);auto r=grids(b);return r.end==end&&r.grids.empty()&&r.grids_complete&&r.parent_byte_requests_complete&&!r.parent_tail_qualified;});
check("source2 never consumes poison parent3 tail",[&]{auto b=complete(2);size_t end=b.size();u32(b,0xffffffff);auto r=grids(b);return r.end==end&&r.grids_complete&&r.parent_byte_requests_complete&&!r.parent_tail_qualified;});
check("source3 missing parent tail can checkpoint but cannot complete",[&]{auto b=complete(3,false);EmpireGridLimits l;l.stop_before_parent_tail=true;auto r=grids(b,l);return r.grids_complete&&!r.parent_tail_qualified&&!r.parent_byte_requests_complete&&r.end==b.size()&&grid_rejects(b,"truncated");});
check("all four proper final parent tail prefixes reject",[&]{auto b=complete();for(size_t n=0;n<4;++n){Bytes cut(b.begin(),b.end()-4+n);if(!grid_rejects(cut,"truncated"))return false;}return true;});
check("selected bulk shared-tail and last-child cutpoints reject",[&]{auto b=grid_prefix();auto start=b.size();grid(b);EmpireGridLimits l;l.max_grids=1;for(size_t n:{size_t(1),size_t(33),size_t(1193),size_t(1194),size_t(1195),size_t(1194+629-1),size_t(1194+21*629+625),size_t(1194+22*629-1),size_t(15365)}){Bytes cut(b.begin(),b.begin()+start+n);if(!grid_rejects(cut,"truncated",l))return false;}return grids(b,l).grids.size()==1;});
check("complete source3 leaves next consumer word and payload untouched",[&]{auto b=complete();auto end=b.size();u16(b,65535);u32(b,0xffffffff);b.insert(b.end(),123,255);auto old=b;auto r=grids(b);return r.end==end&&b.size()-r.end==129&&r.parent_byte_requests_complete&&r.parent_tail_offset==end-4&&b==old;});
check("exact4915584 prefix accepts unknown tail one extra rejects",[&]{auto b=complete();auto end=b.size();b.resize(4915584,255);auto r=grids(b);b.push_back(255);return r.end==end&&grid_rejects(b,"context/modern budget");});
check("incomplete inline checkpoint is rejected before grid header",[&]{auto b=ready(3,2);empire(b);EmpireInlineLimits l;l.max_items=1;auto before=walk(b,l);u16(b,15);try{(void)walk_city19_empire_grids({b.data(),b.size()},before);return false;}catch(const std::runtime_error&e){return !before.inline_complete&&std::string(e.what()).find("preceding context")!=std::string::npos;}});
size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
