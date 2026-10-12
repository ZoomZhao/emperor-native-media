#include "city19_goal_archive.hpp"
#include <iostream>
#include <utility>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void u16(Bytes &b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void u32(Bytes &b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
void header(Bytes &b,uint32_t n){u16(b,1);u32(b,n);}
void declaration(Bytes &b,const std::string &name,uint16_t schema=0){u16(b,65535);u16(b,schema);u16(b,uint16_t(name.size()));b.insert(b.end(),name.begin(),name.end());}
void payload(Bytes &b){u16(b,1);u32(b,0xffffffff);u16(b,1);u32(b,0x80000000);}
GoalWalk walk(const Bytes &b,GoalLimits l={}){return walk_city19_housing_goals({b.data(),b.size()},l);}
int main(){
    std::vector<std::pair<std::string,bool>> checks;auto check=[&](std::string n,bool good){checks.push_back({n,good});};
    // The core begins at3355; no original city context is copied here.
    Bytes b(3355);header(b,2);declaration(b,"cTradingPartnersGoal");payload(b);declaration(b,"cYearlyProductionGoal");b.insert(b.end(),32,255);
    const auto original=b;const auto disabled=walk(b);
    GoalLimits homage;homage.enable_homage=true;const auto old_homage=walk(b,homage);
    check("Trading remains pending with default or Homage-only flags",disabled.end==3387&&old_homage.end==3387&&disabled.cache.size()==1&&old_homage.cache.size()==1);
    GoalLimits trading;trading.enable_trading=true;const auto parsed=walk(b,trading);
    check("Trading12 does not read opaque DWORDs as signed nested counts",parsed.items.size()==1&&parsed.items[0].payload_bytes==12&&parsed.items[0].payload_offset==3387&&parsed.cache.size()==3);
    check("Yearly same-member declaration consumes27 only and leaves32 poison bytes",parsed.end==3426&&parsed.following_member==0&&parsed.following_ordinal==1&&parsed.completed_members==0&&parsed.pending.name_bytes==21&&parsed.pending.class_id_if_resolved==3&&parsed.pending.object_id_if_factory_returns==4&&b.size()-parsed.end==32);
    GoalLimits both=trading;both.enable_homage=true;const auto joint=walk(b,both);
    check("Trading-only and both flags agree without changing later old modes",joint.end==parsed.end&&joint.cache.size()==parsed.cache.size()&&walk(b).end==disabled.end&&walk(b,homage).end==old_homage.end&&b==original);
    Bytes r(3355);header(r,3);declaration(r,"cTradingPartnersGoal");payload(r);u16(r,32769);payload(r);u16(r,2);for(int i=0;i<9;++i)header(r,0);
    const auto refs=walk(r,trading);
    check("Trading class1 reference creates object3 and consumes12",refs.items.size()==3&&refs.cache.size()==4&&refs.items[1].class_id==1&&refs.items[1].object_id==3&&refs.items[1].payload_bytes==12&&refs.cache[3].class_type==GoalClassType::Trading);
    check("Trading object2 reference consumes0 and keeps cache next4",refs.items.size()==3&&refs.items[2].object_reference&&refs.items[2].object_id==2&&refs.items[2].payload_bytes==0&&refs.cache.size()==4&&refs.completed_members==10);
    Bytes m(3355);header(m,2);declaration(m,"cTradingPartnersGoal");payload(m);u16(m,2);m.insert(m.end(),32,255);
    GoalLimits cp=trading;cp.checkpoint_items=2;const auto shortcut=walk(m,cp);
    check("object reference checkpoint leaves payload-looking tail untouched",shortcut.end==3401&&m.size()-shortcut.end==32&&shortcut.following_member==1&&shortcut.following_ordinal==0&&shortcut.cache.size()==3);
    Bytes bad(3355);header(bad,1);declaration(bad,"cTradingPartnersGoal",1);payload(bad);bool rejected=false;
    try{(void)walk(bad,trading);}catch(const std::runtime_error &e){rejected=std::string(e.what()).find("Trading schema mismatch")!=std::string::npos;}
    check("known Trading schema rejects while disabled declaration remains extractable",rejected&&walk(bad).pending.schema==1&&walk(bad).cache.size()==1&&walk(b,trading).cache.size()==3);
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
