// SPDX-License-Identifier: MIT
// Authored presentation contracts only. No assets, original game, or GUI.
#include "native_session_presentation.hpp"
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
std::string snapshot(const NativeSessionState& state){auto copy=state;native_runtime_wire::Writer w;
    native_runtime_wire::stateFields(w,copy);w.field("inspection",copy.inspection);w.field("ticks",copy.ticks);return w.out.str();}
NativeSessionState fixture(){
    NativeSessionState s;s.rulesVersion=8;s.ticks=100;s.runtimePolicies.push_back(nativeInspectionTag);s.inspection.emplace();
    s.objects.push_back({1,NativeObjectKind::House,0,0,7});s.objects.back().house.emplace();
    s.objects.push_back({3,NativeObjectKind::Inspector,3,0,0});
    OriginalBuildingUpdateInputs house;house.identity=1;house.alive=1;house.house.emplace();house.fireRisk=126;house.damageRisk=918;
    s.strictRecords[1]=encodeNativeStrictRecord(house);
    OriginalBuildingUpdateInputs tower;tower.identity=3;tower.alive=1;tower.workers=5;tower.hookIndustry.emplace();tower.hookIndustry->fill(0);tower.modelUpgrade=0;
    s.strictRecords[3]=encodeNativeStrictRecord(tower);return s;
}
std::string payload(size_t slot,uint8_t action=8){OriginalFigure f;f.slot=slot;f.type=39;f.alive=1;f.owner=3;f.action=action;f.counter=30;
    std::string error;const auto p=encodeNativeInspector(f,error);need(bool(p),"authored inspector payload");return *p;}
}
int main(){try{
    auto s=fixture();const auto frozen=snapshot(s);
    auto h=queryNativeInspectorFeedback(s,1);
    need(h.enabled&&h.recordKnown&&!h.tower&&h.error.empty(),"house record visible");
    need(h.collapse==126&&h.ignition==918&&!h.burning&&!h.visit,"risk fields retain canonical causes");
    auto text=nativeInspectionRecordText(h,s.ticks,true,5,5,true);
    need(text.find("918")!=std::string::npos&&text.find("126")!=std::string::npos,"both risk words displayed");
    need(text.find("不是概率")!=std::string::npos&&text.find("尚无实际巡检记录")!=std::string::npos,"workers never imply visit or probability");
    auto tower=queryNativeInspectorFeedback(s,3);
    need(tower.tower&&tower.walkers.empty()&&!tower.visit,"staffed tower has no fabricated patrol");
    need(nativeInspectionRecordText(tower,s.ticks,true,5,5).find("等待派遣")!=std::string::npos,"staffed tower waits for actual dispatch");
    need(nativeInspectionRecordText(tower,s.ticks,true,0,5).find("等待派工")!=std::string::npos,"zero workers reported separately");
    need(nativeInspectionRecordText(tower,s.ticks,false,5,5).find("等道路接通")!=std::string::npos,"road absence takes precedence");
    need(snapshot(s)==frozen,"query leaves complete runtime fields byte identical");
    s.inspection->visits.emplace(1,NativeInspectionVisit{97,2,150,940,126,918});
    const auto visited=snapshot(s);h=queryNativeInspectorFeedback(s,1);
    need(h.visit&&h.visit->tick==97,"only committed receipt supplies last visit");
    text=nativeInspectionRecordText(h,s.ticks,true,5,5,true);
    need(text.find("曾巡检")!=std::string::npos&&text.find("距今 3")!=std::string::npos,"historical visit uses native ticks");
    need(text.find("不表示持续保护")!=std::string::npos,"historical visits do not certify future safety");
    need(snapshot(s)==visited,"visit detail is read only");
    std::string error;auto input=decodeNativeStrictRecord(s.strictRecords.at(3),error);need(bool(input),"tower decoded");
    input->primaryFigure=2;s.strictRecords[3]=encodeNativeStrictRecord(*input);
    for(uint8_t action=6;action<=9;++action){s.inspection->payloads={payload(2,action)};
        const auto before=snapshot(s);tower=queryNativeInspectorFeedback(s,3);
        need(tower.walkers.size()==1&&tower.walkers.front().action==action&&tower.error.empty(),"each supported action has real walker");
        need(nativeInspectionRecordText(tower,s.ticks,true,5,5,true).find(nativeInspectorActionLabel(action))!=std::string::npos,"action-specific label");
        need(snapshot(s)==before,"walker status never advances countdown or movement");
    }
    input->model=1;input->hookIndustry->at(0x6c-4)=4;s.strictRecords[3]=encodeNativeStrictRecord(*input);s.inspection->payloads={payload(2),payload(4,9)};
    tower=queryNativeInspectorFeedback(s,3);need(tower.upgraded&&tower.walkers.size()==2&&tower.error.empty(),"upgraded tower exposes both actual owned walkers");
    s.inspection->payloads.pop_back();need(!queryNativeInspectorFeedback(s,3).error.empty(),"dangling auxiliary is not shown as idle");
    s.inspection->payloads={payload(2),payload(4,9)};s.inspection->visits.at(1).tick=101;
    need(queryNativeInspectorFeedback(s,1).error=="inspection_visit_in_future","future receipt refused");
    s.inspection->visits.at(1).tick=97;s.strictRecords.erase(1);need(!queryNativeInspectorFeedback(s,1).recordKnown,"missing risk record not zero-filled");
    s.runtimePolicies.clear();need(!queryNativeInspectorFeedback(s,1).enabled,"old strategy never gains inspection by optional state alone");
    need(nativeToolCategory(6)==2&&!nativeToolAvailable(6,8)&&nativeToolAvailable(6,8,true),"new direct tool requires explicit strategy");
    need(!nativeToolAvailable(6,6,true)&&!nativeToolAvailable(7,8,true),"old rules and unknown tools stay rejected");
    for(const char* reason:{"inspection_fire_target_unported","inspection_disaster_trigger_unported"})
        need(nativeInspectionErrorText(reason).find("不能灭火")!=std::string::npos,"unported fire never advertised as supported");
    need(nativeInspectionErrorText("inspection_model_inputs_missing").find("模型")!=std::string::npos,"model refusal translated");
    need(nativeInspectionErrorText("inspection_road_removal_would_strand").find("道路")!=std::string::npos,"road refusal translated");
    std::cout<<"{\"passed\":true,\"authored_checks\":"<<checks<<",\"GUI\":false,\"original_data\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
