#include "city19_goal_archive.hpp"
#include <iostream>
#include <utility>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void u16(Bytes &b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void u32(Bytes &b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
void header(Bytes &b,uint32_t n){u16(b,1);u32(b,n);}
void decl(Bytes &b,const std::string &n){u16(b,65535);u16(b,0);u16(b,uint16_t(n.size()));b.insert(b.end(),n.begin(),n.end());}
void base(Bytes &b){u16(b,1);u32(b,0xffffffff);}
void population(Bytes &b,uint16_t own=1){base(b);u16(b,own);u32(b,0x80000000);}
void yearly(Bytes &b){base(b);u16(b,1);u32(b,101);u32(b,102);u32(b,103);}
GoalWalk walk(const Bytes &b,GoalLimits l={}){return walk_city19_housing_goals({b.data(),b.size()},l);}
bool reject(const Bytes &b,GoalLimits l,const std::string &why){try{(void)walk(b,l);return false;}catch(const std::runtime_error &e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){
    std::vector<std::pair<std::string,bool>> checks;auto check=[&](std::string n,bool good){checks.push_back({n,good});};
    // Pure component test context; the CLI independently qualifies the earlier3355 bytes.
    Bytes b(3355);header(b,3);decl(b,"cPopulationGoal");population(b);decl(b,"cYearlyProductionGoal");yearly(b);u16(b,32771);yearly(b);
    const size_t items_end=b.size();for(int i=0;i<9;++i)header(b,0);const size_t ten_end=b.size();
    // A valid-looking eleventh member and payload must remain outside this owner.
    header(b,1);decl(b,"cPopulationGoal");population(b);const auto original=b;
    GoalLimits old;old.enable_homage=true;old.enable_trading=true;old.enable_yearly=true;
    const auto disabled=walk(b,old);
    check("old modes stop at Population declaration and create no cache class",disabled.end==3382&&disabled.known_items==0&&disabled.cache.size()==1&&disabled.pending.name_bytes==15);
    GoalLimits all=old;all.enable_population=true;const auto full=walk(b,all);
    check("three known items plus nine empty members complete exactly10",full.end==ten_end&&full.known_items==3&&full.completed_members==10&&full.members.size()==10&&full.following_member==10&&full.following_ordinal==0);
    check("valid-looking eleventh member remains unread",full.stop_reason=="ten_members_complete_next_owner_unqualified"&&full.pending.present==false&&b.size()-full.end==39&&b==original);
    check("Population12 and following Yearly20 keep distinct cached types",full.items.size()==3&&full.cache.size()==6&&full.items[0].payload_bytes==12&&full.cache[1].class_type==GoalClassType::Population&&full.cache[3].class_type==GoalClassType::Yearly&&full.items[2].class_id==3&&full.items[2].object_id==5&&full.items[2].payload_bytes==20);
    GoalLimits cp=all;cp.checkpoint_items=3;const auto stopped=walk(b,cp);
    check("item checkpoint does not silently consume empty-member headers",stopped.end==items_end&&stopped.completed_members==1&&stopped.following_member==1&&stopped.members.size()==1&&ten_end-stopped.end==54);
    GoalLimits exact=all;exact.max_items=3;
    check("exact item budget still permits zero-item member headers",walk(b,exact).end==ten_end&&walk(b,exact).completed_members==10);
    Bytes malformed=original;malformed.resize(ten_end);malformed.insert(malformed.end(),64,255);
    check("malformed next-consumer bytes cannot change completed-owner result",walk(malformed,all).end==ten_end&&walk(malformed,all).cache.size()==full.cache.size());
    Bytes truncated=original;truncated.resize(ten_end-1);
    check("partial tenth-member header fails instead of claiming completion",reject(truncated,all,"truncated"));
    Bytes bad(3355);header(bad,1);decl(bad,"cPopulationGoal");population(bad,2);
    check("shared one-DWORD helper preserves Population-specific version error",reject(bad,all,"Population goal version outside modern source1 subset")&&walk(b,old).end==3382);
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
