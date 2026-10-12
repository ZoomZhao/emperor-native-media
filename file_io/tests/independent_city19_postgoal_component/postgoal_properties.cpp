#include "city19_postgoals_archive.hpp"
#include <iostream>
#include <utility>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void u16(Bytes &b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void u32(Bytes &b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
bool rejects(const Bytes &b,const GoalWalk &g,const std::string &why,bool festival=true){try{(void)walk_city19_postgoals({b.data(),b.size()},g,festival);return false;}catch(const std::runtime_error &e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){
    std::vector<std::pair<std::string,bool>> checks;auto check=[&](std::string n,bool ok){checks.push_back({n,ok});};
    // Pure component context; the real CLI qualifies the preceding city bytes itself.
    Bytes b(3355);for(int i=0;i<10;++i){u16(b,1);u32(b,0);}const size_t start=b.size();
    u16(b,1);u32(b,0xffffffff);b.push_back(255);u32(b,0x80000000);u32(b,11);u32(b,12);
    const size_t array_start=b.size();for(size_t i=0;i<0xcb10;++i)b.push_back(uint8_t(i*13+7));const size_t flood_end=b.size();
    u16(b,4);u16(b,1);for(int i=0;i<4;++i)u32(b,101+i);for(int i=0;i<8;++i)u32(b,201+i);
    u32(b,301);u32(b,302);b.push_back(31);b.push_back(32);u32(b,303);b.push_back(33);for(int i=0;i<4;++i)u32(b,401+i);
    const size_t final_end=b.size();b.insert(b.end(),64,255);const Bytes before=b;
    const auto g=walk_city19_housing_goals({b.data(),16384});const auto cache_before=g.cache.size();
    const auto f=walk_city19_postgoals({b.data(),b.size()},g,false);const auto all=walk_city19_postgoals({b.data(),b.size()},g,true);
    check("variable odd goal endpoint controls postgoal start",start==3415&&f.start==start&&f.end==start+52003&&all.end==start+52086);
    check("Flood fixed array remains51984 with independent scalar and byte fields",f.sections.size()==1&&f.sections[0].fields.size()==6&&f.sections[0].fields[1].bytes==1&&f.sections[0].fields.back().offset==array_start&&f.sections[0].fields.back().bytes==51984&&flood_end-array_start==51984);
    check("Festival nested versions and four scalar fields precede eight repeated fields",all.sections.size()==2&&all.sections[1].embedded_version_offset==flood_end+2&&all.sections[1].fields.size()==22&&all.sections[1].fields[0].offset==flood_end+4&&all.sections[1].fields[4].offset==flood_end+20&&all.sections[1].fields[11].receiver_offset==0x44);
    check("Festival sparse byte and final receiver order preserved",all.sections.size()==2&&all.sections[1].fields.size()==22&&all.sections[1].fields[14].bytes==1&&all.sections[1].fields[14].receiver_offset==0x20&&all.sections[1].fields[15].bytes==1&&all.sections[1].fields[17].bytes==1&&all.sections[1].fields[17].receiver_offset==0x50&&all.sections[1].fields[20].receiver_offset==0x48&&all.sections[1].fields[21].receiver_offset==0x4c&&final_end-flood_end==83);
    check("Religion poison tail untouched and goal metadata stays immutable",all.end==final_end&&b.size()-all.end==64&&b==before&&g.cache.size()==cache_before&&g.completed_members==10);
    Bytes wrong=b;wrong[flood_end]=1;wrong[flood_end+1]=0;
    check("Flood-only never peeks unsupported Festival version",walk_city19_postgoals({wrong.data(),wrong.size()},g,false).end==flood_end&&rejects(wrong,g,"Festival section version",true));
    Bytes shortarray=b;shortarray.resize(flood_end-1);
    check("fixed array last byte is mandatory",rejects(shortarray,g,"truncated",false));
    Bytes shortfestival=b;shortfestival.resize(final_end-1);
    check("final Festival DWORD last byte is mandatory",rejects(shortfestival,g,"truncated",true));
    Bytes large=b;large.resize(65536,255);
    check("separate postgoal prefix exact65536 accepts known sections",walk_city19_postgoals({large.data(),large.size()},g,true).end==final_end);
    large.push_back(255);check("postgoal prefix65537 refuses before interpretation",rejects(large,g,"prefix/baseline",true));
    bool old_limit=false;try{(void)walk_city19_housing_goals({b.data(),b.size()});}catch(const std::runtime_error &e){old_limit=std::string(e.what()).find("budget/argument")!=std::string::npos;}
    check("old Goal component still rejects views above16384",old_limit);
    GoalWalk incomplete=g;incomplete.completed_members=9;GoalWalk pending=g;pending.pending.present=true;
    check("incomplete or pending goal baseline refuses new section",rejects(b,incomplete,"prefix/baseline",false)&&rejects(b,pending,"prefix/baseline",false));
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
