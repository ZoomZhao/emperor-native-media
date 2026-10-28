// SPDX-License-Identifier: MIT
#include "original_evidence_figures.hpp"
#include "original_evidence_route.hpp"
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
}
int main(){try{
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
