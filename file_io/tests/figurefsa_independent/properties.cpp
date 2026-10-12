#include "city19_figurefsa_archive.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
using Cache=std::vector<FigureFsaCacheEntry>;
constexpr auto contract=FigureFsaContract::cached_refs_root7b5bc8_reader4dad10;
constexpr auto S=FigureFsaFamily::State,T=FigureFsaFamily::Transition,A=FigureFsaFamily::ActionContext;
void require(bool x,const char*why){if(!x)throw std::runtime_error(why);}
void w16(Bytes&b,uint16_t x){b.push_back(uint8_t(x));b.push_back(uint8_t(x>>8));}
void w32(Bytes&b,uint32_t x){for(unsigned i=0;i<4;++i)b.push_back(uint8_t(x>>(8*i)));}
void patch16(Bytes&b,size_t p,uint16_t x){b.at(p)=uint8_t(x);b.at(p+1)=uint8_t(x>>8);}
void patch32(Bytes&b,size_t p,uint32_t x){for(unsigned i=0;i<4;++i)b.at(p+i)=uint8_t(x>>(8*i));}
void str(Bytes&b,const Bytes&s){w32(b,uint32_t(s.size()));b.insert(b.end(),s.begin(),s.end());}
void tag(Bytes&b,uint32_t id,bool extended=false){if(extended){w16(b,0x7fff);w32(b,id);}else w16(b,uint16_t(id));}
Cache nil(){return {{FigureFsaCacheKind::Null,S,0,0}};}
Cache typed(){return {{FigureFsaCacheKind::Null,S,0,0},{FigureFsaCacheKind::Class,S,0,73},
 {FigureFsaCacheKind::Object,S,1,94},{FigureFsaCacheKind::Class,T,0,11},
 {FigureFsaCacheKind::Object,T,3,12},{FigureFsaCacheKind::Class,A,0,81},
 {FigureFsaCacheKind::Object,A,5,82},{FigureFsaCacheKind::Object,S,1,95}};}
// Fixture grammar is bound to the saved source ledger in expectations.json.
// No payloads, factories, pointer values or game number expectations are used.
struct Fixture {
 Bytes b;size_t base_end=0,own=0,presence=0;
 std::vector<size_t> count_offsets,ref_offsets,string_offsets;
 std::array<size_t,3> embedded{};
};
Fixture make(unsigned base=2,unsigned own=2,uint8_t presence=0,
 std::array<uint32_t,8> counts={},uint32_t names=0,Bytes name={},bool typed_refs=false) {
 Fixture f;auto&b=f.b;
 auto reference=[&](FigureFsaFamily family){f.ref_offsets.push_back(b.size());tag(b,typed_refs?(family==S?2:family==T?4:6):0);};
 auto list=[&](uint32_t n,FigureFsaFamily family){f.count_offsets.push_back(b.size());w32(b,n);
   if(n<=0x7fffffff)for(uint32_t j=0;j<n;++j)reference(family);};
 w16(b,uint16_t(base));f.string_offsets.push_back(b.size());str(b,name);b.push_back(0xd3);
 if(base==1){list(counts[0],T);list(counts[1],T);reference(T);list(counts[2],T);
   list(counts[3],T);list(counts[4],A);list(counts[5],A);list(counts[6],S);}
 reference(S);reference(S);reference(S);list(counts[7],S);f.base_end=b.size();f.own=b.size();w16(b,uint16_t(own));
 if(own!=2){f.presence=b.size();b.push_back(presence);
   if(presence){f.string_offsets.push_back(b.size());str(b,Bytes{0,0x80,0xff});}
   if(own!=4){f.count_offsets.push_back(b.size());w32(b,names);
     if(names<=0x7fffffff)for(uint32_t j=0;j<names;++j){f.string_offsets.push_back(b.size());str(b,Bytes{uint8_t(j),0});}}}
 for(size_t i=0;i<3;++i){f.embedded[i]=b.size();w16(b,1);
   for(unsigned k=0;k<4;++k)w32(b,0x10203040+unsigned(i)*0x100+k);
   b.push_back(uint8_t(0xa0+i));}
 return f;
}
FigureFsaRecord read(const Bytes&b,const Cache&c=nil(),size_t start=0,FigureFsaLimits l={}){
 return walk_city19_figurefsa_cached_record({b.data(),b.size()},contract,c,start,l);
}
template<class F> void rejects(F f,const std::string&why){bool caught=false;try{f();}catch(const std::runtime_error&e){caught=true;require(e.what()==why,"wrong rejection category");}require(caught,"expected rejection did not occur");}
std::string digest(const FigureFsaRecord&r){std::ostringstream s;s<<r.start<<','<<r.base_end<<','<<r.end<<','<<r.base_version<<','<<r.version<<','<<r.has_current_name;
 for(auto&x:r.flags)s<<"f"<<x.offset<<','<<x.bytes<<','<<x.source_call;
 for(auto&x:r.strings)s<<"s"<<x.length_offset<<','<<x.offset<<','<<x.bytes<<','<<x.source_call;
 for(auto&x:r.references)s<<"r"<<x.offset<<','<<x.bytes<<','<<x.object_id<<','<<x.source_call<<','<<int(x.expected)<<','<<x.null_reference<<','<<x.extended;
 for(auto&x:r.counts)s<<"c"<<x.offset<<','<<x.declared<<','<<x.iterations<<','<<x.source_call;
 for(auto&x:r.embedded){s<<"e"<<x.offset<<','<<x.end<<','<<x.version;for(auto&y:x.fields)s<<','<<y.offset<<','<<y.bytes<<','<<y.source_call;s<<','<<x.flag.offset<<','<<x.flag.bytes<<','<<x.flag.source_call;}return s.str();}
