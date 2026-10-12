#include "city19_figurefsa_objects.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
using Cache=std::vector<FigureFsaWireCacheEntry>;
using Type=FigureFsaWireType;
constexpr auto contract=FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10;
constexpr Type S=Type::State,T=Type::Transition,AC=Type::ActionContext,A=Type::Action,C=Type::Context,M=Type::Match;
const char*names[]={"FSAState","FSATransition","FSAActionContext","FSAAction","FSAContext","FSAMatch"};
void need(bool p,const char*s){if(!p)throw std::runtime_error(s);}
void u16(Bytes&b,uint16_t x){b.push_back(uint8_t(x));b.push_back(uint8_t(x>>8));}
void u32(Bytes&b,uint32_t x){for(unsigned i=0;i<4;++i)b.push_back(uint8_t(x>>(i*8)));}
void set16(Bytes&b,size_t p,uint16_t x){b.at(p)=uint8_t(x);b.at(p+1)=uint8_t(x>>8);}
void string(Bytes&b,const Bytes&s={}){u32(b,uint32_t(s.size()));b.insert(b.end(),s.begin(),s.end());}
void tag(Bytes&b,uint32_t x,bool extended=false){if(extended){u16(b,0x7fff);u32(b,x);}else u16(b,uint16_t(x&0x7fff)|((x&0x80000000)?0x8000:0));}
Cache catalog(){Cache c{{}};for(auto t:{S,T,AC,A,C,M})c.push_back({FigureFsaCacheKind::Class,t,0,0});return c;}
struct Builder {
 Bytes b;Cache initial=catalog();uint32_t next_id=7;bool declare=false,extended=false;
 std::vector<size_t> payload_starts;
 std::vector<uint32_t> payload_ids;std::vector<Type> payload_types;
 std::vector<size_t> declaration_starts;
 size_t base_end=0;
 void head(unsigned base=2,std::array<uint32_t,7> legacy={}) {
  u16(b,uint16_t(base));string(b);b.push_back(0xff);
  if(base==1){for(size_t i=0;i<7;++i){u32(b,legacy[i]);for(size_t k=0;k<legacy[i];++k)tag(b,0);if(i==1)tag(b,0);}}
 }
 void tail(unsigned own=2,uint32_t name_count=0,bool current=false) {
  base_end=b.size();u16(b,uint16_t(own));if(own!=2){b.push_back(current?0xff:0);if(current)string(b);if(own!=4){u32(b,name_count);for(uint32_t i=0;i<name_count;++i)string(b);}}
  for(unsigned i=0;i<3;++i){u16(b,1);for(unsigned k=0;k<4;++k)u32(b,0x80706050+i*32+k);b.push_back(uint8_t(0xa0+i));}
 }
 void outer_nulls(unsigned n=3){for(unsigned i=0;i<n;++i)tag(b,0);}
 uint32_t object(Type t,const std::function<void(uint32_t)>&payload) {
  uint32_t cls=unsigned(t)+1;
  if(declare){declaration_starts.push_back(b.size());u16(b,0xffff);u16(b,0);u16(b,uint16_t(std::strlen(names[unsigned(t)])));b.insert(b.end(),names[unsigned(t)],names[unsigned(t)]+std::strlen(names[unsigned(t)]));cls=next_id++;}
  else tag(b,0x80000000|cls,extended);
  const auto id=next_id++;payload_starts.push_back(b.size());payload_ids.push_back(id);payload_types.push_back(t);payload(id);return id;
 }
 void transition(const std::function<void()>&match={},const std::function<void()>&state={}){u16(b,1);u32(b,match?1:0);if(match)match();b.push_back(0xff);u32(b,0xffffffff);for(unsigned i=0;i<16;++i)b.push_back(uint8_t(0x80+i));if(state)state();else tag(b,0);u32(b,0);}
};
FigureFsaObjectWalk read(const Builder&f,FigureFsaObjectLimits l={}) {return walk_city19_figurefsa_exact_objects({f.b.data(),f.b.size()},contract,f.initial,0,l);}
template<class F>void rejects(F f,const std::string&expected){bool yes=false;try{f();}catch(const std::runtime_error&e){yes=true;need(e.what()==expected,"wrong error category");}need(yes,"expected rejection missing");}
Builder target(Type target,bool declarations=false,bool extended=false){Builder f;f.declare=declarations;f.extended=extended;if(declarations){f.initial={{}};f.next_id=1;}f.head();
 f.object(S,[&](uint32_t){u16(f.b,1);string(f.b);const bool contexts=target==AC||target==A||target==C;u32(f.b,contexts?1:0);
  if(contexts)f.object(AC,[&](uint32_t){u16(f.b,1);if(target==A)f.object(A,[&](uint32_t){u16(f.b,1);string(f.b);});else tag(f.b,0);if(target==C)f.object(C,[&](uint32_t){u16(f.b,1);u32(f.b,0x89abcdef);});else tag(f.b,0);});
  u32(f.b,0);if(target==T||target==M)f.object(T,[&](uint32_t){f.transition(target==M?std::function<void()>([&]{f.object(M,[&](uint32_t){u16(f.b,1);string(f.b);u32(f.b,0xfedcba98);});}):std::function<void()>{});});else tag(f.b,0);u32(f.b,0);
 });f.outer_nulls(2);u32(f.b,0);f.tail();return f;}
