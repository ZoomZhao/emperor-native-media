#pragma once
#include "city_goal_feedback.hpp"
#include <array>

namespace game_lab {

// Optional self-authored challenges; no original-game scenario or save compatibility claim.
struct CityScenarioClock { int year = -1046; int month = 1; };
struct CityScenarioSpec {
    uint32_t id = 0;
    uint32_t revision = 1;
    CityGoalConfig goals;
    uint32_t deadlineMonths = 12; // Simulated months, not wall time/ticks. Deadline month is inclusive.
};
struct CityScenarioDefinition {
    CityScenarioSpec spec;
    std::string title;
    std::string instructions;
};
enum class CityScenarioStatus : uint8_t { Running = 0, Won = 1, Lost = 2 };
struct CityScenarioCheckpoint {
    CityScenarioSpec spec;
    uint64_t runId = 0; // Caller allocates a unique nonzero run ID and preserves it across loads.
    int64_t startMonth = 0;
    int64_t lastObservedMonth = 0;
    int64_t settledMonth = 0;
    CityScenarioStatus status = CityScenarioStatus::Running;
    bool awardRecorded = false;
};
struct CityScenarioAwardBook {
    // Persistent profile history. Merge with the live book on load, never replace it with an older save.
    // Awards are completion badges only; this module gives no treasury, food, or other simulation reward.
    static constexpr size_t kMaxEntries = 4096;
    std::vector<uint64_t> completedRuns;
};
struct CityScenarioAward { uint64_t runId = 0; uint32_t scenarioId = 0; };
struct CityScenarioUpdate {
    bool ok = false;
    bool active = false;
    CityScenarioStatus status = CityScenarioStatus::Running;
    bool becameWon = false;
    bool becameLost = false;
    int64_t elapsedMonths = 0;
    int64_t remainingMonths = 0;
    std::optional<CityScenarioAward> award;
    std::string message;
};

namespace city_scenario_detail {
inline bool monthNumber(CityScenarioClock clock, int64_t& out) {
    if (clock.month < 1 || clock.month > 12) return false;
    out = int64_t(clock.year) * 12 + clock.month - 1;
    return true;
}
inline bool validSpec(const CityScenarioSpec& spec) {
    // Reuse the published goal validator; no economy/production formula is repeated here.
    return spec.id != 0 && spec.revision != 0 && spec.deadlineMonths >= 1 &&
           spec.deadlineMonths <= 1200 && evaluateCityGoals({}, spec.goals).valid;
}
inline bool validBook(const CityScenarioAwardBook& book) {
    if (book.completedRuns.size() > CityScenarioAwardBook::kMaxEntries) return false;
    for (size_t i = 0; i < book.completedRuns.size(); ++i) {
        if (book.completedRuns[i] == 0 || std::find(book.completedRuns.begin(), book.completedRuns.begin() + i,
                                                 book.completedRuns[i]) != book.completedRuns.begin() + i) return false;
    }
    return true;
}
inline bool contains(const CityScenarioAwardBook& book, uint64_t run) {
    return std::find(book.completedRuns.begin(), book.completedRuns.end(), run) != book.completedRuns.end();
}
inline bool validCheckpoint(const CityScenarioCheckpoint& c) {
    constexpr int64_t first = int64_t(std::numeric_limits<int>::min()) * 12;
    constexpr int64_t last = int64_t(std::numeric_limits<int>::max()) * 12 + 11;
    if (!validSpec(c.spec) || c.runId == 0 || c.startMonth < first || c.startMonth > last ||
        c.startMonth + c.spec.deadlineMonths > last || c.lastObservedMonth < c.startMonth ||
        c.lastObservedMonth > last) return false;
    const int64_t deadline = c.startMonth + c.spec.deadlineMonths;
    switch (c.status) {
        case CityScenarioStatus::Running:
            return c.lastObservedMonth < deadline && c.settledMonth == 0 && !c.awardRecorded;
        case CityScenarioStatus::Won:
            return c.settledMonth >= c.startMonth && c.settledMonth <= deadline &&
                   c.settledMonth <= c.lastObservedMonth;
        case CityScenarioStatus::Lost:
            return c.settledMonth == deadline && c.lastObservedMonth >= deadline && !c.awardRecorded;
        default: return false;
    }
}
inline bool validGoalReport(const CityGoalFeedback& feedback, const CityGoalConfig& config) {
    if (!feedback.valid || feedback.goals.size() < 4) return false;
    const int targets[] = {config.population, config.foodReserve, config.treasuryReserve, config.monthlyBalance};
    for (size_t i = 0; i < 4; ++i) {
        const auto& goal = feedback.goals[i];
        if (goal.current < std::numeric_limits<int>::min() || goal.current > std::numeric_limits<int>::max()) return false;
        if (goal.target != targets[i] || goal.met != (goal.current >= goal.target) ||
            goal.remaining != std::max(int64_t(0), goal.target - goal.current)) return false;
        if (i < 2 && goal.current < 0) return false;
    }
    return true;
}
} // namespace city_scenario_detail

inline bool mergeCityScenarioAwards(CityScenarioAwardBook& live, const CityScenarioAwardBook& saved,
                                   std::string& error) {
    error.clear();
    if (!city_scenario_detail::validBook(live) || !city_scenario_detail::validBook(saved)) {
        error = "unknown：通关徽记账本无效；不能覆盖或重置历史。";
        return false;
    }
    auto candidate = live;
    for (uint64_t run : saved.completedRuns) {
        if (!city_scenario_detail::contains(candidate, run)) {
            if (candidate.completedRuns.size() == CityScenarioAwardBook::kMaxEntries) {
                error = "通关徽记账本已满；保留原账本。";
                return false;
            }
            candidate.completedRuns.push_back(run);
        }
    }
    live = std::move(candidate);
    return true;
}

class CityScenarioSession {
public:
    bool active() const { return active_; }
    const CityScenarioCheckpoint& checkpoint() const { return checkpoint_; }
    void disable() { active_ = false; } // Explicit free-build selection; award history is external and retained.

