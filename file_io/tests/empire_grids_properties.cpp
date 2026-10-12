#include "city19_empire_grids_archive.hpp"
#include <functional>
#include <iostream>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
void word(Bytes &b,uint16_t v){b.push_back(uint8_t(v));b.push_back(uint8_t(v>>8));}
EmpireInlineWalk prior(uint16_t v=3){EmpireInlineWalk p;p.version=v;p.end=11;p.inline_complete=true;p.cache_next=6020;p.following_grids_required=v>=2;p.trailing_parent4_required=v==3;return p;}
void child(Bytes&b,uint16_t v=5){word(b,v);b.insert(b.end(),627,255);}
void grid(Bytes&b,uint16_t v=15){word(b,v);b.insert(b.end(),1192,255);for(int i=0;i<22;++i)child(b);b.insert(b.end(),334,255);}
Bytes full(bool tail=true){Bytes b(11,255);for(int i=0;i<22;++i)grid(b);if(tail)b.insert(b.end(),4,255);return b;}
EmpireGridWalk walk(const Bytes&b,EmpireGridLimits l={},uint16_t v=3){return walk_city19_empire_grids({b.data(),b.size()},prior(v),l);}
bool rejects(const Bytes&b,const char *reason,EmpireGridLimits l={}){try{(void)walk(b,l);return false;}catch(const std::runtime_error&e){return std::string(e.what()).find(reason)!=std::string::npos;}}
int main(){std::vector<std::pair<std::string,bool>> checks;auto check=[&](const char*n,std::function<bool()> f){bool ok=false;try{ok=f();}catch(const std::exception&){ok=false;}checks.push_back({n,ok});};
    check("source request totals distinguish both destination strides",[]{size_t pre=0,post=0,childn=0;for(const auto&f:empire_grid15_pre)pre+=f.bytes;for(const auto&f:empire_grid15_post)post+=f.bytes;for(const auto&f:empire_grid_child5_fields)childn+=f.bytes;return std::size(empire_grid15_pre)==30&&std::size(empire_grid15_post)==28&&std::size(empire_grid_child5_fields)==24&&pre==1192&&post==334&&childn==627&&2+childn!=0x274&&2+pre+22*(2+childn)+post!=0x3c00;});
    check("child5 shared78 and264268 tail are literal raw4",[]{return empire_grid_child5_fields[14].receiver_offset==0x78&&empire_grid_child5_fields[14].bytes==4&&empire_grid_child5_fields[22].receiver_offset==0x264&&empire_grid_child5_fields[23].receiver_offset==0x268&&empire_grid_child5_fields[23].bytes==4;});
    check("first grid15 with22 own5 consumes15366 not15360",[]{Bytes b(11,255);grid(b);EmpireGridLimits l;l.max_grids=1;auto r=walk(b,l);return r.grids.size()==1&&r.grids[0].end-r.grids[0].start==15366&&r.grids[0].children.size()==22&&r.grids[0].children.back().end-r.grids[0].children.back().start==629&&r.end==b.size()&&!r.grids_complete;});
    check("22 grids and parent3 tail consume338056 keeping input and cache",[]{auto b=full();auto original=b;auto r=walk(b);return r.grids.size()==22&&r.grids_complete&&r.parent_tail_qualified&&r.parent_tail_offset==11+338052&&r.parent_byte_requests_complete&&r.end==11+338056&&r.cache_next==6020&&b==original;});
    check("parent2 has22 grids without parent4",[]{auto b=full(false);auto r=walk(b,{},2);return r.grids_complete&&r.parent_byte_requests_complete&&!r.parent_tail_qualified&&r.end==b.size();});
    check("parent1 consumes no grid header",[]{Bytes b(11,255);word(b,65535);auto r=walk(b,{},1);return r.end==11&&r.grids.empty()&&r.parent_byte_requests_complete;});
    check("all15366 proper first grid cuts reject truncated",[]{Bytes b(11,255);grid(b);EmpireGridLimits l;l.max_grids=1;for(size_t n=0;n<15366;++n){Bytes cut(b.begin(),b.begin()+11+n);if(!rejects(cut,"truncated",l))return false;}return walk(b,l).grids.size()==1;});
    check("parent3 four proper tail cuts reject",[]{auto b=full();for(size_t n=0;n<4;++n){Bytes cut(b.begin(),b.end()-4+n);if(!rejects(cut,"truncated"))return false;}return true;});
    check("unknown grid14 consumes word only,never sizeof skip",[]{Bytes b(11,255);word(b,14);b.insert(b.end(),20000,255);auto r=walk(b);return r.end==13&&r.pending.present&&!r.pending.child&&r.pending.version==14&&r.grids.empty()&&!r.parent_byte_requests_complete;});
    check("unknown child4 stops after qualified pre and own word",[]{Bytes b(11,255);word(b,15);b.insert(b.end(),1192,255);child(b,4);b.insert(b.end(),20000,255);auto r=walk(b);return r.end==1207&&r.pending.present&&r.pending.child&&r.pending.child_ordinal==0&&r.pending.version==4&&r.grids.empty();});
    check("unknown child after first629 record is not inferred5",[]{Bytes b(11,255);word(b,15);b.insert(b.end(),1192,255);child(b);child(b,4);auto r=walk(b);return r.pending.child&&r.pending.child_ordinal==1&&r.end==1836;});
    check("one grid checkpoint leaves second header unread",[]{Bytes b(11,255);grid(b);const auto end=b.size();word(b,14);auto r=walk(b,{4915584,1,false,false});return r.end==end&&!r.pending.present&&r.grids.size()==1;});
    check("header only checkpoints both known and unknown versions",[]{for(uint16_t v:{15,14,65535}){Bytes b(11,255);word(b,v);auto r=walk(b,{4915584,22,true,false});if(r.end!=13||!r.pending.present||r.pending.version!=v)return false;}return true;});
    check("all22 stop before missing parent3 raw4",[]{auto b=full(false);auto r=walk(b,{4915584,22,false,true});return r.grids_complete&&!r.parent_tail_qualified&&!r.parent_byte_requests_complete&&r.end==b.size();});
    check("unrelated following bytes stay unread",[]{auto b=full();const auto end=b.size();b.insert(b.end(),128,255);auto r=walk(b);return r.end==end&&b.size()-r.end==128;});
    check("modern budget0,23 and prefix overhard refuse",[]{auto b=full();for(int i=0;i<3;++i){EmpireGridLimits l;if(i==0)l.max_grids=0;if(i==1)l.max_grids=23;if(i==2)l.max_prefix=4915585;if(!rejects(b,"context/modern budget",l))return false;}return true;});
    check("incomplete inline and wrong count or branch context refuse",[]{auto b=full();for(int i=0;i<4;++i){auto p=prior();if(i==0)p.inline_complete=false;if(i==1)p.count=1;if(i==2)p.following_grids_required=false;if(i==3)p.trailing_parent4_required=false;try{(void)walk_city19_empire_grids({b.data(),b.size()},p);return false;}catch(const std::runtime_error&){}}return true;});
    size_t fail=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<checks.size();++i){if(i)std::cout<<',';if(!checks[i].second)++fail;std::cout<<"{\"name\":\""<<checks[i].first<<"\",\"passed\":"<<(checks[i].second?"true":"false")<<'}';}std::cout<<"],\"passed\":"<<checks.size()-fail<<",\"failed\":"<<fail<<"}\n";return fail?1:0;
}