bool same_cache(const Cache&a,const Cache&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)
 if(std::tie(a[i].kind,a[i].family,a[i].class_id,a[i].schema)!=std::tie(b[i].kind,b[i].family,b[i].class_id,b[i].schema))return false;return true;}
void source_order(){
 const std::array<uint32_t,8> ns{{1,2,1,2,1,2,1,2}};
 const std::array<uint32_t,8> ca{{0x4f46c1,0x4f46eb,0x4f471f,0x4f4749,0x4f4773,0x4f479d,0x4f47c7,0x4f480f}};
 const std::array<uint32_t,8> ra{{0x4f46d6,0x4f4700,0x4f4734,0x4f475e,0x4f4788,0x4f47b2,0x4f47dc,0x4f4824}};
 const std::array<FigureFsaFamily,8> families{{T,T,T,T,A,A,S,S}};
 for(unsigned base=1;base<=2;++base)for(unsigned own=2;own<=5;++own){auto f=make(base,own,0xff,ns,2,{0x31},true);auto r=read(f.b,typed());
  require(r.base_end==f.base_end&&r.end==f.b.size(),"source branch endpoints");
  std::vector<std::pair<uint32_t,FigureFsaFamily>> expected;
  std::vector<uint32_t> counts;
  if(base==1){for(size_t i=0;i<7;++i){counts.push_back(ca[i]);for(size_t k=0;k<ns[i];++k)expected.push_back({ra[i],families[i]});if(i==1)expected.push_back({0x4f4713,T});}
   for(auto a:{0x4f47efu,0x4f47f9u,0x4f4803u})expected.push_back({a,S});counts.push_back(ca[7]);for(size_t i=0;i<ns[7];++i)expected.push_back({ra[7],S});}
  else{for(auto a:{0x4f462bu,0x4f4635u,0x4f463fu})expected.push_back({a,S});counts.push_back(0x4f464b);for(size_t i=0;i<ns[7];++i)expected.push_back({0x4f4660,S});}
  require(r.references.size()==expected.size(),"all ordered typed refs present");
  for(size_t i=0;i<expected.size();++i)require(r.references[i].source_call==expected[i].first&&r.references[i].expected==expected[i].second&&r.references[i].offset==f.ref_offsets[i],"typed wrapper/order/source offset");
  if(own==3||own==5)counts.push_back(own==3?0x4db04c:0x4daf28);
  require(r.counts.size()==counts.size(),"own4 has no names count");for(size_t i=0;i<counts.size();++i)require(r.counts[i].source_call==counts[i]&&r.counts[i].offset==f.count_offsets[i],"count source order");
  std::vector<uint32_t> strings{base==1?0x4f4698u:0x4f4604u};if(own!=2)strings.push_back(own==3?0x4db026u:own==4?0x4dafc2u:0x4daf02u);
  if(own==3||own==5){strings.push_back(own==3?0x4db061u:0x4daf3du);strings.push_back(strings.back());}
  require(r.strings.size()==strings.size(),"string source sequence size");for(size_t i=0;i<strings.size();++i)require(r.strings[i].source_call==strings[i]&&r.strings[i].length_offset==f.string_offsets[i],"string source order");
  const std::array<uint32_t,4> raw{{0x4f8d0f,0x4f8d1a,0x4f8d25,0x4f8d30}};
  for(size_t i=0;i<3;++i){require(r.embedded[i].offset==f.embedded[i]&&r.embedded[i].end==f.embedded[i]+19,"embedded wire18 plus outer flag1");for(size_t k=0;k<4;++k)require(r.embedded[i].fields[k].source_call==raw[k],"embedded source calls");require(r.embedded[i].flag.source_call==(own==2?0x4db0e5u:own==3?0x4db0bcu:own==4?0x4daffcu:0x4daf98u),"outer flag source call");}
 }
}
void presence(){for(unsigned own:{3u,4u,5u})for(unsigned p:{0u,1u,2u,0x7fu,0x80u,0xffu}){auto f=make(2,own,uint8_t(p));auto r=read(f.b);require(r.has_current_name==(p!=0)&&r.strings.size()==(p?2u:1u),"presence uses zero versus nonzero byte");require(r.end==f.b.size(),"presence consumes optional name only");}}
void cumulative(){std::array<uint32_t,8> n{{256,256,256,252,0,0,0,0}};auto good=make(1,2,0,n);require(read(good.b).references.size()==1024,"1024 across lists accepted");n[3]=253;auto bad=make(1,2,0,n);rejects([&]{read(bad.b);},"FigureFSA reference budget");FigureFsaLimits l;l.max_refs=1023;rejects([&]{read(good.b,nil(),0,l);},"FigureFSA reference budget");}
void scalar_boundaries(){Bytes name(8192,0x81);name[17]=0;std::array<uint32_t,8> ns{};ns[7]=256;auto f=make(2,3,0,ns,256,name);auto r=read(f.b);require(r.strings[0].bytes==8192&&r.references.size()==259&&r.strings.size()==257&&r.end==f.b.size(),"scalar limits inclusive and independent");auto bad=f.b;patch32(bad,f.string_offsets[0],8193);rejects([&]{read(bad);},"FigureFSA string budget");bad=f.b;patch32(bad,f.count_offsets[0],257);rejects([&]{read(bad);},"FigureFSA count budget");bad=f.b;patch32(bad,f.count_offsets.back(),257);rejects([&]{read(bad);},"FigureFSA count budget");}
void signed_extremes(){for(uint32_t n:{0u,0x80000000u,0xffffffffu}){std::array<uint32_t,8> ns{};ns.fill(n);auto f=make(1,5,0,ns,n);auto r=read(f.b);require(r.references.size()==4&&r.counts.size()==9,"nonpositive lists read no refs/names");for(auto&x:r.counts)require(x.declared==(n<=0x7fffffff?int64_t(n):int64_t(n)-0x100000000LL)&&x.iterations==0,"INT32_MIN and -1 preserved");}auto f=make();patch32(f.b,f.string_offsets[0],0x80000000);rejects([&]{read(f.b);},"FigureFSA string budget");}
void zero_budgets(){auto f=make(2,5);FigureFsaLimits l;l.max_string=0;l.max_items=0;l.max_cache=1;l.max_refs=3;require(read(f.b,nil(),0,l).end==f.b.size(),"zero scalar budgets accept empty values");l.max_refs=0;rejects([&]{read(f.b,nil(),0,l);},"FigureFSA reference budget");l.max_refs=3;f=make(2,5,0xff);rejects([&]{read(f.b,nil(),0,l);},"FigureFSA string budget");}
void high_cache(){Cache c(1024,{FigureFsaCacheKind::Object,S,1,0});c[0]=nil()[0];c[1]={FigureFsaCacheKind::Class,S,0,0xffff};auto f=make();patch16(f.b,f.ref_offsets[0],1023);auto r=read(f.b,c);require(r.references[0].object_id==1023&&!r.references[0].extended,"compact1023 allowed");Bytes b=f.b;b.erase(b.begin()+f.ref_offsets[0],b.begin()+f.ref_offsets[0]+2);Bytes t;tag(t,1023,true);b.insert(b.begin()+f.ref_offsets[0],t.begin(),t.end());r=read(b,c);require(r.references[0].object_id==1023&&r.references[0].bytes==6&&r.references[0].extended,"extended1023 allowed");patch32(b,f.ref_offsets[0]+2,1024);rejects([&]{read(b,c);},"FigureFSA object reference not in cache");FigureFsaLimits l;l.max_cache=1023;rejects([&]{read(f.b,c,0,l);},"FigureFSA view/cache/budget argument");}
void extreme_views(){auto f=make();Bytes b(65536-f.b.size(),0xe1);size_t start=b.size();b.insert(b.end(),f.b.begin(),f.b.end());require(read(b,nil(),start).end==65536,"whole-view inclusive cap");b.push_back(0);rejects([&]{read(b,nil(),start);},"FigureFSA view/cache/budget argument");uint8_t dummy=0;const auto huge=std::numeric_limits<size_t>::max();rejects([&]{walk_city19_figurefsa_cached_record({&dummy,1},contract,nil(),huge);},"FigureFSA view/cache/budget argument");for(unsigned k=0;k<5;++k){FigureFsaLimits l;size_t*fields[]={&l.max_bytes,&l.max_string,&l.max_items,&l.max_refs,&l.max_cache};*fields[k]=huge;rejects([&]{read(f.b,nil(),0,l);},"FigureFSA view/cache/budget argument");}}
void atomic_output(){auto f=make(1,5,0xff,{{1,0,0,0,0,0,1,2}},1,{0x11},true);auto c=typed();const auto before_c=c;FigureFsaRecord output=read(f.b,c);output.start=9999;output.end=7777;const auto before=digest(output);
 std::vector<Bytes> bad;auto b=f.b;patch16(b,f.embedded[2],2);bad.push_back(b);b=f.b;b.pop_back();bad.push_back(b);b=f.b;patch16(b,f.ref_offsets.back(),4);bad.push_back(b);b=f.b;patch16(b,f.ref_offsets.back(),0x8001);bad.push_back(b);
 for(const auto&input:bad){auto immutable=input;bool caught=false;try{output=read(input,c);}catch(const std::runtime_error&){caught=true;}require(caught&&digest(output)==before&&input==immutable&&same_cache(c,before_c),"exception preserves caller output, bytes and complete cache");require(read(f.b,c).end==f.b.size(),"late error does not affect next invocation");}}
