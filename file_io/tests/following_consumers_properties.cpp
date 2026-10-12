#include "city19_following_vectors_archive.hpp"
#include <functional>
#include <iostream>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void word(Bytes&b,uint16_t n){b.push_back(uint8_t(n));b.push_back(uint8_t(n>>8));}
void dword(Bytes&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
EmpireGridWalk prior(){EmpireGridWalk p;p.parent_version=3;p.parent_byte_requests_complete=p.grids_complete=p.parent_tail_qualified=true;p.grids.resize(22);p.end=11;p.cache_next=6020;return p;}
void fixed(Bytes&b,uint16_t v=4,uint8_t presence=0){word(b,v);b.insert(b.end(),32,255);b.push_back(presence);}
Bytes fixed_list(){Bytes b(11,255);for(int i=0;i<12;++i)fixed(b);return b;}
void empty_vector_record(Bytes&b){word(b,1);for(int i=0;i<6;++i)dword(b,0);}
FollowingFixedWalk fwalk(const Bytes&b,FollowingFixedLimits l={}){return walk_city19_following_fixed({b.data(),b.size()},prior(),l);}
FollowingVectorWalk vwalk(const Bytes&b,FollowingVectorLimits l={}){auto f=fwalk(b);return walk_city19_following_vectors({b.data(),b.size()},f,l);}
bool rejects(const Bytes&b,const char*why,FollowingFixedLimits l={}){try{(void)fwalk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
bool vrejects(const Bytes&b,const char*why,FollowingVectorLimits l={}){try{(void)vwalk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(why)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("12 own4 records consume420 without56B padding",[]{auto b=fixed_list();auto original=b;auto r=fwalk(b,{4915584,12,false});return r.complete&&r.records.size()==12&&r.records[0].end-r.records[0].start==35&&r.records.back().end==431&&r.end==b.size()&&r.cache_next==6020&&b==original;});
    check("next header word qualifies independently without payload",[]{auto b=fixed_list();word(b,65535);b.insert(b.end(),100,255);auto r=fwalk(b);return r.complete&&r.next_header_present&&r.next_version==65535&&r.end==433;});
    check("all35 proper first record cuts reject",[]{Bytes b(11,255);fixed(b);for(size_t n=0;n<35;++n){Bytes cut(b.begin(),b.begin()+11+n);if(!rejects(cut,"truncated",{4915584,1,false}))return false;}return fwalk(b,{4915584,1,false}).end==46;});
    check("any nonzero presence stops before unknown virtual8",[]{for(uint8_t p:{1,7,255}){Bytes b(11,255);fixed(b,4,p);word(b,4);b.insert(b.end(),100,255);auto r=fwalk(b);if(!r.pending_extension||r.complete||r.end!=46||r.records.size()!=1||r.records[0].full_record_qualified)return false;}return true;});
    check("legacy1,2,3 andunknown5 stop after word not guessed35",[]{for(uint16_t v:{0,1,2,3,5,65535}){Bytes b(11,255);fixed(b,v);auto r=fwalk(b);if(!r.pending_version_present||r.pending_version!=v||r.end!=13||!r.records.empty())return false;}return true;});
    check("fixed item checkpoint leaves missing next word unread",[]{Bytes b(11,255);fixed(b);auto r=fwalk(b,{4915584,1,false});return r.end==46&&r.records.size()==1&&!r.complete&&!r.next_header_present;});
    check("fixed modern zero13 and prefix limits refuse",[]{auto b=fixed_list();for(int i=0;i<3;++i){FollowingFixedLimits l;if(i==0)l.max_records=0;if(i==1)l.max_records=13;if(i==2)l.max_prefix=4915585;if(!rejects(b,"context/modern budget",l))return false;}return true;});
    check("incomplete previous parent rejects",[]{auto b=fixed_list();auto p=prior();p.parent_byte_requests_complete=false;try{(void)walk_city19_following_fixed({b.data(),b.size()},p);return false;}catch(const std::runtime_error&){return true;}});
    check("27 empty source1 vectors consume702 thennext word",[]{auto b=fixed_list();for(int i=0;i<27;++i)empty_vector_record(b);word(b,7);b.insert(b.end(),100,255);auto original=b;auto r=vwalk(b);return r.complete&&r.records.size()==27&&r.records[0].end-r.records[0].start==26&&r.records[26].end==1133&&r.next_version==7&&r.next_header_start==1133&&r.end==1135&&r.cache_next==6020&&b==original;});
    check("first nonzero vector count stops before object tag,not raw4",[]{auto b=fixed_list();word(b,1);dword(b,2);word(b,65535);b.insert(b.end(),64,255);auto r=vwalk(b);return r.pending_present&&r.pending_objects&&r.pending_group==0&&r.pending.counts.size()==1&&r.pending.counts[0]==2&&r.end==437&&r.records.empty()&&r.cache_next==6020;});
    check("nonzero after five zeros stops at six-count boundary",[]{auto b=fixed_list();word(b,1);for(int i=0;i<5;++i)dword(b,0);dword(b,1);word(b,0);auto r=vwalk(b);return r.pending_group==5&&r.pending.counts.size()==6&&r.end==457&&r.pending_objects;});
    check("unknown next vector2 stops before counts",[]{auto b=fixed_list();word(b,2);b.insert(b.end(),24,255);auto r=vwalk(b);return r.pending_present&&!r.pending_objects&&r.pending.version==2&&r.pending.counts.empty()&&r.end==433;});
    check("all26 proper empty vector record cuts reject",[]{auto b=fixed_list();auto start=b.size();empty_vector_record(b);for(size_t n=0;n<26;++n){Bytes cut(b.begin(),b.begin()+start+n);if(!vrejects(cut,"truncated",{4915584,1,8192,false}))return false;}return vwalk(b,{4915584,1,8192,false}).end==457;});
    check("negative and8193 counts rejected by modern policy",[]{for(uint32_t n:{0xffffffffU,0x80000000U,8193U}){auto b=fixed_list();word(b,1);dword(b,n);if(!vrejects(b,n>0x7fffffff?"negative vector":"exceeds modern budget"))return false;}return true;});
    check("8192 positive count checkpoints without allocation or tag read",[]{auto b=fixed_list();word(b,1);dword(b,8192);auto r=vwalk(b);return r.pending_objects&&r.pending.counts[0]==8192&&r.end==b.size();});
    check("vector batch checkpoint leaves next wrong word unread",[]{auto b=fixed_list();empty_vector_record(b);const auto end=b.size();word(b,2);auto r=vwalk(b,{4915584,1,8192,true});return r.records.size()==1&&!r.complete&&!r.pending_present&&r.end==end;});
    check("all27 may end before missing593140 header",[]{auto b=fixed_list();for(int i=0;i<27;++i)empty_vector_record(b);auto r=vwalk(b,{4915584,27,8192,false});return r.complete&&!r.next_header_present&&r.end==b.size();});
    check("missing or incomplete fixed context cannot enter vectors",[]{auto b=fixed_list();word(b,1);auto f=fwalk(b);f.complete=false;try{(void)walk_city19_following_vectors({b.data(),b.size()},f);return false;}catch(const std::runtime_error&){return true;}});
    size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
