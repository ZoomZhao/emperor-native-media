#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace game_lab {

// Self-authored prototype rules and replaceable level goals, not original-game rules.
// This header depends only on the C++17 standard library; evaluation has no side effects.
enum class CityHarvestRule {
    Unknown,
    BaselineBb00e6b,       // Regular 5/farm; autumn uses max(30%, integer labor percent).
    LaborLimitedPrototype, // Reserved old double-rule ID; deliberately returns unknown in v2.
    LaborLimitedIntegerV1, // Exact pooled integer harvest with legacy spatial services.
    StaffedServicesV1      // EMP1 v3: core predicts housing and migration after staffing changes.
};

struct CityCoreNextMonth {
    int month = 0, population = 0, housingCapacity = 0, foodSupply = 0, treasury = 0;
};

struct CityFeedbackSnapshot {
    int treasury = 5000;
    int population = 0;
    int housingCapacity = 0;
    int foodSupply = 400;
    int month = 1;
    int speed = 1;
    int roads = 0, houses = 0, farms = 0, wells = 0;
    int temples = 0, markets = 0, watchtowers = 0;
    CityHarvestRule harvestRule = CityHarvestRule::Unknown;
    std::optional<int64_t> coreHarvest; // Read-only adapter may supply the authoritative next-month helper result.
    std::optional<CityCoreNextMonth> coreNextMonth;
};

struct CityGoalConfig {
    // Replace these values for another self-authored level. They are not canonical.
    int population = 50;
    int foodReserve = 120;
    int treasuryReserve = 500;
    int monthlyBalance = 0; // Current population tax minus recurring upkeep, excluding construction.
};

struct CityGoalMemory {
    // Transient UI memory. Reset for a new city, a new map, a load, or changed goals.
    bool everMet = false;
    bool metLastEvaluation = false;
};

enum class CityGoalState { Unknown, InProgress, Met, Regressed };
enum class CityGoalTransition { None, Reached, Lost, Restored };
enum class CityFeedbackSeverity { Info, Warning, Critical, Unknown };
enum class CityFeedbackCode {
    InvalidSnapshot, InvalidGoals, InvalidHistory, UnknownHarvestRule, Paused,
    NoHousing, HousingFull, HousingBelowGoal, HousingCoverageUnknown, ServiceLaborShortage, HousingDisplacement,
    NoFarms, LaborShortage, FoodDeficit, Famine, MigrationFoodBlocked,
    MonthlyDeficit, TreasuryDebt, ConstructionFundsLow
};

struct CityGoalProgress {
    std::string label;
    int64_t current = 0;
    int64_t target = 0;
    int64_t remaining = 0;
    bool met = false;
};

struct CityFeedbackIssue {
    CityFeedbackCode code;
    CityFeedbackSeverity severity;
    std::string message; // UTF-8 Chinese, ready for NSString stringWithUTF8String.
};

struct CityMonthPreview {
    bool known = false; // Unknown rule => no fabricated production, migration, or treasury forecast.
    int month = 0;
    int64_t harvest = 0;
    int64_t consumption = 0;
    int64_t foodAfter = 0;
    int64_t migrationFoodGap = 0; // Extra pre-consumption stock/harvest needed to leave >10.
    int famineLoss = 0;
    int housingLoss = 0;
    int incoming = 0;
    int populationAfter = 0;
    int64_t tax = 0;
    int64_t upkeep = 0;
    int64_t balance = 0;
    int64_t treasuryAfter = 0;
};

struct CityGoalFeedback {
    bool valid = false;
    CityGoalState state = CityGoalState::Unknown;
    CityGoalTransition transition = CityGoalTransition::None;
    CityGoalMemory nextMemory;
    std::vector<CityGoalProgress> goals;
    std::vector<CityFeedbackIssue> issues;
    int64_t availableWorkers = 0;
    int64_t neededWorkers = 0;
    int64_t currentMonthlyBalance = 0;
    CityMonthPreview nextMonth;
    std::string status;
    std::string progressText;
    std::string previewText;
    std::string rulesNote = "自写关卡配置（可替换）；未与原版规则对比。";
};

