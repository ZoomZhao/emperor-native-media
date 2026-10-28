// SPDX-License-Identifier: MIT
#pragma once
#include "native_house_runtime_bridge.hpp"
#include <sstream>

namespace game_lab {
// Pure presentation of committed strict records and committed visit receipts.
// Staffing, a live walker, or an old service-grid bit never certify a visit.
struct NativeInspectorWalkerFeedback {
    size_t slot=0;uint8_t action=0;int16_t countdown=0;
};
struct NativeInspectorFeedback {
    bool enabled=false,recordKnown=false,tower=false,upgraded=false;
    uint8_t spawnCounter=0;
    int16_t collapse=0,ignition=0,burning=0;
    std::vector<NativeInspectorWalkerFeedback> walkers;
    std::optional<NativeInspectionVisit> visit;
    std::string error;
};
inline NativeInspectorFeedback queryNativeInspectorFeedback(const NativeSessionState& state,uint64_t id){
    NativeInspectorFeedback out;out.enabled=nativeUsesInspection(state);
    if(!out.enabled)return out;
    if(!state.inspection){out.error="inspection_state_missing";return out;}
    const auto object=std::find_if(state.objects.begin(),state.objects.end(),[&](const auto& o){return o.id==id;});
    if(object==state.objects.end()){out.error="inspection_object_missing";return out;}
    out.tower=object->kind==NativeObjectKind::Inspector;
    const auto record=state.strictRecords.find(id);
    if(record==state.strictRecords.end()){out.error="inspection_record_missing";return out;}
    std::string error;const auto input=decodeNativeStrictRecord(record->second,error);
    if(!input){out.error="inspection_record_invalid:"+error;return out;}
    out.recordKnown=true;out.collapse=input->fireRisk;out.ignition=input->damageRisk;out.burning=input->burning;
    out.spawnCounter=input->spawnCounter;out.upgraded=input->model>0||input->modelUpgrade.value_or(0)!=0;
    if(out.tower){
        std::set<size_t> references;
        if(input->primaryFigure>0)references.insert(size_t(input->primaryFigure));
        if(input->hookIndustry)for(unsigned n=0;n<2;++n){
            const auto slot=detail::i16(input->hookIndustry->data()+0x6a-4+2*n);
            if(slot>0)references.insert(size_t(slot));
        }
        for(const auto& payload:state.inspection->payloads){
            const auto walker=decodeNativeInspector(payload,error);
            if(!walker){out.error="inspection_walker_invalid:"+error;return out;}
            if(walker->owner!=int(id))continue;
            if(!references.erase(walker->slot)){out.error="inspection_owner_reference_mismatch";return out;}
            out.walkers.push_back({walker->slot,walker->action,walker->counter});
        }
        if(!references.empty()){out.error="inspection_walker_reference_missing";return out;}
    }
    const auto visit=state.inspection->visits.find(size_t(id));
    if(visit!=state.inspection->visits.end()){
        if(visit->second.tick>state.ticks){out.error="inspection_visit_in_future";return out;}
        out.visit=visit->second;
    }
    return out;
}
inline const char* nativeInspectorActionLabel(uint8_t action){
    switch(action){case 6:return "出门倒计时";case 7:return "进出塔楼";case 8:return "沿路巡检";case 9:return "返回塔楼";default:return "动作尚未支持";}
}
inline std::string nativeInspectionErrorText(const std::string& error){
    if(error.find("inspection")==std::string::npos&&error.find("inspector")==std::string::npos)return {};
    if(error.find("fire")!=std::string::npos||error.find("disaster")!=std::string::npos||error.find("risk_trigger")!=std::string::npos)
        return "已到未接通的灾害处理边界；本次更新未提交，不能灭火或灾后续跑。";
    if(error.find("model")!=std::string::npos)return "巡检模型不完整；操作未执行。";
    if(error.find("road")!=std::string::npos||error.find("route")!=std::string::npos)return "巡检道路暂不可继续；操作未执行，原状态保留。";
    if(error.find("policy")!=std::string::npos||error.find("requires")!=std::string::npos)return "此存档未启用预防巡检；旧规则保持不变。";
    return "巡检状态不完整或记录不一致；操作未执行，原状态保留。";
}
inline std::string nativeInspectionRecordText(const NativeInspectorFeedback& f,uint64_t now,bool connected,uint32_t workers,uint32_t required,bool detail=false){
    if(!f.enabled)return {};
    if(!f.error.empty())return "巡检状态不可评估；查看不会修改记录。";
    std::ostringstream out;
    if(f.tower){
        out<<(connected?"邻路":"断路")<<" · 工人 "<<workers<<"/"<<required<<"\n";
        if(!connected)out<<"等道路接通";
        else if(!workers)out<<"等待派工";
        else if(f.walkers.empty())out<<"等待派遣 · 计数 "<<unsigned(f.spawnCounter);
        else for(size_t n=0;n<f.walkers.size();++n){if(n)out<<"\n";const auto& w=f.walkers[n];out<<nativeInspectorActionLabel(w.action);if(w.action==6)out<<" "<<std::max<int16_t>(0,w.countdown);if(detail)out<<" · 行人 #"<<w.slot;}
        if(detail)out<<"\n工人或已派出行人不代表住宅已巡到；选择住宅查看实际经过记录。";
    }else{
        out<<"起火 "<<f.ignition<<" · 坍塌 "<<f.collapse;
        if(f.burning)out<<"\n已有燃烧状态；灾害处理尚未接通。";
        else if(f.visit){out<<"\n曾巡检 · 更新 "<<f.visit->tick;
            if(detail)out<<"（距今 "<<now-f.visit->tick<<" 次）\n行人 #"<<f.visit->figure<<"：起火 "<<f.visit->ignitionBefore<<"→"<<f.visit->ignitionAfter<<"，坍塌 "<<f.visit->collapseBefore<<"→"<<f.visit->collapseAfter;
        }else out<<"\n尚无实际巡检记录";
        if(detail)out<<"\n数值为累计计数，不是概率；曾经过也不表示持续保护。";
    }
    return out.str();
}
} // namespace game_lab
