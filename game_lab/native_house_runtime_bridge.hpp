// SPDX-License-Identifier: MIT
#pragma once
// Native v8 DTO <-> full strict OriginalCombinedState conversion. Pure: no IO,
// no asset loading, no retained pointers. Depends on native_session.hpp, the
// phase engine, the walker codec and strict types; never on the native source
// factory (the caller builds NativeStrictTemplate with existing legal helpers).
//
// Ownership (one direction per field):
//  * native DTO owns identity, footprint origin, funds/ledger, residents
//    (native growth) and staffing (native v6 labor plan);
//  * the strict record payload owns every other OriginalBuildingUpdateInputs
//    member; DTO house/well fields are checked projections of it;
//  * strict figure/services/RNG/routes/free FIFO/cell heads/speech state is
//    carried losslessly through NativeFigureRuntimeState + walker payloads.
// strictToNative proves losslessness: nativeToStrict(strictToNative(x)) must
// reproduce x on every persisted/derived field, else a named refusal.
//
// Native-only policies (recorded in `policies`, never presented as original):
//  native_constructed_record_zero_init  - a native object without a strict
//      record gets a zero-initialised record (alive1, identity, model,
//      modelDesirability/bonus/upgrade 0, houseWater 0);
//  native_object_terrain_overlay        - native roads/buildings set terrain
//      bits 0x40 / 0x08 and owners over the template (also on the rebuilt
//      movement terrain) instead of an original scene rebuild;
//  native_labor_v6_workers              - Well/Hunter workers come from the
//      native v6 plan, not strict phase25;
//  native_house_capacity_peak_zero_init - a house without a recorded peak.
#include "native_session.hpp"
#include "native_house_phase_runtime.hpp"
#include "native_walker_codec.hpp"
#include "native_immigrant_codec.hpp"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace game_lab {
// Immutable source inputs, built once per source/model by the caller from the
// existing legal SAV/model/scene helpers (originalCombinedSavedState +
// originalCombinedBindBuildingServices on the empty source city, the scene's
// rebuilt movement terrain, and originalBuildingSavedUpdateContext per type).
struct NativeStrictTemplate {
    uint64_t sourceRevision=0,modelRevision=0;uint32_t side=0;
    OriginalCombinedState emptyCity;                 // no live buildings or figures
    OriginalTerrainState rebuiltSurface;             // scene surface, terrain=rebuilt movement terrain
    OriginalBuildingProfile sourceProfile;           // complete 4000-record empty profile
    std::map<uint16_t,OriginalBuildingUpdateContext> contextPrototypes;  // strict types 2..17,33,72
    std::map<uint16_t,OriginalBuildingProductionInputs> productionPrototypes; // types with production (33)
    // C-only immutable input. Legacy B never loaded/used the Goods service;
    // absence cannot be repaired by inventing a model/policy table.
    std::optional<OriginalFigureGoodsServices> cityGoodsSource;
};
// Additive persistence the v8 DTO does not yet carry (see report to Free).
struct NativeStrictPersisted {
    std::map<uint64_t,std::string> records;   // native object id -> strict record payload
    std::optional<std::string> context;       // desirability grid, house peaks, population
};
struct NativeStrictConversion {
    std::optional<OriginalCombinedState> city;NativeHouseIsolation isolation;
    std::vector<std::string> policies;std::string refusal;
};
struct NativeStrictProjection {
    std::optional<NativeSessionState> state;NativeStrictPersisted persisted;std::string refusal;
};

