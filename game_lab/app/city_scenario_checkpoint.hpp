#pragma once
#include "city_scenario.hpp"

namespace game_lab {
// Standalone metadata only. Does not change EMP1 or perform file I/O.
constexpr size_t kCityScenarioCheckpointBytes = 72;

namespace city_scenario_codec_detail {
inline void put(std::vector<uint8_t>& bytes, uint64_t value, unsigned count) {
    for (unsigned i = 0; i < count; ++i) bytes.push_back(uint8_t(value >> (i * 8)));
}
inline uint64_t get(const std::vector<uint8_t>& bytes, size_t& offset, unsigned count) {
    uint64_t value = 0;
    for (unsigned i = 0; i < count; ++i) value |= uint64_t(bytes[offset++]) << (i * 8);
    return value;
}
inline int64_t signed64(uint64_t bits) {
    return bits <= uint64_t(std::numeric_limits<int64_t>::max()) ? int64_t(bits) : -1 - int64_t(~bits);
}
inline int signed32(uint64_t bits) {
    return bits <= 0x7fffffffu ? int(bits) : int(int64_t(bits) - 0x100000000LL);
}
} // namespace city_scenario_codec_detail

inline std::vector<uint8_t> encodeCityScenarioCheckpoint(const CityScenarioCheckpoint& c, std::string& error) {
    error.clear();
    if (!city_scenario_detail::validCheckpoint(c)) {
        error = "unknown：关卡账本无效，未生成存档元数据。";
        return {};
    }
    std::vector<uint8_t> bytes{'E','C','S','1'};
    bytes.reserve(kCityScenarioCheckpointBytes);
    const auto put = [&](uint64_t value, unsigned count) { city_scenario_codec_detail::put(bytes, value, count); };
    put(1, 4); // Metadata version, separate from the scenario definition revision.
    put(c.spec.id, 4); put(c.spec.revision, 4); put(c.runId, 8);
    put(uint32_t(c.spec.goals.population), 4); put(uint32_t(c.spec.goals.foodReserve), 4);
    put(uint32_t(c.spec.goals.treasuryReserve), 4); put(uint32_t(c.spec.goals.monthlyBalance), 4);
    put(c.spec.deadlineMonths, 4);
    put(uint64_t(c.startMonth), 8); put(uint64_t(c.lastObservedMonth), 8); put(uint64_t(c.settledMonth), 8);
    put(uint8_t(c.status), 1); put(c.awardRecorded ? 1 : 0, 1); put(0, 2);
    return bytes;
}

inline bool decodeCityScenarioCheckpoint(const std::vector<uint8_t>& bytes, CityScenarioCheckpoint& out,
                                        std::string& error) {
    error.clear();
    if (bytes.size() != kCityScenarioCheckpointBytes || bytes[0] != 'E' || bytes[1] != 'C' ||
        bytes[2] != 'S' || bytes[3] != '1') {
        error = "unknown：缺少或损坏 ECS1 关卡账本，不能恢复限期关卡；不会重新开始期限。";
        return false;
    }
    size_t offset = 4;
    const auto get = [&](unsigned count) { return city_scenario_codec_detail::get(bytes, offset, count); };
    if (get(4) != 1) { error = "unknown：不支持的关卡账本版本。"; return false; }
    CityScenarioCheckpoint candidate;
    candidate.spec.id = uint32_t(get(4)); candidate.spec.revision = uint32_t(get(4)); candidate.runId = get(8);
    candidate.spec.goals.population = city_scenario_codec_detail::signed32(get(4));
    candidate.spec.goals.foodReserve = city_scenario_codec_detail::signed32(get(4));
    candidate.spec.goals.treasuryReserve = city_scenario_codec_detail::signed32(get(4));
    candidate.spec.goals.monthlyBalance = city_scenario_codec_detail::signed32(get(4));
    candidate.spec.deadlineMonths = uint32_t(get(4));
    candidate.startMonth = city_scenario_codec_detail::signed64(get(8));
    candidate.lastObservedMonth = city_scenario_codec_detail::signed64(get(8));
    candidate.settledMonth = city_scenario_codec_detail::signed64(get(8));
    candidate.status = CityScenarioStatus(get(1));
    const auto recorded = get(1);
    candidate.awardRecorded = recorded == 1;
    if (recorded > 1 || get(2) != 0 || !city_scenario_detail::validCheckpoint(candidate)) {
        error = "unknown：关卡配置、日期、状态或结算标记不合法；保留原会话。";
        return false;
    }
    out = candidate;
    return true;
}

inline std::vector<uint8_t> encodeCityScenarioAwards(const CityScenarioAwardBook& book, std::string& error) {
    error.clear();
    if (!city_scenario_detail::validBook(book)) { error = "通关徽记账本无效。"; return {}; }
    std::vector<uint8_t> bytes{'E','C','A','1'};
    bytes.reserve(12 + book.completedRuns.size() * 8);
    city_scenario_codec_detail::put(bytes, 1, 4);
    city_scenario_codec_detail::put(bytes, book.completedRuns.size(), 4);
    for (uint64_t run : book.completedRuns) city_scenario_codec_detail::put(bytes, run, 8);
    return bytes;
}

inline bool decodeCityScenarioAwards(const std::vector<uint8_t>& bytes, CityScenarioAwardBook& out,
                                    std::string& error) {
    error.clear();
    if (bytes.size() < 12 || bytes[0] != 'E' || bytes[1] != 'C' || bytes[2] != 'A' || bytes[3] != '1') {
        error = "缺少或损坏通关徽记账本。";
        return false;
    }
    size_t offset = 4;
    if (city_scenario_codec_detail::get(bytes, offset, 4) != 1) { error = "不支持的徽记账本版本。"; return false; }
    const auto count = city_scenario_codec_detail::get(bytes, offset, 4);
    if (count > CityScenarioAwardBook::kMaxEntries || bytes.size() != 12 + count * 8) {
        error = "徽记账本数量/长度不合法。";
        return false;
    }
    CityScenarioAwardBook candidate;
    for (uint64_t i = 0; i < count; ++i) candidate.completedRuns.push_back(city_scenario_codec_detail::get(bytes, offset, 8));
    if (!city_scenario_detail::validBook(candidate)) { error = "徽记账本含零值或重复 runId。"; return false; }
    out = std::move(candidate);
    return true;
}

inline bool resumeCityScenarioFromBytes(CityScenarioSession& session, const std::vector<uint8_t>& bytes,
                                       CityScenarioClock clock, CityScenarioAwardBook& book, std::string& error) {
    CityScenarioCheckpoint saved;
    if (!decodeCityScenarioCheckpoint(bytes, saved, error)) return false;
    const auto result = session.restore(saved, clock, book);
    error = result.ok ? "" : result.message;
    return result.ok;
}
} // namespace game_lab
