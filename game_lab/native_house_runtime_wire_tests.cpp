// SPDX-License-Identifier: MIT
// Authored wire invariants, separate from strict-engine/source acceptance.
#include "native_session.hpp"
#include <iostream>
using namespace game_lab;
NativeSessionSeed fixture(){NativeSessionSeed s;s.source="test.sav";s.sourceRevision=1;s.modelRevision=2;s.side=8;s.initialFunds=100;s.roadCost=2;s.houseCost=15;s.houseCapacity=7;s.buildable.assign(64,1);s.sourceRoad.assign(64,0);return s;}
NativeSessionSeed legacyGoldenSeed(){
 auto seed=fixture();seed.source="golden.sav";seed.initialFunds=1000;seed.wellCost=25;seed.hunterCost=32;
 seed.laborPercent=50;seed.hunterWorkers=2;seed.wellWorkers=4;seed.laborPriority.fill(3);return seed;
}

struct NativeRuntimeValidationFixture {NativeSessionSeed seed;NativeSessionState state;};
NativeRuntimeValidationFixture houseRuntimeValidationFixture(){
 // Entirely synthetic admission to the state validator, not to source loading
 // or the strict phase engine. An empty live-walker pool has every slot free.
 auto seed=legacyGoldenSeed();seed.houseCapacities={{0,7},{2,14},{4,21}};
 seed.sourceClock=NativeHouseClockSeed{};seed.sourceClock->phase=7;seed.sourceClock->cycle=3;
 seed.sourceClock->monthPart=2;seed.sourceClock->month=4;seed.waterOverride=false;
 for(int model=0;model<15;++model){std::array<int32_t,24> row{};
  row[17]=model==2?14:model==4?21:7;seed.houseModels.emplace(model,row);}
 auto state=startNativeSession(seed,8);state.paused=false;state.revision=4;state.nextId=5;
 state.objects={{1,NativeObjectKind::Road,1,2,0},{2,NativeObjectKind::House,2,2,0},
                {3,NativeObjectKind::Well,2,4,0},{4,NativeObjectKind::Hunter,4,4,0}};
 state.funds-=seed.roadCost+seed.houseCost+seed.wellCost+seed.hunterCost;
 auto& house=state.objects[1];house.house.emplace();house.house->servicesKnown=true;house.house->houseWater=0;
 house.automation.emplace();house.automation->cachedWater=0;house.automation->wellId=3;
 state.objects[2].well.emplace();
 state.houseSchedule.emplace();auto& clock=*state.houseSchedule;
 clock.phase=7;clock.cycle=3;clock.monthPart=2;clock.month=4;clock.origin=NativeHouseClockOrigin::SourceSaved;
 state.buildingHookCountdown=0;
 auto& figures=state.figureRuntime.emplace();figures.cellHeads.assign(51984,0);figures.highestFigureSlot=0;
 for(uint16_t slot=1;slot<2000;++slot)figures.freeFigureSlots.push_back(slot);
 auto& services=figures.services;services.directions.assign(500000,0);services.serviceGrid.assign(51984,0);
 services.randomFirst=0xfedcba98u;services.randomSecond=0x89abcdefu;
 services.randomRing.emplace();for(size_t n=0;n<100;++n)services.randomRing->at(n)=uint32_t(n);
 services.randomRing->at(99)=127;services.randomCursor=99;services.randomPrevious=127;
 services.fireCount=0;services.criminalCount=0;services.allocationSerial=0;
 return {std::move(seed),std::move(state)};
}

