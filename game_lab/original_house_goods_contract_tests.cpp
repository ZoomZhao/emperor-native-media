// SPDX-License-Identifier: MIT
// C1 synthetic format/consumer-contract tests only. No original files/assets,
// original game, GUI or claim of independently proved original behavior.
#include "original_scene_timeline.hpp"
#include "original_house_goods_contract.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0,groups=0;
void need(bool ok,const std::string& why){++checks;if(!ok)throw std::runtime_error(why);}
void word(std::vector<uint8_t>& data,size_t at,uint16_t value){data.at(at)=uint8_t(value);data.at(at+1)=uint8_t(value>>8);}
void dword(std::vector<uint8_t>& data,size_t at,uint32_t value){for(unsigned n=0;n<4;++n)data.at(at+n)=uint8_t(value>>(8*n));}
struct Schema {std::vector<uint8_t> bytes;OriginalBuildingProfile profile;};
Schema schema(const OriginalHouseGoods& goods){Schema s;s.bytes.assign(2+182+65,0);word(s.bytes,0,0x8001);const size_t common=2,tail=184;
 word(s.bytes,common,5);s.bytes[common+2]=1;s.bytes[common+5]=2;s.bytes[common+7]=1;
 word(s.bytes,common+8,5);word(s.bytes,common+10,5);dword(s.bytes,common+12,1374);word(s.bytes,common+16,3);
 word(s.bytes,common+27,100);dword(s.bytes,common+162,7);word(s.bytes,tail,1);word(s.bytes,tail+2,1);
 need(originalHouseGoodsToPayload(s.bytes.data()+tail+4,61,goods),"synthetic schema goods populate only bounded payload");
 OriginalBuildingFootprint f;f.slot=7;f.recordOffset=0;f.type=3;f.size=2;f.x=f.y=5;f.imageInputs.emplace();f.imageInputs->gridIndex=1374;
 s.profile.status=OriginalBuildingStatus::Complete;s.profile.parsedRecords=4000;s.profile.liveTypeCounts={{3,1}};s.profile.slotTypes[7]=3;s.profile.footprints.push_back(f);return s;}
OriginalBuildingUpdateContext foodContext(const std::map<size_t,OriginalBuildingUpdateInputs>& inputs,const std::map<size_t,OriginalFigureBuildingLink>& links){
 OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);c.buildingInputs=inputs;c.buildingLinks=links;c.populationTotal=100;c.scenarioFoodOverride=0;
 c.goods.emplace();c.goodsRecords=std::vector<OriginalBuildingGoodsRecord>(1);c.storageRoadStatus.emplace();c.poolRoadSelected.emplace();return c;}
// Snapshot every field FoodStep/StorageStep may publish in this fixture;
// include complete raw house arrays so unrelated bytes cannot silently move.
std::string foodWrites(const OriginalBuildingUpdateContext& c){std::ostringstream o;o<<c.storageCalls<<','<<c.foodCalls<<';';
 if(c.goods){for(auto v:c.goods->warehouse)o<<v<<',';for(auto v:c.goods->trade5)o<<v<<',';for(auto v:c.goods->trade7)o<<v<<',';}
 o<<';'<<bool(c.cityDerived);if(c.cityDerived){const auto& d=*c.cityDerived;o<<d.food<<','<<d.foodMonths<<','<<d.foodConsumption<<','<<d.population;
  for(auto v:d.millFood)o<<v<<',';for(auto v:d.warehouse)o<<v<<',';for(auto v:d.trade5)o<<v<<',';for(auto v:d.trade7)o<<v<<',';}
 if(c.buildingInputs)for(const auto& [id,i]:*c.buildingInputs){o<<';'<<id<<':'<<unsigned(i.available);if(i.house)for(auto b:*i.house)o<<','<<unsigned(b);}
 if(c.buildingLinks)for(const auto& [id,l]:*c.buildingLinks){o<<';'<<id<<':'<<unsigned(l.roadAccess);if(l.storage)for(const auto& v:*l.storage)o<<'/'<<v.resource<<','<<v.amount<<','<<v.capacity;}
 return o.str();}