namespace city_feedback_detail {
inline int64_t upkeep(const CityFeedbackSnapshot& s) {
    return int64_t(s.temples) * 3 + s.farms + s.wells + int64_t(s.markets) * 2 + s.watchtowers;
}

inline bool validSnapshot(const CityFeedbackSnapshot& s) {
    constexpr int64_t tiles = 128 * 128;
    const int counts[] = {s.roads, s.houses, s.farms, s.wells, s.temples, s.markets, s.watchtowers};
    int64_t total = 0;
    for (int count : counts) {
        if (count < 0 || count > tiles) return false;
        total += count;
    }
    return total <= tiles && s.population >= 0 && s.foodSupply >= 0 &&
           s.housingCapacity >= int64_t(s.houses) * 4 &&
           s.housingCapacity <= int64_t(s.houses) * 48 &&
           s.population <= s.housingCapacity && s.month >= 1 && s.month <= 12 &&
           s.speed >= 0 && s.speed <= 3;
}

inline bool validGoals(const CityGoalConfig& g) {
    return g.population > 0 && g.population <= 128 * 128 * 48 &&
           g.foodReserve >= 0 && g.treasuryReserve >= 0 && g.monthlyBalance >= 0;
}

inline int64_t boundedInt(int64_t value) {
    return std::clamp(value, int64_t(std::numeric_limits<int>::min()),
                      int64_t(std::numeric_limits<int>::max()));
}

inline CityMonthPreview preview(const CityFeedbackSnapshot& s, int64_t workers, int64_t jobs) {
    CityMonthPreview p;
    p.month = s.month == 12 ? 1 : s.month + 1; // advanceMonth increments before harvesting.
    p.upkeep = upkeep(s);
    p.consumption = (int64_t(s.population) + 9) / 10;
    if (s.harvestRule != CityHarvestRule::BaselineBb00e6b &&
        s.harvestRule != CityHarvestRule::LaborLimitedIntegerV1 &&
        s.harvestRule != CityHarvestRule::StaffedServicesV1) return p;
    if (s.harvestRule == CityHarvestRule::StaffedServicesV1 && (!s.coreHarvest || !s.coreNextMonth)) return p;
    const bool autumn = p.month >= 8 && p.month <= 10;
    if (s.harvestRule == CityHarvestRule::LaborLimitedIntegerV1 || s.harvestRule == CityHarvestRule::StaffedServicesV1) {
        // Exact match for frozen core farmYieldForMonth: no double intermediate or per-farm truncation.
        if (s.farms > 0 && jobs > 0 && workers > 0) {
            const int64_t filled = std::min(workers, jobs);
            p.harvest = int64_t(s.farms) * 5 * filled / jobs;
            if (autumn) p.harvest += int64_t(s.farms) * 30 * filled / jobs;
        }
    } else {
        p.harvest = int64_t(s.farms) * 5;
        const int percent = jobs == 0 ? 100 : std::clamp(int(double(workers) / double(jobs) * 100.0), 0, 100);
        if (autumn) p.harvest += int64_t(s.farms * 30 * (std::max(30, percent) / 100.0));
    }
    if (s.coreHarvest) {
        // Never silently use a conflicting helper or fall back to our formula on a version mismatch.
        if (*s.coreHarvest != p.harvest) {
            p.harvest = 0;
            return p;
        }
        p.harvest = *s.coreHarvest;
    }
    const int64_t stored = boundedInt(int64_t(s.foodSupply) + p.harvest);
    p.migrationFoodGap = std::max(int64_t(0), p.consumption + 11 - stored);
    if (stored < p.consumption) {
        p.famineLoss = std::min(s.population, 4);
        p.foodAfter = 0;
    } else {
        p.foodAfter = stored - p.consumption;
    }
    p.populationAfter = s.population - p.famineLoss;
    if (p.populationAfter < s.housingCapacity && p.foodAfter > 10) {
        p.incoming = std::min(s.housingCapacity - p.populationAfter, 6);
        p.populationAfter += p.incoming;
    }
    p.tax = p.populationAfter;
    p.balance = p.tax - p.upkeep;
    p.treasuryAfter = boundedInt(int64_t(s.treasury) + p.balance);
    if (s.harvestRule == CityHarvestRule::StaffedServicesV1) {
        const auto& next = *s.coreNextMonth;
        if (next.month != p.month || next.population < 0 || next.housingCapacity < 0 ||
            next.population > next.housingCapacity || next.housingCapacity > int64_t(s.houses) * 48 ||
            next.foodSupply != p.foodAfter ||
            next.treasury != boundedInt(int64_t(s.treasury) + next.population - p.upkeep)) return p;
        const int afterFamine = s.population - p.famineLoss;
        p.populationAfter = next.population;
        p.housingLoss = std::max(0, afterFamine - next.population);
        p.incoming = std::max(0, next.population - afterFamine);
        if (p.incoming > 6 || (p.foodAfter <= 10 && p.incoming > 0)) return p;
        p.tax = next.population;
        p.balance = p.tax - p.upkeep;
        p.treasuryAfter = next.treasury;
    }
    p.known = true;
    return p;
}
} // namespace city_feedback_detail

