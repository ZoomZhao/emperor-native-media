// SPDX-License-Identifier: MIT
#include "city_goal_diagnostics.hpp"
#include "../original_pak.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks = 0;
void need(bool good, const char* message) { ++checks; if (!good) throw std::runtime_error(message); }
bool has(const std::string& text, const char* part) { return text.find(part) != std::string::npos; }
void authored() {
    OriginalBuildingProfile profile;
    auto text = describeAppCityGoalDiagnostics({}, profile, "");
    need(has(text,"尚未输入") && has(text,"活动目标组：未知"),"no default threshold or active group");
    for (const char* input : {"0", "-1", "+150", "150x", "1.5", " 150", "9223372036854775808"}) {
        text = describeAppCityGoalDiagnostics({},profile,input);
        need(has(text,"阈值无效") && !has(text,"有限评估："),"invalid threshold cannot reuse result");
    }
    text = describeAppCityGoalDiagnostics({},profile,"150");
    need(has(text,"手动输入") && has(text,"住宅清单不完整") && has(text,"活动组保持未知"),"separate comparison and framing refusal");
    need(describeAppCityGoalDiagnostics({},profile,"150")==text,"deterministic pure diagnostics");
    need(profile.status==OriginalBuildingStatus::MissingAnchor && profile.footprints.empty(),"diagnostics preserve profile");
    std::cout << "{\"authored_checks\":" << checks << ",\"passed\":true,\"original_data\":false}\n";
}
void real(int argc,char**argv) {
    const auto env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
    need(argc>=4,"explicit threshold and relative saves required");const auto root=ResourceRoot::fromAbsolute(env);
    for(int n=3;n<argc;++n){
        const auto save=loadOriginalSave(root.resolve(argv[n]));std::vector<uint8_t> raw;
        need(detail::readFile(root.resolve(argv[n]),4194304,raw),"read diagnostic source");
        const auto decoded=decodeOriginalMapContainer(raw);need(decoded.complete,"decode diagnostic source");
        const auto before=decoded.payload;
        const auto report=describeAppCityGoalDiagnostics(decoded.payload,save.buildings,argv[2]);
        need(has(report,"活动目标组：未知")&&has(report,"手动输入")&&has(report,"满足等级要求的人数范围："),"report provenance and interval");
        need(!has(report,"阈值无效")&&!has(report,"住宅比较拒绝"),"real supported interval");
        need(has(report,"结构完成：10/10 组，已解析 11 项"),"complete tutorial groups");
        for(const char* entry:{"组 0：1 项  #0 住房","组 1：1 项  #0 住房","组 2：2 项  #0 住房  #1 朝贡","组 3：3 项  #0 住房  #1 贸易伙伴  #2 年度产量","组 4：2 项  #0 住房  #1 年度产量","组 5：2 项  #0 人口  #1 年度产量","组 9：0 项"})need(has(report,entry),"tutorial member/index retained");
        need(report==describeAppCityGoalDiagnostics(decoded.payload,save.buildings,argv[2])&&before==decoded.payload,"read-only repeated report");
        const auto blank=describeAppCityGoalDiagnostics(decoded.payload,save.buildings,"");
        need(has(blank,"尚未输入")&&!has(blank,"有限评估："),"blank threshold clears prior comparison");
        const auto invalid=describeAppCityGoalDiagnostics(decoded.payload,save.buildings,"x");
        need(has(invalid,"阈值无效")&&!has(invalid,"有限评估："),"invalid comparison clears prior result");
        std::cout<<"{\"source\":"<<std::quoted(argv[n])<<",\"threshold\":"<<std::quoted(argv[2])<<",\"groups\":10,\"objects\":11,\"active_group\":\"unknown\",\"report_passed\":true}\n";
    }
}
}
int main(int argc,char**argv){try{if(argc==1)authored();else if(std::string(argv[1])=="--real")real(argc,argv);else throw std::runtime_error("unknown mode");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