void accessors(){++groups;OriginalHousePayload payload;payload.fill(0xa5);const auto untouched=payload;
 OriginalHouseGoods values{{50,1,-1,-32768,32767,0,1234,-2345,255,256,0x1234,0x5678}};
 need(originalHouseGoodsToPayload(payload,values),"all twelve signed LE goods words encode");
 need(originalHouseGoodsFromPayload(payload)==values,"all twelve signed LE goods words decode exactly");
 need(originalHouseGoodsPayloadOffset==13&&originalHouseGoodsWireOffset==17&&originalHousePayloadWireOffset==4,"wire and stripped payload offsets stay explicit");
 for(size_t n=0;n<12;++n){need(originalHouseGoodsOffset(n)==13+2*n,"bounded offset per word");need(originalHouseGoodsWord(payload.data(),61,n)==values[n],"individual signed word accessor");}
 for(size_t n=0;n<payload.size();++n)if(n<13||n>=37)need(payload[n]==untouched[n],"goods writer preserves every non-goods byte");
 need(payload[13]==50&&payload[14]==0&&payload[15]==1&&payload[16]==0,"asymmetric neighbouring words reveal byte-shift");
 need(detail::i16(payload.data()+14)==256&&*originalHouseGoodsWord(payload.data(),61,0)==50,"legacy shifted read256 is distinguishable from normalized food50");
 const auto stable=payload;
 for(size_t index:{size_t(12),std::numeric_limits<size_t>::max()}){need(!originalHouseGoodsWord(payload.data(),61,index),"out-of-range/overflow index refuses read");need(!originalHouseSetGoodsWord(payload.data(),61,index,1)&&payload==stable,"out-of-range write has no prefix");}
 for(size_t bytes:{size_t(0),size_t(60),size_t(62),std::numeric_limits<size_t>::max()}){need(!originalHouseGoodsFromPayload(payload.data(),bytes),"wrong payload size refuses all words");need(!originalHouseGoodsToPayload(payload.data(),bytes,values)&&payload==stable,"wrong payload write preserves all bytes");}
 need(!originalHouseGoodsWord(nullptr,61,0)&&!originalHouseGoodsFromPayload(nullptr,61),"null reads refuse");
 need(!originalHouseSetGoodsWord(nullptr,61,0,1)&&!originalHouseGoodsToPayload(nullptr,61,values),"null writes refuse");
 for(int value:{-32769,32768})need(!originalHouseSetGoodsWord(payload.data(),61,0,value)&&payload==stable,"out-of-range signed value is never silently truncated");
 std::vector<uint8_t> wire(65,0);word(wire,0,1);word(wire,2,1);std::copy(payload.begin(),payload.end(),wire.begin()+4);
 need(originalHouseGoodsFromWire(wire.data(),wire.size())==values,"outer1/data1 wire prefix and payload agree");
 need(!originalHouseGoodsFromWire(wire.data(),64),"truncated framed record refuses");wire[2]=2;need(!originalHouseGoodsFromWire(wire.data(),65),"different embedded schema refuses");
}
void schemaMarketSyncFood(){++groups;OriginalHouseGoods goods{};goods[0]=50;goods[1]=1;const auto raw=schema(goods);
 const auto inputs=originalBuildingUpdateInputs(raw.bytes,raw.profile);const auto links=originalFigureBuildingLinks(raw.bytes,raw.profile);
 need(inputs&&links&&links->at(7).houseGoods&&links->at(7).houseGoods->at(0)==50&&links->at(7).houseGoods->at(1)==1,"actual two legal readers agree on wire17/payload13 first food word");
 need(originalHouseGoodsFromPayload(*inputs->at(7).house)==goods,"normalized accessor matches existing original-format reader");
 auto legacy=foodContext(*inputs,*links),normalized=legacy;normalized.normalizedHouseGoods=true;
 const auto original=raw.bytes;need(!legacy.normalizedHouseGoods,"new context policy is disabled by default");
 need(originalBuildingFoodStep(legacy)==OriginalBuildingUpdateBlocker::None&&legacy.cityDerived->food==256,"legacy policy retains existing shifted statistics256");
 need(originalBuildingFoodStep(normalized)==OriginalBuildingUpdateBlocker::None&&normalized.cityDerived->food==50&&normalized.cityDerived->foodConsumption==25&&normalized.cityDerived->foodMonths==2,"C1 statistics use actual food word50 with unchanged consumption formula");
 need(*normalized.buildingInputs->at(7).house==*inputs->at(7).house&&raw.bytes==original,"food query neither consumes household goods nor changes source wire");
 OriginalCombinedState city;city.figures.buildings=*links;OriginalBuildingUpdateContext local(OriginalBuildingUpdateScope::AdmittedTicks,true);
 city.buildings.emplace(7,OriginalCombinedBuilding{{raw.profile.footprints[0],inputs->at(7),std::nullopt},local});
 auto& h=city.figures.buildings->at(7);OriginalFigureBuildingLink market;market.type=59;market.marketStorage=std::vector<OriginalStorageCompartment>(6,{0,0,400});market.marketStorage->at(0)={28,100,400};market.marketDemand=std::array<int16_t,7>{};market.marketQuality=60;
 OriginalFigureUpdateContext marketContext;for(unsigned col=9;col<=13;++col)marketContext.houseMarketModels[col][1]=0;int count=0;
 need(originalFigureMarketHouse(market,h,marketContext,count)&&count==1&&h.houseGoods->at(0)==100&&h.houseGoods->at(1)==1,"actual MarketHouse writes only food word0 here and preserves adjacent word1");
 need(originalCombinedSyncBuildings(city).empty(),"actual figure-to-building service synchronization completes");
 const auto synced=originalHouseGoodsFromPayload(*city.buildings.at(7).state.inputs.house);
 need(synced&&synced->at(0)==100&&synced->at(1)==1&&market.marketStorage->at(0).amount==50,"actual Sync stores market result at same contract offsets without changing neighbour");
 auto after=foodContext({{7,city.buildings.at(7).state.inputs}},*city.figures.buildings);after.normalizedHouseGoods=true;
 need(originalBuildingFoodStep(after)==OriginalBuildingUpdateBlocker::None&&after.cityDerived->food==100&&after.cityDerived->foodMonths==4,"Market→Sync→Food observes delivered50 as food100, not stale cross-word256");
}
void signedAndMill(){++groups;OriginalHouseGoods goods{};goods[0]=-32768;goods[1]=1;auto raw=schema(goods);const auto inputs=originalBuildingUpdateInputs(raw.bytes,raw.profile);const auto links=originalFigureBuildingLinks(raw.bytes,raw.profile);need(inputs&&links,"signed-word synthetic schema parses");
 auto c=foodContext(*inputs,*links);c.normalizedHouseGoods=true;need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::None&&c.cityDerived->food==-32768,"normalization preserves reader's signed interpretation, no inferred clamp");
 auto& i=c.buildingInputs->at(7);need(originalHouseSetGoodsWord(i.house->data(),61,0,50),"stage household food50");
 OriginalFigureBuildingLink mill;mill.type=53;mill.alive=1;mill.storage=std::vector<OriginalStorageCompartment>{{1,7,100},{9,11,100},{28,99,100}};c.buildingLinks->emplace(8,mill);c.buildingInputs->emplace(8,OriginalBuildingUpdateInputs{});c.poolRoadSelected->emplace(8,true);
 need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::None&&c.cityDerived->food==68&&c.cityDerived->millFood[1]==7&&c.cityDerived->millFood[9]==11,"house normalization does not alter Mill resource1..9 aggregation or include market commodity28");
 c.buildingInputs->at(7).alive=2;need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::None&&c.cityDerived->food==18,"inactive house excluded independently of format offset");
 c.buildingInputs->at(7).alive=3;c.buildingInputs->at(7).residents=0;need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::None&&c.cityDerived->food==18,"unoccupied house excluded independently of goods");
}
void atomicRefusal(){++groups;OriginalHouseGoods goods{};goods[0]=50;goods[1]=1;auto raw=schema(goods);const auto inputs=originalBuildingUpdateInputs(raw.bytes,raw.profile);const auto links=originalFigureBuildingLinks(raw.bytes,raw.profile);need(inputs&&links,"atomic fixture readers");auto c=foodContext(*inputs,*links);c.normalizedHouseGoods=true;
 c.cityDerived.emplace();c.cityDerived->food=123;c.cityDerived->millFood[2]=9;c.storageCalls=4;c.foodCalls=5;c.goods->warehouse[1]=91;c.goods->trade5[1]=92;c.goods->trade7[1]=93;c.goods->policies.resize(1);
 OriginalFigureBuildingLink warehouse;warehouse.type=54;warehouse.alive=1;warehouse.roadAccess=7;warehouse.storagePolicy=0;warehouse.storage=std::vector<OriginalStorageCompartment>{{3,70,100}};
 c.buildingLinks->emplace(8,warehouse);c.buildingInputs->emplace(8,OriginalBuildingUpdateInputs{});c.buildingInputs->at(8).available=7;c.storageRoadStatus->emplace(8,1);
 OriginalBuildingGoodsRecord record;record.alive=1;record.owner=8;c.goodsRecords->push_back(record);
 OriginalFigureBuildingLink badMill;badMill.type=53;badMill.alive=1;c.buildingLinks->emplace(9,badMill);c.buildingInputs->emplace(9,OriginalBuildingUpdateInputs{});c.poolRoadSelected->emplace(9,true);
 const auto before=foodWrites(c);need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::CityPhase&&foodWrites(c)==before,"late missing Mill storage rolls back storage totals, counters, road mirrors and house/statistics prefix");
 auto old=c;old.normalizedHouseGoods=false;need(originalBuildingFoodStep(old)==OriginalBuildingUpdateBlocker::CityPhase&&foodWrites(old)!=before&&old.storageCalls==5,"legacy low-level partial-write contract is preserved under default policy");
 c.buildingLinks->at(9).storage=std::vector<OriginalStorageCompartment>{};need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::None&&c.storageCalls==5&&c.foodCalls==6&&c.goods->warehouse[3]==70&&c.cityDerived->food==50,"repaired complete C1 step publishes every intended storage/food write once");
 const auto committed=foodWrites(c);c.scenarioFoodOverride=1;need(originalBuildingFoodStep(c)==OriginalBuildingUpdateBlocker::CityPhase&&foodWrites(c)==committed,"unported override refuses without clearing prior committed statistics");
}
OriginalBuildingUpdateContext employmentFixture(){OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);OriginalBuildingFootprint b;b.type=4;b.slot=7;b.imageInputs.emplace();b.imageInputs->gridIndex=229;
 OriginalBuildingUpdateInputs i;i.identity=7;i.alive=1;i.workers=9;i.enabled=1;i.operational=100;i.house.emplace();c.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,b}};c.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,i}};c.buildingLinks=std::map<size_t,OriginalFigureBuildingLink>{{7,{}}};
 c.terrain=std::vector<uint32_t>(51984);c.terrain->at(229)=0x40;c.cropBase=229;c.cityEmployment.emplace();c.cityEmployment->shortageNotice=false;c.cityEmployment->unemploymentNotice=false;c.employmentModel.emplace();c.buildingUnavailable=std::map<size_t,bool>{{7,false}};c.unemploymentTrend=0;c.shortageNoticeCountdown=0;c.elapsedYears=0;c.scenarioStartYears=0;return c;}
