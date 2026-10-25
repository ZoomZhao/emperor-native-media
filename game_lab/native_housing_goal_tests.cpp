// SPDX-License-Identifier: MIT
#include "original_housing_goal.hpp"
#include "original_pak.hpp"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
using namespace game_lab;
namespace {
using E = NativeHousingEligibility;
using V = NativeHousingGoalVerdict;
using Error = NativeHousingGoalError;
using Rows = std::vector<NativeHousingResidence>;
size_t checks = 0;
void need(bool ok, const char* why) { ++checks; if (!ok) throw std::runtime_error(why); }
bool same(const NativeHousingGoalInterval& a, const NativeHousingGoalInterval& b) {
    return a.lower == b.lower && a.upper == b.upper && a.totalResidents == b.totalResidents && a.verdict == b.verdict;
}
NativeHousingGoalInterval interval(const Rows& rows, int64_t target) {
    const auto result = evaluateNativeHousingPopulation(rows, true, target);
    need(std::holds_alternative<NativeHousingGoalInterval>(result), "expected valid interval");
    return std::get<NativeHousingGoalInterval>(result);
}
void rejects(const Rows& rows, bool complete, int64_t target, Error expected) {
    const auto r = evaluateNativeHousingPopulation(rows, complete, target);
    need(std::holds_alternative<Error>(r) && std::get<Error>(r) == expected, "named input refusal");
}
void authored() {
    const Rows seventy(10, {7, E::Unknown});
    const auto a = interval(seventy, 150);
    need(a.lower == 0 && a.upper == 70 && a.totalResidents == 70 && a.verdict == V::DefinitelyNotMet,
         "70 proves unmet without claiming exact progress zero");
    need(interval({}, 1).verdict == V::DefinitelyNotMet, "complete empty inventory");
    need(interval({{150, E::Qualifies}}, 150).verdict == V::DefinitelyMet, "inclusive lower boundary");
    need(interval({{150, E::Unknown}}, 150).verdict == V::Undetermined, "upper equality does not prove unmet or met");
    need(interval({{149, E::Qualifies}, {1, E::Unknown}}, 150).verdict == V::Undetermined, "interval straddles threshold");
    need(interval({{150, E::Qualifies}, {100, E::Unknown}}, 150).verdict == V::DefinitelyMet, "known lower already sufficient");
    const auto excluded = interval({{900, E::DoesNotQualify}, {70, E::Unknown}}, 150);
    need(excluded.totalResidents == 970 && excluded.upper == 70, "unqualified residents not in upper bound");
    rejects(seventy, false, 150, Error::IncompleteInventory);
    rejects({}, false, 150, Error::IncompleteInventory);
    rejects({}, true, 0, Error::InvalidThreshold);
    rejects({}, true, -1, Error::InvalidThreshold);
    rejects({{-1, E::Unknown}}, true, 150, Error::NegativeResidents);
    rejects({{150, E::Qualifies}, {-1, E::Unknown}}, true, 150, Error::NegativeResidents);
    rejects({{0, static_cast<E>(255)}}, true, 150, Error::InvalidEligibility);
    const auto max = std::numeric_limits<int64_t>::max();
    for (const auto first : {E::Unknown, E::Qualifies, E::DoesNotQualify})
        rejects({{max, first}, {1, E::DoesNotQualify}}, true, 150, Error::PopulationOverflow);
    need(interval({{max, E::Qualifies}}, max).verdict == V::DefinitelyMet, "largest exact qualified count");
    need(interval({{max, E::Unknown}}, max).verdict == V::Undetermined, "largest exact unknown count");
    need(interval({{max, E::DoesNotQualify}}, max).verdict == V::DefinitelyNotMet, "largest unqualified count");
    need(interval({{max - 1, E::Qualifies}, {1, E::Unknown}}, max).upper == max, "exact summed arithmetic boundary");
    for (int i = 0; i < 3; ++i) need(same(interval(seventy, 150), a), "repeat evaluation has no retained state");
    need(std::all_of(seventy.begin(), seventy.end(), [](const auto& x) { return x.residents == 7 && x.eligibility == E::Unknown; }), "input unchanged");
    // Independent finite-completion oracle: enumerate every possible truth
    // assignment for unknown residences, rather than restating the sum rule.
    size_t oracleCases = 0;
    for (int encoding = 0; encoding < 1728; ++encoding) {
        Rows rows; int n = encoding;
        for (int k = 0; k < 3; ++k) { const int v = n % 12; n /= 12; rows.push_back({v / 3, E(v % 3)}); }
        int64_t minimum = max, maximum = 0;
        for (unsigned bits = 0; bits < 8; ++bits) {
            int64_t count = 0;
            for (unsigned i = 0; i < 3; ++i)
                if (rows[i].eligibility == E::Qualifies || (rows[i].eligibility == E::Unknown && (bits & (1u << i)))) count += rows[i].residents;
            minimum = std::min(minimum, count); maximum = std::max(maximum, count);
        }
        for (int threshold = 1; threshold <= 10; ++threshold) {
            const auto out = interval(rows, threshold);
            need(out.lower == minimum && out.upper == maximum, "interval equals exhaustive feasible range");
            const auto verdict = maximum < threshold ? V::DefinitelyNotMet : minimum >= threshold ? V::DefinitelyMet : V::Undetermined;
            need(out.verdict == verdict, "verdict valid for all completions");
            std::reverse(rows.begin(), rows.end());
            need(same(interval(rows, threshold), out), "order independent");
            std::reverse(rows.begin(), rows.end()); ++oracleCases;
        }
    }
    OriginalBuildingProfile p; p.status = OriginalBuildingStatus::Complete; p.parsedRecords = 4000;
    const auto empty = evaluateOriginalSavedHousingPopulation({}, p, 150);
    need(std::holds_alternative<Error>(empty) && std::get<Error>(empty) == Error::InvalidResidenceInventory, "complete flag without a bounded saved table is invalid");
    p.tableEnd = 1;
    p.liveTypeCounts[3] = 1;
    auto missing = evaluateOriginalSavedHousingPopulation({0}, p, 150);
    need(std::holds_alternative<Error>(missing) && std::get<Error>(missing) == Error::IncompleteInventory, "withheld house not silently empty");
    p.liveTypeCounts.clear(); p.status = OriginalBuildingStatus::UnsupportedTail;
    missing = evaluateOriginalSavedHousingPopulation({}, p, 150);
    need(std::holds_alternative<Error>(missing) && std::get<Error>(missing) == Error::IncompleteInventory, "partial table not complete");
    std::cout << "{\"authored_checks\":" << checks << ",\"exhaustive_cases\":" << oracleCases << ",\"passed\":true,\"original_data\":false}\n";
}
void real(int argc, char** argv) {
    const auto env = std::getenv("EMPEROR_GAMEDATA_ROOT");
    if (!env || !*env) { std::cout << "{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n"; return; }
    need(argc >= 4, "--real needs explicit threshold and relative save paths");
    size_t consumed = 0; const std::string argument = argv[2];
    const auto threshold = std::stoll(argument, &consumed);
    need(consumed == argument.size() && threshold > 0, "positive complete threshold argument");
    const auto root = ResourceRoot::fromAbsolute(env);
    for (int n = 3; n < argc; ++n) {
        const auto path = root.resolve(argv[n]); const auto save = loadOriginalSave(path);
        std::vector<uint8_t> raw; need(detail::readFile(path, 4194304, raw), "read real save");
        const auto d = decodeOriginalMapContainer(raw); need(d.complete, "decode real save");
        const auto original = d.payload;
        const auto r = evaluateOriginalSavedHousingPopulation(d.payload, save.buildings, threshold);
        need(std::holds_alternative<NativeHousingGoalInterval>(r), "complete real housing inventory");
        const auto out = std::get<NativeHousingGoalInterval>(r);
        need(out.lower == 0 && out.upper == out.totalResidents, "all real tiers remain unknown");
        const auto repeat = evaluateOriginalSavedHousingPopulation(d.payload, save.buildings, threshold);
        need(std::holds_alternative<NativeHousingGoalInterval>(repeat) && same(std::get<NativeHousingGoalInterval>(repeat), out) && d.payload == original, "real repeat pure");
        auto bad = save.buildings; bad.tableEnd = d.payload.size() + 1;
        const auto outside = evaluateOriginalSavedHousingPopulation(d.payload, bad, threshold);
        need(std::holds_alternative<Error>(outside) && std::get<Error>(outside) == Error::InvalidResidenceInventory, "real out-of-range inventory boundary refused");
        bad = save.buildings; ++bad.liveTypeCounts[3];
        const auto withheld = evaluateOriginalSavedHousingPopulation(d.payload, bad, threshold);
        need(std::holds_alternative<Error>(withheld) && std::get<Error>(withheld) == Error::IncompleteInventory, "real missing residence refused");
        auto house = std::find_if(save.buildings.footprints.begin(), save.buildings.footprints.end(), [](const auto& f) { return f.type >= 2 && f.type <= 17; });
        if (house != save.buildings.footprints.end()) {
            bad = save.buildings; bad.footprints.push_back(*house);
            const auto duplicate = evaluateOriginalSavedHousingPopulation(d.payload, bad, threshold);
            need(std::holds_alternative<Error>(duplicate), "real duplicate slot refused");
            bad = save.buildings; bad.slotTypes[house->slot] = 1;
            need(std::holds_alternative<Error>(evaluateOriginalSavedHousingPopulation(d.payload, bad, threshold)), "real type identity mismatch refused");
        }
        const char* verdict = out.verdict == V::DefinitelyNotMet ? "definitely_not_met" : out.verdict == V::DefinitelyMet ? "definitely_met" : "undetermined";
        std::cout << "{\"source\":" << std::quoted(argv[n]) << ",\"threshold\":" << threshold << ",\"lower\":" << out.lower << ",\"upper\":" << out.upper << ",\"residents\":" << out.totalResidents << ",\"verdict\":\"" << verdict << "\",\"tier_mapping\":\"unknown\",\"notification_emitted\":false}\n";
    }
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 1) authored();
        else if (std::string(argv[1]) == "--real") real(argc, argv);
        else throw std::runtime_error("unknown test mode");
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
