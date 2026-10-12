#include "city19_figurefsa_objects.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>
using namespace emperor::file_io::research;
using Bytes=std::vector<uint8_t>;
using Cache=std::vector<FigureFsaWireCacheEntry>;
constexpr auto contract=FigureFsaObjectContract::exact_fsa_classes_root7b5bc8_reader4dad10;
constexpr auto S=FigureFsaWireType::State,O=FigureFsaWireType::OpaquePrefix;
void need(bool x,const char*s){if(!x)throw std::runtime_error(s);}
void u16(Bytes&b,uint16_t x){b.push_back(uint8_t(x));b.push_back(uint8_t(x>>8));}
void u32(Bytes&b,uint32_t x){for(unsigned i=0;i<4;++i)b.push_back(uint8_t(x>>(8*i)));}
void tag(Bytes&b,uint32_t x){u16(b,0x7fff);u32(b,x);}
void str(Bytes&b,const std::string&s={}){u32(b,uint32_t(s.size()));b.insert(b.end(),s.begin(),s.end());}
Cache prefix(size_t n=6020){Cache c(n,{FigureFsaCacheKind::Class,O,0,0x1234});c[0]={FigureFsaCacheKind::Null,O,0,0x5678};if(n>2)c[2]={FigureFsaCacheKind::Object,O,1,0xabcd};return c;}
bool same(const Cache&a,const Cache&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(std::tie(a[i].kind,a[i].family,a[i].class_id,a[i].schema)!=std::tie(b[i].kind,b[i].family,b[i].class_id,b[i].schema))return false;return true;}
struct Fixture{Bytes b;size_t tag_offset=0,name_offset=0,name_bytes=0,unknown_end=0;};
void head(Bytes&b,std::string name={}){u16(b,2);str(b,name);b.push_back(0xff);}
void tail(Bytes&b){u16(b,2);for(unsigned i=0;i<3;++i){u16(b,1);for(unsigned k=0;k<4;++k)u32(b,0x80706050+i*32+k);b.push_back(uint8_t(i));}}
void state(Bytes&b){u16(b,1);str(b);u32(b,0);u32(b,0);u16(b,0);u32(b,0);}
void declaration(Bytes&b,std::string name){u16(b,0xffff);u16(b,0);u16(b,uint16_t(name.size()));b.insert(b.end(),name.begin(),name.end());}
Fixture null_record(){Fixture f;head(f.b);for(unsigned i=0;i<3;++i)u16(f.b,0);u32(f.b,0);tail(f.b);return f;}
Fixture first(){Fixture f;head(f.b);declaration(f.b,"FSAState");state(f.b);tag(f.b,6021);u16(f.b,0);u32(f.b,0);tail(f.b);return f;}
Fixture reuse(){Fixture f;head(f.b);tag(f.b,6021);u16(f.b,0);tag(f.b,6021);u32(f.b,0);tail(f.b);return f;}
Fixture many(size_t n){Fixture f;head(f.b);for(unsigned i=0;i<3;++i)u16(f.b,0);u32(f.b,uint32_t(n));for(size_t i=0;i<n;++i){declaration(f.b,"FSAState");state(f.b);}tail(f.b);return f;}
Fixture unknown(bool after_known=false){Fixture f;head(f.b,"demo!");if(after_known){declaration(f.b,"FSAState");state(f.b);}f.tag_offset=f.b.size();declaration(f.b,"UnknownFSAType");f.name_offset=f.tag_offset+6;f.name_bytes=14;f.unknown_end=f.b.size();f.b.insert(f.b.end(),128,0xa5);return f;}
FigureFsaObjectWalk read(const Fixture&f,const Cache&c,size_t base=6020,size_t start=0,FigureFsaObjectLimits l={}){l.max_cache=8192;return walk_city19_figurefsa_with_archive_prefix({f.b.data(),f.b.size()},contract,c,base,start,l);}
template<class F>bool rejected(F f){try{f();return false;}catch(const std::runtime_error&){return true;}}
std::string cache_fingerprint(const Cache&c){std::ostringstream s;for(auto&x:c)s<<int(x.kind)<<','<<int(x.family)<<','<<x.class_id<<','<<x.schema<<';';return s.str();}
std::string fingerprint(const FigureFsaObjectWalk&r){std::ostringstream s;s<<r.start<<','<<r.base_end<<','<<r.end<<','<<r.base_version<<','<<r.version<<','<<r.has_current_name<<cache_fingerprint(r.cache);
 for(auto&x:r.references)s<<"r"<<x.offset<<','<<x.bytes<<','<<x.object_id<<','<<x.source_call<<','<<int(x.expected)<<','<<x.null_reference<<','<<x.extended;
 for(auto&x:r.declarations)s<<"d"<<x.tag_offset<<','<<x.name_offset<<','<<x.name_bytes<<','<<x.class_id<<','<<x.schema<<','<<int(x.family);
 for(auto&x:r.payloads){s<<"p"<<x.offset<<','<<x.end<<','<<x.object_id<<','<<x.class_id<<','<<int(x.family)<<','<<x.version;for(auto&y:x.fields)s<<','<<y.offset<<','<<y.bytes<<','<<y.source_call;}
 for(auto&x:r.strings)s<<"s"<<x.length_offset<<','<<x.offset<<','<<x.bytes<<','<<x.source_call;
 for(auto&x:r.counts)s<<"n"<<x.offset<<','<<x.declared<<','<<x.iterations<<','<<x.source_call;
 for(auto&x:r.flags)s<<"f"<<x.offset<<','<<x.bytes<<','<<x.source_call;
 for(auto&e:r.embedded){s<<"e"<<e.offset<<','<<e.end<<','<<e.version;for(auto&x:e.fields)s<<','<<x.offset<<','<<x.bytes<<','<<x.source_call;s<<','<<e.flag.offset<<','<<e.flag.bytes<<','<<e.flag.source_call;}return s.str();}
