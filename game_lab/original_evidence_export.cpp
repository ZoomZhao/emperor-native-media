// SPDX-License-Identifier: MIT
#include "original_evidence_figures.hpp"
#include "original_evidence_route.hpp"
#include "original_save.hpp"
#include "original_building_update.hpp"
#include "../file_io/include/city19_goal_archive.hpp"
#include <iostream>
#include <variant>
#include <type_traits>

namespace {
struct J {
    using A=std::vector<J>;using O=std::map<std::string,J>;
    std::variant<std::nullptr_t,bool,int64_t,std::string,A,O> v=nullptr;
    J()=default;J(bool b):v(b){};J(const char* s):v(std::string(s)){};J(std::string s):v(std::move(s)){};
    template<class T,std::enable_if_t<std::is_integral_v<T>&&!std::is_same_v<T,bool>,int> =0>J(T n):v(int64_t(n)){}
    static J array(){J j;j.v=A{};return j;}
    static J object(){J j;j.v=O{};return j;}
    J& operator[](const std::string& key){return std::get<O>(v)[key];}
    void add(J j){std::get<A>(v).push_back(std::move(j));}
};
void quoted(std::ostream& o,const std::string& s){
    constexpr char hex[]="0123456789abcdef";o<<'"';
    for(unsigned char c:s){if(c=='"'||c=='\\')o<<'\\'<<c;
        else if(c<32||c>=127)o<<"\\u00"<<hex[c>>4]<<hex[c&15];else o<<c;}
    o<<'"';
}
void dump(std::ostream& o,const J& j){
    if(std::holds_alternative<std::nullptr_t>(j.v))o<<"null";
    else if(const auto p=std::get_if<bool>(&j.v))o<<(*p?"true":"false");
    else if(const auto p=std::get_if<int64_t>(&j.v))o<<*p;
    else if(const auto p=std::get_if<std::string>(&j.v))quoted(o,*p);
    else if(const auto p=std::get_if<J::A>(&j.v)){o<<'[';bool comma=false;for(const auto& a:*p){if(comma)o<<',';comma=true;dump(o,a);}o<<']';}
    else {o<<'{';bool comma=false;for(const auto& e:std::get<J::O>(j.v)){if(comma)o<<',';comma=true;quoted(o,e.first);o<<':';dump(o,e.second);}o<<'}';}
}
template<class T>J number(const std::optional<T>& n){return n?J(*n):J();}
template<class T>J array(const T& values){J a=J::array();for(const auto& value:values)a.add(J(value));return a;}
J storage(const std::optional<std::vector<game_lab::OriginalStorageCompartment>>& s){
    if(!s)return J();J a=J::array();for(const auto& c:*s){J j=J::object();j["resource"]=c.resource;j["amount"]=c.amount;j["capacity_wire"]=c.capacity;a.add(j);}return a;
}
int32_t signedWord(uint32_t v){return v<=32767?int32_t(v):int32_t(v)-65536;}
J exportSnapshot(const std::vector<uint8_t>& input){
    using namespace game_lab;J out=J::object();out["schema"]="original-evidence-v1";
    out["risk_contract"]="common5-risk-v1";
    out["risk_units"]="signed16_accumulators_not_percentages";
    out["risk_semantics"]="existing_project_disaster_branches_not_original_probability";
    out["legacy_risk_aliases"]="fire_risk=common5_68_collapse;damage_risk=common5_70_ignition;deprecated";
    out["whole_sav_accepted"]=false;out["gameplay_executed"]=false;
    const auto saved=decodeOriginalSave(input);out["save_status"]=int(saved.status);
    if(saved.status!=OriginalSaveStatus::LoadedSnapshot){out["error"]="supported_snapshot_required";return out;}
    const auto decoded=decodeOriginalMapContainer(input);if(!decoded.complete){out["error"]="container_required";return out;}
    const auto& d=decoded.payload;out["input_bytes"]=input.size();out["decoded_bytes"]=d.size();
    out["active_side"]=saved.terrain.activeSide;out["terrain_anchor"]=saved.terrainOffset;out["size_anchor"]=saved.sizeOffset;
    out["building_table_complete"]=saved.buildings.status==OriginalBuildingStatus::Complete&&saved.buildings.parsedRecords==4000;
    out["building_records"]=saved.buildings.parsedRecords;
    OriginalBuildingUpdateContext city(OriginalBuildingUpdateScope::AdmittedTicks,true);
    const bool cityValid=originalBuildingSavedCityContext(d,saved.sizeOffset,city);
    out["city_contract_complete"]=cityValid;
    J cj=J::object();
    if(cityValid){
        cj["population"]=number(city.populationTotal);cj["treasury"]=number(city.treasury);
        cj["year_word_wire"]=number(city.elapsedYears);cj["year_reader_member"]="elapsedYears (name is not an elapsed-duration proof)";
        cj["scenario_start_year_wire"]=number(city.scenarioStartYears);
        cj["scenario_months_wire"]=number(city.scenarioMonths);cj["region_wire"]=number(city.engineRegion);
        if(city.accounting){cj["wages"]=city.accounting->wages;cj["tax_rate"]=city.accounting->taxRate;}
        if(city.labor){const auto& l=*city.labor;cj["health_smoothed"]=l.health;cj["sentiment"]=l.sentiment;
            cj["statistics_pending"]=l.statisticsPending;cj["growth_cursor"]=l.growthCursor;cj["death_cursor"]=l.deathCursor;
            cj["arrivals"]=l.arrivals;cj["departures"]=l.departures;cj["migration"]=l.migration;}
        if(city.cycleTail){cj["health_target"]=city.cycleTail->healthTarget;cj["health_rating"]=city.cycleTail->currentHealthRating;}
        if(city.cityEmployment){const auto& e=*city.cityEmployment;J j=J::object();j["available"]=e.available;j["employed"]=e.employed;j["unemployed"]=e.unemployed;J rows=J::array();
            for(const auto& r:e.rows){J row=J::object();row["demand"]=r.demand;row["allocated"]=r.allocated;row["priority"]=r.priority;rows.add(row);}j["rows"]=rows;cj["employment"]=j;}
    }out["city"]=cityValid?cj:J();
    J clock;for(const auto& b:saved.buildings.footprints){const auto c=originalBuildingSavedUpdateContext(d,saved.terrainOffset,saved.sizeOffset,b,OriginalBuildingUpdateScope::AdmittedTicks,true);
        if(!c)continue;clock=J::object();clock["phase"]=number(c->phase);clock["cycle"]=number(c->cycle);clock["month_part"]=number(c->monthPart);clock["month"]=number(c->month);
        clock["ranges_valid"]=c->phase&&*c->phase<=50&&c->monthPart&&*c->monthPart<=15&&c->month&&*c->month<=11;break;}
    out["clock"]=clock;
    const auto links=originalFigureBuildingLinks(d,saved.buildings);const auto inputs=originalBuildingUpdateInputs(d,saved.buildings);
    bool linksComplete=bool(links);if(links){for(const auto& b:saved.buildings.footprints)linksComplete&=links->count(b.slot)>0;
        for(const auto& b:saved.buildings.objectBuildings)linksComplete&=links->count(b.slot)>0;}
    out["building_links_complete"]=linksComplete;out["building_inputs_complete"]=bool(inputs);J buildings=J::array();
    if(links)for(const auto& entry:*links){const auto& b=entry.second;if(!entry.first)continue;J j=J::object();
        j["slot"]=entry.first;j["type"]=b.type;j["alive"]=b.alive;j["x"]=b.x;j["y"]=b.y;j["cell"]=b.cell;
        j["workers"]=b.workers;j["road_access"]=b.roadAccess;j["road_x"]=b.roadX;j["road_y"]=b.roadY;
        j["parent_common_wire"]=b.parent;j["primary_figure"]=b.primaryFigure;j["secondary_figures"]=array(b.secondaryFigures);j["cargo_figures"]=array(b.cargoFigures);
        j["storage_policy_index"]=number(b.storagePolicy);
        j["storage"]=storage(b.storage);j["market_storage"]=storage(b.marketStorage);j["market_goods"]=number(b.marketGoods);
        j["market_quality_raw"]=number(b.marketQuality);j["market_demand"]=b.marketDemand?array(*b.marketDemand):J();
        j["goods12"]=b.houseGoods?array(*b.houseGoods):J();j["food_quality_raw"]=number(b.foodQuality);
        j["water_service"]=b.waterService?array(*b.waterService):J();j["herbalist"]=number(b.herbalist);
        j["residents"]=b.residents;j["model"]=b.houseModel;j["free"]=b.freeSlots;
        // Deprecated numeric aliases remain unchanged for old report readers.
        j["fire_risk"]=b.fireRisk;j["damage_risk"]=b.damageRisk;
        if(inputs){const auto it=inputs->find(entry.first);if(it!=inputs->end()){const auto& i=it->second;
            j["enabled"]=i.enabled;j["operational"]=i.operational;j["labor_disabled"]=i.laborDisabled;j["burning"]=i.burning;
            J risk=J::object();risk["common5_68_s16"]=i.fireRisk;risk["common5_70_s16"]=i.damageRisk;risk["common5_72_s16"]=i.burning;
            risk["collapse_counter"]=i.fireRisk;risk["ignition_counter"]=i.damageRisk;risk["burning_counter"]=i.burning;
            risk["reader_crosscheck"]=i.fireRisk==b.fireRisk&&i.damageRisk==b.damageRisk;j["risk"]=risk;
            j["spawn_counter"]=i.spawnCounter;j["spawn_aux"]=i.spawnAux;
            j["supplier_status"]=number(i.supplierStatus);j["stall_resource_index"]=number(i.stallResourceIndex);
            j["parent_slot"]=number(i.parentSlot);
            j["cached_water"]=i.houseWater;j["desirability"]=number(i.modelDesirability);
            const auto goods=i.house?originalHouseGoodsFromPayload(i.house->data(),i.house->size()):std::nullopt;
            j["goods12_crosscheck"]=goods&&b.houseGoods?J(*goods==*b.houseGoods):J();}}
        buildings.add(j);
    }out["buildings"]=buildings;
    J actors=J::object();EvidenceFigurePrefix prefix;prefix.reason="goal_or_building_prefix_unqualified";
    try{
        emperor::file_io::research::GoalLimits limits;limits.enable_homage=limits.enable_trading=limits.enable_yearly=limits.enable_population=true;
        const auto goals=emperor::file_io::research::walk_city19_housing_goals({d.data(),std::min(d.size(),size_t(16384))},limits);
        if(saved.buildings.status==OriginalBuildingStatus::Complete&&goals.completed_members==10&&!goals.pending.present)
            prefix=evidenceFigurePrefix(d,saved.buildings.tableEnd,saved.terrainOffset,goals.cache.size()+4000+saved.buildings.declaredClasses);
    }catch(const std::exception&){prefix.reason="goal_prefix_refused";}
    actors["base_pool_complete"]=prefix.complete&&prefix.declared==2000&&prefix.completed==2000;
    actors["declared"]=prefix.declared;actors["completed"]=prefix.completed;actors["stop_offset"]=prefix.stopOffset;
    actors["stop_slot"]=prefix.stopSlot;actors["reason"]=prefix.reason;actors["unqualified_class"]=prefix.unqualifiedClass;
    actors["fsa_semantics_complete"]=false;actors["absence_certified"]=prefix.complete&&prefix.declared==2000;
    bool crosscheck=prefix.complete&&saved.layers.figures&&saved.layers.figures->status==OriginalFigureStatus::Complete&&
        saved.layers.figures->slots.size()==prefix.records.size();
    if(crosscheck)for(size_t n=0;n<prefix.records.size();++n){const auto& p=prefix.records[n];const auto& f=saved.layers.figures->slots[n];
        crosscheck&=p.slot==f.slot&&p.field(18)==f.type&&p.field(22)==f.alive&&p.field(64)==f.action&&
            p.field(98)&&signedWord(*p.field(98))==f.owner&&p.field(130)&&signedWord(*p.field(130))==f.amount&&
            p.field(66)&&signedWord(*p.field(66))==f.route&&p.field(68)&&signedWord(*p.field(68))==f.routeCursor&&
            p.field(70)&&signedWord(*p.field(70))==f.routeLength;}
    actors["strict_reader_crosscheck"]=crosscheck;
    J goods;
    if(prefix.complete&&prefix.declared==2000){const auto g=originalFigureSavedGoodsServices(d,prefix.stopOffset,saved.sizeOffset,saved.terrainOffset);
        if(g){goods=J::object();goods["warehouse36"]=array(g->warehouse);goods["trade5_36"]=array(g->trade5);
            goods["trade7_36"]=array(g->trade7);goods["unavailable36"]=array(g->unavailable);J policies=J::array();
            for(const auto& p:g->policies){J j=J::object();j["accept36"]=array(p.accept);j["limit36"]=array(p.limit);j["trade36"]=array(p.trade);policies.add(j);}goods["policies"]=policies;}}
    out["goods_services"]=goods;
    J records=J::array(),counts=J::object();std::map<unsigned,size_t> liveCounts;
    const auto services=originalFigureSavedServices(d,saved.terrainOffset);
    size_t selected=0;for(const auto& f:prefix.records){const auto type=f.field(18),alive=f.field(22);if(!type||!alive||!*alive)continue;++liveCounts[*type];
        if(*type!=2&&*type!=19&&*type!=21&&*type!=22&&*type!=23&&*type!=24&&*type!=28&&*type!=39)continue;
        if(++selected>256)continue; // Explicit output/memory budget, never silently certify truncation.
        J j=J::object();j["slot"]=f.slot;j["type"]=*type;j["record_complete"]=f.recordComplete;j["alias"]=f.alias;
        j["wire_begin"]=f.begin;j["wire_end"]=f.end;
        for(const auto& pair:{std::pair<const char*,unsigned>{"owner",98},{"target",144},{"home",100},{"delivery",104},
            {"amount",130},{"route",66},{"route_cursor",68},{"route_length",70},{"x",28},{"y",30},{"next",16}}){
            const auto v=f.field(pair.second);j[pair.first]=v?J(signedWord(*v)):J();}
        for(const auto& pair:{std::pair<const char*,unsigned>{"action",64},{"resource",136},{"subtype",19},{"cell",40},
            {"destination_cell",48},{"progress_wire",65},{"route_phase",368},{"speed",97},{"age",276},{"allocation_serial",148}})j[pair.first]=number(f.field(pair.second));
        const auto owner=f.field(98);j["owner_building_known"]=owner&&signedWord(*owner)>0&&links&&links->count(size_t(signedWord(*owner)))>0;
        const auto route=f.field(66),length=f.field(70);J routeState;
        if(route&&length&&*route>0&&*route<1000&&*length<=500&&services){routeState=J::object();routeState["saved_owner"]=services->routeOwners[*route];
            J directions=J::array();for(size_t n=0;n<*length;++n)directions.add(services->directions[*route*500+n]);routeState["directions"]=directions;}
        const auto cursor=f.field(68),cell=f.field(40),next=f.field(16);
        EvidenceRouteCheck routeCheck;if(route&&length&&cursor){OriginalFigure r;r.slot=f.slot;r.type=uint8_t(*type);
            r.action=uint8_t(f.field(64).value_or(0));r.route=int16_t(signedWord(*route));r.routeCursor=int16_t(signedWord(*cursor));r.routeLength=int16_t(signedWord(*length));
            routeCheck=evidenceRouteCheck(r,services?&*services:nullptr);}
        j["route_reference_valid"]=number(routeCheck.referenceValid);j["route_contract_state"]=routeCheck.state;
        j["route_cursor_is_array_index"]=routeCheck.cursorIsArrayIndex;j["reciprocal_route_owner"]=number(routeCheck.reciprocalOwner);
        j["route_contract_scope"]="reference_only_not_complete_actor_semantics";
        j["cell_and_next_bounds_valid"]=cell&&*cell<51984&&next&&*next<2000;
        j["saved_route"]=routeState;J fields=J::array();size_t n=0;
        for(const auto& field:emperor::file_io::research::figure7_fields){J q=J::object();q["receiver_offset"]=field.receiver_offset;q["wire_width"]=field.bytes;
            q["unsigned_wire_value"]=field.bytes<=4?J(f.fields.at(n)):J();if(field.bytes==20)q["opaque_numeric_bytes"]=array(f.opaque380);++n;fields.add(q);}j["base_fields"]=fields;records.add(j);
    }
    for(const auto& c:liveCounts)counts[std::to_string(c.first)]=c.second;actors["live_counts_in_read_prefix"]=counts;actors["records"]=records;
    actors["selected_records_complete"]=prefix.complete&&selected<=256;actors["selected_records_count"]=selected;actors["selected_record_budget"]=256;
    out["actors"]=actors;out["unknown"]=array(std::vector<std::string>{"displayed_year_epoch_mapping","monthly_services_algorithm", "health_weights_and_food_consumption_formula","quality_code_to_recipe_mapping","market_business_capacity","variable_figure_fsa_payload_and_semantics","pre_first_dispatch_history","dynamic_patrol_and_risk_trend"});
    return out;
}
}
int main(){
    try{std::vector<uint8_t> input;char buffer[16384];
        while(std::cin){std::cin.read(buffer,sizeof buffer);const auto n=std::cin.gcount();if(input.size()+size_t(n)>4u*1024u*1024u){std::cerr<<"input_budget\n";return 2;}input.insert(input.end(),buffer,buffer+n);}
        if(!std::cin.eof()){std::cerr<<"input_read_failure\n";return 2;}
        const auto result=exportSnapshot(input);dump(std::cout,result);std::cout<<'\n';return std::get<J::O>(result.v).count("error")?3:0;
    }catch(const std::exception&){std::cerr<<"collector_exception_no_partial_output\n";return 2;}
}