void tag_isolation(){auto f=make();const size_t p=f.ref_offsets[0];for(uint16_t word:{uint16_t(0xffff),uint16_t(0x8001),uint16_t(0x8000),uint16_t(0xfffe)}){Bytes b(f.b.begin(),f.b.begin()+p);w16(b,word);rejects([&]{read(b);},"FigureFSA new class/object payload outside cached-reference contract");}
 Bytes b(f.b.begin(),f.b.begin()+p);tag(b,0x80000001,true);rejects([&]{read(b);},"FigureFSA new class/object payload outside cached-reference contract");for(size_t n=2;n<6;++n){Bytes cut(b.begin(),b.begin()+p+n);rejects([&]{read(cut);},"goal field/tag/name truncated in bounded prefix");}
 // Mixed null and the same logical ID remain separate wire references.
 b=f.b;patch16(b,p,2);patch16(b,f.ref_offsets[2],2);auto r=read(b,typed());require(r.references.size()==3&&r.references[0].object_id==2&&r.references[1].null_reference&&r.references[2].object_id==2,"NULL and repeated logical IDs not deduplicated");require(read(f.b).end==f.b.size(),"tag rejection has no persistent cursor");}
void cache_assumptions(){auto f=make();auto c=typed();c[0].family=static_cast<FigureFsaFamily>(999);c[0].class_id=0xffffffff;c[0].schema=0xffff;require(read(f.b,c).end==f.b.size(),"null-slot irrelevant metadata");for(auto&e:c)if(e.kind!=FigureFsaCacheKind::Null)e.schema=0xffff;require(read(f.b,c).end==f.b.size(),"cache schema not stream version");c.back().family=static_cast<FigureFsaFamily>(999);rejects([&]{read(f.b,c);},"FigureFSA invalid cache metadata");c=typed();c.back().class_id=3;rejects([&]{read(f.b,c);},"FigureFSA object/class cache relation");c=typed();patch16(f.b,f.ref_offsets[0],7);require(read(f.b,c).references[0].object_id==7,"distinct cache IDs retain indices without physical identity inference");}
void partition(){uint32_t seed=0x102c19;for(size_t run=0;run<64;++run){seed=seed*1664525+1013904223;unsigned base=1+(seed&1),own=2+((seed>>1)%4);uint8_t presence=uint8_t(seed>>8);std::array<uint32_t,8> ns{};for(size_t i=0;i<8;++i)ns[i]=(seed>>(i*3))%4;auto f=make(base,own,presence,ns,seed%4,{uint8_t(seed),0,0x80},true);Bytes b(run%11,0xd0);size_t start=b.size();b.insert(b.end(),f.b.begin(),f.b.end());b.insert(b.end(),{0xff,0xff,0xff});auto r=read(b,typed(),start);
  std::vector<std::pair<size_t,size_t>> spans{{start,2},{r.base_end,2}};for(auto&x:r.strings){spans.push_back({x.length_offset,4});if(x.bytes)spans.push_back({x.offset,x.bytes});}for(auto&x:r.flags)spans.push_back({x.offset,x.bytes});for(auto&x:r.references)spans.push_back({x.offset,x.bytes});for(auto&x:r.counts)spans.push_back({x.offset,4});for(auto&x:r.embedded){spans.push_back({x.offset,2});for(auto&y:x.fields)spans.push_back({y.offset,y.bytes});spans.push_back({x.flag.offset,x.flag.bytes});}
  std::sort(spans.begin(),spans.end());size_t next=start;for(auto x:spans){require(x.first==next,"wire interval gap or overlap");next+=x.second;}require(next==r.end&&r.end==start+f.b.size(),"wire partition excludes trailing data");require(digest(r)==digest(read(b,typed(),start)),"repeat call produces same immutable record");}}