Builder many(size_t n,unsigned base=2,std::array<uint32_t,7> legacy={},unsigned own=2,uint32_t names_count=0,bool current=false){Builder f;f.head(base,legacy);f.outer_nulls();u32(f.b,uint32_t(n));for(size_t i=0;i<n;++i)f.object(S,[&](uint32_t){u16(f.b,1);string(f.b);u32(f.b,0);u32(f.b,0);tag(f.b,0);u32(f.b,0);});f.tail(own,names_count,current);return f;}
Builder chain(size_t levels){Builder f;f.head();std::function<void(size_t,Type)> node;
 node=[&](size_t n,Type t){f.object(t,[&](uint32_t){if(t==S){u16(f.b,1);string(f.b);u32(f.b,0);u32(f.b,0);if(n>1)node(n-1,T);else tag(f.b,8);u32(f.b,0);}else f.transition({},[&]{if(n>1)node(n-1,S);else tag(f.b,7);});});};
 node(levels,S);f.outer_nulls(2);u32(f.b,0);f.tail();return f;}
std::string fingerprint(const FigureFsaObjectWalk&r){std::ostringstream s;s<<r.start<<','<<r.base_end<<','<<r.end<<','<<r.base_version<<','<<r.version<<','<<r.has_current_name;
 for(auto&x:r.cache)s<<"c"<<int(x.kind)<<','<<int(x.family)<<','<<x.class_id<<','<<x.schema;
 for(auto&x:r.references)s<<"r"<<x.offset<<','<<x.bytes<<','<<x.object_id<<','<<x.source_call<<','<<int(x.expected)<<','<<x.null_reference<<','<<x.extended;
 for(auto&x:r.declarations)s<<"d"<<x.tag_offset<<','<<x.name_offset<<','<<x.name_bytes<<','<<x.class_id<<','<<x.schema<<','<<int(x.family);
 for(auto&x:r.payloads){s<<"p"<<x.offset<<','<<x.end<<','<<x.object_id<<','<<x.class_id<<','<<int(x.family)<<','<<x.version;for(auto&z:x.fields)s<<','<<z.offset<<','<<z.bytes<<','<<z.source_call;}
 for(auto&x:r.strings)s<<"s"<<x.length_offset<<','<<x.offset<<','<<x.bytes<<','<<x.source_call;
 for(auto&x:r.counts)s<<"n"<<x.offset<<','<<x.declared<<','<<x.iterations<<','<<x.source_call;
 for(auto&x:r.flags)s<<"f"<<x.offset<<','<<x.bytes<<','<<x.source_call;
 for(auto&x:r.embedded){s<<"e"<<x.offset<<','<<x.end<<','<<x.version;for(auto&z:x.fields)s<<','<<z.offset<<','<<z.bytes<<','<<z.source_call;s<<','<<x.flag.offset<<','<<x.flag.bytes<<','<<x.flag.source_call;}return s.str();}
