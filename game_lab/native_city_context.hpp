// SPDX-License-Identifier: MIT
#pragma once
#include "original_building_update.hpp"
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace game_lab {
// C1 preserves already-defined mutable city services; it enables no scheduler,
// calendar/monthly/annual service, health algorithm or original timing claim.
// Source: OriginalBuildingUpdateContext declared in original_building_update.hpp.
// Labor/Employment/population, schedule, figure graph, RNG, terrain and immutable
// models remain with their existing owners. Notifications/redraw buffers are
// transaction outputs, not this snapshot. Annual offers/messages belong to the
// outer OriginalCombinedState and are deliberately not claimed by this codec.
inline constexpr uint32_t nativeCityContextVersion=1;
inline constexpr size_t nativeCityContextByteLimit=8u*1024u*1024u;
#define NATIVE_CITY_CONTEXT_MEMBERS(X) \
    X(accounting) \
    X(cityDerived) \
    X(statistics) \
    X(religion) \
    X(cycleTail) \
    X(trade) \
    X(buildingTotals) \
    X(goods) \
    X(goodsRecords) \
    X(storageRoadStatus) \
    X(millAvailable) \
    X(millCandidates) \
    X(millCandidateCount) \
    X(unemploymentTrend) \
    X(shortageNoticeCountdown) \
    X(elapsedYears) \
    X(scenarioStartYears) \
    X(scenarioMonths) \
    X(serviceCoverage) \
    X(serviceRating) \
    X(serviceWeakest) \
    X(servicePenalty) \
    X(treasury) \
    X(empireProduction) \
    X(dispatchedProduction) \
    X(citySignal) \
    X(cityCountA) \
    X(cityCountB) \
    X(highestBuildingSlot) \
    X(buildingRegistrySize) \
    X(highestEverBuildingSlot) \
    X(goodsRegistryLast) \
    X(loadRoadRefreshApplied) \
    X(loadAccountingApplied) \
    X(statisticsScratch) \
    X(hostileCount) \
    X(hostileFlag) \
    X(waterScratch) \
    X(laborCountdownHouses) \
    X(ownershipRepairs) \
    X(fireTotals) \
    X(phase47Counters) \
    X(houseEvolutionResult) \
    X(storageSupplierMismatch) \
    X(farmerTasks) \
    X(supplierRequests) \
    X(stallHookOrdinal) \
    X(accountingCalls) \
    X(formationCalls) \
    X(networkCalls) \
    X(goodsPoolCalls) \
    X(employmentCalls) \
    X(irrigationCalls) \
    X(statisticsCalls) \
    X(storageCalls) \
    X(foodCalls) \
    X(hookPreambleCalls) \
    X(desirabilityCalls) \
    X(houseEvolutionCalls) \
    X(houseConversions) \
    X(houseRedraws) \
    X(lifecycleCalls) \
    X(modelChanges) \
    X(fireCalls) \
    X(riskCalls) \
    X(monumentCalls) \
    X(counterCalls) \
    X(cropDispatchCalls) \
    X(stallSupplierCalls)
struct NativeCityContextState {
    uint32_t schemaVersion=nativeCityContextVersion;
#define NATIVE_CITY_CONTEXT_DECLARE(name) decltype(std::declval<OriginalBuildingUpdateContext>().name) name{};
    NATIVE_CITY_CONTEXT_MEMBERS(NATIVE_CITY_CONTEXT_DECLARE)
#undef NATIVE_CITY_CONTEXT_DECLARE
    // A saved projection for native job types33/72. apply never writes records;
    // the integration owner must prove consistency with its strictRecords.
    std::map<size_t,int16_t> workers;
};
namespace native_city_context_detail {
using Context=OriginalBuildingUpdateContext;
template<class T>struct Optional:std::false_type{};
template<class T>struct Optional<std::optional<T>>:std::true_type{using value_type=T;};
template<class T>struct Vector:std::false_type{};
template<class T,class A>struct Vector<std::vector<T,A>>:std::true_type{using value_type=T;};
template<class T>struct Map:std::false_type{};
template<class K,class V,class C,class A>struct Map<std::map<K,V,C,A>>:std::true_type{using key_type=K;using mapped_type=V;};
template<class T>struct Array:std::false_type{};
template<class T,size_t N>struct Array<std::array<T,N>>:std::true_type{};
template<class A>void fields(A& a,Context::Accounting& v){a.field("taxRate",v.taxRate);a.field("taxableOrdinary",v.taxableOrdinary);a.field("taxableElite",v.taxableElite);a.field("taxReceived",v.taxReceived);a.field("taxOther",v.taxOther);a.field("wages",v.wages);a.field("wagesPaid",v.wagesPaid);a.field("incomeOther",v.incomeOther);a.field("expensesOther",v.expensesOther);a.field("treasuryStart",v.treasuryStart);a.field("incomeReceived",v.incomeReceived);a.field("expensesPaid",v.expensesPaid);a.field("actualProfit",v.actualProfit);a.field("tributeIncomeComponent",v.tributeIncomeComponent);a.field("ordinaryTax",v.ordinaryTax);a.field("eliteTax",v.eliteTax);a.field("taxTotal",v.taxTotal);a.field("taxForecast",v.taxForecast);a.field("wageTotal",v.wageTotal);a.field("wageForecast",v.wageForecast);a.field("income",v.income);a.field("expenses",v.expenses);a.field("profit",v.profit);a.field("treasuryForecast",v.treasuryForecast);a.field("festivalCount",v.festivalCount);a.field("festivalCapacity",v.festivalCapacity);}
template<class A>void fields(A& a,Context::CityDerived& v){a.field("serviceCoverage",v.serviceCoverage);a.field("population",v.population);a.field("food",v.food);a.field("foodMonths",v.foodMonths);a.field("foodConsumption",v.foodConsumption);a.field("millFood",v.millFood);a.field("warehouse",v.warehouse);a.field("trade5",v.trade5);a.field("trade7",v.trade7);}
template<class A>void fields(A& a,Context::Statistics& v){a.field("primaryBuilding",v.primaryBuilding);a.field("alternateBuilding",v.alternateBuilding);a.field("emptyCycles",v.emptyCycles);a.field("currentFaction",v.currentFaction);a.field("capacity",v.capacity);a.field("primaryPresent",v.primaryPresent);a.field("demand",v.demand);a.field("groups",v.groups);a.field("empireState",v.empireState);a.field("religionBonusEnabled",v.religionBonusEnabled);a.field("religionBonus",v.religionBonus);}
template<class A>void fields(A& a,Context::Religion::Entry& v){a.field("minimum",v.minimum);a.field("maximum",v.maximum);a.field("status",v.status);a.field("type",v.type);a.field("value",v.value);}
template<class A>void fields(A& a,Context::Religion::Worship& v){a.field("entries",v.entries);a.field("enabled",v.enabled);a.field("minimum",v.minimum);a.field("maximum",v.maximum);a.field("average",v.average);a.field("served",v.served);a.field("temple",v.temple);}
template<class A>void fields(A& a,Context::Religion& v){a.field("worship",v.worship);a.field("scenarioEnabled",v.scenarioEnabled);a.field("previousRating",v.previousRating);a.field("selected",v.selected);a.field("calls",v.calls);}
template<class A>void fields(A& a,Context::CycleTail& v){a.field("smoothedHealth",v.smoothedHealth);a.field("healthTarget",v.healthTarget);a.field("currentHealthRating",v.currentHealthRating);a.field("productionHistory",v.productionHistory);a.field("foodForecast",v.foodForecast);a.field("alternateMode",v.alternateMode);a.field("previousHealthRating",v.previousHealthRating);a.field("waterMode",v.waterMode);a.field("cropQuality",v.cropQuality);a.field("calls",v.calls);}
template<class A>void fields(A& a,Context::Trade::Partner::Relation& v){a.field("annualFlag",v.annualFlag);a.field("value",v.value);a.field("gift",v.gift);a.field("tribute",v.tribute);a.field("state",v.state);a.field("previousState",v.previousState);a.field("war",v.war);}
template<class A>void fields(A& a,Context::Trade::Partner& v){a.field("alive",v.alive);a.field("kind",v.kind);a.field("faction",v.faction);a.field("sea",v.sea);a.field("blocked",v.blocked);a.field("enabled",v.enabled);a.field("relationship",v.relationship);a.field("timer",v.timer);a.field("suspended",v.suspended);a.field("traders",v.traders);a.field("quota",v.quota);a.field("quotaBonus",v.quotaBonus);a.field("buys",v.buys);a.field("sells",v.sells);a.field("reserveCountdown",v.reserveCountdown);a.field("reserveAmount",v.reserveAmount);a.field("reserves",v.reserves);a.field("relationTimers",v.relationTimers);a.field("relations",v.relations);a.field("state",v.state);a.field("fortified",v.fortified);}
template<class A>void fields(A& a,Context::Trade& v){a.field("partners",v.partners);a.field("counters",v.counters);a.field("dispatchDelay",v.dispatchDelay);a.field("seaEntry",v.seaEntry);a.field("calls",v.calls);a.field("eligible",v.eligible);a.field("timersChanged",v.timersChanged);}
template<class A>void fields(A& a,Context::BuildingTotals& v){a.field("total",v.total);a.field("staffed",v.staffed);a.field("producers",v.producers);a.field("staffedProducers",v.staffedProducers);a.field("calls",v.calls);}
template<class A>void fields(A& a,OriginalFigureStoragePolicy& v){a.field("accept",v.accept);a.field("trade",v.trade);a.field("limit",v.limit);}
template<class A>void fields(A& a,OriginalFigureGoodsServices& v){a.field("policies",v.policies);a.field("unavailable",v.unavailable);a.field("trade7",v.trade7);a.field("trade5",v.trade5);a.field("warehouse",v.warehouse);a.field("savedPhase",v.savedPhase);a.field("storageSearch",v.storageSearch);a.field("industrySearch",v.industrySearch);a.field("godGate",v.godGate);a.field("industryPriority",v.industryPriority);}
template<class A>void fields(A& a,OriginalBuildingGoodsRecord& v){a.field("owner",v.owner);a.field("alive",v.alive);a.field("mode",v.mode);}
template<class A>void fields(A& a,NativeCityContextState& v){
#define NATIVE_CITY_CONTEXT_FIELD(name) a.field(#name,v.name);
    NATIVE_CITY_CONTEXT_MEMBERS(NATIVE_CITY_CONTEXT_FIELD)
#undef NATIVE_CITY_CONTEXT_FIELD
    a.field("workers",v.workers);
}
struct Writer {
    std::ostringstream out;
    template<class T>void value(T& v){
        if constexpr(std::is_integral_v<T>){if constexpr(std::is_signed_v<T>)out<<int64_t(v)<<' ';else out<<uint64_t(v)<<' ';}
        else if constexpr(Optional<T>::value){bool has=bool(v);value(has);if(v)value(*v);}
        else if constexpr(Vector<T>::value||Map<T>::value){size_t n=v.size();value(n);for(auto& item:v){if constexpr(Map<T>::value){auto k=item.first;value(k);value(item.second);}else value(item);}}
        else if constexpr(Array<T>::value){for(auto& item:v)value(item);}
        else fields(*this,v);
    }
    template<class T>void field(const char* name,T& v){out<<name<<' ';value(v);out<<'\n';}
};
struct Reader {
    std::istream& in;bool good=true;size_t allocated=0;std::string current{};
    bool charge(size_t n,size_t unit){if(unit&&n>(nativeCityContextByteLimit-std::min(allocated,nativeCityContextByteLimit))/unit)return false;allocated+=n*unit;return true;}
    size_t vectorLimit()const{return current=="relationTimers"?22u:current=="policies"||current=="goodsRecords"?8192u:current=="cropQuality"?51984u:4000u;}
    template<class T>void value(T& v){
        if(!good)return;
        if constexpr(std::is_integral_v<T>){
            std::string token;if(!(in>>token)){good=false;return;}
            if constexpr(std::is_signed_v<T>){int64_t n=0;const auto r=std::from_chars(token.data(),token.data()+token.size(),n);good=r.ec==std::errc{}&&r.ptr==token.data()+token.size()&&n>=std::numeric_limits<T>::min()&&n<=std::numeric_limits<T>::max();if(good)v=T(n);}
            else{uint64_t n=0;const auto r=std::from_chars(token.data(),token.data()+token.size(),n);good=r.ec==std::errc{}&&r.ptr==token.data()+token.size()&&n<=std::numeric_limits<T>::max();if(good)v=T(n);}
        }else if constexpr(Optional<T>::value){bool has=false;value(has);if(good&&has){typename Optional<T>::value_type x{};value(x);if(good)v=std::move(x);}else if(good)v.reset();}
        else if constexpr(Vector<T>::value){size_t n=0;value(n);if(!good||n>vectorLimit()||!charge(n,sizeof(typename Vector<T>::value_type))){good=false;return;}v.clear();v.reserve(n);for(size_t k=0;k<n&&good;++k){typename Vector<T>::value_type x{};value(x);if(good)v.push_back(std::move(x));}}
        else if constexpr(Map<T>::value){size_t n=0;value(n);if(!good||n>4000||!charge(n,sizeof(typename T::value_type)+64u)){good=false;return;}v.clear();for(size_t k=0;k<n&&good;++k){typename Map<T>::key_type k1{};typename Map<T>::mapped_type x{};value(k1);value(x);if(good&&!v.emplace(k1,std::move(x)).second)good=false;}}
        else if constexpr(Array<T>::value){for(auto& item:v)value(item);}
        else fields(*this,v);
    }
    template<class T>void field(const char* name,T& v){std::string tag;if(!good||!(in>>tag)||tag!=name){good=false;return;}current=name;value(v);}
};
inline bool buildingReference(int64_t n,bool zero=true){return n>=(zero?0:1)&&n<4000;}
inline bool figureReference(int64_t n){return n>=0&&n<2000;}
} // namespace native_city_context_detail

inline NativeCityContextState captureNativeCityContext(const OriginalBuildingUpdateContext& source){
    NativeCityContextState out;
#define NATIVE_CITY_CONTEXT_CAPTURE(name) out.name=source.name;
    NATIVE_CITY_CONTEXT_MEMBERS(NATIVE_CITY_CONTEXT_CAPTURE)
#undef NATIVE_CITY_CONTEXT_CAPTURE
    if(source.buildings)for(const auto& entry:*source.buildings)if(entry.second.type==33||entry.second.type==72||entry.second.type==124){
        const auto* inputs=source.buildingInputs?&*source.buildingInputs:nullptr;
        const auto found=inputs?inputs->find(entry.first):std::map<size_t,OriginalBuildingUpdateInputs>::const_iterator{};
        out.workers.emplace(entry.first,inputs&&found!=inputs->end()?found->second.workers:int16_t(-1));
    }
    return out;
}
// Precondition: validateNativeCityContext was successful. This only applies
// owned fields, leaving all existing authoritative pools/models/Labor untouched.
inline void applyNativeCityContext(const NativeCityContextState& source,OriginalBuildingUpdateContext& target){
#define NATIVE_CITY_CONTEXT_APPLY(name) target.name=source.name;
    NATIVE_CITY_CONTEXT_MEMBERS(NATIVE_CITY_CONTEXT_APPLY)
#undef NATIVE_CITY_CONTEXT_APPLY
}

inline std::string validateNativeCityContext(const NativeCityContextState& s){
    using namespace native_city_context_detail;
    if(s.schemaVersion!=nativeCityContextVersion)return "city_context_version";
    if(s.millCandidateCount>100)return "city_context_mill_count";
    for(size_t i=0;i<s.millCandidateCount;++i)if(!buildingReference(s.millCandidates[i]))return "city_context_mill_reference";
    for(const auto& e:s.workers)if(!buildingReference(int64_t(e.first),false)||e.second<0)return "city_context_worker_reference";
    if(s.workers.size()>3999)return "city_context_worker_count";
    const auto registry=[](const auto& n){return !n||*n<=4000;};
    if(!registry(s.highestBuildingSlot)||!registry(s.highestEverBuildingSlot)||!registry(s.buildingRegistrySize)||(s.highestBuildingSlot&&*s.highestBuildingSlot>=4000)||(s.highestEverBuildingSlot&&*s.highestEverBuildingSlot>=4000))return "city_context_building_registry";
    if(s.goodsRegistryLast&&*s.goodsRegistryLast>=8192)return "city_context_goods_registry";
    if(s.storageRoadStatus){if(s.storageRoadStatus->size()>4000)return "city_context_storage_count";for(const auto& e:*s.storageRoadStatus)if(!buildingReference(int64_t(e.first)))return "city_context_storage_reference";}
    // Inactive records carry no live graph edge. Preserve their typed stale
    // values exactly; only active owners/partners are semantic references.
    if(s.goodsRecords){if(s.goodsRecords->size()>8192)return "city_context_goods_count";for(const auto& r:*s.goodsRecords)if(r.alive&&!buildingReference(r.owner,false))return "city_context_goods_owner";}
    if(s.goods){const auto& g=*s.goods;if(g.policies.size()>8192||g.savedPhase>50)return "city_context_goods_bounds";}
    if(s.cycleTail){const auto& t=*s.cycleTail;if(t.cropQuality.size()>51984)return "city_context_crop_quality_count";if(t.smoothedHealth<0||t.smoothedHealth>100)return "city_context_health_range";}
    if(s.statistics){const auto& x=*s.statistics;if(!buildingReference(x.primaryBuilding)||!buildingReference(x.alternateBuilding)||x.currentFaction< -1||x.currentFaction>=22)return "city_context_statistics_reference";}
    if(s.religion){const auto& r=*s.religion;if(r.selected&&(*r.selected< -1||*r.selected>=4))return "city_context_religion_selected";for(const auto& w:r.worship)for(const auto& e:w.entries)if(e.minimum>e.maximum||e.type<0||e.type>=12)return "city_context_religion_entry";}
    if(s.trade){for(const auto& p:s.trade->partners){if(p.relationTimers.size()>22||(p.alive&&p.faction>=22))return "city_context_trade_bounds";if(p.alive){for(auto id:p.traders)if(!figureReference(id))return "city_context_trade_figure_reference";for(auto r:p.buys)if(r>=36)return "city_context_trade_resource";for(auto r:p.sells)if(r>=36)return "city_context_trade_resource";}}}
    if(s.storageSupplierMismatch.size()>4000||s.farmerTasks.size()>1999||s.supplierRequests.size()>1999)return "city_context_request_count";
    for(const auto& e:s.storageSupplierMismatch)if(!buildingReference(int64_t(e.first),false))return "city_context_supplier_building_reference";
    for(const auto& e:s.farmerTasks)if(!figureReference(int64_t(e.first))||!e.first||e.second>2)return "city_context_farmer_reference";
    for(const auto& e:s.supplierRequests)if(!figureReference(int64_t(e.first))||!e.first)return "city_context_supplier_figure_reference";
    return {};
}
inline std::optional<std::string> encodeNativeCityContext(const NativeCityContextState& source,std::string& error){
    error=validateNativeCityContext(source);if(!error.empty())return std::nullopt;
    auto copy=source;native_city_context_detail::Writer a;a.out<<"NATIVE-CITY-CONTEXT "<<nativeCityContextVersion<<'\n';native_city_context_detail::fields(a,copy);a.out<<"END-NATIVE-CITY-CONTEXT\n";
    auto bytes=a.out.str();if(bytes.size()>nativeCityContextByteLimit){error="city_context_byte_limit";return std::nullopt;}error.clear();return bytes;
}
inline std::optional<NativeCityContextState> decodeNativeCityContext(const std::string& bytes,std::string& error){
    error="city_context_wire";if(bytes.empty()||bytes.size()>nativeCityContextByteLimit||bytes.find('\0')!=std::string::npos)return std::nullopt;
    std::istringstream in(bytes);std::string magic;uint32_t version=0;if(!(in>>magic>>version)||magic!="NATIVE-CITY-CONTEXT"||version!=nativeCityContextVersion)return std::nullopt;
    NativeCityContextState out;out.schemaVersion=version;native_city_context_detail::Reader a{in};native_city_context_detail::fields(a,out);std::string end;if(!a.good||!(in>>end)||end!="END-NATIVE-CITY-CONTEXT")return std::nullopt;in>>std::ws;if(!in.eof())return std::nullopt;
    error=validateNativeCityContext(out);if(!error.empty())return std::nullopt;error.clear();return out;
}
#undef NATIVE_CITY_CONTEXT_MEMBERS
} // namespace game_lab
