// SPDX-License-Identifier: MIT
#include "original_evidence_figures.hpp"
#include "original_evidence_route.hpp"
#include "original_evidence_fields.hpp"
#include "original_terrain.hpp"
#include "original_figure_update.hpp"
#include "original_house_goods_contract.hpp"
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void check(bool b){++checks;if(!b)throw std::runtime_error("check_"+std::to_string(checks));}
void word(std::vector<uint8_t>& d,uint32_t v,unsigned n=2){for(unsigned k=0;k<n;++k)d.push_back(k<4?uint8_t(v>>(8*k)):uint8_t(k));}
std::vector<uint8_t> list(unsigned n){std::vector<uint8_t>d;word(d,1);word(d,n,4);return d;}
void record(std::vector<uint8_t>& d,unsigned type,const std::string& name="Figure",bool extension=false,
    const std::map<unsigned,uint32_t>& overrides={}){
    word(d,0xffff);word(d,0);word(d,name.size());d.insert(d.end(),name.begin(),name.end());
    if(name!="Figure")word(d,2);word(d,7);
    unsigned n=1;for(const auto& f:emperor::file_io::research::figure7_fields){
        unsigned v=n++;if(f.receiver_offset==18)v=type;if(f.receiver_offset==22)v=1;
        if(f.receiver_offset==65)v=0x1234;const auto override=overrides.find(f.receiver_offset);
        if(override!=overrides.end())v=override->second;word(d,v,unsigned(f.bytes));}
    d.push_back(extension?1:0);if(name=="cEmissaryDude"){word(d,3,4);d.insert(d.end(),3,0);}
}
EvidenceFigurePrefix parse(const std::vector<uint8_t>& d){return evidenceFigurePrefix(d,0,d.size(),1);}
void put(std::vector<uint8_t>& d,size_t at,uint32_t v,unsigned n=2){for(unsigned k=0;k<n;++k)d.at(at+k)=uint8_t(v>>(8*k));}
}
int main(){try{
    // No footprint argument: an empty city reads the same anchored clock.
    for(const size_t terrain:{size_t(32),size_t(303),size_t(5003)}){
        const size_t end=terrain+original_save19::c82ef4+4,side=end+176;
        std::vector<uint8_t> raw(side+4,0xa5);put(raw,0,19);put(raw,terrain+original_save19::c82ef8,50,4);
        put(raw,terrain+original_save19::c82ef0,15,4);put(raw,terrain+original_save19::c82ef4,0xffffffffu,4);put(raw,side-176,11,4);
        const auto frozen=raw;const auto clock=evidenceSavedClock(raw,terrain,side);
        check(clock&&clock->rangesValid()&&clock->phase==50&&clock->monthPart==15&&clock->cycle==0xffffffffu&&clock->month==11);
        check(raw==frozen);check(!evidenceSavedClock(raw,SIZE_MAX,side));check(!evidenceSavedClock(raw,terrain,SIZE_MAX));
        check(!evidenceSavedClock(raw,terrain,175));
        for(size_t cut=end-12;cut<end;++cut)check(!evidenceSavedClock(std::vector<uint8_t>(raw.begin(),raw.begin()+cut),terrain,cut));
        for(const auto row:std::vector<std::array<uint32_t,3>>{{0,0,0},{50,15,11},{51,15,11},{50,16,11},{50,15,12},{0xffffffffu,0,0}}){
            auto bad=raw;put(bad,terrain+original_save19::c82ef8,row[0],4);put(bad,terrain+original_save19::c82ef0,row[1],4);put(bad,side-176,row[2],4);
            const auto c=evidenceSavedClock(bad,terrain,side);check(c&&c->phase==row[0]&&c->monthPart==row[1]&&c->month==row[2]);
            check(c->rangesValid()==(row[0]<=50&&row[1]<=15&&row[2]<=11));}
        auto bad=raw;put(bad,0,18);check(!evidenceSavedClock(bad,terrain,side));
    }
    check(!evidenceSavedClock({},0,0));check(!evidenceSavedClock(std::vector<uint8_t>(8u*1024u*1024u+1),0,176));
    // The producer reader owns framing. Poison neighbors and compare two
    // independently existing Common5 readers; no simulated production here.
    for(const uint16_t type:{33,35,43}){
        OriginalBuildingProfile profile;profile.status=OriginalBuildingStatus::Complete;profile.parsedRecords=4000;
        OriginalBuildingFootprint f;f.slot=7;f.type=type;f.size=2;f.recordOffset=0;profile.footprints.push_back(f);profile.slotTypes[7]=type;
        const size_t common=2,tail=184,tailBytes=type==35?136:134;
        std::vector<uint8_t> raw(tail+tailBytes,0xa5);put(raw,0,0x8001);put(raw,common,5);put(raw,common+16,type);put(raw,common+162,7,4);
        for(const size_t offset:{size_t(0),size_t(2),size_t(130),size_t(132)})put(raw,tail+offset,1);
        put(raw,common+18,5);put(raw,common+23,1);put(raw,common+58,0x8000);put(raw,common+62,7);
        raw[common+98]=0;put(raw,common+101,0x7fff);put(raw,common+103,9);raw[common+129]=1;
        put(raw,tail+4,0xfffe);raw[tail+4+88]=3;raw[tail+4+91]=4;raw[tail+4+95]=5;
        const auto frozen=raw;const auto pp=originalBuildingProductionInputs(raw,profile);const auto ll=originalFigureBuildingLinks(raw,profile);
        check(pp&&pp->size()==1&&ll&&ll->count(7));const auto& p=pp->at(7);const auto& b=ll->at(7);
        check(p.primaryStock==-32768&&p.secondaryStock==32767&&p.primaryResource==5&&p.secondaryResource==9);
        check(p.operational==1&&p.workers==7&&p.laborDisabled==0&&p.enabled==1);
        check(detail::i16(p.industry.data())==-2&&p.industry[88]==3&&p.industry[91]==4&&p.industry[95]==5);
        check(raw==frozen);check(!evidenceHunterProductionCrosscheck(b,nullptr));
        if(type==33){check(evidenceHunterProductionCrosscheck(b,&p)==true);check(b.delegatedCount==3);
            auto different=b;++different.hunterReturnGoods;check(evidenceHunterProductionCrosscheck(different,&p)==false);
            different=b;different.delegatedCount.reset();check(evidenceHunterProductionCrosscheck(different,&p)==false);
            different=b;++*different.delegatedCount;check(evidenceHunterProductionCrosscheck(different,&p)==false);
        }else check(!evidenceHunterProductionCrosscheck(b,&p));
        for(size_t cut=0;cut<raw.size();++cut)check(!originalBuildingProductionInputs(std::vector<uint8_t>(raw.begin(),raw.begin()+cut),profile));
        for(const size_t offset:{size_t(0),size_t(2),size_t(130),size_t(132)}){auto bad=raw;put(bad,tail+offset,2);check(!originalBuildingProductionInputs(bad,profile));}
        auto bad=raw;put(bad,common+162,8,4);check(!originalBuildingProductionInputs(bad,profile));
    }
    check(evidenceSelectedFigure(19)&&evidenceSelectedFigure(20)&&evidenceSelectedFigure(40)&&evidenceSelectedFigure(54)&&evidenceSelectedFigure(76));
    check(!evidenceSelectedFigure(0)&&!evidenceSelectedFigure(78));
    for(const unsigned type:{40u,54u,75u,76u,77u}){
        auto wire=list(1);record(wire,type,type==76?"cPheasantFig":"Figure",false,
            {{62,0x8000},{44,0xffff},{46,32767},{52,3},{54,4},{111,1},{328,0xffffffffu},{288,0x80000000u},{168,123},{297,1}});
        const auto frozen=wire;const auto prefix=parse(wire);check(prefix.complete&&wire==frozen);
        const auto fields=evidenceHuntFields(prefix.records[0]);check(fields&&fields->size()==14);
        check(fields->at("counter_s16")==-32768&&fields->at("destination_x_s16")==-1&&fields->at("destination_y_s16")==32767);
        check(fields->at("linked_building_wire_u32")==4294967295LL&&fields->at("flags120_wire_u32")==2147483648LL);
        check(fields->at("hidden_wire_u8")==1&&fields->at("projectile_wire_u8")==1);
        auto truncated=prefix.records[0];truncated.fields.clear();check(!evidenceHuntFields(truncated));
        auto fsa=list(2);record(fsa,type,"Figure",true);const auto end=fsa.size();record(fsa,23);
        const auto stop=parse(fsa);check(!stop.complete&&stop.records.size()==1&&!stop.records[0].recordComplete&&stop.stopOffset==end);
        check(evidenceHuntFields(stop.records[0]).has_value()); // Known base remains explicitly PARTIAL, never later records/Goods.
    }
    // Adjacent fields are poisoned; offsets include Common5's version word,
    // not runtime receiver offsets. Preserve every signed16 bit pattern.
    std::array<uint8_t,182> common;common.fill(0xa5);
    check(!originalBuildingRiskWords(nullptr,common.size()));
    for(size_t length=0;length<74;++length)check(!originalBuildingRiskWords(common.data(),length));
    for(const int value:{-32768,-1,0,918,999,1000,32767}){
        for(const auto offset:{size_t(68),size_t(70),size_t(72)}){
            auto bytes=common;const uint16_t bits=uint16_t(value);
            bytes[offset]=uint8_t(bits);bytes[offset+1]=uint8_t(bits>>8);const auto frozen=bytes;
            const auto risk=originalBuildingRiskWords(bytes.data(),bytes.size());check(bool(risk));
            check((offset==68?risk->collapse:offset==70?risk->ignition:risk->burning)==value);
            check(bytes==frozen);
            check(originalBuildingRiskWords(bytes.data(),74).has_value());
        }
    }
    auto d=list(1);record(d,23);const auto frozen=d;const auto p=parse(d);
    check(p.complete&&p.declared==1&&p.completed==1&&p.records.size()==1);
    check(d==frozen);check(p.records[0].field(18)==23);check(p.records[0].field(65)==0x1234);
    check(!p.records[0].field(999));check(p.records[0].fields.size()==std::size(emperor::file_io::research::figure7_fields));
    size_t n=0;for(const auto& f:emperor::file_io::research::figure7_fields){
        if(f.bytes<=4)check(p.records[0].field(f.receiver_offset)==p.records[0].fields[n]);
        else {check(!p.records[0].field(f.receiver_offset));check(p.records[0].opaque380[19]==19);}++n;}
    for(size_t cut=0;cut<d.size();++cut){const auto r=parse(std::vector<uint8_t>(d.begin(),d.begin()+cut));check(!r.complete);check(r.stopOffset<=cut);}
    auto alias=list(2);record(alias,24);word(alias,2);const auto a=parse(alias);
    check(a.complete&&a.completed==2&&a.records[1].alias&&a.records[1].slot==1);
    check(a.records[1].fields==a.records[0].fields);
    auto bad=list(1);word(bad,2);check(parse(bad).reason=="object_reference");
    bad=list(1);word(bad,0x8001);check(parse(bad).reason=="class_reference");
    bad=list(1);word(bad,0x7fff);word(bad,0x80000001,4);check(parse(bad).reason=="class_reference");
    auto fsa=list(2);record(fsa,24,"Figure",true);const auto bound=fsa.size();record(fsa,23);
    const auto q=parse(fsa);check(!q.complete&&q.completed==0&&q.records.size()==1&&!q.records[0].recordComplete);
    check(q.reason=="variable_fsa_unqualified"&&q.stopOffset==bound&&q.records[0].field(18)==24);
    auto unknown=list(2);record(unknown,23);record(unknown,24,"cSalamanderFig");
    const auto u=parse(unknown);check(!u.complete&&u.completed==1&&u.records.size()==1&&u.reason=="unqualified_class");
    check(u.unqualifiedClass=="cSalamanderFig");
    auto emissary=list(1);record(emissary,24,"cEmissaryDude");check(parse(emissary).complete);
    auto pheasant=list(1);record(pheasant,23,"cPheasantFig");check(parse(pheasant).complete);
    bad=d;bad[0]=2;check(parse(bad).reason=="list_version");
    bad=list(8193);check(parse(bad).reason=="count_budget");
    check(evidenceFigurePrefix(d,1,0,1).reason=="bounds");
    check(evidenceFigurePrefix(d,0,d.size(),0).reason=="bounds");
    check(evidenceFigurePrefix(d,0,d.size(),32769).reason=="budget");
    check(evidenceFigurePrefix(d,0,d.size(),32767).reason=="cache_budget");
    check(parse(std::vector<uint8_t>(8u*1024u*1024u+1)).reason=="budget");
    OriginalHousePayload house{};OriginalHouseGoods goods{};
    for(size_t k=0;k<goods.size();++k)goods[k]=int16_t(100+k);
    check(originalHouseGoodsToPayload(house,goods));check(originalHouseGoodsFromPayload(house)==goods);
    check(house[13]==100&&house[14]==0&&house[15]==101);check(!originalHouseGoodsOffset(12));
    check(!originalHouseGoodsFromPayload(house.data(),house.size()-1));
    OriginalFigureSavedServices services;services.directions.resize(500000,2);services.routeOwners[7]=42;
    OriginalFigure f;f.slot=42;f.alive=1;f.type=23;f.action=1;f.route=0;f.routeCursor=10;f.routeLength=0;
    auto r=evidenceRouteCheck(f,nullptr);check(r.referenceValid==true&&!r.cursorIsArrayIndex);
    check(std::string(r.state)=="roaming_without_route"&&f.routeCursor==10&&f.routeLength==0);
    f.type=28;check(evidenceRouteCheck(f,nullptr).referenceValid==true); // Actual320112 shape.
    f.type=24;f.action=6;check(evidenceRouteCheck(f,nullptr).referenceValid==true);
    check(std::string(evidenceRouteCheck(f,nullptr).state)=="no_active_route_retained_counters");
    f.routeCursor=-32768;f.routeLength=-1;check(!evidenceRouteCheck(f,nullptr).referenceValid.has_value()); // Need terrain lookahead context.
    f.routeCursor=32767;check(evidenceRouteCheck(f,nullptr).referenceValid==true); // Retained, not indexed.
    f.routeCursor=0;f.routeLength=2;check(!evidenceRouteCheck(f,nullptr).referenceValid.has_value());
    f.route=7;f.routeCursor=0;f.routeLength=2;
    r=evidenceRouteCheck(f,&services);check(r.referenceValid==true&&r.cursorIsArrayIndex&&r.reciprocalOwner==true);
    auto direction=originalFigureSavedRouteDirection(f,services);check(direction&&direction->state.direction==2);
    check(!evidenceRouteCheck(f,nullptr).referenceValid.has_value());
    services.routeOwners[7]=43;check(evidenceRouteCheck(f,&services).referenceValid==false);
    f.routeCursor=2;r=evidenceRouteCheck(f,&services);check(r.referenceValid==true&&!r.cursorIsArrayIndex&&r.reciprocalOwner==false);
    direction=originalFigureSavedRouteDirection(f,services);check(direction&&direction->state.route==0);
    check(direction->state.routeCursor==2&&direction->state.routeLength==2&&direction->services.routeOwners[7]==43);
    check(evidenceRouteCheck(direction->state,&direction->services).referenceValid==true);
    services.routeOwners[7]=42;f.routeCursor=10;
    direction=originalFigureSavedRouteDirection(f,services);check(direction&&direction->state.route==0&&direction->services.routeOwners[7]==0);
    check(direction->state.routeCursor==10&&direction->state.routeLength==2&&services.routeOwners[7]==42);
    check(evidenceRouteCheck(f,&services).referenceValid==true);check(evidenceRouteCheck(direction->state,nullptr).referenceValid==true);
    f.route=1000;check(evidenceRouteCheck(f,&services).referenceValid==false);
    f.route=-1;check(!evidenceRouteCheck(f,&services).referenceValid.has_value());
    f.route=7;f.routeCursor=-1;check(evidenceRouteCheck(f,&services).referenceValid==false);
    f.routeCursor=0;f.routeLength=501;check(evidenceRouteCheck(f,&services).referenceValid==false);
    f.routeLength=-1;check(evidenceRouteCheck(f,&services).referenceValid==false);
    f.route=0;f.alive=0;check(evidenceRouteCheck(f,&services).referenceValid==true); // Post-release/reset references.
    // Serialization/restore keeps route counters verbatim across roam, active,
    // exhausted and released states; the inspector never repairs the data.
    for(const auto& state:std::vector<std::array<int,4>>{{23,0,10,0},{28,0,32767,0},{24,7,0,2},{23,7,10,2},{23,0,10,2}}){
        auto wire=list(1);record(wire,unsigned(state[0]),"Figure",false,
            {{64,1},{66,uint16_t(state[1])},{68,uint16_t(state[2])},{70,uint16_t(state[3])}});
        const auto before=wire;const auto restored=parse(wire);check(restored.complete&&wire==before);
        const auto& p=restored.records[0];OriginalFigure snapshot;snapshot.slot=42;snapshot.type=uint8_t(*p.field(18));snapshot.action=uint8_t(*p.field(64));
        const auto signed16=[](uint32_t v){return int16_t(v<=32767?int(v):int(v)-65536);};
        snapshot.route=signed16(*p.field(66));snapshot.routeCursor=signed16(*p.field(68));snapshot.routeLength=signed16(*p.field(70));
        check(snapshot.route==state[1]&&snapshot.routeCursor==state[2]&&snapshot.routeLength==state[3]);
        check(evidenceRouteCheck(snapshot,&services).referenceValid==true);
    }
    std::cout<<checks<<"/"<<checks<<" authored evidence-format checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
