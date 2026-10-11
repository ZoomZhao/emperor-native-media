#pragma once
#include "city_goal_feedback_adapter.hpp"
#include "city_scenario_checkpoint.hpp"
#include <iterator>

namespace game_lab {

// Separate, replaceable authored level. The original three-entry catalog is unchanged.
inline const CityScenarioDefinition& selfAuthoredWarehouseTraining() {
    static const CityScenarioDefinition definition{
        {4, 1, {32, 650, 4000, 0}, 24}, "官仓通路",
        "24个模拟月内同时达到人口32、存粮650、国库4000、月度收支不亏。"
        "仅用于仓储v4新城；初始官仓容量600，需增仓并用四邻道路让农产入仓。"
        "可先体验满仓、断路和重连；这些是操作练习，不是额外强制通关步骤。"};
    return definition;
}

inline bool isCityWarehouseTrainingSpec(const CityScenarioSpec& spec) {
    return spec.id == 4 && spec.revision == 1 && city_scenario_detail::validSpec(spec);
}

namespace city_warehouse_training_detail {
inline CityScenarioUpdate reject(const CityScenarioSession& session, std::string message) {
    CityScenarioUpdate result;
    result.active = session.active();
    result.status = session.checkpoint().status;
    result.message = std::move(message);
    return result;
}
inline CityScenarioClock clock(const CitySimulation& city) {
    return {city.stats().year, city.stats().month};
}
inline bool validLedger(const CityLogisticsMonth& m) {
    const int fields[] = {m.beginStored,m.produced,m.delivered,m.noRouteDiscard,m.fullDiscard,
                          m.consumed,m.unmetDemand,m.endStored};
    if (!m.valid) return std::all_of(std::begin(fields),std::end(fields),[](int n){return n == 0;});
    if (std::any_of(std::begin(fields),std::end(fields),[](int n){return n < 0;})) return false;
    // Conservative published v4 save-ledger bounds; not a production/consumption implementation.
    constexpr int64_t maxStored = int64_t(CitySimulation::kTotalTiles) * CityWarehouseRules::capacity;
    constexpr int64_t maxHarvest = int64_t(CitySimulation::kTotalTiles) *
        (FarmProductionRules::regularYield + FarmProductionRules::autumnBonus);
    constexpr int64_t maxDemand = 78644;
    if (m.beginStored > maxStored || m.endStored > maxStored || m.produced > maxHarvest ||
        m.delivered > maxHarvest || m.noRouteDiscard > maxHarvest || m.fullDiscard > maxHarvest ||
        int64_t(m.consumed) + m.unmetDemand > maxDemand) return false;
    return int64_t(m.produced) == int64_t(m.delivered) + m.noRouteDiscard + m.fullDiscard &&
           int64_t(m.beginStored) + m.delivered == int64_t(m.endStored) + m.consumed;
}
} // namespace city_warehouse_training_detail

// Only these guards opt the new level into v4; no reset, migration, resource grant or simulation mutation.
inline CityScenarioUpdate beginCityWarehouseTraining(CityScenarioSession& session,
    const CitySimulation& city, uint64_t runId, const CityScenarioAwardBook& book) {
    if (city.ruleset() != CityRuleset::RoadWarehouses)
        return city_warehouse_training_detail::reject(session,"unknown：官仓通路需要明确新建仓储v4城市，不自动转换旧城。");
    return session.begin(selfAuthoredWarehouseTraining().spec,runId,
                         city_warehouse_training_detail::clock(city),book);
}

inline CityScenarioUpdate observeCityWarehouseTraining(CityScenarioSession& session,
    const CitySimulation& city, CityScenarioAwardBook& book) {
    if (!session.active() || !isCityWarehouseTrainingSpec(session.checkpoint().spec) ||
        city.ruleset() != CityRuleset::RoadWarehouses)
        return city_warehouse_training_detail::reject(session,"unknown：当前不是配套仓储v4训练会话；未修改期限或奖励账本。");
    const auto snapshot = makeCityFeedbackSnapshot(city,CityHarvestRule::RoadWarehousesV1);
    return session.observe(city_warehouse_training_detail::clock(city),
                           evaluateCityGoals(snapshot,session.checkpoint().spec.goals),book);
}

// The caller must first validate city-body/container binding and use candidate objects.
// This guard checks rule family and date; it cannot identify a foreign city with the same date.
inline CityScenarioUpdate restoreCityWarehouseTraining(CityScenarioSession& session,
    const CityScenarioCheckpoint& saved, const CitySimulation& loadedCity, CityScenarioAwardBook& book) {
    if (!isCityWarehouseTrainingSpec(saved.spec) || loadedCity.ruleset() != CityRuleset::RoadWarehouses)
        return city_warehouse_training_detail::reject(session,"unknown：训练配置与仓储城市规则不匹配，保留旧会话。");
    return session.restore(saved,city_warehouse_training_detail::clock(loadedCity),book);
}

struct CityWarehouseTrainingSnapshot {
    bool warehouseRules = false;
    CityScenarioClock clock;
    CityFeedbackSnapshot city;
    CityLogisticsOverview network;
    CityLogisticsMonth lastMonth;
    CityLogisticsMonth nextMonth; // The core's real copied month, not a duplicate forecast formula.
};

inline CityWarehouseTrainingSnapshot makeCityWarehouseTrainingSnapshot(const CitySimulation& city) {
    return {city.ruleset() == CityRuleset::RoadWarehouses,city_warehouse_training_detail::clock(city),
            makeCityFeedbackSnapshot(city,CityHarvestRule::RoadWarehousesV1),
            city.logisticsOverview(),city.lastLogisticsMonth(),city.nextMonthLogistics()};
}

enum class CityWarehouseTrainingHintCode {
    Unknown, Population, NoFarms, Capacity, Disconnected, NoProduction,
    LastNoRouteLoss, LastCapacityLoss, NextNoRouteLoss, NextCapacityLoss, Connected
};
struct CityWarehouseTrainingHint {
    CityWarehouseTrainingHintCode code;
    CityFeedbackSeverity severity;
    std::string text;
};
struct CityWarehouseTrainingFeedback {
    bool valid = false;
    CityGoalFeedback goals;
    std::vector<CityWarehouseTrainingHint> hints;
};

inline CityWarehouseTrainingFeedback evaluateCityWarehouseTraining(
    const CityWarehouseTrainingSnapshot& s,
    const CityGoalConfig& goals = selfAuthoredWarehouseTraining().spec.goals) {
    CityWarehouseTrainingFeedback result;
    result.goals = evaluateCityGoals(s.city,goals);
    const auto& o = s.network;
    const auto& next = s.nextMonth;
    const bool valid = s.warehouseRules && result.goals.valid && result.goals.nextMonth.known &&
        s.city.harvestRule == CityHarvestRule::RoadWarehousesV1 && s.clock.month == s.city.month &&
        o.warehouses == s.city.warehouses && o.warehouses >= 0 &&
        int64_t(o.capacity) == int64_t(o.warehouses) * CityWarehouseRules::capacity &&
        o.stored == s.city.foodSupply && o.connectedFarms >= 0 && o.disconnectedFarms >= 0 &&
        int64_t(o.connectedFarms) + o.disconnectedFarms == s.city.farms &&
        // Necessary implications only for the next month under the current road layout.
        // Mixed connectivity may receive zero integer share; lastMonth may predate road edits.
        (o.disconnectedFarms != 0 || next.noRouteDiscard == 0) &&
        (o.connectedFarms != 0 || (next.delivered == 0 && next.fullDiscard == 0)) &&
        city_warehouse_training_detail::validLedger(s.lastMonth) &&
        city_warehouse_training_detail::validLedger(next) && next.valid &&
        (!s.lastMonth.valid || s.lastMonth.endStored == o.stored) && next.beginStored == o.stored &&
        s.city.coreHarvest && next.produced == *s.city.coreHarvest &&
        s.city.coreDelivered && next.delivered == *s.city.coreDelivered &&
        int64_t(next.consumed) + next.unmetDemand == result.goals.nextMonth.consumption &&
        s.city.coreNextMonth && next.endStored == s.city.coreNextMonth->foodSupply;
    if (!valid) {
        result.hints.push_back({CityWarehouseTrainingHintCode::Unknown,CityFeedbackSeverity::Unknown,
            "unknown：缺少配套仓储v4道路/容量/月账快照，不能判断满仓或断路原因。"});
        return result;
    }
    result.valid = true;
    const auto hint = [&](CityWarehouseTrainingHintCode code,CityFeedbackSeverity severity,std::string text) {
        result.hints.push_back({code,severity,std::move(text)});
    };
    if (s.lastMonth.valid && s.lastMonth.noRouteDiscard > 0)
        hint(CityWarehouseTrainingHintCode::LastNoRouteLoss,CityFeedbackSeverity::Warning,
             "上次月结因断路弃置"+std::to_string(s.lastMonth.noRouteDiscard)+"担。接回道路只恢复下次月结，不补发已损耗的粮。" );
    if (s.lastMonth.valid && s.lastMonth.fullDiscard > 0)
        hint(CityWarehouseTrainingHintCode::LastCapacityLoss,CityFeedbackSeverity::Warning,
             "上次月结因满仓弃置"+std::to_string(s.lastMonth.fullDiscard)+"担。先入仓再消费，月底尚有空位也可能已经损耗；增仓后接入同一路网。" );
    if (o.disconnectedFarms > 0)
        hint(CityWarehouseTrainingHintCode::Disconnected,CityFeedbackSeverity::Warning,
             std::to_string(o.disconnectedFarms)+"座农庄没有可达官仓。用上下左右相邻的连续道路接到仓库；斜角或农庄直接挨仓都不算。" );
    if (o.capacity < goals.foodReserve)
        hint(CityWarehouseTrainingHintCode::Capacity,CityFeedbackSeverity::Warning,
             "总仓容量"+std::to_string(o.capacity)+"担，低于存粮目标"+std::to_string(goals.foodReserve)+
             "担。另建官仓并接路；只造仓不接路，农产仍可能无法入仓。" );
    if (next.noRouteDiscard > 0)
        hint(CityWarehouseTrainingHintCode::NextNoRouteLoss,CityFeedbackSeverity::Warning,
             "保持当前布局，下次真实月结预演将因断路弃置"+std::to_string(next.noRouteDiscard)+"担。推进月份前补路。" );
    if (next.fullDiscard > 0)
        hint(CityWarehouseTrainingHintCode::NextCapacityLoss,CityFeedbackSeverity::Warning,
             "保持当前布局，下次真实月结预演将因满仓弃置"+std::to_string(next.fullDiscard)+"担。可达仓库需要腾出或增加容量。" );
    if (s.city.farms == 0)
        hint(CityWarehouseTrainingHintCode::NoFarms,CityFeedbackSeverity::Info,
             "初始400担是有限库存。建农庄，并用连续道路接到官仓，才能储备新粮。" );
    else if (next.produced == 0)
        hint(CityWarehouseTrainingHintCode::NoProduction,CityFeedbackSeverity::Warning,
             "下次月结预演产量为0。检查人口和共用岗位用工；接好路也不能替代劳动力。" );
    if (s.city.population < goals.population)
        hint(CityWarehouseTrainingHintCode::Population,CityFeedbackSeverity::Info,
             "人口距离目标还差"+std::to_string(goals.population-s.city.population)+"。先建足够民居，再逐月等待迁入；农庄和官仓共享劳动力。" );
    if (o.connectedFarms > 0)
        hint(CityWarehouseTrainingHintCode::Connected,CityFeedbackSeverity::Info,
             std::to_string(o.connectedFarms)+"座农庄已能经道路到达官仓。道路连通与仓库余量分别检查，通路不保证全部产量都能入仓。" );
    return result;
}

} // namespace game_lab