namespace native_bridge_detail {
using native_walker_detail::parseInteger;
inline const char* hexDigits(){return "0123456789abcdef";}
template<size_t N> std::string hex(const std::array<uint8_t,N>& a){std::string s;s.reserve(2*N);for(auto b:a){s+=hexDigits()[b>>4];s+=hexDigits()[b&15];}return s;}
inline int nibble(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
template<size_t N> bool unhex(std::string_view s,std::array<uint8_t,N>& a){if(s.size()!=2*N)return false;
    for(size_t n=0;n<N;++n){const int h=nibble(s[2*n]),l=nibble(s[2*n+1]);if(h<0||l<0)return false;a[n]=uint8_t(h*16+l);}return true;}
// ---- strict record payload (OriginalBuildingUpdateInputs, full member list) ----
template<class V,class I> void visitInputs(V& v,I& i){
    v("alive",i.alive);v("output",i.output);v("byte52",i.byte52);v("upgrade",i.upgrade);v("identity",i.identity);
    v("industry",i.industry);v("house",i.house);
    v("model",i.model);v("residents",i.residents);v("free",i.free);v("distance",i.distance);v("roadX",i.roadX);v("roadY",i.roadY);
    v("migrant",i.migrant);v("workers",i.workers);v("available",i.available);v("random",i.random);v("network",i.network);
    v("laborCountdown",i.laborCountdown);v("parentSlot",i.parentSlot);v("operational",i.operational);v("parent",i.parent);v("suppression",i.suppression);
    v("laborDisabled",i.laborDisabled);v("enabled",i.enabled);v("employmentCategory",i.employmentCategory);
    v("houseWater",i.houseWater);v("houseService",i.houseService);v("spawnCounter",i.spawnCounter);v("spawnAux",i.spawnAux);v("roadFailure",i.roadFailure);
    v("primaryFigure",i.primaryFigure);v("word30",i.word30);v("word34",i.word34);v("hookIndustry",i.hookIndustry);v("stock",i.stock);
    v("faction",i.faction);v("direction",i.direction);v("houseInUse",i.houseInUse);v("desirability",i.desirability);v("desirabilityBonus",i.desirabilityBonus);
    v("fireRisk",i.fireRisk);v("damageRisk",i.damageRisk);v("burning",i.burning);v("word92",i.word92);
    v("fireproof",i.fireproof);v("fireDirection",i.fireDirection);v("fireSuppressed",i.fireSuppressed);v("watchRisk",i.watchRisk);
    v("modelDesirability",i.modelDesirability);v("modelUpgrade",i.modelUpgrade);v("modelDesirabilityBonus",i.modelDesirabilityBonus);
    v("cropWorkers",i.cropWorkers);v("cropBays",i.cropBays);v("stallResourceIndex",i.stallResourceIndex);v("supplierStatus",i.supplierStatus);v("nextPart",i.nextPart);
}
template<class T> struct IsOpt:std::false_type{};template<class T> struct IsOpt<std::optional<T>>:std::true_type{using type=T;};
template<class T> struct IsByteArray:std::false_type{};template<size_t N> struct IsByteArray<std::array<uint8_t,N>>:std::true_type{};
template<class T> std::string valueText(const T& v){
    if constexpr(IsOpt<T>::value){if(!v)return "-";return valueText(*v);}
    else if constexpr(IsByteArray<T>::value)return hex(v);
    else if constexpr(std::is_same_v<T,std::array<int32_t,2>>)return std::to_string(v[0])+","+std::to_string(v[1]);
    else if constexpr(std::is_same_v<T,std::array<OriginalStorageCompartment,5>>){std::string s;
        for(size_t n=0;n<5;++n){if(n)s+=',';s+=std::to_string(v[n].resource)+","+std::to_string(v[n].amount)+","+std::to_string(v[n].capacity);}return s;}
    else if constexpr(std::is_signed_v<T>)return std::to_string(int64_t(v));
    else return std::to_string(uint64_t(v));
}
inline std::vector<std::string_view> splitComma(std::string_view s){std::vector<std::string_view> out;size_t start=0;
    for(;;){const auto at=s.find(',',start);out.push_back(s.substr(start,at==std::string_view::npos?std::string_view::npos:at-start));if(at==std::string_view::npos)break;start=at+1;}return out;}
template<class T> bool parseValue(std::string_view t,T& v){
    if constexpr(IsOpt<T>::value){if(t=="-"){v.reset();return true;}typename IsOpt<T>::type inner{};if(!parseValue(t,inner))return false;v=inner;return true;}
    else if constexpr(IsByteArray<T>::value)return unhex(t,v);
    else if constexpr(std::is_same_v<T,std::array<int32_t,2>>){const auto p=splitComma(t);return p.size()==2&&parseInteger(p[0],v[0])&&parseInteger(p[1],v[1]);}
    else if constexpr(std::is_same_v<T,std::array<OriginalStorageCompartment,5>>){const auto p=splitComma(t);if(p.size()!=15)return false;
        for(size_t n=0;n<5;++n)if(!parseInteger(p[3*n],v[n].resource)||!parseInteger(p[3*n+1],v[n].amount)||!parseInteger(p[3*n+2],v[n].capacity))return false;return true;}
    else return parseInteger(t,v);
}
struct RecordWriter{std::string out;size_t n=0;template<class T> void operator()(const char* name,const T& v){out+=name;out+=' ';out+=valueText(v);out+='\n';++n;}};
struct RecordCounter{size_t n=0;template<class T> void operator()(const char*,const T&){++n;}};
struct RecordReader{std::vector<std::pair<std::string_view,std::string_view>> lines;size_t at=0;std::string error;
    template<class T> void operator()(const char* name,T& v){if(!error.empty())return;
        if(at>=lines.size()){error=std::string("missing_field:")+name;return;}
        const auto& [k,t]=lines[at++];if(k!=name){error=std::string("field_order:")+name;return;}
        if(!parseValue(t,v))error=std::string("invalid_value:")+name;}};
inline size_t recordFieldCount(){OriginalBuildingUpdateInputs i;RecordCounter c;visitInputs(c,i);return c.n;}
inline constexpr const char* recordMagic="NATIVE-STRICT-RECORD 1 ";
inline bool splitLines(const std::string& bytes,std::vector<std::string_view>& rows,size_t limit){
    if(bytes.size()>limit||bytes.empty()||bytes.back()!='\n'||bytes.find('\0')!=std::string::npos||bytes.find('\r')!=std::string::npos)return false;
    const std::string_view all(bytes);size_t start=0;while(start<all.size()){const size_t end=all.find('\n',start);rows.push_back(all.substr(start,end-start));start=end+1;}return true;}
} // namespace native_bridge_detail

inline std::string encodeNativeStrictRecord(const OriginalBuildingUpdateInputs& inputs){
    using namespace native_bridge_detail;RecordWriter w;w.out=std::string(recordMagic)+std::to_string(recordFieldCount())+'\n';
    auto copy=inputs;visitInputs(w,copy);return w.out;}
inline std::optional<OriginalBuildingUpdateInputs> decodeNativeStrictRecord(const std::string& bytes,std::string& error){
    using namespace native_bridge_detail;std::vector<std::string_view> rows;
    if(!splitLines(bytes,rows,65536)){error="strict_record_framing";return std::nullopt;}
    if(rows.front()!=std::string(recordMagic)+std::to_string(recordFieldCount())){error="strict_record_version";return std::nullopt;}
    RecordReader r;for(size_t n=1;n<rows.size();++n){const auto sp=rows[n].find(' ');
        if(sp==std::string_view::npos||sp==0||rows[n].find(' ',sp+1)!=std::string_view::npos){error="strict_record_line";return std::nullopt;}
        r.lines.emplace_back(rows[n].substr(0,sp),rows[n].substr(sp+1));}
    if(r.lines.size()!=recordFieldCount()){error="strict_record_field_count";return std::nullopt;}
    OriginalBuildingUpdateInputs i;visitInputs(r,i);if(!r.error.empty()){error="strict_record_"+r.error;return std::nullopt;}return i;}

// ---- strict context payload ----
struct NativeStrictContextState {
    std::optional<std::vector<int8_t>> desirability;std::map<size_t,int16_t> housePeaks;
    std::optional<OriginalFigurePopulation> population;
    std::optional<std::pair<int32_t,int32_t>> laborCounters; // last delta, excess departures
    std::optional<uint64_t> nativePopulation;
};
inline std::string encodeNativeStrictContext(const NativeStrictContextState& s){
    const bool extended=s.nativePopulation.has_value()||s.laborCounters.has_value();
    std::string out=extended?"NATIVE-STRICT-CONTEXT 2\ndesirability ":"NATIVE-STRICT-CONTEXT 1\ndesirability ";
    if(!s.desirability)out+='-';else for(int8_t v:*s.desirability){const uint8_t b=uint8_t(v);out+=native_bridge_detail::hexDigits()[b>>4];out+=native_bridge_detail::hexDigits()[b&15];}
    out+="\npeaks "+std::to_string(s.housePeaks.size())+'\n';for(const auto& [slot,peak]:s.housePeaks)out+=std::to_string(slot)+' '+std::to_string(peak)+'\n';
    out+="population ";if(!s.population)out+='-';else out+=std::to_string(s.population->total)+' '+std::to_string(s.population->peak)+' '+(s.population->lastArrival?std::to_string(*s.population->lastArrival):std::string("-"));
    out+='\n';
    if(extended){out+="labor ";if(s.laborCounters)out+=std::to_string(s.laborCounters->first)+' '+std::to_string(s.laborCounters->second);else out+='-';
        out+=" nativePopulation "+(s.nativePopulation?std::to_string(*s.nativePopulation):std::string("-"))+'\n';}
    return out;}
inline std::optional<NativeStrictContextState> decodeNativeStrictContext(const std::string& bytes,std::string& error){
    using namespace native_bridge_detail;std::vector<std::string_view> rows;NativeStrictContextState s;error="strict_context_malformed";
    if(!splitLines(bytes,rows,256*1024)||rows.size()<4||(rows[0]!="NATIVE-STRICT-CONTEXT 1"&&rows[0]!="NATIVE-STRICT-CONTEXT 2"))return std::nullopt;
    if(rows[1].rfind("desirability ",0)!=0)return std::nullopt;const auto d=rows[1].substr(13);
    if(d!="-"){if(d.size()!=2*51984)return std::nullopt;s.desirability.emplace(51984);
        for(size_t n=0;n<51984;++n){const int h=nibble(d[2*n]),l=nibble(d[2*n+1]);if(h<0||l<0)return std::nullopt;(*s.desirability)[n]=int8_t(uint8_t(h*16+l));}}
    if(rows[2].rfind("peaks ",0)!=0)return std::nullopt;size_t count=0;if(!parseInteger(rows[2].substr(6),count)||count>3999||rows.size()!=4+count+(rows[0]=="NATIVE-STRICT-CONTEXT 2"?1:0))return std::nullopt;
    for(size_t n=0;n<count;++n){const auto row=rows[3+n];const auto sp=row.find(' ');size_t slot=0;int16_t peak=0;
        if(sp==std::string_view::npos||!parseInteger(row.substr(0,sp),slot)||!parseInteger(row.substr(sp+1),peak)||!s.housePeaks.emplace(slot,peak).second)return std::nullopt;}
    const auto p=rows[3+count];if(p.rfind("population ",0)!=0)return std::nullopt;const auto body=p.substr(11);
    if(body!="-"){std::vector<std::string_view> parts;size_t start=0;for(;;){const auto at=body.find(' ',start);parts.push_back(body.substr(start,at==std::string_view::npos?std::string_view::npos:at-start));if(at==std::string_view::npos)break;start=at+1;}
        if(parts.size()!=3)return std::nullopt;OriginalFigurePopulation pop;if(!parseInteger(parts[0],pop.total)||!parseInteger(parts[1],pop.peak))return std::nullopt;
        if(parts[2]!="-"){uint32_t a=0;if(!parseInteger(parts[2],a))return std::nullopt;pop.lastArrival=a;}s.population=pop;}
    if(rows[0]=="NATIVE-STRICT-CONTEXT 2"){
        std::istringstream in{std::string(rows[4+count])};std::string tag,a,b,np,tail;
        if(!(in>>tag>>a)||tag!="labor")return std::nullopt;
        if(a!="-"){int32_t x=0,y=0;if(!(in>>b)||!parseInteger(a,x)||!parseInteger(b,y))return std::nullopt;s.laborCounters=std::make_pair(x,y);}
        if(!(in>>tag>>np)||tag!="nativePopulation"||(in>>tail))return std::nullopt;
        if(np!="-"){uint64_t n=0;if(!parseInteger(np,n)||n>INT32_MAX)return std::nullopt;s.nativePopulation=n;}
    }
    error.clear();return s;}

namespace native_bridge_detail {
inline std::optional<uint16_t> strictType(const NativeSessionObject& o){
    if(o.kind==NativeObjectKind::Inspector)return uint16_t(124);if(o.kind==NativeObjectKind::Well)return uint16_t(72);if(o.kind==NativeObjectKind::Hunter)return uint16_t(33);
    if(o.kind==NativeObjectKind::House&&o.house)return o.house->type;return std::nullopt;}
inline bool kindMatches(const NativeSessionObject& o,uint16_t type){
    return o.kind==NativeObjectKind::House?originalBuildingIsHouse(type):o.kind==NativeObjectKind::Well?type==72:o.kind==NativeObjectKind::Hunter?type==33:o.kind==NativeObjectKind::Inspector?type==124:false;}
inline std::string fieldsOf(const OriginalBuildingUpdateInputs& i){return encodeNativeStrictRecord(i);}
// Everything persisted or derived that a lossless round trip must reproduce.
inline std::vector<std::pair<std::string,std::string>> fingerprint(const OriginalCombinedState& city,const NativeHouseIsolation& iso,bool cityContract=false,bool inspection=false){
    std::vector<std::pair<std::string,std::string>> out;const auto add=[&](std::string k,std::string v){out.emplace_back(std::move(k),std::move(v));};
    const auto& c=*city.buildingContext;const auto& f=city.figures;
    if(cityContract){std::string error;const auto bytes=encodeNativeCityContext(captureNativeCityContext(c),error);add("city_context",bytes?*bytes:"invalid:"+error);}
    add("clock",std::to_string(city.phase)+" "+valueText(c.cycle)+" "+valueText(c.monthPart)+" "+valueText(c.month));
    add("hook",valueText(c.buildingHookCountdown));
    if(inspection){add("inspection_maximum",valueText(f.inspectorMaximum));add("inspection_decay",valueText(c.effectDecayCounter));add("inspection_decay_process_mirror",valueText(city.process.effectDecayCounter));}
    for(const auto& [slot,b]:city.buildings){const auto& s=b.state;const std::string k="building_"+std::to_string(slot);
        std::string fp=std::to_string(s.building.type)+" "+std::to_string(s.building.x)+" "+std::to_string(s.building.y)+" "+std::to_string(s.building.size);
        if(s.building.imageInputs)fp+=" img "+std::to_string(s.building.imageInputs->gridIndex)+" "+std::to_string(s.building.imageInputs->word20)+" "+
            std::to_string(s.building.imageInputs->houseByte3c)+" "+valueText(s.building.imageInputs->randomByte);
        if(s.building.wellImageInputs)fp+=" well "+std::to_string(s.building.wellImageInputs->word16)+" "+std::to_string(s.building.wellImageInputs->byte6f);
        add(k+"_footprint",fp);add(k+"_inputs",fieldsOf(s.inputs));
        if(s.production)add(k+"_production",std::to_string(s.production->workers)+" "+std::to_string(s.production->operational)+" "+std::to_string(s.production->secondaryStock)+" "+std::to_string(s.production->primaryStock));
        const auto& l=f.buildings->at(slot);
        add(k+"_link",std::to_string(l.type)+" "+std::to_string(l.alive)+" "+std::to_string(l.residents)+" "+std::to_string(l.freeSlots)+" "+std::to_string(l.houseModel)+" "+
            std::to_string(l.primaryFigure)+" "+std::to_string(l.worker)+" "+std::to_string(l.roamDirection)+" "+valueText(l.waterService)+" "+valueText(l.religion)+" "+valueText(l.herbalist)+" "+valueText(l.foodQuality)+" "+valueText(l.laborDisabled));
        if(c.houses&&c.houses->count(slot)){const auto& h=c.houses->at(slot);add(k+"_capacity",std::to_string(h.peak)+" "+std::to_string(h.model)+" "+std::to_string(h.residents));}}
    if(f.buildings&&f.buildings->count(0)){add("population_context",valueText(c.populationTotal)+" "+valueText(c.populationPeak));const auto& z=f.buildings->at(0);add("native_zero_link",std::to_string(z.type)+" "+std::to_string(z.alive)+" "+std::to_string(z.worker)+" "+std::to_string(z.primaryFigure)+" "+std::to_string(z.residents)+" "+std::to_string(z.freeSlots));}
    std::string live;for(const auto& [slot,g]:f.figures)if(g.alive){std::string e;const auto p=g.type==11?encodeNativeImmigrant(g,e):g.type==13?encodeNativeMigrant(g,e):g.type==39?encodeNativeInspector(g,e):encodeNativeWellWalker(g,e);live+=p?*p:"invalid:"+e;}
    add("walkers",live);
    if(c.labor&&c.labor->excessDepartureTotal!=0)add("labor_excess",std::to_string(c.labor->lastPopulationDelta)+" "+std::to_string(c.labor->excessDepartureTotal));
    add("ephemeral_well_return_policy",valueText(f.wellReturnObservedHomePolicy));
    {native_runtime_wire::Writer a;if(c.labor){auto v=*c.labor;a.field("labor",v);}if(c.cityEmployment){auto v=*c.cityEmployment;a.field("employment",v);}add("population_scheduler",a.out.str());}
    add("cell_heads",std::string(reinterpret_cast<const char*>(f.cellHeads.data()),f.cellHeads.size()*2));
    {std::string s;for(auto v:f.freeSlots)s+=std::to_string(v)+",";add("free_slots",s);}
    add("highest_live",std::to_string(f.highestLiveSlot));
    if(f.services){const auto& v=*f.services;std::string s;
        for(const auto& i:v.formationImpacts)s+=std::to_string(i.alert)+","+std::to_string(i.source)+";";
        for(auto o:v.routeOwners)s+=std::to_string(o)+",";s+=std::string(v.directions.begin(),v.directions.end());
        s+=" "+std::to_string(v.randomFirst)+" "+std::to_string(v.randomSecond)+" "+valueText(v.fireCount)+" "+valueText(v.criminalCount)+" "+valueText(v.allocationSerial);
        if(v.randomHistory){for(auto r:v.randomHistory->ring)s+=std::to_string(r)+",";s+=std::to_string(v.randomHistory->cursor)+" "+std::to_string(v.randomHistory->previous7);}else s+=" norng";
        for(auto g:v.serviceGrid)s+=std::to_string(g)+",";
        if(v.population)s+=" pop "+std::to_string(v.population->total)+" "+std::to_string(v.population->peak)+" "+valueText(v.population->lastArrival);
        add("services",s);}else add("services","-");
    {std::string s;if(f.speechCounters)for(auto v:*f.speechCounters)s+=std::to_string(v)+",";add("speech_counters",s);}
    {std::string s;for(const auto& [k2,v]:c.figureSpeechChoices)s+=std::to_string(k2)+":"+std::to_string(v)+",";add("speech_choices",s);}
    {std::string s;if(c.desirability)s.assign(reinterpret_cast<const char*>(c.desirability->data()),c.desirability->size());else s="-";add("desirability",s);}
    {std::string s;for(const auto& [k2,v]:iso.heldWalkers)s+=std::to_string(k2)+":"+v+",";add("held_walkers",s);}
    {std::string s;for(const auto& [k2,v]:iso.blockedHouses)s+=std::to_string(k2)+":"+v+",";add("blocked_houses",s);}
    return out;
}
} // namespace native_bridge_detail

inline NativeStrictConversion nativeToStrict(const NativeSessionSeed& seed,const NativeSessionState& state,
    const NativeStrictTemplate& tpl,const NativeStrictPersisted& persisted){
    using namespace native_bridge_detail;NativeStrictConversion out;
    const auto refuse=[&](std::string why){out.refusal=std::move(why);out.city.reset();out.isolation={};return out;};
    // ---- admission ----
    if(state.rulesVersion!=8)return refuse("bridge_requires_native_v8");
    if(auto e=validateNativeSession(seed,state);!e.empty())return refuse("native_state_invalid:"+e);
    if(tpl.sourceRevision!=seed.sourceRevision||tpl.modelRevision!=seed.modelRevision)return refuse("template_source_or_model_revision_mismatch");
    if(tpl.side!=seed.side||!seed.side||seed.side>228)return refuse("template_side_mismatch");
    const auto& empty=tpl.emptyCity;
    if(!empty.buildings.empty())return refuse("template_has_live_buildings");
    if(empty.figures.wellReturnObservedHomePolicy)return refuse("template_has_ephemeral_well_return_policy");
    if(!empty.buildingContext)return refuse("template_building_context_required");
    if(empty.figures.figures.size()!=2000)return refuse("template_figure_pool_required");
    for(const auto& [slot,g]:empty.figures.figures)if(g.alive||g.slot!=slot)return refuse("template_figure_pool_not_empty");
    if(!empty.figures.passability||!empty.figures.services||!empty.figures.buildings||!empty.figures.cropBase||!empty.figures.activeSide)
        return refuse("template_figure_services_required");
    const uint32_t origin=(228-seed.side)/2,cropBase=origin*229;
    if(*empty.figures.cropBase!=cropBase||*empty.figures.activeSide!=seed.side)return refuse("template_crop_mismatch");
    const auto& ec=*empty.buildingContext;
    if(!ec.terrainOwners||ec.terrainOwners->size()!=51984||!ec.terrainRandom||ec.terrainRandom->size()!=51984||!ec.roadEndpoints||
       empty.figures.terrain.size()!=51984||empty.figures.buildingOwners.size()!=51984)return refuse("template_terrain_layers_required");
    if(!tpl.rebuiltSurface.valid()||tpl.sourceProfile.status!=OriginalBuildingStatus::Complete||tpl.sourceProfile.parsedRecords!=4000)
        return refuse("template_rebuilt_surface_required");
    const auto& sc=*state.houseSchedule;const auto& fr=*state.figureRuntime;
    if(!fr.highestFigureSlot)return refuse("figure_highest_slot_unknown");
    if(!state.buildingHookCountdown)return refuse("building_hook_countdown_unknown");
    if(!empty.figures.speechCounts&&!ec.figureSpeechCounts)return refuse("template_speech_counts_required");
    if(ec.figureSpeechCounts&&*ec.figureSpeechCounts!=fr.speechCounts)return refuse("speech_counts_source_mismatch");
    NativeStrictContextState extra;
    if(persisted.context){std::string e;const auto d=decodeNativeStrictContext(*persisted.context,e);if(!d)return refuse(e);extra=*d;}
    for(const auto& entry:persisted.records){
        const auto id=entry.first;const auto& payload=entry.second;(void)payload;
        const auto o=std::find_if(state.objects.begin(),state.objects.end(),[&](const auto& x){return x.id==id;});
        if(o==state.objects.end()||o->kind==NativeObjectKind::Road)return refuse("strict_record_without_native_object_"+std::to_string(id));}
    // ---- assemble ----
    auto city=empty;auto& c=*city.buildingContext;auto& f=city.figures;bool zeroInit=false,peakInit=false;
    std::optional<NativeCityContextState> cityState;
    if(nativeUsesCityContext(state)){
        std::string error;cityState=decodeNativeCityContext(*state.cityContext,error);if(!cityState)return refuse("city_context:"+error);
        applyNativeCityContext(*cityState,c);f.goods=c.goods;
        c.normalizedHouseGoods=true;c.employmentCategoryOverrides=std::map<uint16_t,int>{{4,-1}};
        out.policies.push_back("native_type4_no_jobs_adaptation_v1");
    }
    city.phase=sc.phase;c.phase=sc.phase;c.cycle=sc.cycle;c.monthPart=sc.monthPart;c.month=sc.month;
    if(sc.origin==NativeHouseClockOrigin::NativeConstructedZeroInit)out.policies.push_back("native_constructed_clock_zero_init");
    if(sc.cycleWrapTailSkipped)out.policies.push_back("native_cycle_wrap_tail_skipped");
    c.buildingHookCountdown=*state.buildingHookCountdown;
    if(state.observedDry){
        if(!extra.nativePopulation||!extra.laborCounters||!c.labor)return refuse("observed_dry_context_missing");
        c.labor->lastPopulationDelta=extra.laborCounters->first;c.labor->excessDepartureTotal=extra.laborCounters->second;
    }
    if(state.populationRuntime){
        if(!extra.laborCounters||extra.laborCounters->first!=state.populationRuntime->labor.lastPopulationDelta||extra.laborCounters->second!=state.populationRuntime->labor.excessDepartureTotal||!extra.nativePopulation||*extra.nativePopulation!=nativePopulation(state)||!extra.population||extra.population->total!=int64_t(nativePopulation(state)))return refuse("immigration_context_mirror_mismatch");
        c.labor=state.populationRuntime->labor;c.cityEmployment=state.populationRuntime->employment;
        // Template wildlife/formations belong to the discarded source pool.
        // This native constructor owns no military figures or formations.
        c.formations=std::array<OriginalBuildingFormation,100>{};
        city.process.migrantSpacing=c.labor->spacing;
        f.buildingEditMode=false; // constructed native game, never an editor
    }
    auto surface=tpl.rebuiltSurface;auto profile=tpl.sourceProfile;
    if(!c.resourceGrowth||c.resourceGrowth->owners.size()!=51984)return refuse("template_resource_growth_required");
    // Terrain/ownership overlay (native policy).
    for(const auto& o:state.objects){const unsigned n=nativeObjectSide(o.kind);
        for(unsigned y=o.y;y<o.y+n;++y)for(unsigned x=o.x;x<o.x+n;++x){const size_t cell=cropBase+228*y+x;if(cell>=51984)return refuse("native_object_outside_crop");
            if((f.terrain[cell]&0x48)||f.buildingOwners[cell]||ec.terrainOwners->at(cell))return refuse("native_object_overlaps_source_cell_"+std::to_string(cell));
            if(o.kind==NativeObjectKind::Road){f.terrain[cell]|=0x40;surface.terrain[cell]|=0x40;c.resourceGrowth->surface.terrain[cell]|=0x40;c.resourceGrowth->surface.roadDistance[cell]=255;}
            else{if(o.id>=4000)return refuse("native_id_outside_building_registry");
                f.terrain[cell]|=8;surface.terrain[cell]|=8;c.resourceGrowth->surface.terrain[cell]|=8;
                f.buildingOwners[cell]=uint16_t(o.id);c.terrainOwners->at(cell)=uint16_t(o.id);c.resourceGrowth->owners[cell]=uint16_t(o.id);
                if(c.irrigation&&c.irrigation->owners.size()==51984)c.irrigation->owners[cell]=uint16_t(o.id);}}}
    out.policies.push_back("native_object_terrain_overlay");
    c.terrain=f.terrain;city.rebuiltMovementTerrain=surface.terrain;
    // Staffing from the native v6 plan.
    const auto labor=planNativeLabor(seed,state);if(!labor.error.empty())return refuse("native_labor:"+labor.error);
    // Buildings.
    c.buildingInputs.emplace();c.buildings.emplace();c.productionInputs.emplace();c.buildingLinks.emplace();c.houses.emplace();
    f.buildings.emplace();
    for(const auto& o:state.objects){if(o.kind==NativeObjectKind::Road)continue;
        const auto type=strictType(o);if(!type)return refuse("house_runtime_missing_"+std::to_string(o.id));
        const size_t slot=size_t(o.id);const uint32_t grid=uint32_t(cropBase+o.x+228*o.y);
        OriginalBuildingUpdateInputs i;
        const auto rec=persisted.records.find(o.id);
        if(rec!=persisted.records.end()){std::string e;const auto d=decodeNativeStrictRecord(rec->second,e);if(!d)return refuse(e+"_object_"+std::to_string(o.id));i=*d;}
        else{zeroInit=true;i.alive=1;i.identity=uint32_t(slot);i.modelDesirability=0;i.modelDesirabilityBonus=0;i.modelUpgrade=0;
            if(o.kind==NativeObjectKind::House){i.house.emplace();i.house->fill(0);i.model=int16_t(o.house->model);
                if(o.house->remaining!=std::array<uint8_t,nativeHouseTailBytes>{})i.house->operator=(o.house->remaining);
                i.houseWater=o.house->houseWater.value_or(0);i.houseInUse=1;
                const auto capacity=nativeHouseCapacity(seed,state,o);if(!capacity)return refuse("native_house_capacity_missing");i.free=int16_t(*capacity-o.residents);}
            if(o.kind==NativeObjectKind::Inspector){i.hookIndustry.emplace();i.hookIndustry->fill(0);}
            if(o.well){i.primaryFigure=o.well->primaryFigure;i.spawnCounter=o.well->spawnCounter;i.direction=o.well->roamDirection;}}
        if(nativeUsesInspection(state)){if(i.burning)return refuse("inspection_fire_target_unported");if(i.fireRisk<0||i.damageRisk<0)return refuse("inspection_negative_risk_unqualified");if(i.fireRisk>=1000||i.damageRisk>=1000)return refuse("inspection_disaster_trigger_unported");}
        if(i.identity!=slot||(i.alive!=1&&i.alive!=3))return refuse("strict_record_identity_"+std::to_string(o.id));
        if(o.kind==NativeObjectKind::Inspector&&(!i.modelUpgrade||i.model<0||i.model>1||*i.modelUpgrade>1))return refuse("inspection_building_grade_unsupported");
        // DTO projections must agree with the strict record.
        if(o.kind==NativeObjectKind::House){const auto& h=*o.house;
            if(!i.house||*i.house!=h.remaining||i.model!=int16_t(h.model))return refuse("house_projection_mismatch_"+std::to_string(o.id));
            if(h.houseWater&&*h.houseWater!=i.houseWater)return refuse("house_water_projection_mismatch_"+std::to_string(o.id));
            if(o.automation){if(o.automation->cachedWater&&*o.automation->cachedWater!=i.houseWater)return refuse("house_cached_water_mirror_mismatch_"+std::to_string(o.id));
                if(o.automation->delay!=i.house->at(56-4))return refuse("house_delay_mirror_mismatch_"+std::to_string(o.id));}}
        if(o.well&&(o.well->primaryFigure!=i.primaryFigure||o.well->spawnCounter!=i.spawnCounter||o.well->roamDirection!=i.direction))
            return refuse("well_projection_mismatch_"+std::to_string(o.id));
        // Native arrivals own residents and debit available places; strict
        // phase22 can later refresh the complete capacity/distance accounting.
        if(o.kind==NativeObjectKind::House&&o.residents>uint32_t(std::max<int16_t>(0,i.residents)))i.free=int16_t(std::max(0,int(i.free)-int(o.residents-uint32_t(std::max<int16_t>(0,i.residents)))));
        i.residents=int16_t(o.residents);   // native growth owns residents
        if(const auto* job=labor.find(o.id)){
            if(cityState&&rec!=persisted.records.end()&&i.workers!=int16_t(job->workers))return refuse("city_worker_record_mirror_mismatch_"+std::to_string(o.id));
            i.workers=int16_t(job->workers);
        }
        if(cityState&&rec==persisted.records.end()&&(o.kind==NativeObjectKind::Well||o.kind==NativeObjectKind::Hunter||o.kind==NativeObjectKind::Inspector))
            i.enabled=(o.kind!=NativeObjectKind::Hunter||state.huntingEnabled)?1:0;
        OriginalBuildingFootprint fp;fp.type=*type;fp.slot=slot;fp.x=uint16_t(o.x);fp.y=uint16_t(o.y);fp.size=2;
        fp.imageInputs.emplace();fp.imageInputs->gridIndex=grid;
        if(o.kind==NativeObjectKind::House){fp.imageInputs->randomByte=ec.terrainRandom->at(grid);fp.imageInputs->word20=i.residents;fp.imageInputs->houseByte3c=i.house->at(55);
            if(o.house->imageWord20!=fp.imageInputs->word20&&rec!=persisted.records.end())return refuse("house_image_projection_mismatch_"+std::to_string(o.id));}
        if(o.kind==NativeObjectKind::Well||o.kind==NativeObjectKind::Inspector)fp.wellImageInputs=OriginalWellImageInputs{i.model,i.modelUpgrade.value_or(0)};
        std::optional<OriginalBuildingProductionInputs> production;
        if(tpl.productionPrototypes.count(*type)){production=tpl.productionPrototypes.at(*type);production->workers=i.workers;production->operational=i.operational;}
        if(nativeUsesInspection(state)&&*type==33&&!production){
            // This mode explicitly retains the native Hunter economy. Supply
            // its native-constructed zero-stock view to the shared availability
            // consumer; do not pretend to restore original production/output.
            production.emplace();production->workers=i.workers;production->operational=i.operational;
            production->primaryStock=i.stock;production->enabled=i.enabled;production->laborDisabled=i.laborDisabled;
        }
        const auto proto=tpl.contextPrototypes.find(*type);if(proto==tpl.contextPrototypes.end())return refuse("template_context_prototype_type_"+std::to_string(*type));
        auto bctx=proto->second;bctx.phase=sc.phase;
        OriginalCombinedBuilding b{{fp,i,production},bctx};
        c.buildingInputs->emplace(slot,i);c.buildings->emplace(slot,fp);if(production)c.productionInputs->emplace(slot,*production);
        OriginalFigureBuildingLink link;link.type=*type;link.alive=i.alive;link.x=int16_t(o.x);link.y=int16_t(o.y);link.size=2;link.cell=grid;
        c.buildingLinks->emplace(slot,link);
        if(o.kind==NativeObjectKind::House){OriginalBuildingHouseCapacityInputs hc;hc.type=*type;hc.model=i.model;hc.residents=i.residents;hc.free=i.free;hc.distance=i.distance;
            hc.laborDisabled=i.house->at(55);const auto p=extra.housePeaks.find(slot);if(p!=extra.housePeaks.end())hc.peak=p->second;else peakInit=true;
            c.houses->emplace(slot,hc);}
        profile.slotTypes[slot]=*type;profile.footprints.push_back(fp);++profile.liveTypeCounts[*type];
        city.buildings.emplace(slot,std::move(b));
        c.highestBuildingSlot=std::max(c.highestBuildingSlot.value_or(0),uint32_t(slot));}
    if(zeroInit)out.policies.push_back("native_constructed_record_zero_init");
    if(peakInit)out.policies.push_back("native_house_capacity_peak_zero_init");
    out.policies.push_back(cityState?"native_city_scheduled_workers_v1":"native_labor_v6_workers");
    // Canonical links from the strict pool, then fields the timeline publishes.
    if(!originalBuildingSynchronizePool(c))return refuse("strict_link_pool_sync_failed");
    for(auto& [slot,link]:*c.buildingLinks){const auto& i=c.buildingInputs->at(slot);
        link.category=i.network;link.serviceCount=i.operational;link.destinationEnabled=i.enabled;link.accepting=i.houseInUse;link.industryInput=i.stock;
        if(link.type==124){if(!i.hookIndustry)return refuse("inspection_owner_record_missing");link.inspectorSecondary=i.model>0||i.modelUpgrade.value_or(0);for(unsigned n=0;n<2;++n)link.secondaryFigures[n]=detail::i16(i.hookIndustry->data()+0x6a-4+2*n);}
        if(!originalBuildingIsHouse(link.type))link.traderFaction=i.model;
        if(i.house){link.houseGoods=originalHouseGoodsFromPayload(*i.house);link.foodQuality=i.house->at(53-4);}}
    // The original allocator/removal addresses owner slot0 even when a
    // migrant's actual home is nonzero. This explicit native zero link is not
    // a constructed object and never acquires mutable resident/worker state.
    if(nativeUsesNonemptyMigrantRehousing(state))c.buildingLinks->emplace(0,OriginalFigureBuildingLink{});
    f.buildings=c.buildingLinks;
    // Road context recomputed from the overlaid rebuilt terrain (existing helpers).
    // Native objects keep their placement collision footprint when changing
    // grade. A 3->4 conversion changes the semantic/image record, not the
    // occupied 2x2 cells. Type4's original virtual pass-through predicate is
    // not proven: this local movement-only view retains the already admitted
    // native house collision contract and never relabels the actual record.
    auto movementProfile=profile;
    for(const auto& o:state.objects)if(o.house&&o.house->type==4){
        movementProfile.slotTypes[size_t(o.id)]=2;
        if(std::find(out.policies.begin(),out.policies.end(),"native_house_footprint_collision_retained")==out.policies.end())out.policies.push_back("native_house_footprint_collision_retained");
        for(const auto* policy:{"native_house_type4_road_footprint_retained","native_house_type4_initial_roam_footprint_retained"})
            if(std::find(out.policies.begin(),out.policies.end(),policy)==out.policies.end())out.policies.push_back(policy);
    }
    const auto passability=originalFigureMovementPassability(surface,movementProfile,f.buildingOwners,empty.figures.passability->spawn);
    if(!passability)return refuse("native_movement_passability_refused");
    f.passability=passability;f.movementMasks=passability->cells;c.movement=passability;
    const auto networks=originalFigureRoadNetworks(f);if(!networks)return refuse("native_road_networks_refused");
    const auto ranked=originalBuildingRankedRoadNetworks(*networks);if(!ranked)return refuse("native_road_rank_refused");
    f.roadNetworks=*networks;c.roadNetworks=*networks;c.rankedNetworks=*ranked;
    const auto& ep=*ec.roadEndpoints;const int64_t entry=int64_t(cropBase)+ep[0]+228LL*ep[1],exit=int64_t(cropBase)+ep[2]+228LL*ep[3];
    if(entry<0||entry>=51984||exit<0||exit>=51984)return refuse("native_road_endpoints_invalid");
    const auto distance=originalBuildingRoadDistances(passability->cells,uint32_t(entry));if(!distance)return refuse("native_road_distance_refused");
    OriginalBuildingRoadInputs roads;roads.cropBase=cropBase;roads.terrain=surface.terrain;roads.distance=*distance;roads.owners=f.buildingOwners;
    roads.networks=*networks;roads.rankedNetworks=*ranked;roads.ownerTypes=movementProfile.slotTypes;
    c.roadRefreshOrdinaryExit=distance->at(size_t(exit))!=0;
    c.poolRoadAccess.emplace();c.poolRoadReachable.emplace();c.poolRoadSelected.emplace();c.poolHookRoadAccess.emplace();f.workerRoadExits.clear();
    for(const auto& [slot,b]:city.buildings){const auto& fp=b.state.building;const bool house=originalBuildingIsHouse(fp.type);
        auto access=house?originalBuildingHouseRoadAccess(fp,roads):originalBuildingRoadAccess(fp,roads);
        const auto adjacent=originalBuildingHouseRoadAccess(fp,roads,false);
        // The strict distance-required query uses null for a known missing or
        // disconnected road. In the explicit native population mode, preserve
        // that ordinary zero-access state when the structural query succeeds;
        // malformed/unsupported geometry still refuses the whole candidate.
        if(!access&&house&&state.populationRuntime&&adjacent){access=*adjacent;access->distance=0;out.policies.push_back("native_house_disconnected_road_state_v1");}
        if(!access)return refuse("native_road_access_refused_slot_"+std::to_string(slot));
        c.poolRoadAccess->emplace(slot,*access);c.poolRoadReachable->emplace(slot,access->distance!=0);
        if(state.inspection&&fp.type==124){
            // Native-owned road projection, not a claim about the original
            // constructor's Common5 availability byte. Inspector expiry reads
            // this flag to enter its already-ported return action9. Zero-init
            // alone would retire every patrol on the street instead.
            const uint8_t selected=uint8_t(adjacent&&adjacent->selected);
            city.buildings.at(slot).state.inputs.available=selected;
            c.buildingInputs->at(slot).available=selected;
            c.buildingLinks->at(slot).roadAccess=selected;f.buildings->at(slot).roadAccess=selected;
            out.policies.push_back("native_inspector_return_road_from_overlay_v1");
        }
        if(house&&adjacent)f.workerRoadExits.emplace(slot,adjacent->selected&&(!state.populationRuntime||access->distance)?std::make_optional(std::make_pair(adjacent->x,adjacent->y)):std::nullopt);
        if(adjacent){c.poolRoadSelected->emplace(slot,adjacent->selected);c.poolHookRoadAccess->emplace(slot,*adjacent);}}
    // Figures, shared services and speech state.
    std::set<size_t> walkerSlots;
    for(const auto& payload:fr.wellWalkerPayloads){std::string e;const auto w=decodeNativeWellWalker(payload,e);if(!w)return refuse("walker_payload:"+e);
        if(w->slot==0||w->slot>=2000||!walkerSlots.insert(w->slot).second)return refuse("walker_slot_invalid_"+std::to_string(w->slot));
        f.figures.at(w->slot)=*w;}
    for(const auto& payload:fr.migrantPayloads){std::string e;const auto w=decodeNativeMigrant(payload,e);if(!w)return refuse("migrant_payload:"+e);
        if(!state.observedDry||w->slot==0||w->slot>=2000||!walkerSlots.insert(w->slot).second)return refuse("migrant_slot_invalid");
        f.figures.at(w->slot)=*w;}
    if(state.inspection)for(const auto& payload:state.inspection->payloads){std::string e;const auto w=decodeNativeInspector(payload,e);if(!w)return refuse(e);
        if(!walkerSlots.insert(w->slot).second)return refuse("inspection_duplicate_figure_slot");f.figures.at(w->slot)=*w;}
    if(state.inspection)for(const auto& [id,b]:*f.buildings)if(b.type==124){
        if(b.secondaryFigures[0]||(b.primaryFigure&&b.primaryFigure==b.secondaryFigures[1]))return refuse("inspection_auxiliary_reference_invalid");
        for(int slot:{int(b.primaryFigure),int(b.secondaryFigures[1])})if(slot){const auto w=f.figures.find(size_t(slot));
            if(slot<1||w==f.figures.end()||w->second.alive!=1||w->second.type!=39||w->second.owner!=int(id))return refuse("inspection_owner_reference_invalid");}
    }
    uint64_t incomingLive=0;
    for(const auto& payload:fr.immigrantPayloads){std::string e;const auto w=decodeNativeImmigrant(payload,e);if(!w)return refuse("immigrant_payload:"+e);
        if(!state.populationRuntime||!walkerSlots.insert(w->slot).second)return refuse("immigrant_slot_invalid");
        incomingLive+=w->employmentAmount;f.figures.at(w->slot)=*w;}
    if(state.populationRuntime&&state.populationRuntime->admitted!=incomingLive+state.populationRuntime->arrived+state.populationRuntime->unplaced)return refuse("immigration_admission_ledger_mismatch");
    if(state.observedDry){f.migrationRoadEndpoints=ec.roadEndpoints;f.houseCapacities.clear();for(const auto& [model,capacity]:seed.houseCapacities){
        if(capacity>INT_MAX)return refuse("migrant_capacity_overflow");f.houseCapacities.emplace(model,int(capacity));}}
    f.cellHeads=fr.cellHeads;f.freeSlots.assign(fr.freeFigureSlots.begin(),fr.freeFigureSlots.end());f.highestLiveSlot=*fr.highestFigureSlot;
    for(size_t s:f.freeSlots)if(f.figures.at(s).alive)return refuse("free_slot_is_live_"+std::to_string(s));
    auto& sv=*f.services;const auto& dv=fr.services;
    for(size_t n=0;n<100;++n){sv.formationImpacts[n].alert=dv.formationImpacts[n].first;sv.formationImpacts[n].source=dv.formationImpacts[n].second;}
    sv.routeOwners=dv.routeOwners;sv.directions=dv.directions;sv.randomFirst=dv.randomFirst;sv.randomSecond=dv.randomSecond;
    if(dv.randomRing)sv.randomHistory=OriginalFigureRandomHistory{*dv.randomRing,*dv.randomCursor,*dv.randomPrevious};else sv.randomHistory.reset();
    sv.fireCount=dv.fireCount;sv.criminalCount=dv.criminalCount;sv.allocationSerial=dv.allocationSerial;sv.serviceGrid=dv.serviceGrid;
    sv.population=extra.population;
    if(sv.population){const auto population=nativePopulation(state);if(population>INT32_MAX)return refuse("native_population_overflow");
        const int64_t adjusted=state.observedDry?int64_t(sv.population->total)+int64_t(population)-int64_t(*extra.nativePopulation):int64_t(population);
        if(adjusted<0||adjusted>INT32_MAX)return refuse("strict_population_overflow");
        sv.population->total=int32_t(adjusted);sv.population->peak=std::max(sv.population->peak,sv.population->total);}
    c.populationTotal=sv.population?std::make_optional(sv.population->total):std::nullopt;
    c.populationPeak=sv.population?std::make_optional(sv.population->peak):std::nullopt;
    c.figureServices=f.services;city.process.randomHistory=sv.randomHistory;
    if(state.inspection){
        if(!f.inspectorReduction||*f.inspectorReduction<=0||*f.inspectorReduction>32767||!f.models.count(39)||!f.modelSpeeds.count(39))return refuse("inspection_model_inputs_missing");
        f.inspectorMaximum=state.inspection->maximum;c.effectDecayCounter=state.inspection->decayCounter;city.process.effectDecayCounter=c.effectDecayCounter;
        f.captureInspectionVisits=true;f.inspectionVisits.clear();
        if(!f.services->fireCount||*f.services->fireCount!=0)return refuse("inspection_fire_target_unported");
    }
    f.speechCounters=fr.speechCounters;c.figureSpeechCounters=fr.speechCounters;f.speechCounts=fr.speechCounts;c.figureSpeechCounts=fr.speechCounts;
    c.figureSpeechChoices.clear();for(const auto& [k,v]:fr.speechChoices)c.figureSpeechChoices[k]=v;
    c.desirability=extra.desirability;
    if(f.goods)f.goods->savedPhase=sc.phase;
    if(cityState)c.goods=f.goods;
    // Isolation.
    for(const auto& [id,reason]:state.heldHouseReasons){if(!city.buildings.count(size_t(id))||!originalBuildingIsHouse(city.buildings.at(size_t(id)).state.building.type))
            return refuse("held_house_not_house_"+std::to_string(id));out.isolation.blockedHouses[size_t(id)]=reason;}
    for(const auto& entry:state.heldWalkerReasons){
        if(!walkerSlots.count(entry.first))return refuse("held_walker_not_live_"+std::to_string(entry.first));
        out.isolation.heldWalkers[entry.first]=entry.second;
    }
    if(auto w=native_phase_detail::checkWalkers(city,state.observedDry.has_value(),nativeUsesNonemptyMigrantRehousing(state),nativeUsesImmigration(state),nativeUsesInspection(state));!w.empty())return refuse(w);
    if(auto s=originalCombinedSyncBuildings(city);!s.empty())return refuse(s);
    out.city=std::move(city);return out;
}

inline NativeStrictProjection strictToNative(const NativeSessionSeed& seed,const NativeSessionState& base,
    const OriginalCombinedState& inputCandidate,const NativeHouseIsolation& isolation,const NativeStrictTemplate& tpl,
    const std::map<size_t,uint32_t>* departures=nullptr,const std::vector<NativeMigrantArrival>* arrivals=nullptr,const std::vector<NativeImmigrantOffer>* offers=nullptr){
    // New arrivals happen after the building phase. Native v6 staffing remains
    // a derived projection, so rebase it on the committed population before
    // lossless publication; never reinterpret it as strict phase25 execution.
    std::optional<OriginalCombinedState> owned;if(base.populationRuntime)owned=inputCandidate;
    if(owned&&owned->buildingContext&&nativeUsesCityContext(base))owned->buildingContext->goods=owned->figures.goods;
    const auto& candidate=owned?*owned:inputCandidate;
    using namespace native_bridge_detail;NativeStrictProjection out;
    const auto refuse=[&](std::string why){out.refusal=std::move(why);out.state.reset();out.persisted={};return out;};
    if(base.rulesVersion!=8||!base.houseSchedule||!base.figureRuntime)return refuse("bridge_requires_native_v8");
    if(!candidate.buildingContext||!candidate.figures.services||!candidate.figures.buildings||!candidate.buildingContext->buildingInputs||
       !candidate.buildingContext->cycle||!candidate.buildingContext->monthPart||!candidate.buildingContext->month)return refuse("candidate_context_incomplete");
    const auto& c=*candidate.buildingContext;const auto& f=candidate.figures;
    std::set<size_t> objectSlots;for(const auto& o:base.objects)if(o.kind!=NativeObjectKind::Road)objectSlots.insert(size_t(o.id));
    std::set<size_t> buildingSlots;for(const auto& e:candidate.buildings)buildingSlots.insert(e.first);
    if(objectSlots!=buildingSlots)return refuse("candidate_building_identity_mismatch");
    auto state=base;NativeStrictPersisted persisted;NativeStrictContextState extra;
    std::map<size_t,NativeMigrantArrival> arrivalByHouse;std::set<size_t> arrivalFigures;
    if(arrivals)for(const auto& a:*arrivals){
        if((!nativeUsesNonemptyMigrantRehousing(base)&&!nativeUsesImmigration(base))||!base.observedDry||!a.residents||!a.figureSlot||a.figureSlot>=2000||!objectSlots.count(a.houseSlot)||!arrivalByHouse.emplace(a.houseSlot,a).second||!arrivalFigures.insert(a.figureSlot).second)return refuse("candidate_arrival_proof_invalid");
        std::optional<OriginalFigure> prior;
        for(const auto& payload:base.figureRuntime->migrantPayloads){std::string e;const auto w=decodeNativeMigrant(payload,e);if(!w)return refuse("candidate_arrival_prior_payload_invalid");if(w->slot==a.figureSlot)prior=*w;}
        if(a.figureType==11){prior.reset();for(const auto& payload:base.figureRuntime->immigrantPayloads){std::string e;const auto w=decodeNativeImmigrant(payload,e);if(!w)return refuse("candidate_immigrant_prior_payload_invalid");if(w->slot==a.figureSlot)prior=*w;}}
        if((a.figureType!=13&&a.figureType!=11)||(a.figureType==11&&!nativeUsesImmigration(base)))return refuse("candidate_arrival_family_invalid");
        const auto record=base.strictRecords.find(a.houseSlot);std::string e;
        const auto beforeRecord=record==base.strictRecords.end()?std::optional<OriginalBuildingUpdateInputs>{}:decodeNativeStrictRecord(record->second,e);
        if(!prior||prior->action!=8||prior->home!=int(a.houseSlot)||prior->owner||int(prior->freeRemainingX)+prior->freeRemainingY>=1||a.residents>prior->employmentAmount||!beforeRecord||beforeRecord->migrant!=int(a.figureSlot))return refuse("candidate_arrival_not_previously_reserved");
        const auto live=f.figures.find(a.figureSlot);
        if(live==f.figures.end()||live->second.alive||std::count(f.freeSlots.begin(),f.freeSlots.end(),a.figureSlot)!=1)return refuse("candidate_arrival_not_removed_once");
    }
    for(auto& o:state.objects){if(o.kind==NativeObjectKind::Road)continue;const auto& b=candidate.buildings.at(size_t(o.id)).state;const auto& i=b.inputs;
        if(!kindMatches(o,b.building.type))return refuse("candidate_kind_mismatch_"+std::to_string(o.id));
        if(b.building.x!=o.x||b.building.y!=o.y||b.building.size!=2||i.identity!=o.id)return refuse("candidate_geometry_mismatch_"+std::to_string(o.id));
        if(i.residents<0)return refuse("candidate_negative_residents");
        const auto oldResidents=o.residents;
        if(uint32_t(i.residents)<oldResidents){
            if(!state.observedDry||!departures||!departures->count(size_t(o.id))||departures->at(size_t(o.id))!=oldResidents-uint32_t(i.residents)||
               base.houseSchedule->phase!=24||!state.observedDry->pendingExcess.count(o.id)||state.observedDry->pendingExcess.at(o.id)!=oldResidents-uint32_t(i.residents))return refuse("candidate_residents_changed_"+std::to_string(o.id));
            if(state.populationRuntime)state.populationRuntime->populationDeparted+=oldResidents-uint32_t(i.residents);
            state.observedDry->departedSinceMeal+=oldResidents-uint32_t(i.residents);o.residents=uint32_t(i.residents);
        }else if(uint32_t(i.residents)>oldResidents){
            const uint32_t delta=uint32_t(i.residents)-oldResidents;const auto proof=arrivalByHouse.find(size_t(o.id));
            if(!nativeUsesNonemptyMigrantRehousing(state)||!state.observedDry||o.kind!=NativeObjectKind::House||(!oldResidents&&(!nativeUsesImmigration(state)||proof==arrivalByHouse.end()||proof->second.figureType!=11||b.building.type!=3||i.model!=0))||proof==arrivalByHouse.end()||proof->second.residents!=delta)return refuse("candidate_residents_increase_unproved_"+std::to_string(o.id));
            const auto capacity=seed.houseCapacities.find(uint16_t(i.model));if(capacity==seed.houseCapacities.end()||uint32_t(i.residents)>capacity->second)return refuse("candidate_arrival_over_capacity");
            if(state.populationRuntime)state.populationRuntime->populationArrived+=delta;
            state.observedDry->arrivedSinceMeal+=delta;o.residents=uint32_t(i.residents);arrivalByHouse.erase(proof);
        }
        if(state.observedDry&&o.kind==NativeObjectKind::House){
            const auto cap=seed.houseCapacities.find(uint16_t(i.model));if(cap==seed.houseCapacities.end())return refuse("candidate_house_capacity_missing");
            if(o.residents>cap->second){
                if(b.building.type!=3||i.model!=0||(!state.observedDry->pendingExcess.count(o.id)&&
                    !(o.house&&o.house->type==4&&o.house->model==1&&i.houseWater==0&&base.houseSchedule->phase==39)))return refuse("candidate_unobserved_overcapacity");
                state.observedDry->pendingExcess[o.id]=o.residents-cap->second;
            }else state.observedDry->pendingExcess.erase(o.id);
        }
        persisted.records[o.id]=encodeNativeStrictRecord(i);
        if(o.kind==NativeObjectKind::House){if(!i.house)return refuse("candidate_house_tail_missing_"+std::to_string(o.id));
            auto& h=o.house.emplace(o.house.value_or(NativeHouseRuntime{}));h.type=b.building.type;h.model=uint16_t(i.model);h.remaining=*i.house;
            h.houseWater=i.houseWater;h.servicesKnown=true;
            if(b.building.imageInputs){h.imageWord20=b.building.imageInputs->word20;h.imageHouseByte3c=b.building.imageInputs->houseByte3c;}
            if(o.automation){o.automation->cachedWater=i.houseWater;o.automation->delay=i.house->at(56-4);}
            if(c.houses&&c.houses->count(size_t(o.id)))extra.housePeaks[size_t(o.id)]=c.houses->at(size_t(o.id)).peak;}
        if(o.kind==NativeObjectKind::Well)o.well=NativeWellRuntime{i.primaryFigure,i.spawnCounter,i.direction};
    }
    if(!arrivalByHouse.empty())return refuse("candidate_arrival_proof_unused");
    if(owned&&!nativeUsesCityContext(state)){
        const auto plan=planNativeLabor(seed,state);if(!plan.error.empty())return refuse("candidate_native_staffing:"+plan.error);
        for(const auto& o:state.objects)if(const auto* job=plan.find(o.id)){
            auto& b=owned->buildings.at(size_t(o.id)).state;b.inputs.workers=int16_t(job->workers);
            owned->figures.buildings->at(size_t(o.id)).workers=int16_t(job->workers);
            if(b.production)b.production->workers=int16_t(job->workers);
        }
        if(auto error=originalCombinedSyncBuildings(*owned);!error.empty())return refuse("candidate_staffing_sync:"+error);
        for(const auto& [slot,b]:owned->buildings)persisted.records[slot]=encodeNativeStrictRecord(b.state.inputs);
    }

    state.heldWalkerReasons=isolation.heldWalkers;
    for(const auto& [slot,reason]:isolation.heldWalkers){(void)reason;const auto& g=f.figures.at(slot);
        if(!g.alive||(g.type!=28&&!(state.observedDry&&g.type==13&&g.owner==0&&((g.home==0&&reason=="native_migrant_rehousing_unverified")||(nativeUsesNonemptyMigrantRehousing(state)&&reason=="native_migrant_empty_house_conversion_unported"))))||(g.type==28&&!objectSlots.count(size_t(g.owner))))return refuse("held_walker_owner_unmapped_"+std::to_string(slot));}
    state.heldHouseReasons.clear();for(const auto& [slot,reason]:isolation.blockedHouses){if(!objectSlots.count(slot))return refuse("blocked_house_unmapped_"+std::to_string(slot));state.heldHouseReasons[slot]=reason;}
    auto& sc=*state.houseSchedule;if(candidate.phase>50)return refuse("candidate_phase_invalid");
    sc.phase=uint8_t(candidate.phase);sc.cycle=*c.cycle;sc.monthPart=*c.monthPart;sc.month=*c.month;
    auto& fr=*state.figureRuntime;fr.wellWalkerPayloads.clear();fr.migrantPayloads.clear();fr.immigrantPayloads.clear();if(state.inspection)state.inspection->payloads.clear();
    for(const auto& [slot,g]:f.figures){if(!g.alive)continue;std::string e;const auto p=g.type==11?encodeNativeImmigrant(g,e):g.type==13?encodeNativeMigrant(g,e):g.type==39?encodeNativeInspector(g,e):encodeNativeWellWalker(g,e);if(!p)return refuse("walker_payload:"+e);if(g.type==11)fr.immigrantPayloads.push_back(*p);else if(g.type==13)fr.migrantPayloads.push_back(*p);else if(g.type==39&&state.inspection)state.inspection->payloads.push_back(*p);else fr.wellWalkerPayloads.push_back(*p);}
    if(f.cellHeads.size()!=51984)return refuse("candidate_cell_heads_invalid");fr.cellHeads=f.cellHeads;
    fr.freeFigureSlots.clear();for(size_t s:f.freeSlots){if(!s||s>=2000)return refuse("candidate_free_slot_invalid");fr.freeFigureSlots.push_back(uint16_t(s));}
    fr.highestFigureSlot=f.highestLiveSlot;
    const auto& sv=*f.services;auto& dv=fr.services;
    for(size_t n=0;n<100;++n)dv.formationImpacts[n]={sv.formationImpacts[n].alert,sv.formationImpacts[n].source};
    dv.routeOwners=sv.routeOwners;dv.directions=sv.directions;dv.randomFirst=sv.randomFirst;dv.randomSecond=sv.randomSecond;
    if(sv.randomHistory){dv.randomRing=sv.randomHistory->ring;dv.randomCursor=sv.randomHistory->cursor;dv.randomPrevious=sv.randomHistory->previous7;}
    else{dv.randomRing.reset();dv.randomCursor.reset();dv.randomPrevious.reset();}
    dv.fireCount=sv.fireCount;dv.criminalCount=sv.criminalCount;dv.allocationSerial=sv.allocationSerial;dv.serviceGrid=sv.serviceGrid;
    if(state.populationRuntime){
        if(!c.labor||!c.cityEmployment)return refuse("candidate_immigration_context_missing");
        auto& pop=*state.populationRuntime;pop.labor=*c.labor;pop.employment=*c.cityEmployment;
        std::map<size_t,OriginalFigure> prior;
        for(const auto& bytes:base.figureRuntime->immigrantPayloads){std::string e;const auto w=decodeNativeImmigrant(bytes,e);if(!w)return refuse(e);prior.emplace(w->slot,*w);}
        std::map<size_t,NativeImmigrantOffer> allocated;
        if(offers)for(const auto& offer:*offers){
            if(base.houseSchedule->phase!=23||!offer.figureSlot||offer.figureSlot>=2000||!offer.residents||offer.residents>255||prior.count(offer.figureSlot)||!allocated.emplace(offer.figureSlot,offer).second||std::count(base.figureRuntime->freeFigureSlots.begin(),base.figureRuntime->freeFigureSlots.end(),uint16_t(offer.figureSlot))!=1)return refuse("immigrant_offer_proof_invalid");
            const auto record=std::find_if(c.createdFigureRecords.begin(),c.createdFigureRecords.end(),[&](const auto& w){return w.slot==offer.figureSlot&&w.type==11&&w.allocationSerial==offer.allocationSerial&&w.home==int(offer.houseSlot)&&w.employmentAmount==offer.residents;});
            if(record==c.createdFigureRecords.end())return refuse("immigrant_offer_not_allocated");
            pop.admitted+=offer.residents;
        }
        uint64_t live=0;
        for(const auto& [slot,w]:f.figures)if(w.alive&&w.type==11){live+=w.employmentAmount;const auto old=prior.find(slot);
            if(old==prior.end()){
                const auto offer=allocated.find(slot);if(offer==allocated.end()||w.employmentAmount>offer->second.residents||w.allocationSerial!=offer->second.allocationSerial)return refuse("new_immigrant_without_offer_proof");
                pop.unplaced+=offer->second.residents-w.employmentAmount;allocated.erase(offer);
            }
            else{if(w.employmentAmount>old->second.employmentAmount)return refuse("immigrant_reserved_amount_increased");pop.unplaced+=old->second.employmentAmount-w.employmentAmount;prior.erase(old);}}
        for(const auto& [slot,w]:prior){uint64_t arrived=0;if(arrivals)for(const auto& a:*arrivals)if(a.figureSlot==slot&&a.figureType==11)arrived+=a.residents;
            if(arrived>w.employmentAmount)return refuse("immigrant_arrival_exceeds_offer");pop.arrived+=arrived;pop.unplaced+=w.employmentAmount-arrived;}
        for(const auto& [slot,offer]:allocated){const auto actor=f.figures.find(slot);
            if(actor==f.figures.end()||actor->second.alive||std::count(f.freeSlots.begin(),f.freeSlots.end(),slot)!=1)return refuse("immigrant_cancelled_offer_not_released");
            pop.unplaced+=offer.residents;
        }
        if(pop.admitted!=live+pop.arrived+pop.unplaced)return refuse("candidate_immigration_ledger_mismatch");
    }
    if(state.inspection){
        if(!f.inspectorMaximum||!c.effectDecayCounter)return refuse("inspection_scratch_missing");
        auto& inspection=*state.inspection;inspection.maximum=*f.inspectorMaximum;inspection.decayCounter=*c.effectDecayCounter;
        std::set<size_t> oldSlots,newSlots;
        for(const auto& bytes:base.inspection->payloads){std::string e;const auto w=decodeNativeInspector(bytes,e);if(!w)return refuse(e);oldSlots.insert(w->slot);}
        for(const auto& [slot,g]:f.figures)if(g.alive&&g.type==39)newSlots.insert(slot);
        for(size_t slot:newSlots)if(!oldSlots.count(slot))++inspection.dispatches;
        for(size_t slot:oldSlots)if(!newSlots.count(slot))++inspection.returns;
        for(const auto& visit:f.inspectionVisits){if(!objectSlots.count(visit.building))return refuse("inspection_visit_owner_unmapped");
            inspection.visits[visit.building]={state.ticks,visit.figure,visit.collapseBefore,visit.ignitionBefore,visit.collapseAfter,visit.ignitionAfter};++inspection.services;}
    }
    extra.population=sv.population;
    if(state.observedDry){if(!c.labor)return refuse("candidate_labor_context_missing");extra.nativePopulation=nativePopulation(state);extra.laborCounters=std::make_pair(c.labor->lastPopulationDelta,c.labor->excessDepartureTotal);}
    if(!f.speechCounters)return refuse("candidate_speech_counters_missing");fr.speechCounters=*f.speechCounters;
    if(c.figureSpeechCounts)fr.speechCounts=*c.figureSpeechCounts;
    fr.speechChoices.clear();for(const auto& [k,v]:c.figureSpeechChoices)fr.speechChoices[k]=v;
    if(!c.buildingHookCountdown)return refuse("candidate_hook_countdown_missing");state.buildingHookCountdown=*c.buildingHookCountdown;
    extra.desirability=c.desirability;persisted.context=encodeNativeStrictContext(extra);
    if(nativeUsesCityContext(state)){
        std::string error;const auto payload=encodeNativeCityContext(captureNativeCityContext(c),error);
        if(!payload)return refuse("candidate_city_context:"+error);state.cityContext=*payload;
    }
    if(auto e=validateNativeSession(seed,state);!e.empty())return refuse("projected_state_invalid:"+e);
    // Lossless proof: rebuild and compare every persisted/derived component.
    const auto rebuilt=nativeToStrict(seed,state,tpl,persisted);
    if(!rebuilt.city)return refuse("bridge_rebuild_refused:"+rebuilt.refusal);
    const auto a=fingerprint(candidate,isolation,nativeUsesCityContext(state),nativeUsesInspection(state)),b=fingerprint(*rebuilt.city,rebuilt.isolation,nativeUsesCityContext(state),nativeUsesInspection(state));
    if(a.size()!=b.size())return refuse("bridge_not_lossless:component_count");
    for(size_t n=0;n<a.size();++n)if(a[n]!=b[n])return refuse("bridge_not_lossless:"+a[n].first);
    for(const auto& policy:rebuilt.policies)if(std::find(state.runtimePolicies.begin(),state.runtimePolicies.end(),policy)==state.runtimePolicies.end())state.runtimePolicies.push_back(policy);
    state.strictRecords=persisted.records;state.strictContext=persisted.context;
    out.state=std::move(state);out.persisted=std::move(persisted);return out;
}
} // namespace game_lab