void initial_append_budget(){auto f=null_record();auto c=prefix();c.insert(c.end(),1024,{FigureFsaCacheKind::Class,S,0,0});need(read(f,c).cache.size()==7044,"1024 preexisting appended entries inclusive");c.push_back({FigureFsaCacheKind::Class,S,0,0});need(rejected([&]{read(f,c);}),"accepted initial cache7045 with prefix6020: 1025 appended entries exceeds declared1024 cap");}
void prefix_copy_ids(){auto c=prefix(),before=c;auto f=first();auto r=read(f,c);need(same(c,before)&&r.cache.size()==6022,"caller prefix preserved");need(same(Cache(r.cache.begin(),r.cache.begin()+6020),before),"all original IDs/kinds/relations/schema preserved");need(r.declarations[0].class_id==6020&&r.payloads[0].object_id==6021&&r.cache[6021].class_id==6020,"global registration IDs before payload");need(r.references[0].object_id==6021&&r.references[2].object_id==6021,"new State followed by cached State");}
void continued_record(){auto a=first(),b=reuse();Fixture both;both.b=a.b;both.b.insert(both.b.end(),b.b.begin(),b.b.end());auto r1=read(both,prefix());auto r2=read(both,r1.cache,6020,r1.end);need(r1.end==a.b.size()&&r2.start==r1.end&&r2.end==both.b.size(),"one view record boundary continuation");need(r2.payloads.empty()&&r2.declarations.empty()&&same(r2.cache,r1.cache),"backref later record no extra payload/cache");need(r2.references[0].object_id==6021&&r2.references[2].object_id==6021,"same global object ID continued");}
void accumulated_append_cap(){auto c=prefix();auto f=many(128);for(unsigned i=0;i<4;++i){auto r=read(f,c);need(r.payloads.size()==128&&r.declarations.size()==128,"each record128 objects");c=r.cache;}need(c.size()==7044,"four records accumulate1024 class/object entries");auto before=c;need(rejected([&]{read(first(),c);}),"new append beyond persistent1024 rejected");need(same(c,before)&&same(read(null_record(),c).cache,c),"full cache still accepts null-only record without mutation");}
void opaque_is_not_fsa(){auto c=prefix();for(uint32_t id:{1u,2u,0x80000001u}){auto f=null_record();f.b.erase(f.b.begin()+7,f.b.begin()+9);Bytes t;tag(t,id);f.b.insert(f.b.begin()+7,t.begin(),t.end());need(rejected([&]{read(f,c);}),"opaque Class/Object never inferred as FSA");}c.push_back({FigureFsaCacheKind::Class,O,0,0});need(rejected([&]{read(null_record(),c);}),"opaque appended outside original prefix rejected");}
void unknown_diag(){auto f=unknown();auto c=prefix();try{read(f,c);need(false,"unknown class must stop");}catch(const FigureFsaPrefixReadFailure&e){need(e.unknown_class&&e.tag_offset==12&&e.name_offset==18&&e.name_bytes==14&&e.schema==0&&e.attempted_offset==32,"self-authored declaration diagnostics exact; stop before poison payload");need(f.tag_offset==e.tag_offset&&f.unknown_end==e.attempted_offset,"fixture-derived offsets, not actual data");}need(c.size()==6020,"unknown class not published into caller cache");}
void late_unknown_atomic(){auto c=prefix(),before=c;auto output=read(first(),c);const auto saved=fingerprint(output);auto f=unknown(true);bool caught=false;try{output=read(f,c);}catch(const FigureFsaPrefixReadFailure&e){caught=true;need(e.unknown_class&&e.attempted_offset==f.unknown_end,"late unknown after known local object reports own stop");}need(caught&&fingerprint(output)==saved&&same(c,before),"late unknown doesn't publish known partial cache or caller output");f=first();f.b.pop_back();caught=false;try{output=read(f,c);}catch(const FigureFsaPrefixReadFailure&e){caught=true;need(!e.unknown_class,"truncation distinct from unknown declaration");}need(caught&&fingerprint(output)==saved&&same(c,before),"late truncation atomic");}
void initial_invalid_relations(){auto c=prefix();c[2].class_id=2;need(rejected([&]{read(null_record(),c);}),"Object cannot refer to Object as its class");c=prefix();c[2].class_id=6020;need(rejected([&]{read(null_record(),c);}),"out of range original class relation rejected");c=prefix();c[1].kind=FigureFsaCacheKind::Null;need(rejected([&]{read(null_record(),c);}),"only cache0 may be NULL");}
std::string escape(const std::string&s){std::string o;for(char c:s){if(c=='"'||c=='\\')o+='\\';o+=c;}return o;}
int main(){const std::vector<std::pair<const char*,std::function<void()>>> tests{
 {"initial_cache_appended1024_1025_must_be_checked",initial_append_budget},
 {"prefix_all_fields_preserved_and_global6020_6021",prefix_copy_ids},
 {"same_view_next_record_reuses_global_cache",continued_record},
 {"persistent_append_budget_four_records_then_fifth",accumulated_append_cap},
 {"opaque_prefix_does_not_become_fsa",opaque_is_not_fsa},
 {"self_authored_unknown_name_diagnostic_no_payload",unknown_diag},
 {"late_unknown_and_truncation_preserve_full_output",late_unknown_atomic},
 {"initial_object_class_relation_checks",initial_invalid_relations}};
 size_t passed=0;std::cout<<"{\"checks\":[";for(size_t i=0;i<tests.size();++i){if(i)std::cout<<',';std::string error;try{tests[i].second();++passed;}catch(const std::exception&e){error=e.what();}std::cout<<"{\"id\":\""<<tests[i].first<<"\",\"passed\":"<<(error.empty()?"true":"false");if(!error.empty())std::cout<<",\"error\":\""<<escape(error)<<'"';std::cout<<'}';}std::cout<<"],\"passed\":"<<passed<<",\"failed\":"<<tests.size()-passed<<"}\n";return passed==tests.size()?0:1;}
