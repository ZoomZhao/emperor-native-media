#include "city19_goal_archive.hpp"
#include <iostream>
#include <utility>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void u16(Bytes &b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void u32(Bytes &b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
void header(Bytes &b,uint32_t n){u16(b,1);u32(b,n);}
void decl(Bytes &b,const std::string &n){u16(b,65535);u16(b,0);u16(b,uint16_t(n.size()));b.insert(b.end(),n.begin(),n.end());}
void payload(Bytes &b,uint16_t own=1){u16(b,1);u32(b,0xffffffff);u16(b,own);u32(b,0x80000000);u32(b,0x11121314);u32(b,0x21222324);}
GoalWalk walk(const Bytes &b,GoalLimits l={}){return walk_city19_housing_goals({b.data(),b.size()},l);}
int main(){
    std::vector<std::pair<std::string,bool>> checks;auto check=[&](std::string n,bool good){checks.push_back({n,good});};
    // Pure component fixtures start at3355; earlier city context is qualified by the CLI.
    Bytes b(3355);header(b,2);decl(b,"cYearlyProductionGoal");payload(b);decl(b,"cPopulationGoal");payload(b);b.insert(b.end(),32,255);
    GoalLimits old;old.enable_homage=true;old.enable_trading=true;
    const auto pending=walk(b,old);
    check("old enabled modes still stop before Yearly20",pending.end==3388&&pending.known_items==0&&pending.cache.size()==1&&pending.pending.name_bytes==21);
    GoalLimits yearly;yearly.enable_yearly=true;const auto p=walk(b,yearly);
    check("Yearly20 keeps opaque DWORDs outside member-count logic",p.known_items==1&&p.items[0].payload_bytes==20&&p.items[0].payload_offset==3388&&p.cache.size()==3);
    check("Population-looking payload is never borrowed from shared20 pattern",p.end==3429&&p.following_member==0&&p.following_ordinal==1&&p.pending.name_bytes==15&&p.pending.class_id_if_resolved==3&&p.pending.object_id_if_factory_returns==4&&b.size()-p.end==52);
    Bytes mixed(3355);header(mixed,4);decl(mixed,"cHousingGoal");payload(mixed);decl(mixed,"cYearlyProductionGoal");payload(mixed);u16(mixed,32771);payload(mixed);u16(mixed,2);mixed.insert(mixed.end(),32,255);
    GoalLimits cp=yearly;cp.checkpoint_items=4;const auto q=walk(mixed,cp);
    check("shared byte pattern preserves Housing1 and Yearly3 class types",q.items.size()==4&&q.cache.size()==6&&q.cache[1].class_type==GoalClassType::Housing&&q.cache[3].class_type==GoalClassType::Yearly&&q.items[1].class_id==3);
    check("Yearly class3 creates object5 while Housing object2 reuses class1",q.items.size()==4&&q.cache.size()==6&&q.items[2].object_id==5&&q.items[2].class_id==3&&q.items[2].payload_bytes==20&&q.cache[5].class_type==GoalClassType::Yearly&&q.items[3].object_id==2&&q.items[3].class_id==1&&q.items[3].payload_bytes==0&&mixed.size()-q.end==32);
    Bytes bad(3355);header(bad,1);decl(bad,"cYearlyProductionGoal");payload(bad,2);bool right_error=false;
    try{(void)walk(bad,yearly);}catch(const std::runtime_error &e){right_error=std::string(e.what()).find("Yearly production goal version outside modern source1 subset")!=std::string::npos;}
    check("shared helper retains Yearly-specific version refusal",right_error&&walk(b,old).end==3388);
    GoalLimits one=yearly;one.max_items=1;bool capped=false;
    try{(void)walk(b,one);}catch(const std::runtime_error &e){capped=std::string(e.what()).find("item budget exhausted")!=std::string::npos;}
    check("tight item budget refuses next unknown item instead of bypassing limit",capped);
    Bytes shortfield(3355);header(shortfield,1);decl(shortfield,"cYearlyProductionGoal");payload(shortfield);shortfield.pop_back();const auto before=shortfield;bool truncated=false;
    try{(void)walk(shortfield,yearly);}catch(const std::runtime_error &e){truncated=std::string(e.what()).find("truncated")!=std::string::npos;}
    check("last field truncation leaves source unchanged and later call starts fresh cache",truncated&&shortfield==before&&walk(b,yearly).cache.size()==3);
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