bool same_cache(const Cache&a,const Cache&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(std::tie(a[i].kind,a[i].family,a[i].class_id,a[i].schema)!=std::tie(b[i].kind,b[i].family,b[i].class_id,b[i].schema))return false;return true;}
void minimum_source(){const size_t widths[]={20,33,6,6,6,10};for(auto t:{S,T,AC,A,C,M}){auto f=target(t);auto r=read(f);auto it=std::find_if(r.payloads.begin(),r.payloads.end(),[&](auto&p){return p.family==t;});need(it!=r.payloads.end()&&it->version==1&&it->end-it->offset==widths[unsigned(t)],"six minimum widths from readers, not memory sizes");if(t==T){need(it->fields.size()==3&&it->fields[0].bytes==4&&it->fields[1].bytes==8&&it->fields[2].bytes==8,"Transition raw field widths");need(it->fields[0].source_call==0x4f6a19&&it->fields[1].source_call==0x4f6a24&&it->fields[2].source_call==0x4f6a2f,"Transition field source order");}if(t==C||t==M)need(it->fields.size()==1&&it->fields[0].source_call==(t==C?0x4f75feu:0x4f7d62u)&&it->fields[0].bytes==4,"Context/Match DWORD call");}}
void declaration_order(){for(auto t:{S,T,AC,A,C,M}){auto f=target(t,true);auto r=read(f);need(r.declarations.size()==r.payloads.size(),"one class per fixture new object");for(size_t i=0;i<r.payloads.size();++i){auto&p=r.payloads[i];auto&d=r.declarations[i];need(d.class_id+1==p.object_id&&p.class_id==d.class_id&&d.family==p.family&&d.schema==0&&p.version==1,"class/object registration and schema distinction");need(d.tag_offset==f.declaration_starts[i]&&d.name_offset==d.tag_offset+6&&p.offset==d.name_offset+d.name_bytes,"class wire then own version");need(r.cache[d.class_id].kind==FigureFsaCacheKind::Class&&r.cache[p.object_id].kind==FigureFsaCacheKind::Object,"mixed cache kind ledger");}}}
void depth_cycle(){auto good=chain(16);auto r=read(good);need(r.payloads.size()==16&&r.cache.size()==23,"16 new levels accepted");need(r.payloads.back().family==T,"last Transition closes State cycle");bool cycle=false;for(auto&x:r.references)if(x.source_call==0x4f6a39&&x.object_id==7)cycle=true;need(cycle,"in-progress State ID visible before payload finishes");FigureFsaObjectLimits l;l.max_depth=15;rejects([&]{read(good,l);},"FigureFSA object depth/count budget");auto bad=chain(17);rejects([&]{read(bad);},"FigureFSA object depth/count budget");l.max_depth=2;need(read(chain(2),l).payloads.size()==2,"backref doesn't consume depth");}
void object_cache_limits(){auto f=many(128);FigureFsaObjectLimits l;l.max_depth=1;l.max_cache=135;auto r=read(f,l);need(r.payloads.size()==128&&r.cache.size()==135,"128 flattened objects and exact cache boundary");auto bad=many(129);rejects([&]{read(bad);},"FigureFSA object depth/count budget");l.max_cache=134;rejects([&]{read(f,l);},"FigureFSA object cache budget");l={};l.max_objects=127;rejects([&]{read(f,l);},"FigureFSA object depth/count budget");}
void reachable_string_cap(){auto f=many(128,2,{},5,256,true);auto r=read(f);need(r.strings.size()==386,"reachable exact six-class maximum of names");FigureFsaObjectLimits l;l.max_strings=386;need(read(f,l).end==f.b.size(),"lowered inclusive strings cap");l.max_strings=385;rejects([&]{read(f,l);},"FigureFSA object string count budget");}
void global_refs(){std::array<uint32_t,7> n{{256,256,252,0,0,0,0}};auto f=many(128,1,n);need(read(f).references.size()==1024,"nested refs count against outer global budget");n[2]=253;auto bad=many(128,1,n);rejects([&]{read(bad);},"FigureFSA object reference budget");}
void explicit_rejections(){auto f=target(S,true);const auto p=f.declaration_starts[0];for(const std::string&name:std::vector<std::string>{"UnknownDerivedFSA","FSAStateX",std::string("FSAState\0",9)}){auto bad=f;bad.b.erase(bad.b.begin()+p+6,bad.b.begin()+p+14);bad.b.insert(bad.b.begin()+p+6,name.begin(),name.end());set16(bad.b,p+4,uint16_t(name.size()));rejects([&]{read(bad);},"FigureFSA unsupported concrete class declaration");}auto bad=f;set16(bad.b,p+2,1);rejects([&]{read(bad);},"FigureFSA exact class schema mismatch");for(uint16_t v:{uint16_t(0),uint16_t(2),uint16_t(0x100),uint16_t(0xffff)}){bad=f;set16(bad.b,f.payload_starts[0],v);rejects([&]{read(bad);},"FSAState own outside selected1 subset");}for(uint16_t len:{uint16_t(0),uint16_t(64),uint16_t(0xffff)}){bad=f;set16(bad.b,p+4,len);rejects([&]{read(bad);},"FigureFSA class name length outside source bound");}}
void atomic_late_errors(){auto f=target(M,true);auto original=f.initial;auto output=read(f);output.end=9999;const auto before=fingerprint(output);std::vector<Bytes> cases;auto b=f.b;b.pop_back();cases.push_back(b);b=f.b;set16(b,f.base_end+2+38,2);cases.push_back(b);for(auto bad:cases){auto copy=bad;auto faulty=f;faulty.b=bad;bool caught=false;try{output=read(faulty);}catch(const std::runtime_error&){caught=true;}need(caught&&fingerprint(output)==before&&bad==copy&&same_cache(f.initial,original),"all caller output/cache/bytes preserved after late errors");need(read(f).end==f.b.size(),"next invocation clean");}}
void high_cache(){for(bool extended:{false,true}){Builder f;f.initial.assign(1023,{FigureFsaCacheKind::Class,S,0,0});f.initial[0]={};f.head();tag(f.b,0x80000000|1022,extended);u16(f.b,1);string(f.b);u32(f.b,0);u32(f.b,0);tag(f.b,0);u32(f.b,0);tag(f.b,1023,true);tag(f.b,0);u32(f.b,0);f.tail();auto r=read(f);need(r.cache.size()==1024&&r.payloads[0].object_id==1023&&r.references[2].object_id==1023,"new last object followed by cached last index");FigureFsaObjectLimits l;l.max_cache=1023;rejects([&]{read(f,l);},"FigureFSA object cache budget");}}
void partition(){for(bool declarations:{false,true})for(bool extended:{false,true})for(auto t:{S,T,AC,A,C,M}){auto f=target(t,declarations,extended);auto r=read(f);std::vector<std::pair<size_t,size_t>> spans{{r.start,2},{r.base_end,2}};for(auto&p:r.payloads)spans.push_back({p.offset,2});for(auto&d:r.declarations)spans.push_back({d.tag_offset+2,4+d.name_bytes});for(auto&x:r.references)spans.push_back({x.offset,x.bytes});for(auto&x:r.strings){spans.push_back({x.length_offset,4});if(x.bytes)spans.push_back({x.offset,x.bytes});}for(auto&x:r.counts)spans.push_back({x.offset,4});for(auto&x:r.flags)spans.push_back({x.offset,x.bytes});for(auto&p:r.payloads)for(auto&x:p.fields)spans.push_back({x.offset,x.bytes});for(auto&e:r.embedded){spans.push_back({e.offset,2});for(auto&x:e.fields)spans.push_back({x.offset,x.bytes});spans.push_back({e.flag.offset,e.flag.bytes});}std::sort(spans.begin(),spans.end());size_t next=0;for(auto x:spans){need(x.first==next,"graph wire gap/overlap");next+=x.second;}need(next==r.end&&r.end==f.b.size(),"complete graph byte partition");for(size_t i=0;i<r.payloads.size();++i)for(size_t j=i+1;j<r.payloads.size();++j){auto&a=r.payloads[i];auto&b=r.payloads[j];need(a.end<=b.offset||b.end<=a.end,"payload intervals preorder nested or disjoint");}f.b.insert(f.b.end(),{0xff,0xff,0xff});need(fingerprint(read(f))==fingerprint(r),"trailing bytes ignored without extra registration");}}
std::string escape(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';o+=c;}return o;}
int main(){std::vector<std::pair<const char*,std::function<void()>>> tests{
 {"six_source_minimum_widths_and_raw_calls",minimum_source},{"declaration_class_object_registration_ledger",declaration_order},
 {"depth16_17_and_in_progress_cycle",depth_cycle},{"new_object128_129_and_exact_cache_budget",object_cache_limits},
 {"reachable_name_count386_385",reachable_string_cap},{"global_refs_include_nested_payloads1024_1025",global_refs},
 {"unknown_alias_schema_and_version_rejections",explicit_rejections},{"all_output_retained_after_late_graph_failure",atomic_late_errors},
 {"last_cache_slot_new_then_back_reference",high_cache},{"complete_graph_wire_partition_and_laminar_payloads",partition}};
 size_t passed=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<tests.size();++i){if(i)std::cout<<',';std::string error;try{tests[i].second();++passed;}catch(const std::exception&e){error=e.what();}std::cout<<"{\"id\":\""<<tests[i].first<<"\",\"passed\":"<<(error.empty()?"true":"false");if(!error.empty())std::cout<<",\"error\":\""<<escape(error)<<'"';std::cout<<'}';}std::cout<<"],\"passed\":"<<passed<<",\"failed\":"<<tests.size()-passed<<"}\n";return passed==tests.size()?0:1;}
