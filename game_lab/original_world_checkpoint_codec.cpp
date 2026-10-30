// SPDX-License-Identifier: MIT
#ifndef ORIGINAL_WORLD_CODEC_IMPLEMENTATION
#define ORIGINAL_WORLD_CODEC_IMPLEMENTATION
#include "original_world_checkpoint_codec.hpp"
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <type_traits>
namespace game_lab { namespace original_world_codec_detail {
static_assert(sizeof(int)==4&&sizeof(unsigned)==4&&sizeof(size_t)==8&&sizeof(double)==8,"v1 wire requires explicit 32/64-bit scalar widths");
struct Failure:std::runtime_error{using std::runtime_error::runtime_error;};
template<class T>struct Optional:std::false_type{};template<class T>struct Optional<std::optional<T>>:std::true_type{using element=T;};
template<class T>struct Vector:std::false_type{};template<class T,class A>struct Vector<std::vector<T,A>>:std::true_type{using element=T;};
template<class T>struct Map:std::false_type{};template<class K,class V,class C,class A>struct Map<std::map<K,V,C,A>>:std::true_type{using key=K;using element=V;};
template<class T>struct Set:std::false_type{};template<class V,class C,class A>struct Set<std::set<V,C,A>>:std::true_type{using element=V;};
template<class T>struct Array:std::false_type{};template<class T,size_t N>struct Array<std::array<T,N>>:std::true_type{};
template<class T>struct Pair:std::false_type{};template<class A,class B>struct Pair<std::pair<A,B>>:std::true_type{};
template<class T>T fresh(){return T{};}
template<>inline OriginalBuildingUpdateContext fresh(){return {OriginalBuildingUpdateScope::AdmittedTicks,true};}
template<>inline OriginalCombinedBuilding fresh(){return {{},{OriginalBuildingUpdateScope::AdmittedTicks,true}};}
template<class T>constexpr uint8_t kind(){if constexpr(std::is_same_v<T,bool>)return 1;else if constexpr(std::is_integral_v<T>)return uint8_t(2+2*sizeof(T)+std::is_signed_v<T>);else if constexpr(std::is_enum_v<T>)return 30;else if constexpr(std::is_floating_point_v<T>)return 31;else if constexpr(Optional<T>::value)return 40;else if constexpr(Vector<T>::value)return 41;else if constexpr(Map<T>::value)return 42;else if constexpr(Array<T>::value)return 43;else if constexpr(Pair<T>::value)return 44;else if constexpr(std::is_same_v<T,std::string>)return 45;else if constexpr(std::is_same_v<T,const char*>)return 46;else if constexpr(Set<T>::value)return 48;else return 47;}
template<class T>bool enumValid(T v){return int64_t(v)>=0;}
template<>inline bool enumValid(OriginalFigureClass v){return unsigned(v)<=2;}
template<>inline bool enumValid(OriginalBuildingUpdateScope v){return unsigned(v)<=2;}
template<>inline bool enumValid(OriginalFigureImageStatus v){return unsigned(v)<=5;}
template<>inline bool enumValid(AppCityTributeState v){return unsigned(v)<=4;}
template<>inline bool enumValid(OriginalPostFigurePassStatus v){switch(v){case OriginalPostFigurePassStatus::Ok:case OriginalPostFigurePassStatus::SkippedGate:case OriginalPostFigurePassStatus::RefusedMissingSessionInput:case OriginalPostFigurePassStatus::RefusedNullRegistrySlot:case OriginalPostFigurePassStatus::RefusedUnported5653C0:case OriginalPostFigurePassStatus::RefusedUnported56C880:case OriginalPostFigurePassStatus::RefusedUnported56D170:case OriginalPostFigurePassStatus::RefusedUnported564EA0:case OriginalPostFigurePassStatus::Refused564EA0Contract:case OriginalPostFigurePassStatus::RefusedUnported567610:case OriginalPostFigurePassStatus::Refused567610ZeroAdvance:case OriginalPostFigurePassStatus::Refused56C880Containers:case OriginalPostFigurePassStatus::Refused56C880Contract:case OriginalPostFigurePassStatus::Refused56D170Containers:case OriginalPostFigurePassStatus::Refused56D170Contract:return true;}return false;}
inline const std::array<const char*,279> symbols{{"","00000000","00000001","00000100","00000101","000001m0","00000212","00000213","00000312","00001mmm","00010000","00010001","00010100","00010101","00010212","00021112","00021200","00021201","00021212","00021300","00031200","000mmm10","0011mmm0","001mmm00","001mmmm0","001mmmmm","00mmm100","00mmmm10","01000000","01000001","01000100","01000101","01000212","01010000","01010001","01010100","01010101","01010212","01021200","01021201","01021212","01033333","01122220","01133333","02020202","02020212","02021202","02021212","02033333","02101012","02101112","02111012","02111112","02111200","02120000","02120001","02120100","02120101","02120202","02120212","02121200","02121201","02121202","02121212","02130000","02211110","03120000","03230333","03330323","03330414","03333301","03333302","03333311","04140333","0mmm1000","0mmmmm10","0x1-road","0x100-special","0x1000-irrigation","0x10000-monument","0x101000-twin","0x2-land","0x20-general","0x20000-forest","0x2000000-delegated","0x2000000-feature","0x2000000-feature-layout","0x4-shore","0x4-shore-edge","0x4-shore-river","0x4-shore-shore-ring","0x4-shore-water-fringe","0x4-shore-water-ring","0x40-road","0x40-road-material-0","0x40-road-material-2","0x40000-canal","0x53FCB3 shore row count","0x80-farm","0x80-farm-block","0x80-farm-block-2x2","0x80-farm-block-3x3","0x80-farm-block-4x4","0x80-farm-block-fallback","0x80-farm-neighbours","0x80-farm-river","0x80-farm-variant","0x800-water","0x80000000-flood","10000mmm","1000mmmm","10022111","100m0002","100mmmmm","10101010","10101011","10101110","10101111","10101202","10111010","10111011","10111110","10111111","10111202","10120210","10120211","10133333","11033333","11100221","11101010","11101011","11101110","11101111","11101202","11111010","11111011","11111110","11111111","11111202","11112222","11120002","11120210","11120211","11133333","11144144","11222211","11233333","11300000","12000002","12000003","12000102","12000211","12000212","12010002","12010102","12010212","12020202","12020212","12021010","12021011","12021110","12021111","12021202","12021212","12120002","12120102","12120202","12120212","12121202","12121212","12133333","12222001","12233333","13000002","13131313","13333300","13333301","13333310","13333311","13333312","13333321","13333322","14033304","14414411","14441444","1mmm0000","1mmmm000","1mmmmm00","20011222","21111002","21133333","21233333","22111100","22111122","22133333","22200112","22221111","22222222","22233233","22233333","23033303","23232323","23323322","23332333","23333311","23333312","23333321","23333322","31313131","31313133","31313331","31333131","32323232","32332223","32333233","33010333","33011333","33020333","33032303","33041403","33101333","33110333","33111333","33112333","33121333","33122333","33211333","33212333","33221333","33222332","33222333","33233323","33313131","33323332","33330103","33330113","33330203","33331013","33331103","33331113","33331123","33331213","33331223","33332113","33332123","33332213","33332223","33333001","41441114","41444144","44111441","44144414","44414441","DATA/China_General.sg3","FUN_00473170 row count","building-seeded","chain-declined","elevation-block","elevation-block-declined","elevation-direct","elevation-road-material","m0001mmm","m100000m","mm00001m","mm001mmm","mm10000m","mm1000mm","mm100mmm","mmm0001m","mmmm001m","mmmm1000","mmmm100m","outside-ground-traversal","plain-land","plain-land-block-2x2","plain-land-block-3x3","plain-land-block-4x4","saved-owner-road-declined","DATA/China_Terrain.sg3"}};
#include "original_world_checkpoint_fields.inc"
struct Remainder {OriginalFigureUpdateContext& f;};
template<class V>void worldFields(V& v,Remainder& x){v.record(55);v.field(2,x.f.view);v.field(3,x.f.modelSpeeds);v.field(4,x.f.services);v.field(5,x.f.passability);v.field(6,x.f.hunterLinks);v.field(7,x.f.buildings);v.field(8,x.f.models);v.field(9,x.f.cellHeads);v.field(10,x.f.terrain);v.field(11,x.f.cropBase);v.field(12,x.f.movementMasks);v.field(13,x.f.factionModels);v.field(14,x.f.houseCapacities);v.field(15,x.f.houseWatchFloors);v.field(16,x.f.houseMarketModels);v.field(17,x.f.watchtowerReduction);v.field(18,x.f.inspectorReduction);v.field(19,x.f.inspectorMaximum);v.field(20,x.f.captureInspectionVisits);v.field(21,x.f.inspectionVisits);v.field(22,x.f.watchMaximum);v.field(23,x.f.crops);v.field(24,x.f.elevation);v.field(25,x.f.buildingOwners);v.field(26,x.f.feature);v.field(27,x.f.randomByte);v.field(28,x.f.activeSide);v.field(29,x.f.goods);v.field(30,x.f.buildingEmployment);v.field(31,x.f.festivalVenue);v.field(32,x.f.entertainmentAvailable);v.field(33,x.f.waterOverride);v.field(34,x.f.wellReturnObservedHomePolicy);v.field(35,x.f.freeSlots);v.field(36,x.f.highestLiveSlot);v.field(37,x.f.crtRandom);v.field(38,x.f.crtRandomCalls);v.field(39,x.f.animationFlags);v.field(40,x.f.allocationEvents);v.field(41,x.f.allocatedSlots);v.field(42,x.f.allocationCount);v.field(43,x.f.buildingAllocationCount);v.field(44,x.f.primaryImageInputs);v.field(45,x.f.roadblockGate);v.field(46,x.f.soundEffectsEnabled);v.field(47,x.f.empireTraders);v.field(48,x.f.buildingEditMode);v.field(49,x.f.visualEffectCount);v.field(50,x.f.buildingBurning);v.field(51,x.f.farmerTasks);v.field(52,x.f.speechCounters);v.field(53,x.f.speechCounts);v.field(54,x.f.roadNetworks);v.field(55,x.f.workerRoadExits);v.field(56,x.f.migrationRoadEndpoints);}
struct Writer {
 std::string bytes;size_t depth=0;
 void raw(uint64_t n,size_t width){if(width>originalWorldFileLimit-bytes.size())throw Failure("world_checkpoint_budget");for(size_t i=0;i<width;++i)bytes.push_back(char(n>>(8*i)));}
 void patch(size_t at,uint64_t n,size_t width){for(size_t i=0;i<width;++i)bytes.at(at+i)=char(n>>(8*i));}
 void record(uint16_t count){raw(count,2);}
 template<class T>void field(uint16_t id,T& x){raw(id,2);raw(kind<T>(),1);const auto offset=bytes.size();raw(0,8);const auto begin=bytes.size();value(x);patch(offset,bytes.size()-begin,8);}
 template<class T>void value(const T& x){
  if(++depth>32)throw Failure("world_checkpoint_depth");
  if constexpr(std::is_same_v<T,bool>)raw(x?1:0,1);
  else if constexpr(std::is_integral_v<T>)raw(uint64_t(x),sizeof(T));
  else if constexpr(std::is_enum_v<T>){if(!enumValid(x))throw Failure("world_checkpoint_enum");raw(uint32_t(x),4);}
  else if constexpr(std::is_same_v<T,double>){if(!std::isfinite(x))throw Failure("world_checkpoint_float");uint64_t n;std::memcpy(&n,&x,8);raw(n,8);}
  else if constexpr(std::is_same_v<T,std::string>){if(x.size()>1048576||x.find('\0')!=std::string::npos)throw Failure("world_checkpoint_string");raw(x.size(),4);if(x.size()>originalWorldFileLimit-bytes.size())throw Failure("world_checkpoint_budget");bytes+=x;}
  else if constexpr(std::is_same_v<T,const char*>){if(!x)throw Failure("world_checkpoint_symbol");size_t i=0;for(;i<symbols.size();++i)if(std::strcmp(x,symbols[i])==0)break;if(i==symbols.size())throw Failure("world_checkpoint_symbol");raw(i,2);}
  else if constexpr(Optional<T>::value){raw(bool(x),1);if(x)value(*x);}
  else if constexpr(Vector<T>::value){if(x.size()>500000)throw Failure("world_checkpoint_vector");raw(x.size(),4);for(const auto& item:x){if constexpr(std::is_same_v<typename Vector<T>::element,bool>)value(bool(item));else value(item);}}
  else if constexpr(Map<T>::value||Set<T>::value){if(x.size()>4000)throw Failure("world_checkpoint_map");raw(x.size(),4);for(const auto& item:x){if constexpr(Map<T>::value){value(item.first);value(item.second);}else value(item);}}
  else if constexpr(Array<T>::value){for(const auto& item:x)value(item);}
  else if constexpr(Pair<T>::value){value(x.first);value(x.second);}
  else {auto& mutableView=const_cast<T&>(x);worldFields(*this,mutableView);}
  --depth;
 }
};
struct Reader {
 std::string_view bytes;size_t pos=0,limit=0,allocated=0,depth=0;
 explicit Reader(std::string_view b):bytes(b),limit(b.size()){}
 uint64_t raw(size_t width){if(width>limit-pos)throw Failure("world_checkpoint_truncated");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(uint8_t(bytes[pos++]))<<(8*i);return n;}
 void charge(size_t n,size_t unit){if(unit&&n>(originalWorldAllocationLimit-std::min(allocated,originalWorldAllocationLimit))/unit)throw Failure("world_checkpoint_allocation_budget");allocated+=n*unit;}
 void record(uint16_t count){if(raw(2)!=count)throw Failure("world_checkpoint_field_count");}
 template<class T>void field(uint16_t id,T& x){if(raw(2)!=id||raw(1)!=kind<T>())throw Failure("world_checkpoint_field_schema");const auto n=raw(8);if(n>limit-pos)throw Failure("world_checkpoint_field_length");const auto end=pos+size_t(n),old=limit;limit=end;value(x);if(pos!=end)throw Failure("world_checkpoint_field_trailing");limit=old;}
 template<class T>void value(T& x){
  if(++depth>32)throw Failure("world_checkpoint_depth");
  if constexpr(std::is_same_v<T,bool>){const auto n=raw(1);if(n>1)throw Failure("world_checkpoint_bool");x=bool(n);}
  else if constexpr(std::is_integral_v<T>){const uint64_t n=raw(sizeof(T));if constexpr(std::is_signed_v<T>){using U=std::make_unsigned_t<T>;const U bits=U(n);std::memcpy(&x,&bits,sizeof(T));}else x=T(n);}
  else if constexpr(std::is_enum_v<T>){x=static_cast<T>(uint32_t(raw(4)));if(!enumValid(x))throw Failure("world_checkpoint_enum");}
  else if constexpr(std::is_same_v<T,double>){const auto n=raw(8);std::memcpy(&x,&n,8);if(!std::isfinite(x))throw Failure("world_checkpoint_float");}
  else if constexpr(std::is_same_v<T,std::string>){const auto n=raw(4);if(n>1048576||n>limit-pos)throw Failure("world_checkpoint_string");charge(size_t(n),1);x=std::string(bytes.substr(pos,size_t(n)));pos+=size_t(n);if(x.find('\0')!=std::string::npos)throw Failure("world_checkpoint_string");}
  else if constexpr(std::is_same_v<T,const char*>){const auto n=raw(2);if(n>=symbols.size())throw Failure("world_checkpoint_symbol");x=symbols[size_t(n)];}
  else if constexpr(Optional<T>::value){const auto has=raw(1);if(has>1)throw Failure("world_checkpoint_knownness");if(has){auto v=fresh<typename Optional<T>::element>();value(v);x=std::move(v);}else x.reset();}
  else if constexpr(Vector<T>::value){const auto n=raw(4);if(n>500000||n>limit-pos)throw Failure("world_checkpoint_vector");charge(size_t(n),sizeof(typename Vector<T>::element));x.clear();x.reserve(size_t(n));for(size_t i=0;i<n;++i){auto v=fresh<typename Vector<T>::element>();value(v);x.push_back(std::move(v));}}
  else if constexpr(Map<T>::value){const auto n=raw(4);if(n>4000||n>(limit-pos)/2)throw Failure("world_checkpoint_map");charge(size_t(n),sizeof(typename T::value_type)+64);x.clear();std::optional<typename Map<T>::key> previous;for(size_t i=0;i<n;++i){auto key=fresh<typename Map<T>::key>();auto v=fresh<typename Map<T>::element>();value(key);value(v);if(previous&&!(*previous<key))throw Failure("world_checkpoint_map_order");previous=key;x.emplace(std::move(key),std::move(v));}}
  else if constexpr(Set<T>::value){const auto n=raw(4);if(n>4000||n>limit-pos)throw Failure("world_checkpoint_set");charge(size_t(n),sizeof(typename Set<T>::element)+64);x.clear();std::optional<typename Set<T>::element> previous;for(size_t i=0;i<n;++i){auto v=fresh<typename Set<T>::element>();value(v);if(previous&&!(*previous<v))throw Failure("world_checkpoint_set_order");previous=v;x.insert(v);}}
  else if constexpr(Array<T>::value){for(auto& item:x)value(item);}
  else if constexpr(Pair<T>::value){value(x.first);value(x.second);}
  else worldFields(*this,x);
  --depth;
 }
};
template<class V>void section(V& v,unsigned id,OriginalWorldSnapshot& s){switch(id){
 case 1:v.record(1);v.field(1,s.source);break;case 2:v.record(1);v.field(1,s.policy);break;
 case 3:v.record(2);v.field(1,s.city.phase);v.field(2,s.city.process);break;
 case 4:v.record(2);v.field(1,s.city.figures.figures);v.field(2,s.marketBases);break;
 case 5:{Remainder remainder{s.city.figures};worldFields(v,remainder);break;}
 case 6:v.record(1);v.field(1,s.city.buildings);break;case 7:v.record(1);v.field(1,s.city.buildingContext);break;
 case 8:v.record(2);v.field(1,s.city.annualOffers);v.field(2,s.city.scenarioMessages);break;
 case 9:v.record(3);v.field(1,s.city.postFigureRegistry);v.field(2,s.city.postFigureResult);v.field(3,s.city.rebuiltMovementTerrain);break;
 case 10:v.record(1);v.field(1,s.terrain);break;case 11:v.record(1);v.field(1,s.scene);break;
 case 12:v.record(1);v.field(1,s.continuation);break;case 13:v.record(2);v.field(1,s.ledger);v.field(2,s.notifications);break;
 case 14:v.record(1);v.field(1,s.camera);break;default:throw Failure("world_checkpoint_section");}}
inline std::string unchecked(const OriginalWorldSnapshot& input){
 std::array<std::string,14> payloads;auto& s=const_cast<OriginalWorldSnapshot&>(input);size_t total=64+14*56+40;
 for(unsigned id=1;id<=14;++id){Writer w;section(w,id,s);payloads[id-1]=std::move(w.bytes);if(payloads[id-1].size()>originalWorldFileLimit-total)throw Failure("world_checkpoint_budget");total+=payloads[id-1].size();}
 Writer out;out.bytes.assign("EMPEROR-WORLD\0\0\0",16);out.raw(1,2);out.raw(64,2);out.raw(0,4);out.raw(total,8);out.raw(s.commitRevision,8);out.raw(14,4);out.raw(0,4);out.raw(784,8);out.raw(31,8);size_t offset=848;
 for(unsigned id=1;id<=14;++id){const auto& body=payloads[id-1];out.raw(id,2);out.raw(1,2);out.raw(0,4);out.raw(offset,8);out.raw(body.size(),8);const auto hash=originalWorldDigest(body.data(),body.size());out.bytes.append(reinterpret_cast<const char*>(hash.data()),hash.size());offset+=body.size();}
 for(const auto& body:payloads)out.bytes+=body;const auto hash=originalWorldDigest(out.bytes.data(),out.bytes.size());out.bytes+="ENDWORLD";out.bytes.append(reinterpret_cast<const char*>(hash.data()),hash.size());return out.bytes;
}
}}
namespace game_lab {
inline std::string originalWorldCheckpointValueIssue(const OriginalWorldSnapshot& s){
 if(auto error=originalWorldSnapshotIssue(s);!error.empty())return error;
 if(s.continuation.initialFigures){original_world_codec_detail::Writer a,b;a.value(s.continuation.initialFigures->figures);b.value(s.city.figures.figures);if(a.bytes!=b.bytes)return "world_snapshot_initial_pool_alias";}
 return {};
}
inline std::optional<std::string> encodeOriginalWorldCheckpoint(const OriginalWorldSnapshot& s,std::string& error){try{error=originalWorldCheckpointValueIssue(s);if(!error.empty())return std::nullopt;auto bytes=original_world_codec_detail::unchecked(s);error.clear();return bytes;}catch(const std::exception& e){error=e.what();return std::nullopt;}}
inline std::optional<OriginalWorldSnapshot> decodeOriginalWorldCheckpoint(std::string_view bytes,std::string& error){using namespace original_world_codec_detail;try{
 if(bytes.size()<888||bytes.size()>originalWorldFileLimit||bytes.substr(0,16)!=std::string_view("EMPEROR-WORLD\0\0\0",16)||bytes.substr(bytes.size()-40,8)!="ENDWORLD")throw Failure("world_checkpoint_framing");
 const auto hash=originalWorldDigest(bytes.data(),bytes.size()-40);if(std::memcmp(hash.data(),bytes.data()+bytes.size()-32,32))throw Failure("world_checkpoint_checksum");
 Reader header(bytes);header.pos=16;if(header.raw(2)!=1||header.raw(2)!=64||header.raw(4)!=0||header.raw(8)!=bytes.size())throw Failure("world_checkpoint_header");
 OriginalWorldSnapshot s;s.commitRevision=header.raw(8);if(header.raw(4)!=14||header.raw(4)!=0||header.raw(8)!=784||header.raw(8)!=31)throw Failure("world_checkpoint_features");
 size_t expected=848,allocated=0;
 for(unsigned id=1;id<=14;++id){if(header.raw(2)!=id||header.raw(2)!=1||header.raw(4)!=0)throw Failure("world_checkpoint_directory");const auto offset=header.raw(8),length=header.raw(8);if(offset!=expected||length>bytes.size()-40-expected)throw Failure("world_checkpoint_section_bounds");OriginalWorldDigest sectionHash;for(auto& b:sectionHash)b=uint8_t(header.raw(1));
  const auto body=bytes.substr(expected,size_t(length));if(originalWorldDigest(body.data(),body.size())!=sectionHash)throw Failure("world_checkpoint_section_checksum");Reader reader(body);reader.allocated=allocated;section(reader,id,s);if(reader.pos!=reader.limit)throw Failure("world_checkpoint_section_trailing");allocated=reader.allocated;expected+=size_t(length);}
 if(expected!=bytes.size()-40)throw Failure("world_checkpoint_trailing");error=originalWorldCheckpointValueIssue(s);if(!error.empty())return std::nullopt;return s;
 }catch(const std::exception& e){error=e.what();return std::nullopt;}}
inline bool originalWorldPersistentEqual(const OriginalWorldSnapshot& a,const OriginalWorldSnapshot& b,std::string& error){const auto first=encodeOriginalWorldCheckpoint(a,error);if(!first)return false;const auto second=encodeOriginalWorldCheckpoint(b,error);if(!second)return false;if(*first!=*second){error="world_checkpoint_persistent_difference";return false;}error.clear();return true;}
}
#endif