    CityScenarioUpdate begin(const CityScenarioSpec& spec, uint64_t runId, CityScenarioClock clock,
                             const CityScenarioAwardBook& book) {
        int64_t start = 0;
        CityScenarioCheckpoint candidate;
        candidate.spec = spec;
        candidate.runId = runId;
        if (!city_scenario_detail::monthNumber(clock, start) || !city_scenario_detail::validBook(book) ||
            city_scenario_detail::contains(book, runId)) return reject("无法开始：日期/徽记账本无效，或 runId 已通关。请为新尝试分配新的 runId。");
        candidate.startMonth = candidate.lastObservedMonth = start;
        if (!city_scenario_detail::validCheckpoint(candidate)) return reject("无法开始：自写关卡配置或期限无效。期限范围为 1–1200 个模拟月。");
        checkpoint_ = candidate;
        active_ = true;
        return describe(true, "自写关卡已开始：请在期限内同时满足四项目标。");
    }

    CityScenarioUpdate restore(const CityScenarioCheckpoint& saved, CityScenarioClock loadedClock,
                               CityScenarioAwardBook& book) {
        int64_t loaded = 0;
        if (!city_scenario_detail::validCheckpoint(saved) || !city_scenario_detail::validBook(book) ||
            !city_scenario_detail::monthNumber(loadedClock, loaded) || loaded != saved.lastObservedMonth)
            return reject("unknown：关卡账本无效或与城市年月不匹配；保留旧会话，不重新开始期限。");
        // An issued badge may be absent from an older profile. Repair its receipt without issuing an award event.
        auto candidateBook = book;
        if (saved.awardRecorded && !city_scenario_detail::contains(candidateBook, saved.runId)) {
            if (candidateBook.completedRuns.size() == CityScenarioAwardBook::kMaxEntries)
                return reject("通关徽记账本已满；不能恢复已结算关卡。");
            candidateBook.completedRuns.push_back(saved.runId);
        }
        checkpoint_ = saved;
        active_ = true;
        book = std::move(candidateBook);
        return describe(true, "已恢复自写关卡原期限与结算记录。");
    }

