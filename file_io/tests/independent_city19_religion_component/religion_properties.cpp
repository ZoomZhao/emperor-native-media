#include "city19_religion_archive.hpp"
#include <iostream>
#include <utility>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void u16(Bytes &b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void u32(Bytes &b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
bool rejects(const Bytes &b,const PostGoalWalk &p,const std::string &why,bool commerce=true){try{(void)walk_city19_religion_following({b.data(),b.size()},p,commerce);return false;}catch(const std::runtime_error &e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){
    std::vector<std::pair<std::string,bool>> checks;auto check=[&](std::string n,bool ok){checks.push_back({n,ok});};
    // Pure-component prefix: previous city context is qualified separately by the CLI.
    Bytes b(3355);for(int i=0;i<10;++i){u16(b,1);u32(b,0);}u16(b,1);u32(b,11);b.push_back(12);for(int i=0;i<3;++i)u32(b,13+i);b.insert(b.end(),0xcb10,7);
    u16(b,4);u16(b,1);for(int i=0;i<12;++i)u32(b,21+i);u32(b,41);u32(b,42);b.push_back(43);b.push_back(44);u32(b,45);b.push_back(46);for(int i=0;i<4;++i)u32(b,47+i);
    const size_t start=b.size();u16(b,1);
    for(int c=0;c<4;++c){u16(b,3);for(int m=0;m<3;++m){u16(b,2);for(int f=0;f<5;++f)u32(b,0x80000000U+unsigned(c*100+m*10+f));}}
    const size_t raw=b.size();b.insert(b.end(),20,255);const size_t religion_end=b.size();u16(b,1);const size_t pool=b.size();
    // Mainpool-looking version/count/tag bytes are outside this component's domain.
    u16(b,1);u32(b,1);u16(b,65535);b.insert(b.end(),32,255);const Bytes before=b;
    const auto goals=walk_city19_housing_goals({b.data(),16384});const auto post=walk_city19_postgoals({b.data(),b.size()},goals,true);
    const auto full=walk_city19_religion_following({b.data(),b.size()},post,true);const auto only=walk_city19_religion_following({b.data(),b.size()},post,false);
    check("Religion variable start and294 wire includes no constructor clear12",start==55501&&full.start==start&&full.religion.end-full.religion.start==294&&raw-start==274);
    bool nested=full.children.size()==4;
    for(size_t c=0;c<full.children.size();++c){const auto &child=full.children[c];nested=nested&&child.start==start+2+c*68&&child.end-child.start==68&&child.version==3&&child.embedded.size()==3;
        for(size_t m=0;m<child.embedded.size();++m){const auto &entry=child.embedded[m];nested=nested&&entry.start==child.start+2+m*22&&entry.end-entry.start==22&&entry.version==2&&entry.fields.size()==5;
            for(size_t f=0;f<entry.fields.size();++f)nested=nested&&entry.fields[f].bytes==4&&entry.fields[f].receiver_offset==4+f*4;}}
    check("four children and twelve embedded members retain exact boundaries",nested);
    check("raw20 pointer-looking bits stay opaque and contiguous",full.children.size()==4&&full.religion.fields.size()==1&&full.religion.fields[0].offset==raw&&full.religion.fields[0].bytes==20&&full.children.back().end==raw&&b==before);
    check("Commerce reads only2 and leaves mainpool version/count/tag unread",full.commerce_qualified&&full.commerce.end-full.commerce.start==2&&full.end==pool&&b.size()-full.end==40&&full.stop_reason=="mainpool_before_section_and_objects");
    Bytes bad_commerce=b;bad_commerce[religion_end]=255;bad_commerce[religion_end+1]=255;
    check("Religion checkpoint does not peek unsupported Commerce",only.end==religion_end&&!only.commerce_qualified&&walk_city19_religion_following({bad_commerce.data(),bad_commerce.size()},post,false).end==religion_end&&rejects(bad_commerce,post,"Commerce section version",true));
    Bytes legacy=b;legacy[start+2]=2;legacy[start+3]=0;
    check("child legacy2 refuses without implementing numeric adjustment",rejects(legacy,post,"Worship child version outside qualified source3 subset"));
    Bytes embedded1=b;embedded1[start+4]=1;embedded1[start+5]=0;
    check("embedded source1 raw20 alternative remains outside source2 subset",rejects(embedded1,post,"Worship embedded version outside qualified source2 subset"));
    Bytes shortword=b;shortword.resize(pool-1);
    check("Commerce final version byte is mandatory",rejects(shortword,post,"truncated"));
    Bytes large=b;large.resize(65536,255);const bool exact=walk_city19_religion_following({large.data(),large.size()},post,true).end==pool;large.push_back(255);
    check("65536 accepts known296 bytes but65537 rejects",exact&&rejects(large,post,"segment/budget"));
    PostGoalWalk wrong=post;wrong.sections.pop_back();
    check("incomplete preceding consumer set refuses Religion",rejects(b,wrong,"segment/budget"));
    size_t failed=0;std::cout<<"{\"checks\":[";
    for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++failed;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}
    std::cout<<"],\"passed\":"<<checks.size()-failed<<",\"failed\":"<<failed<<"}\n";return failed?1:0;
}