inline CityGoalFeedback evaluateCityGoals(const CityFeedbackSnapshot& s,
                                          const CityGoalConfig& config = {},
                                          const CityGoalMemory& memory = {}) {
    CityGoalFeedback out;
    out.nextMemory = memory; // Invalid input must not erase the caller's prior attainment.
    const auto issue = [&](CityFeedbackCode code, CityFeedbackSeverity severity, std::string text) {
        out.issues.push_back({code, severity, std::move(text)});
    };
    if (!city_feedback_detail::validSnapshot(s)) {
        issue(CityFeedbackCode::InvalidSnapshot, CityFeedbackSeverity::Unknown,
              "unknown：城市快照无效或汇总不一致，不能判断目标与经营原因。请重新读取模型快照。");
    }
    if (!city_feedback_detail::validGoals(config)) {
        issue(CityFeedbackCode::InvalidGoals, CityFeedbackSeverity::Unknown,
              "unknown：自写关卡目标无效；人口目标须在 1–786432，其他目标须非负。请替换关卡配置。");
    }
    if (memory.metLastEvaluation && !memory.everMet) {
        issue(CityFeedbackCode::InvalidHistory, CityFeedbackSeverity::Unknown,
              "unknown：目标历史不一致。新城、载入或切换关卡后请重置 UI 目标历史。");
    }
    if (!out.issues.empty()) {
        out.status = "目标状态未知：输入无效";
        return out;
    }
    out.valid = true;
    out.availableWorkers = s.population / 2;
    out.neededWorkers = int64_t(s.farms) * 8 + int64_t(s.wells) * 2 +
                        int64_t(s.temples) * 4 + int64_t(s.markets) * 4 + int64_t(s.watchtowers) * 2;
    const int64_t upkeep = city_feedback_detail::upkeep(s);
    out.currentMonthlyBalance = int64_t(s.population) - upkeep;
    out.nextMonth = city_feedback_detail::preview(s, out.availableWorkers, out.neededWorkers);
    const auto goal = [&](const char* label, int64_t current, int64_t target) {
        out.goals.push_back({label, current, target, std::max(int64_t(0), target - current), current >= target});
    };
    goal("人口（人）", s.population, config.population);
    goal("存粮（担）", s.foodSupply, config.foodReserve);
    goal("国库（贯）", s.treasury, config.treasuryReserve);
    goal("当前人口月度收支（贯）", out.currentMonthlyBalance, config.monthlyBalance);
    const bool met = std::all_of(out.goals.begin(), out.goals.end(), [](const auto& g) { return g.met; });
    out.nextMemory = {memory.everMet || met, met};
    if (met) {
        out.state = CityGoalState::Met;
        out.transition = memory.metLastEvaluation ? CityGoalTransition::None :
                         (memory.everMet ? CityGoalTransition::Restored : CityGoalTransition::Reached);
        out.status = out.transition == CityGoalTransition::Restored ? "目标已恢复：四项目标重新达成" : "目标已达成：四项目标均满足";
    } else {
        out.state = memory.everMet ? CityGoalState::Regressed : CityGoalState::InProgress;
        out.transition = memory.metLastEvaluation ? CityGoalTransition::Lost : CityGoalTransition::None;
        out.status = memory.everMet ? "目标失守，可恢复：补足下列差额即可重新达成" : "目标进行中：补足下列差额";
    }
    for (const auto& g : out.goals) {
        if (!out.progressText.empty()) out.progressText += " · ";
        out.progressText += g.label + " " + std::to_string(g.current) + "/" + std::to_string(g.target);
        out.progressText += g.met ? "（已达成）" : "（差 " + std::to_string(g.remaining) + "）";
    }

    if (s.speed == 0) {
        issue(CityFeedbackCode::Paused, CityFeedbackSeverity::Info,
              "自动月份推进已暂停。恢复速度或手动推进一月，才会收成、扣粮、迁入与结算税收。");
    }
    if (s.houses == 0) {
        issue(CityFeedbackCode::NoHousing, CityFeedbackSeverity::Warning,
              "尚无民居，住房容量为 0，无法迁入。先建民居；邻近道路与服务可提升住房容量。");
    } else if (s.population == s.housingCapacity) {
        issue(CityFeedbackCode::HousingFull, CityFeedbackSeverity::Warning,
              "住房已满（" + std::to_string(s.population) + "/" + std::to_string(s.housingCapacity) +
              "），没有迁入空间。新建民居或改善现有民居的邻近服务。");
    }
    if (s.housingCapacity < config.population) {
        issue(CityFeedbackCode::HousingBelowGoal, CityFeedbackSeverity::Info,
              "住房容量距离人口目标还差 " + std::to_string(config.population - s.housingCapacity) + " 人。先扩容，再等待按月迁入。");
    }
    if (s.houses > 0 && (s.population == s.housingCapacity || s.housingCapacity < config.population)) {
        issue(CityFeedbackCode::HousingCoverageUnknown, CityFeedbackSeverity::Unknown,
              "unknown：仅有全城汇总，无法判断具体民居缺少哪项邻近服务。请勘查住宅和周边；不能由井、道路或市肆总数推断覆盖。");
    }
    if (s.farms == 0) {
        issue(CityFeedbackCode::NoFarms, CityFeedbackSeverity::Warning,
              "没有农庄，收成为 0。有居民时会继续扣粮；营建农庄可生产粮食，每座农庄需要 8 个岗位。");
    }
    if (s.farms > 0 && out.availableWorkers < out.neededWorkers && out.nextMonth.known) {
        if (s.harvestRule == CityHarvestRule::LaborLimitedIntegerV1 || s.harvestRule == CityHarvestRule::StaffedServicesV1) {
            issue(CityFeedbackCode::LaborShortage, CityFeedbackSeverity::Warning,
                  "农庄缺工：全城劳动力 " + std::to_string(out.availableWorkers) + "/" + std::to_string(out.neededWorkers) +
                  "，差 " + std::to_string(out.neededWorkers - out.availableWorkers) +
                  " 个工人。常规与秋收都按全城岗位满足比例折减；扩大人口或减少非必要岗位，继续加农庄会增加缺口。");
        } else if (s.harvestRule == CityHarvestRule::BaselineBb00e6b) {
            issue(CityFeedbackCode::LaborShortage, CityFeedbackSeverity::Info,
                  "劳动力不足。本关收成规则仅折减秋收（最低 30%），常规每庄 5 担不折减；补充人口可改善秋收产量。");
        }
    }
    const auto& p = out.nextMonth;
    if (s.harvestRule == CityHarvestRule::StaffedServicesV1 && p.known &&
        (s.wells + s.markets + s.temples + s.watchtowers > 0) && out.availableWorkers < out.neededWorkers) {
        issue(CityFeedbackCode::ServiceLaborShortage, CityFeedbackSeverity::Warning,
              "服务缺工：井/市场/宗庙有效范围按岗位比例缩短，更楼需满岗。勘查住宅的有效邻近服务；增加人口或减少非必要岗位可恢复范围。");
    }
    if (p.known && p.housingLoss > 0) {
        issue(CityFeedbackCode::HousingDisplacement, CityFeedbackSeverity::Critical,
              "下一月劳动力下降将缩短服务范围，住房降级另流失 " + std::to_string(p.housingLoss) + " 人。请先补粮或减少非必要岗位。");
    }
    if (!p.known) {
        issue(CityFeedbackCode::UnknownHarvestRule, CityFeedbackSeverity::Unknown,
              "unknown：收成规则未知/过期，或核心 helper 与所选规则不一致。下一月产量、粮荒、迁入和结余不作推断；请核实规则版本与接线。");
        out.previewText = "下一月经营预估 unknown（收成规则未知）；当前人口月度收支 " + std::to_string(out.currentMonthlyBalance) + " 贯。";
    } else {
        if (p.harvest < p.consumption) {
            issue(CityFeedbackCode::FoodDeficit, CityFeedbackSeverity::Warning,
                  "下一月预计收成 " + std::to_string(p.harvest) + " 担、现有人口耗粮 " + std::to_string(p.consumption) +
                  " 担，缺口 " + std::to_string(p.consumption - p.harvest) + " 担需动用存粮。先检查农庄与劳动力；秋收只在 8–10 月。");
        }
        if (p.famineLoss > 0) {
            issue(CityFeedbackCode::Famine, CityFeedbackSeverity::Critical,
                  "下一月粮食不足：存粮加收成仍不够消费，预计流失 " + std::to_string(p.famineLoss) +
                  " 人且繁荣下降 5（最低 0）。补足粮食产出/库存后，仍需住房空位且扣粮后存粮 >10 才能恢复迁入。");
        }
        if (p.foodAfter <= 10) {
            issue(CityFeedbackCode::MigrationFoodBlocked, CityFeedbackSeverity::Warning,
                  "下一月扣粮后预计仅余 " + std::to_string(p.foodAfter) +
                  " 担，不满足迁入条件（须 >10）。存粮与收成合计须再补 " + std::to_string(p.migrationFoodGap) +
                  " 担，才能满足消费并越过门槛；同时要有住房空位。");
        }
        out.previewText = "若不改建/拆除，推进至 " + std::to_string(p.month) + " 月时估算：收成 " + std::to_string(p.harvest) +
                          "、耗粮 " + std::to_string(p.consumption) + "、余粮 " + std::to_string(p.foodAfter) +
                          " 担；人口 " + std::to_string(p.populationAfter) + "（迁入 " + std::to_string(p.incoming) +
                          "，粮荒流失 " + std::to_string(p.famineLoss) + "）；税 " + std::to_string(p.tax) +
                          " − 维护 " + std::to_string(p.upkeep) + " = " + std::to_string(p.balance) + " 贯。";
    }
    if (out.currentMonthlyBalance < 0) {
        issue(CityFeedbackCode::MonthlyDeficit, CityFeedbackSeverity::Warning,
              "按当前人口估算每月亏损 " + std::to_string(-out.currentMonthlyBalance) +
              " 贯：人口税 " + std::to_string(s.population) + " < 建筑维护 " + std::to_string(upkeep) +
              "。税基随人口增长；减少非必要维护也可改善收支。建造和拆除扣款不属于月度维护。");
    }
    if (s.treasury < 0) {
        issue(CityFeedbackCode::TreasuryDebt, CityFeedbackSeverity::Critical,
              "国库已负债 " + std::to_string(-int64_t(s.treasury)) +
              " 贯。恢复税收盈余可偿还；余额不足时无法建造或拆除。破产/强制终局判定尚未实现。");
    }
    if (s.treasury < 15) {
        issue(CityFeedbackCode::ConstructionFundsLow, CityFeedbackSeverity::Warning,
              "国库不足以建 15 贯民居（还差 " + std::to_string(15 - int64_t(s.treasury)) +
              " 贯）。农庄需 40 贯、道路 5 贯、拆除 2 贯；请按实际余额安排，月度税收是否能恢复资金见收支预估。");
    }
    return out;
}

} // namespace game_lab