void employmentPolicy(){++groups;auto c=employmentFixture();need(!c.employmentCategoryOverrides&&!originalBuildingEmploymentCategory(4)&&!originalBuildingEmploymentCategory(4,c),"strict type4 category remains absent by default");
 need(originalBuildingAvailabilityStep(c)==OriginalBuildingUpdateBlocker::Availability&&c.buildingInputs->at(7).workers==9,"default Availability still refuses unknown type4");
 need(originalBuildingEmploymentStep(c)==OriginalBuildingUpdateBlocker::Employment&&c.buildingInputs->at(7).workers==9,"default Employment still refuses unknown type4");
 c.employmentCategoryOverrides=std::map<uint16_t,int>{{4,-1}};need(originalBuildingEmploymentCategory(4,c)==-1&&originalBuildingEmploymentCategory(72,c)==3,"explicit native override has priority and all other strict mappings retain fallback");
 need(originalBuildingAvailabilityStep(c)==OriginalBuildingUpdateBlocker::None&&c.buildingInputs->at(7).operational==100,"C1 explicit type4 adapter allows real road availability");
 need(originalBuildingEmploymentStep(c)==OriginalBuildingUpdateBlocker::None&&c.buildingInputs->at(7).workers==0&&c.cityEmployment->employed==0&&c.employmentCalls==1,"native no-job policy excludes type4 labor demand without a fake original classification");
 for(int category:{-2,10,std::numeric_limits<int>::max()}){auto bad=employmentFixture();bad.employmentCategoryOverrides=std::map<uint16_t,int>{{4,category}};const auto oldWorkers=bad.buildingInputs->at(7).workers;const auto rows=bad.cityEmployment->rows[0].demand;
  need(!originalBuildingEmploymentCategory(4,bad)&&!originalBuildingEmploymentCategoryPolicyValid(bad),"invalid override cannot reach row index");
  need(originalBuildingAvailabilityStep(bad)==OriginalBuildingUpdateBlocker::EmploymentCategoryPolicy&&bad.buildingInputs->at(7).workers==oldWorkers,"Availability rejects invalid category before writing");
  need(originalBuildingEmploymentStep(bad)==OriginalBuildingUpdateBlocker::EmploymentCategoryPolicy&&bad.buildingInputs->at(7).workers==oldWorkers&&bad.cityEmployment->rows[0].demand==rows&&bad.employmentCalls==0,"Employment rejects invalid category before resetting rows or assigning workers");
  need(std::string(originalBuildingUpdateBlockerName(OriginalBuildingUpdateBlocker::EmploymentCategoryPolicy))=="employment_category_override_out_of_range","invalid policy has a named diagnostic");}
 for(int category=-1;category<=9;++category){auto permitted=employmentFixture();permitted.employmentCategoryOverrides=std::map<uint16_t,int>{{4,category}};need(originalBuildingEmploymentCategoryPolicyValid(permitted)&&originalBuildingEmploymentCategory(4,permitted)==category,"declared category domain -1..9 is explicit");}
}
}
int main(){try{accessors();schemaMarketSyncFood();signedAndMill();atomicRefusal();employmentPolicy();std::cout<<"{\"groups\":"<<groups<<",\"checks\":"<<checks<<",\"passed\":true,\"synthetic_format_consumer_contract_only\":true,\"real_original_data_tested\":false}\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