    CityScenarioUpdate observe(CityScenarioClock clock, const CityGoalFeedback& feedback,
                               CityScenarioAwardBook& book) {
        if (!active_) return describe(true, "自由营建：未选择限期关卡。");
        int64_t now = 0;
        if (!city_scenario_detail::validBook(book) || !city_scenario_detail::monthNumber(clock, now) ||
            now < checkpoint_.lastObservedMonth) return reject("unknown：日期倒退或徽记账本无效；请通过 restore 恢复配套存档。");
        // Every active observation must be valid before advancing its paired save timestamp.
        // Terminal states skip new outcome decisions, not input validation.
        if (!city_scenario_detail::validGoalReport(feedback, checkpoint_.spec.goals))
            return reject("unknown：目标反馈无效或未使用本关配置；不能推断胜负。");
        auto candidate = checkpoint_;
        auto candidateBook = book;
        candidate.lastObservedMonth = now;
        bool won = false, lost = false;
        if (candidate.status == CityScenarioStatus::Running) {
            const int64_t deadline = candidate.startMonth + candidate.spec.deadlineMonths;
            // Published first-four metric schema: population, stock, treasury, current monthly balance.
            // Additional read-only progress rows must not silently become new challenge conditions.
            const bool met = std::all_of(feedback.goals.begin(), feedback.goals.begin() + 4, [](const auto& goal) { return goal.met; });
            if (now <= deadline && met) {
                candidate.status = CityScenarioStatus::Won;
                candidate.settledMonth = now;
                won = true;
            } else if (now >= deadline) {
                candidate.status = CityScenarioStatus::Lost;
                candidate.settledMonth = deadline;
                lost = true;
            }
        }
        std::optional<CityScenarioAward> award;
        if (candidate.status == CityScenarioStatus::Won && !candidate.awardRecorded) {
            if (!city_scenario_detail::contains(candidateBook, candidate.runId)) {
                if (candidateBook.completedRuns.size() == CityScenarioAwardBook::kMaxEntries)
                    return reject("通关徽记账本已满；本次结算未提交，请保留会话并处理账本。");
                candidateBook.completedRuns.push_back(candidate.runId);
                award = CityScenarioAward{candidate.runId, candidate.spec.id};
            }
            candidate.awardRecorded = true;
        }
        if (!city_scenario_detail::validCheckpoint(candidate)) return reject("unknown：关卡更新产生无效账本；未提交更新。");
        checkpoint_ = candidate;
        book = std::move(candidateBook);
        auto result = describe(true, candidate.status == CityScenarioStatus::Won ? "胜利：自写关卡目标已按期达成。" :
                                     candidate.status == CityScenarioStatus::Lost ? "失败：到期未达成。可继续自由营建或另开一次挑战。" :
                                     "关卡进行中：按城市月份计算剩余期限。");
        result.becameWon = won;
        result.becameLost = lost;
        result.award = award;
        return result;
    }

    CityScenarioUpdate observeSnapshot(CityScenarioClock clock, const CityFeedbackSnapshot& snapshot,
                                       CityScenarioAwardBook& book) {
        if (clock.month != snapshot.month) return reject("unknown：时钟与目标快照月份不一致。");
        return observe(clock, evaluateCityGoals(snapshot, checkpoint_.spec.goals), book);
    }

private:
    bool active_ = false;
    CityScenarioCheckpoint checkpoint_;
    CityScenarioUpdate describe(bool ok, std::string text) const {
        CityScenarioUpdate result;
        result.ok = ok;
        result.active = active_;
        result.status = checkpoint_.status;
        result.message = std::move(text);
        if (active_) {
            const int64_t month = checkpoint_.status == CityScenarioStatus::Running ? checkpoint_.lastObservedMonth : checkpoint_.settledMonth;
            result.elapsedMonths = month - checkpoint_.startMonth;
            result.remainingMonths = std::max(int64_t(0), int64_t(checkpoint_.spec.deadlineMonths) - result.elapsedMonths);
        }
        return result;
    }
    CityScenarioUpdate reject(std::string text) const { return describe(false, std::move(text)); }
};

inline const std::array<CityScenarioDefinition, 3>& selfAuthoredCityScenarios() {
    static const std::array<CityScenarioDefinition, 3> definitions{{
        {{1, 1, {24, 200, 4000, 0}, 8}, "聚落初成", "8 个模拟月内达到人口24、存粮200、国库4000、月度收支不亏。空城可先建6座民居，逐月迎接迁入。"},
        {{2, 1, {40, 200, 4000, 0}, 12}, "井畔新村", "12 个模拟月内达到人口40、存粮200、国库4000、月度收支不亏。尝试2座民居，邻近道路与水井扩大容量。"},
        {{3, 1, {48, 450, 3500, 0}, 14}, "秋粮储备", "14个月内达到人口48、粮450、国库3500、收支不亏。先建两宅、路、井，人口到40再加市、庙和四农庄，避免起步缺工。"}
    }};
    return definitions;
}

} // namespace game_lab