struct Check{std::string name;bool passed;};std::vector<Check> results;
void check(const char* name,bool passed){results.push_back({name,passed});}
bool reads(const NativeSessionSeed& seed,const std::string& wire){std::string e;return decodeNativeSession(seed,wire,e).has_value();}
std::string replaceOne(std::string s,const std::string& before,const std::string& after){const auto at=s.find(before);if(at==std::string::npos)throw std::runtime_error("probe mutation anchor missing: "+before);s.replace(at,before.size(),after);return s;}
void stringInvariant(const NativeSessionSeed& seed,const NativeSessionState& state,const char* name){
 const auto v=validateNativeSession(seed,state);const auto wire=encodeNativeSession(seed,state);std::string error;
 const auto d=decodeNativeSession(seed,wire,error);check(name,!v.empty()||wire.empty()||(d&&encodeNativeSession(seed,*d)==wire));
 std::cout<<"{\"observation\":"<<std::quoted(name)<<",\"validate_error\":"<<std::quoted(v)<<",\"encoded_bytes\":"<<wire.size()<<",\"decoded\":"<<(d?"true":"false")<<",\"decode_error\":"<<std::quoted(error)<<"}\n";
}
int main(){try{
 auto fixture=houseRuntimeValidationFixture();const auto& seed=fixture.seed;const auto& state=fixture.state;
 check("v8_base_validator",validateNativeSession(seed,state).empty());const auto bytes=encodeNativeSession(seed,state);std::string error;
 const auto restored=decodeNativeSession(seed,bytes,error);check("v8_base_exact_roundtrip",!bytes.empty()&&restored&&encodeNativeSession(seed,*restored)==bytes);
 auto rich=state;rich.houseSchedule->phase=50;rich.houseSchedule->cycle=UINT32_MAX;rich.houseSchedule->monthPart=15;rich.houseSchedule->month=11;rich.houseSchedule->cycleWrapTailSkipped=true;
 auto& fr=*rich.figureRuntime;fr.services.formationImpacts[0]={INT16_MIN,INT16_MAX};fr.services.routeOwners[999]=INT16_MIN;
 fr.services.directions.back()=UINT8_MAX;fr.services.randomFirst=UINT32_MAX;fr.services.randomSecond=0x80000000u;
 fr.services.serviceGrid.back()=UINT32_MAX;fr.speechCounters[0]=INT32_MIN;fr.speechCounts.back()=UINT32_MAX;fr.speechChoices[7]=INT16_MIN;
 rich.objects[1].house->houseWater=2;rich.objects[1].house->remaining[60]=255;rich.objects[1].house->imageWord20=INT16_MIN;rich.objects[1].house->imageHouseByte3c=255;
 auto& automation=*rich.objects[1].automation;automation.cachedWater=2;automation.delay=UINT32_MAX;automation.routeCursor=499999;automation.held=true;automation.holdReason="authored \"quoted\" \\ reason\nnext line";
 rich.objects[2].well->spawnCounter=UINT8_MAX;rich.objects[2].well->roamDirection=UINT8_MAX;
 rich.heldHouseReasons[2]="authored house reason";rich.heldWalkerReasons[1]="authored walker reason";
 rich.strictRecords[2]="authored opaque strict payload\n\"quote\" \\ slash";rich.strictContext="authored context\n";rich.runtimePolicies={"authored-policy-a","authored-policy-b"};
 check("v8_rich_dto_validator",validateNativeSession(seed,rich).empty());const auto richBytes=encodeNativeSession(seed,rich);const auto r=decodeNativeSession(seed,richBytes,error);
 check("v8_rich_exact_roundtrip",!richBytes.empty()&&r&&encodeNativeSession(seed,*r)==richBytes);
 check("v8_full_rng_and_history",r&&r->figureRuntime->services.randomFirst==UINT32_MAX&&r->figureRuntime->services.randomSecond==0x80000000u&&r->figureRuntime->services.randomRing==fr.services.randomRing&&r->figureRuntime->services.randomCursor==99&&r->figureRuntime->services.randomPrevious==127);
 check("v8_signed_and_unsigned_fields",r&&r->figureRuntime->services.formationImpacts[0].first==INT16_MIN&&r->figureRuntime->services.routeOwners[999]==INT16_MIN&&r->figureRuntime->speechCounters[0]==INT32_MIN&&r->figureRuntime->speechCounts.back()==UINT32_MAX&&r->figureRuntime->services.serviceGrid.back()==UINT32_MAX&&r->figureRuntime->speechChoices.at(7)==INT16_MIN);
 check("v8_new_persistence_fields",r&&r->heldHouseReasons==rich.heldHouseReasons&&r->heldWalkerReasons==rich.heldWalkerReasons&&r->strictRecords==rich.strictRecords&&r->strictContext==rich.strictContext&&r->runtimePolicies==rich.runtimePolicies&&r->objects[1].automation->holdReason==automation.holdReason&&r->objects[1].automation->delay==UINT32_MAX&&r->houseSchedule->cycleWrapTailSkipped);
 check("v8_phase_narrowing_refused",!reads(seed,replaceOne(richBytes,"phase 50 ","phase 256 ")));
 check("v8_rng_uint32_overflow_refused",!reads(seed,replaceOne(richBytes,"randomFirst 4294967295 ","randomFirst 4294967296 ")));
 check("v8_bool_out_of_domain_refused",!reads(seed,replaceOne(richBytes,"tailSkipped 1 ","tailSkipped 2 ")));
 check("v8_vector_count_cap_refused",!reads(seed,replaceOne(richBytes,"cellHeads 51984 ","cellHeads 500001 ")));
 {std::istringstream text("2 1 \"a\" 1 \"b\"");native_runtime_wire::Reader reader{text};std::map<uint64_t,std::string> value;reader.value(value);check("runtime_duplicate_map_key_refused",!reader.good);}
 {std::istringstream text("4001");native_runtime_wire::Reader reader{text};std::map<uint64_t,std::string> value;reader.value(value);check("runtime_map_allocation_cap_refused",!reader.good&&value.empty());}
 {std::istringstream text("500001");native_runtime_wire::Reader reader{text};std::vector<uint8_t> value;reader.value(value);check("runtime_vector_allocation_cap_refused",!reader.good&&value.empty());}
 {auto bad=state;bad.objects[1].automation->held=true;bad.objects[1].automation->holdReason=std::string("a\0b",3);stringInvariant(seed,bad,"nul_automation_reason_must_not_self_break_roundtrip");}
 {auto bad=state;bad.heldHouseReasons[2]=std::string("a\0b",3);stringInvariant(seed,bad,"nul_held_house_reason_must_not_self_break_roundtrip");}
 {auto bad=state;bad.heldWalkerReasons[1]=std::string("a\0b",3);stringInvariant(seed,bad,"nul_held_walker_reason_must_not_self_break_roundtrip");}
 {auto bad=state;bad.objects[1].automation->held=true;bad.objects[1].automation->holdReason=std::string(262145,'a');stringInvariant(seed,bad,"oversized_automation_reason_must_not_self_break_roundtrip");}
 const auto oldSeed=legacyGoldenSeed();
 {const std::string wire=R"ENCITY(ENCITY 1 original-data-native-v1
"golden.sav" 1 2
8 1000 2 15 7
0 0 1 1 1000 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_1_v1",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 1 original-data-native-v1
"golden.sav" 1 2
8 1000 2 15 7
10 6 3 1 983 2
1 22 1 2 0
2 2 2 2 3
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_2_v1",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 2 original-data-native-v2
"golden.sav" 1 2
8 1000 2 15 7 25 4
0 0 1 1 1000 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_3_v2",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 2 original-data-native-v2
"golden.sav" 1 2
8 1000 2 15 7 25 4
12 6 5 1 956 4
1 22 1 2 0
2 2 2 2 3
3 72 2 4 0
4 22 1 4 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_4_v2",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 3 original-data-native-v3
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
0 0 1 1 1000 0
FOOD meat 4 100 1 0 0 0 0 0 0 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_5_v3",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 3 original-data-native-v3
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
14 6 7 1 922 6
FOOD meat 4 100 1 6 12 6 4 2 2 1
1 22 1 2 0
2 2 2 2 3
3 72 2 4 0
4 22 1 4 0
5 33 4 4 0
6 22 4 3 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_6_v3",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 4 original-data-native-v4
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
0 0 1 1 1000 0
FOOD meat 4 100 1 0 0 0 0 0 0 0 0
RETIRED 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_7_v4",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 4 original-data-native-v4
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
14 6 7 1 922 6
FOOD meat 4 100 1 6 12 6 4 2 2 1 0
1 22 1 2 0
2 2 2 2 3
3 72 2 4 0
4 22 1 4 0
5 33 4 4 0
6 22 4 3 0
RETIRED 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_8_v4",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 5 original-data-native-v5
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
0 0 1 1 1000 0
FOOD meat 4 100 1 0 0 0 0 0 0 0 0
TAX 4 1 0 0 0 0 0 0
RETIRED 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_9_v5",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 5 original-data-native-v5
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
14 6 7 1 923 6
FOOD meat 4 100 1 6 12 6 4 2 2 1 0
TAX 4 1 1 1 1 0 0 1
1 22 1 2 0
2 2 2 2 3
3 72 2 4 0
4 22 1 4 0
5 33 4 4 0
6 22 4 3 0
RETIRED 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_10_v5",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 6 original-data-native-v6
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
0 0 1 1 1000 0
FOOD meat 4 100 1 0 0 0 0 0 0 0 0
TAX 4 1 0 0 0 0 0 0
LABOR 50 2 4 3 3 3 3 3 3 3 3 3 3
RETIRED 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_11_v6",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {const std::string wire=R"ENCITY(ENCITY 6 original-data-native-v6
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
14 6 7 1 923 6
FOOD meat 4 100 1 0 2 2 2 2 2 1 0
TAX 4 1 1 1 1 0 0 1
LABOR 50 2 4 3 3 3 3 3 3 3 3 3 3
1 22 1 2 0
2 2 2 2 3
3 72 2 4 0
4 22 1 4 0
5 33 4 4 0
6 22 4 3 0
RETIRED 0
)ENCITY";std::string e;auto d=decodeNativeSession(oldSeed,wire,e);check("legacy_literal_12_v6",d&&encodeNativeSession(oldSeed,*d)==wire);}
 {auto v7Seed=oldSeed;v7Seed.houseCapacities={{0,7},{2,14},{4,21}};const std::string wire=R"ENCITY(ENCITY 7 original-data-native-v7-foundation
"golden.sav" 1 2
8 1000 2 15 7 25 4 32
1 0 2 1 985 1
FOOD meat 4 100 1 0 0 0 0 0 0 0 0
TAX 4 1 0 0 0 0 0 0
LABOR 50 2 4 3 3 3 3 3 3 3 3 3 3
HOUSEMODELS 3 0 7 2 14 4 21
HOUSECLOCK unknown
1 2 2 2 0
RETIRED 0
HOUSES 1
1 2 0 1 2 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 -1 3
)ENCITY";std::string e;auto d=decodeNativeSession(v7Seed,wire,e);check("v7_fixed_golden_unchanged",d&&d->rulesVersion==7&&!d->houseClock&&encodeNativeSession(v7Seed,*d)==wire);}
 check("default_latest_still_v6",nativeLatestRulesVersion==6);
 auto alternateSeed=seed;alternateSeed.houseModels.at(0)[0]=99;alternateSeed.waterOverride=true;alternateSeed.sourceClock->phase=9;
 std::cout<<"{\"observation\":\"v8_immutable_inputs_not_separately_bound\",\"accepted_with_same_revisions\":"<<(reads(alternateSeed,bytes)?"true":"false")<<"}\n";
 size_t passed=0;for(const auto& item:results){passed+=item.passed;std::cout<<"{\"case\":"<<std::quoted(item.name)<<",\"passed\":"<<(item.passed?"true":"false")<<"}\n";}
 std::cout<<"{\"passed\":"<<passed<<",\"total\":"<<results.size()<<",\"failed\":"<<results.size()-passed<<",\"scope\":\"synthetic_outer_wire_and_validator_only_no_runtime_bridge\"}\n";
 return passed==results.size()?0:1;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}}
