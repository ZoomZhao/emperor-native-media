#include "city19_following_vectors_archive.hpp"
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

Bytes following_prefix(){return complete();}
EmpireGridWalk grid_before(const Bytes&b){return grids(b);}
void fixed(Bytes&b,uint8_t presence=0){u16(b,4);b.insert(b.end(),32,255);b.push_back(presence);}
void empty_vector(Bytes&b){u16(b,1);for(int i=0;i<6;++i)u32(b,0);}
FollowingFixedWalk fixed_walk(const Bytes&b,FollowingFixedLimits l={}){auto prior=grid_before(b);return walk_city19_following_fixed({b.data(),b.size()},prior,l);}
FollowingVectorWalk vector_walk(const Bytes&b,FollowingVectorLimits l={}){auto before=fixed_walk(b);return walk_city19_following_vectors({b.data(),b.size()},before,l);}
Bytes fixed_full(){auto b=following_prefix();for(int i=0;i<12;++i)fixed(b);return b;}
bool fixed_rejects(const Bytes&b,const char*why,FollowingFixedLimits l={}){try{(void)fixed_walk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
bool vector_rejects(const Bytes&b,const char*why){try{(void)vector_walk(b);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>>checks;auto check=[&](const char*n,const std::function<bool()>&f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
check("actual predecessor plus any nonzero presence yields partial35 and no next word",[&]{for(uint8_t v:{1,2,128,255}){auto b=following_prefix();size_t s=b.size();fixed(b,v);u16(b,1);b.insert(b.end(),64,255);auto old=b;auto r=fixed_walk(b);if(r.start!=s||r.end!=s+35||r.complete||!r.pending_extension||r.pending_ordinal!=0||r.records.size()!=1||r.records[0].full_record_qualified||r.next_header_present||r.cache_next!=10||b!=old)return false;}return true;});
check("opaque pointer-looking DWORDs never infer getter type or null",[&]{auto b=following_prefix();size_t s=b.size();fixed(b,255);auto a=fixed_walk(b);for(size_t i=s+2;i<s+34;++i)b[i]=0;auto q=fixed_walk(b);return a.pending_extension&&q.pending_extension&&a.end==q.end&&a.records.size()==1&&q.records.size()==1&&!a.records[0].full_record_qualified&&!q.records[0].full_record_qualified;});
check("nonzero twelfth prefix stays incomplete despite twelve entries",[&]{auto b=following_prefix();size_t s=b.size();for(int i=0;i<11;++i)fixed(b);fixed(b,1);u16(b,1);auto r=fixed_walk(b);return r.records.size()==12&&!r.complete&&r.pending_extension&&r.pending_ordinal==11&&r.end==s+420&&r.records[10].full_record_qualified&&!r.records[11].full_record_qualified&&!r.next_header_present;});
check("all35 first prefix truncations reject including presence byte",[&]{auto b=following_prefix();size_t s=b.size();fixed(b);FollowingFixedLimits l;l.max_records=1;l.inspect_next_header=false;for(size_t i=0;i<35;++i){Bytes cut(b.begin(),b.begin()+s+i);if(!fixed_rejects(cut,"truncated",l))return false;}return fixed_walk(b,l).end==s+35;});
check("zero presence reads12 times35 without56 destination padding",[&]{auto b=fixed_full();size_t s=grid_before(b).end;FollowingFixedLimits l;l.inspect_next_header=false;auto r=fixed_walk(b,l);if(!r.complete||r.records.size()!=12||r.end!=s+420)return false;for(size_t i=0;i<12;++i)if(r.records[i].start!=s+i*35||r.records[i].end!=s+(i+1)*35||!r.records[i].full_record_qualified)return false;return true;});
check("first modern checkpoint precedes unknown next version",[&]{auto b=following_prefix();fixed(b);size_t end=b.size();u16(b,65535);FollowingFixedLimits l;l.max_records=1;auto r=fixed_walk(b,l);return !r.complete&&r.records.size()==1&&r.end==end&&!r.pending_version_present&&!r.next_header_present;});
check("last unsupported fixed version stops after word before fields",[&]{auto b=following_prefix();size_t s=b.size();for(int i=0;i<11;++i)fixed(b);u16(b,3);b.insert(b.end(),100,255);auto r=fixed_walk(b);return !r.complete&&r.pending_version_present&&r.pending_ordinal==11&&r.pending_version==3&&r.records.size()==11&&r.end==s+11*35+2;});
check("fixed complete next header belongs to vectors once only",[&]{auto b=fixed_full();size_t end=b.size();empty_vector(b);FollowingVectorLimits l;l.max_records=1;l.inspect_next_header=false;auto f=fixed_walk(b);auto r=vector_walk(b,l);return f.complete&&f.next_header_start==end&&f.next_header_present&&f.end==end+2&&r.start==end&&r.records.size()==1&&r.records[0].start==end&&r.records[0].end==end+26&&r.end==end+26&&r.cache_next==10;});
check("nonzero fixed prefix cannot enter empty vectors",[&]{auto b=following_prefix();fixed(b,1);for(int i=0;i<27;++i)empty_vector(b);auto f=fixed_walk(b);try{(void)walk_city19_following_vectors({b.data(),b.size()},f);return false;}catch(const std::runtime_error&e){return !f.complete&&std::string(e.what()).find("preceding context")!=std::string::npos;}});
check("all six positive vector groups stop before unknown object tag",[&]{for(size_t g=0;g<6;++g){auto b=fixed_full();u16(b,1);for(size_t i=0;i<g;++i)u32(b,0);u32(b,1);size_t end=b.size();u16(b,65535);b.insert(b.end(),64,255);auto r=vector_walk(b);if(!r.pending_objects||r.complete||!r.pending_present||r.pending_group!=g||r.end!=end||r.pending.counts.size()!=g+1||!r.records.empty()||r.cache_next!=10)return false;}return true;});
check("twenty seventh partial vector is not a complete collection",[&]{auto b=fixed_full();for(int i=0;i<26;++i)empty_vector(b);u16(b,1);u32(b,0);u32(b,1);auto end=b.size();u16(b,65535);auto r=vector_walk(b);return !r.complete&&r.records.size()==26&&r.pending.ordinal==26&&r.pending_objects&&r.pending_group==1&&r.end==end&&!r.next_header_present;});
check("27 zero-count vectors qualify26 each but no typed payload",[&]{auto b=fixed_full();size_t s=b.size();for(int i=0;i<27;++i)empty_vector(b);FollowingVectorLimits l;l.inspect_next_header=false;auto r=vector_walk(b,l);return r.complete&&r.records.size()==27&&r.start==s&&r.end==s+702&&r.cache_next==10&&!r.pending_present&&!r.next_header_present;});
check("missing final next header may be left unread explicitly",[&]{auto b=fixed_full();for(int i=0;i<27;++i)empty_vector(b);FollowingVectorLimits l;l.inspect_next_header=false;auto r=vector_walk(b,l);return r.end==b.size()&&r.complete&&vector_rejects(b,"truncated");});
check("signed and over-budget vector counts reject before tag",[&]{for(uint32_t n:{0xffffffffU,0x80000000U,0x7fffffffU,8193U}){auto b=fixed_full();u16(b,1);u32(b,n);if(!vector_rejects(b,n>0x7fffffff?"negative vector":"exceeds modern budget"))return false;}return true;});
check("exact fixed4915584 prefix accepts one partial record and one extra rejects",[&]{auto b=following_prefix();fixed(b,1);size_t end=b.size();b.resize(4915584,255);auto r=fixed_walk(b);b.push_back(255);return r.end==end&&r.pending_extension&&fixed_rejects(b,"context/modern budget");});
check("incomplete previous grid parent tail blocks following prefix",[&]{auto b=complete(3,false);EmpireGridLimits l;l.stop_before_parent_tail=true;auto before=grids(b,l);fixed(b);try{(void)walk_city19_following_fixed({b.data(),b.size()},before);return false;}catch(const std::runtime_error&e){return !before.parent_byte_requests_complete&&std::string(e.what()).find("preceding context")!=std::string::npos;}});
size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