std::string escaped(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';o+=c;}return o;}
int main(){const std::vector<std::pair<const char*,std::function<void()>>> tests{
 {"saved_source_typed_order",source_order},{"all_nonzero_presence_values",presence},
 {"cumulative_reference_budget_1024_1025",cumulative},{"independent_scalar_inclusive_caps",scalar_boundaries},
 {"signed_int32_extremes",signed_extremes},{"zero_scalar_budgets_and_null_only_cache",zero_budgets},
 {"cache_boundary_compact_and_extended_1023",high_cache},{"whole_view_and_size_t_extremes",extreme_views},
 {"late_exception_preserves_caller_output",atomic_output},{"new_tags_and_extended_tag_isolation",tag_isolation},
 {"cache_witness_and_identity_boundaries",cache_assumptions},{"complete_wire_partition_and_repeatability",partition}};
 size_t passed=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<tests.size();++i){if(i)std::cout<<',';std::string error;try{tests[i].second();++passed;}catch(const std::exception&e){error=e.what();}std::cout<<"{\"id\":\""<<tests[i].first<<"\",\"passed\":"<<(error.empty()?"true":"false");if(!error.empty())std::cout<<",\"error\":\""<<escaped(error)<<'"';std::cout<<'}';}std::cout<<"],\"passed\":"<<passed<<",\"failed\":"<<tests.size()-passed<<"}\n";return passed==tests.size()?0:1;}
