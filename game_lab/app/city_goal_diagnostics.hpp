// SPDX-License-Identifier: MIT
#pragma once
#include "../original_housing_goal.hpp"
#include "../../file_io/include/city19_goal_archive.hpp"
#include <charconv>
#include <string>

namespace game_lab {
inline const char* appGoalClassLabel(emperor::file_io::research::GoalClassType type) {
    using Type = emperor::file_io::research::GoalClassType;
    switch (type) {
    case Type::Housing: return "住房";
    case Type::Homage: return "朝贡";
    case Type::Trading: return "贸易伙伴";
    case Type::Yearly: return "年度产量";
    case Type::Population: return "人口";
    }
    return "未知类别";
}
// Read-only saved-snapshot diagnostics. The supplied threshold is an explicit
// comparison input, never an inferred active cHousingGoal or completion flag.
inline std::string describeAppCityGoalDiagnostics(const std::vector<uint8_t>& payload,
    const OriginalBuildingProfile& buildings, const std::string& thresholdText) {
    std::string text = "已保存快照 · 只读目标诊断\n活动目标组：未知；不据此放行城市更新或发出完成通知。\n";
    int64_t threshold = 0;
    const auto converted = std::from_chars(thresholdText.data(), thresholdText.data() + thresholdText.size(), threshold);
    if (thresholdText.empty()) text += "住宅比较：尚未输入手动核实的阈值。\n";
    else if (converted.ec != std::errc{} || converted.ptr != thresholdText.data() + thresholdText.size() || threshold <= 0)
        text += "住宅比较：阈值无效，请输入正整数。\n";
    else {
        text += "比较阈值：" + std::to_string(threshold) + "（手动输入，不是自动识别的原版目标）\n";
        const auto result = evaluateOriginalSavedHousingPopulation(payload, buildings, threshold);
        if (const auto* interval = std::get_if<NativeHousingGoalInterval>(&result)) {
            text += "住宅居民总数：" + std::to_string(interval->totalResidents) + "\n";
            text += "满足等级要求的人数范围：[" + std::to_string(interval->lower) + ", " + std::to_string(interval->upper) + "]\n";
            text += "有限评估：";
            text += interval->verdict == NativeHousingGoalVerdict::DefinitelyNotMet ? "确定未达" :
                interval->verdict == NativeHousingGoalVerdict::DefinitelyMet ? "确定达到输入阈值" : "无法确定";
            text += "。住宅等级映射仍未知；范围下界不是原版精确进度。\n";
        } else {
            const auto error = std::get<NativeHousingGoalError>(result);
            text += error == NativeHousingGoalError::IncompleteInventory ? "住宅比较拒绝：住宅清单不完整。\n" :
                error == NativeHousingGoalError::PopulationOverflow ? "住宅比较拒绝：居民数溢出。\n" :
                "住宅比较拒绝：住宅数据无效。\n";
        }
    }
    text += "\n档案目标容器（从0编号；存在不等于启用）：\n";
    try {
        using namespace emperor::file_io::research;
        GoalLimits limits; limits.enable_homage = limits.enable_trading = limits.enable_yearly = limits.enable_population = true;
        const auto goals = walk_city19_housing_goals({payload.data(), std::min(payload.size(), size_t(16384))}, limits);
        for (const auto& member : goals.members) {
            text += "组 " + std::to_string(member.index) + "：" + std::to_string(member.count) + " 项";
            for (const auto& item : goals.items) if (item.member == member.index)
                text += "  #" + std::to_string(item.ordinal) + " " + appGoalClassLabel(goals.cache.at(item.class_id).class_type);
            text += "；启用未知\n";
        }
        text += "结构完成：" + std::to_string(goals.completed_members) + "/10 组，已解析 " + std::to_string(goals.known_items) + " 项。\n";
        if (goals.completed_members != 10) text += "遇到未支持的目标类别；后续清单未知。\n";
    } catch (const std::exception&) {
        text += "目标容器无法完整解析；活动组保持未知。\n";
    }
    return text;
}
} // namespace game_lab
