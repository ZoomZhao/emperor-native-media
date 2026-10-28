// SPDX-License-Identifier: MIT
// Independent synthetic C1 state contracts, never original-data progress.
#include "native_city_context.hpp"
#include <iostream>
#include <functional>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const std::string& message){++checks;if(!v)throw std::runtime_error(message);}
std::string wire(const NativeCityContextState& s){std::string e;const auto b=encodeNativeCityContext(s,e);need(bool(b),"encode: "+e);return b.value_or("");}
std::string changed(std::string s,const std::string& a,const std::string& b){const auto n=s.find(a);need(n!=std::string::npos,"mutation anchor: "+a);s.replace(n,a.size(),b);return s;}
void refuses(const std::string& bytes){std::string e;need(!decodeNativeCityContext(bytes,e)&&!e.empty(),"malformed wire rejected with reason");}
void rejects(const NativeCityContextState& s){std::string e;need(!validateNativeCityContext(s).empty(),"validator rejects invalid DTO");need(!encodeNativeCityContext(s,e)&&!e.empty(),"invalid DTO never encoded");}
std::string zeroes(size_t n){std::string s;for(size_t i=0;i<n;++i)s+="0 ";return s;}
// Fixed v1 sparse golden, independent of the production visitor and sizeof.
std::string emptyGolden(){
 std::string s="NATIVE-CITY-CONTEXT 1\n";
 for(const char* f:{"accounting","cityDerived","statistics","religion","cycleTail","trade","buildingTotals","goods","goodsRecords","storageRoadStatus"})s+=std::string(f)+" 0 \n";
 s+="millAvailable "+zeroes(36)+"\nmillCandidates "+zeroes(100)+"\n";
 for(const char* f:{"millCandidateCount","unemploymentTrend","shortageNoticeCountdown","elapsedYears","scenarioStartYears","scenarioMonths","serviceCoverage","serviceRating","serviceWeakest","servicePenalty","treasury","empireProduction"})s+=std::string(f)+" 0 \n";
 s+="dispatchedProduction "+zeroes(36)+"\n";
 for(const char* f:{"citySignal","cityCountA","cityCountB","highestBuildingSlot","buildingRegistrySize","highestEverBuildingSlot","goodsRegistryLast","loadRoadRefreshApplied","loadAccountingApplied","statisticsScratch","hostileCount","hostileFlag","waterScratch","laborCountdownHouses"})s+=std::string(f)+" 0 \n";
 for(const char* f:{"ownershipRepairs","fireTotals","phase47Counters"})s+=std::string(f)+" "+zeroes(4)+"\n";
 for(const char* f:{"houseEvolutionResult","storageSupplierMismatch","farmerTasks","supplierRequests","stallHookOrdinal","accountingCalls","formationCalls","networkCalls","goodsPoolCalls","employmentCalls","irrigationCalls","statisticsCalls","storageCalls","foodCalls","hookPreambleCalls","desirabilityCalls","houseEvolutionCalls","houseConversions","houseRedraws","lifecycleCalls","modelChanges","fireCalls","riskCalls","monumentCalls","counterCalls","cropDispatchCalls","stallSupplierCalls","workers"})s+=std::string(f)+" 0 \n";
 return s+"END-NATIVE-CITY-CONTEXT\n";
}
OriginalBuildingUpdateContext source(){
 OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
 c.accounting.emplace();auto& a=*c.accounting;
 a.taxRate=1;a.taxableOrdinary=2;a.taxableElite=3;a.taxReceived=4;a.taxOther=5;a.wages=6;a.wagesPaid=7;a.incomeOther=8;a.expensesOther=9;a.treasuryStart=-10;a.incomeReceived=11;a.expensesPaid=12;a.actualProfit=-13;a.tributeIncomeComponent=14;
 a.ordinaryTax=15;a.eliteTax=16;a.taxTotal=17;a.taxForecast=18;a.wageTotal=19;a.wageForecast=20;a.income=21;a.expenses=22;a.profit=-23;a.treasuryForecast=24;a.festivalCount=25;a.festivalCapacity=26;
 c.cityDerived.emplace();auto& d=*c.cityDerived;d.population=101;d.food=102;d.foodMonths=103;d.foodConsumption=104;
 for(size_t i=0;i<11;++i)d.serviceCoverage[i]=int32_t(i+111);
 for(size_t i=0;i<10;++i)d.millFood[i]=int32_t(i+121);
 for(size_t i=0;i<36;++i){d.warehouse[i]=uint32_t(131+i);d.trade5[i]=uint32_t(171+i);d.trade7[i]=uint32_t(211+i);}
 c.statistics.emplace();auto& st=*c.statistics;st.primaryBuilding=5;st.alternateBuilding=9;st.emptyCycles=7;st.currentFaction=2;st.capacity=456;st.primaryPresent=1;st.religionBonusEnabled=false;st.religionBonus=-11;
 st.demand={3,5,7,9};st.groups={4,6,8,10};for(size_t i=0;i<22;++i)st.empireState[i]=uint8_t(i+1);
 c.religion.emplace();auto& r=*c.religion;r.previousRating=-67;r.selected=3;r.calls=73;
 for(size_t i=0;i<4;++i){auto& w=r.worship[i];w.enabled=i%2;w.minimum=-123;w.maximum=234;w.average=int32_t(i+12);w.served=int32_t(i+22);w.temple=i%2;for(size_t j=0;j<3;++j)w.entries[j]={-50,150,int32_t(j),int32_t(i*3+j),int32_t(i*10+j)};}
 for(size_t i=0;i<12;++i)r.scenarioEnabled[i]=uint8_t(i+2);
 c.cycleTail.emplace();auto& h=*c.cycleTail;h.smoothedHealth=62;h.healthTarget=81;h.currentHealthRating=149;h.productionHistory={13,27,-31,45};h.foodForecast={79,-91};h.alternateMode=false;h.previousHealthRating=148;h.waterMode=2;h.cropQuality={0,1,254,255};h.calls=83;
 c.trade.emplace();auto& t=*c.trade;t.counters={1,2,3,4};t.dispatchDelay=std::array<int32_t,2>{111,222};t.seaEntry=std::array<int16_t,2>{-1,2};t.calls=91;t.eligible=92;t.timersChanged=93;
 for(size_t n=0;n<22;++n){auto& x=t.partners[n];x.alive=1;x.kind=uint8_t(n%7);x.faction=uint8_t(n);x.sea=uint8_t(n%2);x.blocked=uint8_t(n+1);x.enabled=1;x.relationship=int32_t(n+20);x.timer=int16_t(n+30);x.suspended=int16_t(n+40);x.traders={1,1998,1999};x.reserveCountdown=6;x.reserveAmount=7;x.state=8;x.fortified=1;
  for(size_t i=0;i<36;++i){x.quota[i]=uint8_t(i+10);x.quotaBonus[i]=uint8_t(i+50);}x.buys={0,1,34,35};x.sells={35,34,1,0};x.reserves={-1,2,-3,4,-5};
  x.relationTimers.resize(22);for(size_t i=0;i<22;++i)for(size_t j=0;j<2;++j)for(size_t k=0;k<34;++k)x.relationTimers[i][j][k]=int32_t(10000*n+100*i+34*j+k);
  for(size_t i=0;i<22;++i)x.relations[i]={uint8_t(i+1),int32_t(i+2),int32_t(i+3),int32_t(i+4),int32_t(i+5),int32_t(i+6),int32_t(i+7)};
 }
 c.buildingTotals.emplace();auto& b=*c.buildingTotals;for(size_t i=0;i<269;++i){b.total[i]=int32_t(i+1);b.staffed[i]=int32_t(i+301);}for(size_t i=0;i<36;++i){b.producers[i]=int32_t(i+601);b.staffedProducers[i]=int32_t(i+701);}b.calls=101;
 c.goods.emplace();auto& g=*c.goods;g.policies.resize(3);g.savedPhase=33;g.storageSearch=std::array<uint8_t,36>{};g.industrySearch=std::array<uint8_t,36>{};g.godGate=4;
 for(size_t i=0;i<36;++i){g.unavailable[i]=uint32_t(i+1001);g.trade7[i]=uint32_t(i+1101);g.trade5[i]=uint32_t(i+1201);g.warehouse[i]=uint32_t(i+1301);g.storageSearch->at(i)=uint8_t(i+1);g.industrySearch->at(i)=uint8_t(i+41);for(size_t j=0;j<3;++j){g.policies[j].accept[i]=uint8_t(i+j);g.policies[j].trade[i]=uint8_t(i+j+10);g.policies[j].limit[i]=int16_t(-int(i)-int(j));}}
 for(size_t i=0;i<10;++i)g.industryPriority[i]=int32_t(i+1401);
 c.goodsRecords=std::vector<OriginalBuildingGoodsRecord>(3);c.goodsRecords->at(1).owner=4;c.goodsRecords->at(1).alive=1;c.goodsRecords->at(1).mode[35]=253;c.goodsRecords->at(2).owner=3999;
 c.storageRoadStatus=std::map<size_t,uint8_t>{{4,1},{3999,0}};
 for(size_t i=0;i<36;++i){c.millAvailable[i]=int32_t(i-19);c.dispatchedProduction[i]=int32_t(i+1501);}c.millCandidateCount=2;c.millCandidates[0]=4;c.millCandidates[1]=3999;
 c.unemploymentTrend=-2;c.shortageNoticeCountdown=42;c.elapsedYears=17;c.scenarioStartYears=-300;c.scenarioMonths=11;c.serviceCoverage=d.serviceCoverage;c.serviceRating=65;c.serviceWeakest=3;c.servicePenalty=15;c.treasury=-987654321;c.empireProduction=std::array<bool,36>{};c.empireProduction->at(35)=true;
 c.citySignal=9;c.cityCountA=-8;c.cityCountB=7;c.highestBuildingSlot=3999;c.buildingRegistrySize=4000;c.highestEverBuildingSlot=3999;c.goodsRegistryLast=8191;c.loadRoadRefreshApplied=true;c.loadAccountingApplied=true;c.statisticsScratch=-123;c.hostileCount=3;c.hostileFlag=1;c.waterScratch=-14;c.laborCountdownHouses=15;
 c.ownershipRepairs={1,2,3,4};c.fireTotals={5,6,7,8};c.phase47Counters={9,10,11,12};c.houseEvolutionResult=-17;c.storageSupplierMismatch={{4,-7}};c.farmerTasks={{1999,2}};c.supplierRequests={{1,{1,2,3,4,5,6,7}}};c.stallHookOrdinal=18;
 c.accountingCalls=1;c.formationCalls=2;c.networkCalls=3;c.goodsPoolCalls=4;c.employmentCalls=5;c.irrigationCalls=6;c.statisticsCalls=7;c.storageCalls=8;c.foodCalls=9;c.hookPreambleCalls=10;c.desirabilityCalls=11;c.houseEvolutionCalls=12;c.houseConversions=13;c.houseRedraws=14;c.lifecycleCalls=15;c.modelChanges=16;c.fireCalls=17;c.riskCalls=18;c.monumentCalls=19;c.counterCalls=20;c.cropDispatchCalls=21;c.stallSupplierCalls=22;
 c.buildings.emplace();OriginalBuildingFootprint f;f.type=33;c.buildings->emplace(4,f);f.type=72;c.buildings->emplace(3999,f);f.type=2;c.buildings->emplace(7,f);c.buildingInputs.emplace();c.buildingInputs->operator[](4).workers=9;c.buildingInputs->operator[](3999).workers=11;c.buildingInputs->operator[](7).workers=2;
 return c;
}
template<class T>void alter(T& v){
 if constexpr(std::is_same_v<T,bool>)v=!v;
 else if constexpr(std::is_integral_v<T>)++v;
 else if constexpr(native_city_context_detail::Optional<T>::value){if(!v)v.emplace();alter(*v);}
 else if constexpr(native_city_context_detail::Array<T>::value){alter(v[0]);}
 else if constexpr(native_city_context_detail::Vector<T>::value){if(v.empty())v.emplace_back();alter(v.front());}
 else if constexpr(native_city_context_detail::Map<T>::value){need(!v.empty(),"map sentinel exists");alter(v.begin()->second);}
 else {struct First {bool done=false;void field(const char*,int&){};};(void)v;}
}
void allOwnedFields(){
 auto c=source();const auto state=captureNativeCityContext(c);const auto bytes=wire(state);std::string e;const auto back=decodeNativeCityContext(bytes,e);need(bool(back),"full fixture decode: "+e);need(wire(*back)==bytes,"canonical full fixture exact byte roundtrip");
 need(state.workers==std::map<size_t,int16_t>{{4,9},{3999,11}},"only native job33/72 workers captured");
 auto target=source();target.month=12;target.phase=50;target.labor.emplace();target.labor->health=77;target.cityEmployment.emplace();target.cityEmployment->available=66;target.terrain=std::vector<uint32_t>{0xabcdef};target.notificationEffects={1234};target.buildingInputs->at(4).workers=42;target.buildings->at(4).type=2;
 applyNativeCityContext(*back,target);need(target.month==12&&target.phase==50,"apply leaves calendar authority unchanged");need(target.labor->health==77&&target.cityEmployment->available==66,"apply never duplicates Labor or Employment authority");need(target.terrain==std::optional<std::vector<uint32_t>>{{0xabcdef}}&&target.notificationEffects==std::vector<unsigned>{1234},"apply leaves terrain and outbox untouched");need(target.buildingInputs->at(4).workers==42&&target.buildings->at(4).type==2,"workers projection never writes authoritative records");
 auto expected=state;expected.workers.clear();auto applied=captureNativeCityContext(target);applied.workers.clear();need(wire(applied)==wire(expected),"capture/apply copies all owned fields without template reset");
#define ACCOUNT(n) need(back->accounting->n==state.accounting->n,"Accounting." #n " exact");
 ACCOUNT(taxRate);ACCOUNT(taxableOrdinary);ACCOUNT(taxableElite);ACCOUNT(taxReceived);ACCOUNT(taxOther);ACCOUNT(wages);ACCOUNT(wagesPaid);ACCOUNT(incomeOther);ACCOUNT(expensesOther);ACCOUNT(treasuryStart);ACCOUNT(incomeReceived);ACCOUNT(expensesPaid);ACCOUNT(actualProfit);ACCOUNT(tributeIncomeComponent);ACCOUNT(ordinaryTax);ACCOUNT(eliteTax);ACCOUNT(taxTotal);ACCOUNT(taxForecast);ACCOUNT(wageTotal);ACCOUNT(wageForecast);ACCOUNT(income);ACCOUNT(expenses);ACCOUNT(profit);ACCOUNT(treasuryForecast);ACCOUNT(festivalCount);ACCOUNT(festivalCapacity);
#undef ACCOUNT
#define DISTINCT(path) {auto x=state;alter(x.path);need(wire(x)!=bytes,"named field " #path " changes encoded snapshot");}
 DISTINCT(accounting->taxRate);
 DISTINCT(accounting->taxableOrdinary);
 DISTINCT(accounting->taxableElite);
 DISTINCT(accounting->taxReceived);
 DISTINCT(accounting->taxOther);
 DISTINCT(accounting->wages);
 DISTINCT(accounting->wagesPaid);
 DISTINCT(accounting->incomeOther);
 DISTINCT(accounting->expensesOther);
 DISTINCT(accounting->treasuryStart);
 DISTINCT(accounting->incomeReceived);
 DISTINCT(accounting->expensesPaid);
 DISTINCT(accounting->actualProfit);
 DISTINCT(accounting->tributeIncomeComponent);
 DISTINCT(accounting->ordinaryTax);
 DISTINCT(accounting->eliteTax);
 DISTINCT(accounting->taxTotal);
 DISTINCT(accounting->taxForecast);
 DISTINCT(accounting->wageTotal);
 DISTINCT(accounting->wageForecast);
 DISTINCT(accounting->income);
 DISTINCT(accounting->expenses);
 DISTINCT(accounting->profit);
 DISTINCT(accounting->treasuryForecast);
 DISTINCT(accounting->festivalCount);
 DISTINCT(accounting->festivalCapacity);
 DISTINCT(cityDerived->serviceCoverage);
 DISTINCT(cityDerived->population);
 DISTINCT(cityDerived->food);
 DISTINCT(cityDerived->foodMonths);
 DISTINCT(cityDerived->foodConsumption);
 DISTINCT(cityDerived->millFood);
 DISTINCT(cityDerived->warehouse);
 DISTINCT(cityDerived->trade5);
 DISTINCT(cityDerived->trade7);
 DISTINCT(statistics->primaryBuilding);
 DISTINCT(statistics->alternateBuilding);
 DISTINCT(statistics->emptyCycles);
 DISTINCT(statistics->currentFaction);
 DISTINCT(statistics->capacity);
 DISTINCT(statistics->primaryPresent);
 DISTINCT(statistics->demand);
 DISTINCT(statistics->groups);
 DISTINCT(statistics->empireState);
 DISTINCT(statistics->religionBonusEnabled);
 DISTINCT(statistics->religionBonus);
 DISTINCT(religion->worship[0].entries[0].minimum);
 DISTINCT(religion->worship[0].entries[0].maximum);
 DISTINCT(religion->worship[0].entries[0].status);
 DISTINCT(religion->worship[0].entries[0].type);
 DISTINCT(religion->worship[0].entries[0].value);
 DISTINCT(religion->worship[0].enabled);
 DISTINCT(religion->worship[0].minimum);
 DISTINCT(religion->worship[0].maximum);
 DISTINCT(religion->worship[0].average);
 DISTINCT(religion->worship[0].served);
 DISTINCT(religion->worship[0].temple);
 DISTINCT(religion->scenarioEnabled);
 DISTINCT(religion->previousRating);
 DISTINCT(religion->calls);
 DISTINCT(cycleTail->smoothedHealth);
 DISTINCT(cycleTail->healthTarget);
 DISTINCT(cycleTail->currentHealthRating);
 DISTINCT(cycleTail->productionHistory);
 DISTINCT(cycleTail->foodForecast);
 DISTINCT(cycleTail->alternateMode);
 DISTINCT(cycleTail->previousHealthRating);
 DISTINCT(cycleTail->waterMode);
 DISTINCT(cycleTail->cropQuality);
 DISTINCT(cycleTail->calls);
 DISTINCT(trade->partners[0].alive);
 DISTINCT(trade->partners[0].kind);
 DISTINCT(trade->partners[0].faction);
 DISTINCT(trade->partners[0].sea);
 DISTINCT(trade->partners[0].blocked);
 DISTINCT(trade->partners[0].enabled);
 DISTINCT(trade->partners[0].relationship);
 DISTINCT(trade->partners[0].timer);
 DISTINCT(trade->partners[0].suspended);
 DISTINCT(trade->partners[0].traders);
 DISTINCT(trade->partners[0].quota);
 DISTINCT(trade->partners[0].quotaBonus);
 DISTINCT(trade->partners[0].buys);
 DISTINCT(trade->partners[0].reserveCountdown);
 DISTINCT(trade->partners[0].reserveAmount);
 DISTINCT(trade->partners[0].reserves);
 DISTINCT(trade->partners[0].relations[0].annualFlag);
 DISTINCT(trade->partners[0].relations[0].value);
 DISTINCT(trade->partners[0].relations[0].gift);
 DISTINCT(trade->partners[0].relations[0].tribute);
 DISTINCT(trade->partners[0].relations[0].state);
 DISTINCT(trade->partners[0].relations[0].previousState);
 DISTINCT(trade->partners[0].relations[0].war);
 DISTINCT(trade->partners[0].state);
 DISTINCT(trade->partners[0].fortified);
 DISTINCT(trade->counters);
 DISTINCT(trade->dispatchDelay);
 DISTINCT(trade->seaEntry);
 DISTINCT(trade->calls);
 DISTINCT(trade->eligible);
 DISTINCT(trade->timersChanged);
 DISTINCT(buildingTotals->total);
 DISTINCT(buildingTotals->staffed);
 DISTINCT(buildingTotals->producers);
 DISTINCT(buildingTotals->staffedProducers);
 DISTINCT(buildingTotals->calls);
 DISTINCT(goods->policies[0].accept);
 DISTINCT(goods->policies[0].trade);
 DISTINCT(goods->policies[0].limit);
 DISTINCT(goods->unavailable);
 DISTINCT(goods->trade7);
 DISTINCT(goods->trade5);
 DISTINCT(goods->warehouse);
 DISTINCT(goods->savedPhase);
 DISTINCT(goods->storageSearch);
 DISTINCT(goods->industrySearch);
 DISTINCT(goods->godGate);
 DISTINCT(goods->industryPriority);
 DISTINCT(goodsRecords->at(1).mode);
 DISTINCT(storageRoadStatus);
 DISTINCT(millAvailable);
 DISTINCT(millCandidateCount);
 DISTINCT(unemploymentTrend);
 DISTINCT(shortageNoticeCountdown);
 DISTINCT(elapsedYears);
 DISTINCT(scenarioStartYears);
 DISTINCT(scenarioMonths);
 DISTINCT(serviceCoverage);
 DISTINCT(serviceRating);
 DISTINCT(serviceWeakest);
 DISTINCT(servicePenalty);
 DISTINCT(treasury);
 DISTINCT(empireProduction);
 DISTINCT(dispatchedProduction);
 DISTINCT(citySignal);
 DISTINCT(cityCountA);
 DISTINCT(cityCountB);
 DISTINCT(loadRoadRefreshApplied);
 DISTINCT(loadAccountingApplied);
 DISTINCT(statisticsScratch);
 DISTINCT(hostileCount);
 DISTINCT(hostileFlag);
 DISTINCT(waterScratch);
 DISTINCT(laborCountdownHouses);
 DISTINCT(ownershipRepairs);
 DISTINCT(fireTotals);
 DISTINCT(phase47Counters);
 DISTINCT(houseEvolutionResult);
 DISTINCT(storageSupplierMismatch);
 DISTINCT(supplierRequests);
 DISTINCT(stallHookOrdinal);
 DISTINCT(accountingCalls);
 DISTINCT(formationCalls);
 DISTINCT(networkCalls);
 DISTINCT(goodsPoolCalls);
 DISTINCT(employmentCalls);
 DISTINCT(irrigationCalls);
 DISTINCT(statisticsCalls);
 DISTINCT(storageCalls);
 DISTINCT(foodCalls);
 DISTINCT(hookPreambleCalls);
 DISTINCT(desirabilityCalls);
 DISTINCT(houseEvolutionCalls);
 DISTINCT(houseConversions);
 DISTINCT(houseRedraws);
 DISTINCT(lifecycleCalls);
 DISTINCT(modelChanges);
 DISTINCT(fireCalls);
 DISTINCT(riskCalls);
 DISTINCT(monumentCalls);
 DISTINCT(counterCalls);
 DISTINCT(cropDispatchCalls);
 DISTINCT(stallSupplierCalls);
 DISTINCT(workers);

#undef DISTINCT
 need(back->religion->selected==3,"religion selected preserved");need(back->highestEverBuildingSlot==3999,"highest-ever independent registry preserved");
 for(size_t p=0;p<22;++p){const auto& x=back->trade->partners[p];const auto& y=state.trade->partners[p];need(x.sells==y.sells&&x.traders==y.traders,"all trade resource and figure ids preserved");need(x.relationTimers==y.relationTimers,"all22x2x34 relation timer words including33 preserved");for(size_t n=0;n<22;++n)need(x.relations[n].previousState==int32_t(n+6)&&x.relations[n].war==int32_t(n+7),"all22 relation records preserve tail fields");}
 need(back->buildingTotals->total==state.buildingTotals->total&&back->buildingTotals->staffed==state.buildingTotals->staffed,"all269 totals rows preserved");need(back->goods->policies[2].limit==state.goods->policies[2].limit,"all36 policy limit values preserved");
 auto absent=state;absent.accounting->tributeIncomeComponent.reset();absent.statistics->religionBonusEnabled.reset();absent.religion->selected.reset();absent.cycleTail->alternateMode.reset();absent.trade->dispatchDelay.reset();absent.goods->storageSearch.reset();absent.goods->godGate.reset();need(wire(absent)!=bytes,"absence distinct from zero/false");const auto a=decodeNativeCityContext(wire(absent),e);need(a&&!a->accounting->tributeIncomeComponent&&!a->statistics->religionBonusEnabled&&!a->religion->selected&&!a->cycleTail->alternateMode&&!a->trade->dispatchDelay&&!a->goods->storageSearch&&!a->goods->godGate,"optional absence survives");
}
void inactiveTypedBits(){
 auto x=captureNativeCityContext(source());x.goodsRecords->at(0).owner=INT16_MIN;x.goodsRecords->at(2).owner=INT16_MAX;
 auto& p=x.trade->partners[21];p.alive=0;p.faction=255;p.traders={INT16_MIN,INT16_MAX,-1};p.buys={255,254,253,252};p.sells={251,250,249,248};
 std::string e;const auto b=wire(x);const auto y=decodeNativeCityContext(b,e);need(y&&wire(*y)==b,"inactive typed fields preserved rather than silently normalized");need(y->goodsRecords->at(0).owner==INT16_MIN&&y->goodsRecords->at(2).owner==INT16_MAX,"inactive owner bits preserve signed extremes");need(y->trade->partners[21].faction==255&&y->trade->partners[21].traders==p.traders&&y->trade->partners[21].buys==p.buys&&y->trade->partners[21].sells==p.sells,"inactive partner does not invent a live reference");
}
void invalidCases(){
 const auto state=captureNativeCityContext(source());const auto bytes=wire(state);const auto sparse=wire(NativeCityContextState{});
 refuses(changed(bytes,"NATIVE-CITY-CONTEXT 1","NATIVE-CITY-CONTEXT 2"));refuses(changed(bytes,"accounting 1","accounting 2"));refuses(changed(bytes,"taxRate 1 ","taxRate 2147483648 "));refuses(changed(bytes,"accountingCalls 1 ","accountingCalls 4294967296 "));refuses(changed(bytes,"accountingCalls 1 ","accountingCalls -1 "));
 refuses(changed(bytes,"timer 30 ","timer 32768 "));refuses(changed(bytes,"faction 0 ","faction 256 "));refuses(changed(bytes,"loadAccountingApplied 1 ","loadAccountingApplied 2 "));
 refuses(changed(bytes,"workers 2 4 9 3999 11 ","workers 2 4 9 4 11 "));refuses(changed(bytes,"workers 2 4 9 3999 11 ","workers 18446744073709551615 "));refuses(changed(bytes,"relationTimers 22 ","relationTimers 23 "));refuses(changed(bytes,"policies 3 ","policies 8193 "));refuses(changed(bytes,"cropQuality 4 ","cropQuality 51985 "));
 refuses(changed(bytes,"taxRate 1 ","taxRate 1 \ntaxRate 2 "));refuses(changed(bytes,"taxRate 1 ","unknown 1 "));refuses(bytes+"trailing");refuses(bytes.substr(0,bytes.size()-25));refuses(std::string("\0",1)+bytes);refuses(std::string(nativeCityContextByteLimit+1,' '));
 auto x=state;x.schemaVersion=2;rejects(x);x=state;x.millCandidateCount=101;rejects(x);x=state;x.millCandidates[0]=4000;rejects(x);x=state;x.workers={{0,1}};rejects(x);x=state;x.workers={{4000,1}};rejects(x);x=state;x.workers={{1,-1}};rejects(x);x=state;x.goodsRecords->at(1).owner=0;rejects(x);x=state;x.goodsRecords->at(1).owner=-1;rejects(x);x=state;x.goods->savedPhase=51;rejects(x);x=state;x.cycleTail->smoothedHealth=101;rejects(x);x=state;x.cycleTail->smoothedHealth=-1;rejects(x);x=state;x.statistics->currentFaction=22;rejects(x);x=state;x.statistics->primaryBuilding=4000;rejects(x);x=state;x.religion->selected=4;rejects(x);x=state;x.religion->worship[0].entries[0].minimum=151;rejects(x);x=state;x.religion->worship[0].entries[0].type=12;rejects(x);x=state;x.trade->partners[0].faction=22;rejects(x);x=state;x.trade->partners[0].traders[0]=2000;rejects(x);x=state;x.trade->partners[0].buys[0]=36;rejects(x);x=state;x.trade->partners[0].relationTimers.resize(23);rejects(x);x=state;x.supplierRequests={{2000,{}}};rejects(x);x=state;x.farmerTasks={{1,3}};rejects(x);x=state;x.storageSupplierMismatch={{0,1}};rejects(x);
 // Encode must validate allocation bounds before formatting large containers.
 x=state;x.goods->policies.resize(8193);rejects(x);x=state;x.goodsRecords->resize(8193);rejects(x);x=state;x.cycleTail->cropQuality.resize(51985);rejects(x);
 std::string e;need(!decodeNativeCityContext(changed(sparse,"workers 0 ","workers 0 \nworkers 0 "),e),"duplicate final field rejected");
}
}
int main(){try{need(wire(NativeCityContextState{})==emptyGolden(),"fixed independent sparse v1 golden");allOwnedFields();inactiveTypedBits();invalidCases();std::cout<<"native_city_context "<<checks<<"/"<<checks<<" synthetic assertions passed\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
