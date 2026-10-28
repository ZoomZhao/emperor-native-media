// SPDX-License-Identifier: MIT
// Included inside game_lab after the native DTO definitions. This is a fixed,
// named field schema, never an object-memory dump. Integers are range checked
// before narrowing; counted allocations and the enclosing document are bounded.
namespace native_runtime_wire {
template<class T> struct Optional:std::false_type{};
template<class T> struct Optional<std::optional<T>>:std::true_type{using value_type=T;};
template<class T> struct Vector:std::false_type{};
template<class T,class A> struct Vector<std::vector<T,A>>:std::true_type{using value_type=T;};
template<class T> struct Map:std::false_type{};
template<class K,class V,class C,class A> struct Map<std::map<K,V,C,A>>:std::true_type{using key_type=K;using mapped_type=V;};
template<class T> struct Array:std::false_type{};
template<class T,size_t N> struct Array<std::array<T,N>>:std::true_type{};
template<class T> struct Pair:std::false_type{};
template<class A,class B> struct Pair<std::pair<A,B>>:std::true_type{};
template<class A> void fields(A& a,NativeHouseSchedule& v){a.field("phase",v.phase);a.field("cycle",v.cycle);a.field("monthPart",v.monthPart);a.field("month",v.month);a.field("origin",v.origin);a.field("tailSkipped",v.cycleWrapTailSkipped);}
template<class A> void fields(A& a,NativeHouseAutomation& v){a.field("cachedWater",v.cachedWater);a.field("well",v.wellId);a.field("delay",v.delay);a.field("routeCursor",v.routeCursor);a.field("held",v.held);a.field("reason",v.holdReason);}
template<class A> void fields(A& a,NativeWellRuntime& v){a.field("primary",v.primaryFigure);a.field("spawn",v.spawnCounter);a.field("direction",v.roamDirection);}
template<class A> void fields(A& a,NativeFigureRuntimeState::SavedServices& v){a.field("formation",v.formationImpacts);a.field("routeOwners",v.routeOwners);a.field("directions",v.directions);a.field("randomFirst",v.randomFirst);a.field("randomSecond",v.randomSecond);a.field("ring",v.randomRing);a.field("cursor",v.randomCursor);a.field("previous",v.randomPrevious);a.field("fires",v.fireCount);a.field("criminals",v.criminalCount);a.field("allocation",v.allocationSerial);a.field("serviceGrid",v.serviceGrid);}
template<class A> void fields(A& a,NativeFigureRuntimeState& v){a.field("walkers",v.wellWalkerPayloads);a.field("cellHeads",v.cellHeads);a.field("freeFIFO",v.freeFigureSlots);a.field("highest",v.highestFigureSlot);a.field("services",v.services);a.field("speechCounters",v.speechCounters);a.field("speechCounts",v.speechCounts);a.field("speechChoices",v.speechChoices);}
template<class A> void fields(A& a,NativeObservedDryState& v){a.field("pendingExcess",v.pendingExcess);a.field("departedSinceMeal",v.departedSinceMeal);a.field("arrivedSinceMeal",v.arrivedSinceMeal);}
template<class A> void fields(A& a,OriginalBuildingUpdateContext::Labor& v){a.field("sentiment",v.sentiment);a.field("health",v.health);a.field("migration",v.migration);a.field("incoming",v.incoming);a.field("outgoing",v.outgoing);a.field("inRemainder",v.inRemainder);a.field("outRemainder",v.outRemainder);a.field("inDelay",v.inDelay);a.field("outDelay",v.outDelay);a.field("arrivals",v.arrivals);a.field("departures",v.departures);a.field("unplaced",v.unplaced);a.field("yearArrivals",v.yearArrivals);a.field("spacing",v.spacing);a.field("statisticsPending",v.statisticsPending);a.field("populationSum",v.populationSum);a.field("sampleCount",v.sampleCount);a.field("averagePopulation",v.averagePopulation);a.field("growthCursor",v.growthCursor);a.field("deathCursor",v.deathCursor);a.field("lastPopulationDelta",v.lastPopulationDelta);a.field("excessDepartureTotal",v.excessDepartureTotal);a.field("difficulty",v.difficulty);a.field("wageBand",v.wageBand);a.field("populationNotices",v.populationNotices);a.field("hostileCount",v.hostileCount);a.field("formationLabor",v.formationLabor);}
template<class A> void fields(A& a,OriginalBuildingCityEmployment::Row& v){a.field("demand",v.demand);a.field("allocated",v.allocated);a.field("preferred",v.preferred);a.field("preferredAllocated",v.preferredAllocated);a.field("priority",v.priority);}
template<class A> void fields(A& a,OriginalBuildingCityEmployment& v){a.field("rows",v.rows);a.field("available",v.available);a.field("employed",v.employed);a.field("unemployed",v.unemployed);a.field("unemploymentPercent",v.unemploymentPercent);a.field("shortage",v.shortage);a.field("shortageNotice",v.shortageNotice);a.field("unemploymentNotice",v.unemploymentNotice);}
template<class A> void fields(A& a,NativePopulationRuntime& v){a.field("labor",v.labor);a.field("employment",v.employment);a.field("admitted",v.admitted);a.field("arrived",v.arrived);a.field("unplaced",v.unplaced);a.field("populationArrived",v.populationArrived);a.field("populationDeparted",v.populationDeparted);}
template<class A> void fields(A& a,NativeInspectionVisit& v){a.field("tick",v.tick);a.field("figure",v.figure);a.field("collapseBefore",v.collapseBefore);a.field("ignitionBefore",v.ignitionBefore);a.field("collapseAfter",v.collapseAfter);a.field("ignitionAfter",v.ignitionAfter);}
template<class A> void fields(A& a,NativeInspectionRuntime& v){a.field("payloads",v.payloads);a.field("maximum",v.maximum);a.field("decayCounter",v.decayCounter);a.field("dispatches",v.dispatches);a.field("returns",v.returns);a.field("services",v.services);a.field("visits",v.visits);}
struct Writer {
    std::ostringstream out;
    template<class T> void value(T& v){
        if constexpr(std::is_same_v<T,std::string>)out<<std::quoted(v)<<' ';
        else if constexpr(std::is_enum_v<T>){auto n=static_cast<std::underlying_type_t<T>>(v);value(n);}
        else if constexpr(std::is_integral_v<T>){if constexpr(std::is_signed_v<T>)out<<int64_t(v)<<' ';else out<<uint64_t(v)<<' ';}
        else if constexpr(Optional<T>::value){bool has=bool(v);value(has);if(v)value(*v);}
        else if constexpr(Vector<T>::value||Map<T>::value){size_t n=v.size();value(n);for(auto& item:v){if constexpr(Map<T>::value){auto key=item.first;value(key);value(item.second);}else value(item);}}
        else if constexpr(Array<T>::value){for(auto& item:v)value(item);}
        else if constexpr(Pair<T>::value){value(v.first);value(v.second);}
        else fields(*this,v);
    }
    template<class T> void field(const char* name,T& v){out<<name<<' ';value(v);out<<'\n';}
};
struct Reader {
    std::istream& in;bool good=true;size_t allocated=0;size_t maximumString=262144;
    template<class T> void value(T& v){
        if(!good)return;
        if constexpr(std::is_same_v<T,std::string>){
            // Quoted strings may contain newlines; the whole document is bounded.
            good=bool(in>>std::quoted(v))&&v.size()<=maximumString&&v.find('\0')==std::string::npos;
            allocated+=v.size();if(allocated>nativeSessionRuntimeByteLimit)good=false;
        }else if constexpr(std::is_enum_v<T>){std::underlying_type_t<T> n{};value(n);if(good)v=static_cast<T>(n);}
        else if constexpr(std::is_integral_v<T>){
            std::string token;if(!(in>>token)){good=false;return;}
            if constexpr(std::is_signed_v<T>){int64_t n=0;const auto r=std::from_chars(token.data(),token.data()+token.size(),n);good=r.ec==std::errc{}&&r.ptr==token.data()+token.size()&&n>=std::numeric_limits<T>::min()&&n<=std::numeric_limits<T>::max();if(good)v=T(n);}
            else{uint64_t n=0;const auto r=std::from_chars(token.data(),token.data()+token.size(),n);good=r.ec==std::errc{}&&r.ptr==token.data()+token.size()&&n<=std::numeric_limits<T>::max();if(good)v=T(n);}
        }else if constexpr(Optional<T>::value){bool has=false;value(has);if(good&&has){typename Optional<T>::value_type item{};value(item);if(good)v=std::move(item);}else if(good)v.reset();}
        else if constexpr(Vector<T>::value){size_t n=0;value(n);if(!good||n>500000||n>(nativeSessionRuntimeByteLimit-std::min(allocated,nativeSessionRuntimeByteLimit))/sizeof(typename Vector<T>::value_type)){good=false;return;}allocated+=n*sizeof(typename Vector<T>::value_type);v.clear();v.reserve(n);for(size_t k=0;k<n&&good;++k){typename Vector<T>::value_type item{};value(item);if(good)v.push_back(std::move(item));}}
        else if constexpr(Map<T>::value){size_t n=0;value(n);if(!good||n>4000){good=false;return;}v.clear();for(size_t k=0;k<n&&good;++k){typename Map<T>::key_type key{};typename Map<T>::mapped_type item{};value(key);value(item);if(good&&!v.emplace(key,std::move(item)).second)good=false;}}
        else if constexpr(Array<T>::value){for(auto& item:v)value(item);}
        else if constexpr(Pair<T>::value){value(v.first);value(v.second);}
        else fields(*this,v);
    }
    template<class T> void field(const char* name,T& v){std::string tag;if(!good||!(in>>tag)||tag!=name){good=false;return;}value(v);}
};
template<class A> void stateFields(A& a,NativeSessionState& s){
    a.field("schedule",s.houseSchedule);a.field("figures",s.figureRuntime);a.field("hook",s.buildingHookCountdown);
    a.field("heldHouses",s.heldHouseReasons);a.field("heldWalkers",s.heldWalkerReasons);
    a.field("strictRecords",s.strictRecords);a.field("strictContext",s.strictContext);a.field("policies",s.runtimePolicies);
}
// The original state/figure schemas above are frozen for RUNTIME8 1. The
// additive dry policy is written separately, never inserted into v1 figures.
inline bool validMigrantPayloadFraming(const NativeSessionState& s){
    if(!s.figureRuntime)return !s.observedDry;
    const auto& f=*s.figureRuntime;
    if(!s.observedDry&&!f.migrantPayloads.empty())return false;
    if(!s.populationRuntime&&!f.immigrantPayloads.empty())return false;
    if(f.wellWalkerPayloads.size()+f.migrantPayloads.size()+f.immigrantPayloads.size()>1999)return false;
    for(const auto& p:f.immigrantPayloads)if(p.empty()||p.size()>16384||p.find('\0')!=std::string::npos)return false;
    for(const auto& p:f.migrantPayloads)if(p.empty()||p.size()>16384||p.find('\0')!=std::string::npos)return false;
    return true;
}
inline bool hasNonemptyMigrantTag(const NativeSessionState& s){
    return std::find(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeMigrantNonemptyTag)!=s.runtimePolicies.end();
}
inline void encode(std::ostream& target,const NativeSessionState& source){
    if(!validMigrantPayloadFraming(source)){target.setstate(std::ios::failbit);return;}
    const bool nonempty=nativeUsesNonemptyMigrantRehousing(source);
    if(hasNonemptyMigrantTag(source)&&(!nonempty||!source.observedDry)){target.setstate(std::ios::failbit);return;}
    const bool rewater=nativeUsesObservedRewatering(source);
    if(rewater&&(!nonempty||!source.observedDry)){target.setstate(std::ios::failbit);return;}
    if(nativeUsesImmigration(source)!=source.populationRuntime.has_value()||(source.populationRuntime&&!rewater)){target.setstate(std::ios::failbit);return;}
    if(nativeUsesCityContext(source)!=source.cityContext.has_value()||(source.cityContext&&!source.populationRuntime)){target.setstate(std::ios::failbit);return;}
    if(nativeUsesInspection(source)!=source.inspection.has_value()||(source.inspection&&!nativeUsesCityContext(source))){target.setstate(std::ios::failbit);return;}
    auto s=source;const bool extended=s.observedDry.has_value();Writer a;
    a.out<<"RUNTIME8 "<<nativeHouseRuntimeWireVersion(s)<<'\n';stateFields(a,s);
    if(extended){a.field("observedDry",s.observedDry);a.field("migrantPayloads",s.figureRuntime->migrantPayloads);}
    if(s.populationRuntime){a.field("populationRuntime",s.populationRuntime);a.field("immigrantPayloads",s.figureRuntime->immigrantPayloads);}
    if(s.cityContext)a.field("cityContext",s.cityContext);
    if(s.inspection)a.field("inspection",s.inspection);
    a.out<<"objects "<<s.objects.size()<<'\n';
    for(auto& o:s.objects){a.field("id",o.id);a.field("automation",o.automation);a.field("well",o.well);}
    a.out<<"ENDRUNTIME8\n";target<<a.out.str();
}
inline bool decode(std::istream& in,NativeSessionState& s){
    Reader a{in};std::string tag;unsigned version=0;
    if(!(in>>tag>>version)||tag!="RUNTIME8"||(version<1||version>7))return false;
    stateFields(a,s);if(!a.good)return false;
    // A version downgrade must not silently enable the newer policy.
    if(version>=3?!nativeUsesNonemptyMigrantRehousing(s):hasNonemptyMigrantTag(s))return false;
    if((version>=4)!=nativeUsesObservedRewatering(s)||(version>=5)!=nativeUsesImmigration(s)||(version>=6)!=nativeUsesCityContext(s)||(version==7)!=nativeUsesInspection(s))return false;
    s.observedDry.reset();if(s.figureRuntime)s.figureRuntime->migrantPayloads.clear();
    if(version>=2){
        if(!s.figureRuntime)return false;
        a.field("observedDry",s.observedDry);if(!a.good||!s.observedDry)return false;
        a.field("migrantPayloads",s.figureRuntime->migrantPayloads);
    }
    s.populationRuntime.reset();if(s.figureRuntime)s.figureRuntime->immigrantPayloads.clear();
    if(version>=5){a.field("populationRuntime",s.populationRuntime);if(!a.good||!s.populationRuntime)return false;a.field("immigrantPayloads",s.figureRuntime->immigrantPayloads);}
    s.cityContext.reset();if(version>=6){a.maximumString=8u*1024u*1024u;a.field("cityContext",s.cityContext);a.maximumString=262144;if(!a.good||!s.cityContext)return false;}
    s.inspection.reset();if(version==7){a.field("inspection",s.inspection);if(!a.good||!s.inspection)return false;}
    if(!a.good||!validMigrantPayloadFraming(s))return false;
    size_t count=0;a.field("objects",count);if(!a.good||count!=s.objects.size())return false;
    for(auto& o:s.objects){uint64_t id=0;a.field("id",id);if(!a.good||id!=o.id)return false;a.field("automation",o.automation);a.field("well",o.well);}
    return a.good&&bool(in>>tag)&&tag=="ENDRUNTIME8";
}
} // namespace native_runtime_wire
